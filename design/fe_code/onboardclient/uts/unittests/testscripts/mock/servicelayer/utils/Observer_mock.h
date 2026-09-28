class MockObserver {
  public:
//     MOCK_METHOD0(get, void*());
    MOCK_METHOD1(==, bool operator(Observer& other));
//     MOCK_METHOD3(onNotify, void(int32_t cmd, int32_t status, int32_t reserved));
};

MockObserver * M_Observer;

Observer::Observer()
{

}

Observer::~Observer()
{

}

void* Observer::get()
{
//    M_Observer->get();
}

bool operator Observer::==(Observer& other)
{
    return M_Observer->==(other);
}

void Observer::onNotify(int32_t cmd, int32_t status, int32_t reserved)
{
//    M_Observer->onNotify(cmd, status, reserved);
}
