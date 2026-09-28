class MockLogOutput {
  public:
//     MOCK_METHOD2(print, void(const char *fmt, ...));
};

MockLogOutput * M_LogOutput;

void LogOutput::print(const char *fmt, ...)
{
//    M_LogOutput->print(fmt, ???);
}

class TestLogOutput : public LogOutput
{
    public :
        TestLogOutput(){};
        virtual ~TestLogOutput(){};
        virtual void print(const char *fmt, ...)
        {}
};
