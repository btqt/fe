class MockStateConditionSet {
  public:
    MOCK_METHOD1(addCondition, int32_t(android::sp<StateCondition> condition));
    MOCK_METHOD0(activateConditions, error_t());
    MOCK_METHOD0(deactivateConditions, error_t());
    MOCK_METHOD1(checkCanExit, bool(int32_t who));
//     MOCK_METHOD1(setNextState, void(int32_t nextId));
};

MockStateConditionSet * M_StateConditionSet;

StateConditionSet::StateConditionSet(int32_t who, uint8_t type)
{

}

StateConditionSet::~StateConditionSet()
{

}

int32_t StateConditionSet::addCondition(android::sp<StateCondition> condition)
{
    return M_StateConditionSet->addCondition(condition);
}

error_t StateConditionSet::activateConditions()
{
    return M_StateConditionSet->activateConditions();
}

error_t StateConditionSet::deactivateConditions()
{
    return M_StateConditionSet->deactivateConditions();
}

bool StateConditionSet::checkCanExit(int32_t who)
{
    return M_StateConditionSet->checkCanExit(who);
}

void StateConditionSet::setNextState(int32_t nextId)
{
//    M_StateConditionSet->setNextState(nextId);
}
