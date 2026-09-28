
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

#define LOG_TAG "onboardclientApplication"
#include <Log.h>

#include <stdio.h>

#include "onboardclientApplication.h"
#include "HMIType.h"

static android::sp<Application> gApp;

onboardclientApplication::onboardclientApplication() {
}

onboardclientApplication::~onboardclientApplication() {
}

void onboardclientApplication::onCreate() {
    LOGV("onboardclientApplication onCreate()");


    mAppManager = interface_cast<IApplicationManagerService>
        (defaultServiceManager()->getService(String16("service_layer.ApplicationManagerService")));
    mHandler = new onboardclientHandler(*this);
    mHandler->obtainMessage(onboardclientHandler::MSG_SEND_POST)->sendToTarget();

    // System Post receiver
    mSystemReceiver = new SystemPostReceiver(*this);
    mAppManager->registerSystemPostReceiver(mSystemReceiver, SYS_POST_ALL);
    mAppManager->registerSystemPostReceiver(mSystemReceiver, SYS_POST_BOOT_COMPLETED | SYS_POST_BOOT_COMPLETED_PRE | SYS_POST_PROVISIONING_COMPLETED);

    monboardclientMgrService = onboardclientManager::instance();

    monboardclientMgrReceiver = new onboardclientReceiver(*this);
    monboardclientMgrService->registerReceiver(0x1111, monboardclientMgrReceiver);

    android::sp<onboardclientData> newonboardclientData = new onboardclientData();
    uint8_t payload[BUFSIZ]= {0,};
//    newonboardclientData->setData(0x1111, 0, (uint8_t)DID_READ_REQUEST, payload);

    LOGV("onboardclient App wishtoUseAPI_1 Call");

    LOGV("boot complete? %d", mAppManager->getBootCompleted());
}

void onboardclientApplication::onDestroy() {
    LOGV("onDestroy()");
    // unregister receiver
    mAppManager->unregisterSystemPostReceiver(mSystemReceiver);
}

void onboardclientApplication::onActive(int32_t param, std::string param2) {
    LOGE("onboardclientApplication onActive");
    std::string app="onboardclient";
    LOGE("onboardclientApplication IsActivem = %d", mAppManager->isActive(app));

}

void onboardclientApplication::onInactive(int32_t param, std::string param2) {
}

void onboardclientApplication::onPostReceived(const sp<Post>& post) {
    LOGV("onPostReceived(), what = %d, arg1 = %d, arg2 = %d",
        post->what, post->arg1, post->arg2);
}

void onboardclientApplication::test() {
     LOGD("test");
}

bool onboardclientApplication::onSystemPostReceived(const sp<Post>& systemPost) {
    LOGV("onSystemPostReceived(), what = %d", systemPost->what);
}

/**
* Write your handler code below!
*/
void onboardclientApplication::onboardclientHandler::handleMessage(const sp<sl::Message>& msg) {

    switch(msg->what) {

        case MSG_FROM_MANAGER: {
            LOGV("onboardclient App handlemMessage from onboardclient Manager");
            break;
        }

        case MSG_TEST_WATCHDOG:
            LOGD("onboardclientApplication watchdog test start");
            while(true) {
            }
            break;
        default:
            break;
    }
}

#ifdef __cplusplus
extern "C" class Application* createApplication() {
    printf("create onboardclientApplication");
    gApp = new onboardclientApplication;
    return gApp.get();
}

extern "C" void destroyApplication(class Application* application) {
    delete (onboardclientApplication*)application;
}
#endif
