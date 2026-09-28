class MockTimerTimeoutHandler {
  public:
//     MOCK_METHOD1(handlerFunction, void(int32_t timerId));
};

MockTimerTimeoutHandler * M_TimerTimeoutHandler;

class MockTimer {
  public:
//     MOCK_METHOD2(setDuration, void(uint32_t value, uint32_t interval));
//     MOCK_METHOD2(setDurationMs, void(uint32_t value, uint32_t interval));
//     MOCK_METHOD0(start, void());
//     MOCK_METHOD0(restart, void());
//     MOCK_METHOD0(stop, void());
    MOCK_METHOD0(getRemainingTimeMs, uint32_t());
//     MOCK_METHOD0(timeout, void());
//     MOCK_METHOD0(callbackWrapper, void());
//     MOCK_METHOD1(timeOutHandler, void(sigval_t This));
    MOCK_METHOD2(createTimer, int32_t(timer_t *timerid, TimerHandler handler_cb));
//     MOCK_METHOD5(startTimer, void(const timer_t timerid, const time_t start_sec, const long start_nsec, const time_t cyclic_sec, const long cyclic_nsec));
//     MOCK_METHOD1(stopTimer, void(timer_t timerid));
//     MOCK_METHOD1(deleteTimer, void(timer_t timerid));
};

MockTimer * M_Timer;

class MockTimeTimeoutHandlerImp {
  public:
//     MOCK_METHOD1(handlerFunction, void(const int32_t timerId));
};

MockTimeTimeoutHandlerImp * M_TimeTimeoutHandlerImp;

void TimerTimeoutHandler::handlerFunction(int32_t timerId)
{
//    M_TimerTimeoutHandler->handlerFunction(timerId);
}


Timer::Timer(TimerTimeoutHandler * timeHandler, const int32_t timerId)
{

}

Timer::~Timer()
{

}

void Timer::setDuration(uint32_t value, uint32_t interval)
{
//    M_Timer->setDuration(value, interval);
}

void Timer::setDurationMs(uint32_t value, uint32_t interval)
{
//    M_Timer->setDurationMs(value, interval);
}

void Timer::start()
{
//    M_Timer->start();
}

void Timer::restart()
{
//    M_Timer->restart();
}

void Timer::stop()
{
//    M_Timer->stop();
}

uint32_t Timer::getRemainingTimeMs()
{
    return M_Timer->getRemainingTimeMs();
}

void Timer::timeout()
{
//    M_Timer->timeout();
}

void Timer::callbackWrapper()
{
//    M_Timer->callbackWrapper();
}

void Timer::timeOutHandler(sigval_t This)
{
//    M_Timer->timeOutHandler(This);
}

int32_t Timer::createTimer(timer_t *timerid, TimerHandler handler_cb)
{
    return M_Timer->createTimer(timerid, handler_cb);
}

void Timer::startTimer(const timer_t timerid, const time_t start_sec, const long start_nsec, const time_t cyclic_sec, const long cyclic_nsec)
{
//    M_Timer->startTimer(timerid, start_sec, start_nsec, cyclic_sec, cyclic_nsec);
}

void Timer::stopTimer(timer_t timerid)
{
//    M_Timer->stopTimer(timerid);
}

void Timer::deleteTimer(timer_t timerid)
{
//    M_Timer->deleteTimer(timerid);
}


void TimeTimeoutHandlerImp::handlerFunction(const int32_t timerId)
{
//    M_TimeTimeoutHandlerImp->handlerFunction(timerId);
}
