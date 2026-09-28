#ifndef REMOTE_ECUINFORMATION_H
#define REMOTE_ECUINFORMATION_H
#include <iostream>
#include <fstream>
#include <list>
#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>

#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "Remotediag.h"
#include "EcuInformationDataType.h"
#include "RemoteDelegate.h"
#include "DiagTrigger.h"
#include "utils/PriorityControl.h"
#include "UdsMessage.h"
#include "utils/CollectionCondition.h"
#include "services/PowerManagerAdapter.h"
#include "services/OnboardclientManagerAdapter.h"
#include <services/TimeManagerService/TimeManager.h>

class Remotediag;

namespace rdgapp {

static const std::string ECU_INFORMATION_LIST_PATH_TEST {DATA_PATH + "ecu_info_list_test"};
static const std::string ECU_INFORMATION_LIST_PATH      {DATA_PATH + "ecu_info_list"};

class RemoteEcuInformation : public android::RefBase, public RemoteDelegate
{
protected:
    void startDiagTask();
    bool checkPrecondition();
    void processErrorHandling();

public:
    static constexpr uint8_t APP_ID {RDG_APPID::ECUINFORMATION};
    RemoteEcuInformation(const Remotediag &app, android::sp<sl::SLLooper> &privateLooper);
    ~RemoteEcuInformation() override;
    static android::sp<RemoteEcuInformation> getInstance();
    RemoteEcuInformation(const RemoteEcuInformation &) = default;
    RemoteEcuInformation(RemoteEcuInformation &&) = default;
    RemoteEcuInformation &operator=(const RemoteEcuInformation &) = default;
    RemoteEcuInformation &operator=(RemoteEcuInformation &&) = default;

    void notifyBootComplete() const noexcept override{};
    void onReceiveIG(const bool status) const override;
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) override;
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0);
    void onCenterCommandForward(const android::sp<CenterReqData> &pCenterReqData);
    void onRdgStop(const bool isStop) const noexcept override;
    uint8_t getAppId() const noexcept override { return APP_ID; };
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final {return std::map<uint64_t, android::sp<UdsMessage>>();};

    void getEcuInformationList(std::list<CommonDefine::EcuInformation> &l_EcuInformation);
    void insertEcuInformation(const CommonDefine::EcuInformation ecu);
    OBCEnum::OBCProtocolType convertObcProtocolType(const CommunicationProtocol protocol, const CommunicationType type, const uint32_t targetAddress) const noexcept;
    OBCEnum::OBCProtocolType getObcProtocolType(const uint32_t targetAddress);
    CommunicationType getCommType(const uint32_t canId);
    uint32_t getCanId(const uint32_t targetAddress);
    bool isEcuExisted(const uint32_t targetAddress) noexcept;
    void clearEcuInformationList();
    void triggerEcuInfo(const uint32_t prio, const uint64_t colID, const DiagTrigger::DiagTriggerType trigType);
    void notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const uint32_t& pTriggerId, const bool dueToIgOff);
    void handleTrigger(const DiagTrigger::DiagTriggerState& pState, const uint32_t& pTriggerId, const bool dueToIgOff);
    void handleUnderRepairStatusChange(const int32_t& what, const int32_t& status);
    void finishAcquisition();

    //for SLDD
    void testStartDiagTask() { startDiagTask(); };
    void testLoadEcuInformationFromFile();
    void testSaveEcuInformationToFile();

