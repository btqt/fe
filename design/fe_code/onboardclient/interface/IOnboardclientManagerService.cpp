
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
 * @MISRA{MISRA C++-2008 Rule 16-0-3,This is intended design}
 */
#define LOG_TAG "IOnboardclientManagerService"
#include "Log.h"

#include <binder/Parcel.h>

#include "./include/services/OnboardclientManagerService/IOnboardclientManagerService.h"

#include "Typedef.h"
#include "utils/Handler.h"
#include <binder/BinderService.h>
#include <utils/RefBase.h>
#include <cstdint>

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

#define IPC_MAX_READABLE_SIZE 1024000

enum
{
    OP_REGISTER_RECEIVER = android::IBinder::FIRST_CALL_TRANSACTION,
    OP_REGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT,
    OP_REGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT,

    OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT,
    OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT,

    OP_SENDUDSDATA,

    OP_REGISTER_TEST_ONNOTIFYOBD2EVENT,
    OP_REGISTER_TEST_ONRESPONSEEVENT,
    OP_CONNECT,
    OP_DISCONNECT,
    OP_SEND_RESPONSE,
    OP_SEND_ACK_ROB,
    OP_SET_APP_PRIORITY,
    OP_SET_QUEUE_SIZE,
    OP_GET_MAX_CONNECT_COUNT,
    // @CGA_VARIANT_START{"__ENUM_SCOPE__:__YOUR_CODE__:variant"}
    // @CGA_VARIANT___END{"__ENUM_SCOPE__:__YOUR_CODE__:variant"}
};

class BpOnboardclientManagerService : public android::BpInterface<IOnboardclientManagerService>
{
public:
    /**
     * @brief It is the constructor of BpOnboardclientManagerService.
     *
     * @param[in] impl android binder
     * @retval   void
     *
     */
    explicit BpOnboardclientManagerService(const android::sp<android::IBinder> &impl) noexcept : BpInterface<IOnboardclientManagerService>(impl)
    {
    }

