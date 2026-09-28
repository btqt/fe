
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
 *  This is the OBCResponseEventInfo class.
 *
 */

#include "Log.h"
#include "./include/services/OnboardclientManagerService/OBCResponseEventInfo.h"
#include <cstdint>
// #include "./include/services/OnboardclientManagerService/OnboardclientCommand.h"

/**
 * @MISRA{MISRA C++-2008 Rule 16-2-1,"This is intended design"}
 */
#define LOG_TAG "IOBCResponseEventInfo"
// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

OBCResponseEventInfo::OBCResponseEventInfo(const OBCResponseEventInfo &other)
{
    init();
    setTo(other);
}

OBCResponseEventInfo::OBCResponseEventInfo()
{
    init();
}

void OBCResponseEventInfo::init()
{
    mErrCode = 0U;
    // 1: errCode = static_cast< uint8_t >();

    mResInfo = new OBCUDSResInfo();
    // 1: resInfo = static_cast< OBCUDSResInfo >();
}

OBCResponseEventInfo &OBCResponseEventInfo::operator=(const OBCResponseEventInfo &other)
{
    setTo(other);
    return *this;
}

void OBCResponseEventInfo::setData(const uint8_t errCode, const android::sp<OBCUDSResInfo> resInfo)
{

    this->mErrCode = errCode;
    this->mResInfo->setData(resInfo->protocolType(),
                            resInfo->canInfo(),
                            resInfo->connectId(),
                            resInfo->responseType(),
                            resInfo->udsData());
}

void OBCResponseEventInfo::setTo(const OBCResponseEventInfo &other)
{
    setData(
        other.mErrCode, other.mResInfo);

    return;
}

error_t OBCResponseEventInfo::writeToParcel(android::Parcel *const parcel) const
{
    // LOGV("writeToParcel start");
    LOGV("writeToParcel 1 errCode = %d", this->mErrCode);
    (void)parcel->writeUint32(static_cast<uint32_t>(this->mErrCode));
    // LOGV("writeToParcel 2 resInfo = ", this->resInfo);
    (void)this->mResInfo->writeToParcel(parcel);
    // LOGV("writeToParcel end");

    return E_OK;
}

error_t OBCResponseEventInfo::readFromParcel(const android::Parcel &parcel)
{
#if 0 // for debugging : LOGV
    toString("readFromParcel OBCResponseEventInfo 1: ");
#endif

    // LOGV("readFromParcel start");
    // LOGV("readFromParcel 1 errCode");
    //  OBCResponseEventInfo :: 1
    const uint32_t tempErr{parcel.readUint32()};
    if (tempErr <= static_cast<uint32_t>(UINT8_MAX))
    {
        this->mErrCode = static_cast<uint8_t>(tempErr);
    }
    else
    {
        LOGV("Out of range");
    }
    (void)this->mResInfo->readFromParcel(parcel);
    // LOGV("readFromParcel end");

#if 1 // for debugging : LOGV
    toString("readFromParcel OBCResponseEventInfo : ");
#endif

    return E_OK;
}

uint8_t OBCResponseEventInfo::getErrCode() const noexcept
{
    return this->mErrCode;
}

android::sp<OBCUDSResInfo> OBCResponseEventInfo::getResInfo() const noexcept
{
    return this->mResInfo;
}

void OBCResponseEventInfo::toString(const char_t *const from) const
{
    LOGV("===toString");
    LOGV("%s : OBCResponseEventInfo-> errCode = %u", from, this->mErrCode);
    // LOGV("%s : OBCResponseEventInfo-> resInfo = %d ", from, this->resInfo);
    // LOGV("===2");
}
