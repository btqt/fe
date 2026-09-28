#ifndef REMOTE_ROBSSR_H
#define REMOTE_ROBSSR_H

#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>
#include <deque>

#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "Remotediag.h"
#include "CRCManager.h"
#include "diagprocess/UDS/UdsMessage.h"
#include "diagprocess/RoBOccurrence/RoBOccurrence.h"
#include "diagprocess/RoBOccurrence/OccurrentRobNotification.h"
#include "services/HttpManagerAdapter.h"
#include <services/TimeManagerService/TimeManager.h>
#include <services/OnboardclientManagerService/IOnboardclientManagerService.h>
#include <services/OnboardclientManagerService/IOnboardClientReceiver.h>
#include <services/OnboardclientManagerService/OBCResponseEventInfo.h>
#include <services/OnboardclientManagerService/OBCUDSResInfo.h>
#include <services/OnboardclientManagerService/OBCCanInfo.h>
#include <services/OnboardclientManagerService/OBCTransportInfo.h>

namespace rdgapp {

class Remotediag;

class RemoteRoBSSR : public android::RefBase, public RemoteDelegate {
protected:
    bool checkPrecondition();
    void handleError();
public:
    static constexpr uint8_t APP_ID {RDG_APPID::ROBSSR};
    RemoteRoBSSR(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper);
    ~RemoteRoBSSR() override;
    static android::sp<RemoteRoBSSR> getInstance();
    RemoteRoBSSR(const RemoteRoBSSR&) = default;
    RemoteRoBSSR(RemoteRoBSSR&&) = default;
    RemoteRoBSSR& operator=(const RemoteRoBSSR&) = default;
    RemoteRoBSSR& operator=(RemoteRoBSSR&&) = default;

    void notifyBootComplete() const noexcept override {};
    void onReceiveIG(const bool status) const override;
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) noexcept override;
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0) noexcept override;
    void onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) noexcept override;
    void onRdgStop(const bool isStop) const noexcept override;
    void onOccurrentRobDetected(const android::sp<OccurrentRobNotification> notification);
    uint8_t getAppId() const noexcept override {return APP_ID;};

    void onTransmissionTimeout(void);
    void handleTrigger(const DiagTrigger::DiagTriggerState& pState, const uint32_t& pTriggerId, const bool dueToIgOff);
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final;
    void triggerRoBSSR(const DiagTrigger::DiagTriggerType type, const uint32_t prio, const uint64_t colId, const uint64_t notiId, const int64_t time);
    void notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const uint32_t& pTriggerId, const bool dueToIgOff);
    void handleUnderRepairStatusChange(const int32_t& what, const int32_t& status);
    void testingMaxFileSize(const uint16_t fileSize) noexcept;

