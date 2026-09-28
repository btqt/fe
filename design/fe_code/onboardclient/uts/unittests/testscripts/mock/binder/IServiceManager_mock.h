namespace android {

class MockIServiceManager {
 public:
  MOCK_CONST_METHOD1(getService, sp<IBinder>(const String16&));
  MOCK_CONST_METHOD1(checkService, sp<IBinder>(const String16& name));
  MOCK_METHOD3(addService, status_t(const String16&, const sp<IBinder>&, bool));
  MOCK_METHOD0(listServices, Vector<String16>());
};

MockIServiceManager * M_IServiceManager;

class MockBnServiceManager {
 public:
  MOCK_METHOD4(onTransact, status_t(uint32_t, const Parcel&, Parcel*, uint32_t));
};

MockBnServiceManager * M_BnServiceManager;

sp<IServiceManager> defaultServiceManager()
{
    std::cout << "defaultServiceManager.1" << std::endl;
    IBinder* pBinder = new BpBinder(0);
    sp<IServiceManager> retVal = interface_cast<IServiceManager>(pBinder);

    std::cout << "defaultServiceManager.2 ret=" << (retVal != NULL ) << std::endl;

    return retVal;
}

// ----------------------------------------------------------------------

class BpServiceManager : public BpInterface<IServiceManager>
{
public:
    BpServiceManager(const sp<IBinder>& impl)
        : BpInterface<IServiceManager>(impl)
    {
    }

    virtual sp<IBinder> getService(const String16& name) const
    {
        return M_IServiceManager->getService(name);
    }

    virtual sp<IBinder> checkService( const String16& name) const
    {
        return M_IServiceManager->checkService(name);
    }

    virtual status_t addService(const String16& name, const sp<IBinder>& service,
            bool allowIsolated)
    {
        return M_IServiceManager->addService(name, service, allowIsolated);
    }

    virtual Vector<String16> listServices()
    {
        return M_IServiceManager->listServices();
    }
};

IMPLEMENT_META_INTERFACE(ServiceManager, "android.os.IServiceManager");

// ----------------------------------------------------------------------


status_t BnServiceManager::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    return M_BnServiceManager->onTransact(code, data, reply, flags);
}

}  // namespace android
