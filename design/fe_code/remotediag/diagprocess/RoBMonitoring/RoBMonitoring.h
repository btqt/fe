#ifndef REMOTE_ROB_MONITORING_H
#define REMOTE_ROB_MONITORING_H

#include <utils/Handler.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/Buffer.h>
#include <services/TimeManagerService/TimeManager.h>   
#include <unordered_map>
#include <services/OnboardclientManagerService/IOnboardclientManagerService.h>
#include <services/OnboardclientManagerService/IOnboardClientReceiver.h>
#include <services/OnboardclientManagerService/OBCResponseEventInfo.h>
#include <services/OnboardclientManagerService/OBCUDSResInfo.h>
#include <services/OnboardclientManagerService/OBCCanInfo.h>
#include <services/OnboardclientManagerService/OBCTransportInfo.h>

#include "services/OnboardclientManagerAdapter.h"
#include "services/HttpManagerAdapter.h"
#include "services/DiagManagerAdapter.h"
#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "utils/FileUtil.h"
#include "utils/PriorityControl.h"
#include "DiagTrigger.h"
#include "RemoteDelegate.h"
#include "Remotediag.h"
#include "utils/CollectionCondition.h"
#include "UdsMessage.h"
#include "utils/UploadManager.h"
#include "utils/UploadTask.h"

// using CollectionConditionRobRobSsrDidEvent = vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent;
using RobInformationPriority = vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_RobPriority;
// using RobMonitoringInformation = vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation;
// using UploadErrorDataRequest = vccomif::rdg::v1::interfaces::UploadErrorDataRequest;
// using EcuAddressInformation = vccomif::rdg::v1::interfaces::EcuAddressInformation;
// using DiagnosticsMessage = vccomif::rdg::v1::interfaces::DiagnosticsMessage;

namespace rdgapp {

class RoBMonitoring : public android::RefBase, public RemoteDelegate {
protected:
    bool checkPrecondition() override; 
public:
    enum class Abort: uint8_t {
        ABORT_INIT = 0,
        ABORT_PRIORITY_DISCARDED,
        ABORT_IG_STATE_CHANGE,
        ABORT_UNDER_REPAIR
    };

    static constexpr uint8_t APP_ID {RDG_APPID::ROB_MONITORING};

    RoBMonitoring(const Remotediag& app, android::sp<sl::SLLooper>& privLooper);
    ~RoBMonitoring() override;
    RoBMonitoring(const RoBMonitoring&) = default;
    RoBMonitoring(RoBMonitoring&&) = default;
    RoBMonitoring& operator=(const RoBMonitoring&) = default;
    RoBMonitoring& operator=(RoBMonitoring&&) = default;
    
    static RoBMonitoring *getInstance(void);

    void notifyBootComplete() const override;
    void onReceiveIG(const bool status) const override;
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) override;
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0) override;
    void onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) override;
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final { return std::map<uint64_t, android::sp<UdsMessage>>();}
    virtual uint8_t getAppId() const noexcept{return APP_ID;};
    error_t getRobMonitoringList(TargetCollectionDataOccurrentRobList& aList, Uint64& collectionConditionId);

    bool notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff); 

