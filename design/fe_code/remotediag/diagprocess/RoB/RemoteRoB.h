#pragma once

#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/Buffer.h>
#include <utils/Singleton.h>

#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "RemoteDelegate.h"

#include "diagprocess/UDS/UdsMessage.h"
#include "diagprocess/EcuInformation/EcuInformationDataType.h"

#include "utils/UploadManager.h"

#include "utils/CollectionCondition.h"

#include <services/TimeManagerService/TimeManager.h>
#include <services/OnboardclientManagerService/IOnboardclientManagerService.h>
#include <services/OnboardclientManagerService/IOnboardClientReceiver.h>
#include <services/OnboardclientManagerService/OBCResponseEventInfo.h>
#include <services/OnboardclientManagerService/OBCUDSResInfo.h>
#include <services/OnboardclientManagerService/OBCCanInfo.h>
#include <services/OnboardclientManagerService/OBCTransportInfo.h>

#include "services/HttpManagerAdapter.h"

namespace rdgapp {

class RemoteRoB : public android::RefBase, public RemoteDelegate, public android::Singleton<RemoteRoB>
{
protected:
    bool checkPrecondition() noexcept final {return false;};
public:
    static constexpr uint8_t APP_ID {RDG_APPID::ROB};

    RemoteRoB();
    ~RemoteRoB() final = default;
    RemoteRoB(const RemoteRoB&) = default;
    RemoteRoB(RemoteRoB&&) = default;
    RemoteRoB& operator=(const RemoteRoB&) = default;
    RemoteRoB& operator=(RemoteRoB&&) = default;

    void notifyBootComplete() const noexcept final {};
    void onReceiveIG(const bool status) const final;
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) final;
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0) final;
    void onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) final;
    void onRobFlagChangeOFF();
    uint8_t getAppId() const noexcept final {return APP_ID;};
    void onTransmissionTimeout(void);

    void init(const Remotediag* const app, android::sp<sl::SLLooper>& privateLooper);
    void startUp();
    void finishCurrentTransmission(void);
    void triggerAllRoB(const DiagTrigger::DiagTriggerType type, const android::sp<CommonDefine::RDGLocationData> location, const uint32_t prio, const uint64_t colId, const int64_t time);
    void triggerFromSSR(const DiagTrigger::DiagTriggerType triggerType
        , const int64_t acquisitionStartTime
        , const android::sp<CommonDefine::RDGLocationData>& location
        , const uint64_t collectionId
        , const uint32_t priority
        , const int64_t warningOccurrenceTime = 0);

    bool notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff);
    void handleTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff);
    void handleUdsResponsePhase5(const uint32_t targetAddress, const uint32_t canIdRx, const uint16_t connectId, const uint8_t responseCode, const android::sp<UdsMessage> udsResponse);
    void handleUdsResponsePhase6(const uint32_t targetAddress, const uint32_t canIdRx, const uint16_t connectId, const uint8_t responseCode, const android::sp<UdsMessage> udsResponse);
    void handleUnderRepairStatusChange(const int32_t what, const int32_t status);
    void finishRoBAcquisition(const int32_t& triggerId);
    bool calculateCRC();
    void readCRC();
    void makeUploadRoBRequest(const android::sp<DiagTrigger>& trigger);
    void makeErrorUploadDataRequest(const RdgProtoInterface::ResponseCode code, const android::sp<DiagTrigger>& trigger);
    void makeErrorUploadDataRequest(const RdgProtoInterface::ResponseCode code, const DiagnosticsMessageList& diagMsg, const android::sp<DiagTrigger>& trigger);
    void abortRobAcquisition(const RdgProtoInterface::ResponseCode code, const android::sp<DiagTrigger>& trigger);
    void suspendRobAcquisition(void);
    void testHandleWarningTrigger(const int32_t pri);
    void testingMaxFileSize(const uint32_t fileSize) noexcept;
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final {return mCrcCheck;};
    error_t getDiagTrigger(uint32_t triggerId, android::sp<DiagTrigger>& trigger);
    error_t getCurrentTrigger(android::sp<DiagTrigger>& trigger) {
        error_t error{E_ERROR};
        if(mTriggerId >= 0)
        {
            error = getDiagTrigger(static_cast<uint32_t>(mTriggerId), trigger);
        }
        else
        {
            //Nothing
        }
        return error;
    };

