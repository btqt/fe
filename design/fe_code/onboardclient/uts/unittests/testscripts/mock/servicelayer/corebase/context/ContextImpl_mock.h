class MockContextImpl {
  public:
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
//     MOCK_METHOD0(init, void());
};

MockContextImpl * M_ContextImpl;

ContextImpl::ContextImpl()
{

}

ContextImpl::~ContextImpl()
{

}

error_t ContextImpl::dump(LogOutput& out)
{
    return M_ContextImpl->dump(out);
}

error_t ContextImpl::startApplication()
{
    return M_ContextImpl->startApplication();
}

error_t ContextImpl::stopApplication()
{
    return M_ContextImpl->stopApplication();
}

Context* ContextImpl::getContext()
{
    return M_ContextImpl->getContext();
}

error_t ContextImpl::startService()
{
    return M_ContextImpl->startService();
}

error_t ContextImpl::stopService()
{
    return M_ContextImpl->stopService();
}

error_t ContextImpl::getContextDirectory()
{
    return M_ContextImpl->getContextDirectory();
}

error_t ContextImpl::broadcastMessage()
{
    return M_ContextImpl->broadcastMessage();
}

error_t ContextImpl::sendMessage()
{
    return M_ContextImpl->sendMessage();
}

void ContextImpl::setAppId(appid_t id)
{
//    M_ContextImpl->setAppId(id);
}

appid_t ContextImpl::getAppId(void)
{
    return M_ContextImpl->getAppId();
}

void ContextImpl::setAppName(std::string name)
{
//    M_ContextImpl->setAppName(name);
}

std::string ContextImpl::getAppName(void)
{
    return M_ContextImpl->getAppName();
}

char* ContextImpl::getProperty(const char* name)
{
    return M_ContextImpl->getProperty(name);
}

void ContextImpl::setProperty(char *name, char *value, bool sync_now)
{
//    M_ContextImpl->setProperty(name, value, sync_now);
}

void ContextImpl::setProperty(char *name, int32_t i_value, bool sync_now)
{
//    M_ContextImpl->setProperty(name, i_value, sync_now);
}

void ContextImpl::init()
{
//    M_ContextImpl->init();
}
