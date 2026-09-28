using namespace android;

#define APPLICATION_SRV_NAME    "service_layer.ApplicationManagerService"

class AppManager : public SystemService, public BnApplicationManagerService {
public:
    AppManager() :
        SystemService(AppManager::getServiceName())
    {
    }
    virtual ~AppManager();

    const static int TIMER_ID_WATCHDOG = 0;

    enum MsgType {
        MSG_MIN = -1,
        MSG_READY_SERVICES,         // 0
        MSG_POPULATE_PACKAGES,
        MSG_INIT_APPINFO,
        MSG_ENABLE_APP,
        MSG_LAUNCH_APPS,
        MSG_ATTACH_APP,             // 5
        MSG_INVOKE_ONCREATE,
        MSG_INVOKE_ONDESTROY,
        MSG_BROADCAST_SYSTEM_POST,
        MSG_REQUEST_KILL_APP,
        MSG_BROADCAST_HMI_EVENT,    // 10
        MSG_SEND_POST,
        MSG_BROADCAST_POST,
        MSG_KICK_DOG,
        MSG_READY_TO_RUN,
        MSG_REQUEST_ACTIVE,
        MSG_REQUEST_INACTIVE,
        MSG_TRIGGER_EVENT,
        MSG_MAX
    };

    static const char* getServiceName() {return APPLICATION_SRV_NAME;};
    virtual error_t dump(LogOutput& out);
    virtual uint8_t getModuleID() { return MODULE_APPLICATION_MGR; }
    virtual bool onInit();
    virtual error_t onStart();
    virtual error_t onStop();
    virtual void instantiate();

    /**
     * ====================================
     * BnApplicationManagerService methods
     * ====================================
    */
    virtual error_t attachApplication(const char* app, const sp<IBinder> proxy);

    /**
     * Post APIs
     */
    virtual error_t sendPost(const char* app, const sp<Post>& post);
    virtual error_t broadcastPost(const sp<Post>& post);

    /**
     * System Post APIs
    */
    virtual error_t broadcastSystemPost(const sp<Post>& post);
    virtual error_t registerSystemPostReceiver(const sp<ISystemPostReceiver>& receiver, const int32_t post);
    virtual error_t unregisterSystemPostReceiver(const sp<ISystemPostReceiver>& receiver);

    /**
     * Property APIs
     */
    virtual sp<Buffer> getProperty(const char* app, const char* name);
    virtual void setProperty(const char* app, const char* name, const char* value);
    virtual void setProperty(const char* app, const char* name, const int32_t i_value);

    /**
     * Application Management APIs
    */
    virtual error_t enableApplication(const char* app, int32_t reason, bool bIgnoreMode=false);
    virtual error_t disableApplication(const char* app, int32_t reason);
    virtual error_t disableAllApplication(void);
    virtual error_t readyToRun(const char* app);
    virtual bool isEnabled(const char* app);
    virtual error_t ReadyToDestroy(const char* app);
    virtual void setBootCompleted(bool complete);
    virtual bool getBootCompleted(bool bPreComplete = false);

    virtual error_t requestActive(std::string app, const int32_t param, std::string param2 = "");
    virtual error_t requestInactive(std::string app, const int32_t param, std::string param2 = "");
    virtual bool isActive(std::string app);
    virtual error_t triggerEvent(std::string app, const int32_t param, std::string param2 = "");

    /**
     * HMI
    */
    virtual void broadcastHMI(const uint32_t type, const uint32_t action);
#ifndef APP_FEATURE
    virtual error_t setFeature(const std::string app, const std::string feature, const int32_t status);
    virtual int32_t getFeature(const std::string feature);
    virtual FeatureStatus getAction(const std::string feature);
#endif // APP_FEATURE

    // TODO TIGER-240, TO BE DEPRECATED below
    virtual error_t sendPost(const appid_t appid, const sp<Post>& post);
    virtual error_t inquireApplicationStatus(int32_t service_sig);
    virtual error_t enumerateAvailableApplication(bool bReset);
    virtual bool isEnabledApplication(appid_t id);
    virtual error_t ReadyToDestroy(appid_t id);
    virtual sp<Buffer> getProperty(appid_t id, const char* name);
    virtual void setProperty(appid_t id, const char* name, const char* value);
    virtual void setProperty(appid_t id, const char* name, const int32_t i_value);
    // end

    // callback
    error_t onSendPost(const sp<Post>& post);
    void onConfigChanged(std::string key, bool value);
    void onOpModeChanged(const int32_t opMode);
    void onWatchdog();

