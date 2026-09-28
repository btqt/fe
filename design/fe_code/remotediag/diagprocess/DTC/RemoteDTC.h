#ifndef REMOTE_DTC_SCOPE
#define REMOTE_DTC_SCOPE

#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <deque>
#include <unordered_map>
#include <list>
#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>

#include <services/TimeManagerService/TimeManager.h>
#include <services/OnboardclientManagerService/IOnboardclientManagerService.h>
#include <services/OnboardclientManagerService/IOnboardClientReceiver.h>
#include <services/OnboardclientManagerService/OBCResponseEventInfo.h>
#include <services/OnboardclientManagerService/OBCUDSResInfo.h>
#include <services/OnboardclientManagerService/OBCCanInfo.h>
#include <services/OnboardclientManagerService/OBCTransportInfo.h>
#include "services/HttpManagerAdapter.h"
#include "services/DiagManagerAdapter.h"
#include "services/PowerManagerAdapter.h"
#include "diagprocess/EcuInformation/RemoteEcuInformation.h"
#include "diagprocess/EcuInformation/EcuInformationDataType.h"

#include "utils/CommonUtils.h"
#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "RemoteDelegate.h"
#include "DiagTrigger.h"
#include "Remotediag.h"
#include "utils/PriorityControl.h"
#include "UdsMessage.h"
#include "CRCManager.h"
#include "utils/UploadManager.h"
#include "utils/UploadTask.h"
#include "DataModel.h"

namespace rdgapp {
class Remotediag;
// class CRCManager;
// class UploadTask;

class RemoteDTC : public android::RefBase, public RemoteDelegate {
protected:
    bool checkPrecondition() noexcept final {return false;};
public:
    // static constexpr uint8_t APP_ID {RDG_APPID::DTC};
    // static constexpr int32_t WHAT_CHANGED_REPAIR_SATUS {1};
    static RemoteDTC* getInstance(void);
    RemoteDTC(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper);
    ~RemoteDTC() override;
    RemoteDTC(const RemoteDTC&) = default;
    RemoteDTC(RemoteDTC&&) = default;
    RemoteDTC& operator=(const RemoteDTC&) = default;
    RemoteDTC& operator=(RemoteDTC&&) = default;

    void notifyBootComplete() const noexcept final {};
    void onReceiveIG(const bool status) const final;
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) final;
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0) final;
    void onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) final;
    void onRdgStop(const bool isStop) const noexcept override;
    void onDTCFlagChangeOFF();
    uint8_t getAppId() const noexcept final;
    bool notifyTrigger(const DiagTrigger::DiagTriggerState& pState,
        const int32_t& pTriggerId, const bool dueToIgOff);
    void onFinishDTCAcquisition(const int32_t& triggerId);
    void handleTrigger(const DiagTrigger::DiagTriggerState& pState,
        const int32_t& pTriggerId, const bool dueToIgOff);
    // android::sp<Timer> getTickTimer() const;
    void finishCurrentTransmission(void);
    void onTransmissionTimeout(void);
    void triggerFromWarning(
    const uint32_t triggerId
    , const DiagTrigger::DiagTriggerType triggerType
    , const int64_t timeData
    , const android::sp<CommonDefine::RDGLocationData> location
    , const uint64_t collectionId
    , const uint32_t priority);
    
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final;
    void changedRemoteStatus(const int32_t what, const uint32_t info);
    uint8_t getUnderRepairStatus() const noexcept;
    void makeErrorUploadData(void);
    void makeErrorUploadData(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colID);
    void abortDtcProcessing(const vccomif::rdg::v1::interfaces::ResponseCode resCode, const bool stopImmediately);/*abort dtc acquisition*/
    void discardTrigger(const android::sp<DiagTrigger> pTrigger);
    void suspendDtcTrigger(const int32_t triggerId);
    void testingMaxFileSize(const uint32_t fileSize) noexcept;
    void handleUDSResponse(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse);

