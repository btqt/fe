
#ifdef uts_remove
class MockIOnboardClientReceiver {
 public:
    MOCK_METHOD(void , DECLARE_META_INTERFACE, (OnboardClientReceiver));

    MOCK_METHOD(void , onNotifyOBD2Event , (   //  P0 : 0
        ),(override));
    MOCK_METHOD(void , onResponseEvent , (   //  P0 : 0
        android::sp< OBCResponseEventInfo >& message         // 0  argument  
        ),(override));
};

class MockBnOnboardClientReceiver {
 public:
  MOCK_METHOD(android::status_t , onTransact, (uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags), (override));
};



MockIOnboardClientReceiver * M_IOnboardClientReceiver;


void IOnboardClientReceiver::DECLARE_META_INTERFACE(OnboardClientReceiver)
{
    M_IOnboardClientReceiver->DECLARE_META_INTERFACE(OnboardClientReceiver);
}

void IOnboardClientReceiver::onNotifyOBD2Event( //  P0 : 0
)
{
    M_IOnboardClientReceiver->onNotifyOBD2Event(      //  P0 : 0
    );


}
void IOnboardClientReceiver::onResponseEvent( //  P0 : 0
        android::sp< OBCResponseEventInfo >& message         // 0  argument  
)
{
    M_IOnboardClientReceiver->onResponseEvent(      //  P0 : 0
         message            // 0  argument  
    );


}

android::status_t BnOnboardClientReceiver::onTransact(uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags)
{
    return M_BnOnboardClientReceiver->onTransact(code, data, reply, flags);
}
#endif

class ITestOnboardClientReceiver: public BnOnboardClientReceiver
{

public:

    virtual void onNotifyOBD2Event(      //  P0 : 0
        )
    {

    }
    virtual void onResponseEvent(      //  P0 : 0
        android::sp< OBCResponseEventInfo >& message         // 0  argument  
        )
    {

    }

};
