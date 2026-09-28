
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
 *  This is the OBCTransportInfo class.
 *
 */

#include "Log.h"
#include <cstdint>

#include "./include/services/OnboardclientManagerService/OBCTransportInfo.h"
// #include "./include/services/OnboardclientManagerService/OnboardclientCommand.h"

/**
 * @MISRA{MISRA C++-2008 Rule 16-2-1,"This is intended design"}
 */
#define LOG_TAG "IOBCTransportInfo"
// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

OBCTransportInfo::OBCTransportInfo(const OBCTransportInfo &other)
{
    init();
    setTo(other);
}

OBCTransportInfo::OBCTransportInfo()
{
    init();
}

void OBCTransportInfo::init() noexcept
{
    mProtocolType = 0U;
    // 1: protocolType = static_cast< uint8_t >();

    // canInfo = new OBCCanInfo();
    // 1: canInfo = static_cast< OBCCanInfo >();
    mPeriodicRes = false;
    mUdsResTimeout = 0U;
    // 1: udsResTimeout = static_cast< uint16_t >();
}

OBCTransportInfo &OBCTransportInfo::operator=(const OBCTransportInfo &other)
{
    setTo(other);
    return *this;
}

void OBCTransportInfo::setData(const uint8_t mprotocolType, const OBCCanInfo mcanInfo, const bool mperiodicRes, const uint16_t mudsResTimeout)
{

    this->mProtocolType = mprotocolType;
    this->mCanInfo = mcanInfo;
    this->mPeriodicRes = mperiodicRes;
    this->mUdsResTimeout = mudsResTimeout;
}

void OBCTransportInfo::setTo(const OBCTransportInfo &other)
{
    setData(
        other.mProtocolType, other.mCanInfo, other.mPeriodicRes, other.mUdsResTimeout);

    return;
}

error_t OBCTransportInfo::writeToParcel(android::Parcel *const parcel) const
{
    // LOGV("writeToParcel start");
    LOGV("writeToParcel 1 protocolType = %d", this->mProtocolType);
    (void)parcel->writeUint32(static_cast<uint32_t>(this->mProtocolType));
    (void)this->mCanInfo.writeToParcel(parcel);
    LOGV("writeToParcel 3 udsResTimeout = %d", this->mUdsResTimeout);
    (void)parcel->writeUint32(static_cast<uint32_t>(this->mUdsResTimeout));

    return E_OK;
}

error_t OBCTransportInfo::readFromParcel(const android::Parcel &parcel)
{
#if 0 // for debugging : LOGV
    toString("readFromParcel OBCTransportInfo 1: ");
#endif

    const uint32_t tempProtocolType{parcel.readUint32()};
    if (tempProtocolType <= static_cast<uint32_t>(UINT8_MAX))
    {
        this->mProtocolType = static_cast<uint8_t>(tempProtocolType);
    }
    else
    {
        LOGV("Out of range");
    }
    (void)this->canInfo().readFromParcel(parcel);
    const uint32_t tempUdsResTimeout{parcel.readUint32()};
    if (tempUdsResTimeout <= static_cast<uint32_t>(UINT16_MAX))
    {
        this->mUdsResTimeout = static_cast<uint16_t>(tempUdsResTimeout);
    }
    else
    {
        LOGV("Out of range");
    }

#if 1 // for debugging : LOGV
    toString("readFromParcel OBCTransportInfo : ");
#endif

    return E_OK;
}

void OBCTransportInfo::toString(const char_t *const from) const
{
    LOGV("===toString");
    LOGV("%s : OBCTransportInfo-> protocolType = %d", from, this->mProtocolType);
    // LOGV("%s : OBCTransportInfo-> canInfo = ", from, this->canInfo);
    LOGV("%s : OBCTransportInfo-> udsResTimeout = %d", from, this->mUdsResTimeout);
    // LOGV("===2");
}

uint16_t OBCTransportInfo::getUdsResTimeout() const
{
    return this->mUdsResTimeout;
}

uint8_t OBCTransportInfo::getProtocolType() const
{
    return this->mProtocolType;
}

OBCCanInfo OBCTransportInfo::getCanInfo() const
{
    return this->mCanInfo;
}

bool OBCTransportInfo::getPeriodicRes() const
{
    return this->mPeriodicRes;
}
