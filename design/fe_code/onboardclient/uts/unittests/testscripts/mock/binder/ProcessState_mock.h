namespace android {

class MockProcessState {
  public:
    MOCK_METHOD0(self, sp<ProcessState>());
//     MOCK_METHOD1(setContextObject, void(const sp<IBinder>& object));
    MOCK_METHOD1(getContextObject, sp<IBinder>(const sp<IBinder>& caller));
//     MOCK_METHOD2(setContextObject, void(const sp<IBinder>& object, const String16& name));
    MOCK_METHOD2(getContextObject, sp<IBinder>(const String16& name, const sp<IBinder>& caller));
//     MOCK_METHOD0(startThreadPool, void());
    MOCK_CONST_METHOD0(isContextManager, bool(void));
    MOCK_METHOD2(becomeContextManager, bool(context_check_func checkFunc, void* userData));
    MOCK_METHOD1(getStrongProxyForHandle, sp<IBinder>(int32_t handle));
    MOCK_METHOD1(getWeakProxyForHandle, wp<IBinder>(int32_t handle));
//     MOCK_METHOD2(expungeHandle, void(int32_t handle, IBinder* binder));
//     MOCK_METHOD2(setArgs, void(int argc, const char* const argv[]));
    MOCK_CONST_METHOD0(getArgC, int());
    MOCK_CONST_METHOD0(getArgV, const char const*());
//     MOCK_METHOD1(setArgV0, void(const char* txt));
//     MOCK_METHOD1(spawnPooledThread, void(bool isMain));
    MOCK_METHOD1(setThreadPoolMaxThreadCount, status_t(size_t maxThreads));
//     MOCK_METHOD0(giveThreadPoolName, void());
    MOCK_METHOD0(makeBinderThreadName, String8());
    MOCK_METHOD1(lookupHandleLocked, handle_entry*(int32_t handle));
};

MockProcessState * M_ProcessState;

sp<ProcessState> ProcessState::self()
{
    return M_ProcessState->self();
}

void ProcessState::setContextObject(const sp<IBinder>& object)
{
//    M_ProcessState->setContextObject(object);
}

sp<IBinder> ProcessState::getContextObject(const sp<IBinder>& caller)
{
    return M_ProcessState->getContextObject(caller);
}

void ProcessState::setContextObject(const sp<IBinder>& object, const String16& name)
{
//    M_ProcessState->setContextObject(object, name);
}

sp<IBinder> ProcessState::getContextObject(const String16& name, const sp<IBinder>& caller)
{
    return M_ProcessState->getContextObject(name, caller);
}

void ProcessState::startThreadPool()
{
//    M_ProcessState->startThreadPool();
}

bool ProcessState::isContextManager(void) const
{
    return M_ProcessState->isContextManager();
}

bool ProcessState::becomeContextManager(context_check_func checkFunc, void* userData)
{
    return M_ProcessState->becomeContextManager(checkFunc, userData);
}

sp<IBinder> ProcessState::getStrongProxyForHandle(int32_t handle)
{
    return M_ProcessState->getStrongProxyForHandle(handle);
}

wp<IBinder> ProcessState::getWeakProxyForHandle(int32_t handle)
{
    return M_ProcessState->getWeakProxyForHandle(handle);
}

void ProcessState::expungeHandle(int32_t handle, IBinder* binder)
{
//    M_ProcessState->expungeHandle(handle, binder);
}

void ProcessState::setArgs(int argc, const char* const argv[])
{
//    M_ProcessState->setArgs(argc, argv);
}

int ProcessState::getArgC() const
{
    return M_ProcessState->getArgC();
}

const char const* ProcessState::getArgV() const
{
    return M_ProcessState->getArgV();
}

void ProcessState::setArgV0(const char* txt)
{
//    M_ProcessState->setArgV0(txt);
}

void ProcessState::spawnPooledThread(bool isMain)
{
//    M_ProcessState->spawnPooledThread(isMain);
}

status_t ProcessState::setThreadPoolMaxThreadCount(size_t maxThreads)
{
    return M_ProcessState->setThreadPoolMaxThreadCount(maxThreads);
}

void ProcessState::giveThreadPoolName()
{
//    M_ProcessState->giveThreadPoolName();
}

ProcessState::ProcessState()
{

}

ProcessState::~ProcessState()
{

}

ProcessState::ProcessState(const ProcessState& o)
{

}

String8 ProcessState::makeBinderThreadName()
{
    return M_ProcessState->makeBinderThreadName();
}

handle_entry* ProcessState::lookupHandleLocked(int32_t handle)
{
    return M_ProcessState->lookupHandleLocked(handle);
}


}  // namespace android
