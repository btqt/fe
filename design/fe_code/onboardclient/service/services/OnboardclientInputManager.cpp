
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
 * @MISRA{MISRA C++-2008 Rule 16-2-1,"This is intended design"}
 */
#define LOG_TAG "OnboardclientInputManager"

#include <Log.h>

#include <binder/IServiceManager.h>

/* auto __ appmgr Inheritance CGA start-------------------------------------------------*/
#include <services/ApplicationManagerService/ApplicationManager.h>
#include <services/ApplicationManagerService/IApplicationManagerServiceType.h>
/* auto __ appmgr Inheritance CGA end-------------------------------------------------*/

#include <utils/watchdog/watchdog_client.h>

#include "OnboardclientInputManager.h"
#include "OnboardclientManagerService.h"
#include "CommunicationManagerAdapter.h"
#include "SystemPostReceiver.h"
#include "DiagManagerAdapter.h"
namespace OBC
{
    // @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
    /*
     * Write your own additional code
     * add your #include
     * add your global declare the function and variables
     * add your function
     */
    // @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

    OnboardclientInputManager::OnboardclientInputManager(const android::sp<OnboardclientManagerService> onboardclientMgrService) noexcept
        : mOnboardclientInputMgrTimer(nullptr), m_WatcdogTimer(nullptr)
    {
        /* auto __ appmgr Inheritance CGA start-------------------------------------------------*/
        mAppManager = nullptr;
        mSystemPostReceiver = nullptr;
        /* auto __ appmgr Inheritance CGA end-------------------------------------------------*/
        mOnboardclientMgrService = onboardclientMgrService;

        // @CGA_VARIANT_START{"OnboardclientInputManager:OnboardclientInputManager()"}
        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"OnboardclientInputManager:OnboardclientInputManager()"}
    }

    OnboardclientInputManager::~OnboardclientInputManager() noexcept
    {
        if (mOnboardclientInputMgrTimer != nullptr)
        {
            delete mOnboardclientInputMgrTimer;
        }

        if (m_WatcdogTimer != nullptr)
        {
            delete m_WatcdogTimer;
        }

        // @CGA_VARIANT_START{"OnboardclientInputManager:~OnboardclientInputManager()"}
        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"OnboardclientInputManager:~OnboardclientInputManager()"}
    }

    error_t OnboardclientInputManager::init()
    {
        constexpr error_t tidlResult{E_OK};
        mServiceDeathRecipient = new ServiceDeathRecipient(*this);

        mMyHandler = new OnboardclientHandler(mOnboardclientMgrService->looper(), *this);

        /* auto __ appmgr Inheritance CGA start-------------------------------------------------*/
        connectToAppMgr();
        /* auto __ appmgr Inheritance CGA end-------------------------------------------------*/

        mOnboardclientInputMgrTimer = new OnboardclientInputMgrTimer(mMyHandler);
        m_WatcdogTimer = new Timer(mOnboardclientInputMgrTimer, OnboardclientInputMgrTimer::ONBOARDCLIENT_WATCHDOG_TIMER);

        m_WatcdogTimer->setDuration(WATCHDOG_START_DURATION, WATCHDOG_TIME_OUT);
        m_WatcdogTimer->start();
        HeartBeat_Ready();

        // @CGA_VARIANT_START{"OnboardclientInputManager:Init()"}
        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"OnboardclientInputManager:Init()"}

        return tidlResult;
    }

    OnboardclientInputManager::OnboardclientHandler *OnboardclientInputManager::OnboardclientHandler::mOnboardclientHandler{nullptr};
    OnboardclientInputManager::OnboardclientHandler::OnboardclientHandler(android::sp<sl::SLLooper> &looper, OnboardclientInputManager &outer) noexcept : Handler(looper), mOnboardclientInputMgr(outer)
    {
        mOnboardclientHandler = this;
    }

    OnboardclientInputManager::OnboardclientHandler *OnboardclientInputManager::OnboardclientHandler::getInstance()
    {
        if (mOnboardclientHandler == nullptr)
        {
            LOGE("mOnboardclientHandler is nullptr");
        }
        return mOnboardclientHandler;
    }

    void OnboardclientInputManager::OnboardclientInputMgrTimer::handlerFunction(const int32_t timerId)
    {
        LOGV("OnboardclientInputMgrTimer is received");
        switch (timerId)
        {
        case ONBOARDCLIENT_WATCHDOG_TIMER:
        {
            (void)mHandler->obtainMessage(OBC_INPUT_HANDLER::MSG_RECEIVE_WATCH_DOG)->sendToTarget();
            break;
        }
        default:
            break;
        }
    }

    void OnboardclientInputManager::OnboardclientHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
    {
        const int32_t what{handlemsg->what};
        switch (what)
        {
        case OBC_INPUT_HANDLER::MSG_CONNECT_TO_APPMGR:
        {
            LOGV("handleMessage OBC_INPUT_HANDLER::MSG_CONNECT_TO_APPMGR");
            mOnboardclientInputMgr.connectToAppMgr();
            break;
        }

        case OBC_INPUT_HANDLER::MSG_RECEIVE_BOOT_COMPLETE:
        {
            mOnboardclientInputMgr.mOnboardclientMgrService->setBootComplete(true);
            LOGV("handleMessage OBC_INPUT_HANDLER::MSG_RECEIVE_BOOT_COMPLETE");
            break;
        }
        case OBC_INPUT_HANDLER::MSG_RECEIVE_WATCH_DOG:
        {
            LOGV("handleMessage OBC_INPUT_HANDLER::MSG_RECEIVE_WATCH_DOG");
            if (handlemsg->arg1 == 1)
            {
                LOGI("WDG kick by Connect");
                mOnboardclientInputMgr.m_WatcdogTimer->restart();
            }

            HeartBeat();
            break;
        }
        case OBC_INPUT_HANDLER::MSG_OBC_CONNECT_TO_COMMMGR:
        {
            LOGV("handleMessage OBC_INPUT_HANDLER::MSG_OBC_CONNECT_TO_COMMMGR");
            CommunicationManagerAdapter::getInstance()->registerService();
            break;
        }
        case OBC_INPUT_HANDLER::MSG_OBC_RECEIVE_BOOT_PRE:
        {
            LOGV("handleMessage OBC_INPUT_HANDLER::MSG_RECEIVE_BOOT_PRE");
            CommunicationManagerAdapter::getInstance()->registerService();
            DiagManagerAdapter::getInstance()->registerService();
            break;
        }
        case OBC_INPUT_HANDLER::MSG_OBC_CONNECT_TO_DIAGMGR:
        {
            LOGV("handleMessage OBC_INPUT_HANDLER::MSG_OBC_CONNECT_TO_DIAGMGR");
            DiagManagerAdapter::getInstance()->registerService();
            break;
        }
        default:
            LOGV("Wrong Message received.[%8x]", what);
            break;
        }

        // @CGA_VARIANT_START{"OnboardclientInputManager:handleMessage()"}
        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"OnboardclientInputManager:handleMessage()"}
    }

    /* auto __ appmgr Inheritance CGA start-------------------------------------------------*/
    void OnboardclientInputManager::connectToAppMgr()
    {
        mAppManager = android::interface_cast<IApplicationManagerService>(
            android::defaultServiceManager()->getService(android::String16("service_layer.ApplicationManagerService")));

        if (mAppManager != nullptr)
        {
            if (mAppManager->getBootCompleted(true /* PreBootCompleted : All service ready? */))
            {
                (void)mMyHandler->sendMessageDelayed(mMyHandler->obtainMessage(OBC_INPUT_HANDLER::MSG_RECEIVE_BOOT_COMPLETE),
                                                     OBC_COMMON::TIME_SEND_RETRY_DELAY_MS);
            }

            if (mSystemPostReceiver == nullptr)
            {
                mSystemPostReceiver = new SystemPostReceiver(mMyHandler); // charles : change the class name
            }
            else
            {
                LOGV("Already mSystemPostReceiver was created.");
            }

            // mAppManager->registerSystemPostReceiver(mSystemPostReceiver, SYS_POST_BOOT_COMPLETED | SYS_POST_BOOT_COMPLETED_PRE | SYS_POST_PROVISIONING_COMPLETED);
            (void)mAppManager->registerSystemPostReceiver(mSystemPostReceiver, SYS_POST_BOOT_COMPLETED);
            (void)mAppManager->registerSystemPostReceiver(mSystemPostReceiver, SYS_POST_BOOT_COMPLETED_PRE);
            (void)mAppManager->registerSystemPostReceiver(mSystemPostReceiver, SYS_POST_PROVISIONING_COMPLETED);
        }
        else
        {
            LOGE("appManager is nullptr retry in 500ms - %s", __func__);
            if (mMyHandler != nullptr)
            {
                (void)mMyHandler->sendMessageDelayed(mMyHandler->obtainMessage(OBC_INPUT_HANDLER::MSG_CONNECT_TO_APPMGR),
                                                     OBC_COMMON::TIME_SEND_RETRY_DELAY_MS);
            }
            else
            {
                LOGE("connectToAppMgr: mMyHandler is nullptr - %s", __func__);
            }
        }
    }
    /* auto __ appmgr Inheritance CGA end-------------------------------------------------*/

    void OnboardclientInputManager::onServiceBinderDied(const android::wp<android::IBinder> &who) noexcept
    {
        const Mutex::Autolock lock{Mutex::Autolock(mBinderDiedLock)};
        (void)who;
    }

    void OnboardclientInputManager::resetWatchDog(void)
    {
        (void)mMyHandler->obtainMessage(OBC_INPUT_HANDLER::MSG_RECEIVE_WATCH_DOG, 1)->sendToTarget();
    }
};
// Yours

// @CGA_VARIANT_START{"__GLOBAL_SCOPETAIL__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPETAIL__:__YOUR_CODE__:variant"}
