
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
 *
 * This is the Onboardclient Input Manager with msg handler and connector of related tiger managers.
 *
 * @cond UML
 * @startuml
 *    class OnboardclientInputManager
 *    class OnboardclientHandler
 *    class sl::Handler
 *    class android::RefBase
 *    class OnboardclientInputMgrTimer
 *    class TimerTimeoutHandler
 *    class ServiceDeathRecipient
 *    class android::IBinder::DeathRecipient
 *    class OnboardclientManagerService
 *
 *    OnboardclientInputManager *--  OnboardclientManagerService :  >
 *     OnboardclientInputManager --> android::RefBase
 *     OnboardclientInputManager *--  OnboardclientHandler : mMyHandler >
 *     OnboardclientHandler -->  sl::Handler
 *     OnboardclientInputManager *--  OnboardclientInputMgrTimer : mOnboardclientInputMgrTimer >
 *     OnboardclientInputMgrTimer --> TimerTimeoutHandler
 *     OnboardclientInputManager *--  ServiceDeathRecipient : mServiceDeathRecipient >
 *     ServiceDeathRecipient -->  android::IBinder::DeathRecipient
 *    class BnSystemPostReceiver
 *    class SystemPostReceiver
 *     OnboardclientInputManager *-up-  SystemPostReceiver : mSystemPostReceiver >
 *     SystemPostReceiver -up-> BnSystemPostReceiver : appmgr
 * @enduml
 * @endcond
 */

#ifndef SERVICELAYER_ONBOARDCLIENT_INPUT_MANAGER_H
#define SERVICELAYER_ONBOARDCLIENT_INPUT_MANAGER_H

/**
 * @MISRA{MISRA C++-2008 Rule 16-2-1,"This is intended design"}
 */

#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include <utils/Handler.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/external/mindroid/lang/String.h>
#include <utils/Mutex.h>

/* auto __ appmgr Inheritance CGA start-------------------------------------------------*/
#include <services/ApplicationManagerService/ISystemPostReceiver.h>
#include <services/ApplicationManagerService/IApplicationManagerService.h>
#include <services/ApplicationManagerService/IApplicationManagerServiceType.h>
/* auto __ appmgr Inheritance CGA end-------------------------------------------------*/

// CommMgr
#include "services/CommunicationManagerService/ICommunicationReceiver.h"
#include "services/CommunicationManagerService/ICommunicationManagerService.h"
#include "services/CommunicationManagerService/ICommunicationManagerServiceType.h"
#include "services/CommunicationManagerService/CommunicationData.h"
#include <services/CommunicationManagerService/CM_Protocol.h>

// DiagMgr
#include <services/DiagManagerService/IDiagManagerService.h>
#include <services/DiagManagerService/IDiagManagerServiceType.h>
#include <services/DiagManagerService/IDiagManagerReceiver.h>
#include <services/DiagManagerService/DiagDtcItemList.h>
#include <services/DiagManagerService/DiagType.h>
#include <services/DiagManagerService/OEM_DiagData_Config.h>

#include <services/OnboardclientManagerService/OnboardclientCommand.h>
#include "Common_def.h"
// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
namespace OBC
{
    class OnboardclientManagerService;

