namespace mindroid {

class MockBundle {
  public:
//     MOCK_METHOD0(clear, void());
    MOCK_METHOD1(containsKey, bool(const char* key));
//     MOCK_METHOD1(remove, void(const char* key));
//     MOCK_METHOD2(putBool, void(const char* key, bool value));
//     MOCK_METHOD2(putByte, void(const char* key, uint8_t value));
//     MOCK_METHOD2(putChar, void(const char* key, char value));
//     MOCK_METHOD2(putInt16, void(const char* key, int16_t value));
//     MOCK_METHOD2(putUInt16, void(const char* key, uint16_t value));
//     MOCK_METHOD2(putInt32, void(const char* key, int32_t value));
//     MOCK_METHOD2(putUInt32, void(const char* key, uint32_t value));
//     MOCK_METHOD2(putInt64, void(const char* key, int64_t value));
//     MOCK_METHOD2(putUInt64, void(const char* key, uint64_t value));
//     MOCK_METHOD2(putFloat, void(const char* key, float value));
//     MOCK_METHOD2(putDouble, void(const char* key, double value));
//     MOCK_METHOD2(putString, void(const char* key, const char* string));
//     MOCK_METHOD2(putString, void(const char* key, const android::sp<String>& string));
//     MOCK_METHOD2(putObject, void(const char* key, const android::sp<RefBase>& object));
    MOCK_CONST_METHOD2(getBool, bool(const char* key, const bool defaultValue));
    MOCK_CONST_METHOD2(getByte, uint8_t(const char* key, const uint8_t defaultValue));
    MOCK_CONST_METHOD2(getChar, char(const char* key, const char defaultValue));
    MOCK_CONST_METHOD2(getInt16, int16_t(const char* key, const int16_t defaultValue));
    MOCK_CONST_METHOD2(getUInt16, uint16_t(const char* key, const uint16_t defaultValue));
    MOCK_CONST_METHOD2(getInt32, int32_t(const char* key, const int32_t defaultValue));
    MOCK_CONST_METHOD2(getUInt32, uint32_t(const char* key, const uint32_t defaultValue));
    MOCK_CONST_METHOD2(getInt64, int64_t(const char* key, const int64_t defaultValue));
    MOCK_CONST_METHOD2(getUInt64, uint64_t(const char* key, const uint64_t defaultValue));
    MOCK_CONST_METHOD2(getFloat, float(const char* key, const float defaultValue));
    MOCK_CONST_METHOD2(getDouble, double(const char* key, const double defaultValue));
    MOCK_CONST_METHOD1(getString, android::sp<String>(const char* key));
    MOCK_CONST_METHOD1(getObject, android::sp<T>(const char* key));
    MOCK_CONST_METHOD2(fillBool, bool(const char* key, bool& value));
    MOCK_CONST_METHOD2(fillByte, bool(const char* key, uint8_t& value));
    MOCK_CONST_METHOD2(fillChar, bool(const char* key, char& value));
    MOCK_CONST_METHOD2(fillInt16, bool(const char* key, int16_t& value));
    MOCK_CONST_METHOD2(fillUInt16, bool(const char* key, uint16_t& value));
    MOCK_CONST_METHOD2(fillInt32, bool(const char* key, int32_t& value));
    MOCK_CONST_METHOD2(fillUInt32, bool(const char* key, uint32_t& value));
    MOCK_CONST_METHOD2(fillInt64, bool(const char* key, int64_t& value));
    MOCK_CONST_METHOD2(fillUInt64, bool(const char* key, uint64_t& value));
    MOCK_CONST_METHOD2(fillFloat, bool(const char* key, float& value));
    MOCK_CONST_METHOD2(fillDouble, bool(const char* key, double& value));
    MOCK_CONST_METHOD2(fillString, bool(const char* key, android::sp<String>& string));
    MOCK_CONST_METHOD2(fillObject, bool(const char* key, android::sp<T>& object));
    MOCK_CONST_METHOD1(findValue, List const_iterator<KeyValuePair>(const char* key));
};

MockBundle * M_Bundle;

Bundle::Bundle()
{

}

Bundle::~Bundle()
{

}

void Bundle::clear()
{
//    M_Bundle->clear();
}

bool Bundle::containsKey(const char* key)
{
    return M_Bundle->containsKey(key);
}

void Bundle::remove(const char* key)
{
//    M_Bundle->remove(key);
}

void Bundle::putBool(const char* key, bool value)
{
//    M_Bundle->putBool(key, value);
}

void Bundle::putByte(const char* key, uint8_t value)
{
//    M_Bundle->putByte(key, value);
}

void Bundle::putChar(const char* key, char value)
{
//    M_Bundle->putChar(key, value);
}

void Bundle::putInt16(const char* key, int16_t value)
{
//    M_Bundle->putInt16(key, value);
}

void Bundle::putUInt16(const char* key, uint16_t value)
{
//    M_Bundle->putUInt16(key, value);
}

void Bundle::putInt32(const char* key, int32_t value)
{
//    M_Bundle->putInt32(key, value);
}

void Bundle::putUInt32(const char* key, uint32_t value)
{
//    M_Bundle->putUInt32(key, value);
}

void Bundle::putInt64(const char* key, int64_t value)
{
//    M_Bundle->putInt64(key, value);
}

void Bundle::putUInt64(const char* key, uint64_t value)
{
//    M_Bundle->putUInt64(key, value);
}

void Bundle::putFloat(const char* key, float value)
{
//    M_Bundle->putFloat(key, value);
}

void Bundle::putDouble(const char* key, double value)
{
//    M_Bundle->putDouble(key, value);
}

void Bundle::putString(const char* key, const char* string)
{
//    M_Bundle->putString(key, string);
}

void Bundle::putString(const char* key, const android::sp<String>& string)
{
//    M_Bundle->putString(key, string);
}

void Bundle::putObject(const char* key, const android::sp<RefBase>& object)
{
//    M_Bundle->putObject(key, object);
}

bool Bundle::getBool(const char* key, const bool defaultValue) const
{
    return M_Bundle->getBool(key, defaultValue);
}

uint8_t Bundle::getByte(const char* key, const uint8_t defaultValue) const
{
    return M_Bundle->getByte(key, defaultValue);
}

char Bundle::getChar(const char* key, const char defaultValue) const
{
    return M_Bundle->getChar(key, defaultValue);
}

int16_t Bundle::getInt16(const char* key, const int16_t defaultValue) const
{
    return M_Bundle->getInt16(key, defaultValue);
}

uint16_t Bundle::getUInt16(const char* key, const uint16_t defaultValue) const
{
    return M_Bundle->getUInt16(key, defaultValue);
}

int32_t Bundle::getInt32(const char* key, const int32_t defaultValue) const
{
    return M_Bundle->getInt32(key, defaultValue);
}

uint32_t Bundle::getUInt32(const char* key, const uint32_t defaultValue) const
{
    return M_Bundle->getUInt32(key, defaultValue);
}

int64_t Bundle::getInt64(const char* key, const int64_t defaultValue) const
{
    return M_Bundle->getInt64(key, defaultValue);
}

uint64_t Bundle::getUInt64(const char* key, const uint64_t defaultValue) const
{
    return M_Bundle->getUInt64(key, defaultValue);
}

float Bundle::getFloat(const char* key, const float defaultValue) const
{
    return M_Bundle->getFloat(key, defaultValue);
}

double Bundle::getDouble(const char* key, const double defaultValue) const
{
    return M_Bundle->getDouble(key, defaultValue);
}

android::sp<String> Bundle::getString(const char* key) const
{
    return M_Bundle->getString(key);
}

android::sp<T> Bundle::getObject(const char* key) const
{
    return M_Bundle->getObject(key);
}

bool Bundle::fillBool(const char* key, bool& value) const
{
    return M_Bundle->fillBool(key, value);
}

bool Bundle::fillByte(const char* key, uint8_t& value) const
{
    return M_Bundle->fillByte(key, value);
}

bool Bundle::fillChar(const char* key, char& value) const
{
    return M_Bundle->fillChar(key, value);
}

bool Bundle::fillInt16(const char* key, int16_t& value) const
{
    return M_Bundle->fillInt16(key, value);
}

bool Bundle::fillUInt16(const char* key, uint16_t& value) const
{
    return M_Bundle->fillUInt16(key, value);
}

bool Bundle::fillInt32(const char* key, int32_t& value) const
{
    return M_Bundle->fillInt32(key, value);
}

bool Bundle::fillUInt32(const char* key, uint32_t& value) const
{
    return M_Bundle->fillUInt32(key, value);
}

bool Bundle::fillInt64(const char* key, int64_t& value) const
{
    return M_Bundle->fillInt64(key, value);
}

bool Bundle::fillUInt64(const char* key, uint64_t& value) const
{
    return M_Bundle->fillUInt64(key, value);
}

bool Bundle::fillFloat(const char* key, float& value) const
{
    return M_Bundle->fillFloat(key, value);
}

bool Bundle::fillDouble(const char* key, double& value) const
{
    return M_Bundle->fillDouble(key, value);
}

bool Bundle::fillString(const char* key, android::sp<String>& string) const
{
    return M_Bundle->fillString(key, string);
}

bool Bundle::fillObject(const char* key, android::sp<T>& object) const
{
    return M_Bundle->fillObject(key, object);
}

List const_iterator<KeyValuePair> Bundle::findValue(const char* key) const
{
    return M_Bundle->findValue(key);
}

Bundle::Bundle(const Bundle&)
{

}


}  // namespace mindroid
