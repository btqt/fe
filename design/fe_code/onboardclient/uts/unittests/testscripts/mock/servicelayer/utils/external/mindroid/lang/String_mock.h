namespace mindroid {

class MockString {
  public:
    MOCK_CONST_METHOD1(equals, bool(const char* string));
    MOCK_CONST_METHOD1(equals, bool(const android::sp<String>& string));
    MOCK_CONST_METHOD1(equalsIgnoreCase, bool(const char* string));
    MOCK_CONST_METHOD1(equalsIgnoreCase, bool(const android::sp<String>& string));
    MOCK_CONST_METHOD1(contains, bool(const char* subString));
    MOCK_CONST_METHOD1(contains, bool(const android::sp<String>& subString));
    MOCK_CONST_METHOD1(startsWith, bool(const char* prefix));
    MOCK_CONST_METHOD1(startsWith, bool(const android::sp<String>& prefix));
    MOCK_CONST_METHOD1(endsWith, bool(const char* suffix));
    MOCK_CONST_METHOD1(endsWith, bool(const android::sp<String>& suffix));
    MOCK_CONST_METHOD1(substr, android::sp<String>(size_t beginIndex));
    MOCK_CONST_METHOD2(substr, android::sp<String>(size_t beginIndex, size_t endIndex));
    MOCK_CONST_METHOD0(toLowerCase, android::sp<String>());
    MOCK_CONST_METHOD0(toUpperCase, android::sp<String>());
    MOCK_CONST_METHOD1(indexOf, ssize_t(const char character));
    MOCK_CONST_METHOD1(indexOf, ssize_t(const char* string));
    MOCK_CONST_METHOD1(indexOf, ssize_t(const android::sp<String>& string));
    MOCK_CONST_METHOD2(indexOf, ssize_t(const char character, size_t fromIndex));
    MOCK_CONST_METHOD2(indexOf, ssize_t(const char* string, size_t fromIndex));
    MOCK_CONST_METHOD2(indexOf, ssize_t(const android::sp<String>& string, size_t fromIndex));
    MOCK_CONST_METHOD0(trim, android::sp<String>());
    MOCK_CONST_METHOD1(left, android::sp<String>(size_t n));
    MOCK_CONST_METHOD1(right, android::sp<String>(size_t n));
    MOCK_CONST_METHOD1(split, android::sp<mindroid::List>(const char* separator));
    MOCK_CONST_METHOD1(split, android::sp<mindroid::List>(const android::sp<String>& separator));
    MOCK_CONST_METHOD1(splits, android::sp<mindroid::List>(const char* separator));
    MOCK_CONST_METHOD2(append, android::sp<String>(const char* data, size_t size));
    MOCK_CONST_METHOD2(appendFormatted, android::sp<String>(const char* format, ...));
    MOCK_METHOD2(format, android::sp<String>(const char* format, ...));
    MOCK_CONST_METHOD2(appendFormattedWithVarArgList, android::sp<String>(const char* format, va_list args));
};

MockString * M_String;

String::String()
{

}

String::String(const char* string)
{

}

String::String(const char* string, size_t size)
{

}

bool String::equals(const char* string) const
{
    return M_String->equals(string);
}

bool String::equals(const android::sp<String>& string) const
{
    return M_String->equals(string);
}

bool String::equalsIgnoreCase(const char* string) const
{
    return M_String->equalsIgnoreCase(string);
}

bool String::equalsIgnoreCase(const android::sp<String>& string) const
{
    return M_String->equalsIgnoreCase(string);
}

bool String::contains(const char* subString) const
{
    return M_String->contains(subString);
}

bool String::contains(const android::sp<String>& subString) const
{
    return M_String->contains(subString);
}

bool String::startsWith(const char* prefix) const
{
    return M_String->startsWith(prefix);
}

bool String::startsWith(const android::sp<String>& prefix) const
{
    return M_String->startsWith(prefix);
}

bool String::endsWith(const char* suffix) const
{
    return M_String->endsWith(suffix);
}

bool String::endsWith(const android::sp<String>& suffix) const
{
    return M_String->endsWith(suffix);
}

android::sp<String> String::substr(size_t beginIndex) const
{
    return M_String->substr(beginIndex);
}

android::sp<String> String::substr(size_t beginIndex, size_t endIndex) const
{
    return M_String->substr(beginIndex, endIndex);
}

android::sp<String> String::toLowerCase() const
{
    return M_String->toLowerCase();
}

android::sp<String> String::toUpperCase() const
{
    return M_String->toUpperCase();
}

ssize_t String::indexOf(const char character) const
{
    return M_String->indexOf(character);
}

ssize_t String::indexOf(const char* string) const
{
    return M_String->indexOf(string);
}

ssize_t String::indexOf(const android::sp<String>& string) const
{
    return M_String->indexOf(string);
}

ssize_t String::indexOf(const char character, size_t fromIndex) const
{
    return M_String->indexOf(character, fromIndex);
}

ssize_t String::indexOf(const char* string, size_t fromIndex) const
{
    return M_String->indexOf(string, fromIndex);
}

ssize_t String::indexOf(const android::sp<String>& string, size_t fromIndex) const
{
    return M_String->indexOf(string, fromIndex);
}

android::sp<String> String::trim() const
{
    return M_String->trim();
}

android::sp<String> String::left(size_t n) const
{
    return M_String->left(n);
}

android::sp<String> String::right(size_t n) const
{
    return M_String->right(n);
}

android::sp<mindroid::List> String::split(const char* separator) const
{
    return M_String->split(separator);
}

android::sp<mindroid::List> String::split(const android::sp<String>& separator) const
{
    return M_String->split(separator);
}

android::sp<mindroid::List> String::splits(const char* separator) const
{
    return M_String->splits(separator);
}

android::sp<String> String::append(const char* data, size_t size) const
{
    return M_String->append(data, size);
}

android::sp<String> String::appendFormatted(const char* format, ...) const
{
    return M_String->appendFormatted(format, ???);
}

android::sp<String> String::format(const char* format, ...)
{
    return M_String->format(format, ???);
}

android::sp<String> String::appendFormattedWithVarArgList(const char* format, va_list args) const
{
    return M_String->appendFormattedWithVarArgList(format, args);
}


}  // namespace mindroid
