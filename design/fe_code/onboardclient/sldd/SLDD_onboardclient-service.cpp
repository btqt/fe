
/**
 * \brief     Implementation of onboardclient
 *
 * \details
 *    This software is copyright protected and proprietary to
 *    LG electronics. LGE grants to you only those rights as
 *    set out in the license conditions. All other rights remain
 *    with LG electronics.
 * \author       sungwoo.oh
 * \date       2015.10.23 UK time
 * \attention Copyright (c) 2015 by LG electronics co, Ltd. All rights reserved.
 */

#define LOG_TAG "SLDDonboardclient"
#include "Log.h"

#include <stdlib.h>
#include "sldd_common.h"
#include "utils/atoi.h"
#include "man.h"

#include "SLDD_onboardclient-service.h"
#include <services/OnboardclientManagerService/OnboardclientManager.h>
#include <services/OnboardclientManagerService/IOnboardclientManagerService.h>
#include <services/OnboardclientManagerService/IOnboardClientReceiver.h>
#include <services/OnboardclientManagerService/OBCResponseEventInfo.h>
#include <services/OnboardclientManagerService/OBCUDSResInfo.h>
#include <services/OnboardclientManagerService/OBCCanInfo.h>
#include <services/OnboardclientManagerService/OBCTransportInfo.h>
#include <services/OnboardclientManagerService/OBCConnectInfo.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdlib>
#include "man.h"

#define NOT_IMPLEMENTED ("Under construction\n")

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
// write your code
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

enum onboardclient_commands
{
    onboardclient_CMD_DEBUG_PUBLIC_API = 1,
    onboardclient_CMD_DEBUG_RECEIVER_CB,
};

// auto API start : wishtoUseAPI

static bool handler_connect(int32_t argc, char **argv);
static bool handler_disconnect(int32_t argc, char **argv);
static bool handler_sendUdsData(int32_t argc, char **argv);
static bool handler_sendUdsResponseData(int32_t argc, char **argv);
static bool handler_sendAckToMcu(int32_t argc, char **argv);
static bool handler_getMaxConnectCount(int32_t argc, char **argv);
static bool handler_setAppPriority(int32_t argc, char **argv);
static bool handler_setMaxExceeded(int32_t argc, char **argv);
// auto API end : wishtoUseAPI

// auto RECEIVER API start
static bool handler_testOnNotifyOBD2Event(int32_t argc, char **argv);
static bool handler_testOnResponseEvent(int32_t argc, char **argv);
// auto RECEIVER API end

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:SLDD_xxx-service.cpp:handler_definition"}
// write your own code here
// static bool handler_get_log_level(int32_t argc, char **argv);
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:SLDD_xxx-service.cpp:handler_definition"}

/**
 *    Declare binder interface
 *
 */

class OnboardClientReceiver : public BnOnboardClientReceiver
{
public:
    OnboardClientReceiver() {}
    virtual ~OnboardClientReceiver() {}

    virtual void onNotifyOBD2Event() noexcept
    {
        _onNotifyOBD2Event_called = true;
        LOGV("%s() called", __func__);
    }

    virtual void onResponseEvent(const android::sp<OBCResponseEventInfo> message) noexcept
    {
        _onResponseEvent_called = true;
        LOGV("%s() called", __func__);
    }

public:
    bool _onNotifyOBD2Event_called = false;
    bool _onResponseEvent_called = false;
};

SL_INTERFACE_DECLARE2(OnboardclientManagerService, ONBOARDCLIENT_SRV_NAME);

