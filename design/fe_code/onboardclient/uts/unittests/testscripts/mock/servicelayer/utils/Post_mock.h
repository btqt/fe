class MockPost {
  public:
    MOCK_METHOD0(obtain, android::sp<Post>());
    MOCK_METHOD1(obtain, android::sp<Post>(const Post* post));
    MOCK_METHOD1(obtain, android::sp<Post>(int32_t p_what));
    MOCK_METHOD2(obtain, android::sp<Post>(const char* p_action, Buffer buffer));
    MOCK_METHOD2(obtain, android::sp<Post>(int32_t p_what, int32_t p_arg1));
    MOCK_METHOD3(obtain, android::sp<Post>(int32_t P_what, int32_t p_arg1, int32_t p_arg2));
    MOCK_CONST_METHOD0(dup, android::sp<Post>());
    MOCK_METHOD1(writeToParcel, error_t(android::Parcel* parcel));
    MOCK_METHOD1(readFromParcel, error_t(android::Parcel& parcel));
//     MOCK_METHOD1(setAction, void(char* action));
    MOCK_METHOD0(getAction, char*());
//     MOCK_METHOD1(setDest, void(char* dest));
    MOCK_METHOD0(getDest, char*());
//     MOCK_METHOD0(dump, void());
//     MOCK_METHOD0(clear, void());
//     MOCK_METHOD1(setTo, void(const Post& other));
};

MockPost * M_Post;

Post::Post()
{

}

Post::Post(appid_t p_owner)
{

}

Post::Post(Post& other)
{

}

Post::~Post()
{

}

android::sp<Post> Post::obtain()
{
    return M_Post->obtain();
}

android::sp<Post> Post::obtain(const Post* post)
{
    return M_Post->obtain(post);
}

android::sp<Post> Post::obtain(int32_t p_what)
{
    return M_Post->obtain(p_what);
}

android::sp<Post> Post::obtain(const char* p_action, Buffer buffer)
{
    return M_Post->obtain(p_action, buffer);
}

android::sp<Post> Post::obtain(int32_t p_what, int32_t p_arg1)
{
    return M_Post->obtain(p_what, p_arg1);
}

android::sp<Post> Post::obtain(int32_t P_what, int32_t p_arg1, int32_t p_arg2)
{
    return M_Post->obtain(P_what, p_arg1, p_arg2);
}

android::sp<Post> Post::dup() const
{
    return M_Post->dup();
}

error_t Post::writeToParcel(android::Parcel* parcel)
{
    return M_Post->writeToParcel(parcel);
}

error_t Post::readFromParcel(android::Parcel& parcel)
{
    return M_Post->readFromParcel(parcel);
}

void Post::setAction(char* action)
{
//    M_Post->setAction(action);
}

char* Post::getAction()
{
    return M_Post->getAction();
}

void Post::setDest(char* dest)
{
//    M_Post->setDest(dest);
}

char* Post::getDest()
{
    return M_Post->getDest();
}

void Post::dump()
{
//    M_Post->dump();
}

void Post::clear()
{
//    M_Post->clear();
}

void Post::setTo(const Post& other)
{
//    M_Post->setTo(other);
}
