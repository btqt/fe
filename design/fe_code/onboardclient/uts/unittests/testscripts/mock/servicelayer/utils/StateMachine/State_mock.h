class MockState {
  public:
//     MOCK_METHOD1(setLooper, void(sp<sl::SLLooper> looper));
    MOCK_METHOD2(addExitCondition, sp<State>(sp<State> me,sp<StateCondition> condition));
    MOCK_METHOD1(OR, sp<State>(sp<StateCondition> condition));
    MOCK_METHOD1(AND, sp<State>(sp<StateCondition> condition));
//     MOCK_METHOD1(nextState, void(sp<State> next));
//     MOCK_METHOD0(activated, void(void));
//     MOCK_METHOD0(deactivated, void(void));
//     MOCK_METHOD0(entry, void());
//     MOCK_METHOD0(doing, void());
//     MOCK_METHOD0(exit, void());
//     MOCK_METHOD3(onNotify, void(int32_t stateId, int32_t setId, int32_t reserved));
    MOCK_METHOD0(name, const char*());
//     MOCK_METHOD0(dump, void());
    MOCK_METHOD2(checkCanTransit, bool(int32_t who, int32_t setId));
//     MOCK_METHOD2(createCondition, void(sp<StateCondition> condition, uint8_t type));
//     MOCK_METHOD2(addCondition, void(sp<StateCondition> condition, uint8_t type));
};

MockState * M_State;

State::State(int32_t id)
{

}

State::~State()
{

}

void State::setLooper(sp<sl::SLLooper> looper)
{
//    M_State->setLooper(looper);
}

sp<State> State::addExitCondition(sp<State> me,sp<StateCondition> condition)
{
    return M_State->addExitCondition(me, condition);
}

sp<State> State::OR(sp<StateCondition> condition)
{
    return M_State->OR(condition);
}

sp<State> State::AND(sp<StateCondition> condition)
{
    return M_State->AND(condition);
}

void State::nextState(sp<State> next)
{
//    M_State->nextState(next);
}

void State::activated(void)
{
//    M_State->activated();
}

void State::deactivated(void)
{
//    M_State->deactivated();
}

void State::entry()
{
//    M_State->entry();
}

void State::doing()
{
//    M_State->doing();
}

void State::exit()
{
//    M_State->exit();
}

void State::onNotify(int32_t stateId, int32_t setId, int32_t reserved)
{
//    M_State->onNotify(stateId, setId, reserved);
}

const char* State::name()
{
    return M_State->name();
}

void State::dump()
{
//    M_State->dump();
}

bool State::checkCanTransit(int32_t who, int32_t setId)
{
    return M_State->checkCanTransit(who, setId);
}

void State::createCondition(sp<StateCondition> condition, uint8_t type)
{
//    M_State->createCondition(condition, type);
}

void State::addCondition(sp<StateCondition> condition, uint8_t type)
{
//    M_State->addCondition(condition, type);
}
