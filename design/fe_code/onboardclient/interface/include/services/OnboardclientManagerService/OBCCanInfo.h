
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

#ifndef SERVICELAYER_XXCLASS_OBCCANINFO_H
#define SERVICELAYER_XXCLASS_OBCCANINFO_H

#include "Typedef.h"
#include <vector>
#include "utils/Buffer.h"

#include "utils/external/mindroid/lang/String.h"
#include "Log.h"
#include "Error.h"
#include <binder/Parcel.h>
#include <utils/RefBase.h>
#include "OBCEnum.h"
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
 * @brief data class : OBCCanInfo
 */
class OBCCanInfo : public android::RefBase
{
public:
    /**
     * @brief constructor of the OBCCanInfo data class.
     *
     */
    OBCCanInfo();
    /**
     * @brief copy operator of the OBCCanInfo data class.
     *
     * @param[in] other OBCCanInfo
     *
     */
    OBCCanInfo(const OBCCanInfo &other);
    /**
     * @brief destructor of the OBCCanInfo data class.
     *
     */
    virtual ~OBCCanInfo() = default;
    /**
     * @brief = operator of the OBCCanInfo data class.
     *
     * @param[in] other OBCCanInfo
     * @retval   OBCCanInfo
     *
     */
    OBCCanInfo &operator=(const OBCCanInfo &other);
    OBCCanInfo(OBCCanInfo &&) = delete;
    OBCCanInfo &operator=(OBCCanInfo &&) = delete;
    /**
     * @brief It is for setting to OBCCanInfo of the OBCCanInfo data class.
     *
     * @param[in] other OBCCanInfo
     * @retval   void
     *
     */
    void setTo(const OBCCanInfo &other);
    /**
     * @brief This function is for writing parcel of the OBCCanInfo data class.
     *
     * @param[in] parcel android Parcel
     * @retval   error_t   If success, return E_OK, otherwise, return E_ERROR.
     *
     */
    error_t writeToParcel(android::Parcel *const parcel) const;
    /**
     * @brief This function is for reading from parcel of the OBCCanInfo data class.
     *
     * @param[in] parcel android Parcel
     * @retval   error_t   If success, return E_OK, otherwise, return E_ERROR.
     *
     */
    error_t readFromParcel(const android::Parcel &parcel);
    /**
     * @brief This is a function that displays data in the OBCCanInfo data class.
     *
     * @retval void
     *
     */
    /**
     * @brief This is a function that displays data by a specific name in the OBCCanInfo data class.
     *
     * @param[in] from  printing data
     * @retval void
     *
     */
    void toString(const char_t *const from) const;
    /**
     * @brief This function is for getting CanId of the OBCCanInfo data class.
     *
     * @retval  uint32_t
     *
     */
    uint32_t getCanId() const noexcept;
    /**
     * @brief This function is for getting NTa of the OBCCanInfo data class.
     *
     * @retval  std::vector<std::string>
     *
     */
    std::vector<std::string> getNTa() noexcept;

    /**
     * @brief function for setting data into OBCCanInfoDataFormat structure
     *
     * @param[in] df  OBCCanInfoDataFormat structure
     * @param[in] mcanId  Can ID
     * @param[in] mnTa  N_TA
     * @retval   void
     *
     */

    /**
     * @brief It is function for setting data of the OBCCanInfo data class.
     *
     * @param[in] mcanId  Can ID
     * @param[in] mnTa  N_TA
     * @retval   void
     *
     */
    void setData(const uint32_t canId, const std::vector<std::string> nTa);

public:
    uint32_t &canId() noexcept { return mCanId; };
    std::vector<std::string> &nTa() noexcept { return prnTa; };

private:
    void init();
    uint32_t mCanId;
    std::vector<std::string> prnTa;
};

#endif /** SERVICELAYER_XXCLASS_OBCCANINFO_H */
