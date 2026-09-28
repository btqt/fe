namespace android {

class MockPermissionCache {
  public:
//     MOCK_METHOD0(purge, void());
    MOCK_CONST_METHOD3(check, status_t(bool* granted, const String16& permission, uid_t uid));
//     MOCK_METHOD3(cache, void(const String16& permission, uid_t uid, bool granted));
    MOCK_METHOD1(checkCallingPermission, bool(const String16& permission));
    MOCK_METHOD3(checkCallingPermission, bool(const String16& permission, int32_t* outPid, int32_t* outUid));
    MOCK_METHOD3(checkPermission, bool(const String16& permission, pid_t pid, uid_t uid));
};

MockPermissionCache * M_PermissionCache;

void PermissionCache::purge()
{
//    M_PermissionCache->purge();
}

status_t PermissionCache::check(bool* granted, const String16& permission, uid_t uid) const
{
    return M_PermissionCache->check(granted, permission, uid);
}

void PermissionCache::cache(const String16& permission, uid_t uid, bool granted)
{
//    M_PermissionCache->cache(permission, uid, granted);
}

PermissionCache::PermissionCache()
{

}

bool PermissionCache::checkCallingPermission(const String16& permission)
{
    return M_PermissionCache->checkCallingPermission(permission);
}

bool PermissionCache::checkCallingPermission(const String16& permission, int32_t* outPid, int32_t* outUid)
{
    return M_PermissionCache->checkCallingPermission(permission, outPid, outUid);
}

bool PermissionCache::checkPermission(const String16& permission, pid_t pid, uid_t uid)
{
    return M_PermissionCache->checkPermission(permission, pid, uid);
}


}  // namespace android
