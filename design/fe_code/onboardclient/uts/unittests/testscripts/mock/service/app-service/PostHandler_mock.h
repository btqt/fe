class MockPostHandler {
 public:
  MOCK_METHOD1(handleMessage, void(const android::sp<sl::Message>& msg));
  MOCK_METHOD1(handleSendPost, void(const android::sp<sl::Message>& msg));
  MOCK_METHOD2(sendPost, error_t(const char* app, const android::sp<Post>& post));
  MOCK_METHOD1(notifyAppLaunched, void(const char* app));
  MOCK_METHOD1(registerPostNotifier, void(std::shared_ptr<PostNotifier> n));
};



MockPostHandler * M_PostHandler;


PostHandler::PostHandler(android::sp<sl::SLLooper>& looper, std::shared_ptr<PostNotifier> n)
{

}

void PostHandler::handleMessage(const android::sp<sl::Message>& msg)
{
  M_PostHandler->handleMessage(msg);
}

void PostHandler::handleSendPost(const android::sp<sl::Message>& msg)
{
  M_PostHandler->handleSendPost(msg);
}

error_t PostHandler::sendPost(const char* app, const android::sp<Post>& post)
{
  return M_PostHandler->sendPost(app, post);
}

void PostHandler::notifyAppLaunched(const char* app)
{
  M_PostHandler->notifyAppLaunched(app);
}

void PostHandler::registerPostNotifier(std::shared_ptr<PostNotifier> n)
{
  M_PostHandler->registerPostNotifier(n);
}
