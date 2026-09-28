#include <utils/Timer.h>
#include <utils/Buffer.h>
#include "utils/Logger.h"
#include "UdsMessage.h"

namespace rdgapp {

UdsMessage::UdsMessage(const android::sp<UdsMessage> udsResponse):android::RefBase() {
    mOptionData = new ::Buffer();
    mUdsPayload = new ::Buffer();
    if(udsResponse != nullptr) {
        mSID = udsResponse->mSID;
        mSFID = udsResponse->mSFID;
        mNRC = udsResponse->mNRC;
        mDtcStatusMask = udsResponse->mDtcStatusMask;
        mMemorySelection = udsResponse->mMemorySelection;
        uint32_t udsResSize{udsResponse->GetUdsPayload()->size()};
        if(udsResSize > static_cast<uint32_t>(INT32_MAX))
        {  
            udsResSize = static_cast<uint32_t>(INT32_MAX);
        }
        mUdsPayload->setTo(udsResponse->GetUdsPayload()->data(), static_cast<int32_t>(udsResSize));
    } else {
        LOG_D("Constructor UdsMessage from nullptr");
        mSID = 0U;
        mSFID = 0U;
        mNRC = 0U;
        mDtcStatusMask = 0U;
        mMemorySelection = 0U;
    }

    if (mDtcStatusMask != 0U) {
        mOptionData->append(&mDtcStatusMask, 1);
    }
    if (mMemorySelection != 0U) {
        mOptionData->append(&mMemorySelection, 1);
    }
}

error_t UdsMessage::Parser(const android::sp<::Buffer> &udsData)
{
    error_t error{TIGER_ERR::E_ERROR};

    if (udsData->empty())
    {
        error = TIGER_ERR::E_ERROR;
    }
    else
    {
        mSID = udsData->data()[SID_BYTE_MASK];
        if (udsData->size() > 1U)
        {
            mSFID = udsData->data()[SFID_BYTE_MASK];
        }

        if (mSID == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE))
        {
            mNRC = udsData->data()[NRC_BYTE_MASK];
        }
        if (udsData->size() > UDS_MESSAGE_HEADER_LENGHT)
        {
            uint32_t udsDataSize{udsData->size() - UDS_MESSAGE_HEADER_LENGHT};
            if(udsDataSize > static_cast<uint32_t>(INT32_MAX))
            {
                
                udsDataSize = static_cast<uint32_t>(INT32_MAX);
            }
            mUdsPayload->setTo(&udsData->data()[UDS_DATA_BYTE_MASK], static_cast<int32_t>(udsDataSize));
        }
        LOG_D("parser success, mSID = 0x%02x, mSFID = 0x%02x", mSID, mSFID);
        if (mUdsData == nullptr)
        {
            mUdsData = new ::Buffer();
        } else {
            mUdsData.clear();
        }
        mUdsData->append(&mSID, 1);
        mUdsData->append(&mSFID, 1);
        uint32_t optionDataSize{mOptionData->size()};
        if(optionDataSize > static_cast<uint32_t>(INT32_MAX))
        {
            optionDataSize = static_cast<uint32_t>(INT32_MAX);
        }
        mUdsData->append(mOptionData->data(), static_cast<int32_t>(optionDataSize));
        (void)optionDataSize;
        uint32_t udsPayloadSize{mUdsPayload->size()};
        if(udsPayloadSize > static_cast<uint32_t>(INT32_MAX))
        {
            udsPayloadSize = static_cast<uint32_t>(INT32_MAX);
        }
        mUdsData->append(mUdsPayload->data(), static_cast<int32_t>(udsPayloadSize));

        error = TIGER_ERR::E_OK;
    }
    return error;
}

error_t UdsMessage::Parser(const std::vector<uint8_t> &udsData)
{
    error_t error{TIGER_ERR::E_ERROR};

    if ((udsData.empty()) || (udsData.size() < UDS_MESSAGE_HEADER_LENGHT))
    {
        error = TIGER_ERR::E_ERROR;
    }
    else
    {
        mSID = udsData[SID_BYTE_MASK];
        mSFID = udsData[SFID_BYTE_MASK];

        if (mSID == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE))
        {
            mNRC = udsData[NRC_BYTE_MASK];
        }
        else
        {
            uint32_t udsDataSize{udsData.size() - UDS_MESSAGE_HEADER_LENGHT};
            if(udsDataSize > static_cast<uint32_t>(INT32_MAX))
            {
                
                udsDataSize = static_cast<uint32_t>(INT32_MAX);
            }
            mUdsPayload->setTo(&udsData[UDS_DATA_BYTE_MASK], static_cast<int32_t>(udsDataSize));
        }
        LOG_D("parser success, sid = 0x%02x, sfid = 0x%02x", mSID, mSFID);

        mUdsData->append(&mSID, 1);
        mUdsData->append(&mSFID, 1);
        uint32_t optionDataSize{mOptionData->size()};
        if(optionDataSize > static_cast<uint32_t>(INT32_MAX))
        {
            optionDataSize = static_cast<uint32_t>(INT32_MAX);
        }
        mUdsData->append(mOptionData->data(), static_cast<int32_t>(optionDataSize));
        (void)optionDataSize;
        uint32_t udsPayloadSize{mUdsPayload->size()};
        if(udsPayloadSize > static_cast<uint32_t>(INT32_MAX))
        {
            udsPayloadSize = static_cast<uint32_t>(INT32_MAX);
        }
        mUdsData->append(mUdsPayload->data(), static_cast<int32_t>(udsPayloadSize));

        error = TIGER_ERR::E_OK;
    }
    return error;
}

android::sp<::Buffer> UdsMessage::ToUdsData(void)
{
    if (this->mUdsData == nullptr) {
        LOG_D("Make uds data mSID = 0x%02x, mSFID = 0x%02x", mSID, mSFID);
        this->mUdsData = new ::Buffer();
        this->mUdsData->append(&mSID, 1);
        this->mUdsData->append(&mSFID, 1);
        uint32_t optionDataSize{mOptionData->size()};
        if(optionDataSize > static_cast<uint32_t>(INT32_MAX))
        {
            optionDataSize = static_cast<uint32_t>(INT32_MAX);
        }
        this->mUdsData->append(mOptionData->data(), static_cast<int32_t>(optionDataSize));
        (void)optionDataSize;
        uint32_t udsPayloadSize{mUdsPayload->size()};
        if(udsPayloadSize > static_cast<uint32_t>(INT32_MAX))
        {
            udsPayloadSize = static_cast<uint32_t>(INT32_MAX);
        }
        this->mUdsData->append(mUdsPayload->data(), static_cast<int32_t>(udsPayloadSize));
    }

    return this->mUdsData;
}

android::sp<::Buffer> UdsMessage::GetUdsPayload(void) const noexcept
{
    return this->mUdsPayload;
}

void UdsMessage::setSID(const uint8_t aSID) noexcept
{
    this->mSID = aSID;
}

uint8_t UdsMessage::getSID() const noexcept
{
    return this->mSID;
}

void UdsMessage::setSFID(const uint8_t aSFID) noexcept
{
    this->mSFID = aSFID;
}

uint8_t UdsMessage::getSFID() const noexcept
{
    return this->mSFID;
}

uint8_t UdsMessage::getNRC() const noexcept
{
    return this->mNRC;
}

android::sp<::Buffer> UdsMessage::getOptionData() const noexcept
{
    return this->mOptionData;
}

}
