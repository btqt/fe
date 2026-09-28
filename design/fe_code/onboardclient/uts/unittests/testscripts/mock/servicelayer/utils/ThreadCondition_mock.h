namespace sl {

class MockThreadCondition {
  public:
    MOCK_METHOD1(wait, error_t(int32_t sigMask));
    MOCK_METHOD2(waitRelative, error_t(int32_t sigMask, int64_t waitMs));
//     MOCK_METHOD1(signal, void(int32_t sig));
//     MOCK_METHOD0(signalAll, void());
//     MOCK_METHOD0(initCondition, void());
};

MockThreadCondition * M_ThreadCondition;

ThreadCondition::ThreadCondition()
{

}

ThreadCondition::~ThreadCondition()
{

}

error_t ThreadCondition::wait(int32_t sigMask)
{
    return M_ThreadCondition->wait(sigMask);
}

error_t ThreadCondition::waitRelative(int32_t sigMask, int64_t waitMs)
{
    return M_ThreadCondition->waitRelative(sigMask, waitMs);
}

void ThreadCondition::signal(int32_t sig)
{
//    M_ThreadCondition->signal(sig);
}

void ThreadCondition::signalAll()
{
//    M_ThreadCondition->signalAll();
}

void ThreadCondition::initCondition()
{
//    M_ThreadCondition->initCondition();
}


}  // namespace sl
