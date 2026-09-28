#ifndef REMOTEDIAG_REG_ADAPTER_POWERMANAGER_H
#define REMOTEDIAG_REG_ADAPTER_POWERMANAGER_H

#include <services/PowerManagerService/IPowerManagerService.h>
#include <services/PowerManagerService/IPowerManagerServiceType.h>
#include <services/PowerManagerService/PowerManager.h>
#include <services/PowerManagerService/PowerLock.h>
#include <services/PowerManagerService/PowerIndexEnum.h>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"

namespace rdgapp {

class PowerManagerAdapter {

    class PowerLockListener: public PowerLockCallback {
        public:
            PowerLockListener(PowerManagerAdapter& powerAdapter) noexcept : mParent(powerAdapter) {}
            virtual ~PowerLockListener() = default;
            PowerLockListener(PowerLockListener const&) = default;
            PowerLockListener& operator=(PowerLockListener const&) = default;
            PowerLockListener(PowerLockListener&&) = delete;
            PowerLockListener& operator=(PowerLockListener&&) = delete;
            virtual void expiredtimeout() {
                mParent.onPowerLockRelease();
            }
        private:
            PowerManagerAdapter& mParent;
    };

    class PowerAdapterListener : public BnPowerStateReceiver {
    public:
        PowerAdapterListener(PowerManagerAdapter& powerAdapter) noexcept : mParent(powerAdapter){}
        virtual ~PowerAdapterListener() = default;
        PowerAdapterListener(PowerAdapterListener const&) = default;
        PowerAdapterListener& operator=(PowerAdapterListener const&) = default;
        PowerAdapterListener(PowerAdapterListener&&) = delete;
        PowerAdapterListener& operator=(PowerAdapterListener&&) = delete;
        virtual void onPowerStateChanged(const int32_t newState, const int32_t reason) {
            mParent.onPowerStateChanged(newState, reason);
        }
        virtual void onErrControlPower(const int32_t err_reason, const int32_t errPowerID, const int32_t currPowerID) {
            mParent.onErrControlPower(err_reason, errPowerID, currPowerID);
        }
        virtual void onPowerModeChanged(const int32_t newMode) {
            mParent.onPowerModeChanged(newMode);
        }
        virtual void onExtValueChanged(const int32_t listenIndex, const int32_t value) {
            mParent.onExtValueChanged(listenIndex, value);
        }

    private:
        PowerManagerAdapter& mParent;
    };

private:
    static std::shared_ptr<PowerManagerAdapter> instance;
    android::sp<RemotediagHandler> mHandler = nullptr;
    android::sp<PowerAdapterListener> mPowerRcv = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<IPowerManagerService> mPowerMgrService = nullptr;
    android::sp<PowerLock> mPowerLock = nullptr;
    PowerLockListener *mPowerLockCallback = nullptr;
    bool mIsLocked = false;
    bool mCurBubStatus = false;
    OPERATION_MODE_POWER_STATE operationPowerState = OPERATION_MODE_POWER_STATE::STATE_STOP;

    // Private functions
    void IGN_changedHandler(const IG_STATUS status);
    android::sp<IPowerManagerService> getService(void) const;
    mutable android::Mutex mDiedLock;
    static android::Mutex mInstanceLock;

public:
    PowerManagerAdapter();
    virtual ~PowerManagerAdapter() noexcept;
    PowerManagerAdapter(PowerManagerAdapter const&) = default;
    PowerManagerAdapter& operator=(PowerManagerAdapter const&) = default;
    PowerManagerAdapter(PowerManagerAdapter&&) = delete;
    PowerManagerAdapter& operator=(PowerManagerAdapter&&) = delete;
    static std::shared_ptr<PowerManagerAdapter> getInstance();
    void registerService();
    // bool requestBUBMode(int32_t powerIndex, int32_t value);
    void acquirePowerLock();
    void releasePowerLock();
    void onPowerStateChanged(const int32_t newState, const int32_t reason);
    static void onErrControlPower(const int32_t err_reason, const int32_t errPowerID, const int32_t currPowerID);
    static void onPowerModeChanged(const int32_t newMode);
    void onExtValueChanged(const int32_t listenIndex, const int32_t value);
    static void onPowerLockRelease();
    void onBinderDied(const android::wp<android::IBinder>& who);
    
    IG_STATUS getIgnitionStatus(void);
    

    void bubTrigger(const bool value);

    constexpr static int32_t MAX_RETRY_TIME {30}; /*30 second*/
};
}
#endif /* REMOTEDIAG_REG_ADAPTER_POWERMANAGER_H */
