#ifndef REMOTEDIAGPROXY_POWERMANAGERADAPTER_H
#define REMOTEDIAGPROXY_POWERMANAGERADAPTER_H

#include <memory>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>

#include <services/PowerManagerService/IPowerManagerService.h>
#include <services/PowerManagerService/IPowerManagerServiceType.h>
#include <services/PowerManagerService/PowerIndexEnum.h>
#include <services/PowerManagerService/PowerLock.h>
#include <services/PowerManagerService/PowerManager.h>

#include "Error.h"
#include "ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../include/ProxyIpcProtocol.h"
#include "../include/IpcMessageHandler.h"
#include "../utils/Logger.h"
#include "../utils/RemotediagProxyHandler.h"

class RemotediagProxyHandler;

class PowerManagerAdapter {
private:
    class PowerLockListener : public PowerLockCallback {
    public:
        explicit PowerLockListener(PowerManagerAdapter &powerAdapter) noexcept : mParent(powerAdapter) {}
        ~PowerLockListener() override = default;
        void expiredtimeout() override {
            mParent.onPowerLockRelease();
        }

    private:
        PowerManagerAdapter &mParent;
    };

    class PowerAdapterListener : public BnPowerStateReceiver {
    public:
        explicit PowerAdapterListener(PowerManagerAdapter &powerAdapter) noexcept : mParent(powerAdapter) {}
        ~PowerAdapterListener() override = default;
        void onPowerStateChanged(const int32_t newState, const int32_t reason) override {
            mParent.onPowerStateChanged(newState, reason);
        }
        void onErrControlPower(const int32_t err_reason, const int32_t errPowerID, const int32_t currPowerID) override {
            mParent.onErrControlPower(err_reason, errPowerID, currPowerID);
        }
        void onPowerModeChanged(const int32_t newMode) override {
            mParent.onPowerModeChanged(newMode);
        }
        void onExtValueChanged(const int32_t listenIndex, const int32_t value) override {
            mParent.onExtValueChanged(listenIndex, value);
        }

    private:
        PowerManagerAdapter &mParent;
    };

public:
    PowerManagerAdapter();
    ~PowerManagerAdapter() noexcept;
    PowerManagerAdapter(const PowerManagerAdapter &) = delete;
    PowerManagerAdapter &operator=(const PowerManagerAdapter &) = delete;
    PowerManagerAdapter(PowerManagerAdapter &&) = delete;
    PowerManagerAdapter &operator=(PowerManagerAdapter &&) = delete;

    static std::shared_ptr<PowerManagerAdapter> getInstance();

    void registerService();
    void acquirePowerLock();
    void releasePowerLock();
    IG_STATUS getIgnitionStatus();

    void onBinderDied(const android::wp<android::IBinder> &who);
    void onPowerStateChanged(int32_t newState, int32_t reason);
    void onErrControlPower(int32_t errReason, int32_t errPowerId, int32_t currPowerId);
    void onPowerModeChanged(int32_t newMode);
    void onExtValueChanged(int32_t listenIndex, int32_t value);
    void onPowerLockRelease();

private:
    android::sp<IPowerManagerService> getService() const;

private:
    // Nested command handler for IPC
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(PowerManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        PowerManagerAdapter* mAdapter;
        rdgipc::CommandResponse handleGetIgnitionStatus(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleAcquireLock(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleReleaseLock(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CommandHandler> mCommandHandler;

private:
    static std::shared_ptr<PowerManagerAdapter> instance;
    static android::Mutex mInstanceLock;

    android::sp<RemotediagProxyHandler> mHandler = nullptr;
    android::sp<PowerAdapterListener> mPowerReceiver = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<IPowerManagerService> mPowerMgrService = nullptr;
    android::sp<PowerLock> mPowerLock = nullptr;
    PowerLockListener *mPowerLockCallback = nullptr;
    mutable android::Mutex mServiceLock;

    static constexpr int32_t MAX_RETRY_TIME{30};
};

#endif // REMOTEDIAGPROXY_POWERMANAGERADAPTER_H
