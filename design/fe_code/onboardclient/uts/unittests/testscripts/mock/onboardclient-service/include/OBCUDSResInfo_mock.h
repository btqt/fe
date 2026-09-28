

class MockOBCUDSResInfo {
 public:
    MOCK_METHOD(void , setTo, (const OBCUDSResInfo& other));
    MOCK_METHOD(error_t , writeToParcel, (android::Parcel* parcel));
    MOCK_METHOD(error_t , readFromParcel, (const android::Parcel& parcel));
    MOCK_METHOD(void , toString, ());
    MOCK_METHOD(void , toString, (const char* from));

    MOCK_METHOD(uint8_t ,  getProtocolType, ());
    MOCK_METHOD(OBCCanInfo ,  getCanInfo, ());
    MOCK_METHOD(uint16_t ,  getConnectId, ());
    MOCK_METHOD(uint8_t ,  getResponseType, ());
    MOCK_METHOD(android::sp<Buffer> ,  getUdsData, ());

    MOCK_METHOD(void , setDataFormat,(
        OBCUDSResInfoDataFormat& mDf
        , uint8_t mProtocolType
        , OBCCanInfo mCanInfo
        , uint16_t mConnectId
        , uint8_t mResponseType
        , android::sp<Buffer> mUdsData
    ));

    MOCK_METHOD(void , setData,(
         uint8_t mProtocolType
        , OBCCanInfo mCanInfo
        , uint16_t mConnectId
        , uint8_t mResponseType
        , android::sp<Buffer> mUdsData
    ));
};

MockOBCUDSResInfo* M_OBCUDSResInfo;


#if 0
// do not need constructor
OBCUDSResInfo::OBCUDSResInfo(const OBCUDSResInfo& other)
{
    M_OBCUDSResInfo->OBCUDSResInfo(other);
}

OBCUDSResInfo::~OBCUDSResInfo()
{
}
#endif

void OBCUDSResInfo::setTo(const OBCUDSResInfo& other)
{
    M_OBCUDSResInfo->setTo(other);
    return ;
}

error_t OBCUDSResInfo::writeToParcel(android::Parcel* parcel)
{
    return M_OBCUDSResInfo->writeToParcel(parcel);
}

error_t OBCUDSResInfo::readFromParcel(const android::Parcel& parcel)
{
    return M_OBCUDSResInfo->readFromParcel(parcel);
}

void OBCUDSResInfo::toString()
{
}

void OBCUDSResInfo::toString(const char* from)
{
}

uint8_t  OBCUDSResInfo::getProtocolType()
{
    return M_OBCUDSResInfo->getProtocolType();
}
OBCCanInfo  OBCUDSResInfo::getCanInfo()
{
    return M_OBCUDSResInfo->getCanInfo();
}
uint16_t  OBCUDSResInfo::getConnectId()
{
    return M_OBCUDSResInfo->getConnectId();
}
uint8_t  OBCUDSResInfo::getResponseType()
{
    return M_OBCUDSResInfo->getResponseType();
}
android::sp<Buffer>  OBCUDSResInfo::getUdsData()
{
    return M_OBCUDSResInfo->getUdsData();
}

void OBCUDSResInfo::setDataFormat(
        OBCUDSResInfoDataFormat& mDf
        , uint8_t mProtocolType
        , OBCCanInfo mCanInfo
        , uint16_t mConnectId
        , uint8_t mResponseType
        , android::sp<Buffer> mUdsData
    )
{
    M_OBCUDSResInfo->setDataFormat(
        mDf
        , mProtocolType
        , mCanInfo
        , mConnectId
        , mResponseType
        , mUdsData
    );
    return ;
}

void OBCUDSResInfo::setData(
     uint8_t mProtocolType
    , OBCCanInfo mCanInfo
    , uint16_t mConnectId
    , uint8_t mResponseType
    , android::sp<Buffer> mUdsData
)
{
    M_OBCUDSResInfo->setData(
         mProtocolType
        , mCanInfo
        , mConnectId
        , mResponseType
        , mUdsData
    );

    return ;
}
