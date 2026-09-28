namespace sl {

class MockSLLooperThread {
  public:
    MOCK_METHOD0(readyToRun, android::status_t());
    MOCK_METHOD0(threadLoop, bool());
//     MOCK_METHOD0(DISALLOW_COPY_ASSIGN_CONSTRUCTORS, void(SLLooperThread));
};

MockSLLooperThread * M_SLLooperThread;

android::status_t SLLooperThread::readyToRun()
{
    return M_SLLooperThread->readyToRun();
}

bool SLLooperThread::threadLoop()
{
    return M_SLLooperThread->threadLoop();
}

void SLLooperThread::DISALLOW_COPY_ASSIGN_CONSTRUCTORS(SLLooperThread)
{
//    M_SLLooperThread->DISALLOW_COPY_ASSIGN_CONSTRUCTORS(SLLooperThread);
}


}  // namespace sl
