namespace sl {

class MockHandler {
 public:
  MOCK_METHOD0(obtainMessage_001, android::sp<Message>());
  MOCK_METHOD1(obtainMessage_002, android::sp<Message>(int32_t what));
  MOCK_METHOD2(obtainMessage_003, android::sp<Message>(int32_t what, int32_t arg1));
  MOCK_METHOD2(obtainMessage_004, android::sp<Message>(int32_t what, void* obj));
  MOCK_METHOD3(obtainMessage_005, android::sp<Message>(int32_t what, int32_t arg1, int32_t arg2));
  MOCK_METHOD4(obtainMessage_006, android::sp<Message>(int32_t what, int32_t arg1, int32_t arg2, void* obj));
  MOCK_METHOD4(obtainMessage_007, android::sp<Message>(int32_t what, int32_t arg1, int32_t arg2, int32_t arg3));
  MOCK_METHOD2(obtainMessage_008, android::sp<Message>(int32_t what, android::sp<android::RefBase> spRef));
  MOCK_METHOD1(sendMessage, bool(const android::sp<Message>& message));
  MOCK_METHOD2(sendMessageDelayed, bool(const android::sp<Message>& message, int64_t delayMs));
  MOCK_METHOD2(sendMessageAtTime, bool(const android::sp<Message>& message, int64_t whenUs));
  MOCK_METHOD1(hasMessages, bool(int32_t what));
  MOCK_METHOD1(removeMessages_001, bool(int32_t what));
  MOCK_METHOD2(removeMessages_002, bool(int32_t what, void* obj));
  // MOCK_METHOD1(dispatchMessage, void(const android::sp<Message>& message));
  // MOCK_METHOD1(handleMessage, void(const android::sp<Message>& msg));
};


MockHandler * M_Handler;


Handler::Handler()
{

}

Handler::Handler(android::sp<SLLooper>& looper)
{

}

Handler::~Handler()
{

}

android::sp<Message> Handler::obtainMessage()
{
  return M_Handler->obtainMessage_001();
}

android::sp<Message> Handler::obtainMessage(int32_t what)
{
  return M_Handler->obtainMessage_002(what);
}

android::sp<Message> Handler::obtainMessage(int32_t what, int32_t arg1)
{
  return M_Handler->obtainMessage_003(what, arg1);
}

android::sp<Message> Handler::obtainMessage(int32_t what, void* obj)
{
  return M_Handler->obtainMessage_004(what, obj);
}

android::sp<Message> Handler::obtainMessage(int32_t what, int32_t arg1, int32_t arg2)
{
  return M_Handler->obtainMessage_005(what, arg1, arg2);
}

android::sp<Message> Handler::obtainMessage(int32_t what, int32_t arg1, int32_t arg2, void* obj)
{
  return M_Handler->obtainMessage_006(what, arg1, arg2, obj);
}

android::sp<Message> Handler::obtainMessage(int32_t what, int32_t arg1, int32_t arg2, int32_t arg3)
{
  return M_Handler->obtainMessage_007(what, arg1, arg2, arg3);
}

android::sp<Message> Handler::obtainMessage(int32_t what, sp<android::RefBase> spRef)
{
  return M_Handler->obtainMessage_008(what, spRef);
}

bool Handler::sendMessage(const android::sp<Message>& message)
{
  return M_Handler->sendMessage(message);
}

bool Handler::sendMessageDelayed(const android::sp<Message>& message, int64_t delayMs)
{
  return M_Handler->sendMessageDelayed(message, delayMs);
}

bool Handler::sendMessageAtTime(const android::sp<Message>& message, int64_t whenUs)
{
  return M_Handler->sendMessageAtTime(message, whenUs);
}

bool Handler::hasMessages(int32_t what)
{
  return M_Handler->hasMessages(what);
}

bool Handler::removeMessages(int32_t what)
{
  return M_Handler->removeMessages_001(what);
}

bool Handler::removeMessages(int32_t what, void* obj)
{
  return M_Handler->removeMessages_002(what, obj);
}

void Handler::dispatchMessage(const android::sp<Message>& message)
{
  // M_Handler->dispatchMessage(message);
}

void Handler::handleMessage(const android::sp<Message>& msg)
{
  // M_Handler->handleMessage(msg);
}
}  // namespace sl
