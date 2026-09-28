

/**
 * @attention Copyright (c) 2019 by LG electronics co, Ltd. All rights reserved.
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
 * @defgroup ONBOARDCLIENTMGR OnboardclientManager
 *
 * @brief OnboardclientManager is
 *
 *  main is starting routine for Onboardclient ManagerService.
 *  - Onboardclient ManagerService
 *  - Onboardclient Manager
 *  - Onboardclient Sample application
 *  - Onboardclient SLDD
 *
 * @cond UML
 *  this is sequence diagram how to do operations through binder.
 * @startuml
 *   box "Application"
 *   participant OnboardclientSampleApplication
 *   participant BnOnboardclientManagerReceiver
 *   participant BpOnboardclientManagerService
 *   end box
 *   participant Binder
 *   box "OnboardclientService"
 *   participant BpOnboardclientManagerReceiver
 *   participant ServiceStub
 *   participant BnOnboardclientManagerService
 *   participant OnboardclientInputManager
 *   participant OnboardclientHandler
 *   end box
 *   box "OtherService"
 *   participant mVifManager
 *   end box
 *   == OnboardclientSampleApplication register to Onboardclient and VIF sends msg to OnboardclientSampleApplication ==
 *     OnboardclientSampleApplication -> BpOnboardclientManagerService : registerReceiver(id,receiver)
 *     BpOnboardclientManagerService -> Binder : receiver >> remote()->onTransact(OP_REGISTER_RECEIVER, Parcel)
 *     Binder -> BnOnboardclientManagerService : onTransact(uint32_t code, const Parcel& data,reply)\n OP_REGISTER_RECEIVER, Parcel >> receiver
 *     BnOnboardclientManagerService -> ServiceStub : registerReceiver(id,receiver)
 *     note right ServiceStub : mReceivers[id].push_back(receiver)\nlinkToDeath() : onReceiverBinderDied(delete it from mReceivers when app dies.)
 *     activate ServiceStub
 *     BnOnboardclientManagerService <- ServiceStub : return E_OK
 *     deactivate ServiceStub
 *     Binder <- BnOnboardclientManagerService : reply->writeInt32
 *     BpOnboardclientManagerService <- Binder : reply.readInt32()
 *     OnboardclientSampleApplication <- BpOnboardclientManagerService : return E_OK
 *     mVifManager -> OnboardclientInputManager : mVifManager->onReceive(uint32_t Sigid, android::sp<Buffer>& buf)
 *     note left OnboardclientInputManager : message->sendToTarget(obtainMessage());
 *     OnboardclientInputManager -> OnboardclientHandler : handleMessage(msg)
 *     note right OnboardclientHandler : switch Onboardclient_RECEIVE_FROM_VIF
 *     OnboardclientHandler -> OnboardclientInputManager : messagefromVIF(msg)
 *     note right OnboardclientInputManager : transferDatabyVIF()
 *     OnboardclientInputManager -> ServiceStub : queryReceiverByID()
 *     note right ServiceStub : mReceivers[id].onReceive(data);
 *     ServiceStub -> BpOnboardclientManagerReceiver : onReceive(data)
 *     BpOnboardclientManagerReceiver -> Binder : (void)remote()->transact(TRANSACT_ONRECEIVE, data, &reply);
 *     Binder -> BnOnboardclientManagerReceiver : onTransact(TRANSACT_ONRECEIVE)
 *     BnOnboardclientManagerReceiver -> OnboardclientSampleApplication : OnboardclientSampleReceiver::onReceive()
 *     note right OnboardclientSampleApplication : message->sendToTarget(obtainMessage());
 *     OnboardclientSampleApplication -> OnboardclientSampleApplication : OnboardclientSampleApplication::OnboardclientHandler::handleMessage(msg)
 * @enduml
 * @endcond
 *
 * @version
 *  1.3  add Low Level Design Documents (This is doxygen comments)
 */
#include <unistd.h>

#include "Log.h"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/process_name.h>

#include "corebase/SystemService.h"
#include "utils/Handler.h"
#include "utils/Observer.h"
#include <utils/Looper.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>

#include "../include/OnboardclientManagerService.h"

/**
 * @brief It is the main functionfor the OnboardclientManagerService.
 *
 * @param[in] argc argument count
 * @param[in] argv argument value lists
 * @retval   bool
 *
 * @CGA_VARIANT_START{main_MISRA}
 * @MISRA{MISRA C++-2008 Rule 3-9-2,This is intended design}
 * @CGA_VARIANT___END{main_MISRA}
 *
 */
#ifdef __UNITTEST__
int UT_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
try
{
        LOGI("OnboardclientManagerService: main");
        OBC::OnboardclientManagerService *const service{new OBC::OnboardclientManagerService()};
        service->instantiate();
        (void)service->onInit();

#ifndef __UNITTEST__
        android::ProcessState::self()->startThreadPool();
#endif
        (void)service->onStart();

#ifndef __UNITTEST__
        android::IPCThreadState::self()->joinThreadPool();
#endif
}
catch (...)
{
        LOGE("throw exception");
}
