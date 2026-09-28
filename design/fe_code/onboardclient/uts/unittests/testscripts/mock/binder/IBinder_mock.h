namespace android {

class MockIBinder {
  public:
    MOCK_METHOD1(queryLocalInterface, sp<IInterface>(const String16& descriptor));
    MOCK_CONST_METHOD0(getInterfaceDescriptor, const String16&());
    MOCK_CONST_METHOD0(isBinderAlive, bool());
    MOCK_METHOD0(pingBinder, status_t());
    MOCK_METHOD2(dump, status_t(int fd, const Vector<String16>& args));
    MOCK_METHOD4(transact, status_t(uint32_t , Parcel , Parcel *, uint32_t ));
    MOCK_METHOD3(linkToDeath, status_t(sp , void *, uint32_t ));
    MOCK_METHOD4(unlinkToDeath, status_t(wp , void *, uint32_t , wp *));
    MOCK_CONST_METHOD1(checkSubclass, bool(const void* subclassID));
//     MOCK_METHOD4(attachObject, void(const void* objectID, void* object, void* cleanupCookie, object_cleanup_func func));
//     MOCK_CONST_METHOD1(findObject, void*(const void* objectID));
//     MOCK_METHOD1(detachObject, void(const void* objectID));
    MOCK_METHOD0(localBinder, BBinder*());
    MOCK_METHOD0(remoteBinder, BpBinder*());
};

MockIBinder * M_IBinder;

IBinder::IBinder()
{

}

sp<IInterface> IBinder::queryLocalInterface(const String16& descriptor)
{
    return M_IBinder->queryLocalInterface(descriptor);
}

const String16& IBinder::getInterfaceDescriptor() const
{
    return M_IBinder->getInterfaceDescriptor();
}

bool IBinder::isBinderAlive() const
{
    return M_IBinder->isBinderAlive();
}

status_t IBinder::pingBinder()
{
    return M_IBinder->pingBinder();
}

status_t IBinder::dump(int fd, const Vector<String16>& args)
{
    return M_IBinder->dump(fd, args);
}

status_t IBinder::transact(uint32_t code, Parcel data, Parcel *reply, uint32_t flags)
{
    return M_IBinder->transact(code, data, reply, flags);
}

status_t IBinder::linkToDeath(sp recipient, void *cookie, uint32_t flags)
{
    return M_IBinder->linkToDeath(recipient, cookie, flags);
}

status_t IBinder::unlinkToDeath(wp recipient, void *cookie, uint32_t flags, wp *outRecipient)
{
    return M_IBinder->unlinkToDeath(recipient, cookie, flags, outRecipient);
}

bool IBinder::checkSubclass(const void* subclassID) const
{
    return M_IBinder->checkSubclass(subclassID);
}

void IBinder::attachObject(const void* objectID, void* object, void* cleanupCookie, object_cleanup_func func)
{
//    M_IBinder->attachObject(objectID, object, cleanupCookie, func);
}

void* IBinder::findObject(const void* objectID) const
{
//    M_IBinder->findObject(objectID);
}

void IBinder::detachObject(const void* objectID)
{
//    M_IBinder->detachObject(objectID);
}

BBinder* IBinder::localBinder()
{
    return M_IBinder->localBinder();
}

BpBinder* IBinder::remoteBinder()
{
    return M_IBinder->remoteBinder();
}

IBinder::~IBinder()
{

}


}  // namespace android
