class MockOpModeAdapter {
 public:
  MOCK_METHOD0(getOpMode, int32_t());
  MOCK_METHOD1(registerObserver, void(IOpModeObserver *o));
  MOCK_METHOD0(initialize, void());
};



MockOpModeAdapter * M_OpModeAdapter;


OpModeAdapter::OpModeAdapter()
{

}

OpModeAdapter::OpModeAdapter()
{

}

int32_t OpModeAdapter::getOpMode()
{
  return M_OpModeAdapter->getOpMode();
}

void OpModeAdapter::registerObserver(IOpModeObserver *o)
{
  M_OpModeAdapter->registerObserver(o);
}

void OpModeAdapter::initialize()
{
  M_OpModeAdapter->initialize();
}
