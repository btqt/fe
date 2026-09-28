
class MockOnboardclientManagerService {
 public:
    MOCK_METHOD(uint8_t , getModuleID, ());
    MOCK_METHOD(bool , onInit, ());
    MOCK_METHOD(void , instantiate, ());
    MOCK_METHOD(error_t , onStart, ());
    MOCK_METHOD(error_t , onStop, ());
    MOCK_METHOD(error_t , dump, (LogOutput&));
        
        
    MOCK_METHOD(error_t , registerReceiverOnboardClientReceiverOnNotifyOBD2Event, (
        const android::sp<IOnboardClientReceiver>& receiver
    ));
        
        
    MOCK_METHOD(error_t , registerReceiverOnboardClientReceiverOnResponseEvent, (
        const android::sp<IOnboardClientReceiver>& receiver
    ));

        
        
    MOCK_METHOD(error_t , unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event, (
        const android::sp<IOnboardClientReceiver>& receiver
    ));
        
        
    MOCK_METHOD(error_t , unregisterReceiverOnboardClientReceiverOnResponseEvent, (
        const android::sp<IOnboardClientReceiver>& receiver
    ));

        
    MOCK_METHOD(error_t , queryReceiverByOnboardClientReceiverOnNotifyOBD2Event , (        //  Parm:0
    ));
        
                    
    MOCK_METHOD(error_t , queryReceiverByOnboardClientReceiverOnResponseEvent , (        //  Parm:0
         android::sp< OBCResponseEventInfo >& message           //  Parm:1
    ));

// auto CGA start : wishtoUseAPI
    
                
                
    MOCK_METHOD(error_t , sendUdsData, (
         uint16_t& connectId
        , android::sp<Buffer>& udsRequest
        ));
// auto CGA end : wishtoUseAPI

// RECEIVER TEST API
    
    MOCK_METHOD(error_t , testOnNotifyOBD2Event , (       //  Parm:0
    ));
    
                
    MOCK_METHOD(error_t , testOnResponseEvent , (       //  Parm:0
         android::sp< OBCResponseEventInfo >& message         //  Parm:1
    ));

};



MockOnboardclientManagerService * M_OnboardclientManagerService;


OnboardclientManagerService::OnboardclientManagerService() : SystemService(OnboardclientManagerService::getServiceName())
{

}

OnboardclientManagerService::~OnboardclientManagerService()
{

}

/*
uint8_t OnboardclientManagerService::getModuleID()
{
        return 10;
}
*/

bool OnboardclientManagerService::onInit()
{
    return M_OnboardclientManagerService->onInit();
}

void OnboardclientManagerService::instantiate()
{
}

error_t OnboardclientManagerService::onStart()
{
    return M_OnboardclientManagerService->onStart();
}

error_t OnboardclientManagerService::onStop()
{
    return M_OnboardclientManagerService->onStop();
}

error_t OnboardclientManagerService::dump(LogOutput& log)
{
    return M_OnboardclientManagerService->dump(log);
}

error_t OnboardclientManagerService::registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
)
{
    return M_OnboardclientManagerService->registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        receiver
    );
}
error_t OnboardclientManagerService::registerReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
)
{
    return M_OnboardclientManagerService->registerReceiverOnboardClientReceiverOnResponseEvent(
        receiver
    );
}


error_t OnboardclientManagerService::unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
)
{
    return M_OnboardclientManagerService->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        receiver
    );
}
error_t OnboardclientManagerService::unregisterReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
)
{
    return M_OnboardclientManagerService->unregisterReceiverOnboardClientReceiverOnResponseEvent(
        receiver
    );
}

error_t OnboardclientManagerService::queryReceiverByOnboardClientReceiverOnNotifyOBD2Event(        //  Parm:0
)
{
    return M_OnboardclientManagerService->queryReceiverByOnboardClientReceiverOnNotifyOBD2Event(        //  Parm:0
    );
}
error_t OnboardclientManagerService::queryReceiverByOnboardClientReceiverOnResponseEvent(        //  Parm:0
         android::sp< OBCResponseEventInfo >& message           //  Parm:1
)
{
    return M_OnboardclientManagerService->queryReceiverByOnboardClientReceiverOnResponseEvent(        //  Parm:0
         message           //  Parm:1
    );
}

/*
bool OnboardclientManagerService::isApplicationExecuted(appid_t id)
{
    return M_OnboardclientManagerService->isApplicationExecuted(id);
}
*/

void OnboardclientManagerService::onReceiverBinderDied(const android::wp<android::IBinder>& who)
{
}

// auto CGA start : wishtoUseAPI
error_t OnboardclientManagerService::sendUdsData(
         uint16_t& connectId
        , android::sp<Buffer>& udsRequest
        )
{
    return M_OnboardclientManagerService->sendUdsData(
         connectId
        , udsRequest
        );
}

// auto CGA end : wishtoUseAPI


// receiver test API start
error_t OnboardclientManagerService::testOnNotifyOBD2Event(       //  Parm:0
)
{
        return M_OnboardclientManagerService->testOnNotifyOBD2Event(      //  Parm:0
        );
}

error_t OnboardclientManagerService::testOnResponseEvent(       //  Parm:0
         android::sp< OBCResponseEventInfo >& message         //  Parm:1
)
{
        return M_OnboardclientManagerService->testOnResponseEvent(      //  Parm:0
         message         //  Parm:1
        );
}

// receiver test API end
