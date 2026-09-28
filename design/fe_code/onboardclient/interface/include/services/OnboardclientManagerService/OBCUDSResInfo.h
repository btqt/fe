
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

#ifndef SERVICELAYER_XXCLASS_OBCUDSRESINFO_H
#define SERVICELAYER_XXCLASS_OBCUDSRESINFO_H

#include "Typedef.h"

#include "utils/Buffer.h"

#include "utils/external/mindroid/lang/String.h"
#include "Log.h"
#include "Error.h"
#include <binder/Parcel.h>
#include <utils/RefBase.h>
#include "OBCEnum.h"
#include "OBCUDSResInfo.h"
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
 * @brief data class : OBCUDSResInfo
 */
class OBCUDSResInfo : public android::RefBase
{
public:
    /**
     * @brief constructor of the OBCUDSResInfo data class.
     *
     */
    OBCUDSResInfo();
    /**
     * @brief copy operator of the OBCUDSResInfo data class.
     *
     * @param[in] other OBCUDSResInfo
     *
     */
    OBCUDSResInfo(const OBCUDSResInfo &other);
    /**
     * @brief destructor of the OBCUDSResInfo data class.
     *
     */
    virtual ~OBCUDSResInfo() = default;
    /**
     * @brief = operator of the OBCUDSResInfo data class.
     *
     * @param[in] other OBCUDSResInfo
     * @retval   OBCUDSResInfo
     *
     */
    OBCUDSResInfo &operator=(const OBCUDSResInfo &other);
    OBCUDSResInfo(OBCUDSResInfo &&) = delete;
    OBCUDSResInfo &operator=(OBCUDSResInfo &&) = delete;
    /**
     * @brief It is for setting to OBCUDSResInfo of the OBCUDSResInfo data class.
     *
     * @param[in] other OBCUDSResInfo
     * @retval   void
     *
     */
    void setTo(const OBCUDSResInfo &other);
    /**
     * @brief This function is for writing parcel of the OBCUDSResInfo data class.
     *
     * @param[in] parcel android Parcel
     * @retval   error_t   If success, return E_OK, otherwise, return E_ERROR.
     *
     */
    error_t writeToParcel(android::Parcel *const parcel) const;
    /**
     * @brief This function is for reading from parcel of the OBCUDSResInfo data class.
     *
     * @param[in] parcel android Parcel
     * @retval   error_t   If success, return E_OK, otherwise, return E_ERROR.
     *
     */
    error_t readFromParcel(const android::Parcel &parcel);

    /**
     * @brief This is a function that displays data by a specific name in the OBCUDSResInfo data class.
     *
     * @param[in] from  printing data
     * @retval void
     *
     */
    void toString(const char_t *const from) const;
    /**
     * @brief This function is for getting ProtocolType of the OBCUDSResInfo data class.
     *
     * @retval  uint8_t
     *
     */
    uint8_t getProtocolType() const noexcept;
    /**
     * @brief This function is for getting CanInfo of the OBCUDSResInfo data class.
     *
     * @retval  OBCCanInfo
     *
     */
    android::sp<OBCCanInfo> getCanInfo() const noexcept;
    /**
     * @brief This function is for getting ConnectId of the OBCUDSResInfo data class.
     *
     * @retval  uint16_t
     *
     */
    uint16_t getConnectId() const noexcept;
    /**
     * @brief This function is for getting ResponseType of the OBCUDSResInfo data class.
     *
     * @retval  uint8_t
     *
     */
    uint8_t getResponseType() const noexcept;
    /**
     * @brief This function is for getting UdsData of the OBCUDSResInfo data class.
     *
     * @retval  android::sp<Buffer>
     *
     */
    android::sp<Buffer> getUdsData() const noexcept;

    /**
     * @brief function for setting data into OBCUDSResInfoDataFormat structure
     *
     * @param[in] df  OBCUDSResInfoDataFormat structure
     * @param[in] mprotocolType  protocol type
     * @param[in] mcanInfo  CAN address information
     * @param[in] mconnectId  connection identifier
     * @param[in] mresponseType  UDS Response
     * @param[in] mudsData  UDS Response
     * @retval   void
     *
     */
    /**
     * @brief It is function for setting data of the OBCUDSResInfo data class.
     *
     * @param[in] mprotocolType  protocol type
     * @param[in] mcanInfo  CAN address information
     * @param[in] mconnectId  connection identifier
     * @param[in] mresponseType  UDS Response
     * @param[in] mudsData  UDS Response
     * @retval   void
     *
     */
    void setData(const uint8_t protocolType, const android::sp<OBCCanInfo> canInfo, const uint16_t connectId, const uint8_t responseType, const android::sp<Buffer> udsData);

public:
    android::sp<Buffer> &udsData() noexcept { return mUdsData; };
    uint8_t &protocolType() noexcept { return mProtocolType; }
    uint8_t &responseType() noexcept { return mResponseType; };
    uint16_t &connectId() noexcept { return mConnectId; };
    android::sp<OBCCanInfo> &canInfo() noexcept { return mCanInfo; };

private:
    void init();
    android::sp<Buffer> mUdsData;
    uint8_t mResponseType;
    uint8_t mProtocolType;
    android::sp<OBCCanInfo> mCanInfo;
    uint16_t mConnectId;
};

#endif /** SERVICELAYER_XXCLASS_OBCUDSRESINFO_H */
