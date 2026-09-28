
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

#ifndef TIGER_SDK_ONBOARDCLIENTMANAGER_H
#define TIGER_SDK_ONBOARDCLIENTMANAGER_H

#include "Error.h"
#include "Log.h"
#include <utils/SLLooper.h>
#include <utils/Handler.h>
#include <binder/IServiceManager.h>
#include "IOnboardclientManagerServiceType.h"
#include "IOnboardclientManagerService.h"
#include <utils/RefBase.h>
#include <list>
#include <tuple>
#include <vector>
#include <algorithm>
#include "OBCEnum.h"
// #include "OBCPriorityType.h"
// #include "OBCProtocolType.h"
// #include "OBCUdsResponseType.h"
using namespace std;

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

class OnboardclientManager : public android::RefBase
{
public:
    /**
     * @brief destructor function for the OnboardclientManager.
     *
     * @retval  void
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:~OnboardclientManager()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:~OnboardclientManager()"}
     */
    ~OnboardclientManager() override;
    OnboardclientManager(const OnboardclientManager &other) = delete;
    OnboardclientManager &operator=(const OnboardclientManager &other) = delete;
    OnboardclientManager(OnboardclientManager &&) = delete;
    OnboardclientManager &operator=(OnboardclientManager &&) = delete;
    /**
     * @brief instance function for the OnboardclientManager.
     *
     * @retval   OnboardclientManager*
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:instance()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:instance()"}
     */
    static OnboardclientManager *instance();

    /**
     * @brief onNotifyOBD2Event(..) is the function to register receiver for the BpOnboardClientReceiver .
     *
     * @details Wired connection detection notification event
     *
     * @param[in] receiver android::sp< OnboardClientReceiver > interface
     *
     * @retval   error_t   If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onNotifyOBD2Event:registerReceiverOnboardClientReceiverOnNotifyOBD2Event()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onNotifyOBD2Event:registerReceiverOnboardClientReceiverOnNotifyOBD2Event()"}
     */
    error_t registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp<IOnboardClientReceiver> &receiver);
    // todo : add multiple registerReceiverOnboardClientReceiverOnNotifyOBD2Event with different arguments types
    // case 1 : id : onReceive with different id (it was changed)
    // case 2 : receiver for special purpose
    /**
     * @brief onResponseEvent(..) is the function to register receiver for the BpOnboardClientReceiver .
     *
     * @details Notify UDS response
     *
     * @param[in] receiver android::sp< OnboardClientReceiver > interface
     *
     * @retval   error_t   If the receiver is registered successfully, return E_OK, otherwise, return E_ERROR.
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onResponseEvent:registerReceiverOnboardClientReceiverOnResponseEvent()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:OnboardClientReceiver:onResponseEvent:registerReceiverOnboardClientReceiverOnResponseEvent()"}
     */
    error_t registerReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp<IOnboardClientReceiver> &receiver);
    // todo : add multiple registerReceiverOnboardClientReceiverOnResponseEvent with different arguments types
    // case 1 : id : onReceive with different id (it was changed)
    // case 2 : receiver for special purpose

    /**
     * @brief onNotifyOBD2Event(..) is the function to unregister receiver for the BpOnboardClientReceiver .
     *
     * @details Wired connection detection notification event
     *
     * @param[in] receiver android::sp< OnboardClientReceiver > interface
     * @retval   error_t   If the receiver is unregistered successfully, return E_OK, otherwise, return E_ERROR.
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event()"}
     */
    error_t unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
        const android::sp<IOnboardClientReceiver> &receiver);
    /**
     * @brief onResponseEvent(..) is the function to unregister receiver for the BpOnboardClientReceiver .
     *
     * @details Notify UDS response
     *
     * @param[in] receiver android::sp< OnboardClientReceiver > interface
     * @retval   error_t   If the receiver is unregistered successfully, return E_OK, otherwise, return E_ERROR.
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:unregisterReceiverOnboardClientReceiverOnResponseEvent()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:unregisterReceiverOnboardClientReceiverOnResponseEvent()"}
     */
    error_t unregisterReceiverOnboardClientReceiverOnResponseEvent(
        const android::sp<IOnboardClientReceiver> &receiver);
    /**
     * @brief It is the function to reregister receiver for the OnboardclientManager.
     *
     * @retval   error_t   If the receiver is reregistered successfully, return E_OK, otherwise, return E_ERROR.
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:reregisterReceiver()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:reregisterReceiver()"}
     */
    error_t reregisterReceiver();
    // we will use reregister when we reconnect.

    // wishtoUseAPI
    /**
     * @brief connectID
     *
     * It is the function for sendUdsData in the OnboardclientManager.
     *
     * @param[in] connectId connectID
     * @param[in] udsRequest UDS request
     * @retval   error_t   If this function works successfully, return E_OK, otherwise, return E_ERROR.
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:sendUdsData()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:sendUdsData()"}
     */
    uint8_t sendUdsData(
        // sendUdsData 1
        // type : uint16_t
        // name : connectId
        // direction : in
        // is xxClass : NO
        // is receiver : NO
        // len variable :
        // default (optional) argument :
        // init value :
        const uint16_t connectId
        // sendUdsData 2
        // type : android::sp<Buffer>
        // name : udsRequest
        // direction : in
        // is xxClass : NO
        // is receiver : NO
        // len variable :
        // default (optional) argument :
        // init value :
        ,
        const android::sp<Buffer> udsRequest);

    // @CGA_VARIANT_START{"__PUBLIC_SCOPE__:__YOUR_LEGACY_API_CODE__:variant"}
    // ....
    // @CGA_VARIANT___END{"__PUBLIC_SCOPE__:__YOUR_LEGACY_API_CODE__:variant"}

    /**
     * @brief It is the function to get service for the OnboardclientManager.
     *
     * @param[in] who  who sends Death state.
     * @retval  void
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:onBinderDied()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:onBinderDied()"}
     */
    void onBinderDied(const android::wp<android::IBinder> &who);

