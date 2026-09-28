class MockServiceCallback {
 public:
  MOCK_METHOD0(onPreEnableApplication, void());
  MOCK_METHOD0(onPostEnableApplication, void());
  MOCK_METHOD0(onPreDisableApplication, void());
  MOCK_METHOD0(onPostDisableApplication, void());
};



MockServiceCallback * M_ServiceCallback;


void ServiceCallback::onPreEnableApplication()
{
  M_ServiceCallback->onPreEnableApplication();
}

void ServiceCallback::onPostEnableApplication()
{
  M_ServiceCallback->onPostEnableApplication();
}

void ServiceCallback::onPreDisableApplication()
{
  M_ServiceCallback->onPreDisableApplication();
}

void ServiceCallback::onPostDisableApplication()
{
  M_ServiceCallback->onPostDisableApplication();
}
