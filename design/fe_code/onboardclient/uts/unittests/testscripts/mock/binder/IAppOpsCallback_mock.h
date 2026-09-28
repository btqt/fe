namespace android {

class MockIAppOpsCallback {
  public:
//     MOCK_METHOD0(DECLARE_META_INTERFACE, void(AppOpsCallback));
//     MOCK_METHOD2(opChanged, void(int32_t op, const String16& packageName));
};

MockIAppOpsCallback * M_IAppOpsCallback;

class MockBnAppOpsCallback {
  public:
    MOCK_METHOD4(onTransact, status_t(uint32_t , Parcel , Parcel *, uint32_t ));
};

MockBnAppOpsCallback * M_BnAppOpsCallback;

void IAppOpsCallback::DECLARE_META_INTERFACE(AppOpsCallback)
{
//    M_IAppOpsCallback->DECLARE_META_INTERFACE(AppOpsCallback);
}

void IAppOpsCallback::opChanged(int32_t op, const String16& packageName)
{
//    M_IAppOpsCallback->opChanged(op, packageName);
}


status_t BnAppOpsCallback::onTransact(uint32_t code, Parcel data, Parcel *reply, uint32_t flags)
{
    return M_BnAppOpsCallback->onTransact(code, data, reply, flags);
}


}  // namespace android
