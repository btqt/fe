

class MockOBCCanInfo {
 public:
    MOCK_METHOD(void , setTo, (const OBCCanInfo& other));
    MOCK_METHOD(error_t , writeToParcel, (android::Parcel* parcel));
    MOCK_METHOD(error_t , readFromParcel, (const android::Parcel& parcel));
    MOCK_METHOD(void , toString, ());
    MOCK_METHOD(void , toString, (const char* from));

    MOCK_METHOD(uint32_t ,  getCanId, ());
    MOCK_METHOD(android::sp<Buffer> ,  getNTa, ());

    MOCK_METHOD(void , setDataFormat,(
        OBCCanInfoDataFormat& mDf
        , uint32_t mCanId
        , android::sp<Buffer> mNTa
    ));

    MOCK_METHOD(void , setData,(
         uint32_t mCanId
        , android::sp<Buffer> mNTa
    ));
};

MockOBCCanInfo* M_OBCCanInfo;


#if 0
// do not need constructor
OBCCanInfo::OBCCanInfo(const OBCCanInfo& other)
{
    M_OBCCanInfo->OBCCanInfo(other);
}

OBCCanInfo::~OBCCanInfo()
{
}
#endif

void OBCCanInfo::setTo(const OBCCanInfo& other)
{
    M_OBCCanInfo->setTo(other);
    return ;
}

error_t OBCCanInfo::writeToParcel(android::Parcel* parcel)
{
    return M_OBCCanInfo->writeToParcel(parcel);
}

error_t OBCCanInfo::readFromParcel(const android::Parcel& parcel)
{
    return M_OBCCanInfo->readFromParcel(parcel);
}

void OBCCanInfo::toString()
{
}

void OBCCanInfo::toString(const char* from)
{
}

uint32_t  OBCCanInfo::getCanId()
{
    return M_OBCCanInfo->getCanId();
}
android::sp<Buffer>  OBCCanInfo::getNTa()
{
    return M_OBCCanInfo->getNTa();
}

void OBCCanInfo::setDataFormat(
        OBCCanInfoDataFormat& mDf
        , uint32_t mCanId
        , android::sp<Buffer> mNTa
    )
{
    M_OBCCanInfo->setDataFormat(
        mDf
        , mCanId
        , mNTa
    );
    return ;
}

void OBCCanInfo::setData(
     uint32_t mCanId
    , android::sp<Buffer> mNTa
)
{
    M_OBCCanInfo->setData(
         mCanId
        , mNTa
    );

    return ;
}
