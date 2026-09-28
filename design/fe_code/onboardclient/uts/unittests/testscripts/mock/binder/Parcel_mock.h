namespace android {

class MockParcel {
  public:
    MOCK_CONST_METHOD0(data, const uint8_t*());
    MOCK_CONST_METHOD0(dataSize, size_t());
    MOCK_CONST_METHOD0(dataAvail, size_t());
    MOCK_CONST_METHOD0(dataPosition, size_t());
    MOCK_CONST_METHOD0(dataCapacity, size_t());
    MOCK_METHOD1(setDataSize, status_t(size_t size));
//     MOCK_CONST_METHOD1(setDataPosition, void(size_t pos));
    MOCK_METHOD1(setDataCapacity, status_t(size_t size));
    MOCK_METHOD2(setData, status_t(const uint8_t* buffer, size_t len));
    MOCK_METHOD3(appendFrom, status_t(const Parcel *parcel, size_t start, size_t len));
    MOCK_METHOD1(pushAllowFds, bool(bool allowFds));
//     MOCK_METHOD1(restoreAllowFds, void(bool lastValue));
    MOCK_CONST_METHOD0(hasFileDescriptors, bool());
    MOCK_METHOD1(writeInterfaceToken, status_t(const String16& interface));
    MOCK_CONST_METHOD2(enforceInterface, bool(String16 , IPCThreadState *));
    MOCK_CONST_METHOD1(checkInterface, bool(IBinder*));
//     MOCK_METHOD0(freeData, void());
    MOCK_CONST_METHOD0(objects, const size_t*());
    MOCK_CONST_METHOD0(objectsCount, size_t());
    MOCK_CONST_METHOD0(errorCheck, status_t());
//     MOCK_METHOD1(setError, void(status_t err));
    MOCK_METHOD2(write, status_t(const void* data, size_t len));
//     MOCK_METHOD1(writeInplace, void*(size_t len));
    MOCK_METHOD2(writeUnpadded, status_t(const void* data, size_t len));
    MOCK_METHOD1(writeInt32, status_t(int32_t val));
    MOCK_METHOD1(writeUint32, status_t(uint32_t val));
    MOCK_METHOD1(writeInt64, status_t(int64_t val));
    MOCK_METHOD1(writeUint64, status_t(uint64_t val));
    MOCK_METHOD1(writeFloat, status_t(float val));
    MOCK_METHOD1(writeDouble, status_t(double val));
    MOCK_METHOD1(writeIntPtr, status_t(intptr_t val));
    MOCK_METHOD1(writeCString, status_t(const char* str));
#ifdef uts_remove
    MOCK_METHOD1(writeString8, status_t(const String8& str));
#endif
    MOCK_METHOD1(writeString16, status_t(const String16& str));
    MOCK_METHOD2(writeString16, status_t(const char16_t* str, size_t len));
    MOCK_METHOD1(writeStrongBinder, status_t(const sp<IBinder>& val));
    MOCK_METHOD1(writeWeakBinder, status_t(const wp<IBinder>& val));
    MOCK_METHOD2(writeInt32Array, status_t(size_t len, const int32_t *val));
    MOCK_METHOD2(writeByteArray, status_t(size_t len, const uint8_t *val));
#ifdef uts_remove
    MOCK_METHOD1(write, status_t(const Flattenable<T>& val));
    MOCK_METHOD1(write, status_t(const LightFlattenable<T>& val));
#endif
    MOCK_METHOD1(writeNativeHandle, status_t(const native_handle* handle));
    MOCK_METHOD2(writeFileDescriptor, status_t(int , bool ));
    MOCK_METHOD1(writeDupFileDescriptor, status_t(int fd));
#ifdef uts_remove
    MOCK_METHOD2(writeBlob, status_t(size_t len, WritableBlob* outBlob));
    MOCK_METHOD2(writeObject, status_t(const flat_binder_object& val, bool nullMetaData));
#endif
    MOCK_METHOD0(writeNoException, status_t());
//     MOCK_METHOD2(remove, void(size_t start, size_t amt));
    MOCK_CONST_METHOD2(read, status_t(void* outData, size_t len));
    MOCK_CONST_METHOD1(readInplace, const void*(size_t len));
    MOCK_CONST_METHOD0(readInt32, int32_t());
    MOCK_CONST_METHOD1(readInt32, status_t(int32_t *pArg));
    MOCK_CONST_METHOD0(readUint32, uint32_t());
    MOCK_CONST_METHOD1(readUint32, status_t(uint32_t *pArg));
    MOCK_CONST_METHOD0(readInt64, int64_t());
    MOCK_CONST_METHOD1(readInt64, status_t(int64_t *pArg));
    MOCK_CONST_METHOD0(readUint64, uint64_t());
    MOCK_CONST_METHOD1(readUint64, status_t(uint64_t *pArg));
    MOCK_CONST_METHOD0(readFloat, float());
    MOCK_CONST_METHOD1(readFloat, status_t(float *pArg));
    MOCK_CONST_METHOD0(readDouble, double());
    MOCK_CONST_METHOD1(readDouble, status_t(double *pArg));
    MOCK_CONST_METHOD0(readIntPtr, intptr_t());
    MOCK_CONST_METHOD1(readIntPtr, status_t(intptr_t *pArg));
    MOCK_CONST_METHOD0(readCString, const char*());
#ifdef uts_remove
    MOCK_CONST_METHOD0(readString8, String8());
#endif
    MOCK_CONST_METHOD0(readString16, String16());
    MOCK_CONST_METHOD1(readString16Inplace, const char16_t*(size_t* outLen));
    MOCK_CONST_METHOD0(readStrongBinder, sp<IBinder>());
    MOCK_CONST_METHOD0(readWeakBinder, wp<IBinder>());
//     MOCK_CONST_METHOD2(readByteArray, void(const uint8_t** data, size_t* len));
#ifdef uts_remove
    MOCK_CONST_METHOD1(read, status_t(Flattenable<T>& val));
    MOCK_CONST_METHOD1(read, status_t(LightFlattenable<T>& val));
#endif
    MOCK_CONST_METHOD0(readExceptionCode, int32_t());
    MOCK_CONST_METHOD0(readNativeHandle, native_handle*());
    MOCK_CONST_METHOD0(readFileDescriptor, int());
#ifdef uts_remove
    MOCK_CONST_METHOD2(readBlob, status_t(size_t len, ReadableBlob* outBlob));
    MOCK_CONST_METHOD1(readObject, const flat_binder_object*(bool nullMetaData));
#endif
//     MOCK_METHOD0(closeFileDescriptors, void());
    MOCK_CONST_METHOD0(ipcData, const uint8_t*());
    MOCK_CONST_METHOD0(ipcDataSize, size_t());
    MOCK_CONST_METHOD0(ipcObjects, const size_t*());
    MOCK_CONST_METHOD0(ipcObjectsCount, size_t());
//     MOCK_METHOD6(ipcSetDataReference, void(const uint8_t* data, size_t dataSize, const size_t* objects, size_t objectsCount, release_func relFunc, void* relCookie));
//     MOCK_CONST_METHOD2(print, void(TextOutput , uint32_t ));
    MOCK_METHOD1(finishWrite, status_t(size_t len));
//     MOCK_METHOD0(releaseObjects, void());
//     MOCK_METHOD0(acquireObjects, void());
    MOCK_METHOD1(growData, status_t(size_t len));
    MOCK_METHOD1(restartWrite, status_t(size_t desired));
    MOCK_METHOD1(continueWrite, status_t(size_t desired));
//     MOCK_METHOD0(freeDataNoInit, void());
//     MOCK_METHOD0(initState, void());
//     MOCK_CONST_METHOD0(scanForFds, void());
#ifdef uts_remove
    MOCK_CONST_METHOD1(readAligned, status_t(T *pArg));
    MOCK_CONST_METHOD0(readAligned, T());
    MOCK_METHOD1(writeAligned, status_t(T val));
    MOCK_METHOD1(write, status_t(const FlattenableHelperInterface& val));
    MOCK_CONST_METHOD1(read, status_t(FlattenableHelperInterface& val));
#endif
};

MockParcel * M_Parcel;

Parcel::Parcel()
{

}

Parcel::~Parcel()
{

}

const uint8_t* Parcel::data() const
{
    return M_Parcel->data();
}

size_t Parcel::dataSize() const
{
    return M_Parcel->dataSize();
}

size_t Parcel::dataAvail() const
{
    return M_Parcel->dataAvail();
}

size_t Parcel::dataPosition() const
{
    return M_Parcel->dataPosition();
}

size_t Parcel::dataCapacity() const
{
    return M_Parcel->dataCapacity();
}

status_t Parcel::setDataSize(size_t size)
{
    return M_Parcel->setDataSize(size);
}

void Parcel::setDataPosition(size_t pos) const
{
//    M_Parcel->setDataPosition(pos);
}

status_t Parcel::setDataCapacity(size_t size)
{
    return M_Parcel->setDataCapacity(size);
}

status_t Parcel::setData(const uint8_t* buffer, size_t len)
{
    return M_Parcel->setData(buffer, len);
}

status_t Parcel::appendFrom(const Parcel *parcel, size_t start, size_t len)
{
    return M_Parcel->appendFrom(parcel, start, len);
}

bool Parcel::pushAllowFds(bool allowFds)
{
    return M_Parcel->pushAllowFds(allowFds);
}

void Parcel::restoreAllowFds(bool lastValue)
{
//    M_Parcel->restoreAllowFds(lastValue);
}

bool Parcel::hasFileDescriptors() const
{
    return M_Parcel->hasFileDescriptors();
}

status_t Parcel::writeInterfaceToken(const String16& interface)
{
    return M_Parcel->writeInterfaceToken(interface);
}

#ifdef uts_remove
bool Parcel::enforceInterface(String16 interface, IPCThreadState *threadState) const
{
    return M_Parcel->enforceInterface(interface, threadState);
}
#endif

bool Parcel::checkInterface(IBinder* iBinder) const
{
    return M_Parcel->checkInterface(iBinder);
}

void Parcel::freeData()
{
//    M_Parcel->freeData();
}

#ifdef uts_remove
const size_t* Parcel::objects() const
{
    return M_Parcel->objects();
}
#endif

size_t Parcel::objectsCount() const
{
    return M_Parcel->objectsCount();
}

status_t Parcel::errorCheck() const
{
    return M_Parcel->errorCheck();
}

void Parcel::setError(status_t err)
{
//    M_Parcel->setError(err);
}

status_t Parcel::write(const void* data, size_t len)
{
    return M_Parcel->write(data, len);
}

void* Parcel::writeInplace(size_t len)
{
//    M_Parcel->writeInplace(len);
}

status_t Parcel::writeUnpadded(const void* data, size_t len)
{
    return M_Parcel->writeUnpadded(data, len);
}

status_t Parcel::writeInt32(int32_t val)
{
     return M_Parcel->writeInt32(val);
}

status_t Parcel::writeUint32(uint32_t val)
{
    return M_Parcel->writeUint32(val);
}

status_t Parcel::writeInt64(int64_t val)
{
    return M_Parcel->writeInt64(val);
}

status_t Parcel::writeUint64(uint64_t val)
{
    return M_Parcel->writeUint64(val);
}

status_t Parcel::writeFloat(float val)
{
    return M_Parcel->writeFloat(val);
}

status_t Parcel::writeDouble(double val)
{
    return M_Parcel->writeDouble(val);
}

#ifdef uts_remove
status_t Parcel::writeIntPtr(intptr_t val)
{
    return M_Parcel->writeIntPtr(val);
}
#endif

status_t Parcel::writeCString(const char* str)
{
    return M_Parcel->writeCString(str);
}

#ifdef uts_remove
status_t Parcel::writeString8(const String8& str)
{
    return M_Parcel->writeString8(str);
}
#endif

status_t Parcel::writeString16(const String16& str)
{
    return M_Parcel->writeString16(str);
}

status_t Parcel::writeString16(const char16_t* str, size_t len)
{
    return M_Parcel->writeString16(str, len);
}

status_t Parcel::writeStrongBinder(const sp<IBinder>& val)
{
    return M_Parcel->writeStrongBinder(val);
}

status_t Parcel::writeWeakBinder(const wp<IBinder>& val)
{
    return M_Parcel->writeWeakBinder(val);
}

status_t Parcel::writeInt32Array(size_t len, const int32_t *val)
{
    return M_Parcel->writeInt32Array(len, val);
}

status_t Parcel::writeByteArray(size_t len, const uint8_t *val)
{
    return M_Parcel->writeByteArray(len, val);
}

#ifdef uts_remove
status_t Parcel::write(const Flattenable<T>& val)
{
    return M_Parcel->write(val);
}

status_t Parcel::write(const LightFlattenable<T>& val)
{
    return M_Parcel->write(val);
}
#endif

status_t Parcel::writeNativeHandle(const native_handle* handle)
{
    return M_Parcel->writeNativeHandle(handle);
}

status_t Parcel::writeFileDescriptor(int fd, bool takeOwnership)
{
    return M_Parcel->writeFileDescriptor(fd, takeOwnership);
}

status_t Parcel::writeDupFileDescriptor(int fd)
{
    return M_Parcel->writeDupFileDescriptor(fd);
}

#ifdef uts_remove
status_t Parcel::writeBlob(size_t len, WritableBlob* outBlob)
{
    return M_Parcel->writeBlob(len, outBlob);
}

status_t Parcel::writeObject(const flat_binder_object& val, bool nullMetaData)
{
    return M_Parcel->writeObject(val, nullMetaData);
}
#endif

status_t Parcel::writeNoException()
{
    return M_Parcel->writeNoException();
}

void Parcel::remove(size_t start, size_t amt)
{
//    M_Parcel->remove(start, amt);
}


status_t Parcel::read(void* outData, size_t len) const
{
    return M_Parcel->read(outData,  len);
}


const void* Parcel::readInplace(size_t len) const
{
    return M_Parcel->readInplace(len);
}

int32_t Parcel::readInt32() const
{
    return M_Parcel->readInt32();
}

status_t Parcel::readInt32(int32_t *pArg) const
{
    return M_Parcel->readInt32(pArg);
}

uint32_t Parcel::readUint32() const
{
    return M_Parcel->readUint32();
}

status_t Parcel::readUint32(uint32_t *pArg) const
{
    return M_Parcel->readUint32(pArg);
}

int64_t Parcel::readInt64() const
{
    return M_Parcel->readInt64();
}

status_t Parcel::readInt64(int64_t *pArg) const
{
    return M_Parcel->readInt64(pArg);
}

uint64_t Parcel::readUint64() const
{
    return M_Parcel->readUint64();
}

status_t Parcel::readUint64(uint64_t *pArg) const
{
    return M_Parcel->readUint64(pArg);
}

float Parcel::readFloat() const
{
    return M_Parcel->readFloat();
}

status_t Parcel::readFloat(float *pArg) const
{
    return M_Parcel->readFloat(pArg);
}

double Parcel::readDouble() const
{
    return M_Parcel->readDouble();
}

status_t Parcel::readDouble(double *pArg) const
{
    return M_Parcel->readDouble(pArg);
}

intptr_t Parcel::readIntPtr() const
{
    return M_Parcel->readIntPtr();
}

status_t Parcel::readIntPtr(intptr_t *pArg) const
{
    return M_Parcel->readIntPtr(pArg);
}

const char* Parcel::readCString() const
{
    return M_Parcel->readCString();
}

#ifdef uts_remove
String8 Parcel::readString8() const
{
    return M_Parcel->readString8();
}
#endif

String16 Parcel::readString16() const
{
    return M_Parcel->readString16();
}

const char16_t* Parcel::readString16Inplace(size_t* outLen) const
{
    return M_Parcel->readString16Inplace(outLen);
}

sp<IBinder> Parcel::readStrongBinder() const
{
    return M_Parcel->readStrongBinder();
}

wp<IBinder> Parcel::readWeakBinder() const
{
    return M_Parcel->readWeakBinder();
}

void Parcel::readByteArray(const uint8_t** data, size_t* len) const
{
//    M_Parcel->readByteArray(data, len);
}

#ifdef uts_remove
status_t Parcel::read(Flattenable<T>& val) const
{
    return M_Parcel->read(val);
}

status_t Parcel::read(LightFlattenable<T>& val) const
{
    return M_Parcel->read(val);
}
#endif

int32_t Parcel::readExceptionCode() const
{
    return M_Parcel->readExceptionCode();
}

native_handle* Parcel::readNativeHandle() const
{
    return M_Parcel->readNativeHandle();
}

int Parcel::readFileDescriptor() const
{
    return M_Parcel->readFileDescriptor();
}

#ifdef uts_remove
status_t Parcel::readBlob(size_t len, ReadableBlob* outBlob) const
{
    return M_Parcel->readBlob(len, outBlob);
}

const flat_binder_object* Parcel::readObject(bool nullMetaData) const
{
    return M_Parcel->readObject(nullMetaData);
}
#endif

void Parcel::closeFileDescriptors()
{
//    M_Parcel->closeFileDescriptors();
}

#ifdef uts_remove
const uint8_t* Parcel::ipcData() const
{
    return M_Parcel->ipcData();
}

size_t Parcel::ipcDataSize() const
{
    return M_Parcel->ipcDataSize();
}

const size_t* Parcel::ipcObjects() const
{
    return M_Parcel->ipcObjects();
}

size_t Parcel::ipcObjectsCount() const
{
    return M_Parcel->ipcObjectsCount();
}
#endif

#ifdef uts_remove
void Parcel::ipcSetDataReference(const uint8_t* data, size_t dataSize, const size_t* objects, size_t objectsCount, release_func relFunc, void* relCookie)
{
//    M_Parcel->ipcSetDataReference(data, dataSize, objects, objectsCount, relFunc, relCookie);
}

void Parcel::print(TextOutput to, uint32_t flags) const
{
//    M_Parcel->print(to, flags);
}
#endif

Parcel::Parcel(const Parcel& o)
{

}

status_t Parcel::finishWrite(size_t len)
{
    return M_Parcel->finishWrite(len);
}

void Parcel::releaseObjects()
{
//    M_Parcel->releaseObjects();
}

void Parcel::acquireObjects()
{
//    M_Parcel->acquireObjects();
}

status_t Parcel::growData(size_t len)
{
    return M_Parcel->growData(len);
}

status_t Parcel::restartWrite(size_t desired)
{
    return M_Parcel->restartWrite(desired);
}

status_t Parcel::continueWrite(size_t desired)
{
    return M_Parcel->continueWrite(desired);
}

void Parcel::freeDataNoInit()
{
//    M_Parcel->freeDataNoInit();
}

void Parcel::initState()
{
//    M_Parcel->initState();
}

void Parcel::scanForFds() const
{
//    M_Parcel->scanForFds();
}

#ifdef uts_remove
status_t Parcel::readAligned(T *pArg) const
{
    return M_Parcel->readAligned(pArg);
}

T Parcel::readAligned() const
{
    return M_Parcel->readAligned();
}

status_t Parcel::writeAligned(T val)
{
    return M_Parcel->writeAligned(val);
}

status_t Parcel::write(const FlattenableHelperInterface& val)
{
    return M_Parcel->write(val);
}

status_t Parcel::read(FlattenableHelperInterface& val) const
{
    return M_Parcel->read(val);
}
#endif


}  // namespace android
