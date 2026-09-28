#ifndef REMOTE_DIAG_COLLECTION_CONDITIO_H
#define REMOTE_DIAG_COLLECTION_CONDITIO_H
#include <memory>
#include <queue>
#include <google/protobuf/util/json_util.h>
#include "diagprocess/DiagTrigger.h"
#include "CenterRequestJob.h"
#include "services/HttpManagerAdapter.h"

namespace rdgapp {

namespace RdgProtoInterface = vccomif::rdg::v1::interfaces;

using GetCollectionConditionResponse = RdgProtoInterface::GetCollectionConditionResponse;
using GetCollectionConditionRequest = RdgProtoInterface::GetCollectionConditionRequest;

using AppCommonHeaderVehicleToCenterElectronicPf = ::vccomif::common::v1::AppCommonHeaderVehicleToCenter_ElectronicPf;
using UploadErrorDataRequest = RdgProtoInterface::UploadErrorDataRequest;
using UploadErrorDataRequestFunctionType = RdgProtoInterface::UploadErrorDataRequest_FunctionType;

using NotifyCollectionConditionUpdateResultRequest = RdgProtoInterface::NotifyCollectionConditionUpdateResultRequest;
using UploadDtcDataRequest = RdgProtoInterface::UploadDtcDataRequest;
using UploadSsrDataRequest = RdgProtoInterface::UploadSsrDataRequest;
using UploadRobDataRequest = RdgProtoInterface::UploadRobDataRequest;
using UploadRobSsrDataRequest = RdgProtoInterface::UploadRobSsrDataRequest;
using UploadDirectCommandDataRequest = RdgProtoInterface::UploadDirectCommandDataRequest;
using UploadWarningInformationRequest = RdgProtoInterface::UploadWarningInformationRequest;
using UploadEcuInformationRequest = RdgProtoInterface::UploadEcuInformationRequest;
using UploadLastDataRequest = RdgProtoInterface::UploadLastDataRequest;
using UploadRequestResponseNotificationRequest = RdgProtoInterface::UploadRequestResponseNotificationRequest;
using UploadDdrDataRequest = RdgProtoInterface::UploadDdrDataRequest;

using CollectionConditionRes = RdgProtoInterface::GetCollectionConditionResponse_CollectionCondition;

using CenterRequestAllDtcSsr = RdgProtoInterface::GetCollectionConditionResponse_CenterRequestAllDtcSsr;
using CenterRequestAllRob = RdgProtoInterface::GetCollectionConditionResponse_CenterRequestAllRob;
using CenterRequestEcuInformation = RdgProtoInterface::GetCollectionConditionResponse_CenterRequestEcuInformation;
using CenterRequestRobSsr = RdgProtoInterface::GetCollectionConditionResponse_CenterRequestRobSsr;
using CenterRequestDirectCommand = RdgProtoInterface::GetCollectionConditionResponse_CenterRequestDirectCommand;
using CenterRequestRobSsrList = google::protobuf::RepeatedPtrField<CenterRequestRobSsr>;
using CenterRequestRobSsrIter = google::protobuf::RepeatedPtrField<CenterRequestRobSsr>::iterator;
using CenterRequestDirectCommandList = google::protobuf::RepeatedPtrField<CenterRequestDirectCommand>;
using CenterRequestDirectCommandIter = google::protobuf::RepeatedPtrField<CenterRequestDirectCommand>::iterator;

using CollectionConditionDiagCommon = RdgProtoInterface::GetCollectionConditionResponse_CollectionConditionDiagCommon;
using CollectionConditionRobRobSsrDidEvent = RdgProtoInterface::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent;
using CollectionConditionEcuInformation = RdgProtoInterface::GetCollectionConditionResponse_CollectionConditionEcuInformation;
using CollectionConditionWarningInformation = RdgProtoInterface::GetCollectionConditionResponse_CollectionConditionWarningInformation;
using CollectionConditionDirectCommand = RdgProtoInterface::GetCollectionConditionResponse_CollectionConditionDirectCommand;

using CollectionConditionDirectCommandList = google::protobuf::RepeatedPtrField<CollectionConditionDirectCommand>;
using CollectionConditionDirectCommandIter = google::protobuf::RepeatedPtrField<CollectionConditionDirectCommand>::iterator;

using DirectCommand = RdgProtoInterface::DirectCommand;
using DirectCommandList = google::protobuf::RepeatedPtrField<RdgProtoInterface::DirectCommand>;

using Uint64 = google::protobuf::uint64;
using Uint64List = google::protobuf::RepeatedField<Uint64>;
using ConstUint64Iter = google::protobuf::RepeatedField<Uint64>::const_iterator;

using TargetCollectionDataOccurrentRob = RdgProtoInterface::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData;
using TargetCollectionDataOccurrentRobList = google::protobuf::RepeatedPtrField<TargetCollectionDataOccurrentRob>;
using TargetCollectionDataOccurrentRobIter = TargetCollectionDataOccurrentRobList::iterator;
using DiagnosticsCommand = RdgProtoInterface::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_DiagnosticsCommand;
using DiagnosticsCommandIter = google::protobuf::RepeatedPtrField<DiagnosticsCommand>::iterator;

using TargetCollectionDataRobSsr = RdgProtoInterface::GetCollectionConditionResponse_CenterRequestRobSsr_TargetCollectionData;

using EcuAddressInformation = RdgProtoInterface::EcuAddressInformation;

using RobInformationOccurrentRob = RdgProtoInterface::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation;
using RobInformationOccurrentRobList = google::protobuf::RepeatedPtrField<RobInformationOccurrentRob>;
using RobInformationOccurrentRobIter = RobInformationOccurrentRobList::iterator;
using CrcInformation = RdgProtoInterface::CrcInformation;

using DiagnosticsMessage = RdgProtoInterface::DiagnosticsMessage;
using DiagnosticsMessageList = google::protobuf::RepeatedPtrField<DiagnosticsMessage>;

using UpdateResultCode = RdgProtoInterface::NotifyCollectionConditionUpdateResultRequest_UpdateResultCode;

using ErrorInformation = RdgProtoInterface::NotifyCollectionConditionUpdateResultRequest_ErrorInformation;
using ErrorInformationList = google::protobuf::RepeatedPtrField<ErrorInformation>;
// Types
using UpdateTypeSingle = RdgProtoInterface::GetCollectionConditionResponse_UpdateTypeSingle;
using UpdateTypeMultiple = RdgProtoInterface::GetCollectionConditionResponse_UpdateTypeMultiple;
using RequestHeaderInterfaceType = RdgProtoInterface::RdgCommonRequestHeader_InterfaceType;

using ScheduleInformation = RdgProtoInterface::ScheduleInformation;
using ScheduleType = RdgProtoInterface::ScheduleInformation_ScheduleType;
using CommunicationProtocol = RdgProtoInterface::EcuAddressInformation_CommunicationProtocol;
using CommunicationType = RdgProtoInterface::EcuAddressInformation_CommunicationType;

constexpr static uint8_t MAX_RETRY {3U};
constexpr static uint8_t SCHEDULE_INTERVAL_DAYS_MAX {0x1FU};    // 31 days

constexpr static uint8_t SCHEDULE_INTERVAL_HOURS_MAX {0x17U};  // 23 hours
constexpr static uint8_t SCHEDULE_INTERVAL_MINUTES_MAX {0x3BU}; // 59 minutes
constexpr static int32_t NUMBER_RoBSSR_ACQUISITION_MAX {200};
constexpr static int32_t NUMBER_CENTER_REQUEST_DIRECT_COMMAND_MAX {70};
constexpr static int32_t NUMBER_COLLECTION_CONDITION_DIRECT_COMMAND_MAX {30};
constexpr static int32_t NUMBER_DIRECT_COMMAND_MAX {512};
constexpr static int32_t NUMBER_OF_TARGET_COLLECTION_DATA_MAX {500};
constexpr static uint32_t WARNING_PROPERTY_TABLE_SIZE_MAX {256U}; // (bytes)

constexpr static uint32_t MAX_RETRY_COUNT {5U};
constexpr static uint32_t REQUEST_TIMEOUT{60U}; // 1 minute
constexpr static uint32_t FIRST_RETRY_TIMEOUT{10U};
constexpr static uint32_t SECOND_RETRY_TIMEOUT{60U};
constexpr static uint32_t THIRD_RETRY_TIMEOUT{300U};
constexpr static uint32_t ID_IG_ON_STATE_MAINTAINED_CHECK_TIMEOUT{60U}; // 1 minute


constexpr static int32_t IG_ON_STATE_MAINTAINED_CHECK_ID{3000};
constexpr static int32_t RETRY_TIMER_ID{3001};

constexpr static ScheduleType ScheduleType_MIN{RdgProtoInterface::ScheduleInformation_ScheduleType_ScheduleType_MIN};
constexpr static ScheduleType ScheduleType_MAX{RdgProtoInterface::ScheduleInformation_ScheduleType_ScheduleType_MAX};
constexpr static ScheduleType ScheduleType_PERIOD_TRIGGER_ROUTINE{RdgProtoInterface::ScheduleInformation_ScheduleType_ST_PERIOD_TRIGGER_ROUTINE};

constexpr static UpdateResultCode URC_FAILED{RdgProtoInterface::NotifyCollectionConditionUpdateResultRequest_UpdateResultCode_URC_FAILED};
constexpr static UpdateResultCode URC_SUCCESS{RdgProtoInterface::NotifyCollectionConditionUpdateResultRequest_UpdateResultCode_URC_SUCCESSFUL};

constexpr static UpdateTypeSingle UpdateTypeSingle_MIN{RdgProtoInterface::GetCollectionConditionResponse_UpdateTypeSingle_UpdateTypeSingle_MIN};
constexpr static UpdateTypeSingle UpdateTypeSingle_MAX{RdgProtoInterface::GetCollectionConditionResponse_UpdateTypeSingle_UpdateTypeSingle_MAX};
constexpr static UpdateTypeMultiple UpdateTypeMultiple_MIN{RdgProtoInterface::GetCollectionConditionResponse_UpdateTypeMultiple_UpdateTypeMultiple_MIN};
constexpr static UpdateTypeMultiple UpdateTypeMultiple_MAX{RdgProtoInterface::GetCollectionConditionResponse_UpdateTypeMultiple_UpdateTypeMultiple_MAX};

static const std::string GET_COLLECTION_CONDITION_RES_TEST_PATH {DATA_PATH + "get_collection_condition_res.json"};
static const std::string GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH {DATA_PATH + "get_collection_condition_res_serial.bin"};

class CollectionCondition : public android::RefBase, public android::Singleton<CollectionCondition>
{
public:
    constexpr static int32_t CMD_CENTER_PUSH_NOTIFICATION{2000};
    constexpr static int32_t CMD_SEND_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT_REQUEST{2001};
    constexpr static int32_t CMD_FEATURE_STATUS_CHANGE{2002};
    constexpr static int32_t CMD_HANDLE_IG_STATUS_CHANGE{2003};
    constexpr static int32_t CMD_RECEIVE_COLLECTION_CONDITION_REQUEST_RESPONSE{2004};
    constexpr static int32_t CMD_RECEIVE_COLLECTION_CONDITION_UPDATE_RESULT_RESPONSE{2005};
    constexpr static int32_t CMD_HANDLE_GRPC_COMMUNICATION_RECONNECT{2006};
    constexpr static int32_t CMD_HANDLE_IG_ON_TIMEOUT{2007};
    constexpr static int32_t CMD_HANDLE_TRANSMISSON_TIMEOUT{2008};
    constexpr static int32_t CMD_STOP_RDG{2009};
    using Uint32 = google::protobuf::uint32;
    using RobInformation = RdgProtoInterface::GetCollectionConditionResponse_CenterRequestRobSsr_TargetCollectionData_RobInformation;
    using RobInformationOccurrentRobPriority = RdgProtoInterface::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_RobPriority;

