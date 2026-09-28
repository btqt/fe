
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
 *
 *
 *  This is the Onboardclient Manager Service. This class includes all native functions.
 *  - APIs ( native codes )
 *
 * @cond UML
 * Basic Process about API using binder
 * @startuml
 *     box "Application (Proxy)"
 *     participant App
 *     participant BpOnboardclientManagerService
 *     end box
 *     participant Binder
 *     box "Service (Native)"
 *     participant BnOnboardclientManagerService
 *     participant ServiceStub
 *     end box
 *
 *     App -> BpOnboardclientManagerService : API(arguments)
 *     BpOnboardclientManagerService -> Binder : OnboardclientData >> remote()->onTransact(OP_REGISTER_API, Parcel)
 *     Binder -> BnOnboardclientManagerService : onTransact(uint32_t code, const Parcel& data,reply)\n OP_REGISTER_API, Parcel >> arguments
 *     BnOnboardclientManagerService -> ServiceStub : API(arguments)
 *     activate ServiceStub
 *     BnOnboardclientManagerService <- ServiceStub : return E_OK
 *     deactivate ServiceStub
 *     Binder <- BnOnboardclientManagerService : reply->writeInt32
 *     BpOnboardclientManagerService <- Binder : reply.readInt32()
 *     App <- BpOnboardclientManagerService : return
 * @enduml
 * @endcond
 *
 */

#ifndef SERVICELAYER_ONBOARDCLIENTNOSTIC_MANAGER_SERVICE_H
#define SERVICELAYER_ONBOARDCLIENTNOSTIC_MANAGER_SERVICE_H

#include <map>
#include <list>
#include <vector>
#include <tuple>
#include <algorithm>

#include <Typedef.h>
#include <corebase/SystemService.h>

#include "OnboardclientInputManager.h"

#include "services/OnboardclientManagerService/OnboardclientCommand.h"
// #include "../include/IOnboardclientManagerReceiver.h"
#include "services/OnboardclientManagerService/IOnboardclientManagerService.h"
#include "services/OnboardclientManagerService/IOnboardClientReceiver.h"
#include "Common_def.h"

#ifndef MODULE_ONBOARDCLIENT_MGR
#define MODULE_ONBOARDCLIENT_MGR 100
#endif

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

/**
 * @brief OnboardclientManagerService is class to serive APIs for application and another services.
 *
 * @cond UML
 * Basic Process about API using binder
 * @startuml
 *     box "Application (Proxy)"
 *     participant App
 *     participant BpOnboardclientManagerService
 *     end box
 *     participant Binder
 *     box "Service (Native)"
 *     participant BnOnboardclientManagerService
 *     participant ServiceStub
 *     end box
 *
 *     App -> BpOnboardclientManagerService : API(arguments)
 *     BpOnboardclientManagerService -> Binder : OnboardclientData >> remote()->onTransact(OP_REGISTER_API, Parcel)
 *     Binder -> BnOnboardclientManagerService : onTransact(uint32_t code, const Parcel& data,reply)\n OP_REGISTER_API, Parcel >> arguments
 *     BnOnboardclientManagerService -> ServiceStub : API(arguments)
 *     activate ServiceStub
 *     BnOnboardclientManagerService <- ServiceStub : return E_OK
 *     deactivate ServiceStub
 *     Binder <- BnOnboardclientManagerService : reply->writeInt32
 *     BpOnboardclientManagerService <- Binder : reply.readInt32()
 *     App <- BpOnboardclientManagerService : return
 * @enduml
 * @endcond
 */
namespace OBC
{
    class OnboardclientManagerService : public SystemService
    {
    public:
        /**
         * @brief It is the constructor function of OnboardclientManagerService.
         *
         * @retval void
         *
         */
        OnboardclientManagerService();
        /**
         * @brief It is the destructor function of OnboardclientManagerService.
         *
         * @retval void
         *
         */
        virtual ~OnboardclientManagerService() = default;
        OnboardclientManagerService(const OnboardclientManagerService &other) = delete;
        OnboardclientManagerService &operator=(const OnboardclientManagerService &other) = delete;
        OnboardclientManagerService(OnboardclientManagerService &&) = delete;
        OnboardclientManagerService &operator=(OnboardclientManagerService &&) = delete;
        static OnboardclientManagerService *instance();

        /**
         * @brief This function gets service name.
         *
         * @retval   service_layer.OnboardclientManagerService service name
         *
         */
        static const char_t *getServiceName() noexcept { return "service_layer.OnboardclientManagerService"; }

        /**
         * @brief This function gets module ID.
         *
         * @retval   MODULE_ONBOARDCLIENT module ID
         *
         */
        virtual uint8_t getModuleID() noexcept;