private:
    static constexpr uint32_t MAX_SPLIT_FILE {3U};
    uint32_t ROBSSR_TOTAL_UPLOAD_DATA_SIZE_MAX; // bytes = 10MB
    uint32_t ROBSSR_UPLOAD_DATA_SIZE_MAX;        // bytes = 4MB
    uint32_t ROBSSR_LAST_UPLOAD_DATA_SIZE_MAX;   // bytes = 2MB
    void init(void);
    void startUp(const DiagTrigger::DiagTriggerType type, const uint64_t timestamp, const android::sp<CommonDefine::RDGLocationData> location);
    void startUp_RoBOccurrence(const android::sp<OccurrentRobNotification> notification);
    void finishCurrentTransmission(void);
    void onFinishAcquisition(const uint32_t& triggerId);
    bool calculateCRC();
    void makeUploadRequest(void);
    void makeHeaderForUploading(const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadRobSsrDataRequest> &robssrDataUpload, const bool isIncreaseCounter = false);
    void makeErrorUploadRequest(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colId);
    void changeIGStatus(const bool status);
    void printData(const std::string data) const;
    void handleTransmissionTimeout();
    void handleReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo);
    void handleStopRDG();
    using CenterCommunicationType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType;

    class MainHandler : public sl::Handler {
    public:
        static constexpr int32_t CMD_INIT_ROBSSR                        {5000};
        static constexpr int32_t CMD_START_ROBSSR                       {5001};
        static constexpr int32_t CMD_TRIGGER_FROM_CENTER                {5002};
        static constexpr int32_t CMD_REQUEST_TO_PRIORITY_CONTROL        {5003};
        static constexpr int32_t CMD_MAKE_UPLOAD_REQUEST                {5004};
        static constexpr int32_t CMD_READ_SAVED_CRC                     {5005};
        static constexpr int32_t CMD_TRIGGER_FROM_ROBOCCURRENCE         {5006};
        static constexpr int32_t CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE   {5007};
        static constexpr int32_t CMD_CHANGE_IG_STATUS                   {5008};
        static constexpr int32_t CMD_ROBSSR_FINISH_TRANSMISSION         {5009};
        static constexpr int32_t CMD_TRANSMISSION_TIMEOUT               {5010};
        static constexpr int32_t CMD_RECEIVE_UDS_RESPONSE               {5011};
        static constexpr int32_t CMD_STOP_RDG                           {5012};
        static constexpr int32_t CMD_PROCESS_ROBOCCURRENCE              {5013};

        explicit MainHandler(android::sp<sl::SLLooper>& privateLooper, RemoteRoBSSR &rob_ssr) noexcept
                : android::RefBase(), sl::Handler(privateLooper), mRoBSSR(rob_ssr) {}
        ~MainHandler() override = default;
        MainHandler(const MainHandler&) = default;
        MainHandler(MainHandler&&) = default;
        MainHandler& operator=(const MainHandler&) = default;
        MainHandler& operator=(MainHandler&&) = default;
        void handleMessage(const android::sp<sl::Message>& handlemsg) override;
    private:
        RemoteRoBSSR& mRoBSSR;
    };

    class TimerHandler : public TimerTimeoutHandler {
    public:
        // static constexpr int32_t ID_IG_ON_TRIGGER {6000};
        static constexpr int32_t ID_TRANSMISSION_TIMEOUT {6001};

        explicit TimerHandler (RemoteRoBSSR& robssr) noexcept : TimerTimeoutHandler(), mRoBSSR(robssr) {}
        ~TimerHandler() override = default;
        TimerHandler(const TimerHandler&) = default;
        TimerHandler(TimerHandler&&) = default;
        TimerHandler& operator=(const TimerHandler&) = default;
        TimerHandler& operator=(TimerHandler&&) = default;

        void handlerFunction (const int32_t timerId) override;
    private:
        RemoteRoBSSR& mRoBSSR;
    };

    class RobSsrUdsTransmission : public RefBase {
    public:
        enum class State: uint8_t {
            ROBSSR_TRANS_INIT = 0,
            ROBSSR_TRANS_SESSIONCONTROL_REQUEST,
            ROBSSR_TRANS_ROBFRAMENO_REQUEST,
            ROBSSR_TRANS_ROBSSRDATA_REQUEST,
            ROBSSR_TRANS_REFERENCEDATA_REQUEST,
            ROBSSR_TRANS_CLOSESESSION_REQUEST
        };

        enum class ResponseType: uint8_t {
            ROBSSR_RES_TYPE_POSITIVE = 0,
            ROBSSR_RES_TYPE_NEGATIVE,
            ROBSSR_RES_TYPE_UNRESPONSIVE
        };
        RobSsrUdsTransmission( RemoteRoBSSR& robssr
                            , const CommonDefine::EcuInformation& fEcuInfo
                            , const uint32_t fRobCode
                            , const bool fHasSubFn
                            , const uint32_t fSubFunction
                            , const bool fHasMemorySelection
                            , const uint32_t fMemorySelection
                            , const bool fHasRobFrameNo
                            , const std::vector<uint32_t> fRobFrameNoList
                            , const bool fHasCrcInfo
                            , const uint32_t fOccurredCRC
                            , const uint32_t fTimeSeriesCRC);
        ~RobSsrUdsTransmission() = default;

        void transInit(); //push UDS request for session control (SID 10) to udsReqList
        void connect();
        void disconnect();
        void sendNextUdsRequest();
        void handleUdsResponse(const android::sp<UdsMessage> udsResponse, const android::sp<OBCResponseEventInfo> responseEventInfo); //handle the response, push necessary UDS requests (frameno, robssr, reference data) to udsReqList
        void stopTimeout();
        void createDiagnosticsData(vccomif::rdg::v1::interfaces::DiagnosticsMessage &diagMessage, const android::sp<OBCResponseEventInfo> responseEventInfo, const ResponseType type);
        void reqAcquireRefData(void); //make UDS request to read reference data
        void reqChangeSession(void); //make UDS request to change session
        void handleNegativeResponse(const android::sp<OBCResponseEventInfo> responseEvent, const android::sp<UdsMessage> udsResponse, const bool isTimeout = false);

        uint16_t getConnectId() const noexcept {return connectId;};
        uint64_t getTransmissionId() const noexcept {return transmissionId;};
        uint32_t getUpload_occurredCRC() const noexcept {return upload_occurredCRC;};
        uint32_t getUpload_timeSeriesCRC() const noexcept {return upload_timeSeriesCRC;};
        uint32_t getMemory_selection() const noexcept {return memorySelection;};
        uint32_t getRob_code() const noexcept {return robCode;};
        bool getFramesAreDiscarded() const noexcept {return framesDiscarded;};
        std::vector<vccomif::rdg::v1::interfaces::DiagnosticsMessage> getDiagRoBSSRResponse() noexcept {return mDiagRoBSSRResponse;};
        CommonDefine::EcuInformation getEcuInfo() const noexcept {return ecuInfo;};
        State getState() const noexcept { return m_state; };
        void setOccurCRC(const uint32_t occurCRC) noexcept { upload_occurredCRC = occurCRC;};
        void setTimeCRC(const uint32_t timeCRC) noexcept {upload_timeSeriesCRC = timeCRC;};
        bool getHasRobFrameNo() const noexcept {return hasRobFrameNo;};
        bool getHasMemorySelection() const noexcept {return hasMemorySelection;};
        bool getHasCrcInfor() const noexcept {return hasCrcInfo;};
        std::vector<uint32_t> getRobFrameNoList() const noexcept {return robFrameNoList;};
        void checkUploadSize(const android::sp<OBCResponseEventInfo> responseEvent, const android::sp<UdsMessage> udsResponse, const bool isTimeout = false);
        uint32_t getRobFrameNoFromReqFront(const std::deque<android::sp<UdsMessage>> &reqList);

    private:
        void printData(const std::string data) const;
        static constexpr uint32_t TRANSMISSION_TIME_OUT_DURATION {195U};
        RemoteRoBSSR& mRoBSSR;
        std::deque<android::sp<UdsMessage>> udsReqList;
        TimerHandler mTimerHandler;  
        Timer mTimeOut;

        CommonDefine::EcuInformation ecuInfo;
        uint16_t connectId;
        uint64_t transmissionId;
        uint32_t robCode;
        bool hasSubFn;
        uint32_t subFunction;
        bool hasMemorySelection;
        uint32_t memorySelection;
        bool hasRobFrameNo;
        std::vector<uint32_t> robFrameNoList;
        bool hasCrcInfo;
        uint32_t occurredCRC;
        uint32_t timeSeriesCRC;
        uint32_t upload_occurredCRC;
        uint32_t upload_timeSeriesCRC;
        bool framesDiscarded;
        uint32_t highestOccurredFrame;
        uint32_t highestTimeSeriesFrame;
        bool isAcquiredRoBSSR;
        bool hasOccurRoBSSR; // whether time-of-occurrence RoBSSR data can be acquired
        bool hasTimeSeriesRoBSSR; // whether time-series RoBSSR data can be acquired
        android::sp<Buffer> mUdsReq;
        std::map<uint64_t, android::sp<UdsMessage>> mDiagResp;
        std::vector<android::sp<UdsMessage>> mDiagResponse;
        std::vector<vccomif::rdg::v1::interfaces::DiagnosticsMessage> mDiagRoBSSRResponse;
        State m_state;
    };

    struct ReferenceData {
    private:
        uint32_t m_ecuMakerCode;
        std::string m_swPartNumber;
    public:
        ReferenceData() : m_ecuMakerCode(0U),  m_swPartNumber("") {}

        void setEcuMakerCode(const uint32_t val) noexcept {m_ecuMakerCode = val;}
        uint32_t getEcuMakerCode() const noexcept {return m_ecuMakerCode;}

        void setSwPartNumber(const std::string s) noexcept {m_swPartNumber = s;}
        std::string getSwPartNumber() const noexcept {return m_swPartNumber;}
    };

    enum class EcuSession: uint8_t
    {
        SESSION_NRC = 0x00U,
        SESSION_DEFAULT = 0x01U,
        SESSION_REMOTE = 0x40U
    };

    const Remotediag& mApp;
    android::sp<MainHandler> mHandler;
    static android::sp<RemoteRoBSSR> mRoBSSR_instance;
    android::sp<CRCManager> mCRCManager;
    std::unordered_map<uint32_t, android::sp<DiagTrigger>> mSaveReq;
    android::sp<CommonDefine::RDGLocationData> mLocationData;
    uint32_t odo_value;
    uint32_t odo_unit;
    bool mIsWaitingObcResource;
    bool mIsRobSsrRunning;
    bool mIsNeedUploadErrorData;
    bool mIsSuspending;
    uint32_t mTriggerId;
    uint64_t mColId;
    uint64_t mNotiId;
    uint32_t mPriority;
    EcuSession mSession;
    DiagTrigger::DiagTriggerType mCurrentDiagTriggerType;
    std::queue<uint64_t> mTransmissioIdList;
    uint64_t mCurrentTransmissionId;
    int64_t mDiagnosticsAcquisitionTime;
    uint32_t mTotalSize;
    android::sp<OccurrentRobNotification> mRobNotificationRequest;
    std::vector<UploadRobSsrDataRequest> l_UploadRoBSSRDataRequest;
    std::unordered_map<uint32_t, ReferenceData> mResSID22;
    std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>> mRobSsrTransList;
    std::shared_ptr<vccomif::rdg::v1::interfaces::UploadRobSsrDataRequest> mRobSsrDataReq;
    std::shared_ptr<vccomif::rdg::v1::interfaces::UploadErrorDataRequest> mRobSsrUploadErrorData;
    std::unordered_map<uint32_t, std::queue<uint64_t>> mReqIDIsSuspended_transId;
    std::unordered_map<uint32_t, std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>> mReqIDIsSuspended_transList;
};
}
#endif // REMOTE_ROBSSR_H
