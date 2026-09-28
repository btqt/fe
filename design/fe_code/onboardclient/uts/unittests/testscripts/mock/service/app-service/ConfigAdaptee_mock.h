class MockConfigAdaptee {
 public:
  MOCK_METHOD1(getAppMode, bool(std::string mode));
  MOCK_METHOD2(setAppMode, bool(const std::string &mode, const bool value));
  MOCK_METHOD1(registerKeys, void(const std::map<std::string, std::string> &list));
  MOCK_METHOD1(registerReceiverByKey, void(std::string key));
  MOCK_METHOD1(registerObserver, void(IConfigObserver *o));
  MOCK_METHOD1(onConfigDataChanged, void(sp<Buffer>& name));
  MOCK_METHOD1(onServiceBinderDied, void(const android::wp<android::IBinder>& who));
  MOCK_METHOD0(initialize, void());
};



MockConfigAdaptee * M_ConfigAdaptee;


ConfigAdaptee::ConfigAdaptee()
{

}

ConfigAdaptee::ConfigAdaptee()
{

}

bool ConfigAdaptee::getAppMode(std::string mode)
{
  return M_ConfigAdaptee->getAppMode(mode);
}

bool ConfigAdaptee::setAppMode(const std::string &mode, const bool value)
{
  return M_ConfigAdaptee->setAppMode(mode, value);
}

void ConfigAdaptee::registerKeys(const std::map<std::string, std::string> &list)
{
  M_ConfigAdaptee->registerKeys(list);
}

void ConfigAdaptee::registerReceiverByKey(std::string key)
{
  M_ConfigAdaptee->registerReceiverByKey(key);
}

void ConfigAdaptee::registerObserver(IConfigObserver *o)
{
  M_ConfigAdaptee->registerObserver(o);
}

void ConfigAdaptee::onConfigDataChanged(sp<Buffer>& name)
{
  M_ConfigAdaptee->onConfigDataChanged(name);
}

void ConfigAdaptee::onServiceBinderDied(const android::wp<android::IBinder>& who)
{
  M_ConfigAdaptee->onServiceBinderDied(who);
}

void ConfigAdaptee::initialize()
{
  M_ConfigAdaptee->initialize();
}
