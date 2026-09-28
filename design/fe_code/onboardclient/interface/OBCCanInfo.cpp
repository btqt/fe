
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
 *  This is the OBCCanInfo class.
 *
 */

#include "Log.h"
#include "./include/services/OnboardclientManagerService/OBCCanInfo.h"
// #include "./include/services/OnboardclientManagerService/OnboardclientCommand.h"

/**
 * @MISRA{MISRA C++-2008 Rule 16-2-1,"This is intended design"}
 */
#define LOG_TAG "IOBCCanInfo"
// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

OBCCanInfo::OBCCanInfo(const OBCCanInfo &other)
{
    init();
    setTo(other);
}

OBCCanInfo::OBCCanInfo()
{
    init();
}

void OBCCanInfo::init()
{
    mCanId = 0U;
    // 1: canId = static_cast< uint32_t >();
    // nTa = new Buffer();
    prnTa.resize(0U);
    // 1: nTa = static_cast< std::vector<std::string> >();
}

OBCCanInfo &OBCCanInfo::operator=(const OBCCanInfo &other)
{
    setTo(other);
    return *this;
}

void OBCCanInfo::setData(const uint32_t canId, const std::vector<std::string> nTa)
{

    this->mCanId = canId;
    this->prnTa = nTa;
}

void OBCCanInfo::setTo(const OBCCanInfo &other)
{
    setData(
        other.mCanId, other.prnTa);

    return;
}

error_t OBCCanInfo::writeToParcel(android::Parcel *const parcel) const
{
    // LOGV("writeToParcel start");
    LOGV("writeToParcel 1 canId = 0x%02X", this->mCanId);
    (void)parcel->writeUint32(this->mCanId);
    // LOGV("writeToParcel 2 nTa = ", this->nTa);
    (void)parcel->writeUint32(this->prnTa.size());
    LOGI("writeToParcel 1 nTa =%d", this->prnTa.size());
    if (this->prnTa.size() > 0U)
    {
        for (uint32_t i{0U}; i < this->prnTa.size(); i++)
        {
            (void)parcel->writeCString(this->prnTa[i].c_str());
            LOGI("writeToParcel 1 nTa = %s", this->prnTa[i].c_str());
        }
    }

    // LOGV("writeToParcel end");

    return E_OK;
}

error_t OBCCanInfo::readFromParcel(const android::Parcel &parcel)
{
#if 0 // for debugging : LOGV
    toString("readFromParcel OBCCanInfo 1: ");
#endif

    // LOGV("readFromParcel start");
    // LOGV("readFromParcel 1 canId");
    //  OBCCanInfo :: 1
    this->mCanId = static_cast<uint32_t>(parcel.readUint32()); // ? need static_cast
    LOGI("readFromParcel 1 canId = 0x%02X", this->mCanId);
    // LOGV("readFromParcel 2 nTa");
    //  OBCCanInfo :: 2
    // this->nTAsize = parcel.readInt32();
    const int32_t nTA_size{parcel.readInt32()};
    LOGI("readFromParcel 1 nTA_size = %d", nTA_size);
    this->prnTa.clear();
    if (nTA_size > 0)
    {
        for (int32_t i{0}; i < nTA_size; i++)
        {
            this->nTa().push_back(parcel.readCString());
            LOGI("readFromParcel 1 nTa = %s", this->prnTa[i].c_str());
        }
    }
    else
    {
        // do nothing
    }
    // LOGV("readFromParcel end");

#if 0 // for debugging : LOGV
    toString("readFromParcel OBCCanInfo : ");
#endif

    return E_OK;
}

uint32_t OBCCanInfo::getCanId() const noexcept
{
    return this->mCanId;
}

std::vector<std::string> OBCCanInfo::getNTa() noexcept
{
    return this->prnTa;
}

void OBCCanInfo::toString(const char_t *const from) const
{
    LOGV("===toString");
    LOGV("%s : OBCCanInfo-> canId = 0x%02X", from, this->mCanId);
    // LOGV("%s : OBCCanInfo-> nTa = ", from, this->nTa);
    // LOGV("===2");
}
