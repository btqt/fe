namespace sl {

class MockMessage {
  public:
    MOCK_METHOD0(obtain, sp<Message>());
    MOCK_METHOD1(obtain, sp<Message>(const Message* message));
    MOCK_METHOD1(obtain, sp<Message>(const sp<Handler>& handler));
    MOCK_METHOD2(obtain, sp<Message>(const sp<Handler>& handler, int32_t obtain_what));
    MOCK_METHOD3(obtain, sp<Message>(const sp<Handler>& handler, int32_t obtain_what, int32_t obtain_arg1));
    MOCK_METHOD3(obtain, sp<Message>(const sp<Handler>& handler, int32_t obtain_what, void* obtain_obj));
    MOCK_METHOD4(obtain, sp<Message>(const sp<Handler>& handler, int32_t obtain_what, int32_t obtain_arg1, int32_t obtain_arg2));
    MOCK_METHOD5(obtain, sp<Message>(const sp<Handler>& handler, int32_t obtain_what, int32_t obtain_arg1, int32_t obtain_arg2, void* obtain_obj));
    MOCK_METHOD5(obtain, sp<Message>(const sp<Handler>& handler, int32_t obtain_what, int32_t obtain_arg1, int32_t obtain_arg2, int32_t obtain_arg3));
    MOCK_METHOD3(obtain, sp<Message>(const sp<Handler>& handler, int32_t obtain_what, sp<android::RefBase> obtain_spRef));
    MOCK_METHOD0(sendToTarget, bool());
    MOCK_CONST_METHOD0(dup, sp<Message>());
    MOCK_METHOD1(writeToParcel, error_t(Parcel* parcel));
    MOCK_METHOD1(readFromParcel, error_t(Parcel& parcel));
//     MOCK_METHOD0(dump, void());
//     MOCK_METHOD0(clear, void());
//     MOCK_METHOD1(setTo, void(const Message& other));
};

MockMessage * M_Message;

Message::Message()
{

}

Message::Message(Message& other)
{

}

Message::~Message()
{

}

sp<Message> Message::obtain()
{
    return M_Message->obtain();
}

sp<Message> Message::obtain(const Message* message)
{
    return M_Message->obtain(message);
}

sp<Message> Message::obtain(const sp<Handler>& handler)
{
    return M_Message->obtain(handler);
}

sp<Message> Message::obtain(const sp<Handler>& handler, int32_t obtain_what)
{
    return M_Message->obtain(handler, obtain_what);
}

sp<Message> Message::obtain(const sp<Handler>& handler, int32_t obtain_what, int32_t obtain_arg1)
{
    return M_Message->obtain(handler, obtain_what, obtain_arg1);
}

sp<Message> Message::obtain(const sp<Handler>& handler, int32_t obtain_what, void* obtain_obj)
{
    return M_Message->obtain(handler, obtain_what, obtain_obj);
}

sp<Message> Message::obtain(const sp<Handler>& handler, int32_t obtain_what, int32_t obtain_arg1, int32_t obtain_arg2)
{
    return M_Message->obtain(handler, obtain_what, obtain_arg1, obtain_arg2);
}

sp<Message> Message::obtain(const sp<Handler>& handler, int32_t obtain_what, int32_t obtain_arg1, int32_t obtain_arg2, void* obtain_obj)
{
    return M_Message->obtain(handler, obtain_what, obtain_arg1, obtain_arg2, obtain_obj);
}

sp<Message> Message::obtain(const sp<Handler>& handler, int32_t obtain_what, int32_t obtain_arg1, int32_t obtain_arg2, int32_t obtain_arg3)
{
    return M_Message->obtain(handler, obtain_what, obtain_arg1, obtain_arg2, obtain_arg3);
}

sp<Message> Message::obtain(const sp<Handler>& handler, int32_t obtain_what, sp<android::RefBase> obtain_spRef)
{
    return M_Message->obtain(handler, obtain_what, obtain_spRef);
}

bool Message::sendToTarget()
{
    return M_Message->sendToTarget();
}

sp<Message> Message::dup() const
{
    return M_Message->dup();
}

error_t Message::writeToParcel(Parcel* parcel)
{
    return M_Message->writeToParcel(parcel);
}

error_t Message::readFromParcel(Parcel& parcel)
{
    return M_Message->readFromParcel(parcel);
}

void Message::dump()
{
//    M_Message->dump();
}

void Message::clear()
{
//    M_Message->clear();
}

void Message::setTo(const Message& other)
{
//    M_Message->setTo(other);
}


}  // namespace sl