    void onAppBinderDied(const wp<IBinder>& who);
    void onSysPostBinderDied(const wp<IBinder>& who);

private:
    void initTimer();

#ifdef uts_todo
    sp<IApplication> getApplicationProxy(const char* app);
#endif
    error_t requestToLaunchApp(const std::string& name);
    error_t requestToKillApp(const char* name);
    error_t enableApplicationInternal(const char* app, int32_t reason, bool onChangedConfig = false, bool bIgnoreMode=false);
    error_t disableApplicationInternal(const char* app, int32_t reason, bool onChangedConfig = false);

    void initializeAppInfo();
    bool getAppMode(std::string mode);
    void initializeAppModes();
    void initializeOpModeAdapter();
    void initializeConfigAdapter();
    void initializeManagers();
    void createAppInfo(const std::string app);
    void launchApplications();
    error_t launchApplication(std::string name, const bool onChangedConfig = false, const bool bIgnoreMode=false, std::string launchLable="");
    void notifyAppStatusChange(const char* name, int32_t status, int32_t param = -1);
#ifndef APP_FEATURE
    void notifyFeatureStatusChange(std::string feature, int32_t status);
#endif // APP_FEATURE
    bool checkAllAppsReady();
#ifdef uts_todo
    bool shouldLaunchByBootMode(const Pkg &p);
#endif
    bool checkPriority(const int32_t priority, const int32_t param = -1);
    void beBlocked(std::string event);
    void beUnBlocked(std::string event);
    bool isExcluding(const char* app);

    // handler sub-functions
    void handleReadyServices();
    void handleInvokeOnCreate(const sp<sl::Message>& msg);
    void handleInvokeOnDestroy(const sp<sl::Message>& msg);
    void handleBroadcastSystemPost(const sp<sl::Message>& msg);
    void handleBroadastHmiEvent(const sp<sl::Message>& msg);
    void handleSendPost(const sp<sl::Message>& msg);
    void handleBroadcastPost(const sp<sl::Message>& msg);
    void handleReadyToRun(const sp<sl::Message>& msg);
    void handleRequestActive(const sp<sl::Message>& msg);
    void handleRequestInactive(const sp<sl::Message>& msg);
    void handleTriggerEvent(const sp<sl::Message>& msg);
};

class MockAppManager {
 public:
  MOCK_METHOD1(dump, error_t(LogOutput& out));
  MOCK_METHOD0(onInit, bool());
  MOCK_METHOD0(onStart, error_t());
  MOCK_METHOD0(onStop, error_t());
  MOCK_METHOD2(attachApplication, error_t(const char* app, const sp<IBinder> proxy));
  MOCK_METHOD2(sendPost, error_t(const char* app, const sp<Post>& post));
  MOCK_METHOD1(broadcastPost, error_t(const sp<Post>& post));
  MOCK_METHOD1(broadcastSystemPost, error_t(const sp<Post>& post));
  MOCK_METHOD2(registerSystemPostReceiver, error_t(const sp<ISystemPostReceiver>& receiver, const int32_t post));
  MOCK_METHOD1(unregisterSystemPostReceiver, error_t(const sp<ISystemPostReceiver>& receiver));
  MOCK_METHOD2(getProperty, sp<Buffer>(const char* app, const char* name));
  MOCK_METHOD3(enableApplication, error_t(const char*, int32_t, bool));
  MOCK_METHOD2(disableApplication, error_t(const char* app, int32_t reason));
  MOCK_METHOD0(disableAllApplication, error_t(void));
  MOCK_METHOD1(readyToRun, error_t(const char* app));
  MOCK_METHOD1(isEnabled, bool(const char* app));
  MOCK_METHOD1(ReadyToDestroy, error_t(const char* app));
  MOCK_METHOD1(getBootCompleted, bool(bool bPreComplete));
#ifndef APP_FEATURE
    MOCK_METHOD3( setFeature , error_t (const std::string app, const std::string feature, const int32_t status));
    MOCK_METHOD1( getFeature, int32_t (const std::string feature));
    MOCK_METHOD1( getAction, FeatureStatus (const std::string feature));
#endif // APP_FEATURE
  MOCK_METHOD3(requestActive, error_t(std::string, const int32_t, std::string));
  MOCK_METHOD3(requestInactive, error_t(std::string, const int32_t, std::string));
  MOCK_METHOD1(isActive, bool(std::string app));
  MOCK_METHOD3(triggerEvent, error_t(std::string, const int32_t, std::string));
  MOCK_METHOD2(sendPost, error_t(const appid_t appid, const sp<Post>& post));
  MOCK_METHOD1(inquireApplicationStatus, error_t(int32_t service_sig));
  MOCK_METHOD1(enumerateAvailableApplication, error_t(bool bReset));
  MOCK_METHOD1(isEnabledApplication, bool(appid_t id));
  MOCK_METHOD1(ReadyToDestroy, error_t(appid_t id));
  MOCK_METHOD2(getProperty, sp<Buffer>(appid_t id, const char* name));
  MOCK_METHOD1(onSendPost, error_t(const sp<Post>& post));
#ifdef uts_todo
  MOCK_METHOD1(getApplicationProxy, sp<IApplication>(const char* app));
#endif
  MOCK_METHOD1(requestToLaunchApp, error_t(const std::string& name));
  MOCK_METHOD1(requestToKillApp, error_t(const char* name));
  MOCK_METHOD4(enableApplicationInternal, error_t(const char*, int32_t, bool, bool));
  MOCK_METHOD3(disableApplicationInternal, error_t(const char*, int32_t, bool));
  MOCK_METHOD1(getAppMode, bool(std::string mode));
  MOCK_METHOD4(launchApplication, error_t(std::string, const bool, const bool, std::string));
  MOCK_METHOD0(checkAllAppsReady, bool());
#ifdef uts_todo
  MOCK_METHOD1(shouldLaunchByBootMode, bool(const Pkg &p));
#endif
  MOCK_METHOD2(checkPriority, bool(const int32_t, const int32_t));
};

