namespace android {

class MockIPermissionController {
  public:
//     MOCK_METHOD0(DECLARE_META_INTERFACE, void(PermissionController));
    MOCK_METHOD3(checkPermission, bool(const String16& permission, int32_t pid, int32_t uid));
};

MockIPermissionController * M_IPermissionController;

class MockBnPermissionController {
  public:
    MOCK_METHOD4(onTransact, status_t(uint32_t , Parcel , Parcel *, uint32_t ));
};

MockBnPermissionController * M_BnPermissionController;

void IPermissionController::DECLARE_META_INTERFACE(PermissionController)
{
//    M_IPermissionController->DECLARE_META_INTERFACE(PermissionController);
}

bool IPermissionController::checkPermission(const String16& permission, int32_t pid, int32_t uid)
{
    return M_IPermissionController->checkPermission(permission, pid, uid);
}


status_t BnPermissionController::onTransact(uint32_t code, Parcel data, Parcel *reply, uint32_t flags)
{
    return M_BnPermissionController->onTransact(code, data, reply, flags);
}


}  // namespace android
