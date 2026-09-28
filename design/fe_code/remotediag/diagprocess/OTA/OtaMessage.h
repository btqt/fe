#pragma once
#include <string>
#include <binder/Parcel.h>
#include <utils/Buffer.h>

#include "OtaMessageDefine.h"

namespace rdgapp {

class OtaMessage : public android::RefBase 
{
public:

    ~OtaMessage() final = default;

    OtaMessage(const OTA_MID aMID, const uint8_t aFaProtoVer = 1U)
        : android::RefBase()
        , mMid(static_cast<uint8_t>(aMID))
        , mFaProtoVer(aFaProtoVer)
        , mSequenceNumber(0U)
        , mPayloadSize(0U)
        , mPayload(new ::Buffer()) {}
        
    explicit OtaMessage() noexcept
        : OtaMessage(OTA_MID::OTA_MID_UNKNOW, 0U) {}

    ::TIGER_ERR Parser(const android::sp<::Buffer> rawData);
    android::sp<::Buffer> ToRaw(void);
    inline android::sp<::Buffer> Payload(void) const noexcept {return mPayload;};
    inline OTA_MID Mid() const noexcept {
        return ((mMid < static_cast<uint8_t>(OTA_MID::OTA_MID_MAX)) && (mMid >= static_cast<uint8_t>(OTA_MID::OTA_MID_1_GET_OBC_RESOURCE_REQ)))
                ? static_cast<OTA_MID>(mMid) : OTA_MID::OTA_MID_UNKNOW;
    }

    inline void SetSequenceNumber(const uint16_t sequenceNumber) noexcept {mSequenceNumber = sequenceNumber;};

    inline uint8_t FaProtoVer() const noexcept {return mFaProtoVer;};
    inline uint32_t PayloadSize() const noexcept {return mPayloadSize;};
    inline uint16_t SequenceNumber() const noexcept {return mSequenceNumber;};
private:
    uint8_t mMid;
    uint8_t mFaProtoVer;                    
    uint16_t mSequenceNumber;                          
    uint32_t mPayloadSize;                                                      
    android::sp<Buffer> mPayload;
};
} // namespace rdgapp
