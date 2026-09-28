namespace android {

class MockIPCThreadState {
  public:
    MOCK_METHOD0(self, IPCThreadState*());
    MOCK_METHOD0(selfOrNull, IPCThreadState*());
    MOCK_METHOD0(process, sp<ProcessState>());
    MOCK_METHOD0(clearLastError, status_t());
    MOCK_CONST_METHOD0(getCallingPid, int());
    MOCK_CONST_METHOD0(getCallingUid, int());
//     MOCK_METHOD1(setStrictModePolicy, void(int32_t policy));
    MOCK_CONST_METHOD0(getStrictModePolicy, int32_t());
//     MOCK_METHOD1(setLastTransactionBinderFlags, void(int32_t flags));
    MOCK_CONST_METHOD0(getLastTransactionBinderFlags, int32_t());
    MOCK_METHOD0(clearCallingIdentity, int64_t());
//     MOCK_METHOD1(restoreCallingIdentity, void(int64_t token));
    MOCK_METHOD1(setupPolling, int(int* fd));
    MOCK_METHOD0(handlePolledCommands, status_t());
//     MOCK_METHOD0(flushCommands, void());
//     MOCK_METHOD1(joinThreadPool, void(bool ));
//     MOCK_METHOD1(stopProcess, void(bool ));
    MOCK_METHOD5(transact, status_t(int32_t handle, uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags));
//     MOCK_METHOD1(incStrongHandle, void(int32_t handle));
//     MOCK_METHOD1(decStrongHandle, void(int32_t handle));
//     MOCK_METHOD1(incWeakHandle, void(int32_t handle));
//     MOCK_METHOD1(decWeakHandle, void(int32_t handle));
    MOCK_METHOD1(attemptIncStrongHandle, status_t(int32_t handle));
//     MOCK_METHOD2(expungeHandle, void(int32_t handle, IBinder* binder));
    MOCK_METHOD2(requestDeathNotification, status_t(int32_t handle, BpBinder* proxy));
    MOCK_METHOD2(clearDeathNotification, status_t(int32_t handle, BpBinder* proxy));
//     MOCK_METHOD0(shutdown, void());
//     MOCK_METHOD1(disableBackgroundScheduling, void(bool disable));
    MOCK_METHOD2(sendReply, status_t(const Parcel& reply, uint32_t flags));
    MOCK_METHOD2(waitForResponse, status_t(Parcel *, status_t *));
    MOCK_METHOD1(talkWithDriver, status_t(bool ));
    MOCK_METHOD6(writeTransactionData, status_t(int32_t cmd, uint32_t binderFlags, int32_t handle, uint32_t code, const Parcel& data, status_t* statusBuffer));
    MOCK_METHOD0(getAndExecuteCommand, status_t());
    MOCK_METHOD1(executeCommand, status_t(int32_t command));
//     MOCK_METHOD0(processPendingDerefs, void());
//     MOCK_METHOD0(clearCaller, void());
//     MOCK_METHOD1(threadDestructor, void(void *st));
//     MOCK_METHOD6(freeBuffer, void(Parcel* parcel, const uint8_t* data, size_t dataSize, const size_t* objects, size_t objectsSize, void* cookie));
};

MockIPCThreadState * M_IPCThreadState;

IPCThreadState* IPCThreadState::self()
{
    return M_IPCThreadState->self();
}

IPCThreadState* IPCThreadState::selfOrNull()
{
    return M_IPCThreadState->selfOrNull();
}

sp<ProcessState> IPCThreadState::process()
{
    return M_IPCThreadState->process();
}

status_t IPCThreadState::clearLastError()
{
    return M_IPCThreadState->clearLastError();
}

int IPCThreadState::getCallingPid() const
{
    return M_IPCThreadState->getCallingPid();
}

int IPCThreadState::getCallingUid() const
{
    return M_IPCThreadState->getCallingUid();
}

void IPCThreadState::setStrictModePolicy(int32_t policy)
{
//    M_IPCThreadState->setStrictModePolicy(policy);
}

int32_t IPCThreadState::getStrictModePolicy() const
{
    return M_IPCThreadState->getStrictModePolicy();
}

void IPCThreadState::setLastTransactionBinderFlags(int32_t flags)
{
//    M_IPCThreadState->setLastTransactionBinderFlags(flags);
}

int32_t IPCThreadState::getLastTransactionBinderFlags() const
{
    return M_IPCThreadState->getLastTransactionBinderFlags();
}

int64_t IPCThreadState::clearCallingIdentity()
{
    return M_IPCThreadState->clearCallingIdentity();
}

void IPCThreadState::restoreCallingIdentity(int64_t token)
{
//    M_IPCThreadState->restoreCallingIdentity(token);
}

int IPCThreadState::setupPolling(int* fd)
{
    return M_IPCThreadState->setupPolling(fd);
}

status_t IPCThreadState::handlePolledCommands()
{
    return M_IPCThreadState->handlePolledCommands();
}

void IPCThreadState::flushCommands()
{
//    M_IPCThreadState->flushCommands();
}

void IPCThreadState::joinThreadPool(bool isMain)
{
//    M_IPCThreadState->joinThreadPool(isMain);
}

void IPCThreadState::stopProcess(bool immediate)
{
//    M_IPCThreadState->stopProcess(immediate);
}

status_t IPCThreadState::transact(int32_t handle, uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    return M_IPCThreadState->transact(handle, code, data, reply, flags);
}

void IPCThreadState::incStrongHandle(int32_t handle)
{
//    M_IPCThreadState->incStrongHandle(handle);
}

void IPCThreadState::decStrongHandle(int32_t handle)
{
//    M_IPCThreadState->decStrongHandle(handle);
}

void IPCThreadState::incWeakHandle(int32_t handle)
{
//    M_IPCThreadState->incWeakHandle(handle);
}

void IPCThreadState::decWeakHandle(int32_t handle)
{
//    M_IPCThreadState->decWeakHandle(handle);
}

status_t IPCThreadState::attemptIncStrongHandle(int32_t handle)
{
    return M_IPCThreadState->attemptIncStrongHandle(handle);
}

void IPCThreadState::expungeHandle(int32_t handle, IBinder* binder)
{
//    M_IPCThreadState->expungeHandle(handle, binder);
}

status_t IPCThreadState::requestDeathNotification(int32_t handle, BpBinder* proxy)
{
    return M_IPCThreadState->requestDeathNotification(handle, proxy);
}

status_t IPCThreadState::clearDeathNotification(int32_t handle, BpBinder* proxy)
{
    return M_IPCThreadState->clearDeathNotification(handle, proxy);
}

void IPCThreadState::shutdown()
{
//    M_IPCThreadState->shutdown();
}

void IPCThreadState::disableBackgroundScheduling(bool disable)
{
//    M_IPCThreadState->disableBackgroundScheduling(disable);
}

IPCThreadState::IPCThreadState()
{

}

IPCThreadState::~IPCThreadState()
{

}

status_t IPCThreadState::sendReply(const Parcel& reply, uint32_t flags)
{
    return M_IPCThreadState->sendReply(reply, flags);
}

status_t IPCThreadState::waitForResponse(Parcel *reply, status_t *acquireResult)
{
    return M_IPCThreadState->waitForResponse(reply, acquireResult);
}

status_t IPCThreadState::talkWithDriver(bool doReceive)
{
    return M_IPCThreadState->talkWithDriver(doReceive);
}

status_t IPCThreadState::writeTransactionData(int32_t cmd, uint32_t binderFlags, int32_t handle, uint32_t code, const Parcel& data, status_t* statusBuffer)
{
    return M_IPCThreadState->writeTransactionData(cmd, binderFlags, handle, code, data, statusBuffer);
}

status_t IPCThreadState::getAndExecuteCommand()
{
    return M_IPCThreadState->getAndExecuteCommand();
}

status_t IPCThreadState::executeCommand(int32_t command)
{
    return M_IPCThreadState->executeCommand(command);
}

void IPCThreadState::processPendingDerefs()
{
//    M_IPCThreadState->processPendingDerefs();
}

void IPCThreadState::clearCaller()
{
//    M_IPCThreadState->clearCaller();
}

void IPCThreadState::threadDestructor(void *st)
{
//    M_IPCThreadState->threadDestructor();
}

void IPCThreadState::freeBuffer(Parcel* parcel, const uint8_t* data, size_t dataSize, const size_t* objects, size_t objectsSize, void* cookie)
{
//    M_IPCThreadState->freeBuffer(parcel, data, dataSize, objects, objectsSize, cookie);
}


}  // namespace android
