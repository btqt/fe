#include "ApplicationManagerAdapter.h"

namespace rdgapp {
ApplicationManagerAdapter::ApplicationManagerAdapter() {
    mIsBootCompleted = false;
    mServiceDeathRecipient = new ServiceDeathRecipient( [this] ( const android::wp<android::IBinder>& who ){
        this->onBinderDied(who);
    });
}

ApplicationManagerAdapter::~ApplicationManagerAdapter() noexcept {
    if(ApplicationManagerAdapter::instance != nullptr) {
        ApplicationManagerAdapter::instance = nullptr;
    }
}
std::shared_ptr<ApplicationManagerAdapter> ApplicationManagerAdapter::instance{nullptr};
android::Mutex ApplicationManagerAdapter::mInstanceLock{};
std::shared_ptr<ApplicationManagerAdapter> ApplicationManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<ApplicationManagerAdapter>();
        }
    }
    return instance;
}

android::sp<IApplicationManagerService> ApplicationManagerAdapter::getService() {
    return android::interface_cast<IApplicationManagerService> (
        android::defaultServiceManager()->getService(android::String16("service_layer.ApplicationManagerService")));

}

void ApplicationManagerAdapter::registerService() {
    LOG_I("Start register ApplicationManagerAdapter");
    mHandler = RemotediagHandler::getInstance();
    if (mAppManager != nullptr) {
        mAppManager = nullptr;
        mSystemReceiver = nullptr;
    }
    mAppManager = android::interface_cast<IApplicationManagerService> (
        android::defaultServiceManager()->getService(android::String16("service_layer.ApplicationManagerService")));
    if(mAppManager != nullptr) {
        LOG_I("ApplicationManagerAdapter Registed");
        const android::status_t result{android::IInterface::asBinder(mAppManager)->linkToDeath(mServiceDeathRecipient)};
        if(result == android::OK) {
            LOG_I("LinkToDeath success");
        } else {
            //do nothing
            LOG_I("LinkToDeath fail");
        }
        mSystemReceiver = android::sp<SystemPostReceiver>(new SystemPostReceiver(*this));
        if (mAppManager->getBootCompleted()) { //Get bootcomplete status after register application manager
            if (mIsBootCompleted ==  false) { //if boot status before is false => notify boot complete
                if (mHandler != nullptr) {
                    LOG_I("mAppManager->getBootCompleted MSG_APPL_ON_BOOT_COMPLETED");
                    (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_BOOT_COMPLETED)->sendToTarget();
                    mIsBootCompleted = true; //update local variable
                }
            }
        }
        (void)mAppManager->registerSystemPostReceiver(mSystemReceiver, SYS_POST_ALL);
    } else {
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_APPLICATION_MGR), 
                static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
    }

}

bool ApplicationManagerAdapter::onSystemPostReceived(const android::sp<::Post> &systemPost) {
    LOG_I("What: %d", systemPost->what);
    switch(systemPost->what) {
    case SYS_POST_BOOT_COMPLETED: {
        LOG_I("SYS_POST_BOOT_COMPLETED -> MSG_APPL_ON_BOOT_COMPLETED");
        if (mIsBootCompleted == false) {
            if (mHandler != nullptr) {
                (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_BOOT_COMPLETED)->sendToTarget();
                mIsBootCompleted = true;
            }
        }
        break;
    }
    case SYS_POST_FEATURE_STATUS_CHANGED: {
        LOG_I("SYS_POST_FEATURE_STATUS_CHANGED -> MSG_APPL_ON_FEATURE_STATUS_CHANGED");
        if (mHandler != nullptr) {
            (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_STATUS_CHANGED, systemPost)->sendToTarget();
        }
        break;
    }
    case SYS_POST_FEATURE_ACTION_DELIVERED: {
        LOG_I("SYS_POST_FEATURE_ACTION_DELIVERED -> MSG_APPL_ON_FEATURE_ACTION_DELIVERED");
        if (mHandler != nullptr) {
            LOG_I("Feature name: %s action: %d", systemPost->buffer.data(), systemPost->arg1);
            (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_ACTION_DELIVERED, systemPost)->sendToTarget();
        }
        break;
    }
    case SYS_POST_APP_STATUS_CHANGED: {
        LOG_I("SYS_POST_APP_STATUS_CHANGED -> MSG_APPL_POST_APP_STATUS_CHANGED");
        if (mHandler != nullptr) {
            (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_APPL_POST_APP_STATUS_CHANGED, systemPost)->sendToTarget();
        }
        break;
    }
    default: {
        LOG_I("default");
        break;
    }
    }
    return true;
}
int32_t ApplicationManagerAdapter::queryActionForFeature(const std::string name) {
    LOG_I("Query action for feature name : %s", name.c_str());
    int32_t res{};
    const android::sp<IApplicationManagerService> appMgr{getService()};
    if (appMgr == nullptr){
        res = ParamsDef::UNKNOWN;
    } else {
        const int32_t action{appMgr->queryActionForFeature(name)};
        mRequestedFeature = name;
        switch (action) {
        case FeatureAction::LAUNCH:
            LOG_I("[LAUNCH] feature name: %s", name.c_str());
            break;
        case FeatureAction::UPDATE:
            LOG_I("[UPDATE] feature name: %s", name.c_str());
            break;
        case FeatureAction::POSTPONE:
            LOG_I("[POSTPONE] feature name: %s", name.c_str());
            break;
        case FeatureAction::TRIGGER:
            LOG_I("[TRIGGER] feature name: %s", name.c_str());
            break;
        case FeatureAction::IGNORE:
            LOG_I("[IGNORE] feature name: %s", name.c_str());
            break;
        default:
            LOG_E("[UNKNOWN] Query action failed");
            break;
        }
        res = action;
    }
    return res;
}

int32_t ApplicationManagerAdapter::setFeatureStatus(const std::string appNames, const std::string feaName, const bool onOf) {
    int32_t value{TIGER_ERR::E_ERROR};
    const android::sp<IApplicationManagerService> appMgr{getService()};
    if (appMgr != nullptr)
    {
        value = appMgr->setFeatureStatus(appNames, feaName, (onOf ? FeatureStatus::ON : FeatureStatus::OFF));
    }
    return value;
}

FeatureStatus ApplicationManagerAdapter::getFeatureStatus(const std::string name) {
    FeatureStatus value{FeatureStatus::OFF};
    const android::sp<IApplicationManagerService> appMgr{getService()};
    if (appMgr != nullptr)
    {
        value = appMgr->getFeatureStatus(name);
    }
    return value;
}

void ApplicationManagerAdapter::onBinderDied(const android::wp<android::IBinder>& who) {
    LOG_I("ApplicationManagerAdapter::onBinderDied");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
    NOTUSED(who);
    mAppManager = nullptr;
    mSystemReceiver = nullptr;
    (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_APPLICATION_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
}
}
