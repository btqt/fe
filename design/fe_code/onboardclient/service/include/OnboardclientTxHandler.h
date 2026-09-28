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
 * @author  tuyen2.nguyen@lge.com
 * @date    2023.12.22
 * @version 3.0.00
 */

#ifndef ONBOARDCLIENT_TX_HANDLER_H
#define ONBOARDCLIENT_TX_HANDLER_H

#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include <utils/Handler.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/external/mindroid/lang/String.h>
#include <utils/Mutex.h>
#include "OnboardclientManagerService.h"
#include "Common_def.h"
namespace OBC
{
    class OnboardclientTxHandler : public sl::Handler
    {
    private:
        static OnboardclientTxHandler *mOnboardclientTxHandler;
        const android::sp<OnboardclientManagerService> mOnboardclientMgrService;

    public:
        class OnboardclientTxTimerHandler : public TimerTimeoutHandler
        {
        public:
            static constexpr int32_t OBC_TX_TIMER_MONITORING_WIRE_CONNECTION{1};
            static constexpr int32_t OBC_TX_TIMER_SEND_UDS_COMMUNICATION_ERROR{2};
            static constexpr int32_t OBC_TX_TIMER_SEND_UDS_BUSY_RESPONSE{3};
            static constexpr int32_t OBC_TX_TIMER_TIMEOUT_RESPONSE{4};
            static constexpr int32_t OBC_TX_TIMER_TIMEOUT_ACK_RESPONSE{5};
            OnboardclientTxTimerHandler() noexcept;
            void handlerFunction(const int32_t timerId) override;
        };

    private:
        constexpr static uint8_t MAX_RETRY_COUNT_NO_ACK{3U};
        OnboardclientTxTimerHandler *mOnboardclientTxTimerHandler;
        android::sp<Timer> mTimeOutWireConenction;
        android::sp<Timer> mTimeOutSendUds;
        android::sp<Timer> mTimeOutSendBusy;
        android::sp<Timer> mTimeOutSendFail;
        android::sp<Timer> mTimeOutNoAck;
        uint8_t attemptNoAck;
        void initTimer(void);

    public:
        OnboardclientTxHandler(sp<sl::SLLooper> &looper, const android::sp<OnboardclientManagerService> onboardclientMgrService);
        virtual ~OnboardclientTxHandler() = default;
        OnboardclientTxHandler(OnboardclientTxHandler const &) = default;
        OnboardclientTxHandler &operator=(OnboardclientTxHandler const &) = default;
        OnboardclientTxHandler(OnboardclientTxHandler &&) = delete;
        OnboardclientTxHandler &operator=(OnboardclientTxHandler &&) = delete;
        static OnboardclientTxHandler *getInstance();
        virtual void handleMessage(const android::sp<sl::Message> &handlemsg);
        void startTimer(const int32_t timerId, const uint32_t duration = 60U);
        void stopTimer(const int32_t timerId);
        void resetCounterNoAck();
    };
};
#endif // ONBOARDCLIENT_TX_HANDLER_H