    using DirectCommandIter = google::protobuf::RepeatedPtrField<RdgProtoInterface::DirectCommand>::iterator;

    using RdgCommonRequestHeader = RdgProtoInterface::RdgCommonRequestHeader;

    enum class CollectionConditionType: uint8_t
    {
        CC_DID_EVENT = 0,
        CC_ECU_INFORMATION = 1,
        CC_WARNING_INFORMATION = 2,
        CC_DIRECT_COMMAND = 3,
        CC_MAX = 4
    };

    CollectionCondition();
    ~CollectionCondition() = default;

    void init(android::sp<sl::SLLooper> &privateLooper);

    void onReceivedCenterRequest(void);
    void onReceivedGetCollectionConditionResponse(const android::sp<GrpcResData> pGrpcResData);
    void onReceivedNotifyCollectionConditionUpdateResultResponse(const android::sp<GrpcResData> pGrpcResData);
    void onReceiveIG(const bool status);
    void onRdgStop(const bool isStop) const;
    void onGrpcReconnect(void);
    void onOtherFeatureStatusOff();

    void testPrintCollectionConditionData(void);
    void testReceivedCollectionConditionResponse() const;
    void testReceivedCollectionConditionResponseBinData() const;

    std::shared_ptr<GetCollectionConditionRequest> makeGetCollectionConditionRequest(const DiagTrigger::DiagTriggerType type);

