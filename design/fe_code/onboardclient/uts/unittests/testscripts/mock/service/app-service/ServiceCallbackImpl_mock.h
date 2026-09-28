class MockServiceCallbackImpl {
 public:
  MOCK_METHOD0(onPreEnableApplication, void());
  MOCK_METHOD0(onPostEnableApplication, void());
  MOCK_METHOD0(onPreDisableApplication, void());
  MOCK_METHOD0(onPostDisableApplication, void());
};



MockServiceCallbackImpl * M_ServiceCallbackImpl;


ServiceCallbackImpl::ServiceCallbackImpl()
{

}

ServiceCallbackImpl::ServiceCallbackImpl()
{

}

void ServiceCallbackImpl::onPreEnableApplication()
{
  M_ServiceCallbackImpl->onPreEnableApplication();
}

void ServiceCallbackImpl::onPostEnableApplication()
{
  M_ServiceCallbackImpl->onPostEnableApplication();
}

void ServiceCallbackImpl::onPreDisableApplication()
{
  M_ServiceCallbackImpl->onPreDisableApplication();
}

void ServiceCallbackImpl::onPostDisableApplication()
{
  M_ServiceCallbackImpl->onPostDisableApplication();
}
