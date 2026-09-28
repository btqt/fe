namespace android {

class MockMemoryHeapBase {
  public:
    MOCK_CONST_METHOD0(getHeapID, int());
//     MOCK_CONST_METHOD0(getBase, void*());
    MOCK_CONST_METHOD0(getSize, size_t());
    MOCK_CONST_METHOD0(getFlags, uint32_t());
    MOCK_CONST_METHOD0(getOffset, uint32_t());
    MOCK_CONST_METHOD0(getDevice, const char*());
//     MOCK_METHOD0(dispose, void());
    MOCK_METHOD5(init, status_t(int , void *, int , int , char *));
    MOCK_METHOD3(mapfd, status_t(int , size_t , uint32_t ));
};

MockMemoryHeapBase * M_MemoryHeapBase;

MemoryHeapBase::MemoryHeapBase(int fd, size_t size, uint32_t flags, uint32_t offset)
{

}

MemoryHeapBase::MemoryHeapBase(char *device, size_t size, uint32_t flags)
{

}

MemoryHeapBase::MemoryHeapBase(size_t size, uint32_t flags, const *name)
{

}

MemoryHeapBase::~MemoryHeapBase()
{

}

int MemoryHeapBase::getHeapID() const
{
    return M_MemoryHeapBase->getHeapID();
}

void* MemoryHeapBase::getBase() const
{
//    M_MemoryHeapBase->getBase();
}

size_t MemoryHeapBase::getSize() const
{
    return M_MemoryHeapBase->getSize();
}

uint32_t MemoryHeapBase::getFlags() const
{
    return M_MemoryHeapBase->getFlags();
}

uint32_t MemoryHeapBase::getOffset() const
{
    return M_MemoryHeapBase->getOffset();
}

const char* MemoryHeapBase::getDevice() const
{
    return M_MemoryHeapBase->getDevice();
}

void MemoryHeapBase::dispose()
{
//    M_MemoryHeapBase->dispose();
}

MemoryHeapBase::MemoryHeapBase()
{

}

status_t MemoryHeapBase::init(int fd, void *base, int size, int flags, char *device)
{
    return M_MemoryHeapBase->init(fd, base, size, flags, device);
}

status_t MemoryHeapBase::mapfd(int fd, size_t size, uint32_t offset)
{
    return M_MemoryHeapBase->mapfd(fd, size, offset);
}


}  // namespace android