    /**
     * @brief onNotifyOBD2Event(..) is the function to register receiver for the BpOnboardClientReceiver .
     *
     * @details Wired connection detection notification event
     *
     * @param[in] receiver android::sp< IOnboardClientReceiver > interface
     *
     * @retval   error_t   If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual error_t registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        Parcel data{};
        Parcel reply{};
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        LOGV("BP registerReceiverOnboardClientReceiverOnNotifyOBD2Event : OnboardClientReceiver onNotifyOBD2Event");

        (void)data.writeStrongBinder(android::IInterface::asBinder(receiver));

        (void)remote()->transact(OP_REGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT, data, &reply);
        return reply.readInt32();
    }

    /**
     * @brief onResponseEvent(..) is the function to register receiver for the BpOnboardClientReceiver .
     *
     * @details Notify UDS response
     *
     * @param[in] receiver android::sp< IOnboardClientReceiver > interface
     *
     * @retval   error_t   If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual error_t registerReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        Parcel data{};
        Parcel reply{};
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        LOGV("BP registerReceiverOnboardClientReceiverOnResponseEvent : OnboardClientReceiver onResponseEvent");

        (void)data.writeStrongBinder(android::IInterface::asBinder(receiver));

        (void)remote()->transact(OP_REGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT, data, &reply);
        return reply.readInt32();
    }

    /**
     * @brief onNotifyOBD2Event(..) is the function to unregister receiver for the BpOnboardClientReceiver .
     *
     * @details Wired connection detection notification event
     *
     * @param[in] receiver android::sp< IOnboardClientReceiver > interface
     * @retval   error_t   If the receiver is unregistered successfully, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual error_t unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        Parcel data{};
        Parcel reply{};
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        LOGV("BP unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event : OnboardClientReceiver onNotifyOBD2Event");

        (void)data.writeStrongBinder(android::IInterface::asBinder(receiver));

        (void)remote()->transact(OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT, data, &reply);
        return reply.readInt32();
    }

    /**
     * @brief onResponseEvent(..) is the function to unregister receiver for the BpOnboardClientReceiver .
     *
     * @details Notify UDS response
     *
     * @param[in] receiver android::sp< IOnboardClientReceiver > interface
     * @retval   error_t   If the receiver is unregistered successfully, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual error_t unregisterReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        Parcel data{};
        Parcel reply{};
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        LOGV("BP unregisterReceiverOnboardClientReceiverOnResponseEvent : OnboardClientReceiver onResponseEvent");

        (void)data.writeStrongBinder(android::IInterface::asBinder(receiver));

        (void)remote()->transact(OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT, data, &reply);
        return reply.readInt32();
    }

    /* auto __ wishtoUseAPI func auto CGA start-------------------------------------------------*/
    /**
     * @brief API : send UDS request
     *
     * this funciton will be invoked when application or another services call this function through binder in the BpOnboardclientManagerService.
     *
     * @param[in] connectId connectID
     * @param[in] udsRequest UDS request
     * @retval   uint8_t     If it is successful, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual uint8_t sendUdsData(const uint16_t connectId, const android::sp<Buffer> udsRequest)
    {
        Parcel data{};
        Parcel reply{};

        LOGV("Bp Interface ManagerService sendUdsData");
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        LOGV("writeToParcel 5 connectId = %u", connectId);
        (void)data.writeUint32(static_cast<uint32_t>(connectId));
        LOGV("writeToParcel 5 udsRequest = %d", udsRequest->size());
        (void)data.writeByteArray(udsRequest->size(), udsRequest->data());
        // @CGA_VARIANT_START{"IOnboardclientManager:sendUdsData()"}
        (void)remote()->transact(OP_SENDUDSDATA, data, &reply);
        const uint32_t tempVal{reply.readUint32()};
        uint8_t ret{0U};
        if (tempVal <= static_cast<uint32_t>(UINT8_MAX))
        {
            ret = static_cast<uint8_t>(tempVal);
        }
        else
        {
            LOGV("Out of range");
        }
        return ret;
        // @CGA_VARIANT___END{"IOnboardclientManager:sendUdsData()"}
    }
    /* auto __ wishtoUseAPI func auto CGA end-------------------------------------------------*/

    /* auto __ RECEIVER API func auto CGA start-------------------------------------------------*/
    // RECEIVER API (testCallBack)
    /**
     * @brief receiver API for sldd : Wired connection detection notification event
     *
     * this funciton will be invoked when application or another services call this function through binder in the BpOnboardclientManagerService.
     *
     *
     * @retval   error_t     If it is successful, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual error_t testOnNotifyOBD2Event()
    {
        Parcel data{};
        Parcel reply{};

        LOGV("Bp Interface ManagerService onNotifyOBD2Event");
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());

#if 1 // for debugging : LOGV(1-3)
#endif

        (void)remote()->transact(OP_REGISTER_TEST_ONNOTIFYOBD2EVENT, data, &reply);

        //  ISSUE inOut

#if 1 // for debugging : LOGV(1-4)
#endif

        return reply.readInt32();
    }
    /**
     * @brief receiver API for sldd : Notify UDS response
     *
     * this funciton will be invoked when application or another services call this function through binder in the BpOnboardclientManagerService.
     *
     * @param[in] message Response Information
     *
     * @retval   error_t     If it is successful, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual error_t testOnResponseEvent(const android::sp<OBCResponseEventInfo> message)
    {
        Parcel data{};
        Parcel reply{};

        LOGV("Bp Interface ManagerService onResponseEvent");
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());

        (void)message->writeToParcel(&data);
        // (void)data.readFromParcel(message);

        (void)remote()->transact(OP_REGISTER_TEST_ONRESPONSEEVENT, data, &reply);

        //  ISSUE inOut
        return reply.readInt32();
    }
    /* auto __ RECEIVER API func auto CGA end-------------------------------------------------*/
    virtual error_t connect(const android::sp<OBCTransportInfo> transportInfo, const std::string appName, android::sp<OBCConnectInfo> &connectInfo)
    {
        Parcel data{};
        Parcel reply{};

        LOGV("Bp Interface ManagerService onResponseEvent");
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        error_t res{E_OK};
        // OBCTransportInfo transportInfo ;
        (void)transportInfo->writeToParcel(&data);
        (void)data.writeCString(appName.c_str());
        if (connectInfo == nullptr)
        {
            connectInfo = new OBCConnectInfo();
        }
        // TODO add logic
        (void)remote()->transact(OP_CONNECT, data, &reply);
        (void)connectInfo->readFromParcel(reply);
        res = reply.readInt32();
        return res;
    }

