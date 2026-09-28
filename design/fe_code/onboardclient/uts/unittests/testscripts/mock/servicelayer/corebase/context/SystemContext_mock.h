class MockSystemContext {
  public:
//     MOCK_METHOD1(setContext, void(ContextImpl* impl));
    MOCK_METHOD1(dump, error_t(LogOutput& out));
    MOCK_METHOD0(startApplication, error_t());
    MOCK_METHOD0(stopApplication, error_t());
    MOCK_METHOD0(getContext, Context*());
    MOCK_METHOD0(startService, error_t());
    MOCK_METHOD0(stopService, error_t());
    MOCK_METHOD0(getContextDirectory, error_t());
    MOCK_METHOD0(broadcastMessage, error_t());
    MOCK_METHOD0(sendMessage, error_t());
//     MOCK_METHOD1(setAppId, void(appid_t id));
    MOCK_METHOD0(getAppId, appid_t(void));
//     MOCK_METHOD1(setAppName, void(std::string name));
    MOCK_METHOD0(getAppName, std::string(void));
    MOCK_METHOD1(getProperty, char*(const char* name));
//     MOCK_METHOD3(setProperty, void(char *, char *, bool ));
//     MOCK_METHOD3(setProperty, void(char *, int32_t , bool ));
};

MockSystemContext * M_SystemContext;

SystemContext::SystemContext()
{

}

SystemContext::~SystemContext()
{

}

void SystemContext::setContext(ContextImpl* impl)
{
//    M_SystemContext->setContext(impl);
}

error_t SystemContext::dump(LogOutput& out)
{
    return M_SystemContext->dump(out);
}

error_t SystemContext::startApplication()
{
    return M_SystemContext->startApplication();
}

error_t SystemContext::stopApplication()
{
    return M_SystemContext->stopApplication();
}

Context* SystemContext::getContext()
{
    return M_SystemContext->getContext();
}

error_t SystemContext::startService()
{
    return M_SystemContext->startService();
}

error_t SystemContext::stopService()
{
    return M_SystemContext->stopService();
}

error_t SystemContext::getContextDirectory()
{
    return M_SystemContext->getContextDirectory();
}

error_t SystemContext::broadcastMessage()
{
    return M_SystemContext->broadcastMessage();
}

error_t SystemContext::sendMessage()
{
    return M_SystemContext->sendMessage();
}

void SystemContext::setAppId(appid_t id)
{
//    M_SystemContext->setAppId(id);
}

appid_t SystemContext::getAppId(void)
{
    return M_SystemContext->getAppId();
}

void SystemContext::setAppName(std::string name)
{
//    M_SystemContext->setAppName(name);
}

std::string SystemContext::getAppName(void)
{
    return M_SystemContext->getAppName();
}

char* SystemContext::getProperty(const char* name)
{
    return M_SystemContext->getProperty(name);
}

void SystemContext::setProperty(char *name, char *value, bool sync_now)
{
//    M_SystemContext->setProperty(name, value, sync_now);
}

void SystemContext::setProperty(char *name, int32_t value_i, bool sync_now)
{
//    M_SystemContext->setProperty(name, value_i, sync_now);
}