// auto API start
sldd_cmd_table_t onboardclient_cmdTable[] = {
    {"connect", onboardclient_CMD_DEBUG_PUBLIC_API, handler_connect, "sldd onboardclient connect <parameter> \n"},
    {"disconnect", onboardclient_CMD_DEBUG_PUBLIC_API, handler_disconnect, "sldd onboardclient disconnect <parameter> \n"},
    {"sendUdsData", onboardclient_CMD_DEBUG_PUBLIC_API, handler_sendUdsData, "sldd onboardclient sendUdsData <parameter> \n"},
    {"sendUdsResponseData", onboardclient_CMD_DEBUG_PUBLIC_API, handler_sendUdsResponseData, "sldd onboardclient sendUdsResponseData <parameter> \n"},
    {"sendAckToMcu", onboardclient_CMD_DEBUG_PUBLIC_API, handler_sendAckToMcu, "sldd onboardclient sendAckToMcu <parameter> \n"},
    {"getMaxConnectCount", onboardclient_CMD_DEBUG_PUBLIC_API, handler_getMaxConnectCount, "sldd onboardclient getMaxConnectCount <parameter> \n"},
    {"setMaxExceeded", onboardclient_CMD_DEBUG_PUBLIC_API, handler_setMaxExceeded, "sldd onboardclient setMaxExceeded <parameter> \n"},
    {"sendAckToMcu", onboardclient_CMD_DEBUG_PUBLIC_API, handler_sendAckToMcu, "sldd onboardclient sendAckToMcu <parameter> \n"},
    {"testOnNotifyOBD2Event", onboardclient_CMD_DEBUG_RECEIVER_CB, handler_testOnNotifyOBD2Event, "sldd onboardclient testOnNotifyOBD2Event <parameter> \n"},
    {"testOnResponseEvent", onboardclient_CMD_DEBUG_RECEIVER_CB, handler_testOnResponseEvent, "sldd onboardclient testOnResponseEvent <parameter>\n"},

    // @CGA_VARIANT_START{"SLDD_xxx-service.cpp:onResponseEvent__CUSTOM_PARSING__CODE__:variant"}
    // example>
    //{"getLogLevel", onboardclient_CMD_DEBUG_PUBLIC_API, handler_get_log_level,"sldd tidl getLogLevel\n"},
    // @CGA_VARIANT___END{"SLDD_xxx-service.cpp:onResponseEvent__CUSTOM_PARSING__CODE__:variant"}
    {nullptr, 0, nullptr, nullptr}};
// auto API end

bool commandActiononboardclient(int32_t argc, char **argv)
{
    if (argc == 0)
    {
        usage_onboardclient(nullptr);
        return true;
    }
    DO_HANDLER(onboardclient_cmdTable, argv[0], argc - 1, argv + 1);
}

char *usage_onboardclient(char *cmd)
{
    printMessage(" sldd onboardclient [<command>]           - perform onboardclient operation for TCU3 \n");
    printMessage("    <command> \n");
    PRINT_USAGE(onboardclient_cmdTable);

    return nullptr;
}

bool handler_connect(int32_t argc, char **argv) // 0
{
    uint8_t res{0U};
    char *endptr;
    android::sp<OBCTransportInfo> mObcTransportInfo = new OBCTransportInfo();
    android::sp<OBCConnectInfo> mOBCConnectInfo = new OBCConnectInfo();
    uint8_t protocolType{0U};
    OBCCanInfo canInfo;
    uint32_t canId{0U};
    std::vector<std::string> nTa;
    uint8_t periodicRes{false};
    uint16_t udsResTimeout{0U};
    std::string appName{""};
    uint8_t nTaSize{0U};
    // sldd onboardclient connect 0 0x05040302 1 FF 0 100 remotediag
    str_convert(protocolType, argv[0]);
    str_convert(canId, argv[1]);
    str_convert(nTaSize, argv[2]);
    nTa.resize(nTaSize);
    for (uint8_t i = 0; i < nTaSize; i++)
    {
        // nTa.push_back(argv[3 + i]);
        nTa[i] = argv[3 + i];
    }
    str_convert(periodicRes, argv[argc - 3]);
    str_convert(udsResTimeout, argv[argc - 2]);
    appName = argv[argc - 1];

    canInfo.setData(canId, nTa);
    mObcTransportInfo->setData(protocolType, canInfo, periodicRes, udsResTimeout);

    android::sp<IOnboardclientManagerService> mOnboardclient{interface_cast<IOnboardclientManagerService>(defaultServiceManager()->getService(String16("service_layer.OnboardclientManagerService")))};
    res = mOnboardclient->connect(mObcTransportInfo, appName, mOBCConnectInfo);
    printMessage("mOBCConnectInfo response %u, connectId %lu\n", mOBCConnectInfo->getResponse(), mOBCConnectInfo->getConnectId());
    return res;
}