private:
    /**
     * @brief constructor function for the OnboardclientManager.
     *
     * @retval  void
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:OnboardclientManager()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:OnboardclientManager()"}
     */
    OnboardclientManager();
    /**
     * @brief get service for the OnboardclientManager.
     *
     * @retval  android::sp<IOnboardclientManagerService>
     *
     * @CGA_VARIANT_START{"DOXYGEN:OnboardclientManager:getService()"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:OnboardclientManager:getService()"}
     */
    static android::sp<IOnboardclientManagerService> getService();

#ifdef __UNITTEST__
    android::sp<IOnboardclientManagerService> getService_mock();
#endif

    class ManagerDeathRecipient : public android::IBinder::DeathRecipient
    {
    public:
        /**
         * @brief This is the constructor of DeathRecipient
         *
         * @param[in] parent     onboardclientManager
         * @retval   void
         *
         */
        explicit ManagerDeathRecipient(OnboardclientManager &parent) noexcept : mParent(parent) {}

        /**
         * @brief This is the destructor of DeathRecipient
         *
         * @retval   void
         *
         */
        virtual ~ManagerDeathRecipient() = default;
        ManagerDeathRecipient(const ManagerDeathRecipient &other) = delete;
        ManagerDeathRecipient &operator=(const ManagerDeathRecipient &other) = delete;
        ManagerDeathRecipient(ManagerDeathRecipient &&) = delete;
        ManagerDeathRecipient &operator=(ManagerDeathRecipient &&) = delete;
        /**
         * @brief This function is called when binder is died.
         *
         * @param[in] who     IBinder
         * @retval   void
         *
         */
        virtual void binderDied(const android::wp<android::IBinder> &who) noexcept;

    private:
        OnboardclientManager &mParent;
    };

private:
    static OnboardclientManager *mInstance;
    android::sp<android::IBinder> mToken;
    android::sp<ManagerDeathRecipient> mDeathRecipient;
    android::sp<IOnboardclientManagerService> mOnboardclientService;

    android::sp<IOnboardClientReceiver> mOnboardClientReceiver;
    // todo : question : when we have this variables , i do not know whether I should implement a pur virtual function or not.
    //    depends on each manager's code

    // @CGA_VARIANT_START{"ONBOARDCLIENTMANAGER_H:ONBOARDCLIENTMANAGER()"}
    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"ONBOARDCLIENTMANAGER_H:ONBOARDCLIENTMANAGER()"}
};

#endif // TIGER_SDK_ONBOARDCLIENTMANAGER_H
