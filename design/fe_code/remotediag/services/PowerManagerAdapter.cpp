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
std::shared_ptr<PowerManagerAdapter> PowerManagerAdapter::getInstance() {
    if (instance == nullptr) {
        instance = std::make_shared<PowerManagerAdapter>();
    }
    return instance;
}

void PowerManagerAdapter::registerService(void)
{
    error_t error {TIGER_ERR::E_ERROR};

    mHandler = RemotediagHandler::getInstance_2();

    if (mPowerMgrService != nullptr) 
    {
        mPowerMgrService = nullptr;
    }
    mPowerMgrService = android::interface_cast<IPowerManagerService>(
                android::defaultServiceManager()->getService(
                    android::String16(POWER_SRV_NAME)
                    )
                );

    if (mPowerMgrService != nullptr) {
        LOG_I("PowerManagerAdapter Registed");
        const android::status_t result{android::IInterface::asBinder(mPowerMgrService)->linkToDeath(mServiceDeathRecipient)};
        if (result == android::OK) {
            LOG_I("LinkToDeath success");
            error = mPowerMgrService->registerPowerStateReceiver(mPowerRcv, MASK_FOR_EXT_VALUE_CHANGED_NOTI, true);
        } else {
            //do nothing
            LOG_E("LinkToDeath fail");
        }
    }

    if (error != TIGER_ERR::E_OK) {

        if(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS < 2147483647U)
        {
            LOG_E("Cannot register Receiver for Power Service, error: %d, try again after %d (ms)", static_cast<int32_t>(error), static_cast<int32_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
        else
        {
            // do nothing
        }
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
            LOG_D("POWER_IDX::LISTEN_INDEX_MCU_STATUS_IG");
            if ((value >= static_cast<int32_t>(IG_STATUS::IG_STATUS_UNDECIDED)) && (value <= static_cast<int32_t>(IG_STATUS::IG_STATUS_MAX)))
            {
                IGN_changedHandler(static_cast<IG_STATUS>(value));
            }
            break;
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
    if(mPowerMgrService != nullptr) {
        int32_t tempVal {-1};
        (void)mPowerMgrService->requestToGet(POWER_IDX::DCM_IG_STATUS, tempVal);
        if ((tempVal >= static_cast<int32_t>(IG_STATUS::IG_STATUS_UNDECIDED)) && (tempVal <= static_cast<int32_t>(IG_STATUS::IG_STATUS_MAX)))
        {
            val = static_cast<IG_STATUS>(tempVal);
        }
    } else {
        LOG_E("mPowerMgrService is nullptr");
        LOG_E("PowerMgrService is not ready");
    }

    return val;
}
}
