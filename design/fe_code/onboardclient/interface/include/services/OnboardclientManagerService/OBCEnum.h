

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

#ifndef SERVICELAYER_XXENUM_OBCENUM_H
#define SERVICELAYER_XXENUM_OBCENUM_H

#include <climits>
#include <unordered_map>
#include <iostream>
#include <vector>
#include "Log.h"
#include "Error.h"
#include <binder/Parcel.h>
#include <utils/RefBase.h>

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
namespace OBCEnum
{
    enum OBCErrCode : uint8_t
    {
        OBC_OK = 0U,
        /**< normal response */
        OBC_NEGATIVE = 1U,
        /**< negative response */
        OBC_TIMEOUT = 2U,
        /**< time-out */
        OBC_ERR_FAILED = 3U,
        /**< Other errors */
        OBC_ERR_INVALID_PARAMETERS = 4U,
        /**< parameter error */
        OBC_ERR_NOT_CONNECTED = 5U,
        /**< Unconnected */
        OBC_ERR_SEND_UDS_DATA = 6U,
        /**< transmission error */
        OBC_ERR_NOT_DISCONNECTED = 7U,
        /**< disconnection error */
        OBC_ERR_NOT_SUPPORTED = 8U,
        /**< Unsupported */
        OBC_ERR_BUSY = 9U,
        /**< Busy status (OBC/IP and CAN Client are busy) */
        OBC_ERR_CONNECTED_MAX = 10U,
        /**< number of connections error */
        OBC_ERR_USED = 11U,
        /**< In-use state (processing high priority) */
        OBC_ERR_UDS_CANCEL_PRIORITY = 12U,
        /**< UDS request aborted (high priority processing interrupt) */
        OBC_ERR_DISCONNECTED_PRIORITY = 13U,
        /**< Interrupt disconnect (high priority processing interrupt) */
        OBC_ERR_MAX = 14U
    };
    enum OBCPriorityType : uint8_t
    {
        NONE = 0U,
        /**< unset */
        LOW = 1U,
        /**< Low Priority */
        MIDDLE = 2U,
        /**< Medium Priority */
        HIGH = 3U,
        /**< High priority */
        OBC_PRI_MAX = 4U
    };
    enum OBCProtocolType : uint8_t
    {
        UNKNOWN = 0U,
        /**< unknow */
        DOCAN = 1U,
        /**< DoCAN (11-bit standard CAN-ID) */
        DOCAN29BIT = 3U,
        /**< DoCAN (29-bit CAN-ID) */
        DOCAN11BITEX = 4U,
        /**< DoCAN (11-bit extended CAN-ID) */
        DOCAN29BITCANFD = 6U,
        /**< DoCAN (29-bit CAN-ID (CANFD)) */
        OBC_PROTOCOL_MAX = 7U
    };
    enum MCUProtocolType : uint8_t
    {
        MCU_DOCAN = 0U,
        MCU_DOCAN29BIT = 1U,
        MCU_DOCAN11BITEX = 2U,
        MCU_DOCAN29BITCANFD = 5U,
        MCU_PROTOCOL_MAX = 6U
    };
    enum OBCUdsResponseType : uint8_t
    {
        UNKOWN = 0U,
        /**< Unknown response */
        NORMAL = 1U,
        /**< Normal response */
        PERIODEC = 2U,
        /**< Periodic data response */
        EVENT = 3U,
        /**< Event response */
        OBC_RES_MAX = 4U
    };
    enum OBC_UDS_SID : uint8_t
    {
        SID_10_SESSION_CONTROL = 0x10U,
        SID_11_ECU_RESET = 0x11U,
        SID_14_CLEAR_DIAG_INFORMATION = 0x14U,
        SID_19_READ_DTC_INFORMATION = 0x19U,
        SID_22_READ_DATA_BY_IDENTIFIER = 0x22U,
        SID_23_READ_MEMORY_BY_ADDRESS = 0x23U,
        SID_27_SECURITY_ACCESS = 0x27U,
        SID_28_COMMUNICATION_CONTROL = 0x28U,
        SID_29_AUTHENTICATION = 0x29U,
        SID_2A_READ_DATA_BY_PERIODIC_IDENTIFIER = 0x2AU,
        SID_2C_DYNAMICALLY_DEFINE_DATA_IDENTIFIER = 0x2CU,
        SID_2E_WRITE_DATA_BY_IDENTIFIER = 0x2EU,
        SID_2F_IO_CONTROL_BY_IDENTIFIER = 0x2FU,
        SID_31_ROUTINE_CONTROL = 0x31U,
        SID_34_REQUEST_DOWNLOAD = 0x34U,
        SID_36_TRANSFER_DATA = 0x36U,
        SID_37_REQUEST_TRANSFER_EXIT = 0x37U,
        SID_38_REQUEST_FILE_TRANSFER = 0x38U,
        SID_3E_TESTER_PRESENT = 0x3EU,
        SID_85_CONTROL_DTC_SETTING = 0x85U,
        SID_86_RESPONSE_ON_EVENT = 0x86U
    };

}

#pragma pack(1)
typedef struct
{
    uint32_t canId;
    std::vector<std::string> nTa;
} tOBCCanInfo;
#pragma pack()

#pragma pack(1)
typedef struct
{
    OBCEnum::OBCErrCode response;
    uint16_t connectId;
} tOBCConnectInfo;
#pragma pack()

#pragma pack(1)
typedef struct
{
    OBCEnum::OBCProtocolType protocolType;
    tOBCCanInfo canInfo;
    bool periodicRes;
    uint16_t udsResTimeout;
} tOBCTransportInfo;
#pragma pack()

#pragma pack(1)
typedef struct
{
    OBCEnum::OBCProtocolType protocolType;
    tOBCCanInfo canInfo;
    uint16_t connectId;
    OBCEnum::OBCUdsResponseType responseType;
    std::vector<uint8_t> udsData;
} tOBCUDSResInfo;
#pragma pack()

#endif // SERVICELAYER_XXENUM_OBCENUM_H
