#ifndef REMOTEDIAGPROXY_DIAGMANAGERADAPTER_H
#define REMOTEDIAGPROXY_DIAGMANAGERADAPTER_H

#include <cstdint>
#include <memory>
#include <vector>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>

#include <services/DiagManagerService/IDiagManagerService.h>
#include <services/DiagManagerService/IDiagManagerServiceType.h>

#include "Error.h"
#include "ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../include/IpcMessageHandler.h"
#include "../utils/Logger.h"
#include "../utils/RemotediagProxyHandler.h"

class RemotediagProxyHandler;

class DiagManagerAdapter {
public:
    DiagManagerAdapter();
    ~DiagManagerAdapter() noexcept;
    DiagManagerAdapter(const DiagManagerAdapter &) = delete;
    DiagManagerAdapter &operator=(const DiagManagerAdapter &) = delete;
    DiagManagerAdapter(DiagManagerAdapter &&) = delete;
    DiagManagerAdapter &operator=(DiagManagerAdapter &&) = delete;

    static std::shared_ptr<DiagManagerAdapter> getInstance();

    void registerService();
    bool writeDidData(uint16_t did, const std::vector<uint8_t> &didData);
    bool readDidData(uint16_t did, std::vector<uint8_t> &didData);

    void onBinderDied(const android::wp<android::IBinder> &who);

private:
    android::sp<IDiagManagerService> getService() const;
    bool ensureServiceReady();

private:
    static std::shared_ptr<DiagManagerAdapter> instance;
    static android::Mutex mInstanceLock;

    android::sp<RemotediagProxyHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<IDiagManagerService> mDiagMService = nullptr;
    mutable android::Mutex mServiceLock;
    mutable android::Mutex mDiedLock;
    
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(DiagManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        DiagManagerAdapter* mAdapter;
    };
    std::shared_ptr<CommandHandler> mCommandHandler;
};

#endif // REMOTEDIAGPROXY_DIAGMANAGERADAPTER_H
