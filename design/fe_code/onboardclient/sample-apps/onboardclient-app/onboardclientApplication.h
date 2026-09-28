
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
 */

#ifndef _TEMPLATE_APPLICATION_H_
#define _TEMPLATE_APPLICATION_H_

#include <utils/SLLooper.h>
#include <utils/Handler.h>
#include <binder/IServiceManager.h>

#include <corebase/application/Application.h>
#include <services/ApplicationManagerService/ISystemPostReceiver.h>
#include <services/ApplicationManagerService/IApplicationManagerService.h>
#include "services/onboardclientManagerService/onboardclientCommand.h"
#include "services/onboardclientManagerService/IonboardclientManagerService.h"
#include "services/onboardclientManagerService/IonboardclientManagerReceiver.h"
#include "services/onboardclientManagerService/onboardclientManager.h"

class onboardclientApplication : public Application {
private:
    /**
    * Write your own handler class inside your application class
    */
    class onboardclientHandler : public sl::Handler {
        public:
            static const uint32_t MSG_FROM_MANAGER = 0;
            static const uint32_t MSG_TEMP_1 = 1;
            static const uint32_t MSG_TEST_WATCHDOG = 2;
            static const uint32_t MSG_SEND_POST = 3;

            onboardclientHandler(onboardclientApplication &app) : mApp(app) {}
            virtual ~onboardclientHandler() {}
            virtual void handleMessage(const sp<sl::Message>& msg);
        private:
            onboardclientApplication &mApp;
    };

    /**
     * SystemPostReceiver to receive system post event
     *
     */
    bool onSystemPostReceived(const sp<Post>& systemPost);

    class SystemPostReceiver : public BnSystemPostReceiver
    {
    public:
        SystemPostReceiver(onboardclientApplication &app) : mApp(app) {}
        virtual bool onSystemPostReceived(const sp<Post>& systemPost)
        {
            return mApp.onSystemPostReceived(systemPost);
        }
    private:
            onboardclientApplication &mApp;
    };


    class onboardclientReceiver : public BnOnboardclientManagerReceiver {
    public:
        onboardclientReceiver(onboardclientApplication &app) : mApp(app) {}

        virtual void onReceive(android::sp<onboardclientData>& onboardclientData){
            LOGV("onboardclient App onboardclientReceiver onReceive");
            mApp.onReceive();
        }

    private:
        onboardclientApplication &mApp;
    };

    void onReceive(void){mHandler->obtainMessage(onboardclientHandler::MSG_FROM_MANAGER)->sendToTarget();}

    sp<ISystemPostReceiver> mSystemReceiver;
    sp<IApplicationManagerService> mAppManager;
    android::sp<onboardclientManager> monboardclientMgrService;
    android::sp<IOnboardclientManagerReceiver> monboardclientMgrReceiver;

    /**
    * Declare handler to use
    */
    sp<onboardclientHandler> mHandler;

public:
    onboardclientApplication();
    virtual ~onboardclientApplication();

    /**
    * Application has two lifecycle method, onCreate() and onDestroy()
    */
    virtual void onCreate();
    virtual void onDestroy();
    virtual void onActive(const int32_t param, std::string param2);
    virtual void onInactive(const int32_t param, std::string param2);

    /**
    * Callback method
    */
    virtual void onPostReceived(const sp<Post>& post);

    /**
    * your own method
    */
    void test();
};
#endif // _TEMPLATE_APPLICATION_H_
