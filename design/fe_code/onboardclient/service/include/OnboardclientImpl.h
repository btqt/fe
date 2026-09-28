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
 * @date    2023.12.22
 * @version 3.0.00
 */

#ifndef ONBOARDCLIENT_IMPL_H
#define ONBOARDCLIENT_IMPL_H
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <sstream>
#include <iomanip>

#include <utils/Handler.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/external/mindroid/lang/String.h>
#include <utils/Mutex.h>
#include <thread>
#include <queue>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <random>
#include <Log.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <services/CommunicationManagerService/ICommunicationManagerService.h>
#include <services/CommunicationManagerService/ICommunicationManagerServiceType.h>
#include "services/CommunicationManagerService/CommunicationData.h"
#include <services/CommunicationManagerService/CM_Protocol.h>

#include "services/OnboardclientManagerService/OBCEnum.h"
#include "services/OnboardclientManagerService/OnboardclientCommand.h"
#include "services/OnboardclientManagerService/IOnboardclientManagerService.h"
#include "services/OnboardclientManagerService/IOnboardClientReceiver.h"

#include <unordered_map>

#include "OnboardclientManagerService.h"
#include "Common_def.h"
// using namespace std;
namespace OBC
{

    class OnboardclientImpl : public android::RefBase
    {
    private:
        /* data */
        static OnboardclientImpl *mOnboardclientImpl;
        std::unordered_map<uint16_t, std::pair<uint8_t, uint32_t>> mpConnectedCAN;
        std::unordered_map<uint32_t, std::pair<uint8_t, uint16_t>> mpCanIdConnectedId;
        std::unordered_map<uint16_t, uint16_t> mpConnectIdTimeout;
        std::mutex mMutexCommDataQueue;
        std::mutex mMutexDiagData;
        mutable Mutex mMutexRequestData;
        mutable Mutex mMutexResponseData;
        std::queue<sp<CommunicationData>> mConnectQueue;
        std::queue<sp<CommunicationData>> mDisconnectQueue;
        std::deque<sp<CommunicationData>> mUdsResponseQueue;
        std::queue<sp<CommunicationData>> mUdsAckQueue;
        std::deque<std::pair<uint16_t, android::sp<Buffer>>> mUdsReqQueue;
        std::condition_variable mWaitConditionCommDataQueue;
        uint32_t RECEIVE_QUEUE_MAX_SIZE; // 64kb * 50
        uint8_t attemptTransErr;
        uint8_t attemptBusyErr;
        bool isProcessing;
        uint16_t currentProcessingId;
        uint16_t makeConnectId(uint8_t protocolType, uint32_t canId);

        bool startConnectCanThread(const android::sp<OBCTransportInfo> transportInfo, const android::sp<OBCConnectInfo> connectInfo);
        bool handleConnectCanClient(const android::sp<OBCTransportInfo> transportInfo, android::sp<OBCConnectInfo> connectInfo);
        bool startDisconnectCanThread(const uint16_t connectId, uint8_t &responseCode);
        uint8_t handleDisconnectCanClient(const uint16_t connectId);

        size_t commDataMessageCount(const uint8_t want);
        // void requestSendMessageToMcu(uint8_t type, uint8_t category, uint8_t cmd, uint8_t cmd2, sp<Buffer> payload);
        bool receiveMessageFromMcu(const uint8_t want, const sp<CommunicationData> commData);
        uint32_t getReceiveQueueSize(void);
        android::sp<ICommunicationManagerService> mCommunicationManager;

    public:
        constexpr static uint8_t MAX_RETRY_COUNT_TRANS_ERR{3U};
        constexpr static uint8_t MAX_RETRY_COUNT_BUSY_ERR{10U};
        constexpr static uint8_t REQUEST_CONNECT{0x20U};
        constexpr static uint8_t REQUEST_DISCONNECT{0x30U};
        constexpr static uint8_t SEND_UDS_REQUEST{0x40U};
        OnboardclientImpl(/* args */);
        virtual ~OnboardclientImpl() = default;
        OnboardclientImpl(const OnboardclientImpl &other) = delete;
        OnboardclientImpl &operator=(const OnboardclientImpl &other) = delete;
        OnboardclientImpl(OnboardclientImpl &&) = delete;
        OnboardclientImpl &operator=(OnboardclientImpl &&) = delete;
        static OnboardclientImpl *getInstance(void);

        uint8_t getConnectedId(uint16_t &connectId, const uint32_t canId);
        uint8_t getConnectedCanID(const uint16_t connectId, uint8_t &protocolType, uint32_t &canId);
        uint8_t getConnectedProtocolType(const uint32_t connectCanId);
        bool deleteUdsDataFromMap(const uint16_t connectedId);

        static uint8_t convertProtocolType(const uint8_t origin, const bool isForMcu);
        // static bool adjustCanIdFormatForMcu(const uint8_t protocolType, const uint32_t canId, uint8_t *const retCanId);
        static uint32_t adjustCanIdFormatForObc(const uint16_t index, const android::sp<Buffer> payload);
        static uint16_t adjustNumForObc(const uint16_t index, const bool isUdsLen, const android::sp<Buffer> payload);

        void handleUdsRespFromDiagMgr(const sp<Buffer> udsResp);
        void pushToQueue(const sp<CommunicationData> commData, const OBC_COMMON::COMM_RESPONSE_TYPE type);
        void connectCanClient(android::sp<OBCTransportInfo> transportInfo, android::sp<OBCConnectInfo> &connectInfo);
        uint8_t disconnectCanClient(const uint16_t connectId);

        void onUdsAckResponse(void);
        void onUdsResponse(void);
        static void convertToCommData(const uint8_t protocolType, const uint32_t canId, android::sp<Buffer> connectPayload);
        void convertToCommData(const uint32_t connectedCanId, const android::sp<Buffer> udsData, const android::sp<CommunicationData> &commData);
        static uint32_t calTxCanId(const uint8_t protocolType, const uint32_t rxCanId);
        static uint32_t calRxCanId(const uint8_t protocolType, const uint32_t txCanId);
        uint16_t convertToObcData(const uint16_t index, const android::sp<Buffer> commResData, android::sp<OBCResponseEventInfo> &obcEventResInfo);
        void popFromQueue(const uint8_t want, sp<CommunicationData> &tmpData);
        void pushToTransQueue(const uint16_t connectedId, const android::sp<Buffer> udsReq);
        bool getUdsReqFromQueue(const uint16_t connectedId, const android::sp<Buffer> &udsReqData);
        uint16_t getCurrentProcessingId(void) const noexcept;
        void setCurrentProcessingId(const uint16_t processingId) noexcept;
        void processingNextRequest(void);
        void responseErrorEventToApp(const OBCEnum::OBCErrCode errCode);
        uint32_t getTransQueueSize(void);
        void processingRetry(const OBCEnum::OBCErrCode errType, const uint16_t connectedId);
        void startTimeOutUdsRes(const uint16_t requestId);
        void setTempMaxExceedSize(const uint32_t queueSize) noexcept;
    };
};
#endif // ONBOARDCLIENT_IMPL_H
