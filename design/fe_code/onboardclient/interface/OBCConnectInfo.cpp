
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
 * @author  tuyen2.nguyen@lge.com
 * @date    2023.10.16
 * @version 3.0.00
 */

/**
 *  This is the OBCTransportInfo class.
 *
 */

#include "Log.h"
#include <cstdint>
#include "./include/services/OnboardclientManagerService/OBCConnectInfo.h"
// #include "./include/services/OnboardclientManagerService/OnboardclientCommand.h"

/**
 * @MISRA{MISRA C++-2008 Rule 16-2-1,"This is intended design"}
 */
#define LOG_TAG "IOBCConnectInfo"
OBCConnectInfo::OBCConnectInfo() noexcept
{
    response = 0U;
    connectId = 0U;
}

OBCConnectInfo::OBCConnectInfo(const OBCConnectInfo &other) noexcept
{
    response = other.getResponse();
    connectId = other.getConnectId();
}

OBCConnectInfo &OBCConnectInfo::operator=(const OBCConnectInfo &other) noexcept
{
    setTo(other);
    return *this;
}

void OBCConnectInfo::setTo(const OBCConnectInfo &other) noexcept
{
    setData(other.response, other.connectId);
    return;
}

void OBCConnectInfo::setData(const uint8_t mresponse, const uint16_t mconnectId) noexcept
{
    this->response = mresponse;
    this->connectId = mconnectId;
}

error_t OBCConnectInfo::writeToParcel(android::Parcel *const parcel) const
{
    (void)parcel->writeUint32(static_cast<uint32_t>(this->response));
    (void)parcel->writeUint32(static_cast<uint32_t>(this->connectId));
    return E_OK;
}

error_t OBCConnectInfo::readFromParcel(const android::Parcel &parcel)
{
    const uint32_t tempRes{parcel.readUint32()};
    if (tempRes <= static_cast<uint32_t>(UINT8_MAX))
    {
        this->response = static_cast<uint8_t>(tempRes);
    }
    else
    {
        LOGV("Out of range");
    }
    const uint32_t tempConnId{parcel.readUint32()};
    if (tempConnId <= static_cast<uint32_t>(UINT16_MAX))
    {
        this->connectId = static_cast<uint16_t>(tempConnId);
    }
    else
    {
        LOGV("Out of range");
    }
    return E_OK;
}

void OBCConnectInfo::setDataFormat(tOBCConnectInfo &df) noexcept
{
    if (this->response <= OBCEnum::OBCErrCode::OBC_ERR_MAX)
    {
        df.response = static_cast<OBCEnum::OBCErrCode>(this->response);
    }
    df.connectId = this->connectId;
}

uint8_t OBCConnectInfo::getResponse() const noexcept
{
    return this->response;
}
uint16_t OBCConnectInfo::getConnectId() const noexcept
{
    return this->connectId;
}
