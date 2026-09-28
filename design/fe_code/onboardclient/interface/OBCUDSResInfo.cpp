
/**
 * @attention Copyright (c) 2015 by LG electronics co, Ltd. All rights reserved.
 *   This program or software including the accompanying associated documentation ("Software") is
 *   the proprietary software of LG Electronics Inc.  and or its licensors, and may only be used,
 *   duplicated, modified or distributed pursuant to the terms and conditions of a separate written license agreement
 *   between you and LG Electronics Inc. ("Authorized License").
 *   Except as set forth in an Authorized License, LG Electronics Inc. grants no license (express or implied), rights to use,
 *   or waiver of any kind with respect to the Software, and LG Electronics Inc. expressly reserves all rights in
 *   and to the Software and all intellectual property therein.
 *   If you have no Authorized License, then you have no rights to use the Software in any ways,
 *   and should immediately notify LG Electronics Inc. and discontinue all use of the Software.
 *
 * @author  DCV-Toy24DCM-RS@lge.com
 * @date    2020.12.11
 * @version 3.0.00
 */

/**
 *  This is the OBCUDSResInfo class.
 *
 */

#include "Log.h"
#include <cstdint>
#include "./include/services/OnboardclientManagerService/OBCUDSResInfo.h"
// #include "./include/services/OnboardclientManagerService/OnboardclientCommand.h"

/**
 * @MISRA{MISRA C++-2008 Rule 16-2-1,"This is intended design"}
 */
#define LOG_TAG "IOBCUDSResInfo"
// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

OBCUDSResInfo::OBCUDSResInfo(const OBCUDSResInfo &other)
{
    init();
    setTo(other);
}

OBCUDSResInfo::OBCUDSResInfo()
{
    init();
}

void OBCUDSResInfo::init()
{
    mProtocolType = 0U;
    // 1: protocolType = static_cast< uint8_t >();

    mCanInfo = new OBCCanInfo();
    // 1: canInfo = static_cast< OBCCanInfo >();

    mConnectId = 0U;
    // 1: connectId = static_cast< uint16_t >();

    mResponseType = 0U;
    // 1: responseType = static_cast< uint8_t >();

    mUdsData = new Buffer();
    // 1: udsData = static_cast< android::sp<Buffer> >();
}

OBCUDSResInfo &OBCUDSResInfo::operator=(const OBCUDSResInfo &other)
{
    setTo(other);
    return *this;
}

void OBCUDSResInfo::setData(const uint8_t protocolType, const android::sp<OBCCanInfo> canInfo, const uint16_t connectId, const uint8_t responseType, const android::sp<Buffer> udsData)
{

    this->mProtocolType = protocolType;
    this->mCanInfo->setData(canInfo->canId(), canInfo->nTa());
    this->mConnectId = connectId;
    this->mResponseType = responseType;
    const uint32_t tempVal{udsData->size()};
    if (tempVal <= static_cast<uint32_t>(INT32_MAX))
    {
        this->mUdsData->setTo(udsData->data(), static_cast<int32_t>(tempVal));
    }
    else
    {
        LOGV("Out of range");
    }
}

void OBCUDSResInfo::setTo(const OBCUDSResInfo &other)
{
    setData(
        other.mProtocolType, other.mCanInfo, other.mConnectId, other.mResponseType, other.mUdsData);

    return;
}

error_t OBCUDSResInfo::writeToParcel(android::Parcel *const parcel) const
{
    // LOGV("writeToParcel start");
    LOGI("writeToParcel 1 protocolType = %d", this->mProtocolType);
    (void)parcel->writeUint32(static_cast<uint32_t>(this->mProtocolType));
    (void)this->mCanInfo->writeToParcel(parcel);
    LOGI("writeToParcel 3 connectId = %d", this->mConnectId);
    (void)parcel->writeUint32(static_cast<uint32_t>(this->mConnectId));
    LOGI("writeToParcel 4 responseType = %d", this->mResponseType);
    (void)parcel->writeUint32(static_cast<uint32_t>(this->mResponseType));
    LOGI("writeToParcel 5 udsData = %d", this->mUdsData->size());
    (void)parcel->writeByteArray(this->mUdsData->size(), this->mUdsData->data());
    // LOGV("writeToParcel end");

    return E_OK;
}

error_t OBCUDSResInfo::readFromParcel(const android::Parcel &parcel)
{
    const uint32_t tempProtocolType{parcel.readUint32()};
    if (tempProtocolType <= static_cast<uint32_t>(UINT8_MAX))
    {
        this->mProtocolType = static_cast<uint8_t>(tempProtocolType);
    }
    else
    {
        LOGV("Out of range");
    }
    (void)this->mCanInfo->readFromParcel(parcel);
    const uint32_t tempConnId{parcel.readUint32()};
    if (tempConnId <= static_cast<uint32_t>(UINT16_MAX))
    {
        this->mConnectId = static_cast<uint16_t>(tempConnId);
    }
    else
    {
        LOGV("Out of range");
    }
    const uint32_t tempRes{parcel.readUint32()};
    if (tempRes <= static_cast<uint32_t>(UINT8_MAX))
    {
        this->mResponseType = static_cast<uint8_t>(tempRes);
    }
    else
    {
        LOGV("Out of range");
    }
    const int32_t dataSize{parcel.readInt32()};
    LOGI("Size of UDS data %d", dataSize);
    if (dataSize > 0)
    {
        const uint8_t *const udsData{static_cast<const uint8_t *>(parcel.readInplace(static_cast<size_t>(dataSize)))};
        if (udsData != nullptr)
        {
            this->mUdsData->setTo(udsData, dataSize);
        }
        else
        {
            LOGV("udsData is nullptr");
        }
#if 1 // for debugging : LOGV
        toString("readFromParcel OBCUDSResInfo : ");
#endif
    }
    return E_OK;
}

uint8_t OBCUDSResInfo::getProtocolType() const noexcept
{
    return this->mProtocolType;
}

android::sp<OBCCanInfo> OBCUDSResInfo::getCanInfo() const noexcept
{
    return this->mCanInfo;
}

uint16_t OBCUDSResInfo::getConnectId() const noexcept
{
    return this->mConnectId;
}

uint8_t OBCUDSResInfo::getResponseType() const noexcept
{
    return this->mResponseType;
}

android::sp<Buffer> OBCUDSResInfo::getUdsData() const noexcept
{
    return this->mUdsData;
}

void OBCUDSResInfo::toString(const char_t *const from) const
{
    LOGV("===toString");
    LOGV("%s : OBCUDSResInfo-> protocolType = %u", from, this->mProtocolType);
    // LOGV("%s : OBCUDSResInfo-> canInfo = ", from, this->canInfo);
    LOGV("%s : OBCUDSResInfo-> connectId = %u", from, this->mConnectId);
    LOGV("%s : OBCUDSResInfo-> responseType = %u", from, this->mResponseType);
    LOGV("%s : OBCUDSResInfo-> udsData size = %lu", from, this->mUdsData->size());
}
