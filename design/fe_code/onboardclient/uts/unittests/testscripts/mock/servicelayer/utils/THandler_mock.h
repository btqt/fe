#include <utils/THandler.h>

namespace sl {

class MockTHandler {
  public:
//     MOCK_METHOD1(kickMe, void(int));
//     MOCK_METHOD1(setTimeout, void(int sec));
//     MOCK_METHOD1(handleMessage, void(const sp<Message>& msg));
};

MockTHandler * M_THandler;

THandler::THandler(sp<SLLooper>& looper, const int period_sec)
{

}

THandler::~THandler()
{

}

void THandler::kickMe (int timer_id)
{
//    M_THandler->kickMe(int);
}

void THandler::setTimeout(int sec, bool currThread)
{
//    M_THandler->setTimeout(sec,currThread);
}

void THandler::handleMessage(const sp<Message>& msg)
{
//    M_THandler->handleMessage(msg);
}


}  // namespace sl
