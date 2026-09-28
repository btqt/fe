#include "PPIManagerAdapter.h"

#include <cstring>
#include <exception>
#include <sstream>

#include "../utils/ProxyIpcClient.h"

std::shared_ptr<PPIManagerAdapter> PPIManagerAdapter::instance{nullptr};
android::Mutex PPIManagerAdapter::mInstanceLock{};

PPIManagerAdapter::PPIManagerAdapter() {
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who) {
        this->onBinderDied(who);
    });
    
    // Create and register IPC command handler
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
}

PPIManagerAdapter::~PPIManagerAdapter() noexcept {
    if (PPIManagerAdapter::instance != nullptr) {
        PPIManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<PPIManagerAdapter> PPIManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<PPIManagerAdapter>();
        }
    }
    return instance;
}

void PPIManagerAdapter::registerService() {
    LOG_I("PPIManagerAdapter registerService");
    mHandler = RemotediagProxyHandler::getInstance();

    if (!registerServiceLocked()) {
        LOG_W("PPIManagerAdapter: register service failed, retry");
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(
                mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_PPI_MGR),
                static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
    }
}

bool PPIManagerAdapter::registerServiceLocked() {
    android::sp<IPPIManagerService> service{getService()};
    if (service == nullptr) {
        LOG_W("PPIManagerAdapter: PPIManagerService not available");
        return false;
    }

    if (android::IInterface::asBinder(service)->linkToDeath(mServiceDeathRecipient) != android::OK) {
        LOG_E("PPIManagerAdapter: linkToDeath failed");
        return false;
    }

    if (mPPIStatusReceiver == nullptr) {
        mPPIStatusReceiver = android::sp<IPPIStatusReceiver>{new PPIMgrReceiver(*this)};
    }

    const error_t registerResult{service->registerReceiverPPIStatusReceiverOnStatusChanged(mPPIStatusReceiver)};
    if (registerResult != E_OK) {
        LOG_E("PPIManagerAdapter: registerReceiverPPIStatusReceiverOnStatusChanged failed: %d",
              static_cast<int32_t>(registerResult));
        return false;
    }

    {
        const android::Mutex::Autolock lock{mServiceLock};
        mPPIManagerService = service;
    }

    LOG_I("PPIManagerAdapter: registered successfully");
    return true;
}

uint32_t PPIManagerAdapter::receivePPIErase(const char_t *buf) {
    if (std::strncmp(buf, "PPIFlag", std::strlen("PPIFlag")) != 0) {
        return 0U;
    }

    android::sp<IPPIManagerService> service{nullptr};
    {
        const android::Mutex::Autolock lock{mServiceLock};
        service = mPPIManagerService;
    }

    if (service == nullptr) {
        LOG_W("PPIManagerAdapter::receivePPIErase service unavailable");
        return 0U;
    }

    try {
        return static_cast<uint32_t>(std::stoul(service->getPropertyPPIValue("PPIFlag")));
    } catch (const std::exception &e) {
        LOG_E("PPIManagerAdapter::receivePPIErase parse failed: %s", e.what());
        return 0U;
    }
}

void PPIManagerAdapter::responsePPIErase(const uint32_t appType, const uint32_t appState) {
    android::sp<IPPIManagerService> service{nullptr};
    {
        const android::Mutex::Autolock lock{mServiceLock};
        service = mPPIManagerService;
    }

    if (service == nullptr) {
        LOG_W("PPIManagerAdapter::responsePPIErase service unavailable");
        return;
    }

    (void)service->responseDeletePPInformation(appType, appState);
}

void PPIManagerAdapter::onStatusChanged(android::sp<::Buffer> &name) {
    if ((name == nullptr) || (name->data() == nullptr) || (name->size() <= 0)) {
        LOG_W("PPIManagerAdapter::onStatusChanged empty payload");
        return;
    }

    std::vector<uint8_t> payload(static_cast<size_t>(name->size()));
    std::memcpy(payload.data(), name->data(), static_cast<size_t>(name->size()));
    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::PpiStatusChanged), payload);
}

void PPIManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
    NOTUSED(who);
    LOG_I("PPIManagerAdapter::onBinderDied");
    {
        const android::Mutex::Autolock lock{mServiceLock};
        mPPIManagerService = nullptr;
        mPPIStatusReceiver = nullptr;
    }

    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(
            mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_PPI_MGR),
            static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
    }
}

android::sp<IPPIManagerService> PPIManagerAdapter::getService() const {
    return android::interface_cast<IPPIManagerService>(
        android::defaultServiceManager()->getService(android::String16("service_layer.PPIManagerService")));
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

PPIManagerAdapter::CommandHandler::CommandHandler(PPIManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("PPIManagerAdapter::CommandHandler: created");
}

void PPIManagerAdapter::CommandHandler::initialize() {
    LOG_I("PPIManagerAdapter::CommandHandler: registering with ProxyIpcClient");
    
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::PpiGetFlag),
        static_cast<uint32_t>(rdgipc::CommandId::PpiResponseDelete)
    };
    
    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("PPIManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("PPIManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

PPIManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("PPIManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse PPIManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);
    
    LOG_I("PPIManagerAdapter::CommandHandler: handling command=%s id=%u payloadSize=%zu",
          rdgipc::commandName(cmdId), commandId, payload.size());
    
    switch (cmdId) {
    case rdgipc::CommandId::PpiGetFlag:
        return handleGetFlag(payload);
    case rdgipc::CommandId::PpiResponseDelete:
        return handleResponseDelete(payload);
    default:
        LOG_W("PPIManagerAdapter::CommandHandler: unsupported command id=%u", commandId);
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse PPIManagerAdapter::CommandHandler::handleGetFlag(const std::vector<uint8_t> &payload) {
    const std::string flagName = rdgipc::toString(payload);
    const uint32_t flagValue = mAdapter->receivePPIErase(flagName.c_str());
    return rdgipc::CommandResponse::ok(flagValue);
}

rdgipc::CommandResponse PPIManagerAdapter::CommandHandler::handleResponseDelete(const std::vector<uint8_t> &payload) {
    // Payload format: "appType,appState"
    const std::string payloadStr = rdgipc::toString(payload);
    std::vector<std::string> parts;
    std::stringstream ss(payloadStr);
    std::string part;
    while (std::getline(ss, part, ',')) {
        parts.push_back(part);
    }
    
    if (parts.size() != 2) {
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::InvalidPayload);
    }
    
    const uint32_t appType = static_cast<uint32_t>(std::stoul(parts[0]));
    const uint32_t appState = static_cast<uint32_t>(std::stoul(parts[1]));
    mAdapter->responsePPIErase(appType, appState);
    return rdgipc::CommandResponse::ok();
}
