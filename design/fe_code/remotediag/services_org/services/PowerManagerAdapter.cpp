#include "PowerManagerAdapter.h"

namespace rdgapp {

PowerManagerAdapter::PowerManagerAdapter() {
    mPowerRcv = new PowerAdapterListener(*this);
    mPowerLockCallback = new PowerLockListener(*this);
    mPowerLock = new PowerLock(getpid());
    mPowerLock->setPowerLockType(POWER_LOCK_LVL_DEFAULT);
    mServiceDeathRecipient = new ServiceDeathRecipient( [this] ( const android::wp<android::IBinder>& who ){
        this->onBinderDied(who);
    });
}

PowerManagerAdapter::~PowerManagerAdapter() noexcept {
    if (mPowerLockCallback != nullptr) {
        delete mPowerLockCallback;
    }
    if(PowerManagerAdapter::instance != nullptr) {
        PowerManagerAdapter::instance = nullptr;
    } 
}

std::shared_ptr<PowerManagerAdapter> PowerManagerAdapter::instance{nullptr};
android::Mutex PowerManagerAdapter::mInstanceLock{};
std::shared_ptr<PowerManagerAdapter> PowerManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<PowerManagerAdapter>();
        }
    }
    return instance;
}

void PowerManagerAdapter::registerService(void)
{
    error_t error {TIGER_ERR::E_ERROR};

    mHandler = RemotediagHandler::getInstance();

    if (mPowerMgrService != nullptr) 
    {
        mPowerMgrService = nullptr;
    }
    mPowerMgrService = getService();
    if (mPowerMgrService != nullptr) {
        LOG_I("PowerManagerAdapter Registed");
        const android::status_t result{android::IInterface::asBinder(mPowerMgrService)->linkToDeath(mServiceDeathRecipient)};
        if (result == android::OK) {
            // LOG_I("LinkToDeath success");
            constexpr uint32_t MASK_EXT_VALUE_CHANGED {static_cast<uint32_t>(MASK_FOR_EXT_VALUE_CHANGED_NOTI)};
            constexpr uint32_t MASK_POWER_STATE_CHANGED {static_cast<uint32_t>(MASK_FOR_CHANGED_POWER_STATE_NOTI)};
            constexpr int32_t mask {static_cast<int32_t>(MASK_EXT_VALUE_CHANGED | MASK_POWER_STATE_CHANGED)};
            error = mPowerMgrService->registerPowerStateReceiver(mPowerRcv, mask, true);
        } else {
            //do nothing
            LOG_E("LinkToDeath fail");
        }
    }

    if (error != TIGER_ERR::E_OK) {
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_POWER_MODE_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        } else {
            LOG_E("mHandler is nullptr");
        }
    }
}

void PowerManagerAdapter::onPowerStateChanged(const int32_t newState, const int32_t reason) {
    //TBD
    LOG_I("newState: %d reason: %d",newState, reason);
    if (newState == POWER_STATE_WAITING_SHUTDOWN) {
        LOG_I("PowerMng will be reboot in next few second!");
        // MSG_PREPARE_TO_SHUTDOWN
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_PREPARE_TO_SHUTDOWN)->sendToTarget();
    }
}
void PowerManagerAdapter::onErrControlPower(const int32_t err_reason, const int32_t errPowerID, const int32_t currPowerID) {
    //TBD
    LOG_D("err_reason: %d errPowerID: %d currPowerID: %d",err_reason, errPowerID, currPowerID);
}
void PowerManagerAdapter::onPowerModeChanged(const int32_t newMode) {
    LOG_I("onPowerModeChanged newMode = %d", newMode);    
}

