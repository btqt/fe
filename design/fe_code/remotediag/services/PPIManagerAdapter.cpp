#include "PPIManagerAdapter.h"

#include <cstring>
#include <exception>

#include "utils/ProxyIpcServer.h"

#include "../remotediagproxy/include/ProxyIpcProtocol.h"

namespace rdgapp {

// CallbackHandler implementation
PPIManagerAdapter::CallbackHandler::CallbackHandler(PPIManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("PPIManagerAdapter::CallbackHandler: created");
}

void PPIManagerAdapter::CallbackHandler::initialize() {
    // Register this handler for PPI callbacks
    std::vector<uint32_t> callbackIds = {
        static_cast<uint32_t>(rdgipc::CallbackId::PpiStatusChanged)
    };
    
    rdgapp::ProxyIpcServer& server = rdgapp::ProxyIpcServer::getInstance();
    server.registerCallbackHandler(shared_from_this(), callbackIds);
    LOG_I("PPIManagerAdapter::CallbackHandler: registered %zu callbacks", callbackIds.size());
}

void PPIManagerAdapter::CallbackHandler::handle(uint32_t callbackId, const std::vector<uint8_t>& payload) {
    LOG_I("PPIManagerAdapter::CallbackHandler::handle id=%u payloadSize=%zu",
          callbackId, payload.size());

    if (callbackId == static_cast<uint32_t>(rdgipc::CallbackId::PpiStatusChanged)) {
        if (payload.empty()) {
            LOG_W("PPIManagerAdapter: invalid PPI callback payload size=%zu", payload.size());
            return;
        }

        android::sp<Buffer> spBuf{new Buffer()};
        spBuf->setTo(payload.data(), static_cast<int32_t>(payload.size()));
        mAdapter->onStatusChanged(spBuf);
        return;
    }
}

PPIManagerAdapter::PPIManagerAdapter() {
    mCallbackHandler = std::make_shared<CallbackHandler>(this);
    mCallbackHandler->initialize();
    LOG_I("constructor PPIManagerAdapter");
}

PPIManagerAdapter::~PPIManagerAdapter() noexcept {
    if(PPIManagerAdapter::instance != nullptr) {
        PPIManagerAdapter::instance = nullptr;
    } 
}

std::shared_ptr<PPIManagerAdapter> PPIManagerAdapter::instance{nullptr};
android::Mutex PPIManagerAdapter::mInstanceLock{};
std::shared_ptr<PPIManagerAdapter> PPIManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<PPIManagerAdapter>();
        }
    }
    return instance;
}

void PPIManagerAdapter::onStatusChanged(android::sp<::Buffer> &name) const
{
    LOG_I("PPIManagerAdapter::onStatusChanged");
    const android::sp<sl::Message> msg {mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_PPI_INFO_RECEIVED)};
    msg->buffer = *name;
    (void)msg->sendToTarget();
}

void PPIManagerAdapter::registerService(){
    LOG_I("Start register PPIManagerAdapter");
    mHandler = RemotediagHandler::getInstance();
}

uint32_t PPIManagerAdapter::receivePPIErase(const char_t* const buf){
    uint32_t PPIFlag {0U};
    if(strncmp(buf, "PPIFlag", strlen("PPIFlag")) == 0) {
        LOG_I("PPIManagerAdapter::receivePPIErase process");
        std::vector<uint8_t> response{};
        const std::vector<uint8_t> payload{rdgipc::toBytes("PPIFlag")};
        if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::PpiGetFlag,
                                                          payload,
                                                          response,
                                                          2000U)) {
            LOG_E("PPIManagerAdapter::receivePPIErase proxy request failed");
            return 0U;
        }

        const std::string strFlag{rdgipc::toString(response)};
        try
        {
            PPIFlag = static_cast<uint32_t>(std::stoul(strFlag));
        }
        catch (const std::exception &e)
        {
            LOG_E("Exception: %s", e.what());
        }
    }
    else{
        LOG_I("PPIManagerAdapter::receivePPIErase do not process");
    }
    return PPIFlag;
}

void PPIManagerAdapter::responsePPIErase(const uint32_t appType, const uint32_t appState)
{
    LOG_I("appType = %u, appState = %u", appType, appState);
    const std::string reqPayload{std::to_string(appType) + "," + std::to_string(appState)};
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::PpiResponseDelete,
                                                      rdgipc::toBytes(reqPayload),
                                                      response,
                                                      2000U)) {
        LOG_E("PPIManagerAdapter::responsePPIErase proxy request failed");
    }
}
}
