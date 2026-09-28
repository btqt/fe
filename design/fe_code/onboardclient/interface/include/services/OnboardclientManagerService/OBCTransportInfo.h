
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

#ifndef SERVICELAYER_XXCLASS_OBCTRANSPORTINFO_H
#define SERVICELAYER_XXCLASS_OBCTRANSPORTINFO_H

#include "Typedef.h"

#include "utils/Buffer.h"

#include "utils/external/mindroid/lang/String.h"
#include "Log.h"
#include "Error.h"
#include <binder/Parcel.h>
#include <utils/RefBase.h>
#include "OBCEnum.h"
#include "OBCTransportInfo.h"
#include "OBCCanInfo.h"
// #include "OBCPriorityType.h"
// #include "OBCProtocolType.h"
// #include "OBCUdsResponseType.h"

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
/// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/**
 * @brief data class : OBCTransportInfo
 */
class OBCTransportInfo : public android::RefBase
{
public:
    /**
     * @brief constructor of the OBCTransportInfo data class.
     *
     */
    OBCTransportInfo();
    /**
     * @brief copy operator of the OBCTransportInfo data class.
     *
     * @param[in] other OBCTransportInfo
     *
     */
    OBCTransportInfo(const OBCTransportInfo &other);
    /**
     * @brief destructor of the OBCTransportInfo data class.
     *
     */
    virtual ~OBCTransportInfo() = default;
    /**
     * @brief = operator of the OBCTransportInfo data class.
     *
     * @param[in] other OBCTransportInfo
     * @retval   OBCTransportInfo
     *
     */
    OBCTransportInfo &operator=(const OBCTransportInfo &other);
    OBCTransportInfo(OBCTransportInfo &&) = delete;
    OBCTransportInfo &operator=(OBCTransportInfo &&) = delete;
    /**
     * @brief It is for setting to OBCTransportInfo of the OBCTransportInfo data class.
     *
     * @param[in] other OBCTransportInfo
     * @retval   void
     *
     */
    void setTo(const OBCTransportInfo &other);
    /**
     * @brief This function is for writing parcel of the OBCTransportInfo data class.
     *
     * @param[in] parcel android Parcel
     * @retval   error_t   If success, return E_OK, otherwise, return E_ERROR.
     *
     */
    error_t writeToParcel(android::Parcel *const parcel) const;
    /**
     * @brief This function is for reading from parcel of the OBCTransportInfo data class.
     *
     * @param[in] parcel android Parcel
     * @retval   error_t   If success, return E_OK, otherwise, return E_ERROR.
     *
     */
    error_t readFromParcel(const android::Parcel &parcel);
    /**
     * @brief This is a function that displays data in the OBCTransportInfo data class.
     *
     * @retval void
     *
     */
    /**
     * @brief This is a function that displays data by a specific name in the OBCTransportInfo data class.
     *
     * @param[in] from  printing data
     * @retval void
     *
     */
    void toString(const char_t *const from) const;
    /**
     * @brief function for setting data into tOBCTransportInfo structure
     *
     * @param[in] df  tOBCTransportInfo structure
     * @param[in] mprotocolType  protocol type
     * @param[in] mcanInfo  CAN address information
     * @param[in] mudsResTimeout  Response timeout for UDS request
     * @retval   void
     *
     */

    /**
     * @brief It is function for setting data of the OBCTransportInfo data class.
     *
     * @param[in] mprotocolType  protocol type
     * @param[in] mcanInfo  CAN address information
     * @param[in] mudsResTimeout  Response timeout for UDS request
     * @retval   void
     *
     */
    void setData(const uint8_t mprotocolType, const OBCCanInfo mcanInfo, const bool mperiodicRes, const uint16_t mudsResTimeout);
    uint16_t getUdsResTimeout() const;
    uint8_t getProtocolType() const;
    OBCCanInfo getCanInfo() const;
    bool getPeriodicRes() const;

public:
    uint16_t &udsResTimeout() noexcept { return mUdsResTimeout; };
    uint8_t &protocolType() noexcept { return mProtocolType; };
    OBCCanInfo &canInfo() noexcept { return mCanInfo; };
    bool &periodicRes() noexcept { return mPeriodicRes; };

private:
    void init() noexcept;
    uint16_t mUdsResTimeout;
    uint8_t mProtocolType;
    OBCCanInfo mCanInfo;
    bool mPeriodicRes;
};

#endif /** SERVICELAYER_XXCLASS_OBCTRANSPORTINFO_H */
