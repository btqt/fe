#ifndef REMOTEDIAGPROXY_PPIMANAGERADAPTER_H
#define REMOTEDIAGPROXY_PPIMANAGERADAPTER_H

#include <memory>
#include <string>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <utils/Buffer.h>
#include <utils/Mutex.h>

#include <services/PPIManagerService/IPPIManagerService.h>
#include <services/PPIManagerService/IPPIManagerServiceType.h>
#include <services/PPIManagerService/IPPIStatusReceiver.h>

#include "Error.h"
#include "ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../include/ProxyIpcProtocol.h"
#include "../include/IpcMessageHandler.h"
#include "../utils/Logger.h"
#include "../utils/RemotediagProxyHandler.h"

class RemotediagProxyHandler;

class PPIManagerAdapter {
private:
    class PPIMgrReceiver : public BnPPIStatusReceiver {
    public:
        explicit PPIMgrReceiver(PPIManagerAdapter &adapter) noexcept : mParent(adapter) {}
        ~PPIMgrReceiver() override = default;

        void onStatusChanged(android::sp<::Buffer> &name) override {
            mParent.onStatusChanged(name);
        }

    private:
        PPIManagerAdapter &mParent;
    };

public:
    PPIManagerAdapter();
    ~PPIManagerAdapter() noexcept;
    PPIManagerAdapter(const PPIManagerAdapter &) = delete;
    PPIManagerAdapter &operator=(const PPIManagerAdapter &) = delete;
    PPIManagerAdapter(PPIManagerAdapter &&) = delete;
    PPIManagerAdapter &operator=(PPIManagerAdapter &&) = delete;

    static std::shared_ptr<PPIManagerAdapter> getInstance();

    void registerService();
    uint32_t receivePPIErase(const char_t *buf);
    void responsePPIErase(uint32_t appType, uint32_t appState);

    void onStatusChanged(android::sp<::Buffer> &name);
    void onBinderDied(const android::wp<android::IBinder> &who);

private:
    android::sp<IPPIManagerService> getService() const;
    bool registerServiceLocked();

private:
    // Nested command handler for IPC
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(PPIManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        PPIManagerAdapter* mAdapter;
        rdgipc::CommandResponse handleGetFlag(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleResponseDelete(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CommandHandler> mCommandHandler;

private:
    static std::shared_ptr<PPIManagerAdapter> instance;
    static android::Mutex mInstanceLock;

    android::sp<RemotediagProxyHandler> mHandler = nullptr;
    android::sp<IPPIManagerService> mPPIManagerService = nullptr;
    android::sp<IPPIStatusReceiver> mPPIStatusReceiver = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    mutable android::Mutex mServiceLock;
};

#endif // REMOTEDIAGPROXY_PPIMANAGERADAPTER_H
