class MockConfigAdapter {
 public:
  MOCK_METHOD1(getAppMode, bool(const std::string &mode));
  MOCK_METHOD2(setAppMode, bool(const std::string &mode, const bool value));
  MOCK_METHOD1(registerKeys, void(const std::map<std::string, std::string> &list));
  MOCK_METHOD1(registerObserver, void(IConfigObserver *o));
  MOCK_METHOD0(initialize, void());
};



MockConfigAdapter * M_ConfigAdapter;


ConfigAdapter::ConfigAdapter()
{

}

ConfigAdapter::ConfigAdapter()
{

}

bool ConfigAdapter::getAppMode(const std::string &mode)
{
  return M_ConfigAdapter->getAppMode(mode);
}

bool ConfigAdapter::setAppMode(const std::string &mode, const bool value)
{
  return M_ConfigAdapter->setAppMode(mode, value);
}

void ConfigAdapter::registerKeys(const std::map<std::string, std::string> &list)
{
  M_ConfigAdapter->registerKeys(list);
}

void ConfigAdapter::registerObserver(IConfigObserver *o)
{
  M_ConfigAdapter->registerObserver(o);
}

void ConfigAdapter::initialize()
{
  M_ConfigAdapter->initialize();
}