    UpdateResultCode verifyScheduleInformationForCentrerRequest(ErrorInformation &errorInfo, const ScheduleInformation &inputData) const;
    UpdateResultCode verifyScheduleInformationForCollectionCondition(ErrorInformation &errorInfo, const ScheduleInformation &inputData) const;
    UpdateResultCode verifyCenterRequestAllDtcSsr(ErrorInformation &errorInfo, const CenterRequestAllDtcSsr &inputData);
    UpdateResultCode verifyCenterRequestAllRob(ErrorInformation &errorInfo, const CenterRequestAllRob &inputData);
    UpdateResultCode verifyCenterRequestRobSsr(ErrorInformation &errorInfo, const CenterRequestRobSsr &inputData);
    UpdateResultCode verifyEcuAddressInformation(ErrorInformation &errorInfo, const EcuAddressInformation &inputData) const;
    UpdateResultCode verifyCenterRequestEcuInformation(ErrorInformation &errorInfo, const CenterRequestEcuInformation &inputData);
    UpdateResultCode verifyCenterRequestDirectCommand(ErrorInformation &errorInfo, const CenterRequestDirectCommand &inputData);

    UpdateResultCode verifyCenterRequestRobSsrs(ErrorInformationList &errorInfoList, const CenterRequestRobSsrList &inputDataList);

