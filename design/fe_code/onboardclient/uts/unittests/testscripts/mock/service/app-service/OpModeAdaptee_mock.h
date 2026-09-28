class MockOpModeAdaptee {
 public:
  MOCK_METHOD0(getOpMode, int32_t());
  MOCK_METHOD1(registerObserver, void(IOpModeObserver *o));
  MOCK_METHOD0(initialize, void());
};



MockOpModeAdaptee * M_OpModeAdaptee;


OpModeAdaptee::OpModeAdaptee()
{

}

OpModeAdaptee::OpModeAdaptee()
{

}

int32_t OpModeAdaptee::getOpMode()
{
  return M_OpModeAdaptee->getOpMode();
}

void OpModeAdaptee::registerObserver(IOpModeObserver *o)
{
  M_OpModeAdaptee->registerObserver(o);
}

void OpModeAdaptee::initialize()
{
  M_OpModeAdaptee->initialize();
}
