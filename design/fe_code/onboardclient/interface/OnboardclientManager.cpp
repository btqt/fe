
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

#include <binder/Parcel.h>
#include <binder/IServiceManager.h>
#include <utils/String16.h>

#include "Error.h"
#include "Log.h"
#include <utils/SLLooper.h>
#include <utils/Handler.h>
#include <binder/IServiceManager.h>
#include "./include/services/OnboardclientManagerService/IOnboardclientManagerServiceType.h"
#include "./include/services/OnboardclientManagerService/OnboardclientManager.h"
#include "./include/services/OnboardclientManagerService/IOnboardclientManagerService.h"
#include <utils/RefBase.h>
#include <list>
#include <tuple>
#include <algorithm>
#include <vector>

using namespace std;

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

/**
 * @MISRA{MISRA C++-2008 Rule 16-0-3,"This is intended design"}
 */
#undef LOG_TAG
#define LOG_TAG "OnboardclientManager"

OnboardclientManager *OnboardclientManager::mInstance{nullptr};

OnboardclientManager *OnboardclientManager::instance()
{
    // @CGA_VARIANT_START{"OnboardclientManager:instance()"}
    if (mInstance == nullptr)
    {
        mInstance = new OnboardclientManager();
    }

    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"OnboardclientManager:instance()"}

    return mInstance;
}

OnboardclientManager::OnboardclientManager()
{
    // @CGA_VARIANT_START{"OnboardclientManager:OnboardclientManager()"}
    LOGV("OnboardclientManager constructor - entry");
    mToken = new android::BBinder();
    mOnboardclientService = getService();
    if (mOnboardclientService != nullptr)
    {
        mDeathRecipient = new ManagerDeathRecipient(*this);
        (void)android::IInterface::asBinder(mOnboardclientService)->linkToDeath(mDeathRecipient);
    }
    LOGV("OnboardclientManager constructor - exit");

    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"OnboardclientManager:OnboardclientManager()"}
}

OnboardclientManager::~OnboardclientManager()
{
    // @CGA_VARIANT_START{"OnboardclientManager:~OnboardclientManager()"}
    LOGV("~OnboardclientManager destructor - entry");

    mOnboardclientService = nullptr;

    LOGV("~OnboardclientManager destructor - exit");

    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"OnboardclientManager:~OnboardclientManager()"}
}

android::sp<IOnboardclientManagerService> OnboardclientManager::getService()
{
    LOGV("OnboardclientManager getService() - entry");

    // @CGA_VARIANT_START{"OnboardclientManager:getService()"}
    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"OnboardclientManager:getService()"}

    return android::interface_cast<IOnboardclientManagerService>(android::defaultServiceManager()->getService(android::String16(ONBOARDCLIENT_SRV_NAME)));
}

void OnboardclientManager::onBinderDied(const android::wp<android::IBinder> &who)
{

    // @CGA_VARIANT_START{"OnboardclientManager:onBinderDied()"}
    LOGE("OnboardclientService onBinderDied - %s", __func__);

    /**
     * @MISRA{MISRA C++-2008 Rule 16-2-1,This is intended design}
     */
    if (android::IInterface::asBinder(mOnboardclientService) == who)
    {
        LOGE("Died Onboardclient Service!! - %s", __func__);
        mOnboardclientService = getService();
        if (mOnboardclientService != nullptr)
        {
            (void)android::IInterface::asBinder(mOnboardclientService)->linkToDeath(mDeathRecipient);
            (void)reregisterReceiver();
        }
    }

    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"OnboardclientManager:onBinderDied()"}
}

error_t OnboardclientManager::registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
    const android::sp<IOnboardClientReceiver> &receiver)
{
    error_t _tidl_result{E_OK};

    // @CGA_VARIANT_START{"OnboardclientManager:OnboardClientReceiver:onNotifyOBD2Event:registerReceiverOnboardClientReceiverOnNotifyOBD2Event()"}
    if (mOnboardclientService != nullptr)
    {
        LOGV("interface registerReceiverOnboardClientReceiverOnNotifyOBD2Event : OnboardClientReceiver : onNotifyOBD2Event");

        _tidl_result = mOnboardclientService->registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
            receiver);

        if (_tidl_result == E_ERROR)
        {
            LOGE("result == ERROR!! - %s", __func__);
        }
        else
        {
            mOnboardClientReceiver = receiver;
        }
    }
    else
    {
        LOGE("OnboardclientManager %s() fail : mOnboardclientService is empty", __func__);
        _tidl_result = E_ERROR;
    }

    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"OnboardclientManager:OnboardClientReceiver:onNotifyOBD2Event:registerReceiverOnboardClientReceiverOnNotifyOBD2Event()"}

    return _tidl_result;
}

