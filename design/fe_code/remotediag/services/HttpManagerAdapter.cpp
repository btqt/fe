#include "HttpManagerAdapter.h"
#include <common/v1/ProtoUtils.h>
#include <sstream>

#include "utils/ProxyIpcServer.h"

namespace rdgapp {

namespace {

std::vector<uint8_t> serializeGrpcReqData(const android::sp<GrpcReqData> &reqData) {
    std::vector<uint8_t> encoded{};
    if (reqData == nullptr) {
        return encoded;
    }

    android::Parcel parcel{};
    if (reqData->writeToParcel(&parcel) != android::OK) {
        LOG_E("HttpManagerAdapter: failed to serialize GrpcReqData");
        return encoded;
    }

    if (parcel.dataSize() > 0) {
        encoded.resize(parcel.dataSize());
        std::memcpy(encoded.data(), parcel.data(), parcel.dataSize());
    }
    return encoded;
}

bool parseIntListPayload(const std::string &value, std::vector<int32_t> &numbers) {
    numbers.clear();
    std::stringstream ss{value};
    std::string token{};
    while (std::getline(ss, token, ',')) {
        if (token.empty()) {
            return false;
        }
        char *endPtr{nullptr};
        const long parsed{std::strtol(token.c_str(), &endPtr, 10)};
        if ((endPtr == nullptr) || (*endPtr != '\0')) {
            return false;
        }
        numbers.push_back(static_cast<int32_t>(parsed));
    }
    return !numbers.empty();
}

} // namespace

// CallbackHandler implementation
HttpManagerAdapter::CallbackHandler::CallbackHandler(HttpManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("HttpManagerAdapter::CallbackHandler: created");
}

void HttpManagerAdapter::CallbackHandler::initialize() {
    // Register this handler for HTTP callbacks
    std::vector<uint32_t> callbackIds = {
        static_cast<uint32_t>(rdgipc::CallbackId::HttpGrpcConnState),
        static_cast<uint32_t>(rdgipc::CallbackId::HttpGrpcResponse)
    };
    
    rdgapp::ProxyIpcServer& server = rdgapp::ProxyIpcServer::getInstance();
    server.registerCallbackHandler(shared_from_this(), callbackIds);
    LOG_I("HttpManagerAdapter::CallbackHandler: registered %zu callbacks", callbackIds.size());
}

void HttpManagerAdapter::CallbackHandler::handle(uint32_t callbackId, const std::vector<uint8_t>& payload) {
    LOG_I("HttpManagerAdapter::CallbackHandler::handle id=%u payloadSize=%zu",
          callbackId, payload.size());

    if (callbackId == static_cast<uint32_t>(rdgipc::CallbackId::HttpGrpcConnState)) {
        const std::string payloadStr{std::string(payload.begin(), payload.end())};
        std::vector<int32_t> values{};
        if (!parseIntListPayload(payloadStr, values) || (values.size() != 2U)) {
            LOG_W("HttpManagerAdapter: invalid HTTP connection state callback payload");
            return;
        }
        mAdapter->onDataConnStateChange(static_cast<GRPC_APP_TYPE>(values[0]), values[1] != 0);
        return;
    }

    if (callbackId == static_cast<uint32_t>(rdgipc::CallbackId::HttpGrpcResponse)) {
        if (payload.empty()) {
            LOG_W("HttpManagerAdapter: empty HTTP gRPC response callback payload");
            return;
        }

        android::Parcel parcel{};
        parcel.setData(payload.data(), payload.size());
        android::sp<GrpcResData> resData{new GrpcResData()};
        if ((resData == nullptr) || (resData->readFromParcel(parcel) != android::OK)) {
            LOG_E("HttpManagerAdapter: failed to deserialize HTTP gRPC response");
            return;
        }
        mAdapter->onReceive(resData);
        return;
    }
}

HttpManagerAdapter::HttpManagerAdapter() {
    mCallbackHandler = std::make_shared<CallbackHandler>(this);
    mCallbackHandler->initialize();
    LOG_I("HttpManagerAdapter Constructor");
    connectionAvail = true;
    mProtoTextVer = 0x00U;
}

HttpManagerAdapter::~HttpManagerAdapter() {
    if(HttpManagerAdapter::instance != nullptr) {
        HttpManagerAdapter::instance = nullptr;
    }
    mHandler = nullptr;
}

std::shared_ptr<HttpManagerAdapter> HttpManagerAdapter::instance{nullptr};
android::Mutex HttpManagerAdapter::mInstanceLock{};
std::shared_ptr<HttpManagerAdapter> HttpManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<HttpManagerAdapter>();
        }
    }
    return instance;
}