    UpdateResultCode verifyCollectionConditionDiagCommon(ErrorInformation &errorInfo, const CollectionConditionDiagCommon &inputData) const;
    UpdateResultCode verifyCollectionConditionRobRobSsrDidEvent(ErrorInformation &errorInfo, const CollectionConditionRobRobSsrDidEvent &inputData);
    UpdateResultCode verifyCollectionConditionEcuInformation(ErrorInformation &errorInfo, const CollectionConditionEcuInformation &inputData);
    UpdateResultCode verifyCollectionConditionWarningInformation(ErrorInformation &errorInfo, const CollectionConditionWarningInformation &inputData) const;
    UpdateResultCode verifyCollectionConditionDirectCommand(ErrorInformation &errorInfo, const CollectionConditionDirectCommand &inputData);

    error_t saveCollectionConditionDiagCommon(const CollectionConditionDiagCommon &obj);
    std::shared_ptr<CollectionConditionDiagCommon> getCollectionConditionDiagCommon(void) const noexcept;

    error_t saveCollectionConditionRobRobSsrDidEvent(const CollectionConditionRobRobSsrDidEvent &obj);
    std::shared_ptr<CollectionConditionRobRobSsrDidEvent> getCollectionConditionRobRobSsrDidEvent(void) const noexcept;

