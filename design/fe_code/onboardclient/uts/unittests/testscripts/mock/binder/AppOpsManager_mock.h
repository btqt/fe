namespace android {

class MockAppOpsManager {
  public:
    MOCK_METHOD3(checkOp, int32_t(int32_t op, int32_t uid, const String16& callingPackage));
    MOCK_METHOD3(noteOp, int32_t(int32_t op, int32_t uid, const String16& callingPackage));
    MOCK_METHOD3(startOp, int32_t(int32_t op, int32_t uid, const String16& callingPackage));
//     MOCK_METHOD3(finishOp, void(int32_t op, int32_t uid, const String16& callingPackage));
//     MOCK_METHOD3(startWatchingMode, void(int32_t op, const String16& packageName, const sp<IAppOpsCallback>& callback));
//     MOCK_METHOD1(stopWatchingMode, void(const sp<IAppOpsCallback>& callback));
    MOCK_METHOD0(getService, sp<IAppOpsService>());
};

MockAppOpsManager * M_AppOpsManager;

AppOpsManager::AppOpsManager()
{

}

int32_t AppOpsManager::checkOp(int32_t op, int32_t uid, const String16& callingPackage)
{
    return M_AppOpsManager->checkOp(op, uid, callingPackage);
}

int32_t AppOpsManager::noteOp(int32_t op, int32_t uid, const String16& callingPackage)
{
    return M_AppOpsManager->noteOp(op, uid, callingPackage);
}

int32_t AppOpsManager::startOp(int32_t op, int32_t uid, const String16& callingPackage)
{
    return M_AppOpsManager->startOp(op, uid, callingPackage);
}

void AppOpsManager::finishOp(int32_t op, int32_t uid, const String16& callingPackage)
{
//    M_AppOpsManager->finishOp(op, uid, callingPackage);
}

void AppOpsManager::startWatchingMode(int32_t op, const String16& packageName, const sp<IAppOpsCallback>& callback)
{
//    M_AppOpsManager->startWatchingMode(op, packageName, callback);
}

void AppOpsManager::stopWatchingMode(const sp<IAppOpsCallback>& callback)
{
//    M_AppOpsManager->stopWatchingMode(callback);
}

sp<IAppOpsService> AppOpsManager::getService()
{
    return M_AppOpsManager->getService();
}


}  // namespace android
