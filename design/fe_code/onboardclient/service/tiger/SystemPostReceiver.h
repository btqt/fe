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

#ifndef SYSTEM_POST_RECEIVER_H
#define SYSTEM_POST_RECEIVER_H

#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include <utils/Handler.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/external/mindroid/lang/String.h>
#include <utils/Mutex.h>

/* auto __ appmgr Inheritance CGA start-------------------------------------------------*/
#include <services/ApplicationManagerService/ISystemPostReceiver.h>
#include "../include/OnboardclientInputManager.h"
namespace OBC
{
    class SystemPostReceiver : public BnSystemPostReceiver
    {
    public:
        /**
         * @brief This is the constructor function for the systemPostReceiver.
         *
         * @param[in] handler     android OnboardclientHandler
         * @retval   void
         *
         */
        SystemPostReceiver(const android::sp<OnboardclientInputManager::OnboardclientHandler> handler);
        SystemPostReceiver(const SystemPostReceiver &other) = delete;
        SystemPostReceiver &operator=(const SystemPostReceiver &other) = delete;
        SystemPostReceiver(SystemPostReceiver &&) = delete;
        SystemPostReceiver &operator=(SystemPostReceiver &&) = delete;
        /**
         * @brief This is the function to receive system post event.
         *
         * @param[in] systemPost     android Post
         * @retval   void
         *
         */
        bool onSystemPostReceived(const android::sp<Post> &systemPost) override;

        /**
         * @brief This is the virtual destructor function.
         *
         * @retval   void
         *
         */
        ~SystemPostReceiver() override = default;

    private:
        android::sp<OnboardclientInputManager::OnboardclientHandler> mHandler;

        // @CGA_VARIANT_START{"class_BnSystemPostReceiver:systemPostReceiver"}
        /*
         * Write your own code
         */
        // @CGA_VARIANT___END{"class_BnSystemPostReceiver:systemPostReceiver"}
    };
};
#endif // SYSTEM_POST_RECEIVER_H
