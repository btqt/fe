#include "ApplicationManagerAdapter.h"
#include "../utils/ProxyIpcClient.h"
#include <sstream>

// namespace rdgapp {
ApplicationManagerAdapter::ApplicationManagerAdapter() {
    mIsBootCompleted = false;
    mServiceDeathRecipient = new ServiceDeathRecipient( [this] ( const android::wp<android::IBinder>& who ){
        this->onBinderDied(who);
    });
    
    // Create and register IPC command handler
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
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
    const android::Mutex::Autolock lock{mAppManagerLock};
    return mAppManager;
}

bool ApplicationManagerAdapter::getBootCompleted() const noexcept {
    const android::Mutex::Autolock lock{mBootCompletedLock};
    return mIsBootCompleted;
}

void ApplicationManagerAdapter::registerService() {
    LOG_I("Start register ApplicationManagerAdapter");
    mHandler = RemotediagProxyHandler::getInstance();
    android::sp<IApplicationManagerService> appManager{nullptr};
    {
        const android::Mutex::Autolock lock{mAppManagerLock};
        if (mAppManager != nullptr) {
            const android::sp<android::IBinder> binder{android::IInterface::asBinder(mAppManager)};
            if (binder != nullptr) {
                (void)binder->unlinkToDeath(mServiceDeathRecipient);
            }
            mAppManager = nullptr;
            mSystemReceiver = nullptr;
        }
        appManager = android::interface_cast<IApplicationManagerService> (
            android::defaultServiceManager()->getService(android::String16("service_layer.ApplicationManagerService")));
        mAppManager = appManager;
    }
    if(appManager != nullptr) {
        LOG_I("ApplicationManagerAdapter Registed");
        const android::status_t result{android::IInterface::asBinder(appManager)->linkToDeath(mServiceDeathRecipient)};
        if(result == android::OK) {
            LOG_I("LinkToDeath success");
        } else {
            LOG_I("LinkToDeath fail");
            const android::Mutex::Autolock lock{mAppManagerLock};
            mAppManager = nullptr;
            mSystemReceiver = nullptr;
            if (mHandler != nullptr) {
                (void)mHandler->sendMessageDelayed(
                    mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_APPLICATION_MGR),
                    static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
            }
            return;
        }
        const android::sp<ISystemPostReceiver> receiver{new SystemPostReceiver(*this)};
        {
            const android::Mutex::Autolock lock{mAppManagerLock};
            mSystemReceiver = receiver;
        }
        if (appManager->getBootCompleted()) { //Get bootcomplete status after register application manager
            const android::Mutex::Autolock lock{mBootCompletedLock};
            if (mIsBootCompleted ==  false) { //if boot status before is false => notify boot complete
                if (mHandler != nullptr) {
                    LOG_I("mAppManager->getBootCompleted MSG_APPL_ON_BOOT_COMPLETED");
                    (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_BOOT_COMPLETED)->sendToTarget();
                    mIsBootCompleted = true; //update local variable
                }
            }
        }
        (void)appManager->registerSystemPostReceiver(receiver, SYS_POST_ALL);
    } else {
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_APPLICATION_MGR), 
                static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
    }

}

bool ApplicationManagerAdapter::onSystemPostReceived(const android::sp<::Post> &systemPost) {
    if (systemPost == nullptr) {
        LOG_W("ApplicationManagerAdapter received a null system post");
        return false;
    }

    LOG_I("What: %d", systemPost->what);
    switch(systemPost->what) {
    case SYS_POST_BOOT_COMPLETED: {
        LOG_I("SYS_POST_BOOT_COMPLETED -> MSG_APPL_ON_BOOT_COMPLETED");
        const android::Mutex::Autolock lock{mBootCompletedLock};
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
    {
        const android::Mutex::Autolock appManagerLock{mAppManagerLock};
        mAppManager = nullptr;
        mSystemReceiver = nullptr;
    }
    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_APPLICATION_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

ApplicationManagerAdapter::CommandHandler::CommandHandler(ApplicationManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("ApplicationManagerAdapter::CommandHandler: created");
}

void ApplicationManagerAdapter::CommandHandler::initialize() {
    LOG_I("ApplicationManagerAdapter::CommandHandler: registering with ProxyIpcClient");
    
    // Register with the specific command IDs this handler supports
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::ApplicationGetBootCompleted),
        static_cast<uint32_t>(rdgipc::CommandId::ApplicationQueryAction),
        static_cast<uint32_t>(rdgipc::CommandId::ApplicationSetFeatureStatus),
        static_cast<uint32_t>(rdgipc::CommandId::ApplicationGetFeatureStatus)
    };
    
    // Auto-register this handler with ProxyIpcClient
    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("ApplicationManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("ApplicationManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

ApplicationManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("ApplicationManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse ApplicationManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);
    
    LOG_I("ApplicationManagerAdapter::CommandHandler: handling command=%s id=%u payloadSize=%zu",
          rdgipc::commandName(cmdId), commandId, payload.size());
    
    switch (cmdId) {
    case rdgipc::CommandId::ApplicationGetBootCompleted:
        return handleGetBootCompleted(payload);
    case rdgipc::CommandId::ApplicationQueryAction:
        return handleQueryActionForFeature(payload);
    case rdgipc::CommandId::ApplicationSetFeatureStatus:
        return handleSetFeatureStatus(payload);
    case rdgipc::CommandId::ApplicationGetFeatureStatus:
        return handleGetFeatureStatus(payload);
    default:
        LOG_W("ApplicationManagerAdapter::CommandHandler: unsupported command id=%u", commandId);
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse ApplicationManagerAdapter::CommandHandler::handleGetBootCompleted(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    return rdgipc::CommandResponse::ok(mAdapter->getBootCompleted() ? 1 : 0);
}

rdgipc::CommandResponse ApplicationManagerAdapter::CommandHandler::handleQueryActionForFeature(const std::vector<uint8_t> &payload) {
    const std::string featureName = rdgipc::toString(payload);
    const int32_t action = mAdapter->queryActionForFeature(featureName);
    return rdgipc::CommandResponse::ok(action);
}

rdgipc::CommandResponse ApplicationManagerAdapter::CommandHandler::handleSetFeatureStatus(const std::vector<uint8_t> &payload) {
    // Payload format: "appNames,feaName,onOff"
    const std::string payloadStr = rdgipc::toString(payload);
    std::vector<std::string> parts;
    std::stringstream ss(payloadStr);
    std::string part;
    while (std::getline(ss, part, ',')) {
        parts.push_back(part);
    }
    
    if (parts.size() != 3) {
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::InvalidPayload);
    }
    
    const bool onOff = (parts[2] == "1" || parts[2] == "true");
    const int32_t result = mAdapter->setFeatureStatus(parts[0], parts[1], onOff);
    return rdgipc::CommandResponse::ok(result);
}

rdgipc::CommandResponse ApplicationManagerAdapter::CommandHandler::handleGetFeatureStatus(const std::vector<uint8_t> &payload) {
    const std::string featureName = rdgipc::toString(payload);
    const FeatureStatus status = mAdapter->getFeatureStatus(featureName);
    return rdgipc::CommandResponse::ok(static_cast<int32_t>(status));
}
// }
