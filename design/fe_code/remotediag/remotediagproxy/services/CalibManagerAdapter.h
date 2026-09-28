#ifndef REMOTEDIAGPROXY_CALIBMANAGERADAPTER_H
#define REMOTEDIAGPROXY_CALIBMANAGERADAPTER_H

#include <cstdint>
#include <memory>
#include <string>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>

#include <services/CalibManagerService/ICalibManagerReceiver.h>
#include <services/CalibManagerService/ICalibManagerService.h>
#include <services/CalibManagerService/ICalibManagerServiceType.h>

#include "Error.h"
#include "ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../include/ProxyIpcProtocol.h"
#include "../include/IpcMessageHandler.h"
#include "../utils/Logger.h"
#include "../utils/RemotediagProxyHandler.h"

class RemotediagProxyHandler;

class CalibManagerAdapter {
private:
    class CalibReceiver : public BnCalibManagerReceiver {
    public:
        explicit CalibReceiver(CalibManagerAdapter &parent) noexcept : mParent(parent) {}
        ~CalibReceiver() override = default;

        int32_t onCalibDidChanged(uint16_t did, size_t bufLen, uint8_t *buf) override {
            return mParent.onCalibDidChanged(did, bufLen, buf);
        }

    private:
        CalibManagerAdapter &mParent;
    };

public:
    CalibManagerAdapter();
    ~CalibManagerAdapter() noexcept;
    CalibManagerAdapter(const CalibManagerAdapter &) = delete;
    CalibManagerAdapter &operator=(const CalibManagerAdapter &) = delete;
    CalibManagerAdapter(CalibManagerAdapter &&) = delete;
    CalibManagerAdapter &operator=(CalibManagerAdapter &&) = delete;

    static std::shared_ptr<CalibManagerAdapter> getInstance();

    void registerService();
    bool registerDidWatch();
    void onBinderDied(const android::wp<android::IBinder> &who);

private:
    android::sp<ICalibManagerService> getService() const;
    int32_t onCalibDidChanged(uint16_t did, size_t bufLen, const uint8_t *buf);

private:
    // Nested command handler for IPC
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(CalibManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        CalibManagerAdapter* mAdapter;
        rdgipc::CommandResponse handleRegisterDidWatch(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CommandHandler> mCommandHandler;

private:
    static std::shared_ptr<CalibManagerAdapter> instance;
    static android::Mutex mInstanceLock;

    android::sp<RemotediagProxyHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<ICalibManagerService> mCalibMgrService = nullptr;
    android::sp<ICalibManagerReceiver> mCalibReceiver = nullptr;
    mutable android::Mutex mServiceLock;
    mutable android::Mutex mDiedLock;
};

#endif // REMOTEDIAGPROXY_CALIBMANAGERADAPTER_H
