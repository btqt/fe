#ifndef COMMUNICATION_MANAGER_ADAPTER_H
#define COMMUNICATION_MANAGER_ADAPTER_H
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
#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include <utils/Handler.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/external/mindroid/lang/String.h>
#include <utils/Mutex.h>

#include <binder/IServiceManager.h>

#include "services/CommunicationManagerService/ICommunicationReceiver.h"
#include "services/CommunicationManagerService/ICommunicationManagerService.h"
#include "services/CommunicationManagerService/ICommunicationManagerServiceType.h"
#include "services/CommunicationManagerService/CommunicationData.h"
#include <services/CommunicationManagerService/CM_Protocol.h>
#include "OnboardclientInputManager.h"
#include "ServiceDeathReceipient.hpp"
namespace OBC
{
    class CommunicationManagerAdapter : public android::RefBase
    {
    public:
        constexpr static uint8_t REQUEST_INITIALIZE{0x10U};
        CommunicationManagerAdapter();
        ~CommunicationManagerAdapter() noexcept override;
        CommunicationManagerAdapter(CommunicationManagerAdapter const &) = default;
        CommunicationManagerAdapter &operator=(CommunicationManagerAdapter const &) = default;
        CommunicationManagerAdapter(CommunicationManagerAdapter &&) = delete;
        CommunicationManagerAdapter &operator=(CommunicationManagerAdapter &&) = delete;
        static CommunicationManagerAdapter *getInstance();
        void registerService();
        static void requestWireMonitoring();
        static error_t sendUdsDataToMcu(android::sp<CommunicationData> commData);
        static error_t sendUdsDataToMcu(const uint8_t type, const uint8_t category, const uint8_t cmd, const uint8_t cmd2, const sp<Buffer> payload);

    private:
        void onBinderDied(const android::wp<android::IBinder> &who);
        static CommunicationManagerAdapter *mCommunicationManagerAdapter;
        android::sp<sl::Handler> mOnboardclientInputHandler;
        android::sp<ServiceDeathRecipient> mServiceDeathRecipient{nullptr};
        android::sp<ICommunicationManagerService> mCommService{nullptr};
        android::sp<ICommunicationReceiver> mCommReceiver{nullptr};
        mutable android::Mutex mDiedLock;
    };

    class CommunicationReceiver : public BnCommunicationReceiver
    {
    public:
        explicit CommunicationReceiver(CommunicationManagerAdapter &commRcv);
        virtual ~CommunicationReceiver() = default;
        CommunicationReceiver(CommunicationReceiver const &) = delete;
        CommunicationReceiver &operator=(CommunicationReceiver const &) = delete;
        CommunicationReceiver(CommunicationReceiver &&) = delete;
        CommunicationReceiver &operator=(CommunicationReceiver &&) = delete;
        error_t onReceive(sp<CommunicationData> &commData) override;

    private:
        android::sp<sl::Handler> mHandler;
        CommunicationManagerAdapter &mCommunicationManagerAdapter;
    };
};
#endif // COMMUNICATION_MANAGER_ADAPTER_H