    error_t saveCollectionConditionEcuInformation(const CollectionConditionEcuInformation &obj);
    std::shared_ptr<CollectionConditionEcuInformation> getCollectionConditionEcuInformation(void) const noexcept;

    error_t saveCollectionConditionWarningInformation(const CollectionConditionWarningInformation &obj);
    std::shared_ptr<CollectionConditionWarningInformation> getCollectionConditionWarningInformation(void) const noexcept;

    error_t saveCollectionConditionDirectCommand(const CollectionConditionDirectCommand &obj) const;
    std::shared_ptr<CollectionConditionDirectCommand> getCollectionConditionDirectCommand(const uint64_t collectionConditionId) const;
    error_t deleteCollectionConditionDirectCommand(const uint64_t collectionConditionId) const;
    bool checkExitsCollectionConditionDirectCommand(const uint64_t collectionConditionId) const;

    std::shared_ptr<GetCollectionConditionRequest> getGetCollectionConditionRequest() const noexcept;
    const CollectionConditionDirectCommandList &getCollectionConditionDirectCommandRequest(void);
    void removeErrorDirectCommand(const uint64_t collId, vector<uint32_t> vRemoveIdx);
    std::vector<uint64_t> getNewCenterRequestList(void);

    inline std::vector<uint64_t> getDeletedCollectionConditionIds(void) const noexcept {return mDeletedCollectionConditionIds;}
    inline std::vector<std::pair<uint64_t, uint8_t>> getUpdatedCollectionConditionIds(void) const noexcept {return mUpdatedCollectionConditionIds;}

    error_t removeCenterRequestJob(const uint64_t aJob);
    void onFinishCenterRequestJob(const uint64_t aJob);
    const std::shared_ptr<CenterRequestJob> getCenterRequestJob(const uint64_t aJob);
    android::sp<sl::Handler> getHandler(void) const noexcept;

private:



private:

    void handleCenterRespondSuccess(const android::sp<GrpcResData> pGrpcResData);
    class MainHandler : public sl::Handler
    {

    public:
        constexpr static int32_t CMD_CENTER_PUSH_NOTIFICATION{2000};
        constexpr static int32_t CMD_SEND_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT_REQUEST{2001};
        

        explicit MainHandler(android::sp<sl::SLLooper> &aLooper, CollectionCondition &inst) noexcept
            : android::RefBase(), sl::Handler(aLooper), mCollectionCondition(inst) {}
        MainHandler(const MainHandler &) = default;
        MainHandler(MainHandler &&) = default;
        MainHandler &operator=(const MainHandler &) = default;
        MainHandler &operator=(MainHandler &&) = default;
        ~MainHandler() override = default;
        void handleMessage(const android::sp<sl::Message> &handlemsg) override;

    private:
        CollectionCondition &mCollectionCondition;
    };

    class TimerHandler : public TimerTimeoutHandler
    {
    public:

        explicit TimerHandler(CollectionCondition &inst) noexcept : TimerTimeoutHandler(), mCollectionCondition(inst) {}
        ~TimerHandler() override = default;
        TimerHandler(const TimerHandler &) = default;
        TimerHandler(TimerHandler &&) = default;
        TimerHandler &operator=(const TimerHandler &) = default;
        TimerHandler &operator=(TimerHandler &&) = default;

        void handlerFunction(const int32_t timerId) override;

    private:
        CollectionCondition &mCollectionCondition;
    };
    class CocoTransmission : public RefBase {
    public:
        enum class State: uint8_t {
            COCO_TRANS_IDLE = 0U,
            COCO_TRANS_SEND_REQUEST,
            COCO_TRANS_OPERATION_A,
            COCO_TRANS_OPERATION_B,
            COCO_TRANS_FINISHED
        };
        
        CocoTransmission(CollectionCondition &inst, const DiagTrigger::DiagTriggerType triggerType, const GRPC_IF_TYPE type);
        ~CocoTransmission() final = default;