private:
    const uint32_t IGON_TRIGGER_DURATION {60U * 5U};/* 5mins */
    const uint8_t DTC_RUNNING {0x01U};
    const uint8_t DTC_STOP {0x00U};

    void printData(const std::string data) const; 
    void init();
    void changeIGStatus(const bool status);
    void trigger_DTC(const DiagTrigger::DiagTriggerType type, const uint32_t prio, 
        const uint64_t colId, const int64_t time, const android::sp<CommonDefine::RDGLocationData> loc);
    void trigger_DTC(const android::sp<DiagTrigger> pDiag);
    void sendDTC();
    void finishDTC();
    void finalize();
    uint8_t calculateCRC();/*1:CRC is DIFF 0:CRC is SAME*/
    void triggerToNextService();
    void readCRC();
    void makeUploadData();
    void handleUnresponsiveEcu(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse);
    void handleStopRDG();
    void handleTransmissionTimeout(void);
    class MainHandler : public sl::Handler {
    public:
        static constexpr int32_t CMD_INIT_DTC {2000};
        static constexpr int32_t CMD_CHANGE_IG_STATUS {2001};
        // static constexpr int32_t CMD_PARSE_TABLE {2002};
        static constexpr int32_t CMD_READ_SAVED_CRC {2003};
        static constexpr int32_t CMD_RECEIVE_STATUS_FROM_CENTER {2004};
        static constexpr int32_t CMD_TRIGGER_FROM_IGON {2005};
        static constexpr int32_t CMD_TRIGGER_FROM_WARNING {2006};
        static constexpr int32_t CMD_TRIGGER_FROM_CENTER {2007};
        static constexpr int32_t CMD_REQUEST_TO_PRIORITY_CONTROL {2008};
        static constexpr int32_t CMD_SEND_DTC {2009};
        // static constexpr int32_t CMD_RECEIVE_DTC {2010};
        // static constexpr int32_t CMD_UPLOAD_FILE {2011};
        static constexpr int32_t CMD_UPLOAD_ERROR {2012};
        static constexpr int32_t CMD_DTC_FINISH_TRANSMISSION {2013};
        /*Message to notify UDS response received*/
        static constexpr int32_t CMD_RECEIVE_UDS_RESPONSE {2014};
        static constexpr int32_t CMD_STOP_RDG {2015};
        static constexpr int32_t CMD_TRANSMISSION_TIMEOUT {2016};
        
        explicit MainHandler(android::sp<sl::SLLooper>& privateLooper, RemoteDTC &dtc) noexcept
                : android::RefBase(), sl::Handler(privateLooper), mDTC(dtc) {}
        ~MainHandler() override = default;
        MainHandler(const MainHandler&) = default;
        MainHandler(MainHandler&&) = default;
        MainHandler& operator=(const MainHandler&) = default;
        MainHandler& operator=(MainHandler&&) = default;
        void handleMessage (const android::sp<sl::Message>& handlemsg) override;
    private:
        RemoteDTC& mDTC;
    };

    class TimerHandler : public TimerTimeoutHandler {
    public:
        static constexpr int32_t ID_IG_ON_TRIGGER {3000};
        static constexpr int32_t ID_TRANSMISSION_TIMEOUT {3001};
        explicit TimerHandler (RemoteDTC& dtc) noexcept;
        ~TimerHandler() override = default;
        TimerHandler(const TimerHandler&) = default;
        TimerHandler(TimerHandler&&) = default;
        TimerHandler& operator=(const TimerHandler&) = default;
        TimerHandler& operator=(TimerHandler&&) = default;

        void handlerFunction (const int32_t timerId) override;
    private:
        RemoteDTC& mDTC;
    };

    class DTCUdsTransmission : public RefBase {
    public:
        friend class RemoteDTC;
        enum class State: uint8_t {
            DTC_TRANS_INIT = 0,
            DTC_TRANS_CONNECT,
            DTC_TRANS_REMOTE_SS,
            DTC_TRANS_SEND_UDS,
            DTC_TRANS_DEFAULT_SS,
            DTC_TRANS_DISCONNECT,
            DTC_TRANS_DONE
        };
        DTCUdsTransmission(RemoteDTC& dtc
                            , const CommonDefine::EcuInformation& ecuInformation_dat
                            , const uint8_t aSID
                            , const uint8_t aSFID
                            , const uint8_t aDtcStatusMask = 0x00U);
        ~DTCUdsTransmission() override = default;

        void connect();
        void disconnect();
        void stopTimeoutTimer();
        CommonDefine::EcuInformation getEcuInformation() const noexcept {return ecuInformation;};
        uint16_t getConnectId() const noexcept{return connectId;};
        uint64_t getTransmissionId() const noexcept{return transmissionId;};
        State getState() const noexcept {return mState;};
        void setState(const State data) noexcept {mState = data;};
        void setRxAdd(const uint32_t address) noexcept {centerRxAdd = address;};
        uint32_t getRxAdd() const noexcept {return centerRxAdd;};
        void changeToRemoteSS();
        void changeToDefaultSS();
    private:
        CommonDefine::EcuInformation ecuInformation;
        uint16_t connectId;
        uint64_t transmissionId;
        State mState;
        void send();
        static constexpr uint32_t TRANSMISSION_TIME_OUT_DURATION {195U};
        RemoteDTC& mDTC;
        TimerHandler mTimerHandler;  
        Timer mTimeOut;
        UdsMessage udsReq;
        android::sp<UdsMessage> mpUdsReqLast;
        UdsMessage udsRes;
        uint32_t centerRxAdd;
    };

    const Remotediag& mApp;
    android::sp<MainHandler> mHandler;
    std::shared_ptr<TimerHandler> mTimerHandler;
    android::sp<Timer> mTickTimer;
    uint8_t mDTCFlag;
    std::unordered_map<uint32_t, android::sp<DiagTrigger>> mSaveReq;
    int32_t mTriggerId;
    DiagTrigger::DiagTriggerType mTriggerType;
    uint64_t mDiagnosticsAcquisitionTime;
    uint64_t mWarningTriggerOccurrenceTime;
    android::sp<CommonDefine::RDGLocationData> mLocationData;
    std::vector<pair<uint32_t, uint32_t>> v_targetEcuList;
    std::shared_ptr<::vccomif::rdg::v1::interfaces::UploadDtcDataRequest> mDTCDataReq;
    std::shared_ptr<vccomif::rdg::v1::interfaces::UploadErrorDataRequest> mDTCUploadErrorData;
    using TransmissionInter = std::map<uint64_t, android::sp<DTCUdsTransmission>>::iterator;
    std::map<uint64_t, android::sp<DTCUdsTransmission>> mDTCTransList;
    std::queue<uint64_t> mTransmissioIdList;
    uint64_t  mCurrentTransmissionId;
    volatile uint8_t mIsDTCRunning;
    uint64_t mColId;
    android::sp<CRCManager> mCRCManager;
    std::map<uint64_t, android::sp<UdsMessage>> mDiagResp_CRC;
    std::deque<std::pair<uint64_t, android::sp<UdsMessage>>> mDiagResp_2;
    uint32_t mPriority;
    bool bUploadErrorData;
    volatile bool isDtcAcquireAborted;
    volatile bool isDtcAcquireSuspend;
    volatile bool mStopImmediately;
    uint32_t mMaxUploadFileSize;
    static RemoteDTC* mDTC_instance;
};
}
#endif // REMOTE_DTC_SCOPE
