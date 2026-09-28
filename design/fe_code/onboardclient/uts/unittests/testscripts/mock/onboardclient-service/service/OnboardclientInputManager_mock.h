

class MockOnboardclientInputManager {
 public:
  MOCK_METHOD(void ,onServiceBinderDied, (const android::wp<android::IBinder>& who));
  MOCK_METHOD(error_t , init, ());

  MOCK_METHOD0(connectToAppMgr, void(void));
};



MockOnboardclientInputManager * M_OnboardclientInputManager;


void OnboardclientInputManager::onServiceBinderDied(const android::wp<android::IBinder>& who)
{
    M_OnboardclientInputManager->onServiceBinderDied(who);
}
OnboardclientInputManager::OnboardclientInputManager(android::sp<OnboardclientManagerService> mOnboardclientMgrService)
        : mOnboardclientMgrService(mOnboardclientMgrService),  mOnboardclientInputMgrTimer(NULL)
{
    /* auto __ appmgr Inheritance CGA start-------------------------------------------------*/
    mAppManager = NULL;
    mSystemPostReceiver = NULL;
    /* auto __ appmgr Inheritance CGA end-------------------------------------------------*/


}

OnboardclientInputManager::~OnboardclientInputManager()
{

}

error_t OnboardclientInputManager::init()
{
    return M_OnboardclientInputManager->init();
}

void OnboardclientInputManager::connectToAppMgr(void)
{
    M_OnboardclientInputManager->connectToAppMgr();
}