bool handler_disconnect(int32_t argc, char **argv) // 0
{
    uint8_t res{0U};
    if (argc != 1)
    {
        printMessage("please input parameter properly\n");
        return false;
    }
    uint16_t connectId{0U};
    str_convert(connectId, argv[0]);
    printMessage("handler_disconnect, connectId = %u  \n", connectId);
    android::sp<IOnboardclientManagerService> mOnboardclient{interface_cast<IOnboardclientManagerService>(defaultServiceManager()->getService(String16("service_layer.OnboardclientManagerService")))};
    res = mOnboardclient->disconnect(connectId);
    printMessage("handler_disconnect, ret = %d  \n", res);
    return true;
}

bool handler_sendUdsData(int32_t argc, char **argv) // 0
{
    printMessage("sendUdsData  \n");
    // @CGA_VARIANT_START{"OnboardclientManager-SLDD::handler_sendUdsData"}
    // write your own code here
    uint8_t ret{0U};
    char *endptr;
    int32_t payloadLength{0};
    uint16_t connectId{0U};
    str_convert(connectId, argv[0]);
    payloadLength = argc - 1;
    android::sp<Buffer> udsData = new Buffer();
    if (argc < 1)
    {
        printMessage("please input parameter properly\n");
        return false;
    }
    udsData->setSize(payloadLength);
    for (int i = 0; i < payloadLength; i++)
    {
        udsData->data()[i] = static_cast<uint8_t>(strtol(argv[i + 1], &endptr, 16));
    }

    android::sp<IOnboardclientManagerService> mOnboardclient{interface_cast<IOnboardclientManagerService>(defaultServiceManager()->getService(String16("service_layer.OnboardclientManagerService")))};
    ret = mOnboardclient->sendUdsData(connectId, udsData);
    printMessage("handler_sendUdsData, ret = %d  \n", ret);
    return true;
    // @CGA_VARIANT___END{"OnboardclientManager-SLDD::handler_sendUdsData"}
}

bool handler_sendUdsResponseData(int32_t argc, char **argv) // 0
{
    printMessage("sendUdsResponse  \n");
    // @CGA_VARIANT_START{"OnboardclientManager-SLDD::handler_sendUdsResponseData"}
    // write your own code here
    error_t ret{E_OK};
    char *endptr;
    int32_t payloadLength{0};
    payloadLength = argc;
    android::sp<Buffer> udsResponse = new Buffer();
    if (argc < 1)
    {
        printMessage("please input parameter properly\n");
        return false;
    }
    udsResponse->setSize(payloadLength);
    for (int i = 0; i < payloadLength; i++)
    {
        udsResponse->data()[i] = static_cast<uint8_t>(strtol(argv[i], &endptr, 16));
    }

    android::sp<IOnboardclientManagerService> mOnboardclient{interface_cast<IOnboardclientManagerService>(defaultServiceManager()->getService(String16("service_layer.OnboardclientManagerService")))};
    ret = mOnboardclient->sendUdsResponseData(udsResponse);
    printMessage("handler_sendUdsResponseData, ret = %d  \n", ret);
    return true;
    // @CGA_VARIANT___END{"OnboardclientManager-SLDD::handler_sendUdsResponseData"}
}

bool handler_sendAckToMcu(int32_t argc, char **argv) // 0
{
    error_t res{E_OK};
    char *endptr;
    android::sp<IOnboardclientManagerService> mOnboardclient{interface_cast<IOnboardclientManagerService>(defaultServiceManager()->getService(String16("service_layer.OnboardclientManagerService")))};
    uint8_t reason{0U};
    if (argc > 2)
    {
        printMessage("Invalid parameter");
        return res;
    }
    reason = static_cast<uint8_t>(strtol(argv[0], &endptr, 16));
    res = mOnboardclient->sendAckToMcu(reason);
    printMessage("sendAckToMcu %d\n", res);
    return true;
}

