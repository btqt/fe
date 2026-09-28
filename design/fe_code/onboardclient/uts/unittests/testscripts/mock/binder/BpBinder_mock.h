#include <binder/BpBinder.h>

namespace android {

class MockBpBinder : public IBinder{
 public:
  MOCK_CONST_METHOD0(handle, int32_t());
  MOCK_CONST_METHOD0(getInterfaceDescriptor, const String16&());
  MOCK_CONST_METHOD0(isBinderAlive, bool());
  MOCK_METHOD0(pingBinder, status_t());
  MOCK_METHOD2(dump, status_t(int fd, const Vector<String16>& args));
  MOCK_METHOD4(transact, status_t(uint32_t, const Parcel&, Parcel*, uint32_t));
  MOCK_METHOD3(linkToDeath, status_t(const sp<DeathRecipient>&, void*, uint32_t));
  MOCK_METHOD4(unlinkToDeath, status_t(const wp<DeathRecipient>&, void*, uint32_t, wp<DeathRecipient>*));
  MOCK_CONST_METHOD1(findObject, void*(const void* objectID));
  MOCK_METHOD0(remoteBinder, BpBinder*());
  MOCK_METHOD2(setConstantData, status_t(const void* data, size_t size));
  MOCK_METHOD2(onIncStrongAttempted, bool(uint32_t flags, const void* id));
  MOCK_CONST_METHOD0(isDescriptorCached, bool());
};

MockBpBinder * M_BpBinder;
BpBinder::ObjectManager::ObjectManager()
{
}

BpBinder::ObjectManager::~ObjectManager()
{
}

void BpBinder::ObjectManager::attach(
    const void* objectID, void* object, void* cleanupCookie,
    IBinder::object_cleanup_func func)
{
}

void* BpBinder::ObjectManager::find(const void* objectID) const
{
    return NULL;
}

void BpBinder::ObjectManager::detach(const void* objectID)
{
}

void BpBinder::ObjectManager::kill()
{
}


BpBinder::BpBinder(int32_t handle)
    : mHandle(handle)
    , mAlive(1)
    , mObitsSent(0)
    , mObituaries(NULL)
{

}

const String16& BpBinder::getInterfaceDescriptor() const
{
    return M_BpBinder->getInterfaceDescriptor();
}

bool BpBinder::isBinderAlive() const
{
    return M_BpBinder->isBinderAlive();
}

status_t BpBinder::pingBinder()
{
    return M_BpBinder->pingBinder();
}

status_t BpBinder::dump(int fd, const Vector<String16>& args)
{
    return M_BpBinder->dump(fd, args);
}

status_t BpBinder::transact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    return M_BpBinder->transact(code, data, reply, flags);
}

status_t BpBinder::linkToDeath(const sp<DeathRecipient>& recipient, void* cookie, uint32_t flags)
{
    return M_BpBinder->linkToDeath(recipient, cookie, flags);
}

status_t BpBinder::unlinkToDeath(const wp<DeathRecipient>& recipient, void* cookie,
        uint32_t flags, wp<DeathRecipient>* outRecipient)
{
    return M_BpBinder->unlinkToDeath(recipient, cookie, flags, outRecipient);
}

void BpBinder::attachObject(const void* objectID, void* object, void* cleanupCookie, object_cleanup_func func)
{
}

void* BpBinder::findObject(const void* objectID) const
{
    M_BpBinder->findObject(objectID);
}

void BpBinder::detachObject(const void* objectID)
{
}

BpBinder* BpBinder::remoteBinder()
{
    return M_BpBinder->remoteBinder();
}

status_t BpBinder::setConstantData(const void* data, size_t size)
{
    return M_BpBinder->setConstantData(data, size);
}

void BpBinder::sendObituary()
{
}

BpBinder::~BpBinder()
{

}

void BpBinder::onFirstRef()
{
}

void BpBinder::onLastStrongRef(const void* id)
{
}

bool BpBinder::onIncStrongAttempted(uint32_t flags, const void* id)
{
    return M_BpBinder->onIncStrongAttempted(flags, id);
}

void BpBinder::reportOneDeath(const Obituary& obit)
{
}

bool BpBinder::isDescriptorCached() const
{
    return M_BpBinder->isDescriptorCached();
}

}  // namespace android
