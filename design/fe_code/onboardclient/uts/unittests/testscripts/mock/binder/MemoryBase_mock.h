namespace android {

class MockMemoryBase {
  public:
    MOCK_CONST_METHOD2(getMemory, sp<IMemoryHeap>(ssize_t* offset, size_t* size));
};

MockMemoryBase * M_MemoryBase;

MemoryBase::MemoryBase(const sp<IMemoryHeap>& heap, ssize_t offset, size_t size)
{

}

MemoryBase::~MemoryBase()
{

}

sp<IMemoryHeap> MemoryBase::getMemory(ssize_t* offset, size_t* size) const
{
    return M_MemoryBase->getMemory(offset, size);
}


}  // namespace android
