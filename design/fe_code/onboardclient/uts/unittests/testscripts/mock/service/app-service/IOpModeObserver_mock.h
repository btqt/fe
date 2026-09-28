class MockIOpModeObserver {
 public:
  MOCK_METHOD1(onOpModeChanged, void(const int32_t opMode));
  MOCK_METHOD0(onOpModeDied, void());
};



MockIOpModeObserver * M_IOpModeObserver;


IOpModeObserver::IOpModeObserver()
{

}

void IOpModeObserver::onOpModeChanged(const int32_t opMode)
{
  M_IOpModeObserver->onOpModeChanged(opMode);
}

void IOpModeObserver::onOpModeDied()
{
  M_IOpModeObserver->onOpModeDied();
}
