class MockStateMachine {
  public:
    MOCK_METHOD1(root, android::sp<StateMachine>(android::sp<State> root));
    MOCK_METHOD2(registerState, android::sp<StateMachine>(android::sp , bool ));
//     MOCK_METHOD1(setLooper, void(android::sp<sl::SLLooper> looper));
    MOCK_METHOD0(start, error_t());
    MOCK_METHOD1(reset, error_t(bool autostart));
    MOCK_METHOD1(setRoot, error_t(int32_t stateId));
    MOCK_METHOD1(move, error_t(int32_t stateId));
//     MOCK_METHOD1(init, void(android::sp<sl::SLLooper> looper));
//     MOCK_METHOD0(doState, void());
//     MOCK_METHOD0(started, void());
//     MOCK_METHOD1(startTransition, void(int32_t next));
//     MOCK_METHOD1(transit, void(int32_t nextStateId));
//     MOCK_METHOD1(endTransition, void(int32_t next));
};

MockStateMachine * M_StateMachine;

StateMachine::~StateMachine()
{

}

android::sp<StateMachine> StateMachine::root(android::sp<State> root)
{
    return M_StateMachine->root(root);
}

android::sp<StateMachine> StateMachine::registerState(android::sp state, bool root)
{
    return M_StateMachine->registerState(state, root);
}

void StateMachine::setLooper(android::sp<sl::SLLooper> looper)
{
//    M_StateMachine->setLooper(looper);
}

error_t StateMachine::start()
{
    return M_StateMachine->start();
}

error_t StateMachine::reset(bool autostart)
{
    return M_StateMachine->reset(autostart);
}

error_t StateMachine::setRoot(int32_t stateId)
{
    return M_StateMachine->setRoot(stateId);
}

error_t StateMachine::move(int32_t stateId)
{
    return M_StateMachine->move(stateId);
}

StateMachine::StateMachine(android::sp<sl::SLLooper>& looper)
{

}

void StateMachine::init(android::sp<sl::SLLooper> looper)
{
//    M_StateMachine->init(looper);
}

void StateMachine::doState()
{
//    M_StateMachine->doState();
}

void StateMachine::started()
{
//    M_StateMachine->started();
}

void StateMachine::startTransition(int32_t next)
{
//    M_StateMachine->startTransition(next);
}

void StateMachine::transit(int32_t nextStateId)
{
//    M_StateMachine->transit(nextStateId);
}

void StateMachine::endTransition(int32_t next)
{
//    M_StateMachine->endTransition(next);
}
