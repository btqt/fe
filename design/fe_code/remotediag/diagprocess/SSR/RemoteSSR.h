#ifndef REMOTE_SSR_H
#define REMOTE_SSR_H

#include <ctime>
#include <unordered_map>
#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/Buffer.h>
#include <stack>

#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "RemoteDelegate.h"
#include "Remotediag.h"
#include "diagprocess/UDS/UdsMessage.h"
#include "diagprocess/EcuInformation/EcuInformationDataType.h"
#include "CRCManager.h"
#include "utils/UploadManager.h"
#include "utils/UploadTask.h"
#include "DiagTrigger.h"
#include "utils/PriorityControl.h"

#include <services/TimeManagerService/TimeManager.h>
#include <services/OnboardclientManagerService/IOnboardclientManagerService.h>
#include <services/OnboardclientManagerService/IOnboardClientReceiver.h>
#include <services/OnboardclientManagerService/OBCResponseEventInfo.h>
#include <services/OnboardclientManagerService/OBCUDSResInfo.h>
#include <services/OnboardclientManagerService/OBCCanInfo.h>
#include <services/OnboardclientManagerService/OBCTransportInfo.h>

#include "services/HttpManagerAdapter.h"

namespace rdgapp {

class Remotediag;
class CRCManager;

class RemoteSSR : public android::RefBase, public RemoteDelegate {
protected:
    bool checkPrecondition();
public:
    //static constexpr uint8_t APP_ID {RDG_APPID::SSR};

    RemoteSSR(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper);
    ~RemoteSSR() override;
    RemoteSSR(const RemoteSSR&) = default;
    RemoteSSR(RemoteSSR&&) = default;
    RemoteSSR& operator=(const RemoteSSR&) = default;
    RemoteSSR& operator=(RemoteSSR&&) = default;

    static RemoteSSR* getInstance(void);
    virtual void notifyBootComplete() const noexcept {};
    void onReceiveIG(const bool status) const override;
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) override;
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0);
    void onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) noexcept override {};
    uint8_t getAppId(void) const noexcept final;
    void onTransmissionTimeout(void);
    void init(void);
    void startUp(const DiagTrigger::DiagTriggerType type, const uint64_t timestamp, const android::sp<CommonDefine::RDGLocationData> location);
    void finishCurrentTransmission(void);
    void startNextTransmission(void);
    void triggerFromDTC(const DiagTrigger::DiagTriggerType triggerType
        , const int64_t timeData
        , const android::sp<CommonDefine::RDGLocationData> location
        , const uint64_t collectionId
        , const uint32_t priority
        , const std::vector<uint32_t> v_targetEcu);
    
    void notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff);
    void handleTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff);
    void handleUnderRepairStatusChange(const int32_t& what);
    void onFinishSSRAcquisition(const uint32_t& triggerId);
    bool calculateCRC();
    void readCRC();
    void makeUploadSSRRequest();
    void onSSRFlagChangeOFF();
    void makeErrorUploadDataRequest(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colID);
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final {return mDiagResp_CRC;};
    void abortSSRAcquisition(const RdgProtoInterface::ResponseCode code);
    void testingMaxFileSize(const uint16_t fileSize) noexcept;

