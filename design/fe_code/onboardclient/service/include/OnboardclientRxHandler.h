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
 * @date    2023.12.14
 * @version 3.0.00
 */

#ifndef ONBOARDCLIENT_RX_HANDLER_H
#define ONBOARDCLIENT_RX_HANDLER_H

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
    class OnboardclientRxHandler : public sl::Handler
    {
    private:
        static OnboardclientRxHandler *mOnboardclientRxHandler;
        const android::sp<OnboardclientManagerService> mOnboardclientMgrService;

    public:
        constexpr static uint8_t RESPONSE_INITIALIZE{0x11U};
        constexpr static uint8_t WIRED_CONNECTION_PROGRESS{0x61U};
        OnboardclientRxHandler(sp<sl::SLLooper> &looper, const android::sp<OnboardclientManagerService> onboardclientMgrService);
        virtual ~OnboardclientRxHandler() = default;
        OnboardclientRxHandler(OnboardclientRxHandler const &) = default;
        OnboardclientRxHandler &operator=(OnboardclientRxHandler const &) = default;
        OnboardclientRxHandler(OnboardclientRxHandler &&) = delete;
        OnboardclientRxHandler &operator=(OnboardclientRxHandler &&) = delete;
        static OnboardclientRxHandler *getInstance();

        virtual void handleMessage(const android::sp<sl::Message> &handlemsg);
    };
};
#endif // ONBOARDCLIENT_RX_HANDLER_H