void PowerManagerAdapter::onExtValueChanged(const int32_t listenIndex, const int32_t value) {
    switch (listenIndex)
    {
        case POWER_IDX::LISTEN_INDEX_MCU_STATUS_IG:
        {
            LOG_D("POWER_IDX::LISTEN_INDEX_MCU_STATUS_IG");
            if ((value >= static_cast<int32_t>(IG_STATUS::IG_STATUS_UNDECIDED)) && (value < static_cast<int32_t>(IG_STATUS::IG_STATUS_MAX)))
            {
                IGN_changedHandler(static_cast<IG_STATUS>(value));
            }
            break;
        }
        case POWER_IDX::LISTEN_INDEX_POWER_SOURCE:
        {
            LOG_I("Power source change to: %s", value == POWER_SOURCE::POWER_SOURCE_BUB ? "POWER_SOURCE_BUB" : "POWER_SOURCE_MAIN");
    
            if (value == static_cast<int32_t>(POWER_SOURCE::POWER_SOURCE_BUB))
            { /* POWER_SOURCE_BUB */
                bubTrigger(true);
            }
            else if (value == static_cast<int32_t>(POWER_SOURCE::POWER_SOURCE_MAIN))
            {
                bubTrigger(false);
            }
            else
            {
                LOG_I("Power source is invalid");
            }
            break;
        }
        default:
            break;
    }
}

void PowerManagerAdapter::onPowerLockRelease() {
    //TBD
    LOG_I("onPowerLockRelease");
}

void PowerManagerAdapter::acquirePowerLock() {
    (void)mPowerLock->acquire(MAX_RETRY_TIME);
}

void PowerManagerAdapter::releasePowerLock() {
    if (mPowerLock->isLocked()) 
    {
        (void)mPowerLock->release();
    }
}

void PowerManagerAdapter::onBinderDied(const android::wp<android::IBinder>& who) {
    LOG_I("onBinderDied");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
    NOTUSED(who);
    mPowerMgrService = nullptr;
    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_POWER_MODE_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}

void PowerManagerAdapter::IGN_changedHandler(const IG_STATUS status)
{
    LOG_I("IG status changed to %s", status == IG_STATUS::IG_STATUS_ON ? "IGN_ON" : "IGN_OFF");

    if (mHandler != nullptr)
    {
        switch (status) 
        {
            case IG_STATUS::IG_STATUS_ON:
                (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_ON)->sendToTarget();
                break;
            case IG_STATUS::IG_STATUS_OFF:
                (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_OFF)->sendToTarget();
                break;
            default:
                LOG_W("IG unknow status, value = %d", static_cast<int32_t>(status));
                break;
        }
    }
    else {
        // do nothing
        LOG_E("mHandler is nullptr");
    }
}

IG_STATUS PowerManagerAdapter::getIgnitionStatus(void)
{
    IG_STATUS val {IG_STATUS::IG_STATUS_MAX};
    const android::sp<IPowerManagerService> powerMgr{getService()};
    if(powerMgr != nullptr) {
        int32_t tempVal {-1};
        (void)powerMgr->requestToGet(POWER_IDX::DCM_IG_STATUS, tempVal);
        if ((tempVal >= static_cast<int32_t>(IG_STATUS::IG_STATUS_UNDECIDED)) && (tempVal <= static_cast<int32_t>(IG_STATUS::IG_STATUS_MAX)))
        {
            val = static_cast<IG_STATUS>(tempVal);
        }
    } else {
        LOG_E("PowerMgrService is not ready");
    }

    return val;
}
android::sp<IPowerManagerService> PowerManagerAdapter::getService(void) const
{
    return android::interface_cast<IPowerManagerService>(android::defaultServiceManager()->getService(android::String16(POWER_SRV_NAME)));
}

void PowerManagerAdapter::bubTrigger(const bool value)
{
    LOG_I("BUB Trigger: %s", value ? "ON" : "OFF");
    if (mCurBubStatus != value)
    {
        mCurBubStatus = value;
        if (mHandler != nullptr)
        {
            int32_t message{0};
            if (mCurBubStatus)
            {
                message = HANDLE_MESSAGE_REQUEST::HANDLE_MSG_BUB_ON;
            }
            else
            {
                message = HANDLE_MESSAGE_REQUEST::HANDLE_MSG_BUB_OFF;
            }
            (void)mHandler->obtainMessage(message)->sendToTarget();
        }
        else
        {
            LOG_E("mHandler is nullptr");
        }
    }
    else
    {
        LOG_I("BUB status unchanged");
    }
}
}
