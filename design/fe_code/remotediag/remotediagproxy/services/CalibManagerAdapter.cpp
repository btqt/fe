#include "CalibManagerAdapter.h"

#include "../utils/ProxyIpcClient.h"

std::shared_ptr<CalibManagerAdapter> CalibManagerAdapter::instance{nullptr};
android::Mutex CalibManagerAdapter::mInstanceLock{};

namespace {
constexpr uint16_t kDidPpiConsentState{0x1022U};
constexpr uint16_t kDidServiceFlag{0x3500U};
constexpr uint16_t kDidVin{0xF190U};
}

CalibManagerAdapter::CalibManagerAdapter() {
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who) {
        this->onBinderDied(who);
    });
    
    // Create and register IPC command handler
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
}

CalibManagerAdapter::~CalibManagerAdapter() noexcept {
    if (CalibManagerAdapter::instance != nullptr) {
        CalibManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<CalibManagerAdapter> CalibManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<CalibManagerAdapter>();
        }
    }
    return instance;
}

void CalibManagerAdapter::registerService() {
    mHandler = RemotediagProxyHandler::getInstance();
    if (!registerDidWatch()) {
        LOG_W("CalibManagerAdapter: register failed, retry");
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(
                mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CALIB_MGR),
                static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
    }
}

android::sp<ICalibManagerService> CalibManagerAdapter::getService() const {
    return android::interface_cast<ICalibManagerService>(
        android::defaultServiceManager()->getService(android::String16("service_layer.CalibManagerService")));
}

bool CalibManagerAdapter::registerDidWatch() {
    android::sp<ICalibManagerService> service{nullptr};
    {
        const android::Mutex::Autolock lock{mServiceLock};
        service = mCalibMgrService;
    }

    if (service == nullptr) {
        service = getService();
        if (service == nullptr) {
            return false;
        }

        const android::status_t result{android::IInterface::asBinder(service)->linkToDeath(mServiceDeathRecipient)};
        if (result != android::OK) {
            LOG_E("CalibManagerAdapter: linkToDeath failed: %d", static_cast<int32_t>(result));
            return false;
        }

        if (mCalibReceiver == nullptr) {
            mCalibReceiver = android::sp<CalibReceiver>(new CalibReceiver(*this));
        }

        const error_t res1{service->registerReceiverCalibManagerReceiverOnCalibReceiveDID(mCalibReceiver, kDidPpiConsentState)};
        const error_t res2{service->registerReceiverCalibManagerReceiverOnCalibReceiveDID(mCalibReceiver, kDidServiceFlag)};
        const error_t res3{service->registerReceiverCalibManagerReceiverOnCalibReceiveDID(mCalibReceiver, kDidVin)};
        if ((res1 != E_OK) || (res2 != E_OK) || (res3 != E_OK)) {
            LOG_E("CalibManagerAdapter: DID receiver registration failed");
            return false;
        }

        const android::Mutex::Autolock lock{mServiceLock};
        mCalibMgrService = service;
    }

    return true;
}

void CalibManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
    NOTUSED(who);
    LOG_I("CalibManagerAdapter::onBinderDied");
    const android::Mutex::Autolock lock{mDiedLock};
    {
        const android::Mutex::Autolock serviceLock{mServiceLock};
        mCalibMgrService = nullptr;
        mCalibReceiver = nullptr;
    }

    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(
            mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CALIB_MGR),
            static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
    }
}

int32_t CalibManagerAdapter::onCalibDidChanged(const uint16_t did, const size_t bufLen, const uint8_t *buf) {
    if ((buf == nullptr) || (bufLen == 0U) || (bufLen > 17U)) {
        LOG_W("CalibManagerAdapter: invalid callback did=0x%04X len=%zu", did, bufLen);
        return 0;
    }

    rdgipc::CallbackId callbackId{rdgipc::CallbackId::CalibPpiFlagChanged};
    size_t expectedLen{0U};
    switch (did) {
    case kDidServiceFlag:
        callbackId = rdgipc::CallbackId::CalibServiceFlagChanged;
        expectedLen = 2U;
        break;
    case kDidPpiConsentState:
        callbackId = rdgipc::CallbackId::CalibPpiFlagChanged;
        expectedLen = 1U;
        break;
    case kDidVin:
        callbackId = rdgipc::CallbackId::CalibVinChanged;
        expectedLen = 17U;
        break;
    default:
        return 0;
    }

    if (bufLen != expectedLen) {
        LOG_W("CalibManagerAdapter: unexpected callback length did=0x%04X len=%zu expected=%zu",
              did,
              bufLen,
              expectedLen);
        return 0;
    }

    std::vector<uint8_t> payload(buf, buf + bufLen);
    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(callbackId), payload);
    return 0;
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

CalibManagerAdapter::CommandHandler::CommandHandler(CalibManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("CalibManagerAdapter::CommandHandler: created");
}

void CalibManagerAdapter::CommandHandler::initialize() {
    LOG_I("CalibManagerAdapter::CommandHandler: registering with ProxyIpcClient");
    
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::CalibRegisterDidWatch)
    };
    
    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("CalibManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("CalibManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

CalibManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("CalibManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse CalibManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);
    
    LOG_I("CalibManagerAdapter::CommandHandler: handling command=%s id=%u payloadSize=%zu",
          rdgipc::commandName(cmdId), commandId, payload.size());
    
    switch (cmdId) {
    case rdgipc::CommandId::CalibRegisterDidWatch:
        return handleRegisterDidWatch(payload);
    default:
        LOG_W("CalibManagerAdapter::CommandHandler: unsupported command id=%u", commandId);
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse CalibManagerAdapter::CommandHandler::handleRegisterDidWatch(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    const bool success = mAdapter->registerDidWatch();
    return success ? rdgipc::CommandResponse::ok() : rdgipc::CommandResponse::err(rdgipc::ErrorCode::OperationFailed);
}
