#ifndef REMOTEDIAG_REG_APPLICATIONMANAGERADAPTER_H
#define REMOTEDIAG_REG_APPLICATIONMANAGERADAPTER_H

#include <cstdint>
#include <iostream>
#include <string>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include "Error.h"
#include "services/ApplicationManagerService/IApplicationManagerService.h"
#include "services/ApplicationManagerService/IApplicationManagerServiceType.h"

#include "../include/common_def.h"
#include "../include/ParamsDef.h"
#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"

namespace rdgapp {

class RemotediagHandler;
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
    //static ApplicationManagerAdapter* instance;
    static std::shared_ptr<ApplicationManagerAdapter> instance;
    android::sp<IApplicationManagerService> mAppManager = nullptr;
    android::sp<ISystemPostReceiver> mSystemReceiver = nullptr;
    bool mIsBootCompleted{false};
    android::sp<RemotediagHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient;
    uint32_t mAppStatus;
    std::string mRequestedFeature{""};
public:
    ApplicationManagerAdapter();
    virtual ~ApplicationManagerAdapter() noexcept;
    ApplicationManagerAdapter(ApplicationManagerAdapter const&) = default;
    ApplicationManagerAdapter& operator=(ApplicationManagerAdapter const&) = default;
    ApplicationManagerAdapter(ApplicationManagerAdapter&&) = delete;
    ApplicationManagerAdapter& operator=(ApplicationManagerAdapter&&) = delete;    
    //static ApplicationManagerAdapter* getInstance();
    static std::shared_ptr<ApplicationManagerAdapter> getInstance();
    void registerService();
    // bool getBootComplete();
    android::sp<IApplicationManagerService> getService();
    // error_t requestActive(std::string app, const int32_t param = -1, const std::string param2 = "");
    // error_t requestInActive(std::string app, const int32_t param = -1, const std::string param2 = "");
    int32_t queryActionForFeature(const std::string name);
    int32_t setFeatureStatus(const std::string appNames, const std::string feaName, const bool onOf);
    FeatureStatus getFeatureStatus(const std::string name);

    void onBinderDied(const android::wp<android::IBinder>& who);
    // uint32_t getAppStatus();
};
}
#endif /* REMOTEDIAG_REG_APPLICATIONMANAGERADAPTER_H */
