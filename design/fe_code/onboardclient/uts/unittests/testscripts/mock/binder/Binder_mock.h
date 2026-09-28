namespace android {

class MockBBinder {
 public:
  MOCK_CONST_METHOD0(getInterfaceDescriptor, const String16&());
  MOCK_CONST_METHOD0(isBinderAlive, bool());
  MOCK_METHOD0(pingBinder, status_t());
  MOCK_METHOD2(dump, status_t(int fd, const Vector<String16>& args));
  MOCK_METHOD4(transact, status_t(uint32_t, const Parcel&, Parcel*, uint32_t));
  MOCK_METHOD3(linkToDeath, status_t(const sp<IBinder::DeathRecipient>&, void*, uint32_t));
  MOCK_METHOD4(unlinkToDeath, status_t(const wp<IBinder::DeathRecipient>&, void*, uint32_t, wp<IBinder::DeathRecipient>*));
  MOCK_CONST_METHOD1(findObject, void*(const void* objectID));
  MOCK_METHOD0(localBinder, BBinder*());
  MOCK_METHOD4(onTransact, status_t(uint32_t, const Parcel&, Parcel*, uint32_t));
};

MockBBinder * M_BBinder;

class MockBpRefBase {
 public:
  MOCK_METHOD2(onIncStrongAttempted, bool(uint32_t flags, const void* id));
};
MockBpRefBase * M_BpRefBase;

IBinder::IBinder()
    : RefBase()
{
}

IBinder::~IBinder()
{
}

// ---------------------------------------------------------------------------

sp<IInterface>  IBinder::queryLocalInterface(const String16& /*descriptor*/)
{
    return NULL;
}

BBinder* IBinder::localBinder()
{
    return NULL;
}

BpBinder* IBinder::remoteBinder()
{
    return NULL;
}

bool IBinder::checkSubclass(const void* /*subclassID*/) const
{
    return false;
}
BBinder::BBinder()
{

}

const String16& BBinder::getInterfaceDescriptor() const
{
    return M_BBinder->getInterfaceDescriptor();
}

bool BBinder::isBinderAlive() const
{
    return M_BBinder->isBinderAlive();
}

status_t BBinder::pingBinder()
{
    return M_BBinder->pingBinder();
}

status_t BBinder::dump(int fd, const Vector<String16>& args)
{
    return M_BBinder->dump(fd, args);
}

status_t BBinder::transact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    return M_BBinder->transact(code, data, reply, flags);
}

status_t BBinder::linkToDeath(const sp<DeathRecipient>& recipient, void* cookie, uint32_t flags)
{
    return M_BBinder->linkToDeath(recipient, cookie, flags);
}

status_t BBinder::unlinkToDeath(const wp<DeathRecipient>& recipient, void* cookie, uint32_t flags, wp<DeathRecipient>* outRecipient)
{
    return M_BBinder->unlinkToDeath(recipient, cookie, flags, outRecipient);
}

void BBinder::attachObject(const void* objectID, void* object, void* cleanupCookie, object_cleanup_func func)
{
}

void * BBinder::findObject(const void* objectID) const
{
    return M_BBinder->findObject(objectID);
}

void BBinder::detachObject(const void* objectID)
{
}

BBinder* BBinder::localBinder()
{
    return M_BBinder->localBinder();
}

BBinder::~BBinder()
{

}

status_t BBinder::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    return M_BBinder->onTransact(code, data, reply, flags);
}

BBinder::BBinder(const BBinder& o)
{

}


BpRefBase::BpRefBase(const sp<IBinder>& o)
{

}

BpRefBase::~BpRefBase()
{

}

void BpRefBase::onFirstRef()
{
}

void BpRefBase::onLastStrongRef(const void* id)
{
}

bool BpRefBase::onIncStrongAttempted(uint32_t flags, const void* id)
{
    return M_BpRefBase->onIncStrongAttempted(flags, id);
}

}  // namespace android
