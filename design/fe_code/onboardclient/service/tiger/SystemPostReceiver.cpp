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
 * @date    2023.10.02
 * @version 3.0.00
 */

#define LOG_TAG "SystemPostReceiver"
#include <Log.h>
#include "SystemPostReceiver.h"
namespace OBC
{
    SystemPostReceiver::SystemPostReceiver(const android::sp<OnboardclientInputManager::OnboardclientHandler> handler)
    {
        mHandler = handler;
    }

    bool SystemPostReceiver::onSystemPostReceived(const android::sp<Post> &systemPost)
    {
        const int32_t cmd{systemPost->what};
        switch (cmd)
        {
        case SYS_POST_BOOT_COMPLETED:
        {
            LOGV("systemPostReceiver Boot Complete"); // charles : chage the name (log)
            const android::sp<sl::Message> message{mHandler->obtainMessage(OBC_INPUT_HANDLER::MSG_RECEIVE_BOOT_COMPLETE)};
            (void)message->sendToTarget();
            break;
        }
        case SYS_POST_BOOT_COMPLETED_PRE:
        {
            LOGV("systemPostReceiver Boot Pre"); // charles : chage the name (log)
            const android::sp<sl::Message> message{mHandler->obtainMessage(OBC_INPUT_HANDLER::MSG_OBC_RECEIVE_BOOT_PRE)};
            (void)message->sendToTarget();
            break;
        }
        default:
            break;
        }
        return true;
    }
};
