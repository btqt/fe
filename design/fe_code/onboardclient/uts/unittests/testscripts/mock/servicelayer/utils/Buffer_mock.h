class MockBuffer {
  public:
//     MOCK_METHOD1(setSize, void(int32_t len));
//     MOCK_METHOD1(setTo, void(const Buffer& set_buf));
//     MOCK_METHOD2(setTo, void(uint8_t* buf, int32_t len));
//     MOCK_METHOD2(setTo, void(char* buf, int32_t len));
//     MOCK_METHOD2(append, void(uint8_t* buf, int32_t len));
    MOCK_METHOD0(data, uint8_t*());
    MOCK_CONST_METHOD0(size, uint32_t());
    MOCK_METHOD0(empty, bool());
//     MOCK_METHOD0(dump, void());
//     MOCK_METHOD2(dump, void(uint8_t* s, int32_t len));
//     MOCK_METHOD0(clear, void());
//     MOCK_METHOD1(_assign, void(uint32_t _size));
//     MOCK_METHOD0(_release, void());
};

MockBuffer * M_Buffer;

Buffer::Buffer()
{

}

Buffer::Buffer(Buffer& other)
{

}

void Buffer::setSize(int32_t len)
{
//    M_Buffer->setSize(len);
}

void Buffer::setTo(const Buffer& set_buf)
{
//    M_Buffer->setTo(set_buf);
}

void Buffer::setTo(uint8_t* buf, int32_t len)
{
//    M_Buffer->setTo(buf, len);
}

void Buffer::setTo(char* buf, int32_t len)
{
//    M_Buffer->setTo(buf, len);
}

void Buffer::append(uint8_t* buf, int32_t len)
{
//    M_Buffer->append(buf, len);
}

uint8_t* Buffer::data()
{
    return M_Buffer->data();
}

uint32_t Buffer::size() const
{
    return M_Buffer->size();
}

bool Buffer::empty()
{
    return M_Buffer->empty();
}

void Buffer::dump()
{
//    M_Buffer->dump();
}

void Buffer::dump(uint8_t* s, int32_t len)
{
//    M_Buffer->dump(s, len);
}

void Buffer::clear()
{
//    M_Buffer->clear();
}

void Buffer::_assign(uint32_t _size)
{
//    M_Buffer->_assign(_size);
}

void Buffer::_release()
{
//    M_Buffer->_release();
}
