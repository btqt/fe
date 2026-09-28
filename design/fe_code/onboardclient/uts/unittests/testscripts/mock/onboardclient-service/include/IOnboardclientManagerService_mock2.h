


class ITestIOnboardclientManagerService : public BnOnboardclientManagerService
{
public:
    //virtual error_t registerReceiverOnboardClientReceiverOnNotifyOBD2Event(serviceid_t id, android::sp<IOnboardclientManagerReceiver>& receiver) {
    //}
    // registerReceiverOnboardClientReceiverOnNotifyOBD2Event : OnboardClientReceiver receiver onNotifyOBD2Event receiver_function
    virtual error_t registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
    )
    {
    }
    //virtual error_t registerReceiverOnboardClientReceiverOnResponseEvent(serviceid_t id, android::sp<IOnboardclientManagerReceiver>& receiver) {
    //}
    // registerReceiverOnboardClientReceiverOnResponseEvent : OnboardClientReceiver receiver onResponseEvent receiver_function
    virtual error_t registerReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
    )
    {
    }

    //virtual error_t unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event( const android::sp<IOnboardclientManagerReceiver>& receiver) {
    //}
    // unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event : OnboardClientReceiver receiver onNotifyOBD2Event receiver_function
    virtual error_t unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
    )
    {
    }
    //virtual error_t unregisterReceiverOnboardClientReceiverOnResponseEvent( const android::sp<IOnboardclientManagerReceiver>& receiver) {
    //}
    // unregisterReceiverOnboardClientReceiverOnResponseEvent : OnboardClientReceiver receiver onResponseEvent receiver_function
    virtual error_t unregisterReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp< IOnboardClientReceiver >& receiver       // IOnboardClientReceiver receiver class
    )
    {
    }

    // auto CGA start : wishtoUseAPI
    virtual error_t sendUdsData(
         uint16_t& connectId
        , android::sp<Buffer>& udsRequest
    )
    {
    }
    // auto CGA end : wishtoUseAPI


    // receiver test API start
    virtual error_t testOnNotifyOBD2Event(         //  Parm:0
        )
    {
    }

    virtual error_t testOnResponseEvent(         //  Parm:0
         android::sp< OBCResponseEventInfo >& message         //  Parm:1
        )
    {
    }

    // receiver test API end


public:
    int32_t change_value = 0;

// CGA_VARIANT:IOnboardclientManagerService:IV2XAntennaManagerService() START
    /*
     * Write your own code
     */
// CGA_VARIANT:IOnboardclientManagerService:IV2XAntennaManagerService() END

};
