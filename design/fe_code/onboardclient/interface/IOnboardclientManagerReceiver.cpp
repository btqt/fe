
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

/**
 * @MISRA{MISRA C++-2008 Rule 16-2-1,This is intended design}
 */
#define LOG_TAG "IOnboardclientManagerReceiver"

#include <binder/Parcel.h>

#include "./include/services/OnboardclientManagerService/IOnboardClientReceiver.h"

#include "./include/services/OnboardclientManagerService/OnboardclientCommand.h"

#include "Log.h"
#include "Error.h"

enum
{
    TRANSACT_ONRECEIVE = android::IBinder::FIRST_CALL_TRANSACTION,
    TRANSACT_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT,
    TRANSACT_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT,
    TRANSACT_ONBOARDCLIENT_COMMAND
};

class BpOnboardClientReceiver : public android::BpInterface<IOnboardClientReceiver>
{
public:
    /**
     * @brief It is the constructor for the BpOnboardClientReceiver.
     *
     * @param[in] impl  android binder implementation
     * @retval   void
     *
     */
    BpOnboardClientReceiver(const android::sp<android::IBinder> &impl) noexcept : BpInterface<IOnboardClientReceiver>(impl)
    {
    }

    /**
     * @brief It is the OnboardClientReceiver function for the BpOnboardClientReceiver.
     *
     *
     * @retval   void
     *
     */
    virtual void onNotifyOBD2Event() noexcept
    {
        android::Parcel data{};
        android::Parcel reply{};

        LOGV("OnboardClientReceiver onNotifyOBD2Event Bp start : return type is void");

        (void)data.writeInterfaceToken(IOnboardClientReceiver::getInterfaceDescriptor());

        (void)remote()->transact(TRANSACT_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT, data, &reply);

        // ISSUE inOut
        LOGV("OnboardClientReceiver onNotifyOBD2Event Bp end");

        // ISSUE inOut , return
    }
    /**
     * @brief It is the OnboardClientReceiver function for the BpOnboardClientReceiver.
     *
     * @param[in] message OBCResponseEventInfo : Response Information
     *
     * @retval   void
     *
     */
    virtual void onResponseEvent(const android::sp<OBCResponseEventInfo> message) noexcept
    {
        android::Parcel data{};
        android::Parcel reply{};

        // Argument  OnboardClientReceiver onResponseEvent 1 : OBCResponseEventInfo  message  [in]  [CallBack Argument]

        LOGV("OnboardClientReceiver onResponseEvent Bp start : return type is void");

        (void)data.writeInterfaceToken(IOnboardClientReceiver::getInterfaceDescriptor());
        LOGV("OnboardClientReceiver onResponseEvent Bp 1: 1 write message");
        (void)message->writeToParcel(&data);
        // (void)data.readFromParcel ( message );

        (void)remote()->transact(TRANSACT_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT, data, &reply);

        // ISSUE inOut
        LOGV("OnboardClientReceiver onResponseEvent Bp end");

        // ISSUE inOut , return
    }
};

IMPLEMENT_META_INTERFACE(OnboardClientReceiver, "service_layer.IOnboardClientReceiver");

// ----------------------------------------------------------------------

android::status_t BnOnboardClientReceiver::onTransact(const uint32_t code, const android::Parcel &data, android::Parcel *const reply, const uint32_t flags)
{
    switch (code)
    {
    case TRANSACT_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT:
    {

        LOGV("OnboardClientReceiver onNotifyOBD2Event Bn TRANSACT_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT");
        CHECK_INTERFACE(OnboardClientReceiver, data, reply);

        onNotifyOBD2Event();

        // ISSUE inOut

        // delete buffer with new()
    }
    break;
    case TRANSACT_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT:
    {
        // Argument  OnboardClientReceiver onResponseEvent 1 : OBCResponseEventInfo  message  [in]  [CallBack Argument]

        LOGV("OnboardClientReceiver onResponseEvent Bn TRANSACT_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT");
        CHECK_INTERFACE(OnboardClientReceiver, data, reply);
        LOGV("OnboardClientReceiver onResponseEvent Bn 1 TRANSACT_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT : 1 read message");
        const android::sp<OBCResponseEventInfo> _local_message{new OBCResponseEventInfo()};
        (void)_local_message->readFromParcel((const android::Parcel &)data);
        // OBCResponseEventInfo _local_message;
        // _local_message = static_cast<OBCResponseEventInfo>(data.writeToParcel()); // ? need static_cast

        onResponseEvent(_local_message);

        // ISSUE inOut

        // delete buffer with new()
    }
    break;
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }

    return E_OK;
}