        /**
         * @brief It is the function to onInit for the OnboardclientManagerService.
         *
         * @retval   bool
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManagerService:onInit"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManagerService:onInit"}
         */
        virtual bool onInit();
        /**
         * @brief It is the function to instantiate for the OnboardclientManagerService.
         *
         * @retval   void
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManagerService:instantiate"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManagerService:instantiate"}
         */
        virtual void instantiate();
        /**
         * @brief It is the function to onStart for the OnboardclientManagerService.
         *
         * @retval   error_t   E_OK is returned if this function works properly, otherwise return E_ERROR.
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManagerService:onStart"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManagerService:onStart"}
         */
        virtual error_t onStart();
        /**
         * @brief It is the function to onStop for the OnboardclientManagerService.
         *
         * @retval   error_t   E_OK is returned if this function works properly, otherwise return E_ERROR.
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManagerService:onStop"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManagerService:onStop"}
         */
        virtual error_t onStop();
        /**
         * @brief It is the function to dump for the OnboardclientManagerService.
         *
         * @param[in] Data   data for dump
         * @retval   error_t E_OK is returned if this function works properly, otherwise return E_ERROR.
         *
         */
        virtual error_t dump(LogOutput &out) noexcept;

        /**
         * @brief registerReceiverOnboardClientReceiverOnNotifyOBD2Event is the function to register receiver for the BnOnboardClientReceiver .
         *
         * @details Wired connection detection notification event
         *
         * @param[in] receiver android::sp< IOnboardClientReceiver > interface
         * @retval   error_t   If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onNotifyOBD2Event:registerReceiverOnboardClientReceiverOnNotifyOBD2Event()"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onNotifyOBD2Event:registerReceiverOnboardClientReceiverOnNotifyOBD2Event()"}
         */
        error_t registerReceiverOnboardClientReceiverOnNotifyOBD2Event(const android::sp<IOnboardClientReceiver> &receiver);
        /**
         * @brief registerReceiverOnboardClientReceiverOnResponseEvent is the function to register receiver for the BnOnboardClientReceiver .
         *
         * @details Notify UDS response
         *
         * @param[in] receiver android::sp< IOnboardClientReceiver > interface
         * @retval   error_t   If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onResponseEvent:registerReceiverOnboardClientReceiverOnResponseEvent()"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onResponseEvent:registerReceiverOnboardClientReceiverOnResponseEvent()"}
         */
        error_t registerReceiverOnboardClientReceiverOnResponseEvent(const android::sp<IOnboardClientReceiver> &receiver);

        /**
         * @brief unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event is the function to unregister receiver for the BnOnboardClientReceiver .
         *
         * @details Wired connection detection notification event
         *
         * @param[in] receiver android::sp< IOnboardClientReceiver > interface
         * @retval   error_t   If the receiver is unregistered successfully, return E_OK, otherwise, return E_ERROR.
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onNotifyOBD2Event:unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event()"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onNotifyOBD2Event:unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event()"}
         */
        error_t unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(const android::sp<IOnboardClientReceiver> &receiver);
        /**
         * @brief unregisterReceiverOnboardClientReceiverOnResponseEvent is the function to unregister receiver for the BnOnboardClientReceiver .
         *
         * @details Notify UDS response
         *
         * @param[in] receiver android::sp< IOnboardClientReceiver > interface
         * @retval   error_t   If the receiver is unregistered successfully, return E_OK, otherwise, return E_ERROR.
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onResponseEvent:unregisterReceiverOnboardClientReceiverOnResponseEvent()"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onResponseEvent:unregisterReceiverOnboardClientReceiverOnResponseEvent()"}
         */
        error_t unregisterReceiverOnboardClientReceiverOnResponseEvent(const android::sp<IOnboardClientReceiver> &receiver);

        /**
         * @brief queryReceiverByOnboardClientReceiverOnNotifyOBD2Event is the function to OnboardClientReceiver receiver (OnboardClientReceiver::onNotifyOBD2Event)
         *
         * @retval   error_t   E_OK is returned if this function works properly, otherwise return E_ERROR.
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManagerService:queryReceiverByOnboardClientReceiverOnNotifyOBD2Event"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManagerService:queryReceiverByOnboardClientReceiverOnNotifyOBD2Event"}
         */
        error_t queryReceiverByOnboardClientReceiverOnNotifyOBD2Event( //  Parm:0
        );
        /**
         * @brief queryReceiverByOnboardClientReceiverOnResponseEvent is the function to OnboardClientReceiver receiver (OnboardClientReceiver::onResponseEvent)
         *
         * @param[in] message Response Information ( android::sp< OBCResponseEventInfo >& )
         * @retval   error_t   E_OK is returned if this function works properly, otherwise return E_ERROR.
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManagerService:queryReceiverByOnboardClientReceiverOnResponseEvent"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManagerService:queryReceiverByOnboardClientReceiverOnResponseEvent"}
         */
        error_t queryReceiverByOnboardClientReceiverOnResponseEvent( //  Parm:0
            const android::sp<OBCResponseEventInfo> message          //  Parm:1
        );

