
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

#ifndef SERVICELAYER_XXCLASS_OBCCONNECTINFO_H
#define SERVICELAYER_XXCLASS_OBCCONNECTINFO_H

#include "Typedef.h"

#include "utils/Buffer.h"

#include "utils/external/mindroid/lang/String.h"
#include "Log.h"
#include "Error.h"
#include <binder/Parcel.h>
#include <utils/RefBase.h>
#include "OBCEnum.h"

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
/// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

/**
 * @brief data class : OBCConnectInfo
 */
class OBCConnectInfo : public android::RefBase
{
public:
    OBCConnectInfo() noexcept;
    ~OBCConnectInfo() = default;
    OBCConnectInfo(const OBCConnectInfo &other) noexcept;
    OBCConnectInfo &operator=(const OBCConnectInfo &other) noexcept;
    OBCConnectInfo(OBCConnectInfo &&) = delete;
    OBCConnectInfo &operator=(OBCConnectInfo &&) = delete;
    void setTo(const OBCConnectInfo &other) noexcept;
    error_t writeToParcel(android::Parcel *const parcel) const;
    error_t readFromParcel(const android::Parcel &parcel);
    void setDataFormat(tOBCConnectInfo &df) noexcept;
    void setData(const uint8_t mresponse, const uint16_t mconnectId) noexcept;
    uint8_t getResponse() const noexcept;
    uint16_t getConnectId() const noexcept;

private:
    uint8_t response;
    uint16_t connectId;
};

#endif /** SERVICELAYER_XXCLASS_OBCCONNECTINFO_H */
