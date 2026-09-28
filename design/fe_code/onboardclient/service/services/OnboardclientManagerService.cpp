
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
#define LOG_TAG "OnboardclientManagerService"

#include <Log.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

/* auto __ appmgr Inheritance CGA start-------------------------------------------------*/
#include <services/ApplicationManagerService/IApplicationManagerService.h>
#include <services/ApplicationManagerService/IApplicationManagerServiceType.h>
/* auto __ appmgr Inheritance CGA end-------------------------------------------------*/

#include "services/CommunicationManagerService/CommunicationData.h"
#include <services/CommunicationManagerService/CM_Protocol.h>

#include <utils/watchdog/watchdog_client.h>

#include "OnboardclientInputManager.h"
#include "OnboardclientManagerService.h"
#include "OnboardclientRxHandler.h"
#include "OnboardclientTxHandler.h"
#include "OnboardclientImpl.h"
#ifdef __UNITTEST__
#define CHECK_SERVICE_API_READY() \
    {                             \
    }
#else // ! __UNITTEST__
#define CHECK_SERVICE_API_READY()                                                                                \
    do                                                                                                           \
    {                                                                                                            \
        if (check_condition_api_ready() == false)                                                                \
        {                                                                                                        \
            LOGW("Who(pid:%d) call me? System is not ready.", android::IPCThreadState::self()->getCallingPid()); \
        }                                                                                                        \
    } while (false);