        void send();
        void doOperationB(const int32_t httpRetryTime);
        void doOperationA();
        void stopTimeout();
        void startTimeout(const uint32_t duration);
        void onNetworkOnline();
        void onFeatureStatusOn();
        
        uint32_t updateRetryDuration();
        void handleTransmissionTimeout();

        inline void setState(const State tmp) noexcept { mState = tmp; }
        inline void setNetworkOutOfRange(const bool value) noexcept { mNetworkOutOfRange = value; }
        inline void setDuration(const uint32_t value) noexcept { mDuration = value; }
        inline void setReqPayload(const std::shared_ptr<google::protobuf::Message>& payload) noexcept { mReqPayload = payload; }
        inline void setRequestType(const GRPC_IF_TYPE type) noexcept { mRequestType = type; }
        inline void setRetryCounter(const uint8_t value) noexcept { mRetryCounter = value; }
        inline State getState() const noexcept { return mState; }
        inline uint32_t getDuration(void) const noexcept { return mDuration; }
        inline uint8_t getRetryCounter() const noexcept { return mRetryCounter; }
        inline DiagTrigger::DiagTriggerType getTriggerType() const noexcept { return mTriggerType; }
        inline int32_t getCallId() const noexcept { return mCallId; }
        inline const GRPC_IF_TYPE getRequestType() const noexcept { return mRequestType; }

    private:
        class TransmissionTimerHandler : public TimerTimeoutHandler
        {
        public:

            explicit TransmissionTimerHandler(CocoTransmission& inst) noexcept 
                : TimerTimeoutHandler()
                , mTransmission(inst)
                {}

            ~TransmissionTimerHandler() override = default;
            TransmissionTimerHandler(const TransmissionTimerHandler &) = default;
            TransmissionTimerHandler(TransmissionTimerHandler &&) = default;
            TransmissionTimerHandler &operator=(const TransmissionTimerHandler &) = default;
            TransmissionTimerHandler &operator=(TransmissionTimerHandler &&) = default;

            void handlerFunction(const int32_t timerId) override;

        private:
            CocoTransmission& mTransmission;
        };

        friend class TransmissionTimerHandler;

        CollectionCondition &mCollectionCondition;
        bool mNetworkOutOfRange;
        State mState;
        uint8_t mRetryCounter;
        int32_t mCallId;
        uint32_t mDuration;
        DiagTrigger::DiagTriggerType mTriggerType;
        GRPC_IF_TYPE mRequestType;
        TransmissionTimerHandler mTransmissionTimerHandler;
        Timer mTransmissionTimer;
        std::shared_ptr<google::protobuf::Message> mReqPayload;
    };

    friend class TimerHandler;
    
    inline static std::string VEHICLE_RELATED_ERROR(const std::string MSG) { return std::string("100; ") + MSG + ".";};
    inline static std::string NO_COLLECTION_CONDITION() noexcept {return std::string("200; collection_condition is not set.");};
    inline static std::string TOO_MANY_COLLECTION_CONDITIOS(const std::string FIELD_NAME) {return std::string("201; ") + FIELD_NAME + " has too many collection conditions.";};
    inline static std::string INCORRECT_UPDATE_TYPE() noexcept {return std::string("202; Update type is incorrect.");};
    inline static std::string NO_REQUIRED_SETTING_VALUES(const std::string FIELD_NAME) {return std::string("210; ") + FIELD_NAME + " is not set.";};
    inline static std::string TOO_MANY_REPEATED_SETTING_VALUES(const std::string FIELD_NAME) {return std::string("211; ") + FIELD_NAME + " has too many setting values.";};
    inline static std::string OUT_OF_RANGE(const std::string FIELD_NAME) {return std::string("220; ") + FIELD_NAME + " is out of range.";};
    inline static std::string INVALID_SETTING_VALUES(const std::string FIELD_NAME) {return std::string("221; ") + FIELD_NAME + " is invalid.";};
    inline static std::string RDG_INVALID_SCHEDULE_TYPE() noexcept {return std::string("400; Schedule type is invalid.");};

