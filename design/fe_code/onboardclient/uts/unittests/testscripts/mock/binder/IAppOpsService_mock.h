namespace android {

class MockIAppOpsService {
  public:
//     MOCK_METHOD0(DECLARE_META_INTERFACE, void(AppOpsService));
    MOCK_METHOD3(checkOperation, int32_t(int32_t code, int32_t uid, const String16& packageName));
    MOCK_METHOD3(noteOperation, int32_t(int32_t code, int32_t uid, const String16& packageName));
    MOCK_METHOD4(startOperation, int32_t(const sp<IBinder>& token, int32_t code, int32_t uid, const String16& packageName));
//     MOCK_METHOD4(finishOperation, void(const sp<IBinder>& token, int32_t code, int32_t uid, const String16& packageName));
//     MOCK_METHOD3(startWatchingMode, void(int32_t op, const String16& packageName, const sp<IAppOpsCallback>& callback));
//     MOCK_METHOD1(stopWatchingMode, void(const sp<IAppOpsCallback>& callback));
    MOCK_METHOD1(getToken, sp<IBinder>(const sp<IBinder>& clientToken));
};

MockIAppOpsService * M_IAppOpsService;

class MockBnAppOpsService {
  public:
    MOCK_METHOD4(onTransact, status_t(uint32_t , Parcel , Parcel *, uint32_t ));
};

MockBnAppOpsService * M_BnAppOpsService;

void IAppOpsService::DECLARE_META_INTERFACE(AppOpsService)
{
//    M_IAppOpsService->DECLARE_META_INTERFACE(AppOpsService);
}

int32_t IAppOpsService::checkOperation(int32_t code, int32_t uid, const String16& packageName)
{
    return M_IAppOpsService->checkOperation(code, uid, packageName);
}

int32_t IAppOpsService::noteOperation(int32_t code, int32_t uid, const String16& packageName)
{
    return M_IAppOpsService->noteOperation(code, uid, packageName);
}

int32_t IAppOpsService::startOperation(const sp<IBinder>& token, int32_t code, int32_t uid, const String16& packageName)
{
    return M_IAppOpsService->startOperation(token, code, uid, packageName);
}

void IAppOpsService::finishOperation(const sp<IBinder>& token, int32_t code, int32_t uid, const String16& packageName)
{
//    M_IAppOpsService->finishOperation(token, code, uid, packageName);
}

void IAppOpsService::startWatchingMode(int32_t op, const String16& packageName, const sp<IAppOpsCallback>& callback)
{
//    M_IAppOpsService->startWatchingMode(op, packageName, callback);
}

void IAppOpsService::stopWatchingMode(const sp<IAppOpsCallback>& callback)
{
//    M_IAppOpsService->stopWatchingMode(callback);
}

sp<IBinder> IAppOpsService::getToken(const sp<IBinder>& clientToken)
{
    return M_IAppOpsService->getToken(clientToken);
}


status_t BnAppOpsService::onTransact(uint32_t code, Parcel data, Parcel *reply, uint32_t flags)
{
    return M_BnAppOpsService->onTransact(code, data, reply, flags);
}


}  // namespace android
