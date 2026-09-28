namespace sl {

class MockSLLooper {
  public:
    MOCK_METHOD0(prepare, bool());
    MOCK_METHOD0(myLooper, android::sp<SLLooper>());
//     MOCK_METHOD1(setName, void(const char* name));
    MOCK_METHOD0(getName, char*());
    MOCK_METHOD1(start, error_t(bool attachCurrentThread));
    MOCK_METHOD0(stop, error_t());
//     MOCK_METHOD1(setThreadWatcherTimeout, void(int sec));
    MOCK_METHOD0(messageQueue, android::sp<MessageQueue>());
//     MOCK_METHOD0(dump, void());
//     MOCK_METHOD0(initTLSKey, void());
//     MOCK_METHOD1(threadDestructor, void(void *st));
//     MOCK_METHOD1(setForThread, void(const android::sp<SLLooper>& looper));
    MOCK_METHOD0(getForThread, android::sp<SLLooper>());
    MOCK_METHOD0(loop, bool());
//     MOCK_METHOD0(flush, void());
//     MOCK_METHOD0(DISALLOW_COPY_ASSIGN_CONSTRUCTORS, void(SLLooper));
};

MockSLLooper * M_SLLooper;

SLLooper::SLLooper(const int reportPeriodSec) :
    mReportSec(reportPeriodSec)
{

}

SLLooper::~SLLooper()
{

}

bool SLLooper::prepare()
{
    return M_SLLooper->prepare();
}

android::sp<SLLooper> SLLooper::myLooper()
{
    return M_SLLooper->myLooper();
}

void SLLooper::setName(const char* name)
{
//    M_SLLooper->setName(name);
}

char* SLLooper::getName()
{
    return M_SLLooper->getName();
}

error_t SLLooper::start(bool attachCurrentThread)
{
    return M_SLLooper->start(attachCurrentThread);
}

error_t SLLooper::stop()
{
    return M_SLLooper->stop();
}

void SLLooper::setThreadWatcherTimeout(int sec)
{
//    M_SLLooper->setThreadWatcherTimeout(sec);
}

android::sp<MessageQueue> SLLooper::messageQueue()
{
    return M_SLLooper->messageQueue();
}

void SLLooper::dump()
{
//    M_SLLooper->dump();
}

void SLLooper::initTLSKey()
{
//    M_SLLooper->initTLSKey();
}

void SLLooper::threadDestructor(void *st)
{
//    M_SLLooper->threadDestructor();
}

void SLLooper::setForThread(const android::sp<SLLooper>& looper)
{
//    M_SLLooper->setForThread(looper);
}

android::sp<SLLooper> SLLooper::getForThread()
{
    return M_SLLooper->getForThread();
}

bool SLLooper::loop()
{
    return M_SLLooper->loop();
}

void SLLooper::flush()
{
//    M_SLLooper->flush();
}
}  // namespace sl
