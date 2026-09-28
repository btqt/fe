#ifndef REMOTEDIAGPROXY_APPLICATIONMANAGERADAPTER_H
#define REMOTEDIAGPROXY_APPLICATIONMANAGERADAPTER_H

#include <cstdint>
#include <iostream>
#include <string>
#include <memory>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include "Error.h"
#include "services/ApplicationManagerService/IApplicationManagerService.h"
#include "services/ApplicationManagerService/IApplicationManagerServiceType.h"

// #include "../include/common_def.h"
#include "../include/ParamsDef.h"
#include "../utils/RemotediagProxyHandler.h"
#include "../utils/Logger.h"
#include "ServiceDeathRecipient.h"
#include "../include/IpcMessageHandler.h"
#include "../include/ProxyIpcProtocol.h"

// namespace rdgapp {

class RemotediagProxyHandler;
class ApplicationManagerAdapter : public android::RefBase {
    class SystemPostReceiver : public BnSystemPostReceiver{
        public:
            SystemPostReceiver(ApplicationManagerAdapter &rReceiverMgr) noexcept : mrReceiverMgr(rReceiverMgr) {
            }
            virtual ~SystemPostReceiver() = default;
            SystemPostReceiver(SystemPostReceiver const&) = default;
            SystemPostReceiver& operator=(SystemPostReceiver const&) = default;
            SystemPostReceiver(SystemPostReceiver&&) = delete;
            SystemPostReceiver& operator=(SystemPostReceiver&&) = delete;
            virtual bool onSystemPostReceived(const sp<::Post> &systemPost) {
                return mrReceiverMgr.onSystemPostReceived(systemPost);
            }

        private:
            ApplicationManagerAdapter &mrReceiverMgr;
    };

    bool onSystemPostReceived(const android::sp<::Post> &systemPost);

private:
    static std::shared_ptr<ApplicationManagerAdapter> instance;
    android::sp<IApplicationManagerService> mAppManager = nullptr;
    android::sp<ISystemPostReceiver> mSystemReceiver = nullptr;
    bool mIsBootCompleted{false};
    android::sp<RemotediagProxyHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient;
    uint32_t mAppStatus;
    std::string mRequestedFeature{""};
    mutable android::Mutex mAppManagerLock;
    mutable android::Mutex mDiedLock;
    mutable android::Mutex mBootCompletedLock;
    static android::Mutex mInstanceLock;

public:
    ApplicationManagerAdapter();
    virtual ~ApplicationManagerAdapter() noexcept;
    ApplicationManagerAdapter(ApplicationManagerAdapter const&) = default;
    ApplicationManagerAdapter& operator=(ApplicationManagerAdapter const&) = default;
    ApplicationManagerAdapter(ApplicationManagerAdapter&&) = delete;
    ApplicationManagerAdapter& operator=(ApplicationManagerAdapter&&) = delete;    
    static std::shared_ptr<ApplicationManagerAdapter> getInstance();
    void registerService();
    bool getBootCompleted() const noexcept;
    android::sp<IApplicationManagerService> getService();
    // error_t requestActive(std::string app, const int32_t param = -1, const std::string param2 = "");
    // error_t requestInActive(std::string app, const int32_t param = -1, const std::string param2 = "");
    int32_t queryActionForFeature(const std::string name);
    int32_t setFeatureStatus(const std::string appNames, const std::string feaName, const bool onOf);
    FeatureStatus getFeatureStatus(const std::string name);

    void onBinderDied(const android::wp<android::IBinder>& who);
    // uint32_t getAppStatus();
    
private:
    // ========================================================================
    // NESTED IPC COMMAND HANDLER (handles IPC commands for this adapter)
    // ========================================================================
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(ApplicationManagerAdapter* adapter);
        ~CommandHandler() override;
        
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
        
    private:
        ApplicationManagerAdapter* mAdapter;
        
        rdgipc::CommandResponse handleGetBootCompleted(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleQueryActionForFeature(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleSetFeatureStatus(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleGetFeatureStatus(const std::vector<uint8_t> &payload);
    };
    
    std::shared_ptr<CommandHandler> mCommandHandler;
};
// }
#endif /* REMOTEDIAGPROXY_APPLICATIONMANAGERADAPTER_H */
