class MockSystemService {
  public:
    MOCK_METHOD1(dump, error_t(LogOutput& out));
    MOCK_METHOD0(getModuleID, uint8_t());
    MOCK_METHOD0(onInit, bool());
    MOCK_METHOD0(onStart, error_t());
    MOCK_METHOD0(onStop, error_t());
//     MOCK_METHOD0(instantiate, void());
//     MOCK_METHOD0(start, void());
    MOCK_METHOD1(getProperty, char*(const char* name));
//     MOCK_METHOD3(setProperty, void(char *, char *, bool ));
//     MOCK_METHOD3(setProperty, void(char *, int32_t , bool ));
    MOCK_METHOD0(syncProperties, bool());
};

MockSystemService * M_SystemService;

SystemService::SystemService(const char* name)
{

}

SystemService::~SystemService()
{

}

error_t SystemService::dump(LogOutput& out)
{
    return M_SystemService->dump(out);
}

uint8_t SystemService::getModuleID()
{
    return M_SystemService->getModuleID();
}

bool SystemService::onInit()
{
    return M_SystemService->onInit();
}

error_t SystemService::onStart()
{
    return M_SystemService->onStart();
}

error_t SystemService::onStop()
{
    return M_SystemService->onStop();
}

void SystemService::instantiate()
{
//    M_SystemService->instantiate();
}

void SystemService::start()
{
//    M_SystemService->start();
}

char* SystemService::getProperty(const char* name)
{
    return M_SystemService->getProperty(name);
}

void SystemService::setProperty(const char* name, const char* value, bool sync_now)
{
//    M_SystemService->setProperty(name, value, sync_now);
}

void SystemService::setProperty(const char* name, const int32_t i_value, bool sync_now)
{
//    M_SystemService->setProperty(name, i_value, sync_now);
}

bool SystemService::syncProperties()
{
    return M_SystemService->syncProperties();
}
