#ifndef REMOTEDIAG_REG_ADAPTER_CALIB_MANAGER_H
#define REMOTEDIAG_REG_ADAPTER_CALIB_MANAGER_H

// Calib Manager
#include <iostream>
#include <memory>
#include <services/CalibManagerService/ICalibManagerService.h>
#include <services/CalibManagerService/ICalibManagerServiceType.h>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <Error.h>

#include "DiagManagerAdapter.h"
#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../remotediagproxy/include/IpcMessageHandler.h"

namespace rdgapp {

class RemotediagHandler;
class CalibManagerAdapter
{
public:
    CalibManagerAdapter() noexcept ;
    virtual ~CalibManagerAdapter() noexcept;
    CalibManagerAdapter(CalibManagerAdapter const&) = default;
    CalibManagerAdapter& operator=(CalibManagerAdapter const&) = default;
    CalibManagerAdapter(CalibManagerAdapter&&) = delete;
    CalibManagerAdapter& operator=(CalibManagerAdapter&&) = delete;
    static std::shared_ptr<CalibManagerAdapter> getInstance();

    android::sp<ICalibManagerService> getService();

    void registerService();
    void onBinderDied(const android::wp<android::IBinder>& who);

private:
    // Nested CallbackHandler for self-registering callback handling
    class CallbackHandler : public rdgipc::ICallbackHandler,
                           public std::enable_shared_from_this<CallbackHandler> {
    public:
        explicit CallbackHandler(CalibManagerAdapter* adapter);
        ~CallbackHandler() override = default;
        void initialize();
        void handle(uint32_t callbackId, const std::vector<uint8_t>& payload) override;
    private:
        CalibManagerAdapter* mAdapter;
    };
    std::shared_ptr<CallbackHandler> mCallbackHandler;

    static std::shared_ptr<CalibManagerAdapter> instance;
    android::sp<ICalibManagerService> mCalibMgrService;
    android::sp<RemotediagHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    mutable android::Mutex mDiedLock;
    static android::Mutex mInstanceLock;
};
}
#endif