#endif // __UNITTEST__

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
    OnboardclientManagerService *OnboardclientManagerService::mOnboardclientManagerService{nullptr};

    OnboardclientManagerService::OnboardclientManagerService()
        : SystemService(OnboardclientManagerService::getServiceName()),
          mBootCompleted(false)
    {
        // @CGA_VARIANT_START{"OnboardclientManagerService():__YOUR_CODE__:variant"}
        // @CGA_VARIANT___END{"OnboardclientManagerService():__YOUR_CODE__:variant"}
        mOnboardclientManagerService = this;
        MAX_UDS_REQ_SIZE = 65536U; // 64kb
    }

    OnboardclientManagerService *OnboardclientManagerService::instance()
    {
        if (mOnboardclientManagerService == nullptr)
        {
            mOnboardclientManagerService = new OnboardclientManagerService();
        }

        return mOnboardclientManagerService;
    }

    uint8_t OnboardclientManagerService::getModuleID() noexcept
    {
        return MODULE_ONBOARDCLIENT_MGR;
    }

    error_t OnboardclientManagerService::dump(LogOutput &out) noexcept
    {
        return E_OK;
    }

    bool OnboardclientManagerService::onInit()
    {
        LOGV("onInit");

        // @CGA_VARIANT_START{"OnboardclientManagerService:onInit"}
        mReceiverDeathRecipient = new ReceiverDeathRecipient(*this);

        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"OnboardclientManagerService:onInit"}

        return false;
    }

    void OnboardclientManagerService::instantiate()
    {
        LOGV("instantiate");

        // @CGA_VARIANT_START{"OnboardclientManagerService:instantiate"}
        (void)android::defaultServiceManager()->addService(android::String16(OnboardclientManagerService::getServiceName()), new ServiceStub(*this));

        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"OnboardclientManagerService:instantiate"}
    }

    error_t OnboardclientManagerService::onStart()
    {
        LOGV("onStart");
        error_t ret{E_OK};

        // @CGA_VARIANT_START{"OnboardclientManagerService:onStart"}

        mOnboardclientInputMgr = new OnboardclientInputManager(this);
        ret = mOnboardclientInputMgr->init();
        // init Rx Handler
        mOnboardclientRxHandlerLooper = new sl::SLLooper();
        (void)mOnboardclientRxHandlerLooper->prepare();
        mOnboardclientRxHandlerLooper->setName("OnboardclientRxHandler");
        (void)mOnboardclientRxHandlerLooper->start(false);
        mOnboardclientRxHandler = new OnboardclientRxHandler(mOnboardclientRxHandlerLooper, this);
        (void)OnboardclientRxHandler::getInstance()->obtainMessage(OBC_RX_HANDLER::CMD_RX_INIT)->sendToTarget();
        // init Tx Handler
        mOnboardclientTxHandlerLooper = new sl::SLLooper();
        (void)mOnboardclientTxHandlerLooper->prepare();
        mOnboardclientTxHandlerLooper->setName("OnboardclientTxHandler");
        (void)mOnboardclientTxHandlerLooper->start(false);
        mOnboardclientTxHandler = new OnboardclientTxHandler(mOnboardclientTxHandlerLooper, this);
        (void)OnboardclientTxHandler::getInstance()->obtainMessage(OBC_TX_HANDLER::CMD_TX_INIT)->sendToTarget();
        (void)SystemService::onStart();

        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"OnboardclientManagerService:onStart"}

        return ret;
    }

    error_t OnboardclientManagerService::onStop()
    {
        LOGV("onStop");

        // @CGA_VARIANT_START{"OnboardclientManagerService:onStop"}
        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"OnboardclientManagerService:onStop"}

        return E_OK;
    }

    // registerReceiverOnboardClientReceiverOnNotifyOBD2Event : OnboardClientReceiver receiver onNotifyOBD2Event receiver_function
    error_t OnboardclientManagerService::registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        error_t ret{E_OK};

        // @CGA_VARIANT_START{"OnboardclientManagerService:OnboardClientReceiver:onNotifyOBD2Event:registerReceiverOnboardClientReceiverOnNotifyOBD2Event"}
        LOGV("registerReceiverOnboardClientReceiverOnNotifyOBD2Event()");

        if (receiver == nullptr)
        {
            LOGE("registerReceiverOnboardClientReceiverOnNotifyOBD2Event(), receiver is nullptr : %s", __func__);
            ret = E_ERROR;
        }
        else
        {
            const Mutex::Autolock _l{Mutex::Autolock(mReceiverLock)};
            std::list<android::sp<IOnboardClientReceiver>> &_l_receivers{mlReceiversOnboardClientReceiverOnNotifyOBD2Event};

            // check duplication
            for (auto &_l_r : _l_receivers)
            {
                if (
#ifdef _MORE_MARSHM
                    android::IInterface::asBinder(_l_r) == android::IInterface::asBinder(receiver) // 64 bit
#else                                                                                              // !(_MORE_MARSHM)
                    _l_r->asBinder() == receiver->asBinder() // 32 bit
#endif                                                                                             // _MORE_MARSHM
                )
                {
                    LOGE("registerReceiverOnboardClientReceiverOnNotifyOBD2Event(), existed : %s", __func__);
                    ret = E_ERROR;
                    break;
                }
            }

            if (ret == E_OK)
            {
#ifdef _MORE_MARSHM
                (void)android::IInterface::asBinder(receiver)->linkToDeath(mReceiverDeathRecipient); // 64 bit
#else                                                                                                // !(_MORE_MARSHM)
                receiver->asBinder()->linkToDeath(mReceiverDeathRecipient); // 32 bit
#endif                                                                                               // _MORE_MARSHM
                _l_receivers.push_back(receiver);
                /*
                 * Write your own code
                 */
                // @CGA_VARIANT___END{"OnboardclientManagerService:OnboardClientReceiver:onNotifyOBD2Event:registerReceiverOnboardClientReceiverOnNotifyOBD2Event"}
            }
        }
        return ret;
    }

    // registerReceiverOnboardClientReceiverOnResponseEvent : OnboardClientReceiver receiver onResponseEvent receiver_function
    error_t OnboardclientManagerService::registerReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        error_t ret{E_OK};

        // @CGA_VARIANT_START{"OnboardclientManagerService:OnboardClientReceiver:onResponseEvent:registerReceiverOnboardClientReceiverOnResponseEvent"}
        LOGV("registerReceiverOnboardClientReceiverOnResponseEvent()");

        if (receiver == nullptr)
        {
            LOGE("registerReceiverOnboardClientReceiverOnResponseEvent(), receiver is nullptr : %s", __func__);
            ret = E_ERROR;
        }
        else
        {
            const Mutex::Autolock _l{Mutex::Autolock(mReceiverLock)};
            std::list<android::sp<IOnboardClientReceiver>> &_l_receivers{mlReceiversOnboardClientReceiverOnResponseEvent};

            // check duplication
            for (auto &_l_r : _l_receivers)
            {
                if (
#ifdef _MORE_MARSHM
                    android::IInterface::asBinder(_l_r) == android::IInterface::asBinder(receiver) // 64 bit
#else                                                                                              // !(_MORE_MARSHM)
                    _l_r->asBinder() == receiver->asBinder() // 32 bit
#endif                                                                                             // _MORE_MARSHM
                )
                {
                    LOGE("registerReceiverOnboardClientReceiverOnResponseEvent(), existed : %s", __func__);
                    ret = E_ERROR;
                    break;
                }
            }

            if (ret == E_OK)
            {
#ifdef _MORE_MARSHM
                (void)android::IInterface::asBinder(receiver)->linkToDeath(mReceiverDeathRecipient); // 64 bit
#else                                                                                                // !(_MORE_MARSHM)
                receiver->asBinder()->linkToDeath(mReceiverDeathRecipient); // 32 bit
#endif                                                                                               // _MORE_MARSHM
                _l_receivers.push_back(receiver);

                /*
                 * Write your own code
                 */
                // @CGA_VARIANT___END{"OnboardclientManagerService:OnboardClientReceiver:onResponseEvent:registerReceiverOnboardClientReceiverOnResponseEvent"}
            }
        }
        return ret;
    }

    // unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event : OnboardClientReceiver receiver onNotifyOBD2Event receiver_function
    error_t OnboardclientManagerService::unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        error_t ret{E_ERROR};

        // @CGA_VARIANT_START{"OnboardclientManagerService:OnboardClientReceiver:onNotifyOBD2Event:unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event"}
        LOGV("unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event()");

        const Mutex::Autolock _l{Mutex::Autolock(mReceiverLock)};

        std::list<android::sp<IOnboardClientReceiver>> &_l_receivers{mlReceiversOnboardClientReceiverOnNotifyOBD2Event};

        for (auto &_l_r : _l_receivers)
        {
            if (
#ifdef _MORE_MARSHM
                android::IInterface::asBinder(_l_r) == android::IInterface::asBinder(receiver) // 64 bit
#else                                                                                          // !(_MORE_MARSHM)
                _l_r->asBinder() == receiver->asBinder() // 32 bit
#endif                                                                                         // _MORE_MARSHM
            )
            {
#ifdef _MORE_MARSHM
                (void)android::IInterface::asBinder(_l_r)->unlinkToDeath(mReceiverDeathRecipient); // 64 bit
#else                                                                                              // !(_MORE_MARSHM)
                _l_r->asBinder()->unlinkToDeath(mReceiverDeathRecipient); // 32 bit
#endif                                                                                             // _MORE_MARSHM
                _l_receivers.remove(_l_r);
                ret = E_OK;
                break;
            }
        }

        if (ret == E_ERROR)
        {
            LOGE("OnboardclientMgr unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(), no hit : %s", __func__);

            /*
             * Write your own code
             */
            // @CGA_VARIANT___END{"OnboardclientManagerService:OnboardClientReceiver:onNotifyOBD2Event:unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event"}
        }
        return ret;
    }

    // unregisterReceiverOnboardClientReceiverOnResponseEvent : OnboardClientReceiver receiver onResponseEvent receiver_function
    error_t OnboardclientManagerService::unregisterReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        error_t ret{E_ERROR};

        // @CGA_VARIANT_START{"OnboardclientManagerService:OnboardClientReceiver:onResponseEvent:unregisterReceiverOnboardClientReceiverOnResponseEvent"}
        LOGV("unregisterReceiverOnboardClientReceiverOnResponseEvent()");

        const Mutex::Autolock _l{Mutex::Autolock(mReceiverLock)};

        std::list<android::sp<IOnboardClientReceiver>> &_l_receivers{mlReceiversOnboardClientReceiverOnResponseEvent};

        for (android::sp<IOnboardClientReceiver> &_l_r : _l_receivers)
        {
            if (
#ifdef _MORE_MARSHM
                android::IInterface::asBinder(_l_r) == android::IInterface::asBinder(receiver) // 64 bit
#else                                                                                          // !(_MORE_MARSHM)
                _l_r->asBinder() == receiver->asBinder() // 32 bit
#endif                                                                                         // _MORE_MARSHM
            )
            {
#ifdef _MORE_MARSHM
                (void)android::IInterface::asBinder(_l_r)->unlinkToDeath(mReceiverDeathRecipient); // 64 bit
#else                                                                                              // !(_MORE_MARSHM)
                _l_r->asBinder()->unlinkToDeath(mReceiverDeathRecipient); // 32 bit
#endif                                                                                             // _MORE_MARSHM
                _l_receivers.remove(_l_r);
                ret = E_OK;
                break;
            }
        }

        if (ret == E_ERROR)
        {
            LOGE("OnboardclientMgr unregisterReceiverOnboardClientReceiverOnResponseEvent(), no hit : %s", __func__);

            /*
             * Write your own code
             */
            // @CGA_VARIANT___END{"OnboardclientManagerService:OnboardClientReceiver:onResponseEvent:unregisterReceiverOnboardClientReceiverOnResponseEvent"}
        }
        return ret;
    }

    void OnboardclientManagerService::onReceiverBinderDied(const android::wp<android::IBinder> &who)
    {
        LOGE("OnboardclientManagerService::onReceiverBinderDied() : %s", __func__);

        const Mutex::Autolock _l{Mutex::Autolock(mReceiverLock)};

        for (auto &_l_r : mlReceiversOnboardClientReceiverOnNotifyOBD2Event)
        {
            if (
#ifdef _MORE_MARSHM
                android::IInterface::asBinder(_l_r) == who // 64 bit
#else                                                      // !(_MORE_MARSHM)
                _l_r->asBinder() == who // 32 bit
#endif                                                     // _MORE_MARSHM
            )
            {
                LOGE("OnboardclientManagerService::onReceiverBinderDied(), remove died receiver list OnboardClientReceiver onNotifyOBD2Event : %s", __func__);
                mlReceiversOnboardClientReceiverOnNotifyOBD2Event.remove(_l_r);
                break;
            }
        }

        for (auto &_l_r : mlReceiversOnboardClientReceiverOnResponseEvent)
        {
            if (
#ifdef _MORE_MARSHM
                android::IInterface::asBinder(_l_r) == who // 64 bit
#else                                                      // !(_MORE_MARSHM)
                _l_r->asBinder() == who // 32 bit
#endif                                                     // _MORE_MARSHM
            )
            {
                LOGE("OnboardclientManagerService::onReceiverBinderDied(), remove died receiver list OnboardClientReceiver onResponseEvent : %s", __func__);
                mlReceiversOnboardClientReceiverOnResponseEvent.remove(_l_r);
                break;
            }
        }

        // @CGA_VARIANT_START{"OnboardclientManagerService:onReceiverBinderDied()"}
        // your additional code ...
        // @CGA_VARIANT___END{"OnboardclientManagerService:onReceiverBinderDied()"}
    }

    // queryReceiverByOnboardClientReceiver : OnboardClientReceiver receiver onNotifyOBD2Event receiver_function
    error_t OnboardclientManagerService::queryReceiverByOnboardClientReceiverOnNotifyOBD2Event( //  Parm:0
    )
    {

#if 1 // for debugging : LOGV
        LOGV("OnboardclientManagerService :: queryReceiverByOnboardClientReceiverOnNotifyOBD2Event");
#endif

        const Mutex::Autolock _l{Mutex::Autolock(mReceiverLock)};

        // @CGA_VARIANT_START{"OnboardclientManagerService:queryReceiverByOnboardClientReceiverOnNotifyOBD2Event"}
        std::list<android::sp<IOnboardClientReceiver>> &_l_receivers{mlReceiversOnboardClientReceiverOnNotifyOBD2Event};

        for (const auto _l_r : _l_receivers)
        {
            LOGV("queryReceiverByOnboardClientReceiverOnNotifyOBD2Event : call r->onNotifyOBD2Event");
            // callback
            _l_r->onNotifyOBD2Event( //  Parm:0
            );
        }

        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"OnboardclientManagerService:queryReceiverByOnboardClientReceiverOnNotifyOBD2Event"}

        return E_OK;
    }

    // queryReceiverByOnboardClientReceiver : OnboardClientReceiver receiver onResponseEvent receiver_function
    error_t OnboardclientManagerService::queryReceiverByOnboardClientReceiverOnResponseEvent( //  Parm:0
        const android::sp<OBCResponseEventInfo> message                                       //  Parm:1
    )
    {
        const Mutex::Autolock _l{Mutex::Autolock(mReceiverLock)};

        // @CGA_VARIANT_START{"OnboardclientManagerService:queryReceiverByOnboardClientReceiverOnResponseEvent"}
        std::list<android::sp<IOnboardClientReceiver>> &_l_receivers{mlReceiversOnboardClientReceiverOnResponseEvent};
        const android::sp<OBCResponseEventInfo> _m{message};

        for (const auto _l_r : _l_receivers)
        {
            LOGV("queryReceiverByOnboardClientReceiverOnResponseEvent : call r->onResponseEvent");
            // callback
            _l_r->onResponseEvent(_m);
        }

        return E_OK;
    }

    error_t OnboardclientManagerService::ServiceStub::registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        return mParent.registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
            receiver);
    }

    error_t OnboardclientManagerService::ServiceStub::registerReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        return mParent.registerReceiverOnboardClientReceiverOnResponseEvent(
            receiver);
    }

    error_t OnboardclientManagerService::ServiceStub::unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        return mParent.unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
            receiver);
    }

    error_t OnboardclientManagerService::ServiceStub::unregisterReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp<IOnboardClientReceiver> &receiver)
    {
        return mParent.unregisterReceiverOnboardClientReceiverOnResponseEvent(
            receiver);
    }

    error_t OnboardclientManagerService::ServiceStub::queryReceiverByOnboardClientReceiverOnNotifyOBD2Event( //  Parm:0
    )
    {
        return mParent.queryReceiverByOnboardClientReceiverOnNotifyOBD2Event( //  Parm:0
        );
    }

    error_t OnboardclientManagerService::ServiceStub::queryReceiverByOnboardClientReceiverOnResponseEvent( //  Parm:0
        const android::sp<OBCResponseEventInfo> message                                                    //  Parm:1
    )
    {
        return mParent.queryReceiverByOnboardClientReceiverOnResponseEvent( //  Parm:0
            message                                                         //  Parm:1
        );
    }

    // auto API start : wishtoUseAPI
    uint8_t OnboardclientManagerService::ServiceStub::sendUdsData(const uint16_t connectId, const android::sp<Buffer> udsRequest)
    {
        CHECK_SERVICE_API_READY();

        LOGV("API ServiceStub::sendUdsData");
        // @CGA_VARIANT_START{"OnboardclientManagerService::ServiceStub::sendUdsData"}
        return mParent.sendUdsData(
            connectId, udsRequest);
        // @CGA_VARIANT___END{"OnboardclientManagerService::ServiceStub::sendUdsData"}
    }

    // auto API end : wishtoUseAPI
    // @CGA_VARIANT_START{"xxxManagerService.cpp:__CUSTOM_API__CODE__:variant"}
    // @CGA_VARIANT___END{"xxxManagerService.cpp:__CUSTOM_API__CODE__:variant"}

    // RECEIVER API start
    error_t OnboardclientManagerService::ServiceStub::testOnNotifyOBD2Event( //  Parm:0
    )
    {
        LOGV("API ServiceStub: testOnNotifyOBD2Event");
        return mParent.testOnNotifyOBD2Event( //  Parm:0
        );
    }

    error_t OnboardclientManagerService::ServiceStub::testOnResponseEvent(const android::sp<OBCResponseEventInfo> message)
    {
        LOGV("API ServiceStub: testOnResponseEvent");
        return mParent.testOnResponseEvent( //  Parm:0
            message                         //  Parm:1
        );
    }

    // RECEIVE API end

    bool OnboardclientManagerService::ServiceStub::check_condition_api_ready(void) const
    {
        bool isReady{true};
        if (mParent.mBootCompleted == false)
        {
            LOGW("mBootCompleted is false");
            isReady = false;
        }
        return isReady;
    }

    error_t OnboardclientManagerService::ServiceStub::connect(const android::sp<OBCTransportInfo> transportInfo, const std::string appName, android::sp<OBCConnectInfo> &connectInfo)
    {
        return mParent.connect(transportInfo, appName, connectInfo);
    }

    uint8_t OnboardclientManagerService::ServiceStub::disconnect(const uint16_t connectId)
    {
        return mParent.disconnect(connectId);
    }

    error_t OnboardclientManagerService::ServiceStub::sendUdsResponseData(const android::sp<Buffer> udsResponse)
    {

        return mParent.sendUdsResponseData(udsResponse);
    }

    error_t OnboardclientManagerService::ServiceStub::sendAckToMcu(const uint8_t reason)
    {
        return mParent.sendAckToMcu(reason);
    }

    uint16_t OnboardclientManagerService::ServiceStub::getMaxConnectCount(const uint8_t protocolType)
    {
        return mParent.getMaxConnectCount(protocolType);
    }

    uint8_t OnboardclientManagerService::ServiceStub::setAppPriority(const std::string appName, const uint8_t priority)
    {
        return mParent.setAppPriority(appName, priority);
    }
    uint8_t OnboardclientManagerService::ServiceStub::setMaxExceeded(const uint8_t queueType, const uint32_t queueSize)
    {
        return mParent.setMaxExceeded(queueType, queueSize);
    }
    // RECEIVER API start
    error_t OnboardclientManagerService::testOnNotifyOBD2Event( //  Parm:0
    )
    {
        LOGV("OnboardclientManagerService Bn : testOnNotifyOBD2Event");

        // @CGA_VARIANT_START{"OnboardclientManagerService:onNotifyOBD2Event"}

        // TIDL follows the sequence.  so we call queryReceiverBy...() directly for test. (This is receiver function test through privileged API.
        //      1. receive the msg from related manager (service/XXXInputManager.h : RelatedManager's Receiver class :: RelatedManager's receiver function)
        //      2. RelatedManager's receiver function sends this msg to handler
        //      3. invoke proper queryReceiverBy...() with proper calculation in handleMessage()'s case.   (service/XXXInputManager.cpp)
        //      4. queryReceiverBy...() will invoke registered manager's receiver functions with this msg. (service/XXXManagerService.cpp)
        (void)queryReceiverByOnboardClientReceiverOnNotifyOBD2Event( //  Parm:0
        );

        /*
         * Write your own code
         */

        // @CGA_VARIANT___END{"OnboardclientManagerService:onNotifyOBD2Event"}

        return E_OK;
    }

    error_t OnboardclientManagerService::testOnResponseEvent(const android::sp<OBCResponseEventInfo> message)
    {
        LOGV("OnboardclientManagerService Bn : testOnResponseEvent");

        // @CGA_VARIANT_START{"OnboardclientManagerService:onResponseEvent"}

        // TIDL follows the sequence.  so we call queryReceiverBy...() directly for test. (This is receiver function test through privileged API.
        //      1. receive the msg from related manager (service/XXXInputManager.h : RelatedManager's Receiver class :: RelatedManager's receiver function)
        //      2. RelatedManager's receiver function sends this msg to handler
        //      3. invoke proper queryReceiverBy...() with proper calculation in handleMessage()'s case.   (service/XXXInputManager.cpp)
        //      4. queryReceiverBy...() will invoke registered manager's receiver functions with this msg. (service/XXXManagerService.cpp)
        (void)queryReceiverByOnboardClientReceiverOnResponseEvent( //  Parm:0
            message                                                //  Parm:1
        );

        /*
         * Write your own code
         */

        // @CGA_VARIANT___END{"OnboardclientManagerService:onResponseEvent"}

        return E_OK;
    }

    // RECEIVER API end

    /*
     * @MISRA{MISRA C++-2008 Rule 0-1-3,"This is intended design"}
     */

    bool OnboardclientManagerService::isApplicationExecuted(const appid_t id) noexcept
    {
        const Mutex::Autolock _l{Mutex::Autolock(mReceiverLock)};
        constexpr bool _tidl_result{false};
        (void)id;
        /* you should implement it for your purpose. (jk.byun)
        auto &r: mlReceiversOnboardClientReceiverOnNotifyOBD2Event
        auto &r: mlReceiversOnboardClientReceiverOnResponseEvent
        for (const auto s: mReceivers) {
            if (s.first == id) {
                for (const auto r: s.second) {
                    LOGI("Onboardclient isApplicationExecuted : [%2x]", id);
                    _tidl_result = true;
                }
            }
        }
        */

        // @CGA_VARIANT_START{"OnboardclientManagerService:isApplicationExecuted"}
        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"OnboardclientManagerService:isApplicationExecuted"}

        return _tidl_result;
    }

    // Yours
    // auto API start : wishtoUseAPI
    uint8_t OnboardclientManagerService::sendUdsData(const uint16_t connectId, const android::sp<Buffer> udsRequest)
    {
        uint8_t res{0U};
        LOGI("[%s] connectID %u, sizeUDS %d", __func__, connectId, udsRequest->size());
        uint32_t connectedCanId{0U};
        uint8_t protocolType{0U};
        const uint8_t isConnected{OnboardclientImpl::getInstance()->getConnectedCanID(connectId, protocolType, connectedCanId)};
        if (isConnected == OBCEnum::OBCErrCode::OBC_OK)
        {
            const uint32_t transQueueSize{OnboardclientImpl::getInstance()->getTransQueueSize()};
            const uint64_t tmpQueueSize{static_cast<uint64_t>(transQueueSize) + static_cast<uint64_t>(udsRequest->size())};
            if (tmpQueueSize <= static_cast<uint64_t>(UINT32_MAX))
            {
                if (static_cast<uint32_t>(tmpQueueSize) > MAX_UDS_REQ_SIZE)
                {
                    LOGD("Transmission queue is full");
                    res = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_BUSY);
                }
                else
                {
                    OnboardclientImpl::getInstance()->pushToTransQueue(connectId, udsRequest);
                    res = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK);
                }
            }
            else
            {
                LOGV("invalid transQueueSize");
            }
        }
        else
        {
            res = static_cast<uint8_t>(isConnected);
        }
        return res;
    }

    error_t OnboardclientManagerService::connect(const android::sp<OBCTransportInfo> transportInfo, const std::string appName, android::sp<OBCConnectInfo> &connectInfo)
    {
        error_t ret{E_OK};
        if (connectInfo == nullptr)
        {
            connectInfo = new OBCConnectInfo();
        }
        const uint32_t canId{transportInfo->canInfo().getCanId()};
        if ((canId > 0U) && (canId <= UINT32_MAX))
        {
            uint8_t isConnected{0U};
            uint16_t connectedId{0U};
            isConnected = OnboardclientImpl::getInstance()->getConnectedId(connectedId, canId);
            if (isConnected == OBCEnum::OBC_ERR_NOT_CONNECTED)
            {
                if ((appName.length() > 0U) && (appName.length() <= 30U))
                {
                    if ((transportInfo->protocolType() == OBCEnum::OBCProtocolType::DOCAN) ||
                        (transportInfo->protocolType() == OBCEnum::OBCProtocolType::DOCAN29BIT) ||
                        (transportInfo->protocolType() == OBCEnum::OBCProtocolType::DOCAN11BITEX) ||
                        (transportInfo->protocolType() == OBCEnum::OBCProtocolType::DOCAN29BITCANFD))
                    {
                        mOnboardclientInputMgr->resetWatchDog();
                        OnboardclientImpl::getInstance()->connectCanClient(transportInfo, connectInfo);
                        LOGI("OBCConnectInfo: response: %02X, connectId: %d", connectInfo->getResponse(), connectInfo->getConnectId());
                    }
                    else
                    {
                        ret = E_ERROR;
                        connectInfo->setData(OBCEnum::OBCErrCode::OBC_ERR_NOT_SUPPORTED, 0U);
                    }
                }
                else
                {
                    ret = E_ERROR;
                    connectInfo->setData(OBCEnum::OBCErrCode::OBC_ERR_INVALID_PARAMETERS, 0U);
                }
            }
            else if (isConnected == OBCEnum::OBC_OK)
            {
                connectInfo->setData(OBCEnum::OBC_OK, connectedId);
            }
            else
            {
                ret = E_ERROR;
                connectInfo->setData(OBCEnum::OBCErrCode::OBC_ERR_CONNECTED_MAX, 0U);
            }
        }
        else
        {
            LOGD("invalid canID");
            ret = E_ERROR;
            connectInfo->setData(OBCEnum::OBCErrCode::OBC_ERR_CONNECTED_MAX, 0U);
        }

        return ret;
    }

    uint8_t OnboardclientManagerService::disconnect(const uint16_t connectId)
    {
        LOGE("[OnboardclientManagerService] [%s]", __func__);
        uint8_t responseCode{static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_NOT_CONNECTED)};
        uint8_t protocolType{0U};
        uint32_t canId{0U};
        responseCode = OnboardclientImpl::getInstance()->getConnectedCanID(connectId, protocolType, canId);
        if (responseCode == static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK))
        {
            mOnboardclientInputMgr->resetWatchDog();
            responseCode = OnboardclientImpl::getInstance()->disconnectCanClient(connectId);
            LOGI("[OnboardclientManagerService] [%s] Disconnected id = %d", __func__, connectId);
        }
        return responseCode;
    }

    error_t OnboardclientManagerService::sendUdsResponseData(const android::sp<Buffer> udsResponse)
    {
        // TODO
        LOGV("OnboardclientManagerService Bn : sendUdsResponseData");
        LOGI("Receive form DiagMgr");
        const android::sp<sl::Message> message{OnboardclientRxHandler::getInstance()->obtainMessage(OBC_RX_HANDLER::MSG_OBC_RECEIVE_FROM_DIAG)};
        message->spRef = udsResponse;
        (void)message->sendToTarget();
        return E_OK;
    }

    error_t OnboardclientManagerService::sendAckToMcu(const uint8_t reason)
    {
        (void)mOnboardclientTxHandler->obtainMessage(OBC_TX_HANDLER::CMD_TX_SEND_ACK_ROB, static_cast<int32_t>(reason))->sendToTarget();
        return E_OK;
    }

    uint16_t OnboardclientManagerService::getMaxConnectCount(const uint8_t protocolType)
    {
        LOGV("[getMaxConnectCount] Max Connect count always is 1");
        (void)protocolType;
        return 1U; // Max Connect count always is 1
    }

    uint8_t OnboardclientManagerService::setAppPriority(const std::string appName, const uint8_t priority)
    {
        uint8_t res{0U};
        LOGV("[setAppPriority] appName: %s, priority: %u", appName.c_str(), priority);
        if (appName.length() > 30U)
        {
            LOGV("App name is exceeded");
            res = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_INVALID_PARAMETERS);
        }
        else
        {
            res = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK);
        }
        return res;
    }

    uint8_t OnboardclientManagerService::setMaxExceeded(const uint8_t queueType, const uint32_t queueSize)
    {
        uint8_t res{2U};
        if (queueType == 0x01U)
        {
            res = 1U;
            MAX_UDS_REQ_SIZE = queueSize;
            LOGV("[setMaxExceeded] Req queue size: %lu", MAX_UDS_REQ_SIZE);
        }
        else if (queueType == 0x02U)
        {
            res = 1U;
            OnboardclientImpl::getInstance()->setTempMaxExceedSize(queueSize);
        }
        else
        {
            res = 0U;
            LOGV("[setMaxExceeded] Invalid queue type");
        }

        return res;
    }

    void OnboardclientManagerService::setBootComplete(const bool isCompleted) noexcept
    {
        this->mBootCompleted = isCompleted;
    }
};
// auto API end : wishtoUseAPI

// @CGA_VARIANT_START{"__GLOBAL_SCOPETAIL__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPETAIL__:__YOUR_CODE__:variant"}