        /**
         * @brief It is the function to check registered receivers for the OnboardclientManagerService.
         *
         * @param[in] id application id
         * @retval   bool
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManagerService:isApplicationExecuted"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManagerService:isApplicationExecuted"}
         */
        bool isApplicationExecuted(const appid_t id) noexcept;

        /**
         * @brief It is the function to reconnect registered receivers for the OnboardclientManagerService.
         *
         * @param[in] who   who sends death state.
         * @retval   void
         *
         */
        void onReceiverBinderDied(const android::wp<android::IBinder> &who);

        // auto API start : wishtoUseAPI
        /**
         *
         * @brief API : send UDS request
         *
         * this funciton will be invoked when application or another services call this function through binder.
         *
         * @param[in] connectId  connectID
         * @param[in] udsRequest  UDS request
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
        uint8_t sendUdsData(const uint16_t connectId, const android::sp<Buffer> udsRequest);
        // auto API end : wishtoUseAPI
        // @CGA_VARIANT_START{"xxxManagerService.h:__CUSTOM_API__CODE__:variant"}
        // @CGA_VARIANT___END{"xxxManagerService.h:__CUSTOM_API__CODE__:variant"}

        // auto RECEIVER API start
        // RECEIVER API (tesetCallBack)
        /**
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
        error_t testOnNotifyOBD2Event();
        /**
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
        error_t testOnResponseEvent(const android::sp<OBCResponseEventInfo> message);
        error_t connect(const android::sp<OBCTransportInfo> transportInfo, const std::string appName, android::sp<OBCConnectInfo> &connectInfo);
        // auto RECEIVER API end
        uint8_t disconnect(const uint16_t connectId);
        static error_t sendUdsResponseData(const android::sp<Buffer> udsResponse);
        error_t sendAckToMcu(const uint8_t reason);
        static uint16_t getMaxConnectCount(const uint8_t protocolType);
        static uint8_t setAppPriority(const std::string appName, const uint8_t priority);
        uint8_t setMaxExceeded(const uint8_t queueType, const uint32_t queueSize);
        void setBootComplete(const bool isCompleted) noexcept;

    private:
        class ReceiverDeathRecipient : public android::IBinder::DeathRecipient
        {
        public:
            /**
             * @brief This is the constructor of ReceiverDeathRecipient
             *
             * @param[in] parent     OnboardclientManagerService
             * @retval   void
             *
             */
            ReceiverDeathRecipient(OnboardclientManagerService &parent) noexcept : mParent(parent) {}

            /**
             * @brief This is the destructor of ReceiverDeathRecipient
             *
             * @retval   void
             *
             */
            virtual ~ReceiverDeathRecipient() = default;
            ReceiverDeathRecipient(ReceiverDeathRecipient const &) = delete;
            ReceiverDeathRecipient &operator=(ReceiverDeathRecipient const &) = delete;
            ReceiverDeathRecipient(ReceiverDeathRecipient &&) = delete;
            ReceiverDeathRecipient &operator=(ReceiverDeathRecipient &&) = delete;
            /**
             * @brief This function is called when binder is died.
             *
             * @param[in] who     IBinder
             * @retval   void
             *
             */
            virtual void binderDied(const android::wp<android::IBinder> &who) noexcept
            {
                mParent.onReceiverBinderDied(who);
            }

        private:
            OnboardclientManagerService &mParent;
        };

    private:
        class ServiceStub : public BnOnboardclientManagerService
        {
        public:
            ServiceStub(OnboardclientManagerService &parent) noexcept : mParent(parent) {}

            /**
             * @brief It is the function to queryReceiverByOnboardClientReceiverOnNotifyOBD2Event for service stub in the OnboardclientManagerService.
             *
             * @retval   error_t   E_OK is returned if this function works properly, otherwise return E_ERROR.
             *
             */
            error_t queryReceiverByOnboardClientReceiverOnNotifyOBD2Event();
            /**
             * @brief It is the function to queryReceiverByOnboardClientReceiverOnResponseEvent for service stub in the OnboardclientManagerService.
             *
             * @param[in] message Response Information ( android::sp< OBCResponseEventInfo >& )
             * @retval   error_t   E_OK is returned if this function works properly, otherwise return E_ERROR.
             *
             */
            error_t queryReceiverByOnboardClientReceiverOnResponseEvent(const android::sp<OBCResponseEventInfo> message);

