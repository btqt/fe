

class MockOBCTransportInfo {
 public:
    MOCK_METHOD(void , setTo, (const OBCTransportInfo& other));
    MOCK_METHOD(error_t , writeToParcel, (android::Parcel* parcel));
    MOCK_METHOD(error_t , readFromParcel, (const android::Parcel& parcel));
    MOCK_METHOD(void , toString, ());
    MOCK_METHOD(void , toString, (const char* from));

    MOCK_METHOD(uint8_t ,  getProtocolType, ());
    MOCK_METHOD(OBCCanInfo ,  getCanInfo, ());
    MOCK_METHOD(uint16_t ,  getUdsResTimeout, ());

    MOCK_METHOD(void , setDataFormat,(
        OBCTransportInfoDataFormat& mDf
        , uint8_t mProtocolType
        , OBCCanInfo mCanInfo
        , uint16_t mUdsResTimeout
    ));

    MOCK_METHOD(void , setData,(
         uint8_t mProtocolType
        , OBCCanInfo mCanInfo
        , uint16_t mUdsResTimeout
    ));
};

MockOBCTransportInfo* M_OBCTransportInfo;


#if 0
// do not need constructor
OBCTransportInfo::OBCTransportInfo(const OBCTransportInfo& other)
{
    M_OBCTransportInfo->OBCTransportInfo(other);
}

OBCTransportInfo::~OBCTransportInfo()
{
}
#endif

void OBCTransportInfo::setTo(const OBCTransportInfo& other)
{
    M_OBCTransportInfo->setTo(other);
    return ;
}

error_t OBCTransportInfo::writeToParcel(android::Parcel* parcel)
{
    return M_OBCTransportInfo->writeToParcel(parcel);
}

error_t OBCTransportInfo::readFromParcel(const android::Parcel& parcel)
{
    return M_OBCTransportInfo->readFromParcel(parcel);
}

void OBCTransportInfo::toString()
{
}

void OBCTransportInfo::toString(const char* from)
{
}

uint8_t  OBCTransportInfo::getProtocolType()
{
    return M_OBCTransportInfo->getProtocolType();
}
OBCCanInfo  OBCTransportInfo::getCanInfo()
{
    return M_OBCTransportInfo->getCanInfo();
}
uint16_t  OBCTransportInfo::getUdsResTimeout()
{
    return M_OBCTransportInfo->getUdsResTimeout();
}

void OBCTransportInfo::setDataFormat(
        OBCTransportInfoDataFormat& mDf
        , uint8_t mProtocolType
        , OBCCanInfo mCanInfo
        , uint16_t mUdsResTimeout
    )
{
    M_OBCTransportInfo->setDataFormat(
        mDf
        , mProtocolType
        , mCanInfo
        , mUdsResTimeout
    );
    return ;
}

void OBCTransportInfo::setData(
     uint8_t mProtocolType
    , OBCCanInfo mCanInfo
    , uint16_t mUdsResTimeout
)
{
    M_OBCTransportInfo->setData(
         mProtocolType
        , mCanInfo
        , mUdsResTimeout
    );

    return ;
}
