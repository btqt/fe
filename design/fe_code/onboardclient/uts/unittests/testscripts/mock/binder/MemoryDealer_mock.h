namespace android {

class MockMemoryDealer {
  public:
    MOCK_METHOD1(allocate, sp<IMemory>(size_t size));
//     MOCK_METHOD1(deallocate, void(size_t offset));
//     MOCK_CONST_METHOD1(dump, void(const char* what));
    MOCK_CONST_METHOD0(heap, const sp<IMemoryHeap>&());
    MOCK_CONST_METHOD0(allocator, SimpleBestFitAllocator*());
};

MockMemoryDealer * M_MemoryDealer;

MemoryDealer::MemoryDealer(size_t size, char *name)
{

}

sp<IMemory> MemoryDealer::allocate(size_t size)
{
    return M_MemoryDealer->allocate(size);
}

void MemoryDealer::deallocate(size_t offset)
{
//    M_MemoryDealer->deallocate(offset);
}

void MemoryDealer::dump(const char* what) const
{
//    M_MemoryDealer->dump(what);
}

MemoryDealer::~MemoryDealer()
{

}

const sp<IMemoryHeap>& MemoryDealer::heap() const
{
    return M_MemoryDealer->heap();
}

SimpleBestFitAllocator* MemoryDealer::allocator() const
{
    return M_MemoryDealer->allocator();
}


}  // namespace android