bool handler_getMaxConnectCount(int32_t argc, char **argv)
{
    uint16_t res{false};
    char *endptr;
    android::sp<IOnboardclientManagerService> mOnboardclient{interface_cast<IOnboardclientManagerService>(defaultServiceManager()->getService(String16("service_layer.OnboardclientManagerService")))};
    uint8_t protocolType{0U};
    if (argc > 1)
    {
        printMessage("Invalid parameter");
        return res;
    }
    protocolType = static_cast<uint8_t>(strtol(argv[0], &endptr, 16));
    res = mOnboardclient->getMaxConnectCount(protocolType);
    printMessage("getMaxConnectCount %u\n", res);
    return true;
}

bool handler_setAppPriority(int32_t argc, char **argv)
{
    uint8_t res{0U};
    char *endptr;
    android::sp<IOnboardclientManagerService> mOnboardclient{interface_cast<IOnboardclientManagerService>(defaultServiceManager()->getService(String16("service_layer.OnboardclientManagerService")))};
    uint8_t priority{0U};
    std::string appName{""};
    if (argc > 2)
    {
        printMessage("Invalid parameter");
        return res;
    }
    appName = argv[0];
    priority = static_cast<uint8_t>(strtol(argv[1], &endptr, 16));
    res = mOnboardclient->setAppPriority(appName, priority);
    printMessage("setAppPriority %u\n", res);
    return true;
}

bool handler_setMaxExceeded(int32_t argc, char **argv)
{
    uint8_t res{0U};
    char *endptr;
    android::sp<IOnboardclientManagerService> mOnboardclient{interface_cast<IOnboardclientManagerService>(defaultServiceManager()->getService(String16("service_layer.OnboardclientManagerService")))};
    uint8_t queueType{0U};
    uint32_t queueSize{0U};
    if (argc > 2)
    {
        printMessage("Invalid parameter");
        return res;
    }
    str_convert(queueType, argv[0]);
    str_convert(queueSize, argv[1]);

    res = mOnboardclient->setMaxExceeded(queueType, queueSize);
    printMessage("setAppPriority %u\n", res);
    return true;
}

bool handler_testOnNotifyOBD2Event(int32_t argc, char **argv) // 0
{
    printMessage("testOnNotifyOBD2Event  \n");

    error_t _tidl_result{E_OK};

    // minimum param count : 0

    if (argc < 0)
    {
        printMessage("please input parameter properly\n");
        return false;
    }

    class OnboardClientReceiver *my_rcv = new OnboardClientReceiver();
    android::sp<IOnboardClientReceiver> receiver = my_rcv;

    OnboardclientManager *onboardclient_mgr = OnboardclientManager::instance();

    // Initialize: declare variables

    // Input: set the value from argv

    // Set xxData class  if has xxData : each API has only one xxData.

    _tidl_result = onboardclient_mgr->registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        receiver);

    printMessage("regist OK (%d)\n", _tidl_result);

    // Call API
    _tidl_result = SL_INTERFACE_GET(OnboardclientManagerService)->testOnNotifyOBD2Event( //  Parm:0
    );

    int max_wait_sec = 10 /* 10 sec */;
    int max_cnt = max_wait_sec * 10;
    int cnt = 0;
    while (my_rcv->_onNotifyOBD2Event_called == false)
    {
        if (cnt++ >= max_cnt)
        {

            printMessage("Timeout (%d sec)\n", (max_cnt / 10));
            return false;
        }
        if (0 == (cnt % 10 /* per a sec */))
        {
            printMessage("waiting... receiver callback function\n");
        }
        usleep(100 * 1000);
    }
#if 1 // if you want to test of binderDied , you skip to run unregister function : onReceiverBinderDied()
    _tidl_result = onboardclient_mgr->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        receiver);
#endif
    printMessage("OK_done (%d)\n", _tidl_result);
    return true;
}