    virtual uint8_t disconnect(const uint16_t connectId)
    {
        Parcel data{};
        Parcel reply{};

        LOGV("Bp Interface ManagerService onResponseEvent");
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        (void)data.writeUint32(static_cast<uint32_t>(connectId));
        (void)remote()->transact(OP_DISCONNECT, data, &reply);
        const uint32_t tempVal{reply.readUint32()};
        uint8_t ret{0U};
        if (tempVal <= static_cast<uint32_t>(UINT8_MAX))
        {
            ret = static_cast<uint8_t>(tempVal);
        }
        else
        {
            LOGV("Out of range");
        }
        return ret;
    }

    virtual error_t sendUdsResponseData(const android::sp<Buffer> udsResponse)
    {
        Parcel data{};
        Parcel reply{};

        LOGV("Bp Interface ManagerService onResponseEvent");
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        (void)data.writeByteArray(udsResponse->size(), udsResponse->data());
        (void)remote()->transact(OP_SEND_RESPONSE, data, &reply);
        return reply.readInt32();
    }
    virtual error_t sendAckToMcu(const uint8_t reason)
    {
        Parcel data{};
        Parcel reply{};

        LOGV("Bp Interface ManagerService sendAckToMcu");
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        (void)data.writeUint32(static_cast<uint32_t>(reason));
        (void)remote()->transact(OP_SEND_ACK_ROB, data, &reply);
        return reply.readInt32();
    }
    virtual uint16_t getMaxConnectCount(const uint8_t protocolType)
    {
        Parcel data{};
        Parcel reply{};
        LOGV("Bp Interface ManagerService getMaxConnectCount");
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        (void)data.writeUint32(static_cast<uint32_t>(protocolType));
        (void)remote()->transact(OP_GET_MAX_CONNECT_COUNT, data, &reply);
        const uint32_t tempVal{reply.readUint32()};
        uint16_t ret{0U};
        if (tempVal <= static_cast<uint32_t>(UINT16_MAX))
        {
            ret = static_cast<uint16_t>(tempVal);
        }
        else
        {
            LOGV("Out of range");
        }
        return ret;
    }
    virtual uint8_t setAppPriority(const std::string appName, const uint8_t priority)
    {
        Parcel data{};
        Parcel reply{};
        LOGV("Bp Interface ManagerService setAppPriority");
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        (void)data.writeCString(appName.c_str());
        (void)data.writeUint32(static_cast<uint32_t>(priority));
        LOGV("Bp Interface ManagerService setAppPriority: app name: %s, %u", appName.c_str(), priority);
        (void)remote()->transact(OP_SET_APP_PRIORITY, data, &reply);
        const uint32_t tempVal{reply.readUint32()};
        uint8_t ret{0U};
        if (tempVal <= static_cast<uint32_t>(UINT8_MAX))
        {
            ret = static_cast<uint8_t>(tempVal);
        }
        else
        {
            LOGV("Out of range");
        }
        return ret;
    }
    virtual uint8_t setMaxExceeded(const uint8_t queueType, const uint32_t queueSize)
    {
        Parcel data{};
        Parcel reply{};
        LOGV("Bp Interface ManagerService setMaxExceeded");
        (void)data.writeInterfaceToken(IOnboardclientManagerService::getInterfaceDescriptor());
        (void)data.writeUint32(static_cast<uint32_t>(queueType));
        (void)data.writeUint32(queueSize);
        LOGV("Bp Interface ManagerService setMaxExceeded: queue type: %u, queue size: %lu", queueType, queueSize);
        (void)remote()->transact(OP_SET_QUEUE_SIZE, data, &reply);
        const uint32_t tempVal{reply.readUint32()};
        uint8_t ret{0U};
        if (tempVal <= static_cast<uint32_t>(UINT8_MAX))
        {
            ret = static_cast<uint8_t>(tempVal);
        }
        else
        {
            LOGV("Out of range");
        }
        return ret;
    }

