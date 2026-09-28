namespace android {

class MockBufferedTextOutput {
  public:
    MOCK_METHOD2(print, status_t(const char* txt, size_t len));
//     MOCK_METHOD1(moveIndent, void(int delta));
//     MOCK_METHOD0(pushBundle, void());
//     MOCK_METHOD0(popBundle, void());
    MOCK_METHOD2(writeLines, status_t(const struct iovec& vec, size_t N));
    MOCK_METHOD0(getThreadState, ThreadState*());
//     MOCK_METHOD1(threadDestructor, void(void *st));
    MOCK_CONST_METHOD0(getBuffer, BufferState*());
};

MockBufferedTextOutput * M_BufferedTextOutput;

BufferedTextOutput::BufferedTextOutput(uint32_t flags)
{

}

BufferedTextOutput::~BufferedTextOutput()
{

}

status_t BufferedTextOutput::print(const char* txt, size_t len)
{
    return M_BufferedTextOutput->print(txt, len);
}

void BufferedTextOutput::moveIndent(int delta)
{
//    M_BufferedTextOutput->moveIndent(delta);
}

void BufferedTextOutput::pushBundle()
{
//    M_BufferedTextOutput->pushBundle();
}

void BufferedTextOutput::popBundle()
{
//    M_BufferedTextOutput->popBundle();
}

status_t BufferedTextOutput::writeLines(const struct iovec& vec, size_t N)
{
    return M_BufferedTextOutput->writeLines(vec, N);
}

ThreadState* BufferedTextOutput::getThreadState()
{
    return M_BufferedTextOutput->getThreadState();
}

void BufferedTextOutput::threadDestructor(void *st)
{
//    M_BufferedTextOutput->threadDestructor();
}

BufferState* BufferedTextOutput::getBuffer() const
{
    return M_BufferedTextOutput->getBuffer();
}


}  // namespace android