    /**
     * @brief OnboardclientInputManager is Receiver class with msgHandler and relations with another Tiger Manager.
     *
     * @cond UML
     * This is process how to run operations in OnboardclientInputManager class.
     * @startuml
     *    OnboardclientManagerService -> OnboardclientInputManager : init()
     *    OnboardclientManagerService -> OnboardclientInputManager : onStart()
     *    OnboardclientInputManager -> ServiceDeathRecipient : mServiceDeathRecipient - contructor Death
     *    OnboardclientInputManager -> OnboardclientHandler : mMyHandler - looper of MSG
     *    loop each Modules (ex. app , audio , HMI , vif Manager) until all Modules are alive
     *      OnboardclientInputManager -> OnboardclientInputManager : connectToAppMgr()
     *      alt each module  manager is alive
     *        OnboardclientInputManager <-> binder : register Death Recipient / ex. mAppManager
     *      else
     *        OnboardclientInputManager -> OnboardclientHandler : handleMessage   ex. OBC_INPUT_HANDLER::MSG_CONNECT_TO_APPMGR
     *      end
     *    end
     *    OnboardclientInputManager -> OnboardclientInputMgrTimer : mOnboardclientInputMgrTimer - timer
     *    OnboardclientInputManager -> OnboardclientInputMgrTimer : watchDog start
     *    OtherService -> OnboardclientHandler : handleMessage - ex. DID_WORK_FOR_DEFINE_0
     *    alt death of related service module (ex. app, audio, HMI, vif etc)
     *      OtherService -> binder : Death
     *      binder -> OnboardclientInputManager : onServiceBinderDied
     *      loop Died Module (ex. app , audio , HMI , vif Manager) until Died Module is alive
     *        OnboardclientInputManager -> OnboardclientInputManager : ex. connectToAPPMg()
     *        alt each module manager is alive
     *          OnboardclientInputManager <-> binder : register Death Recipient / ex. mAppManager
     *        else
     *          OnboardclientInputManager -> OnboardclientHandler : handleMessage  ex. OBC_INPUT_HANDLER::MSG_CONNECT_TO_APPMGR
     *        end
     *      end
     *    end
     * @enduml
     * @endcond
     **/
    class OnboardclientInputManager : public android::RefBase
    {
    public:
        class OnboardclientHandler : public sl::Handler
        {
        public:
            /**
             * @brief This is the constructor function for the OnboardclientHandler.
             *
             * @param[in] looper     android SLLooper
             * @param[in] outer     OnboardclientInputManager
             * @retval   void
             *
             */
            OnboardclientHandler(android::sp<sl::SLLooper> &looper, OnboardclientInputManager &outer) noexcept;
            /**
             * @brief This is the handle function of the input manager.
             *
             * @param[in] msg     Message for handler
             * @retval   void
             *
             * @CGA_VARIANT_START{"DOXYGEN:OnboardclientInputManager:handleMessage()"}
             * @details
             * @CGA_VARIANT___END{"DOXYGEN:OnboardclientInputManager:handleMessage()"}
             */
            static OnboardclientHandler *getInstance();
            virtual void handleMessage(const android::sp<sl::Message> &handlemsg);

        private:
            OnboardclientInputManager &mOnboardclientInputMgr;
            static OnboardclientHandler *mOnboardclientHandler;
        };

        class OnboardclientInputMgrTimer : public TimerTimeoutHandler
        {
        public:
            /* Timer ID */
            static constexpr int32_t ONBOARDCLIENT_WATCHDOG_TIMER{1};

            /**
             * @brief This is the constructor function for the OnboardclientInputMgrTimer.
             *
             * @param[in] handler     android OnboardclientHandler
             * @retval   void
             *
             */
            explicit OnboardclientInputMgrTimer(const android::sp<OnboardclientInputManager::OnboardclientHandler> handler) : mHandler(handler) {}

            /**
             * @brief This is the virtual handle function for the OnboardclientInputMgrTimer.
             *
             * @param[in] timer_id     timer id
             * @retval   void
             *
             */
            void handlerFunction(const int32_t timerId) override;

        private:
            android::sp<OnboardclientInputManager::OnboardclientHandler> mHandler;

            // @CGA_VARIANT_START{"class_OnboardclientInputManager_H:OnboardclientInputMgrTimer"}
            /*
             * Write your own code
             */
            // @CGA_VARIANT___END{"class_OnboardclientInputManager_H:OnboardclientInputMgrTimer"}
        };

        /**
         * @brief When the services are died, it is where you put the code to reconnect.
         *
         * @param[in] who     who generates Dead state
         * @retval   void
         *
         */
        void onServiceBinderDied(const android::wp<android::IBinder> &who) noexcept;

        class ServiceDeathRecipient : public android::IBinder::DeathRecipient
        {
        public:
            /**
             * @brief This is the constructor function for the ServiceDeathRecipient.
             *
             * @param[in] parent     OnboardclientInputManager
             * @retval   void
             *
             */
            ServiceDeathRecipient(OnboardclientInputManager &parent) noexcept : mParent(parent) {}

