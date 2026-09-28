#include "PowerManagerAdapter.h"

#include <sstream>

#include "../utils/ProxyIpcClient.h"

namespace {
std::vector<uint8_t> makePayload2(const int32_t first, const int32_t second) {
    const std::string str = std::to_string(first) + "," + std::to_string(second);
    return std::vector<uint8_t>(str.begin(), str.end());
}

std::vector<uint8_t> makePayload3(const int32_t first, const int32_t second, const int32_t third) {
    const std::string str = std::to_string(first) + "," + std::to_string(second) + "," + std::to_string(third);
    return std::vector<uint8_t>(str.begin(), str.end());
}
} // namespace

std::shared_ptr<PowerManagerAdapter> PowerManagerAdapter::instance{nullptr};
android::Mutex PowerManagerAdapter::mInstanceLock{};

PowerManagerAdapter::PowerManagerAdapter() {
    mPowerReceiver = new PowerAdapterListener(*this);
    mPowerLockCallback = new PowerLockListener(*this);
    mPowerLock = new PowerLock(getpid());
    mPowerLock->setPowerLockType(POWER_LOCK_LVL_DEFAULT);
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who) {
        this->onBinderDied(who);
    });
    
    // Create and register IPC command handler
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
}

PowerManagerAdapter::~PowerManagerAdapter() noexcept {
    if (mPowerLockCallback != nullptr) {
        delete mPowerLockCallback;
    }
    if (PowerManagerAdapter::instance != nullptr) {
        PowerManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<PowerManagerAdapter> PowerManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<PowerManagerAdapter>();
        }
    }
    return instance;
}

void PowerManagerAdapter::registerService() {
    mHandler = RemotediagProxyHandler::getInstance();

    error_t error{TIGER_ERR::E_ERROR};
    android::sp<IPowerManagerService> service{getService()};
    if (service != nullptr) {
        LOG_I("PowerManagerAdapter Registered");
        const android::status_t result{android::IInterface::asBinder(service)->linkToDeath(mServiceDeathRecipient)};
        if (result == android::OK) {
            constexpr uint32_t MASK_EXT_VALUE_CHANGED{static_cast<uint32_t>(MASK_FOR_EXT_VALUE_CHANGED_NOTI)};
            constexpr uint32_t MASK_POWER_STATE_CHANGED{static_cast<uint32_t>(MASK_FOR_CHANGED_POWER_STATE_NOTI)};
            constexpr int32_t mask{static_cast<int32_t>(MASK_EXT_VALUE_CHANGED | MASK_POWER_STATE_CHANGED)};
            error = service->registerPowerStateReceiver(mPowerReceiver, mask, true);
        } else {
            LOG_E("PowerManagerAdapter linkToDeath failed");
        }

        const android::Mutex::Autolock lock{mServiceLock};
        mPowerMgrService = service;
    }

    if ((error != TIGER_ERR::E_OK) && (mHandler != nullptr)) {
        (void)mHandler->sendMessageDelayed(
            mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_POWER_MODE_MGR),
            static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
    }
}

void PowerManagerAdapter::acquirePowerLock() {
    if (mPowerLock != nullptr) {
        (void)mPowerLock->acquire(MAX_RETRY_TIME);
    }
}

void PowerManagerAdapter::releasePowerLock() {
    if ((mPowerLock != nullptr) && mPowerLock->isLocked()) {
        (void)mPowerLock->release();
    }
}

IG_STATUS PowerManagerAdapter::getIgnitionStatus() {
    IG_STATUS value{IG_STATUS::IG_STATUS_MAX};
    const android::sp<IPowerManagerService> powerMgr{getService()};
    if (powerMgr != nullptr) {
        int32_t tempVal{-1};
        (void)powerMgr->requestToGet(POWER_IDX::DCM_IG_STATUS, tempVal);
        if ((tempVal >= static_cast<int32_t>(IG_STATUS::IG_STATUS_UNDECIDED)) &&
            (tempVal <= static_cast<int32_t>(IG_STATUS::IG_STATUS_MAX))) {
            value = static_cast<IG_STATUS>(tempVal);
        }
    } else {
        LOG_E("PowerMgrService is not ready");
    }

    return value;
}

void PowerManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
    NOTUSED(who);
    LOG_I("PowerManagerAdapter::onBinderDied");
    {
        const android::Mutex::Autolock lock{mServiceLock};
        mPowerMgrService = nullptr;
    }

    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(
            mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_POWER_MODE_MGR),
            static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
    }
}

void PowerManagerAdapter::onPowerStateChanged(const int32_t newState, const int32_t reason) {
    LOG_I("onPowerStateChanged newState=%d reason=%d", newState, reason);
    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::PowerStateChanged), makePayload2(newState, reason));
}

void PowerManagerAdapter::onErrControlPower(const int32_t errReason, const int32_t errPowerId, const int32_t currPowerId) {
    LOG_D("onErrControlPower errReason=%d errPowerId=%d currPowerId=%d", errReason, errPowerId, currPowerId);
    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::PowerErrControl), makePayload3(errReason, errPowerId, currPowerId));
}

void PowerManagerAdapter::onPowerModeChanged(const int32_t newMode) {
    LOG_I("onPowerModeChanged newMode=%d", newMode);
    const std::string str = std::to_string(newMode);
    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::PowerModeChanged), std::vector<uint8_t>(str.begin(), str.end()));
}

void PowerManagerAdapter::onExtValueChanged(const int32_t listenIndex, const int32_t value) {
    LOG_I("onExtValueChanged listenIndex=%d value=%d", listenIndex, value);
    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::PowerExtValueChanged), makePayload2(listenIndex, value));
}

void PowerManagerAdapter::onPowerLockRelease() {
    LOG_I("onPowerLockRelease");
    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::PowerLockReleased));
}

android::sp<IPowerManagerService> PowerManagerAdapter::getService() const {
    return android::interface_cast<IPowerManagerService>(
        android::defaultServiceManager()->getService(android::String16(POWER_SRV_NAME)));
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

PowerManagerAdapter::CommandHandler::CommandHandler(PowerManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("PowerManagerAdapter::CommandHandler: created");
}

void PowerManagerAdapter::CommandHandler::initialize() {
    LOG_I("PowerManagerAdapter::CommandHandler: registering with ProxyIpcClient");
    
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::PowerGetIgnitionStatus),
        static_cast<uint32_t>(rdgipc::CommandId::PowerAcquireLock),
        static_cast<uint32_t>(rdgipc::CommandId::PowerReleaseLock)
    };
    
    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("PowerManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("PowerManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

PowerManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("PowerManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse PowerManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);
    
    LOG_I("PowerManagerAdapter::CommandHandler: handling command=%s id=%u payloadSize=%zu",
          rdgipc::commandName(cmdId), commandId, payload.size());
    
    switch (cmdId) {
    case rdgipc::CommandId::PowerGetIgnitionStatus:
        return handleGetIgnitionStatus(payload);
    case rdgipc::CommandId::PowerAcquireLock:
        return handleAcquireLock(payload);
    case rdgipc::CommandId::PowerReleaseLock:
        return handleReleaseLock(payload);
    default:
        LOG_W("PowerManagerAdapter::CommandHandler: unsupported command id=%u", commandId);
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse PowerManagerAdapter::CommandHandler::handleGetIgnitionStatus(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    const IG_STATUS status = mAdapter->getIgnitionStatus();
    return rdgipc::CommandResponse::ok(static_cast<int32_t>(status));
}

rdgipc::CommandResponse PowerManagerAdapter::CommandHandler::handleAcquireLock(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    mAdapter->acquirePowerLock();
    return rdgipc::CommandResponse::ok();
}

rdgipc::CommandResponse PowerManagerAdapter::CommandHandler::handleReleaseLock(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    mAdapter->releasePowerLock();
    return rdgipc::CommandResponse::ok();
}
