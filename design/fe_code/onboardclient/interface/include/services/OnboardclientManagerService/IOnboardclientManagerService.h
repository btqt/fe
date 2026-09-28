
/**
 * @if (CGA)
 *  @CGA_INCLUDE{IOnboardclientManagerService.h}
 * @endif
 */
// FileName - IOnboardclientManagerService.h
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

/** @defgroup ONBOARDCLIENTMGR_API ONBOARDCLIENTMGR ONBOARDCLIENT API
 *  @ingroup ONBOARDCLIENTMGR
 *
 *  Thease are the ONBOARDCLIENT APIs
 */

/** @defgroup ONBOARDCLIENTMGR_CB ONBOARDCLIENTMGR Callback
 *  @ingroup ONBOARDCLIENTMGR
 *
 *  This is the ONBOARDCLIENT Manager Receiver list.
 */

/** @defgroup ONBOARDCLIENT_RECEIVER_TEST_API ONBOARDCLIENT receiver Test API list
 *  @ingroup ONBOARDCLIENTMGR
 *
 *  This is the ONBOARDCLIENT Test API list for receiver.
 */

#ifndef IONBOARDCLIENTNOSTIC_MANAGER_SERVICE_H
#define IONBOARDCLIENTNOSTIC_MANAGER_SERVICE_H

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include "Typedef.h"

#include "OnboardclientCommand.h"
#include "IOnboardclientManagerServiceType.h"
#include "IOnboardClientReceiver.h"
#include "OBCEnum.h"
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

// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

/**
 * @brief If Apps or other services need to control ONBOARDCLIENT, It could be obatined by these API.
 * To use these API, APP or Service, You Should get IOnboardclientManaerService
 *
 * @note
 *  onboardclientManagerService service name is @b Onboardclient_SRV_NAME (="service_layer.onboardclientManagerService")
 */
class IOnboardclientManagerService : public android::IInterface
{
public:
    DECLARE_META_INTERFACE(OnboardclientManagerService);
    //< Interfaces for IOnboardclientManagerService