    // @CGA_VARIANT_START{"BpOnboardclientManagerService:__LEGACY_API_YOUR_CODE__:variant"}
    // @CGA_VARIANT___END{"BpOnboardclientManagerService:__LEGACY_API_YOUR_CODE__:variant"}
};

IMPLEMENT_META_INTERFACE(OnboardclientManagerService, "service_layer.IOnboardclientManagerService");

// ----------------------------------------------------------------------

android::status_t BnOnboardclientManagerService::onTransact(const uint32_t code, const Parcel &data, Parcel *const reply, const uint32_t flags)
{

    switch (code)
    {
    case OP_REGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT:
    {
        LOGV("OP_REGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        const android::sp<IOnboardClientReceiver> receiver{android::interface_cast<IOnboardClientReceiver>(data.readStrongBinder())};
        error_t tidl_result{E_OK};
        if (receiver != nullptr)
        {
            tidl_result = registerReceiverOnboardClientReceiverOnNotifyOBD2Event(receiver);
        }
        if (tidl_result != E_OK)
        {
            tidl_result = E_ERROR;
        }
        (void)reply->writeInt32(tidl_result);
    }
    break;
    case OP_REGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT:
    {
        LOGV("OP_REGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        const android::sp<IOnboardClientReceiver> receiver{android::interface_cast<IOnboardClientReceiver>(data.readStrongBinder())};
        error_t tidl_result{E_OK};
        if (receiver != nullptr)
        {
            tidl_result = registerReceiverOnboardClientReceiverOnResponseEvent(receiver);
        }
        if (tidl_result != E_OK)
        {
            tidl_result = E_ERROR;
        }

        (void)reply->writeInt32(tidl_result);
    }
    break;

    case OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT:
    {
        LOGV("OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        const android::sp<IOnboardClientReceiver> receiver{android::interface_cast<IOnboardClientReceiver>(data.readStrongBinder())};
        error_t tidl_result{E_OK};
        if (receiver != nullptr)
        {
            tidl_result = unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(receiver);
        }
        if (tidl_result != E_OK)
        {
            tidl_result = E_ERROR;
        }

        (void)reply->writeInt32(tidl_result);
    }
    break;
    case OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT:
    {
        LOGV("OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        const android::sp<IOnboardClientReceiver> receiver{android::interface_cast<IOnboardClientReceiver>(data.readStrongBinder())};
        error_t tidl_result{E_OK};
        if (receiver != nullptr)
        {
            tidl_result = unregisterReceiverOnboardClientReceiverOnResponseEvent(receiver);
        }
        if (tidl_result != E_OK)
        {
            tidl_result = E_ERROR;
        }

        (void)reply->writeInt32(tidl_result);
    }
    break;

        /* auto __ wishtoUseAPI IMPLEMENT_META_INTERFACE auto CGA start-------------------------------------------------*/
    case OP_SENDUDSDATA:
    {
        // declare the temporary variable with _local_
        LOGV("Bn onTransact sendUdsData API");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        uint8_t res{0U};
        const uint32_t tempConId{data.readUint32()};
        uint16_t connectId{0U};
        if (tempConId <= static_cast<uint32_t>(UINT16_MAX))
        {
            connectId = static_cast<uint16_t>(tempConId);
        }
        else
        {
            (void)connectId;
            LOGV("Out of range");
        }
        const android::sp<Buffer> local_udsData{new Buffer()};
        const int32_t dataSize{data.readInt32()};
        LOGI("Size of UDS data %d", dataSize);
        if (dataSize > 0)
        {
            const uint8_t *const udsData{static_cast<const uint8_t *>(data.readInplace(static_cast<uint32_t>(dataSize)))};
            if (udsData != nullptr)
            {
                local_udsData->setTo(udsData, dataSize);
                res = sendUdsData(connectId, local_udsData);
            }
            else
            {
                (void)connectId;
                res = OBCEnum::OBCErrCode::OBC_ERR_INVALID_PARAMETERS;
            }
        }
        else
        {
            // do nothing
            (void)connectId;
            res = OBCEnum::OBCErrCode::OBC_ERR_INVALID_PARAMETERS;
        }
        (void)reply->writeUint32(static_cast<uint32_t>(res));
        // @CGA_VARIANT_START{"OnboardclientManager:sendUdsData()::OP_SENDUDSDATA"}
        // write your own code here
        // @CGA_VARIANT___END{"OnboardclientManager:sendUdsData()::OP_SENDUDSDATA"}
    }
    break;
        /* auto __ wishtoUseAPI IMPLEMENT_META_INTERFACE auto CGA end-------------------------------------------------*/

        /* auto __ RECEIVER API IMPLEMENT_META_INTERFACE auto CGA start-------------------------------------------------*/
        // RECEIVER API (testCallBack)
    case OP_REGISTER_TEST_ONNOTIFYOBD2EVENT:
    {
        // declare the temporary variable with _local_
        LOGV("Bn onTransact onNotifyOBD2Event RECEIVER");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        error_t _local_err{E_OK};
        _local_err = testOnNotifyOBD2Event( //  Parm:0
        );
        (void)reply->writeInt32(_local_err);
    }
    break;
    case OP_REGISTER_TEST_ONRESPONSEEVENT:
    {
        // declare the temporary variable with _local_
        LOGV("Bn onTransact onResponseEvent RECEIVER");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        const android::sp<OBCResponseEventInfo> _local_message{new OBCResponseEventInfo()};
        (void)_local_message->readFromParcel((const android::Parcel &)data);
        // OBCResponseEventInfo  _local_message  = static_cast<OBCResponseEventInfo>( data.writeToParcel() ); // ? need static_cast

        error_t _local_err{E_OK};
        _local_err = testOnResponseEvent( //  Parm:0
            _local_message                // android::sp< OBCResponseEventInfo >&  ,  Parm:1
        );

        //  ISSUE inOut

        (void)reply->writeInt32(_local_err);
    }
    break;
    case OP_CONNECT:
    {
        LOGV("Bn onTransact connect API");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        error_t res{E_OK};
        std::string mAppName{""};
        android::sp<OBCConnectInfo> mConnectInfo{new OBCConnectInfo()};
        const android::sp<OBCTransportInfo> mTransportInfo{new OBCTransportInfo()};
        (void)mTransportInfo->readFromParcel(data);
        mAppName = static_cast<std::string>(data.readCString());
        res = connect(mTransportInfo, mAppName, mConnectInfo);
        (void)mConnectInfo->writeToParcel(reply);
        (void)reply->writeInt32(res);
        break;
    }
    case OP_DISCONNECT:
    {
        LOGV("Bn onTransact disconnect API");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        const uint32_t tempConnId{data.readUint32()};
        if (tempConnId <= static_cast<uint32_t>(UINT16_MAX))
        {
            uint8_t res{E_OK};
            const uint16_t connectId{static_cast<uint16_t>(tempConnId)};
            res = disconnect(connectId);
            (void)reply->writeUint32(static_cast<uint32_t>(res));
        }
        else
        {
            LOGV("Out of range");
        }
        break;
    }
    case OP_SEND_RESPONSE:
    {
        LOGV("Bn onTransact send UDS response");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        const android::sp<Buffer> spBuf{new Buffer()};
        error_t res{E_ERROR};
        const int32_t dataSize{data.readInt32()};
        LOGI("Size of UDS data %d", dataSize);
        if (dataSize > 0)
        {
            const uint8_t *const udsData{static_cast<const uint8_t *>(data.readInplace(static_cast<uint32_t>(dataSize)))};
            if (udsData != nullptr)
            {
                spBuf->setTo(udsData, dataSize);
                if (spBuf != nullptr)
                {
                    res = sendUdsResponseData(spBuf);
                }
            }
        }
        if (res != E_OK)
        {
            res = E_ERROR;
        }
        (void)reply->writeInt32(res);
        break;
    }
    case OP_SEND_ACK_ROB:
    {
        LOGV("Bn onTransact send ACK RoB to MCU API");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        error_t res{E_OK};
        uint8_t reason{0U};
        const uint32_t tempReason{data.readUint32()};
        if (tempReason <= static_cast<uint32_t>(UINT8_MAX))
        {
            reason = static_cast<uint8_t>(tempReason);
        }
        else
        {
            LOGV("Out of range");
        }
        res = sendAckToMcu(reason);
        if ((res > E_OK) || (res < E_FRAME_NOTREADY))
        {
            res = E_ERROR;
        }

        (void)reply->writeInt32(res);
        break;
    }
    case OP_GET_MAX_CONNECT_COUNT:
    {
        LOGV("Bn onTransact getMaxConnectCount API");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        uint16_t res{0U};
        uint8_t protocolType{0U};
        const uint32_t tempProto{data.readUint32()};
        if (tempProto <= static_cast<uint32_t>(UINT8_MAX))
        {
            protocolType = static_cast<uint8_t>(tempProto);
        }
        else
        {
            LOGV("Out of range");
        }
        res = getMaxConnectCount(protocolType);
        (void)reply->writeUint32(static_cast<uint32_t>(res));
        break;
    }
    case OP_SET_APP_PRIORITY:
    {
        LOGV("Bn onTransact setAppPriority API");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        uint8_t res{0U};
        std::string mAppName{""};
        uint8_t priority{0U};
        mAppName = static_cast<std::string>(data.readCString());
        const uint32_t tempPriority{data.readUint32()};
        if (tempPriority <= static_cast<uint32_t>(UINT8_MAX))
        {
            priority = static_cast<uint8_t>(tempPriority);
        }
        else
        {
            LOGV("Out of range");
        }
        LOGV("Bn Interface ManagerService setAppPriority: app name: %s, %u", mAppName.c_str(), priority);
        res = setAppPriority(mAppName, priority);
        (void)reply->writeUint32(static_cast<uint32_t>(res));
        break;
    }
    case OP_SET_QUEUE_SIZE:
    {
        LOGV("Bn onTransact setMaxExceeded API");
        CHECK_INTERFACE(OnboardclientManagerService, data, reply);
        uint8_t res{0U};
        uint8_t queueType{0U};
        const uint32_t tempPriority{data.readUint32()};
        const uint32_t queueSize{data.readUint32()};
        if (tempPriority <= static_cast<uint32_t>(UINT8_MAX))
        {
            queueType = static_cast<uint8_t>(tempPriority);
        }
        else
        {
            LOGV("Out of range");
        }
        LOGV("Bn Interface ManagerService setMaxExceeded: type: %u, queueSize %lu", queueType, queueSize);
        res = setMaxExceeded(queueType, queueSize);
        (void)reply->writeUint32(static_cast<uint32_t>(res));
        break;
    }
    /* auto __ RECEIVER API IMPLEMENT_META_INTERFACE auto CGA end-------------------------------------------------*/
    // @CGA_VARIANT_START{"BnOnboardclientManagerService:__LEGACY_API_YOUR_CODE__:variant"}
    // @CGA_VARIANT___END{"BnOnboardclientManagerService:__LEGACY_API_YOUR_CODE__:variant"}
    default:
        return android::BBinder::onTransact(code, data, reply, flags);
    }

    return E_OK;
}
//**********************************************************************************************