private:
    void printData(const std::string data) const;
    uint32_t SSR_UPLOAD_DATA_SIZE_MAX;
    void changeIGStatus(const bool status);
    void triggerToNextService(const bool hasHistoryFile);
    class MainHandler : public sl::Handler {
    public:
        static constexpr int32_t CMD_INIT_SSR                           {2000};
        static constexpr int32_t CMD_CHANGE_IG_STATUS                   {2001};
        static constexpr int32_t CMD_READ_SAVED_CRC                     {2002};
        static constexpr int32_t CMD_REQUEST_TO_PRIORITY_CONTROL        {2003};
        static constexpr int32_t CMD_MAKE_UPLOAD_REQUEST                {2004};
        static constexpr int32_t CMD_START_SSR                          {2005};
        static constexpr int32_t CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE   {2006};
        static constexpr int32_t CMD_SSR_FINISH_TRANSMISSION            {2007};

        explicit MainHandler(android::sp<sl::SLLooper>& privateLooper, RemoteSSR &ssr) noexcept
                : android::RefBase(), sl::Handler(privateLooper), mSSR(ssr) {}
        ~MainHandler() override = default;
        MainHandler(const MainHandler&) = default;
        MainHandler(MainHandler&&) = default;
        MainHandler& operator=(const MainHandler&) = default;
        MainHandler& operator=(MainHandler&&) = default;
        void handleMessage (const android::sp<sl::Message>& handlemsg) override;
    private:
        RemoteSSR& mSSR;
    };

    class TimerHandler : public TimerTimeoutHandler {
    public:
        static constexpr int32_t ID_TRANSMISSION_TIMEOUT {3000};

        explicit TimerHandler (RemoteSSR& ssr) noexcept : TimerTimeoutHandler(), mSSR(ssr) {}
        ~TimerHandler() override = default;
        TimerHandler(const TimerHandler&) = default;
        TimerHandler(TimerHandler&&) = default;
        TimerHandler& operator=(const TimerHandler&) = default;
        TimerHandler& operator=(TimerHandler&&) = default;

        void handlerFunction (const int32_t timerId) override {
            switch (timerId) {
                case ID_TRANSMISSION_TIMEOUT:
                    mSSR.onTransmissionTimeout();
                    break;
                default:
                    break;
            }
        }
    private:
        RemoteSSR& mSSR;
    };

    class SsrUdsTransmission : public RefBase {
    public:
        friend class RemoteSSR;
        enum class State: uint8_t {
            SSR_TRANS_INIT = 0,
            SSR_TRANS_SESSIONCONTROL_REQUEST,
            SSR_TRANS_ROBFRAMENO_REQUEST,
            SSR_TRANS_ROBSSR_REQUEST,
            SSR_TRANS_REFERENCEDATA_REQUEST,
            SSR_TRANS_DONE
        };
        SsrUdsTransmission( RemoteSSR& ssr
                            , const CommonDefine::EcuInformation& ecuInformation);
        ~SsrUdsTransmission() = default;

        void transInit();
        void connect();
        void disconnect();
        void sendNextUdsRequest();
        void handleUdsResponse(android::sp<UdsMessage> udsResponse); //handle the response, push UDS requests to UdsReqList
        void stopTimeout();
        void handleTimeout() const noexcept {};
        State getState() const noexcept { return m_state; };
        void setState(const State st) noexcept { m_state = st; };
        uint16_t getConnectId() const noexcept {return connectId;};
        uint64_t getTransmissionId() const noexcept {return transmissionId;};
        CommonDefine::EcuInformation& getEcuInformation() noexcept {return ecuInfo;};
        void setRxAdd(const uint32_t address) noexcept {centerRxAdd = address;};
        uint32_t getRxAdd() const noexcept {return centerRxAdd;};

    private:
        static constexpr uint32_t TRANSMISSION_TIME_OUT_DURATION {65U};
        RemoteSSR& mSSR;
        std::queue<android::sp<UdsMessage>> udsReqList;
        TimerHandler mTimerHandler;  
        Timer mTimeOut;
        CommonDefine::EcuInformation ecuInfo;
        uint16_t connectId;
        uint64_t transmissionId;
        State m_state;
        uint32_t centerRxAdd;
    };

    struct RefData {
    private:
        uint32_t m_ecuMakerCode;
        std::string m_swPartNumber;
    public:
        RefData() : m_ecuMakerCode(0U),  m_swPartNumber("") {}
        void setEcuMakerCode(const uint32_t val) noexcept {m_ecuMakerCode = val;}
        uint32_t getEcuMakerCode() const noexcept {return m_ecuMakerCode;}
        void setSwPartNumber(const std::string s) noexcept {m_swPartNumber = s;}
        std::string getSwPartNumber() const noexcept {return m_swPartNumber;}
    };
    using TransmissionInter = std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator;
    static RemoteSSR* mSSR_instance;
    const Remotediag& mApp;
    android::sp<MainHandler> mHandler;
    android::sp<CRCManager> mCRCManager;
    std::map<uint64_t, android::sp<UdsMessage>> mDiagResp_CRC;
    std::deque<std::pair<uint64_t, android::sp<UdsMessage>>> mDiagResp;
    std::deque<std::pair<uint64_t, android::sp<UdsMessage>>> mDiagNegativeResp;
    std::unordered_map<uint32_t, android::sp<DiagTrigger>> mSaveReq;
    // std::deque<std::pair<uint64_t, uint32_t>> mTransDTCNo;
    // std::deque<std::pair<uint32_t, android::sp<UdsMessage>>> mDTCNoResp;
    std::unordered_map<uint64_t, std::vector<uint32_t>> mTransDTCNo;
    std::unordered_map<uint32_t, android::sp<UdsMessage>> mDTCNoResp;
    uint64_t  mCurrentTransmissionId;
    uint64_t mDiagnosticsAcquisitionTime;
    uint64_t mWarningTriggerOccurrenceTime;
    uint32_t mTriggerId;
    bool mIsSsrRunning;
    bool mIsSuspending;
    bool mIsNeedUploadErrorData;
    bool mIsNeedtoUploadSSRData;
    uint32_t mPriority;
    uint64_t mColId;
    DiagTrigger::DiagTriggerType mCurrentDiagTriggerType;
    int64_t mCurrentTriggerTimestamp;
    std::vector<uint32_t> mSsrTargetECUList;
    android::sp<CommonDefine::RDGLocationData> mCurrentTriggerLocation;
    std::unordered_map<uint32_t, RefData> mResSID22;
    std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>> mSsrTransList;
    std::queue<uint64_t> mTransmissioIdList;
    std::shared_ptr<vccomif::rdg::v1::interfaces::UploadErrorDataRequest> mSSRUploadErrorData;
};
}
#endif // REMOTE_SSR_H
