#pragma once

#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/Buffer.h>

#include <services/OnboardclientManagerService/IOnboardclientManagerService.h>
#include <services/OnboardclientManagerService/IOnboardClientReceiver.h>
#include <services/OnboardclientManagerService/OBCResponseEventInfo.h>
#include <services/OnboardclientManagerService/OBCUDSResInfo.h>
#include <services/OnboardclientManagerService/OBCCanInfo.h>
#include <services/OnboardclientManagerService/OBCTransportInfo.h>

#include "ParamsDef.h"
#include "common_def.h"
#include "RemoteDelegate.h"

#include "diagprocess/OTA/OtaMessage.h"
#include "diagprocess/OTA/FaServer.h"
#include "diagprocess/UDS/UdsMessage.h"

namespace rdgapp {

class RemoteOTA : public android::RefBase, public RemoteDelegate, public android::Singleton<RemoteOTA>
{
protected:
    bool checkPrecondition() noexcept final {return false;};
public:

    static constexpr uint8_t APP_ID {RDG_APPID::OTA};

    RemoteOTA();
    ~RemoteOTA() final;
    RemoteOTA(const RemoteOTA&) = default;
    RemoteOTA(RemoteOTA&&) = default;
    RemoteOTA& operator=(const RemoteOTA&) = default;
    RemoteOTA& operator=(RemoteOTA&&) = default;

    void notifyBootComplete() const noexcept final  {};
    bool notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff);
    void onReceiveIG(const bool status) const noexcept final  {};
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) final;
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0) noexcept final  {};
    void onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) noexcept final {};
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final {return std::map<uint64_t, android::sp<UdsMessage>>();};
    uint8_t getAppId() const noexcept final  {return APP_ID;};
    void onFirewallActionDiagDisable();
    void onFirewallActionDiagEnable();

    void init(android::sp<sl::SLLooper>& privateLooper);
    void handleConnectReq(const android::sp<OtaMessage>& otaReqMessage);
    void handleDisconnectReq(const android::sp<OtaMessage>& otaReqMessage);
    void handleGetObcResourceReq(const android::sp<OtaMessage>& otaReqMessage);
    void handleReleaseObcResourceReq(const android::sp<OtaMessage>& otaReqMessage);
    void handleSendUdsDataReq(const android::sp<OtaMessage>& otaReqMessage);
    void handleSendUdsDataReqTimeout(const android::sp<::Buffer> reqData);
    void handleUdsResponseTimeout(void);
    void handleDiscardedEvent(void);
    void handleFaClientDisconnectEvent(void);
    void handleOtaEnableStateTimeout(void);

    void connectResultNotify(const uint16_t sequenceNumber, const android::sp<OBCConnectInfo> info);
    void disconnectResultNotify(const uint16_t sequenceNumber, const OBCEnum::OBCErrCode& code);
    void ocbResourceEventNotify(const OBCResourceEventCode& event);
    void ocbResourceEventNotify(const uint16_t sequenceNumber, const OBCResourceEventCode& event);
    void responseEventNotify(const android::sp<OBCResponseEventInfo> responseEventInfo);

    void execulteOtaReq(const android::sp<::Buffer> reqData);
    void sendOtaRes(const android::sp<::Buffer> resData);

    inline bool isWaitingObcResource(void) const noexcept {return mIsWaitingObcResource;}

    inline void updateOtaEnableStateTimeout(void) {
        mOtaTimer.stop();
        mOtaTimer.start();
    }; 

    inline void stopOtaEnableStateTimeout(void) {
        mOtaTimer.stop();
    };

    void updateUdsResponseTimeout(const uint32_t duration);

    inline void stopUdsResponseTimeout(void) {
        mUdsResponseTimer.stop();
    }; 

    class MainHandler : public sl::Handler {

        public:
            static constexpr int32_t CMD_OTA_EXECULTE_REQ                       {2010};
            static constexpr int32_t CMD_OTA_FA_CLIENT_DISCONNECTED             {2011};
            static constexpr int32_t CMD_OTA_ENABLE_STATE_TIMEOUT               {2012};
            static constexpr int32_t CMD_OTA_DISCARD                            {2013};
            static constexpr int32_t CMD_OTA_WAITING_FRAGMENT_DATA              {2014};
            static constexpr int32_t CMD_OTA_FA_CLIENT_TIMEOUT                  {2015};
            static constexpr int32_t CMD_OTA_USD_RESPONSE_TIMEOUT_EVENT         {2016};

            explicit MainHandler(android::sp<sl::SLLooper>& aLooper, RemoteOTA& ota) noexcept
                            :  android::RefBase(), sl::Handler(aLooper), mOTA(ota) {}
            MainHandler(const MainHandler&) = default;
            MainHandler(MainHandler&&) = default;
            MainHandler& operator= (const MainHandler&) = default;
            MainHandler& operator= (MainHandler&&) = default;
            ~MainHandler() final = default;
            void handleMessage(const android::sp<sl::Message>& handlemsg) final;

        private:
            RemoteOTA& mOTA;
    };

    friend class MainHandler;

private:

    class TimerHandler : public TimerTimeoutHandler {
    public:
        // RDG30-R-0535
        static constexpr uint32_t OTA_SESSION_TIMEOUT {10U};   // If no request is received from the OTA application
                                                               // for 10 seconds in “OTA Enable state”,the OTA application
                                                               // shall be notified of the timeout and the “OTA Enable state” shall be released.
        static constexpr uint32_t UDS_RESPONSE_TIMEOUT_EXTEND {5U};  

        static constexpr int32_t OTA_SESSION_TIMEOUT_TIMER_ID {3000};
        static constexpr int32_t UDS_RESPONSE_TIMEOUT_TIMER_ID {3001};

        explicit TimerHandler (const RemoteOTA& ota) noexcept : TimerTimeoutHandler(), mRemoteOTA(ota) {}
        ~TimerHandler() final = default;
        TimerHandler(const TimerHandler&) = default;
        TimerHandler(TimerHandler&&) = default;
        TimerHandler& operator=(const TimerHandler&) = default;
        TimerHandler& operator=(TimerHandler&&) = default;

        void handlerFunction (const int32_t timerId) final {
            switch (timerId) {
                case OTA_SESSION_TIMEOUT_TIMER_ID:
                    (void)mRemoteOTA.mHandler->obtainMessage(MainHandler::CMD_OTA_ENABLE_STATE_TIMEOUT)->sendToTarget();
                    break;
                case UDS_RESPONSE_TIMEOUT_TIMER_ID:
                    (void)mRemoteOTA.mHandler->obtainMessage(MainHandler::CMD_OTA_USD_RESPONSE_TIMEOUT_EVENT)->sendToTarget();
                    break;
                default:
                    break;
            }
        }
    private:
        const RemoteOTA& mRemoteOTA;
    };

    void requestPriorityControll(const OTAPriorityType priority);
    void releaseObcResource();

    android::sp<MainHandler> mHandler;
    TimerHandler mOtaTimerHandler;
    TimerHandler mUdsResponseTimerHandler;
    Timer mOtaTimer;
    Timer mUdsResponseTimer;
    android::sp<FaServer> mFaServer;
    uint16_t mReqSeqNum;
    uint32_t mCurrentTriggerId;
    bool mOtaEnableState;
    bool mIsWaitingObcResource;
    bool mIsActive;
    uint16_t mCurrentConnectId;
    uint16_t mCurrentUdsResTimeout;
    OTAPriorityType mOtaPriority;
    tOBCUDSResInfo mCurrentResInfo;
    std::unordered_map<uint32_t, android::sp<DiagTrigger>> mTriggerList;
};
}