            /**
             * @brief This is the virtual destructor function for the ServiceDeathRecipient.
             *
             * @retval   void
             *
             */
            virtual ~ServiceDeathRecipient() = default;
            ServiceDeathRecipient(ServiceDeathRecipient const &) = delete;
            ServiceDeathRecipient &operator=(ServiceDeathRecipient const &) = delete;
            ServiceDeathRecipient(ServiceDeathRecipient &&) = delete;
            ServiceDeathRecipient &operator=(ServiceDeathRecipient &&) = delete;
            /**
             * @brief This is the binderdied virtual function for the ServiceDeathRecipient.
             *
             * @param[in] who     IBinder
             * @retval   void
             *
             */
            virtual void binderDied(const android::wp<android::IBinder> &who) noexcept
            {
                mParent.onServiceBinderDied(who);
            }

        private:
            OnboardclientInputManager &mParent;

            // @CGA_VARIANT_START{"class_ServiceDeathRecipient:ServiceDeathRecipient"}
            /*
             * Write your own code
             */
            // @CGA_VARIANT___END{"class_ServiceDeathRecipient:ServiceDeathRecipient"}
        };

        /* auto __ appmgr Inheritance CGA start-------------------------------------------------*/

        /* auto __ appmgr Inheritance CGA end-------------------------------------------------*/

        // CommMgr

    public:
        /**
         * @brief It is a function for the constructor of the input manager class.
         *
         * @param[in] onboardclientMgrService android::sp<OnboardclientManagerService>
         * @retval   void
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientInputManager::OnboardclientInputManager_MISRA"}
         * @MISRA{MISRA C++-2008 Rule 2-10-2,This is intended design}
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientInputManager::OnboardclientInputManager_MISRA"}
         *
         */
        OnboardclientInputManager(const android::sp<OnboardclientManagerService> onboardclientMgrService) noexcept;
        /**
         * @brief It is a function for the destructor of the input manager class.
         *
         * @retval   void
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientInputManager:~OnboardclientInputManager()"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientInputManager:~OnboardclientInputManager()"}
         */
        virtual ~OnboardclientInputManager() noexcept;
        OnboardclientInputManager(const OnboardclientInputManager &other) = delete;
        OnboardclientInputManager &operator=(const OnboardclientInputManager &other) = delete;
        OnboardclientInputManager(OnboardclientInputManager &&) = delete;
        OnboardclientInputManager &operator=(OnboardclientInputManager &&) = delete;
        /**
         * @brief It is a function for the initialization of the input manager.
         *
         * @retval   void
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientInputManager:Init()"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientInputManager:Init()"}
         */
        error_t init();
        void resetWatchDog(void);

    private:
        /* auto __ appmgr Inheritance CGA start-------------------------------------------------*/
        /**
         * @brief When appmgr service is died, it is where you put the code to reconnect.
         *
         * @retval   void
         *
         * @CGA_VARIANT_START{"DOXYGEN:OnboardclientInputManager:connectToAppMgr()"}
         * @details
         * @CGA_VARIANT___END{"DOXYGEN:OnboardclientInputManager:connectToAppMgr()"}
         */
        void connectToAppMgr();
        /* auto __ appmgr Inheritance CGA end-------------------------------------------------*/

    private:
        android::sp<OnboardclientManagerService> mOnboardclientMgrService;

        mutable android::Mutex mBinderDiedLock;

        /* auto __ appmgr Inheritance CGA start-------------------------------------------------*/
        android::sp<IApplicationManagerService> mAppManager;
        android::sp<ISystemPostReceiver> mSystemPostReceiver;
        /* auto __ appmgr Inheritance CGA end-------------------------------------------------*/

        OnboardclientInputMgrTimer *mOnboardclientInputMgrTimer;

        android::sp<ServiceDeathRecipient> mServiceDeathRecipient;

        // @CGA_VARIANT_START{"class_OnboardclientInputManager"}
        // CommMgr instance
        android::sp<ICommunicationManagerService> mCommMgr;
        android::sp<ICommunicationReceiver> mCommReceiver;
        android::sp<OnboardclientHandler> mMyHandler;
        Timer *m_WatcdogTimer;
        // @CGA_VARIANT___END{"class_OnboardclientInputManager"}
    };
};
#endif /* SERVICELAYER_ONBOARDCLIENT_INPUT_MANAGER_H */
