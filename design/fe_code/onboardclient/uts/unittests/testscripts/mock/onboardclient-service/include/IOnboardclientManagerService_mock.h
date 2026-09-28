
class MockIOnboardclientManagerService {
 public:
    MOCK_METHOD(error_t , registerReceiverOnboardClientReceiverOnNotifyOBD2Event, (const android::sp<IOnboardClientReceiver>& receiver
    ));
    MOCK_METHOD(error_t , registerReceiverOnboardClientReceiverOnResponseEvent, (const android::sp<IOnboardClientReceiver>& receiver
    ));

    MOCK_METHOD(error_t , unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event, (const android::sp<IOnboardClientReceiver>& receiver
    ));
    MOCK_METHOD(error_t , unregisterReceiverOnboardClientReceiverOnResponseEvent, (const android::sp<IOnboardClientReceiver>& receiver
    ));

// auto CGA start : wishtoUseAPI
    MOCK_METHOD(error_t , sendUdsData,
        (
         uint16_t& connectId
        , android::sp<Buffer>& udsRequest
        ));
// auto CGA end : wishtoUseAPI


// RECEIVER TEST API
    MOCK_METHOD(error_t , testOnNotifyOBD2Event , (       // Parm:0
        ));
    MOCK_METHOD(error_t , testOnResponseEvent , (       // Parm:0
         android::sp< OBCResponseEventInfo >& message         // Parm:1
        ));


};

class MockBnOnboardclientManagerService {
 public:
  MOCK_METHOD(android::status_t , onTransact, (uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags));
};



MockIOnboardclientManagerService * M_IOnboardclientManagerService;

MockBnOnboardclientManagerService * M_BnOnboardclientManagerService;

android::status_t BnOnboardclientManagerService::onTransact(uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags)
{
    return M_BnOnboardclientManagerService->onTransact(code, data, reply, flags);
}


class ITestIOnboardclientManagerService : public BnOnboardclientManagerService
{
public:
    virtual error_t registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
    )
    {
        return M_IOnboardclientManagerService->registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
            receiver
            );
    }
    virtual error_t registerReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
    )
    {
        return M_IOnboardclientManagerService->registerReceiverOnboardClientReceiverOnResponseEvent(
            receiver
            );
    }

    virtual error_t unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
    )
    {
        return M_IOnboardclientManagerService->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
            receiver
            );
    }
    virtual error_t unregisterReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
    )
    {
        return M_IOnboardclientManagerService->unregisterReceiverOnboardClientReceiverOnResponseEvent(
            receiver
            );
    }

    // auto CGA start : wishtoUseAPI
    virtual error_t sendUdsData(
         uint16_t& connectId
        , android::sp<Buffer>& udsRequest
    )
    {
        return M_IOnboardclientManagerService->sendUdsData(
             connectId
            , udsRequest
            );
    }
    // auto CGA end : wishtoUseAPI


    // receiver test API start
    virtual error_t testOnNotifyOBD2Event(     // Parm:0
        )
    {
        return M_IOnboardclientManagerService->testOnNotifyOBD2Event(     // Parm:0
        );
    }

    virtual error_t testOnResponseEvent(     // Parm:0
         android::sp< OBCResponseEventInfo >& message         // Parm:1
        )
    {
        return M_IOnboardclientManagerService->testOnResponseEvent(     // Parm:0
         message         // Parm:1
        );
    }

    // receiver test API end


// CGA_VARIANT:IOnboardclientManagerService:IOnboardclientManagerService() START
    /*
     * Write your own code
     */
// CGA_VARIANT:IOnboardclientManagerService:IOnboardclientManagerService() END

};
