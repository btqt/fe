#include "HttpManagerAdapter.h"

#include <binder/Parcel.h>
#include <cstring>

#include "../utils/ProxyIpcClient.h"

std::shared_ptr<HttpManagerAdapter> HttpManagerAdapter::instance{nullptr};
android::Mutex HttpManagerAdapter::mInstanceLock{};

HttpManagerAdapter::HttpManagerAdapter() {
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who) {
        this->onBinderDied(who);
    });
    
    // Create and register IPC command handler
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
}

HttpManagerAdapter::~HttpManagerAdapter() noexcept {
    if (HttpManagerAdapter::instance != nullptr) {
        HttpManagerAdapter::instance = nullptr;
    }
}

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
    mHandler = RemotediagProxyHandler::getInstance();

    if (!registerServiceLocked()) {
        LOG_W("HttpManagerAdapter: register service failed, retry");
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(
                mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_HTTP_MGR),
                static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        } else {
            LOG_E("HttpManagerAdapter: mHandler is nullptr");
        }
    }
}

bool HttpManagerAdapter::registerServiceLocked() {
    android::sp<IHttpManagerService> service{getService()};
    if (service == nullptr) {
        LOG_W("HttpManagerAdapter: HttpManagerService not available");
        return false;
    }

    const android::status_t linkResult{android::IInterface::asBinder(service)->linkToDeath(mServiceDeathRecipient)};
    if (linkResult != android::OK) {
        LOG_E("HttpManagerAdapter: linkToDeath failed: %d", static_cast<int32_t>(linkResult));
        return false;
    }

    if (mGrpcReceiver == nullptr) {
        mGrpcReceiver = android::sp<IGRPCReceiver>{new HttpGrpcReceiver(*this)};
    }

    const error_t registerResult{service->registerReceiverGRPCReceive(mGrpcReceiver, GRPC_APP_TYPE::RMT_DIAG)};
    if (registerResult != E_OK) {
        LOG_E("HttpManagerAdapter: registerReceiverGRPCReceive failed: %d", static_cast<int32_t>(registerResult));
        return false;
    }

    {
        const android::Mutex::Autolock lock{mServiceLock};
        mHttpMgrService = service;
    }

    LOG_I("HttpManagerAdapter: registered successfully");
    return true;
}

int32_t HttpManagerAdapter::sendGrpcMessageRaw(const std::vector<uint8_t> &encodedReq) {
    android::sp<IHttpManagerService> service{nullptr};
    {
        const android::Mutex::Autolock lock{mServiceLock};
        service = mHttpMgrService;
    }

    if (service == nullptr) {
        LOG_W("HttpManagerAdapter: sendGrpcMessageRaw called while service unavailable");
        return -1;
    }

    android::Parcel parcel{};
    if (!encodedReq.empty()) {
        parcel.setData(encodedReq.data(), encodedReq.size());
    }

    android::sp<GrpcReqData> reqData{new GrpcReqData()};
    if ((reqData == nullptr) || (reqData->readFromParcel(parcel) != android::OK)) {
        LOG_E("HttpManagerAdapter: failed to deserialize GrpcReqData");
        return -1;
    }

    const int32_t callId{service->sendOverGRPC(reqData)};
    if (callId < 0) {
        LOG_E("HttpManagerAdapter: sendOverGRPC failed: %d", callId);
    } else {
        LOG_D("HttpManagerAdapter: sendOverGRPC success, callId=%d", callId);
    }
    return callId;
}

void HttpManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
    NOTUSED(who);
    LOG_I("HttpManagerAdapter::onBinderDied");
    {
        const android::Mutex::Autolock lock{mServiceLock};
        mHttpMgrService = nullptr;
        mGrpcReceiver = nullptr;
    }

    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(
            mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_HTTP_MGR),
            static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
    }
}

void HttpManagerAdapter::onReceive(const android::sp<GrpcResData> pGrpcResData) {
    if (pGrpcResData == nullptr) {
        LOG_E("HttpManagerAdapter: onReceive called with nullptr");
        return;
    }

    android::Parcel parcel{};
    if (pGrpcResData->writeToParcel(&parcel) != android::OK) {
        LOG_E("HttpManagerAdapter: failed to serialize GrpcResData");
        return;
    }

    std::vector<uint8_t> payload(parcel.dataSize());
    if (parcel.dataSize() > 0) {
        std::memcpy(payload.data(), parcel.data(), parcel.dataSize());
    }

    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::HttpGrpcResponse), payload);
}

void HttpManagerAdapter::onDataConnStateChange(const GRPC_APP_TYPE pAppType, const bool pIsConnected) {
    if (pAppType != GRPC_APP_TYPE::RMT_DIAG) {
        return;
    }

    const std::string str{std::to_string(static_cast<int32_t>(pAppType)) + "," + (pIsConnected ? "1" : "0")};
    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::HttpGrpcConnState), std::vector<uint8_t>(str.begin(), str.end()));
}

android::sp<IHttpManagerService> HttpManagerAdapter::getService() const {
    return android::interface_cast<IHttpManagerService>(
        android::defaultServiceManager()->getService(android::String16("service_layer.HttpManagerService")));
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

HttpManagerAdapter::CommandHandler::CommandHandler(HttpManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("HttpManagerAdapter::CommandHandler: created");
}

void HttpManagerAdapter::CommandHandler::initialize() {
    LOG_I("HttpManagerAdapter::CommandHandler: registering with ProxyIpcClient");
    
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::HttpSendGrpc)
    };
    
    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("HttpManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("HttpManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

HttpManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("HttpManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse HttpManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    LOG_I("HttpManagerAdapter::CommandHandler: handling command id=%u payloadSize=%zu",
          commandId, payload.size());
    
    if (commandId == static_cast<uint32_t>(rdgipc::CommandId::HttpSendGrpc)) {
        const int32_t callId = mAdapter->sendGrpcMessageRaw(payload);
        if (callId < 0) {
            return rdgipc::CommandResponse::err(rdgipc::ErrorCode::OperationFailed);
        }
        return rdgipc::CommandResponse::ok(callId);
    }
    
    return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
}
