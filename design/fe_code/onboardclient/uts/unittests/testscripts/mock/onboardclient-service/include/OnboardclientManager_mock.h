

class MockOnboardclientManager {
 public:
    MOCK_METHOD(OnboardclientManager* , instance, ());
    MOCK_METHOD(error_t , registerReceiverOnboardClientReceiverOnNotifyOBD2Event, (const android::sp<IOnboardClientReceiver>& receiver
    ));
    MOCK_METHOD(error_t , registerReceiverOnboardClientReceiverOnResponseEvent, (const android::sp<IOnboardClientReceiver>& receiver
    ));

    MOCK_METHOD(error_t , unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event, (const android::sp<IOnboardClientReceiver>& receiver
    ));
    MOCK_METHOD(error_t , unregisterReceiverOnboardClientReceiverOnResponseEvent, (const android::sp<IOnboardClientReceiver>& receiver
    ));

    MOCK_METHOD1(error_t , reregisterReceiver, ());


// auto CGA start : wishtoUseAPI
    MOCK_METHOD(error_t , sendUdsData,
        (
             uint16_t connectId
            , android::sp<Buffer> udsRequest
            ));
// auto CGA end : wishtoUseAPI

  MOCK_METHOD(void , onBinderDied, (const android::wp<android::IBinder>& who));
  MOCK_METHOD(android::sp<IOnboardclientManagerService> , getService, ());
  MOCK_METHOD(android::sp<IOnboardclientManagerService> , getService_mock, ());
};



MockOnboardclientManager * M_OnboardclientManager;


OnboardclientManager::~OnboardclientManager()
{

}

OnboardclientManager* OnboardclientManager::instance()
{
    return M_OnboardclientManager->instance();
}

error_t OnboardclientManager::registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
)
{
    return M_OnboardclientManager->registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        receiver
    );
}
error_t OnboardclientManager::registerReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
)
{
    return M_OnboardclientManager->registerReceiverOnboardClientReceiverOnResponseEvent(
        receiver
    );
}

error_t OnboardclientManager::unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
)
{
    return M_OnboardclientManager->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        receiver
    );
}
error_t OnboardclientManager::unregisterReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
)
{
    return M_OnboardclientManager->unregisterReceiverOnboardClientReceiverOnResponseEvent(
        receiver
    );
}

error_t OnboardclientManager::reregisterReceiver()
{
    return M_OnboardclientManager->reregisterReceiver();
}

// auto CGA start : wishtoUseAPI
error_t OnboardclientManager::sendUdsData(
             uint16_t connectId 
            , android::sp<Buffer> udsRequest 
        )
{
    return M_OnboardclientManager->sendUdsData(
         connectId
        ,  udsRequest
        );
}

// auto CGA end : wishtoUseAPI


void OnboardclientManager::onBinderDied(const android::wp<android::IBinder>& who)
{
    //M_OnboardclientManager->onBinderDied(who);
}

OnboardclientManager::OnboardclientManager()
{

}

android::sp<IOnboardclientManagerService> OnboardclientManager::getService()
{
    return M_OnboardclientManager->getService();
}

android::sp<IOnboardclientManagerService> OnboardclientManager::getService_mock()
{
    return M_OnboardclientManager->getService();
}
