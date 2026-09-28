namespace sl {

class MockMessageQueue {
  public:
    MOCK_METHOD2(enqueueMessage, bool(const sp<Message>& enqueuemsg, int64_t whenUs));
    MOCK_METHOD0(poll, sp<Message>());
    MOCK_METHOD3(hasMessage, bool(const sp<Handler>& h, int32_t what, void* obj));
    MOCK_METHOD3(removeMessages, bool(const sp<Handler>& h, int32_t what, void* obj));
};

MockMessageQueue * M_MessageQueue;

MessageQueue::MessageQueue()
{

}

MessageQueue::~MessageQueue()
{

}

bool MessageQueue::enqueueMessage(const sp<Message>& enqueuemsg, int64_t whenUs)
{
    return M_MessageQueue->enqueueMessage(enqueuemsg, whenUs);
}

sp<Message> MessageQueue::poll()
{
    return M_MessageQueue->poll();
}

bool MessageQueue::hasMessage(const sp<Handler>& h, int32_t what, void* obj)
{
    return M_MessageQueue->hasMessage(h, what, obj);
}

bool MessageQueue::removeMessages(const sp<Handler>& h, int32_t what, void* obj)
{
    return M_MessageQueue->removeMessages(h, what, obj);
}


}  // namespace sl