private:
    enum class EcuState: uint8_t
    {
        ECU_IDLE = 0,
        ECU_RUNNING,
        ECU_SUSPENDING,
        ECU_SUSPEND_PENDING
    };
    enum class TransmissionType: uint8_t
    {
        TYPE_NONE,
        TYPE_EXISTENCE,
        TYPE_DID_PHASE_5,
        TYPE_DID_PHASE_6
    };
    enum class EcuSession: uint8_t
    {
        SESSION_DEFAULT = 0x01U,
        SESSION_REMOTE = 0x40U,
        SESSION_FAIL_REMOTE = 0xFFU,
    };
    class MainHandler : public sl::Handler
    {
    public:
        static constexpr int32_t CMD_INIT_ECU_INFORMATION               {4000};
        static constexpr int32_t CMD_TRIGGER_FROM_CENTER                {4001};
        static constexpr int32_t CMD_START_ECU_INFORMATION              {4002};
        static constexpr int32_t CMD_REQUEST_TO_PRIORITY_CONTROL        {4003};
        static constexpr int32_t CMD_MAKE_UPLOAD_REQUEST                {4004};
        static constexpr int32_t CMD_START_DID_ACQUISITION              {4005};
        static constexpr int32_t CMD_CHANGE_IG_STATUS                   {4006};
        static constexpr int32_t CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE   {4007};
        static constexpr int32_t CMD_RECEIVE_UDS_RESPONSE               {4008};
        static constexpr int32_t CMD_TRANSMISSION_TIMEOUT               {4009};
        static constexpr int32_t CMD_ECUINFO_TRANSMISSION_FINISH        {4010};
        static constexpr int32_t CMD_STOP_RDG                          {4011};

        explicit MainHandler(android::sp<sl::SLLooper> &privateLooper, RemoteEcuInformation &ecu_info) noexcept
            : android::RefBase(), sl::Handler(privateLooper), mECUInfo(ecu_info) {}
        ~MainHandler() override = default;
        MainHandler(const MainHandler &) = default;
        MainHandler(MainHandler &&) = default;
        MainHandler &operator=(const MainHandler &) = default;
        MainHandler &operator=(MainHandler &&) = default;
        void handleMessage(const android::sp<sl::Message> &handlemsg) override;

    private:
        RemoteEcuInformation &mECUInfo;
    };

    class TimerHandler : public TimerTimeoutHandler
    {
    public:
        static constexpr int32_t ID_TRANSMISSION_TIMEOUT    {4100};

        explicit TimerHandler(RemoteEcuInformation &ecu_info) noexcept
            : TimerTimeoutHandler(), mECUInfo(ecu_info) {}
        ~TimerHandler(void) override = default;
        TimerHandler(const TimerHandler &) = default;
        TimerHandler(TimerHandler &&) = default;
        TimerHandler &operator=(const TimerHandler &) = default;
        TimerHandler &operator=(TimerHandler &&) = default;

        void handlerFunction(const int32_t timerId) override;

    private:
        RemoteEcuInformation &mECUInfo;
    };

    class EcuUdsTransmission : public RefBase
    {
    public:
        EcuUdsTransmission( RemoteEcuInformation& ecu, const uint32_t canId, const uint8_t protocolType, const TransmissionType transType);
        ~EcuUdsTransmission() = default;

        void connect();
        void disconnect();
        void sendUdsRequest();
        void stopTimeout();
        uint64_t getTransmissionId() const noexcept;
        uint16_t getConnectId() const noexcept;
        uint8_t getUdsReqSID() const noexcept;
        uint8_t getUdsReqSFID() const noexcept;
        bool checkfunctionRequest() const noexcept;
        TransmissionType getTransmissionType() const noexcept {return mTransmissionType;}
        void setSession(const EcuSession session) noexcept {mSession = session;}
        void nextRequestIndex() noexcept {mCurrentRequestIndex < SIZE_MAX ? mCurrentRequestIndex++ : 0U;}

    private:
        static constexpr uint32_t TRANSMISSION_TIME_OUT_DURATION    {195U};
        RemoteEcuInformation& mECUInfo;
        TimerHandler mTimerHandler;  
        uint32_t mCanId;
        uint8_t mProtocolType;
        uint16_t mConnectId;
        uint64_t mTransmissionId;
        bool mIsFunctionalRequest;
        Timer mTimeOut;
        std::vector<android::sp<UdsMessage>> mUdsReqList;
        size_t mCurrentRequestIndex;
        TransmissionType mTransmissionType;
        EcuSession mSession;
    };

    struct EcuPartNumber
    {
    private:
        std::string swPartNumber;
        std::string hwPartNumber;

    public:
        EcuPartNumber() : swPartNumber(""), hwPartNumber("") {}
        void setSwPartNumber(const std::string val) noexcept { swPartNumber = val; }
        std::string getSwPartNumber() const noexcept { return swPartNumber; }
        void setHwPartNumber(const std::string val) noexcept { hwPartNumber = val; }
        std::string getHwPartNumber() const noexcept { return hwPartNumber; }
    };

private:
    void printData(const std::string data) const;
    void loadEcuInfoListFromFile();
    void saveEcuInfoListToFile();
    void deactiveAllEcu() noexcept;
    void startEcuExistenceCheck();
    void startDidAcquisition();
    void updateEcuInformationList(const CommonDefine::EcuInformation ecuInformation);
    void handleExistenceCheckResponse(const uint8_t protocolType, const android::sp<OBCCanInfo> canInfo, const android::sp<UdsMessage> udsResponse);
    void handleReadDidResponse(const uint8_t protocolType, const android::sp<OBCCanInfo> canInfo, const android::sp<UdsMessage> udsResponse);
    void makeUploadRequest();
    void onTransmissionTimeout(void);
    void finishCurrentTransmission(void);
    void changeIGStatus(const bool status);
    void handleReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo);
    void handleTransmissionTimeout();
    void handleStopRDG();

    const Remotediag &mApp;
    android::sp<sl::Handler> mHandler;
    TimerHandler *mTimerHandler;
    std::list<CommonDefine::EcuInformation> m_EcuInformationList;
    static android::sp<RemoteEcuInformation> mRemoteEcuInformation;
    EcuState mState;
    std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>> mEcuTransList;
    std::queue<uint64_t> mTransmissioIdList;
    uint64_t mCurrentTransmissionId;
    std::unordered_map<uint32_t, android::sp<DiagTrigger>> mSaveReq;
    uint32_t mPriority;
    uint64_t mColId;
    uint64_t mLastUpdateTime;
    uint64_t mDataCreationDate;
    bool mIsNeedUploadErrorData;
    std::shared_ptr<vccomif::rdg::v1::interfaces::UploadErrorDataRequest> mEcuUploadErrorData;
    uint32_t mTriggerId;
    DiagTrigger::DiagTriggerType mTriggerType;
    bool m_srvc_disregard_flag;
    std::unordered_map<uint32_t, EcuPartNumber> mEcuPartNumberMap;
};
}
#endif // REMOTE_ECUINFORMATION_H