bool handler_testOnResponseEvent(int32_t argc, char **argv) // 0
{
    printMessage("testOnResponseEvent \n");
    error_t _tidl_result{E_OK};

    // minimum param count : 2
    if (argc < 7)
    {
        printMessage("please input parameter properly\n");
        return false;
    }
    char *endptr;
    int32_t payloadLength{0};
    // errcode protocolType canId nTa connectId responseType udsData
    uint8_t merrCode{0U};
    uint8_t protocolType{0U};
    uint16_t connectId{0U};
    uint32_t canId{0U};
    vector<std::string> temp;
    uint8_t responseType{0U};
    str_convert(merrCode, argv[0]);
    str_convert(protocolType, argv[1]);
    str_convert(canId, argv[2]);
    temp.push_back(argv[3]);
    str_convert(connectId, argv[4]);
    str_convert(responseType, argv[5]);
    payloadLength = argc - 6;
    android::sp<Buffer> udsResponse = new Buffer();

    udsResponse->setSize(payloadLength);
    for (int i = 0; i < payloadLength; i++)
    {
        udsResponse->data()[i] = static_cast<uint8_t>(strtol(argv[i + 6], &endptr, 16));
    }

    class OnboardClientReceiver *my_rcv = new OnboardClientReceiver();
    android::sp<IOnboardClientReceiver> receiver = my_rcv;

    OnboardclientManager *onboardclient_mgr = OnboardclientManager::instance();

    // Initialize: declare variables
    uint8_t data[2];
    data[0] = 0x01;
    data[1] = 0x05;
    android::sp<OBCUDSResInfo> mresInfo = new OBCUDSResInfo();
    android::sp<OBCCanInfo> mcanInfo = new OBCCanInfo();
    mcanInfo->setData(canId, temp);
    mresInfo->setData(protocolType, mcanInfo, connectId, responseType, udsResponse);
    // Input: set the value from argv
    // Set xxData class  if has xxData : each API has only one xxData.
    android::sp<OBCResponseEventInfo> mOBCResponseEventInfo = new OBCResponseEventInfo();
    // uint8_t * tempbuf = new uint8_t[mdataLen];
    //(void)std::memcpy(tempbuf, argv[2], mdataLen);
    mOBCResponseEventInfo->setData(merrCode, mresInfo);

    _tidl_result = onboardclient_mgr->registerReceiverOnboardClientReceiverOnResponseEvent(
        receiver);

    printMessage("regist OK (%d)\n", _tidl_result);

    // Call API
    _tidl_result = SL_INTERFACE_GET(OnboardclientManagerService)->testOnResponseEvent( //  Parm:0
        mOBCResponseEventInfo                                                          //  Parm:1
    );

    // mOBCResponseEventInfo->toString("sldd RECEIVER result:");

    int max_wait_sec = 10 /* 10 sec */;
    int max_cnt = max_wait_sec * 10;
    int cnt = 0;
    while (my_rcv->_onResponseEvent_called == false)
    {
        if (cnt++ >= max_cnt)
        {

            printMessage("Timeout (%d sec)\n", (max_cnt / 10));
            return false;
        }
        if (0 == (cnt % 10 /* per a sec */))
        {
            printMessage("waiting... receiver callback function\n");
        }
        usleep(100 * 1000);
    }
#if 1 // if you want to test of binderDied , you skip to run unregister function : onReceiverBinderDied()
    _tidl_result = onboardclient_mgr->unregisterReceiverOnboardClientReceiverOnResponseEvent(
        receiver);
#endif
    printMessage("OK_done (%d)\n", _tidl_result);
    return true;
}

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:SLDD_xxx-service.cpp:handler_implementation"}
/*
// dynamic log level : handler_get_log_level
bool handler_get_log_level(int32_t argc, char **argv)
{
    error_t __result{E_OK};
    OnboardclientManager*  _mgr = OnboardclientManager::instance();

    int32_t lvl=-1;
    __result = _mgr->getLogLevel(lvl);
    printMessage("getLogLevel(%d)\n", lvl);
    printMessage("OK_done (%d)\n", __result);
    return true;
}
*/
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:SLDD_xxx-service.cpp:handler_implementation"}

void register_onboardclient()
{
    registerCommands(MODULE_onboardclient_SLDD, nullptr, onboardclient_cmdTable);
}