private:
    void changeIGStatus(const bool status);
    void stopRobAcquisition(void);
    void printDataDebug(const std::string data) const;
    class MainHandler : public sl::Handler {

        public:
            //static constexpr int32_t CMD_ROB_INIT                         {2000};
            static constexpr int32_t CMD_ROB_START_UP                       {2001};
            //static constexpr int32_t CMD_ROB_UPLOAD_DATA                  {2002};
            //static constexpr int32_t CMD_ROB_GET_OBC_RESOURCE             {2003};
            static constexpr int32_t CMD_READ_SAVED_CRC                     {2008};
            static constexpr int32_t CMD_TRIGGER_FROM_CENTER                {2009};
            static constexpr int32_t CMD_REQUEST_TO_PRIORITY_CONTROL        {2010};
            static constexpr int32_t CMD_MAKE_UPLOAD_REQUEST                {2011};
            static constexpr int32_t CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE   {2012};
            static constexpr int32_t CMD_SIMULATE_WARNING_TRIGGER_EVENT     {2013};
            static constexpr int32_t CMD_CHANGE_IG_STATUS                   {2014};
            static constexpr int32_t CMD_ROB_FINISH_TRANSMISSION            {2015};

            explicit MainHandler(android::sp<sl::SLLooper>& aLooper, RemoteRoB& rob) noexcept
                            :  android::RefBase(), sl::Handler(aLooper), mRoB(rob) {}
            MainHandler(const MainHandler&) = default;
            MainHandler(MainHandler&&) = default;
            MainHandler& operator= (const MainHandler&) = default;
            MainHandler& operator= (MainHandler&&) = default;
            ~MainHandler() final = default;
            void handleMessage(const android::sp<sl::Message>& handlemsg) final;

        private:
            RemoteRoB& mRoB;
    };

    class TimerHandler : public TimerTimeoutHandler {
    public:
        //static constexpr int32_t ID_IG_ON_TRIGGER {3000};
        static constexpr int32_t ID_TRANSMISSION_TIMEOUT {3001};

        explicit TimerHandler (RemoteRoB& rob) noexcept : TimerTimeoutHandler(), mRoB(rob) {}
        virtual ~TimerHandler() override = default;
        TimerHandler(const TimerHandler&) = default;
        TimerHandler(TimerHandler&&) = default;
        TimerHandler& operator=(const TimerHandler&) = default;
        TimerHandler& operator=(TimerHandler&&) = default;

        void handlerFunction (const int32_t timerId) final {
            switch(timerId) 
            {
                case ID_TRANSMISSION_TIMEOUT:
                    mRoB.onTransmissionTimeout();
                    break;
                default:
                    break;
            }
        }
    private:
        RemoteRoB& mRoB;
    };

    class RobUdsTransmission : public RefBase {
    public:
        friend class RemoteRoB;
        enum class State: uint8_t {
            ROB_TRANS_INIT = 0U,
            ROB_TRANS_CONNECT,
            ROB_TRANS_SEND_UDS,
            ROB_TRANS_DISCONNECT,
            ROB_TRANS_DONE
        };
        RobUdsTransmission( RemoteRoB& rob
                            , const CommonDefine::EcuInformation& mecuInformation
                            , const uint8_t aSID
                            , const uint8_t aSFID
                            , const uint8_t aMemorySelection = 0U
                            , const uint8_t aDtcStatusMask = 0U);
        virtual ~RobUdsTransmission() = default;

        void connect();
        void disconnect();
        void send();
        void stopTimeout();
        
        CommonDefine::EcuInformation getEcuInformation() const noexcept {return ecuInformation;};
        //void setEcuInformation(const CommonDefine::EcuInformation tmp) noexcept{ecuInformation = tmp;};

        inline uint16_t getConnectId() const noexcept {return connectId;};
        // inline void setConnectId(const uint16_t tmp) noexcept {connectId = tmp;}
        inline uint64_t getTransmissionId() const noexcept {return transmissionId;};
        // inline void setTransmissionId(const uint64_t tmp) noexcept {transmissionId=tmp;};
        inline State getState() const noexcept {return mstate;};
        inline void setState(const State tmp) noexcept {mstate = tmp;};
        inline uint32_t getCanIdRx() const noexcept {return canIdRx;};
        inline void setCanIdRx(const uint32_t value) noexcept {canIdRx = value;};
    private:
        CommonDefine::EcuInformation ecuInformation;
        uint16_t connectId;
        uint64_t transmissionId;
        uint32_t canIdRx;
        State mstate;
        static constexpr uint32_t TRANSMISSION_TIME_OUT_DURATION {5U};
        RemoteRoB& mRoB;
        TimerHandler mTimerHandler;  
        Timer mTimeOut;
        UdsMessage udsReq;
        UdsMessage udsRes;
    };

    // Folowing RDG30-R-0790, only apply for Phase6
    enum class UDS_MEMORY_SELECTION: uint8_t
    {
        OccurrenceDTC = 0x11U,
        MaintenanceDTC = 0x12U,
        SystemOperationDTC = 0x13U,
        SecurityEventDTC = 0x14U,
    };

    using TransmissionInter = std::unordered_map<uint64_t, android::sp<RobUdsTransmission>>::iterator;

    const Remotediag* mApp;
    android::sp<MainHandler> mHandler;
    uint64_t  mCurrentTransmissionId;
    android::sp<RobUdsTransmission> mCurrentTransmission;
    int32_t mTriggerId;
    bool mIsRobRunning;
    bool mIsAcquisitionAbort;
    int64_t mAcquisitionTime;
    uint32_t mMaxUploadFileSize;
    std::unordered_map<uint64_t, android::sp<RobUdsTransmission>> mRobTransList;
    std::deque<uint64_t> mTransmissioIdList;
    std::unordered_map<uint32_t, uint64_t> mEcuDiagPhase6List;
    std::unordered_map<uint32_t, android::sp<DiagTrigger>> mTriggerList;
    std::deque<std::pair<uint64_t, android::sp<UdsMessage>>> mDiagResp;
    std::map<uint64_t, android::sp<UdsMessage>> mCrcCheck;
    android::sp<CRCManager> mCRCManager;
    android::sp<CommonDefine::RDGLocationData> mCurrentTriggerLocation;
};
}
