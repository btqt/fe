

class MockOBCResponseEventInfo {
 public:
    MOCK_METHOD(void , setTo, (const OBCResponseEventInfo& other));
    MOCK_METHOD(error_t , writeToParcel, (android::Parcel* parcel));
    MOCK_METHOD(error_t , readFromParcel, (const android::Parcel& parcel));
    MOCK_METHOD(void , toString, ());
    MOCK_METHOD(void , toString, (const char* from));

    MOCK_METHOD(uint8_t ,  getErrCode, ());
    MOCK_METHOD(OBCUDSResInfo ,  getResInfo, ());

    MOCK_METHOD(void , setDataFormat,(
        OBCResponseEventInfoDataFormat& mDf
        , uint8_t mErrCode
        , OBCUDSResInfo mResInfo
    ));

    MOCK_METHOD(void , setData,(
         uint8_t mErrCode
        , OBCUDSResInfo mResInfo
    ));
};

MockOBCResponseEventInfo* M_OBCResponseEventInfo;


#if 0
// do not need constructor
OBCResponseEventInfo::OBCResponseEventInfo(const OBCResponseEventInfo& other)
{
    M_OBCResponseEventInfo->OBCResponseEventInfo(other);
}

OBCResponseEventInfo::~OBCResponseEventInfo()
{
}
#endif

void OBCResponseEventInfo::setTo(const OBCResponseEventInfo& other)
{
    M_OBCResponseEventInfo->setTo(other);
    return ;
}

error_t OBCResponseEventInfo::writeToParcel(android::Parcel* parcel)
{
    return M_OBCResponseEventInfo->writeToParcel(parcel);
}

error_t OBCResponseEventInfo::readFromParcel(const android::Parcel& parcel)
{
    return M_OBCResponseEventInfo->readFromParcel(parcel);
}

void OBCResponseEventInfo::toString()
{
}

void OBCResponseEventInfo::toString(const char* from)
{
}

uint8_t  OBCResponseEventInfo::getErrCode()
{
    return M_OBCResponseEventInfo->getErrCode();
}
OBCUDSResInfo  OBCResponseEventInfo::getResInfo()
{
    return M_OBCResponseEventInfo->getResInfo();
}

void OBCResponseEventInfo::setDataFormat(
        OBCResponseEventInfoDataFormat& mDf
        , uint8_t mErrCode
        , OBCUDSResInfo mResInfo
    )
{
    M_OBCResponseEventInfo->setDataFormat(
        mDf
        , mErrCode
        , mResInfo
    );
    return ;
}

void OBCResponseEventInfo::setData(
     uint8_t mErrCode
    , OBCUDSResInfo mResInfo
)
{
    M_OBCResponseEventInfo->setData(
         mErrCode
        , mResInfo
    );

    return ;
}