private:

    class MainHandler : public sl::Handler {
    public:

        static constexpr int32_t CMD_INIT_ROB_MONITORING {2000};
        static constexpr int32_t CMD_CHANGE_IG_STATUS {2001};
        static constexpr int32_t CMD_RECEIVE_STATUS_FROM_CENTER {2005};  //receive collection condition done
        static constexpr int32_t CMD_TIMER_EXPIRED {2011};
        static constexpr int32_t CMD_REQUEST_TO_PRIORITY_CONTROL {2014};
        static constexpr int32_t CMD_MONITORING_PRIORITY_TRIGGER {2015};
        // static constexpr int32_t CMD_READ_MONITORING_LIST {2016};
        static constexpr int32_t CMD_MONITORING_START_TRANMISSION {2017};
        static constexpr int32_t CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE {2018};
        static constexpr int32_t CMD_ROBMONITORING_FINISH_TRANSMISSION {2019};

        //update



        explicit MainHandler(android::sp<sl::SLLooper>& looper, RoBMonitoring& robMonitoringData) noexcept
                : android::RefBase(), sl::Handler(looper), mMonitoring(robMonitoringData) {}
        ~MainHandler() override = default;
        MainHandler(const MainHandler&) = default;
        MainHandler(MainHandler&&) = default;
        MainHandler& operator=(const MainHandler&) = default;
        MainHandler& operator=(MainHandler&&) = default;

        void handleMessage (const android::sp<sl::Message>& handlemsg) override;
    private:
        RoBMonitoring& mMonitoring;
    };

    class TimerHandler : public TimerTimeoutHandler {
    public:
        static constexpr int32_t ID_TRANSMISSION_TIMEOUT {3001};

        explicit TimerHandler(RoBMonitoring& robMonitoringData) noexcept : TimerTimeoutHandler(), mMonitoring(robMonitoringData) {}
        ~TimerHandler() override = default;
        TimerHandler(const TimerHandler&) = default;
        TimerHandler(TimerHandler&&) = default;
        TimerHandler& operator=(const TimerHandler&) = default;
        TimerHandler& operator=(TimerHandler&&) = default;

        void handlerFunction(const int32_t timerId) override {
            (void)mMonitoring.mHandler->obtainMessage(MainHandler::CMD_TIMER_EXPIRED, timerId)->sendToTarget();
        }

    private:
        RoBMonitoring& mMonitoring;
    };

    class MonitoringTransmission : public RefBase {
    public:

    //SFID: stop 06 - setting 03 -start 45
    //EventWindowTime : stop/start/setting: 02
    //setting: A005 - SID22 - A006

        enum class State: uint32_t {
            MONITORING_TRANS_INIT = 0U,
            MONITORING_TRANS_CONNECT,
            // MONITORING_TRANS_SEND_UDS,
            MONITORING_TRANS_SEND_UDS_START,
            MONITORING_TRANS_SEND_UDS_CONFIGURE,
            MONITORING_TRANS_SEND_UDS_STOP,
            MONITORING_TRANS_DISCONNECT,
            MONITORING_TRANS_DONE,
            MONITORING_TRANS_MAX
        };

        enum class Type: uint32_t {
            MONITORING_INIT = 10U,
            MONITORING_STOP,        //stop monitoting
            MONITORING_CONFIGURE,   //setting => start monitoring
            MONITORING_RECONFIGURE  //stop => setting => start monitoring
        };

        MonitoringTransmission( RoBMonitoring& robMonitoringData, const CommonDefine::EcuInformation ecuInformation, const Type typeData);
        ~MonitoringTransmission() = default;

        void connect();
        void disconnect();
        void sendUds(const MonitoringTransmission::State nState, const android::sp<::Buffer> uds);
        void stopTimeout();
        bool isContinueTransmission(MonitoringTransmission::State &nState, android::sp<::Buffer> &uds);
        std::string enumToString(const uint32_t input) const noexcept;

        uint16_t& connectId() noexcept{return mConnectId;};
        uint64_t& transmissionId() noexcept{return mTransmissionId;};
        State getStateData() const noexcept {return mState;}
        void setStateData(const State st) noexcept{mState = st;}
        Type getTypeData() const noexcept{return mType;}
        void setTypeData(const Type ty) noexcept{mType = ty;}
        CommonDefine::EcuInformation& ecuInformation() noexcept {return mEcuInformation;};
    private:
        static constexpr uint32_t TRANSMISSION_TIME_OUT_DURATION {195U};
        RoBMonitoring& mMonitoring;
        TimerHandler mTimerHandler;  
        Timer mTimeOut;

        uint16_t mConnectId;
        uint64_t mTransmissionId;
        State mState;
        Type mType;
        CommonDefine::EcuInformation mEcuInformation;

        UdsMessage mUdsReqStop;
        UdsMessage mUdsReqSetting;
        UdsMessage mUdsReqStart;
        UdsMessage mUdsRes;
    };

    using TransmissionInter = std::unordered_map<uint64_t, android::sp<MonitoringTransmission>>::iterator;
    std::unordered_map<uint64_t, android::sp<MonitoringTransmission>> mMonitoringTransList;
    std::queue<uint64_t> mTransmissionIdList;
    std::list<std::pair<EcuAddressInformation, DiagnosticsMessage>> mEcuInformationError;
    std::list<std::pair<EcuAddressInformation, DiagnosticsMessage>> mEcuInformationRes;

    std::list<std::pair<CommonDefine::EcuInformation, RoBMonitoring::MonitoringTransmission::Type>>  mTranmissionList;
    uint64_t mCurrentTransmissionId{0U};
    uint16_t mCurrentConnectId{0U};

    const Remotediag& mApp;
    static RoBMonitoring* mRoBMonitoring;
    android::sp<MainHandler> mHandler;
    TimerHandler* mTimerHandler;
    android::sp<CommonDefine::RDGLocationData> mLocationData;
    bool mIGState;
    std::unordered_map<uint32_t, android::sp<DiagTrigger>> mSaveReq;
    bool mPriority;  //Check RoBMonitoring is run in priority function
    int32_t mPriorityId;
    bool mIsMonitoringRunning;
    Abort mCurrentAbortState;
    uint64_t mColId;
    bool mIsPrioritySuspend; 
    bool mIsSendLastUploadData; //RDG30-R-1130  + RDG30-R-0552

    //Service Flag
    bool mRDGFlag = false;
    bool mConsentState = false;

    bool mUnderRepair = false;  

    bool mPPIFlag = false;
    DiagTrigger::DiagTriggerType mTriggerType;

    void printData(const std::string data) const;
    void handleIGStatus(const bool state);

    void initRoBMonitoring();
    void handleCollectionConditionUpdate();
    void changedUnderRepairState(const int32_t what, const int32_t info);

    void uploadIfNeeded() const;

    void handleMonitoringRequest(const int32_t pTriggerType);

    void handleTimerExpired(const int32_t timerID);
    void handleChangeErasePPIFlag() const;
    void createLastFile() const;
    void requestPriority(const uint32_t prio, const uint64_t colID);
    void handleTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff);

    bool getPPIFlag() const noexcept {return mPPIFlag;}
    void setPPIFlag(const bool pPPIFlag) noexcept {mPPIFlag = pPPIFlag;}
    void getServiceFlag();

    bool isValidCenterRequest();
    bool isValidECUAddress(const EcuAddressInformation ecu);
    bool isUnderMonitor(const EcuAddressInformation ecu);
    bool isAliveECU(const EcuAddressInformation ecu, CommonDefine::EcuInformation& ecuInformation );
    bool isMakeRequestMonitor(const EcuAddressInformation ecu);  //for there are many same address in a co-co request

    void makeRequestMonitoring(void);
    void startTranmission();
    void finishCurrentTransmission();
    void finishMonitoringTranmission();
    void onTransmissionTimeout();
    void makeUploadErrorData(const vccomif::rdg::v1::interfaces::ResponseCode resCode);   //Error in validate step
    void makeLastUploadErrorData();   //Error in setup monitoring steps
    void makeUploadResponseData();     //RDG30-R-0551  - RDG 130
    void convertEcuInformation(const CommonDefine::EcuInformation ecuInformation, EcuAddressInformation &ecu, const uint32_t rxAdd = 0U) const;  //convert commonDefine ECU to vcomif ECU

    std::shared_ptr<CollectionConditionRobRobSsrDidEvent> mMonitoringRequestData;  //Request Data from center
    std::shared_ptr<CollectionConditionRobRobSsrDidEvent> mMonitoringUploadData;  //Upload Data (save memory and upload to Center)
    std::shared_ptr<CollectionConditionRobRobSsrDidEvent> mMonitoringList;         //Temporary rob Monitoring
    //Package and upload data
    std::shared_ptr<UploadErrorDataRequest> mMonitoringUploadErrorData;
    std::shared_ptr<UploadRequestResponseNotificationRequest> mMonitoringUploadResponseData;

    void resetUploadData() const;
    void abortRoBMonitoring(const Abort abortReason);
    void modifyRoBInformationList(void) const;  // RDG30-R-0935 + RDG30-R-0558
    void notifyTriggerDone(); //For don't trigger from priority

    //handle RobMonitoring information list 
    error_t saveRoBInformationList(const CollectionConditionRobRobSsrDidEvent& obj);
    std::shared_ptr<CollectionConditionRobRobSsrDidEvent> getRoBInformationList(void) const;   
    uint8_t getOperation() const noexcept;
    void updateRoBInformationList(const EcuAddressInformation& ecu); 
    void removeStopECU(const EcuAddressInformation ecu);     //remove ECU which stop success in EMMC list
    void generateRoBInformationList();                       //Add EMMC list and Center list after monitoring done

    char_t uint8ToChar(const uint8_t num) const noexcept;

    std::list<CommonDefine::EcuInformation> mEcuInformationList;
public:
    error_t clearRoBInformationList(void) const;      //delete file
};

} // namespace rdgapp
#endif // REMOTE_ROB_MONITORING_H`