void HttpManagerAdapter::registerService() {
    LOG_I("HttpManagerAdapter registerService");
    mHandler = RemotediagHandler::getInstance();
    mProtoTextVer = ProtoUtils::getProtoFileVer_Hex();
}

void HttpManagerAdapter::testTriggerReceive(const android::sp<GrpcResData>& pGrpcResData)
{
    if ((pGrpcResData->getAppType() == GRPC_APP_TYPE::RMT_DIAG) && (mHandler != nullptr))
    {
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RPC_MESSAGE_RECEIVED, pGrpcResData)->sendToTarget();
    } else {

    }
}

void HttpManagerAdapter::onReceive(const android::sp<GrpcResData> pGrpcResData) {
    // handleReceive(pGrpcResData);
    if(pGrpcResData != nullptr) 
    {
        const android::sp<GrpcResData> localGrpcResData {new GrpcResData()};
        localGrpcResData->setTo(*pGrpcResData);
        if ((localGrpcResData->getAppType() == GRPC_APP_TYPE::RMT_DIAG) && (mHandler != nullptr))
        {
            (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RPC_MESSAGE_RECEIVED, localGrpcResData)->sendToTarget();
        } else {

        }
    } else {
        LOG_E("Receive GrpcResData is empty");
    }
}

void HttpManagerAdapter::onDataConnStateChange(const GRPC_APP_TYPE pAppType, const bool pIsConnected)
{
    LOG_I("GRPC Communication change apptype = %d", static_cast<int32_t>(pAppType));
    if ((pAppType == GRPC_APP_TYPE::RMT_DIAG) && (mHandler != nullptr))
    {
        if(pIsConnected == true)
        {
            connectionAvail = true;
            (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_GRPC_COMMUNICATION_RECONNECT)->sendToTarget();
        } else
        {
            connectionAvail = false;
            (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_GRPC_COMMUNICATION_DISCONNECT)->sendToTarget();
        }
    }
}

bool HttpManagerAdapter::getConnectionAvail() {
    if(connectionAvail) {
        LOG_D("Check connectionAvail OK ");
    } else {
        LOG_D("Check connectionAvail NOT OK ");
    }
    return connectionAvail;
}

uint32_t HttpManagerAdapter::getProtoTextVersion() const noexcept {
    return mProtoTextVer;
}

int32_t HttpManagerAdapter::sendGrpcMessage(const android::sp<GrpcReqData>& pGrpcReqData)
{
    if (pGrpcReqData == nullptr) {
        LOG_E("HttpManagerAdapter::sendGrpcMessage pGrpcReqData is nullptr");
        return -1;
    }

    const std::vector<uint8_t> payload{serializeGrpcReqData(pGrpcReqData)};
    if (payload.empty()) {
        LOG_E("HttpManagerAdapter::sendGrpcMessage serialized payload is empty");
        return -1;
    }

    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::HttpSendGrpc,
                                                       payload,
                                                       response,
                                                       static_cast<uint32_t>(pGrpcReqData->getTimeoutSec() * 1000))) {
        LOG_E("HttpManagerAdapter::sendGrpcMessage request through proxy failed");
        return -1;
    }

    const std::string responseStr{rdgipc::toString(response)};
    try {
        const int32_t callId{std::stoi(responseStr)};
        LOG_D("HttpManagerAdapter::sendGrpcMessage success, callId=%d", callId);
        return callId;
    } catch (const std::exception &) {
        LOG_E("HttpManagerAdapter::sendGrpcMessage invalid response: %s", responseStr.c_str());
        return -1;
    }
}

}
