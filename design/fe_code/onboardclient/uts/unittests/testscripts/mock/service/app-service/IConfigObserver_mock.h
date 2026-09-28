class MockIConfigObserver {
 public:
  MOCK_METHOD2(onConfigChanged, void(std::string key, bool value));
  MOCK_METHOD0(onConfigDied, void());
};



MockIConfigObserver * M_IConfigObserver;


IConfigObserver::IConfigObserver()
{

}

void IConfigObserver::onConfigChanged(std::string key, bool value)
{
  M_IConfigObserver->onConfigChanged(key, value);
}

void IConfigObserver::onConfigDied()
{
  M_IConfigObserver->onConfigDied();
}
