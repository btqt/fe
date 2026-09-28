#include <Typedef.h>
#include <utils/Timer.h>
#include <utils/Buffer.h>
#include "utils/Logger.h"
#include "utils/CommonUtils.h"
#include "ParamsDef.h"
#include "OtaMessage.h"

namespace rdgapp {

::TIGER_ERR OtaMessage::Parser(const android::sp<::Buffer> rawData)
{
    const bool isEmply {rawData->empty()};
    ::TIGER_ERR error {E_ERROR};
    if (isEmply == true)
    {
        LOG_E("OTA request data is empty");
        error = E_INPUT_EMPTY;
    } 
    else 
    {
        const uint32_t rawDataSize {rawData->size()};
        if (rawDataSize < OtaMessageDefs::FA_PROTOCOL_HEADER_LENGHT)
        {
            LOG_E("OTA request data not enough (size < 8 bytes)");
            error = E_DATA_CORRUPTED;
        } else {
            (void)std::memcpy(&mFaProtoVer, rawData->data() + OtaMessageDefs::FA_PROTO_VERSION_BYTE_MASK, 1U);
            if (mFaProtoVer > 1U)
            {
                LOG_E("FA Protocol Version: %d is not supported", mFaProtoVer);
                error = E_INVALID_PARAM;
            } else {
                (void)std::memcpy(&mMid, rawData->data() + OtaMessageDefs::MID_BYTE_MASK, 1U);

                if ((mMid >= static_cast<uint8_t>(OTA_MID::OTA_MID_MAX)) || (mMid < static_cast<uint8_t>(OTA_MID::OTA_MID_1_GET_OBC_RESOURCE_REQ)))
                {
                    LOG_E("OTA Message ID is not supported");
                    error = E_INVALID_PARAM;
                } else {
                    const uint8_t payloadSizeHigh {*(rawData->data() + OtaMessageDefs::PAYLOAD_SIZE_BYTE_MASK)};
                    const uint8_t payloadSizeMid {*(rawData->data() + OtaMessageDefs::PAYLOAD_SIZE_BYTE_MASK + 1U)};
                    const uint8_t payloadSizeLow {*(rawData->data() + OtaMessageDefs::PAYLOAD_SIZE_BYTE_MASK + 2U)};

                    mPayloadSize |= static_cast<uint32_t>(payloadSizeHigh) << 16U;
                    mPayloadSize |= static_cast<uint32_t>(payloadSizeMid) << 8U;
                    mPayloadSize |= static_cast<uint32_t>(payloadSizeLow);

                    const uint32_t actualPayloadsize {rawData->size() - OtaMessageDefs::FA_PROTOCOL_HEADER_LENGHT};

                    if (mPayloadSize > actualPayloadsize)
                    {
                        LOG_E("OTA Request is Rejected because payloadSize = %d is not match to actual payload size = %d", mPayloadSize, actualPayloadsize);
                        error = E_REJECTED;
                    } else {

                        mSequenceNumber = CommonUtils::makeSerializeUint16(rawData, OtaMessageDefs::SEQUENCE_NUMBER_BYTE_MASK);


                        if (mPayloadSize <= static_cast<uint32_t>(INT32_MAX)) {
                            mPayload->setTo((rawData->data() + OtaMessageDefs::PAYLOAD_BYTE_MASK), static_cast<int32_t>(mPayloadSize));
                            LOG_I("Ota Request parsering success, FA protocol version %d, mMid = %d, sequenceNumber = %d"
                                , mFaProtoVer
                                , mMid
                                , mSequenceNumber);
                            error = E_OK;
                        } else {
                            LOG_E("mPayloadSize out of range INT32");
                            error = E_INVALID_PARAM;
                        }
                    }
                }
            }
        }
    }
    return error;
}

android::sp<::Buffer> OtaMessage::ToRaw(void)
{
    mPayloadSize = mPayload->size();
    LOG_I("ToRaw mMid = %d, mSequenceNumber = %d, mPayloadSize = %d"
        , static_cast<uint8_t>(mMid)
        , mSequenceNumber
        , mPayloadSize);

    uint8_t reservedByte;
    reservedByte = 0x00U;
    const android::sp<::Buffer> rawData {new ::Buffer()};
    const SerializeUint32_t payloadSize_Serialized {mPayloadSize};
    uint8_t payloadSizeHigh {0U};
    payloadSizeHigh = payloadSize_Serialized.data[2];
    uint8_t payloadSizeMid {0U} ;
    payloadSizeMid = payloadSize_Serialized.data[1];
    uint8_t payloadSizeLow {0U};
    payloadSizeLow = payloadSize_Serialized.data[0];

    uint8_t sequenceNumHigh {0U};
    const SerializeUint16_t sequenceNum_Serialized {mSequenceNumber};
    sequenceNumHigh = sequenceNum_Serialized.data[1];
    uint8_t sequenceNumLow {0U};
    sequenceNumLow = sequenceNum_Serialized.data[0];

    rawData->setTo(&mFaProtoVer, OtaMessageDefs::FA_PROTO_VERSION_LENGHT);
    rawData->append(&mMid, OtaMessageDefs::MID_LENGHT);
    rawData->append(&sequenceNumHigh, 1);
    rawData->append(&sequenceNumLow, 1);
    rawData->append(&reservedByte, 1);
    rawData->append(&payloadSizeHigh, 1);
    rawData->append(&payloadSizeMid, 1);
    rawData->append(&payloadSizeLow, 1);

    if (mPayloadSize <= static_cast<uint32_t>(INT32_MAX)) {
        rawData->append(mPayload->data(), static_cast<int32_t>(mPayloadSize));
    } else {
        LOG_E("mPayloadSize out of range INT32");
    }
    return rawData;
}
}
