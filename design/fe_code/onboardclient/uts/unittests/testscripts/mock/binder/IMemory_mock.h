namespace android {

class MockIMemoryHeap {
  public:
//     MOCK_METHOD0(DECLARE_META_INTERFACE, void(MemoryHeap));
    MOCK_CONST_METHOD0(getHeapID, int());
//     MOCK_CONST_METHOD0(getBase, void*());
    MOCK_CONST_METHOD0(getSize, size_t());
    MOCK_CONST_METHOD0(getFlags, uint32_t());
    MOCK_CONST_METHOD0(getOffset, uint32_t());
};

MockIMemoryHeap * M_IMemoryHeap;

class MockBnMemoryHeap {
  public:
    MOCK_METHOD4(onTransact, status_t(uint32_t , Parcel , Parcel *, uint32_t ));
};

MockBnMemoryHeap * M_BnMemoryHeap;

class MockIMemory {
  public:
//     MOCK_METHOD0(DECLARE_META_INTERFACE, void(Memory));
    MOCK_CONST_METHOD2(getMemory, sp<IMemoryHeap>(ssize_t *, size_t *));
//     MOCK_CONST_METHOD2(fastPointer, void*(const sp<IBinder>& heap, ssize_t offset));
//     MOCK_CONST_METHOD0(pointer, void*());
    MOCK_CONST_METHOD0(size, size_t());
    MOCK_CONST_METHOD0(offset, ssize_t());
};

MockIMemory * M_IMemory;

class MockBnMemory {
  public:
    MOCK_METHOD4(onTransact, status_t(uint32_t , Parcel , Parcel *, uint32_t ));
};

MockBnMemory * M_BnMemory;

void IMemoryHeap::DECLARE_META_INTERFACE(MemoryHeap)
{
//    M_IMemoryHeap->DECLARE_META_INTERFACE(MemoryHeap);
}

int IMemoryHeap::getHeapID() const
{
    return M_IMemoryHeap->getHeapID();
}

void* IMemoryHeap::getBase() const
{
//    M_IMemoryHeap->getBase();
}

size_t IMemoryHeap::getSize() const
{
    return M_IMemoryHeap->getSize();
}

uint32_t IMemoryHeap::getFlags() const
{
    return M_IMemoryHeap->getFlags();
}

uint32_t IMemoryHeap::getOffset() const
{
    return M_IMemoryHeap->getOffset();
}


status_t BnMemoryHeap::onTransact(uint32_t code, Parcel data, Parcel *reply, uint32_t flags)
{
    return M_BnMemoryHeap->onTransact(code, data, reply, flags);
}

BnMemoryHeap::BnMemoryHeap()
{

}

BnMemoryHeap::~BnMemoryHeap()
{

}


void IMemory::DECLARE_META_INTERFACE(Memory)
{
//    M_IMemory->DECLARE_META_INTERFACE(Memory);
}

sp<IMemoryHeap> IMemory::getMemory(ssize_t *offset, size_t *size) const
{
    return M_IMemory->getMemory(offset, size);
}

void* IMemory::fastPointer(const sp<IBinder>& heap, ssize_t offset) const
{
//    M_IMemory->fastPointer(heap, offset);
}

void* IMemory::pointer() const
{
//    M_IMemory->pointer();
}

size_t IMemory::size() const
{
    return M_IMemory->size();
}

ssize_t IMemory::offset() const
{
    return M_IMemory->offset();
}


status_t BnMemory::onTransact(uint32_t code, Parcel data, Parcel *reply, uint32_t flags)
{
    return M_BnMemory->onTransact(code, data, reply, flags);
}

BnMemory::BnMemory()
{

}

BnMemory::~BnMemory()
{

}


}  // namespace android