    /**
     * @ingroup ONBOARDCLIENTMGR_API
     *
     * @brief onNotifyOBD2Event(..) is the function to register receiver for the BpOnboardClientReceiver .
     *
     * @details Wired connection detection notification event
     * Managers created with TIDL can broadcast messages to other managers and apps.
     * When a specific event occurs in the Manager made with TIDL, if a manager wants to receive the event from the TIDL Manager,
     * manager can register events of interest to TIDL Manager with the registerreceiver function.
     * Then, when the event occurs, a specific event is delivered to the requested Manager through broadcast.
     * For example, if you want to know when the car has started, register the event through the registerreceiver function, and the car has started.
     * An event indicating that startup has occurred will be delivered to the manager who created the RegisterReceiver request.
     *
     * @param[in] receiver android::sp< OnboardClientReceiver > interface
     * @retval   error_t   If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual error_t registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp<IOnboardClientReceiver> &receiver) = 0;

    /**
     * @ingroup ONBOARDCLIENTMGR_API
     *
     * @brief onResponseEvent(..) is the function to register receiver for the BpOnboardClientReceiver .
     *
     * @details Notify UDS response
     * Managers created with TIDL can broadcast messages to other managers and apps.
     * When a specific event occurs in the Manager made with TIDL, if a manager wants to receive the event from the TIDL Manager,
     * manager can register events of interest to TIDL Manager with the registerreceiver function.
     * Then, when the event occurs, a specific event is delivered to the requested Manager through broadcast.
     * For example, if you want to know when the car has started, register the event through the registerreceiver function, and the car has started.
     * An event indicating that startup has occurred will be delivered to the manager who created the RegisterReceiver request.
     *
     * @param[in] receiver android::sp< OnboardClientReceiver > interface
     * @retval   error_t   If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual error_t registerReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp<IOnboardClientReceiver> &receiver) = 0;

    /**
     * @ingroup ONBOARDCLIENTMGR_API
     *
     * @brief onNotifyOBD2Event(..) is the function to unregister receiver for the BpOnboardClientReceiver .
     *
     * @details Wired connection detection notification event
     *
     * @param[in] receiver android::sp< OnboardClientReceiver > interface
     * @retval   error_t   If the receiver is unregistered successfully, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual error_t unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp<IOnboardClientReceiver> &receiver) = 0;

    /**
     * @ingroup ONBOARDCLIENTMGR_API
     *
     * @brief onResponseEvent(..) is the function to unregister receiver for the BpOnboardClientReceiver .
     *
     * @details Notify UDS response
     *
     * @param[in] receiver android::sp< OnboardClientReceiver > interface
     * @retval   error_t   If the receiver is unregistered successfully, return E_OK, otherwise, return E_ERROR.
     *
     */
    virtual error_t unregisterReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp<IOnboardClientReceiver> &receiver) = 0;

    // auto API start : wishtoUseAPI
    /**
     * @ingroup ONBOARDCLIENTMGR_API
     *
     * @brief API : send UDS request
     *
     * this funciton will be invoked when application or another services call this function through binder.
     *
     * @param[in] connectId  connectID
     * @param[in] udsRequest  UDS request
     * @retval   uint8_t     If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
     *
     * @cond UML
     * @startuml
     *     box "Application (Proxy)\n Other Services"
     *     participant App
     *     participant BpOnboardclientManagerService
     *     end box
     *     participant Binder
     *     box "Service (Native) - API"
     *     participant BnOnboardclientManagerService
     *     participant ServiceStub
     *     end box
     *
     *     == send UDS request ==
     *     App -> BpOnboardclientManagerService : sendUdsData(arguments)
     *     BpOnboardclientManagerService -> Binder : OnboardclientData >> remote()->onTransact(OP_REGISTER_SENDUDSDATA, Parcel)
     *     Binder -> BnOnboardclientManagerService : onTransact(uint32_t code, const Parcel& data,reply)\n OP_REGISTER_SENDUDSDATA, Parcel >> arguments
     *     BnOnboardclientManagerService -> ServiceStub : sendUdsData(arguments)
     *     activate ServiceStub
     *     BnOnboardclientManagerService <- ServiceStub : return E_OK
     *     deactivate ServiceStub
     *     Binder <- BnOnboardclientManagerService : reply->writeInt32
     *     BpOnboardclientManagerService <- Binder : reply.readInt32()
     *     App <- BpOnboardclientManagerService : return
     * @enduml
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManagerService::sendUdsData()_SRS"}
     * @SRS{TIDL-FR-001,Explanation for FR-001}
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManagerService::sendUdsData()_SRS"}
     * @endcond
     */
    virtual uint8_t sendUdsData(const uint16_t connectId, const android::sp<Buffer> udsRequest) = 0;
    // RECEIVER API start
    /**
     * @ingroup ONBOARDCLIENT_RECEIVER_TEST_API
     *
     * @brief API : Wired connection detection notification event
     *
     * this funciton will be invoked when application or another services call this function through binder.
     *
     *
     * @retval   error_t     If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
     *
     * @cond UML
     * @startuml
     *     box "Application (Proxy)\n Other Services"
     *     participant App
     *     participant BpOnboardclientManagerService
     *     end box
     *     participant Binder
     *     box "Service (Native) - API"
     *     participant BnOnboardclientManagerService
     *     participant ServiceStub
     *     end box
     *
     *     == Wired connection detection notification event ==
     *     App -> BpOnboardclientManagerService : onNotifyOBD2Event(arguments)
     *     BpOnboardclientManagerService -> Binder : OnboardclientData >> remote()->onTransact(OP_REGISTER_ONNOTIFYOBD2EVENT, Parcel)
     *     Binder -> BnOnboardclientManagerService : onTransact(uint32_t code, const Parcel& data,reply)\n OP_REGISTER_ONNOTIFYOBD2EVENT, Parcel >> arguments
     *     BnOnboardclientManagerService -> ServiceStub : onNotifyOBD2Event(arguments)
     *     activate ServiceStub
     *     BnOnboardclientManagerService <- ServiceStub : return E_OK
     *     deactivate ServiceStub
     *     Binder <- BnOnboardclientManagerService : reply->writeInt32
     *     BpOnboardclientManagerService <- Binder : reply.readInt32()
     *     App <- BpOnboardclientManagerService : return
     * @enduml
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManagerService::onNotifyOBD2Event()_SRS"}
     * @SRS{TIDL-FR-001,Explanation for FR-001}
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManagerService::onNotifyOBD2Event()_SRS"}
     * @endcond
     */
    virtual error_t testOnNotifyOBD2Event() = 0;
    /**
     * @ingroup ONBOARDCLIENT_RECEIVER_TEST_API
     *
     * @brief API : Notify UDS response
     *
     * this funciton will be invoked when application or another services call this function through binder.
     *
     * @param[in] message  Response Information
     *
     * @retval   error_t     If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
     *
     * @cond UML
     * @startuml
     *     box "Application (Proxy)\n Other Services"
     *     participant App
     *     participant BpOnboardclientManagerService
     *     end box
     *     participant Binder
     *     box "Service (Native) - API"
     *     participant BnOnboardclientManagerService
     *     participant ServiceStub
     *     end box
     *
     *     == Notify UDS response ==
     *     App -> BpOnboardclientManagerService : onResponseEvent(arguments)
     *     BpOnboardclientManagerService -> Binder : OnboardclientData >> remote()->onTransact(OP_REGISTER_ONRESPONSEEVENT, Parcel)
     *     Binder -> BnOnboardclientManagerService : onTransact(uint32_t code, const Parcel& data,reply)\n OP_REGISTER_ONRESPONSEEVENT, Parcel >> arguments
     *     BnOnboardclientManagerService -> ServiceStub : onResponseEvent(arguments)
     *     activate ServiceStub
     *     BnOnboardclientManagerService <- ServiceStub : return E_OK
     *     deactivate ServiceStub
     *     Binder <- BnOnboardclientManagerService : reply->writeInt32
     *     BpOnboardclientManagerService <- Binder : reply.readInt32()
     *     App <- BpOnboardclientManagerService : return
     * @enduml
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManagerService::onResponseEvent()_SRS"}
     * @SRS{TIDL-FR-001,Explanation for FR-001}
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManagerService::onResponseEvent()_SRS"}
     * @endcond
     */
    virtual error_t testOnResponseEvent(const android::sp<OBCResponseEventInfo> message) = 0;
    // auto RECEIVER API end for test : receiver_function
    virtual error_t connect(const android::sp<OBCTransportInfo> transportInfo, const std::string appName, android::sp<OBCConnectInfo> &connectInfo) = 0;
    virtual uint8_t disconnect(const uint16_t connectId) = 0;
    virtual error_t sendUdsResponseData(const android::sp<Buffer> udsResponse) = 0;
    virtual error_t sendAckToMcu(const uint8_t reason) = 0;
    virtual uint16_t getMaxConnectCount(const uint8_t protocolType) = 0;
    virtual uint8_t setAppPriority(const std::string appName, const uint8_t priority) = 0;
    virtual uint8_t setMaxExceeded(const uint8_t queueType, const uint32_t queueSize) = 0;
    // @CGA_VARIANT_START{"IOnboardclientManagerService:IOnboardclientManagerService()"}
    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"IOnboardclientManagerService:IOnboardclientManagerService()"}
};

class BnOnboardclientManagerService : public android::BnInterface<IOnboardclientManagerService>
{

public:
    /**
     * @brief onTransact function for the BnOnboardclientManagerService.
     *
     * @param[in] code operation code in binder
     * @param[in] data data parcel in binder
     * @param[in,out] reply reply packet in binder
     * @param[in] flags flags
     * @retval   status_t It returns the status of onTransact function.
     *
     */
    virtual android::status_t onTransact(const uint32_t code, const android::Parcel &data, android::Parcel *const reply, const uint32_t flags);
};

#endif /**IONBOARDCLIENT_MANAGER_SERVICE_H*/