MockAppManager * M_AppManager;

AppManager::~AppManager()
{

}

error_t AppManager::dump(LogOutput& out)
{
    return M_AppManager->dump(out);
}

bool AppManager::onInit()
{
    std::cout << "AppManager::onInit" << std::endl;
    return M_AppManager->onInit();
}

error_t AppManager::onStart()
{
    std::cout << "AppManager::onStart" << std::endl;
    return M_AppManager->onStart();
}

error_t AppManager::onStop()
{
    return M_AppManager->onStop();
}

void AppManager::instantiate()
{
    std::cout << "AppManager::instantiate" << std::endl;
}

error_t AppManager::attachApplication(const char* app, const sp<IBinder> proxy)
{
    return M_AppManager->attachApplication(app, proxy);
}

error_t AppManager::sendPost(const char* app, const sp<Post>& post)
{
    return M_AppManager->sendPost(app, post);
}

error_t AppManager::broadcastPost(const sp<Post>& post)
{
    return M_AppManager->broadcastPost(post);
}

error_t AppManager::broadcastSystemPost(const sp<Post>& post)
{
    return M_AppManager->broadcastSystemPost(post);
}

error_t AppManager::registerSystemPostReceiver(const sp<ISystemPostReceiver>& receiver, const int32_t post)
{
    return M_AppManager->registerSystemPostReceiver(receiver, post);
}

error_t AppManager::unregisterSystemPostReceiver(const sp<ISystemPostReceiver>& receiver)
{
    return M_AppManager->unregisterSystemPostReceiver(receiver);
}

sp<Buffer> AppManager::getProperty(const char* app, const char* name)
{
    return M_AppManager->getProperty(app, name);
}

void AppManager::setProperty(const char* app, const char* name, const char* value)
{
}

void AppManager::setProperty(const char* app, const char* name, const int32_t i_value)
{
}

error_t AppManager::enableApplication(const char* app, int32_t reason, bool bIgnoreMode)
{
    return M_AppManager->enableApplication(app, reason, bIgnoreMode);
}

error_t AppManager::disableApplication(const char* app, int32_t reason)
{
    return M_AppManager->disableApplication(app, reason);
}

error_t AppManager::disableAllApplication(void)
{
    return M_AppManager->disableAllApplication();
}

error_t AppManager::readyToRun(const char* app)
{
    return M_AppManager->readyToRun(app);
}

bool AppManager::isEnabled(const char* app)
{
    return M_AppManager->isEnabled(app);
}

error_t AppManager::ReadyToDestroy(const char* app)
{
    return M_AppManager->ReadyToDestroy(app);
}

void AppManager::setBootCompleted(bool complete)
{
}

bool AppManager::getBootCompleted(bool bPreComplete = false)
{
    return M_AppManager->getBootCompleted(bPreComplete);
}

#ifndef APP_FEATURE
error_t AppManager::setFeature(const std::string app, const std::string feature, const int32_t status)
{
    return  M_AppManager->setFeature(app, feature, status);
}
int32_t AppManager::getFeature(const std::string feature)
{
    return M_AppManager->getFeature(feature);
}
FeatureStatus AppManager::getAction(const std::string feature)
{
    return M_AppManager->getAction(feature);
}
#endif // APP_FEATURE

error_t AppManager::requestActive(std::string app, const int32_t param, std::string param2)
{
    return M_AppManager->requestActive(app, param, param2);
}

error_t AppManager::requestInactive(std::string app, const int32_t param, std::string param2)
{
    return M_AppManager->requestInactive(app, param, param2);
}

bool AppManager::isActive(std::string app)
{
    return M_AppManager->isActive(app);
}