            /**
             * @brief It is the function to register receiver for service stub in the OnboardclientManagerService.
             *
             * @param[in] receiver android::sp< IOnboardClientReceiver > interface
             * @retval   error_t   E_OK is returned if this function works properly, otherwise return E_ERROR.
             *
             */
            error_t registerReceiverOnboardClientReceiverOnNotifyOBD2Event(const android::sp<IOnboardClientReceiver> &receiver) final;
            /**
             * @brief It is the function to register receiver for service stub in the OnboardclientManagerService.
             *
             * @param[in] receiver android::sp< IOnboardClientReceiver > interface
             * @retval   error_t   E_OK is returned if this function works properly, otherwise return E_ERROR.
             *
             */
            error_t registerReceiverOnboardClientReceiverOnResponseEvent(const android::sp<IOnboardClientReceiver> &receiver) final;

            /**
             * @brief It is the function to unregister receiver for service stub in the OnboardclientManagerService.
             *
             * @param[in] receiver android::sp< IOnboardClientReceiver > interface
             * @retval   error_t   E_OK is returned if this function works properly, otherwise return E_ERROR.
             *
             */
            error_t unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(const android::sp<IOnboardClientReceiver> &receiver) final;
            /**
             * @brief It is the function to unregister receiver for service stub in the OnboardclientManagerService.
             *
             * @param[in] receiver android::sp< IOnboardClientReceiver > interface
             * @retval   error_t   E_OK is returned if this function works properly, otherwise return E_ERROR.
             *
             */
            error_t unregisterReceiverOnboardClientReceiverOnResponseEvent(const android::sp<IOnboardClientReceiver> &receiver) final;

            // auto API start : wishtoUseAPI
            /**
             * @brief API : This is stub code. Implementation will be in parent class with the function name.
             *
             * @param[in] connectId  connectID
             * @param[in] udsRequest  UDS request
             * @retval   uint8_t     If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
             */
            uint8_t sendUdsData(const uint16_t connectId, const android::sp<Buffer> udsRequest) final;
            // auto API end : wishtoUseAPI
            // @CGA_VARIANT_START{"xxxManagerService.h_ServiceStub:__CUSTOM_API__CODE__:variant"}
            // @CGA_VARIANT___END{"xxxManagerService.h_ServiceStub:__CUSTOM_API__CODE__:variant"}

            // RECEIVER API start
            /**
             * @brief API : This is stub code. Implementation will be in parent class with the function name.
             *
             *
             * @retval   error_t     If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
             */
            error_t testOnNotifyOBD2Event() final;
            /**
             * @brief API : This is stub code. Implementation will be in parent class with the function name.
             *
             * @param[in] message  Response Information
             *
             * @retval   error_t     If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
             */
            error_t testOnResponseEvent(const android::sp<OBCResponseEventInfo> message) final;
            error_t connect(const android::sp<OBCTransportInfo> transportInfo, const std::string appName, android::sp<OBCConnectInfo> &connectInfo) final;
            uint8_t disconnect(const uint16_t connectId) final;
            error_t sendUdsResponseData(const android::sp<Buffer> udsResponse) final;
            error_t sendAckToMcu(const uint8_t reason) final;
            uint16_t getMaxConnectCount(const uint8_t protocolType) final;
            uint8_t setAppPriority(const std::string appName, const uint8_t priority) final;
            uint8_t setMaxExceeded(const uint8_t queueType, const uint32_t queueSize) final;
            // RECEIVER API end

        private:
            bool check_condition_api_ready(void) const;

        private:
            OnboardclientManagerService &mParent;
        };

    private:
        /**
         * @MISRA{MISRA C++-2008 Rule 8-0-1,This is intended design}
         */
        uint32_t MAX_UDS_REQ_SIZE; // 64kb
        static OnboardclientManagerService *mOnboardclientManagerService;
        android::sp<OnboardclientInputManager> mOnboardclientInputMgr;
        bool mBootCompleted;
        std::list<android::sp<IOnboardClientReceiver>> mlReceiversOnboardClientReceiverOnNotifyOBD2Event;
        std::list<android::sp<IOnboardClientReceiver>> mlReceiversOnboardClientReceiverOnResponseEvent;

        // int32_t mCurrentPowerState;
        mutable Mutex mReceiverLock;

        android::sp<ReceiverDeathRecipient> mReceiverDeathRecipient;
        android::sp<sl::SLLooper> mOnboardclientRxHandlerLooper;
        android::sp<sl::Handler> mOnboardclientRxHandler;
        android::sp<sl::SLLooper> mOnboardclientTxHandlerLooper;
        android::sp<sl::Handler> mOnboardclientTxHandler;
        // @CGA_VARIANT_START{"OnboardclientManagerService:__YOUR_CODE__:variant"}
        /*
         * Write your own additional code
         * add your #include
         * add your global declare the function and variables
         * add your function
         */
        // @CGA_VARIANT___END{"OnboardclientManagerService:__YOUR_CODE__:variant"}
    };
};
#endif // SERVICELAYER_ONBOARDCLIENTNOSTIC_MANAGER_SERVICE_H
