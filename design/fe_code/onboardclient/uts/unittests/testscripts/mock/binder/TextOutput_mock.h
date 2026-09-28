namespace android {

class MockTextOutput {
  public:
    MOCK_METHOD2(print, status_t(const char* txt, size_t len));
//     MOCK_METHOD1(moveIndent, void(int delta));
//     MOCK_METHOD0(pushBundle, void());
//     MOCK_METHOD0(popBundle, void());
};

MockTextOutput * M_TextOutput;

class MockTypeCode {
  public:
    MOCK_CONST_METHOD0(typeCode, uint32_t());
};

MockTypeCode * M_TypeCode;

class MockHexDump {
  public:
    MOCK_METHOD1(setBytesPerLine, HexDump&(size_t bytesPerLine));
    MOCK_METHOD1(setSingleLineCutoff, HexDump&(int32_t bytes));
    MOCK_METHOD1(setAlignment, HexDump&(size_t alignment));
    MOCK_METHOD1(setCArrayStyle, HexDump&(bool enabled));
    MOCK_CONST_METHOD0(buffer, const void*());
    MOCK_CONST_METHOD0(size, size_t());
    MOCK_CONST_METHOD0(bytesPerLine, size_t());
    MOCK_CONST_METHOD0(singleLineCutoff, int32_t());
    MOCK_CONST_METHOD0(alignment, size_t());
    MOCK_CONST_METHOD0(carrayStyle, bool());
};

MockHexDump * M_HexDump;

TextOutput::TextOutput()
{

}

TextOutput::~TextOutput()
{

}

status_t TextOutput::print(const char* txt, size_t len)
{
    return M_TextOutput->print(txt, len);
}

void TextOutput::moveIndent(int delta)
{
//    M_TextOutput->moveIndent(delta);
}

void TextOutput::pushBundle()
{
//    M_TextOutput->pushBundle();
}

void TextOutput::popBundle()
{
//    M_TextOutput->popBundle();
}


TypeCode::TypeCode(uint32_t code)
{

}

TypeCode::~TypeCode()
{

}

uint32_t TypeCode::typeCode() const
{
    return M_TypeCode->typeCode();
}


HexDump::HexDump(void *buf, size_t size, size_t bytesPerLine)
{

}

HexDump::~HexDump()
{

}

HexDump& HexDump::setBytesPerLine(size_t bytesPerLine)
{
    return M_HexDump->setBytesPerLine(bytesPerLine);
}

HexDump& HexDump::setSingleLineCutoff(int32_t bytes)
{
    return M_HexDump->setSingleLineCutoff(bytes);
}

HexDump& HexDump::setAlignment(size_t alignment)
{
    return M_HexDump->setAlignment(alignment);
}

HexDump& HexDump::setCArrayStyle(bool enabled)
{
    return M_HexDump->setCArrayStyle(enabled);
}

const void* HexDump::buffer() const
{
    return M_HexDump->buffer();
}

size_t HexDump::size() const
{
    return M_HexDump->size();
}

size_t HexDump::bytesPerLine() const
{
    return M_HexDump->bytesPerLine();
}

int32_t HexDump::singleLineCutoff() const
{
    return M_HexDump->singleLineCutoff();
}

size_t HexDump::alignment() const
{
    return M_HexDump->alignment();
}

bool HexDump::carrayStyle() const
{
    return M_HexDump->carrayStyle();
}


}  // namespace android
