class MockStateCondition {
  public:
    MOCK_METHOD0(who, int32_t());
//     MOCK_METHOD1(setContainerId, void(int32_t id));
//     MOCK_METHOD1(registerConditionObserver, void(android::sp<Observer> cb));
//     MOCK_METHOD3(onNotify, void(int32_t stateid, int32_t status, int32_t reserved));
//     MOCK_METHOD0(registerCondition, void());
//     MOCK_METHOD0(unregisterCondition, void());
};

MockStateCondition * M_StateCondition;

StateCondition::StateCondition()
{

}

StateCondition::StateCondition(int32_t who)
{

}

StateCondition::~StateCondition()
{

}

int32_t StateCondition::who()
{
    return M_StateCondition->who();
}

void StateCondition::setContainerId(int32_t id)
{
//    M_StateCondition->setContainerId(id);
}

void StateCondition::registerConditionObserver(android::sp<Observer> cb)
{
//    M_StateCondition->registerConditionObserver(cb);
}

void StateCondition::onNotify(int32_t stateid, int32_t status, int32_t reserved)
{
//    M_StateCondition->onNotify(stateid, status, reserved);
}

void StateCondition::registerCondition()
{
//    M_StateCondition->registerCondition();
}

void StateCondition::unregisterCondition()
{
//    M_StateCondition->unregisterCondition();
}
