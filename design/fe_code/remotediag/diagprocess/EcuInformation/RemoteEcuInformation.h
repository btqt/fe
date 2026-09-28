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

class RemoteEcuInformation : public android::RefBase, public RemoteDelegate
{
protected:
    bool startDiagTask();
    bool checkPrecondition();
    void processErrorHandling();

public:
    static constexpr uint8_t APP_ID {RDG_APPID::ECUINFORMATION};
    RemoteEcuInformation(const Remotediag &app, android::sp<sl::SLLooper> &privateLooper);
    ~RemoteEcuInformation() override;
    static RemoteEcuInformation *getInstance();
    RemoteEcuInformation(const RemoteEcuInformation &) = default;
    RemoteEcuInformation(RemoteEcuInformation &&) = default;
    RemoteEcuInformation &operator=(const RemoteEcuInformation &) = default;
    RemoteEcuInformation &operator=(RemoteEcuInformation &&) = default;

    void notifyBootComplete() const noexcept override{};
    void onReceiveIG(const bool status) const override;
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) override;
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0);
    void onCenterCommandForward(const android::sp<CenterReqData> &pCenterReqData);
    uint8_t getAppId() const noexcept override { return APP_ID; };
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final {return std::map<uint64_t, android::sp<UdsMessage>>();};

    void getEcuInformationList(std::list<CommonDefine::EcuInformation> &l_EcuInformation);
    void insertEcuInformation(const CommonDefine::EcuInformation ecu);
    OBCEnum::OBCProtocolType convertObcProtocolType(const CommunicationProtocol protocol, const CommunicationType type, const uint32_t targetAddress) const noexcept;
    OBCEnum::OBCProtocolType getObcProtocolType(const uint32_t targetAddress);
    CommunicationType getCommType(const uint32_t canId);
    uint32_t getCanId(const uint32_t targetAddress);
    void clearEcuInformationList();
    void triggerEcuInfo(const uint32_t prio, const uint64_t colID, const DiagTrigger::DiagTriggerType trigType);
    void notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const uint32_t& pTriggerId, const bool dueToIgOff);
    void handleTrigger(const DiagTrigger::DiagTriggerState& pState, const uint32_t& pTriggerId, const bool dueToIgOff);
    void handleUnderRepairStatusChange(const int32_t& what, const int32_t& status);

    //for SLDD
    void testStartDiagTask() { (void)startDiagTask(); };
    void testLoadEcuInformationFromFile();
    void testSaveEcuInformationToFile();

private:
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
        static constexpr int32_t CMD_ECUINFO_TRANSMISSION_FINISH               {4010};

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
        enum class EcuTransState: uint8_t {
            ECU_TRANS_INIT = 0,
            ECU_TRANS_CONNECT,
            ECU_TRANS_SEND_UDS,
            ECU_TRANS_DISCONNECT,
            ECU_TRANS_DONE
        };
        EcuUdsTransmission( RemoteEcuInformation& ecu, const uint32_t mCanId, const uint8_t mNTa, const uint8_t mProtocolType, const android::sp<::Buffer> UdsData);
        ~EcuUdsTransmission() = default;

        void connect();
        void disconnect();
        void send();
        void stopTimeout();
        void resetTimer();
        uint64_t getTransmissionId() const noexcept;
        EcuTransState getState() const noexcept;
        uint16_t getConnectId() const noexcept;
        void setRequestType(const bool isFunctional) noexcept;
        bool checkfunctionRequest() const noexcept;
    private:
        static constexpr uint32_t TRANSMISSION_TIME_OUT_DURATION    {200U};
        RemoteEcuInformation& mECUInfo;
        TimerHandler mTimerHandler;  
        uint8_t nTa;
        uint32_t canId;
        uint8_t protocolType;
        uint16_t connectId;
        uint64_t transmissionId;
        EcuTransState state;
        bool isFunctionalRequest;
        Timer mTimeOut;
        UdsMessage udsReq;
    };

private:
    enum class EcuState: uint8_t
    {
        ECU_IDLE = 0,
        ECU_EXISTENCE_CHECK,
        ECU_DID_DATA_ACQUISITION
    };
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
    uint8_t getOperation() const noexcept;
    void handleReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo);
    void handleTransmissionTimeout();

    const Remotediag &mApp;
    android::sp<sl::Handler> mHandler;
    TimerHandler *mTimerHandler;
    std::list<CommonDefine::EcuInformation> m_EcuInformationList;
    std::unordered_map<uint32_t, CommonDefine::EcuInformation> mEcuInformationMap;
    static RemoteEcuInformation *mRemoteEcuInformation;
    EcuState mState;
    bool mIsEcuRunning;
    bool mIsSuspending;
    std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>> mEcuTransList;
    std::queue<uint64_t> mTransmissioIdList;
    uint64_t mCurrentTransmissionId;
    std::unordered_map<uint32_t, android::sp<DiagTrigger>> mSaveReq;
    uint32_t mPriority;
    uint64_t mColId;
    uint64_t mLastUpdateTime;
    bool mIsNeedUploadErrorData;
    std::shared_ptr<vccomif::rdg::v1::interfaces::UploadErrorDataRequest> mEcuUploadErrorData;
    uint32_t mTriggerId;
    DiagTrigger::DiagTriggerType mTriggerType;
    bool m_srvc_disregard_flag;
};
}
#endif // REMOTE_ECUINFORMATION_H
