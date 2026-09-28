class MockSystemProperty {
  public:
    MOCK_METHOD1(getInternal, char*(const char* key));
//     MOCK_METHOD3(setInternal, void(char *, char *, bool ));
//     MOCK_METHOD3(setInternal, void(char *, int32_t , bool ));
//     MOCK_METHOD0(releaseInternal, void());
};

MockSystemProperty * M_SystemProperty;

class MockSLProperty {
  public:
    MOCK_METHOD2(getPropertyRootFileName, bool(char (&fileNameBuf)[MAX_PROP_PATH_LENGTH], const int32_t bufLen));
//     MOCK_METHOD0(initPropertyCache, void());
//     MOCK_METHOD2(setProperty, void(const char* key, const char* c_value));
//     MOCK_METHOD2(setProperty, void(const char* key, const int32_t i_value));
    MOCK_METHOD2(getProperty, char*(char *, bool ));
    MOCK_METHOD2(getPropertyInt, int32_t(char *, bool ));
    MOCK_METHOD0(syncProperties, bool());
//     MOCK_METHOD0(removeUnusedProp, void());
//     MOCK_METHOD0(updateRoProperties, void());
//     MOCK_METHOD0(lockReadonly, void());
//     MOCK_METHOD0(unlockReadonly, void());
    MOCK_METHOD2(getPropertyInternal, char*(char *, bool ));
//     MOCK_METHOD2(setPropertyInternal, void(const char* key, const char* c_value));
    MOCK_METHOD1(findPropertyfromUsrProp, int32_t(const char* key));
//     MOCK_METHOD0(updateFinished, void());
//     MOCK_METHOD0(syncFinished, void());
//     MOCK_METHOD0(initCacheInternal, void());
//     MOCK_METHOD0(updateCacheInternal, void());
//     MOCK_METHOD0(initPropertyInternal, void());
//     MOCK_METHOD0(loadProperty, void());
    MOCK_METHOD1(findProperty, int32_t(const char* name));
    MOCK_METHOD2(getPropertyMMAP, char*(const int32_t id, const char* name));
    MOCK_METHOD3(setPropertyMMAP, bool(const int32_t id, const char* name, const char* c_value));
//     MOCK_METHOD1(readInFile, void(const char* filename));
//     MOCK_METHOD2(parseLine, void(int32_t id, const char* c_buffer));
    MOCK_METHOD3(findKeyInFile, bool(const char* fileName, const char* key, char* outValue));
//     MOCK_METHOD0(DISALLOW_COPY_ASSIGN_CONSTRUCTORS, void(SLProperty));
};

MockSLProperty * M_SLProperty;

SystemProperty::SystemProperty()
{

}

SystemProperty::~SystemProperty()
{

}

char* SystemProperty::getInternal(const char* key)
{
    return M_SystemProperty->getInternal(key);
}

void SystemProperty::setInternal(char *key, char *c_val, bool force_add)
{
//    M_SystemProperty->setInternal(key, c_val, force_add);
}

void SystemProperty::setInternal(char *key, int32_t i_val, bool force_add)
{
//    M_SystemProperty->setInternal(key, i_val, force_add);
}

void SystemProperty::releaseInternal()
{
//    M_SystemProperty->releaseInternal();
}


SLProperty::SLProperty(int32_t id, bool realtimeSync)
{

}

SLProperty::SLProperty(const char* name, bool realtimeSync)
{

}

SLProperty::~SLProperty()
{

}

bool SLProperty::getPropertyRootFileName(char (&fileNameBuf)[MAX_PROP_PATH_LENGTH], const int32_t bufLen)
{
    return M_SLProperty->getPropertyRootFileName(MAX_PROP_PATH_LENGTH, bufLen);
}

void SLProperty::initPropertyCache()
{
//    M_SLProperty->initPropertyCache();
}

void SLProperty::setProperty(const char* key, const char* c_value)
{
//    M_SLProperty->setProperty(key, c_value);
}

void SLProperty::setProperty(const char* key, const int32_t i_value)
{
//    M_SLProperty->setProperty(key, i_value);
}

char* SLProperty::getProperty(char *key, bool searchUsrProp)
{
    return M_SLProperty->getProperty(key, searchUsrProp);
}

int32_t SLProperty::getPropertyInt(char *key, bool searchUsrProp)
{
    return M_SLProperty->getPropertyInt(key, searchUsrProp);
}

bool SLProperty::syncProperties()
{
    return M_SLProperty->syncProperties();
}

void SLProperty::removeUnusedProp()
{
//    M_SLProperty->removeUnusedProp();
}

void SLProperty::updateRoProperties()
{
//    M_SLProperty->updateRoProperties();
}

void SLProperty::lockReadonly()
{
//    M_SLProperty->lockReadonly();
}

void SLProperty::unlockReadonly()
{
//    M_SLProperty->unlockReadonly();
}

char* SLProperty::getPropertyInternal(char *key, bool searchUsrProp)
{
    return M_SLProperty->getPropertyInternal(key, searchUsrProp);
}

void SLProperty::setPropertyInternal(const char* key, const char* c_value)
{
//    M_SLProperty->setPropertyInternal(key, c_value);
}

int32_t SLProperty::findPropertyfromUsrProp(const char* key)
{
    return M_SLProperty->findPropertyfromUsrProp(key);
}

void SLProperty::updateFinished()
{
//    M_SLProperty->updateFinished();
}

void SLProperty::syncFinished()
{
//    M_SLProperty->syncFinished();
}

void SLProperty::initCacheInternal()
{
//    M_SLProperty->initCacheInternal();
}

void SLProperty::updateCacheInternal()
{
//    M_SLProperty->updateCacheInternal();
}

void SLProperty::initPropertyInternal()
{
//    M_SLProperty->initPropertyInternal();
}

void SLProperty::loadProperty()
{
//    M_SLProperty->loadProperty();
}

int32_t SLProperty::findProperty(const char* name)
{
    return M_SLProperty->findProperty(name);
}

char* SLProperty::getPropertyMMAP(const int32_t id, const char* name)
{
    return M_SLProperty->getPropertyMMAP(id, name);
}

bool SLProperty::setPropertyMMAP(const int32_t id, const char* name, const char* c_value)
{
    return M_SLProperty->setPropertyMMAP(id, name, c_value);
}

void SLProperty::readInFile(const char* filename)
{
//    M_SLProperty->readInFile(filename);
}

void SLProperty::parseLine(int32_t id, const char* c_buffer)
{
//    M_SLProperty->parseLine(id, c_buffer);
}

bool SLProperty::findKeyInFile(const char* fileName, const char* key, char* outValue)
{
    return M_SLProperty->findKeyInFile(fileName, key, outValue);
}

void SLProperty::DISALLOW_COPY_ASSIGN_CONSTRUCTORS(SLProperty)
{
//    M_SLProperty->DISALLOW_COPY_ASSIGN_CONSTRUCTORS(SLProperty);
}