    void printData(const std::string data) const;

    UpdateResultCode UpdateCollectionConditionDirectCommand(const CollectionConditionDirectCommand &inputData);

    UpdateResultCode handleCenterRequestRobSsrs(ErrorInformationList &errorInfoList, const CenterRequestRobSsrList &inputDataList);
    UpdateResultCode handleCenterRequestDirectCommands(ErrorInformationList &errorInfoList, const CenterRequestDirectCommandList &inputDataList);
    UpdateResultCode handleCollectionConditionDirectCommands(ErrorInformationList &errorInfoList, const CollectionConditionDirectCommandList &inputDataList);

    UpdateResultCode handleCollectionConditionDiagCommonData(ErrorInformationList &errorInfoList, const CollectionConditionDiagCommon &inputData);
    UpdateResultCode handleCollectionConditionRobRobSsrDidEvent(ErrorInformationList &errorInfoList, const CollectionConditionRobRobSsrDidEvent &inputData);
    UpdateResultCode handleCollectionConditionEcuInformation(ErrorInformationList &errorInfoList, const CollectionConditionEcuInformation &inputData);
    UpdateResultCode handleCollectionConditionWarningInformation(ErrorInformationList &errorInfoList, const CollectionConditionWarningInformation &inputData);


    void handleGrpcClientErrorEvent(const android::sp<GrpcResData> resData);
    void handleDeleteCollectionConditionDiagCommon(const Uint64 id = 0U);
    void handleDeleteCollectionConditionRobRobSsrDidEvent(const Uint64 id = 0U);
    void handleDeleteCollectionConditionEcuInformation(const Uint64 id = 0U);
    void handleDeleteCollectionConditionWarningInformation(const Uint64 id = 0U);
    void handleDeleteCollectionConditionDirectCommands(void);
    void handleRdgActiveFlagOff(void);
    static bool takeFeatureStatus();

    std::vector<android::sp<CocoTransmission>>::iterator findCocoTransmission(const int32_t callId, const CocoTransmission::State aState, const GRPC_IF_TYPE aType);
    std::vector<android::sp<CocoTransmission>>::iterator findCocoTransmission(const android::sp<CocoTransmission>& trans);
    void addCocoTransmission(const android::sp<CocoTransmission>& trans);
    void removeCocoTransmission(const int32_t callId, const CocoTransmission::State aState, const GRPC_IF_TYPE aType);
    void addCenterRequestJob(const std::shared_ptr<CenterRequestJob>& aJob);
    bool isCenterRequestJobExits(const uint64_t jobId);
    void onIgOnTimeout(void);
    std::shared_ptr<GetCollectionConditionRequest> mGetCollectionConditionRequest;
    std::shared_ptr<GetCollectionConditionRequest> mGetCollectionConditionRequestBuff;
    std::shared_ptr<NotifyCollectionConditionUpdateResultRequest> mNotifyCollectionConditionUpdateResultRequest;
    CollectionConditionDirectCommandList mCollectionConditionDirectCommandList;
    android::sp<MainHandler> mCollectionConditionHandler;
    TimerHandler mCollectionConditionTimerHandler;
    Timer mCollectionConditionTimer;
    DiagTrigger::DiagTriggerType mTriggerType;
    std::vector<uint64_t> mDeletedCollectionConditionIds;
    std::queue<std::vector<uint64_t>> mNewCenterRequests;
    std::vector<uint64_t> mNewCenterRequestList;
    std::vector<std::pair<uint64_t, uint8_t>> mUpdatedCollectionConditionIds;
    std::vector<android::sp<CocoTransmission>> mCocoTransmissionList;
    std::unordered_map<uint64_t, std::shared_ptr<CenterRequestJob>> mCenterRequestJobList;
    mutable android::Mutex mLock;
    bool isIgOnStateMaintainedCheck;
};
}
#endif // REMOTE_DIAG_COLLECTION_CONDITIO_H