error_t AppManager::triggerEvent(std::string app, const int32_t param, std::string param2)
{
    return M_AppManager->triggerEvent(app, param, param2);
}

void AppManager::broadcastHMI(const uint32_t type, const uint32_t action)
{
}

error_t AppManager::sendPost(const appid_t appid, const sp<Post>& post)
{
    return M_AppManager->sendPost(appid, post);
}

error_t AppManager::inquireApplicationStatus(int32_t service_sig)
{
    return M_AppManager->inquireApplicationStatus(service_sig);
}

error_t AppManager::enumerateAvailableApplication(bool bReset)
{
    return M_AppManager->enumerateAvailableApplication(bReset);
}

bool AppManager::isEnabledApplication(appid_t id)
{
    return M_AppManager->isEnabledApplication(id);
}

error_t AppManager::ReadyToDestroy(appid_t id)
{
    return M_AppManager->ReadyToDestroy(id);
}

sp<Buffer> AppManager::getProperty(appid_t id, const char* name)
{
    return M_AppManager->getProperty(id, name);
}

void AppManager::setProperty(appid_t id, const char* name, const char* value)
{
}

void AppManager::setProperty(appid_t id, const char* name, const int32_t i_value)
{
}

error_t AppManager::onSendPost(const sp<Post>& post)
{
    return M_AppManager->onSendPost(post);
}

void AppManager::onConfigChanged(std::string key, bool value)
{
}

void AppManager::onWatchdog()
{
}

void AppManager::onAppBinderDied(const wp<IBinder>& who)
{
}

void AppManager::onSysPostBinderDied(const wp<IBinder>& who)
{
}

void AppManager::initTimer()
{
}

#ifdef uts_todo
sp<IApplication> AppManager::getApplicationProxy(const char* app)
{
    return M_AppManager->getApplicationProxy(app);
}
#endif

error_t AppManager::requestToLaunchApp(const std::string& name)
{
    return M_AppManager->requestToLaunchApp(name);
}

error_t AppManager::requestToKillApp(const char* name)
{
    return M_AppManager->requestToKillApp(name);
}

error_t AppManager::enableApplicationInternal(const char* app, int32_t reason, bool onChangedConfig, bool bIgnoreMode)
{
    return M_AppManager->enableApplicationInternal(app, reason, onChangedConfig, bIgnoreMode);
}

error_t AppManager::disableApplicationInternal(const char* app, int32_t reason, bool onChangedConfig)
{
    return M_AppManager->disableApplicationInternal(app, reason, onChangedConfig);
}

void AppManager::initializeAppInfo()
{
}

bool AppManager::getAppMode(std::string mode)
{
    return M_AppManager->getAppMode(mode);
}

void AppManager::initializeAppModes()
{
}

void AppManager::initializeOpModeAdapter()
{
}

void AppManager::initializeConfigAdapter()
{
}

void AppManager::initializeManagers()
{
}

void AppManager::createAppInfo(const std::string app)
{
}

void AppManager::launchApplications()
{
}

error_t AppManager::launchApplication(std::string name, const bool onChangedConfig, const bool bIgnoreMode, std::string launchLable)
{
    return M_AppManager->launchApplication(name, onChangedConfig, bIgnoreMode, launchLable);
}

void AppManager::notifyAppStatusChange(const char* name, int32_t status, int32_t param)
{
}

bool AppManager::checkAllAppsReady()
{
    return M_AppManager->checkAllAppsReady();
}

#ifdef uts_todo
bool AppManager::shouldLaunchByBootMode(const Pkg &p)
{
    return M_AppManager->shouldLaunchByBootMode(p);
}
#endif

bool AppManager::checkPriority(const int32_t priority, const int32_t param)
{
    return M_AppManager->checkPriority(priority, param);
}

void AppManager::beBlocked(std::string event)
{
}

void AppManager::beUnBlocked(std::string event)
{
}

void AppManager::handleReadyServices()
{
}

void AppManager::handleInvokeOnCreate(const sp<sl::Message>& msg)
{
}

void AppManager::handleInvokeOnDestroy(const sp<sl::Message>& msg)
{
}

void AppManager::handleBroadcastSystemPost(const sp<sl::Message>& msg)
{
}

void AppManager::handleBroadastHmiEvent(const sp<sl::Message>& msg)
{
}

void AppManager::handleSendPost(const sp<sl::Message>& msg)
{
}

void AppManager::handleBroadcastPost(const sp<sl::Message>& msg)
{
}

void AppManager::handleReadyToRun(const sp<sl::Message>& msg)
{
}

void AppManager::handleRequestActive(const sp<sl::Message>& msg)
{
}

void AppManager::handleRequestInactive(const sp<sl::Message>& msg)
{
}

void AppManager::handleTriggerEvent(const sp<sl::Message>& msg)
{
}
