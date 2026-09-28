class Mockinstant_func {
};

Mockinstant_func * M_instant_func;

namespace android {

class MockIInterface {
  public:
    MOCK_METHOD0(asBinder_001, sp<IBinder>());
    MOCK_CONST_METHOD0(asBinder_002, sp<const IBinder>());
    MOCK_METHOD1(asBinder_003, sp<IBinder>(const IInterface* iface));
    MOCK_METHOD1(asBinder_004, sp<IBinder>(const sp<IInterface>& iface));
    MOCK_METHOD0(onAsBinder, IBinder*());
};

template <typename T0>
class MockBnInterface {
 public:
  MOCK_METHOD1_T(queryLocalInterface, sp<IInterface>(const String16& _descriptor));
  MOCK_CONST_METHOD0_T(getInterfaceDescriptor, const String16&());
  MOCK_METHOD0_T(onAsBinder, IBinder*());
};

template <typename T0>
class MockBpInterface {
 public:
  MOCK_METHOD0_T(onAsBinder, IBinder*());
};

MockIInterface * M_IInterface;

IInterface::IInterface()
{

}

sp<IBinder> IInterface::asBinder()
{
    return M_IInterface->asBinder_001();
}

sp<const IBinder> IInterface::asBinder() const
{
    return M_IInterface->asBinder_002();
}

sp<IBinder> IInterface::asBinder(const IInterface* iface)
{
    return M_IInterface->asBinder_003(iface);
}

sp<IBinder> IInterface::asBinder(const sp<IInterface>& iface)
{
    return M_IInterface->asBinder_004(iface);
}

IInterface::~IInterface()
{

}

IBinder* IInterface::onAsBinder()
{
    return M_IInterface->onAsBinder();
}


}  // namespace android
