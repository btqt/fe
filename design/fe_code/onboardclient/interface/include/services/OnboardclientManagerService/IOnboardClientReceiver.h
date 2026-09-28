
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

#ifndef IONBOARDCLIENTRECEIVER_MANAGER_RECEIVER_H
#define IONBOARDCLIENTRECEIVER_MANAGER_RECEIVER_H

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include "utils/Buffer.h"
#include "Typedef.h"
#include "OnboardclientCommand.h"
#include "OBCEnum.h"
// #include "OBCPriorityType.h"
// #include "OBCProtocolType.h"
// #include "OBCUdsResponseType.h"
// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}
/*
 * Write your own additional code
 * add your #include
 * add your global declare the function and variables
 * add your function
 */
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__YOUR_CODE__:variant"}

/**
 * @brief IOnboardClientReceiver defines receiver interface to receive the msg from Onboardclient service.
 *
 * @CGA_VARIANT_START{"DOXYGEN:IOnboardClientReceiver:class"}
 * @details
 * @CGA_VARIANT___END{"DOXYGEN:IOnboardClientReceiver:class"}
 *
 * @if (CGA)
 *   @CGA_BNCLASS{BnOnboardClientReceiver}
 * @endif
 */
class IOnboardClientReceiver : public android::IInterface
{
public:
    DECLARE_META_INTERFACE(OnboardClientReceiver); // remove first character substitute(OnboardClientReceiver,"^.","")
    /**
     * @ingroup ONBOARDCLIENTMGR_CB
     *
     * @brief onNotifyOBD2Event : classname BnOnboardClientReceiver : pure virtual function to receive the msg : Wired connection detection notification event
     *
     *
     * @details
     * Whenever the event for which the receiver is registered is occurred, onNotifyOBD2Event() is called.
     * For example, suppose you register a power state event because you want to know
     * whenever the power state changes.
     * In that case, the onNotifyOBD2Event() function is called whenever the power state changes.
     *
     * @CGA_VARIANT_START{"DOXYGEN:IOnboardClientReceiver:onNotifyOBD2Event"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:IOnboardClientReceiver:onNotifyOBD2Event"}
     *
     *
     * @return   void
     *
     */
    virtual void onNotifyOBD2Event() noexcept = 0;
    /**
     * @ingroup ONBOARDCLIENTMGR_CB
     *
     * @brief onResponseEvent : classname BnOnboardClientReceiver : pure virtual function to receive the msg : Notify UDS response
     *
     *
     * @details
     * Whenever the event for which the receiver is registered is occurred, onResponseEvent() is called.
     * For example, suppose you register a power state event because you want to know
     * whenever the power state changes.
     * In that case, the onResponseEvent() function is called whenever the power state changes.
     *
     * @CGA_VARIANT_START{"DOXYGEN:IOnboardClientReceiver:onResponseEvent"}
     * @details
     * @CGA_VARIANT___END{"DOXYGEN:IOnboardClientReceiver:onResponseEvent"}
     *
     * @param[in] message  Response Information
     *
     * @return   void
     *
     */
    virtual void onResponseEvent(const android::sp<OBCResponseEventInfo> message) noexcept = 0;

private:
    uint8_t targetModuleID{0x00U};
    appid_t targetAppID{0x00};

    // @CGA_VARIANT_START{"IOnboardClientReceiver:class_member"}
    /*
     * Write your own code
     */
    // @CGA_VARIANT___END{"IOnboardClientReceiver:class_member"}
};

/**
 * @brief BnOnboardClientReceiver defines binder native interface for transfer the operation(call) with function name and arguments.
 *
 */
class BnOnboardClientReceiver : public android::BnInterface<IOnboardClientReceiver>
{
public:
    /**
     * @brief It is the onTransact function in related to BpOnboardClientReceiver.
     *
     * @param[in] code operation code in binder
     * @param[in] data data parcel in binder
     * @param[in,out] reply reply packet in binder
     * @param[in] flags flags
     * @retval   status_t It returns the status of onTransact function.
     *
     */
    virtual android::status_t onTransact(const uint32_t code, const android::Parcel &data, android::Parcel *const reply, const uint32_t flags);
};

#endif /** IONBOARDCLIENTRECEIVER_MANAGER_RECEIVER_H*/