error_t OnboardclientManager::registerReceiverOnboardClientReceiverOnResponseEvent(
    const android::sp<IOnboardClientReceiver> &receiver)
{
    error_t _tidl_result{E_OK};

    // @CGA_VARIANT_START{"OnboardclientManager:OnboardClientReceiver:onResponseEvent:registerReceiverOnboardClientReceiverOnResponseEvent()"}
    if (mOnboardclientService != nullptr)
    {
        LOGV("interface registerReceiverOnboardClientReceiverOnResponseEvent : OnboardClientReceiver : onResponseEvent");

        _tidl_result = mOnboardclientService->registerReceiverOnboardClientReceiverOnResponseEvent(
            receiver);

        if (_tidl_result == E_ERROR)
        {
            LOGE("result == ERROR!! - %s", __func__);
        }
        else
        {
            mOnboardClientReceiver = receiver;
        }
    }
    else
    {
        LOGE("OnboardclientManager %s() fail : mOnboardclientService is empty", __func__);
        _tidl_result = E_ERROR;
    }

    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"OnboardclientManager:OnboardClientReceiver:onResponseEvent:registerReceiverOnboardClientReceiverOnResponseEvent()"}

    return _tidl_result;
}

error_t OnboardclientManager::reregisterReceiver()
{
    // @CGA_VARIANT_START{"OnboardclientManager:reregisterReceiver()"}
    LOGV("interface reregisterReceiver");
    error_t _tidl_result{E_OK};

    if (mOnboardclientService == nullptr)
    {
        LOGE("OnboardclientManager %s() fail : mOnboardclientService is empty", __func__);
        _tidl_result = E_ERROR;
    }
    else
    {
        if (mOnboardClientReceiver != nullptr)
        {
            _tidl_result = mOnboardclientService->registerReceiverOnboardClientReceiverOnNotifyOBD2Event(mOnboardClientReceiver);
            if (_tidl_result == E_ERROR)
            {
                LOGE("ret_val == ERROR!! - OnboardClientReceiver : onNotifyOBD2Event :  - %s", __func__);
            }
            else
            {
                _tidl_result = mOnboardclientService->registerReceiverOnboardClientReceiverOnResponseEvent(mOnboardClientReceiver);
                if (_tidl_result == E_ERROR)
                {
                    LOGE("ret_val == ERROR!! - OnboardClientReceiver : onResponseEvent : OBCResponseEventInfo - %s", __func__);
                }
            }
        }
    }

    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"OnboardclientManager:reregisterReceiver()"}

    return _tidl_result;
}

error_t OnboardclientManager::unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
    const android::sp<IOnboardClientReceiver> &receiver)
{
    error_t _tidl_result{E_OK};

    // @CGA_VARIANT_START{"OnboardclientManager:unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event()"}
    if (mOnboardclientService != nullptr)
    {
        LOGV("interface unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event : OnboardClientReceiver : onNotifyOBD2Event");

        _tidl_result = mOnboardclientService->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
            receiver);

        if (_tidl_result == E_ERROR)
        {
            LOGE("_tidl_result == ERROR!! : unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event : OnboardClientReceiver onNotifyOBD2Event - %s", __func__);
        }
        else
        {
            /* todo
                mOnboardClientReceiver = nullptr;
            */
        }
    }
    else
    {
        LOGE("OnboardclientManager %s() fail : mOnboardclientService is empty", __func__);
        _tidl_result = E_ERROR;
    }

    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"OnboardclientManager:unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event()"}

    return _tidl_result;
}

error_t OnboardclientManager::unregisterReceiverOnboardClientReceiverOnResponseEvent(
    const android::sp<IOnboardClientReceiver> &receiver)
{
    error_t _tidl_result{E_OK};

    // @CGA_VARIANT_START{"OnboardclientManager:unregisterReceiverOnboardClientReceiverOnResponseEvent()"}
    if (mOnboardclientService != nullptr)
    {
        LOGV("interface unregisterReceiverOnboardClientReceiverOnResponseEvent : OnboardClientReceiver : onResponseEvent");

        _tidl_result = mOnboardclientService->unregisterReceiverOnboardClientReceiverOnResponseEvent(receiver);

        if (_tidl_result == E_ERROR)
        {
            LOGE("_tidl_result == ERROR!! : unregisterReceiverOnboardClientReceiverOnResponseEvent : OnboardClientReceiver onResponseEvent - %s", __func__);
        }
        else
        {
            /* todo
                mOnboardClientReceiver = nullptr;
            */
        }
    }
    else
    {
        LOGE("OnboardclientManager %s() fail : mOnboardclientService is empty", __func__);
        _tidl_result = E_ERROR;
    }

    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"OnboardclientManager:unregisterReceiverOnboardClientReceiverOnResponseEvent()"}

    return _tidl_result;
}

// wishtoUseAPI
uint8_t OnboardclientManager::sendUdsData(const uint16_t connectId, const android::sp<Buffer> udsRequest)
{
    // @CGA_VARIANT_START{"OnboardclientManager::sendUdsData"}
    // write your own code here
    uint8_t res{1U}; // OBC_NEGATIVE
    if (mOnboardclientService != nullptr)
    {
        res = mOnboardclientService->sendUdsData(connectId, udsRequest);
    }
    return res;
    // @CGA_VARIANT___END{"OnboardclientManager::sendUdsData"}
}

// @CGA_VARIANT_START{"__PUBLIC_SCOPE__:__YOUR_LEGACY_API_CODE__:variant"}
// ...
// @CGA_VARIANT___END{"__PUBLIC_SCOPE__:__YOUR_LEGACY_API_CODE__:variant"}

void OnboardclientManager::ManagerDeathRecipient::binderDied(const android::wp<android::IBinder> &who) noexcept
{
    mParent.onBinderDied(who);
}
