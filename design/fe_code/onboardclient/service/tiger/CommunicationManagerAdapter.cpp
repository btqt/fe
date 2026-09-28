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

#define LOG_TAG "CommunicationManagerAdapter"
#include <Log.h>
#include "CommunicationManagerAdapter.h"
#include "OnboardclientRxHandler.h"
#include "OnboardclientTxHandler.h"
#include <climits>
namespace OBC
{
    CommunicationManagerAdapter *CommunicationManagerAdapter::mCommunicationManagerAdapter{nullptr};
    CommunicationManagerAdapter::CommunicationManagerAdapter() : android::RefBase()
    {
        LOGI("CommunicationManagerAdapter Constructor");
        mCommunicationManagerAdapter = this;
        mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who)
                                                           { this->onBinderDied(who); });
    }

    CommunicationManagerAdapter::~CommunicationManagerAdapter() noexcept
    {
        if (CommunicationManagerAdapter::mCommunicationManagerAdapter != nullptr)
        {
            delete mCommunicationManagerAdapter;
            mCommunicationManagerAdapter = nullptr;
        }
    }

    CommunicationManagerAdapter *CommunicationManagerAdapter::getInstance()
    {
        if (mCommunicationManagerAdapter == nullptr)
        {
            mCommunicationManagerAdapter = new CommunicationManagerAdapter();
        }
        return mCommunicationManagerAdapter;
    }

    void CommunicationManagerAdapter::registerService()
    {
        LOGI("CommunicationManagerAdapter registerService");
        mOnboardclientInputHandler = OnboardclientInputManager::OnboardclientHandler::getInstance();
        mCommService = android::interface_cast<ICommunicationManagerService>(
            android::defaultServiceManager()->getService(android::String16("service_layer.CommunicationManagerService")));
        if (mCommService != nullptr)
        {
            LOGI("MqttManagerAdapter registered");
            if (mCommReceiver != nullptr)
            {
                mCommReceiver.clear();
            }

            mCommReceiver = android::sp<CommunicationReceiver>(new CommunicationReceiver(*this));
            (void)mCommService->registerReceiver(mCommReceiver, OBC_COMM_ENUM::CATEGORY_OBC); // Receiver : mCommReceiver , Category : CATEGORY_OBC
#ifdef _MORE_MARSHM
            (void)android::IInterface::asBinder(mCommService)->linkToDeath(mServiceDeathRecipient);
#else  // !(_MORE_MARSHM)
            (void)mCommService->asBinder()->linkToDeath(mServiceDeathRecipient);
#endif // _MORE_MARSHM
       // Start Wire notification
            (void)OnboardclientTxHandler::getInstance()->obtainMessage(OBC_TX_HANDLER::CMD_TX_START_MONITORING_WIRE_CONNECTION)->sendToTarget();
            ;
        }
        else
        {
            LOGE("Register comm service fail, try again after 500ms");
            if (mOnboardclientInputHandler != nullptr)
            {
                (void)mOnboardclientInputHandler->sendMessageDelayed(mOnboardclientInputHandler->obtainMessage(OBC_INPUT_HANDLER::MSG_OBC_CONNECT_TO_COMMMGR),
                                                                     OBC_COMMON::TIME_SEND_RETRY_DELAY_MS);
            }
        }
    }

    void CommunicationManagerAdapter::requestWireMonitoring()
    {
        LOGI("Request wire monitoring");
        const sp<Buffer> payload{new Buffer()};
        payload->setSize(0);
        const error_t result{sendUdsDataToMcu(static_cast<uint8_t>(TYPE_REQUEST), OBC_COMM_ENUM::CATEGORY_OBC, REQUEST_INITIALIZE, OBC_COMM_ENUM::CMD2_OBC_DEFAULT, payload)};
        if (result != static_cast<int32_t>(E_OK))
        {
            LOGD("Error code: %d", result);
        }
        else
        {
            // Do Nothing
        }
    }

    void CommunicationManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who)
    {
        LOGI("CommunicationManagerAdapter onBinderDied");
        const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
#ifdef _MORE_MARSHM
        if ((mCommService != nullptr) && (android::IInterface::asBinder(mCommService) == who))
        {
#else  // !(_MORE_MARSHM)
        if (mCommService != nullptr && mCommService->asBinder() == who)
        {
#endif // _MORE_MARSHM
            if (mOnboardclientInputHandler != nullptr)
            {
                (void)mOnboardclientInputHandler->obtainMessage(OBC_INPUT_HANDLER::MSG_OBC_CONNECT_TO_COMMMGR)->sendToTarget();
            }
        }
        else
        {
            // Do nothing
        }
    }

    error_t CommunicationManagerAdapter::sendUdsDataToMcu(android::sp<CommunicationData> commData)
    {
        const android::sp<ICommunicationManagerService> mCommManager{android::interface_cast<ICommunicationManagerService>(
            android::defaultServiceManager()->getService(android::String16("service_layer.CommunicationManagerService")))};
        error_t res{E_OK};
        if (mCommManager.get() != nullptr)
        {
            res = mCommManager->sendDataToMcu(commData);
        }
        else
        {
            res = E_ERROR;
            LOGE("Communication Manager is null!!");
        }
        return res;
    }

    error_t CommunicationManagerAdapter::sendUdsDataToMcu(const uint8_t type, const uint8_t category, const uint8_t cmd, const uint8_t cmd2, const sp<Buffer> payload)
    {
        const android::sp<ICommunicationManagerService> mCommManager{android::interface_cast<ICommunicationManagerService>(
            android::defaultServiceManager()->getService(android::String16("service_layer.CommunicationManagerService")))};
        error_t res{E_OK};
        if (mCommManager.get() != nullptr)
        {
            android::sp<CommunicationData> commData{new CommunicationData()};
            if (commData != nullptr)
            {
                commData->type = type;
                commData->category = category;
                commData->cmd = cmd;
                commData->cmd2 = cmd2;
                const uint32_t tmp{payload->size()};
                if (tmp < static_cast<uint32_t>(INT32_MAX))
                {
                    commData->payload->setTo(payload->data(), static_cast<int32_t>(tmp));
                }
                res = mCommManager->sendDataToMcu(commData);
            }
            else
            {
                res = E_ERROR;
            }
        }
        else
        {
            res = E_ERROR;
            LOGE("Communication Manager is null!!");
        }
        return res;
    }

    CommunicationReceiver::CommunicationReceiver(CommunicationManagerAdapter &commRcv) : mCommunicationManagerAdapter(commRcv)
    {
    }

    error_t CommunicationReceiver::onReceive(sp<CommunicationData> &commData)
    {
        error_t result{E_OK};
        if (commData->category == OBC_COMM_ENUM::CATEGORY_OBC)
        {
            LOGV("Receive form CommMgr");
            const android::sp<sl::Message> message{OnboardclientRxHandler::getInstance()->obtainMessage(OBC_RX_HANDLER::MSG_OBC_RECEIVE_FROM_COMM)};
            message->spRef = commData;
            const bool tmpResult{message->sendToTarget()};
            if (tmpResult == false)
            {
                result = E_ERROR;
            }
            else
            {
                result = E_OK;
            }
        }
        return result;
    }
};
