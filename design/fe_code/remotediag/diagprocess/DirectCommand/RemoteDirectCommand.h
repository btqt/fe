#ifndef REMOTE_DIRECTCOMMAND_H
#define REMOTE_DIRECTCOMMAND_H

#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/Mutex.h>
#include <list>
#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "RemoteDelegate.h"
#include "Remotediag.h"
#include "DiagTrigger.h"
#include "services/HttpManagerAdapter.h"
#include "services/LocationManagerAdapter.h"
#include "services/OnboardclientManagerAdapter.h"
#include "services/DiagManagerAdapter.h"
// #include "services/LocationManagerAdapter.h"
#include "services/OnboardclientManagerAdapter.h"
#include "diagprocess/EcuInformation/EcuInformationDataType.h"
#include "diagprocess/EcuInformation/RemoteEcuInformation.h"
#include "diagprocess/RoBOccurrence/RoBOccurrence.h"
#include <services/TimeManagerService/TimeManager.h>
#include "PriorityControl.h"
#include "utils/CollectionCondition.h"
#include "DiagTrigger.h"

namespace rdgapp
{

    class Remotediag;
    class OccurrentRobNotification;

    class RemoteDirectCommand : public RemoteDelegate, public android::RefBase
    {
    private:
        using CenterCommunicationProtocol = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol;
        using CenterCommunicationType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType;

    public:
        static constexpr int32_t WHAT_CHANGED_REPAIR_SATUS{1};
        RemoteDirectCommand(const Remotediag &app, android::sp<sl::SLLooper> &privateLooper);
        ~RemoteDirectCommand() override;
        RemoteDirectCommand(const RemoteDirectCommand &) = default;
        RemoteDirectCommand(RemoteDirectCommand &&) = default;
        RemoteDirectCommand &operator=(const RemoteDirectCommand &) = default;
        RemoteDirectCommand &operator=(RemoteDirectCommand &&) = default;
        static RemoteDirectCommand *getInstance(void);
        bool checkPrecondition(void) noexcept override;
        bool checkPrecondition(const DiagTrigger::DiagTriggerType diagTriggerType, const uint64_t collectionId);
        void notifyBootComplete(void) const noexcept override;
        void onReceiveIG(const bool status) const noexcept override;
        void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) noexcept override;
        void onChangedRemoteInfo(const int32_t what, const int32_t info = 0) noexcept override;
        void onCenterCommandForward(const android::sp<CenterReqData> &pCenterReqData) noexcept override;
        std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final { return std::map<uint64_t, android::sp<UdsMessage>>(); }
        void onOccurrentRobDetected(const android::sp<OccurrentRobNotification> notification);
        uint8_t getAppId(void) const noexcept override;
        void triggerDirectCommand(const DiagTrigger::DiagTriggerType type, const uint32_t prio, const uint64_t colId, const int64_t time);
        // static void forSLDDTesting(const int32_t what, const int32_t arg1, const int32_t arg2);
        void onTransmissionTimeout(void);
        void startTimer(const int32_t timerId);
        void stopTimer(const int32_t timerId);
        bool notifyTrigger(const DiagTrigger::DiagTriggerState &pState, const int32_t &pTriggerId, const bool dueToIgOff);
        void finishCurrentTransmission();
        void abortDirectCommand(const vccomif::rdg::v1::interfaces::ResponseCode resCode);
        void suspendDirectCommand();
        uint8_t getOperation() const;
        void testingMaxFileSize(const uint16_t fileSize) noexcept;

    private:
        // static constexpr uint32_t IGON_TRIGGER_DURATION{60U * 5U};
        static constexpr uint8_t DIRECTCOMMAND_RUNNING{1U};
        static constexpr uint8_t DIRECTCOMMAND_STOP{0U};
        static constexpr uint32_t MAX_SPLIT_FILE{3U};
        uint32_t DIRECTCOMMAND_TOTAL_UPLOAD_DATA_SIZE_MAX; // bytes = 10MB
        uint32_t DIRECTCOMMAND_UPLOAD_DATA_SIZE_MAX;       // bytes = 4MB
        uint32_t DIRECTCOMMAND_LAST_UPLOAD_DATA_SIZE_MAX;  // bytes = 2MB
        void printData(const std::string data) const;
        void init(void) const;
        void startUp(void);
        void getDirectCommandRequest(const DiagTrigger::DiagTriggerType triggerType, const uint64_t collectionId, google::protobuf::RepeatedPtrField<vccomif::rdg::v1::interfaces::DirectCommand> &commandList, bool &srvc_ac_flag);
        bool validateDirectCommand(const vccomif::rdg::v1::interfaces::DirectCommand bDirectCommandReq, CommonDefine::EcuInformation &ecuInformation);
        void initTimer(void);
        void handleTrigger(const DiagTrigger::DiagTriggerState &pState, const int32_t &pTriggerId, const bool dueToIgOff);
        void sendDirectCommand(void);

        void finishDirectCommand(void);
        void makeUploadData(void);
        void changedRemoteStatus(const uint8_t what, const uint32_t info);
        void makeErrorUploadData(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colId, const bool srvc_ac_flag);
        // bool convertToUdsReq(std::string commandMessage, sp<::Buffer> &udsReq);
        void makeHeaderForUploading(const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequest> &directCommandUploadData, const bool isIncreaseCount = false);
        void notifyTriggerDone(const int32_t pTriggerId, const bool isNotify, const bool isCompletedRob, const bool isRemoveTrigger);
        uint8_t getAppStatus() const noexcept;
        void changeAppStatus(const uint8_t appStatus) noexcept;

    public:
        class MainHandler : public sl::Handler
        {
        public:
            static constexpr int32_t CMD_DIRECTCOMMAND_INIT{3000};
            static constexpr int32_t CMD_DIRECTCOMMAND_START_UP{3001};
            static constexpr int32_t CMD_DIRECTCOMMAND_UPLOAD_DATA{3002};
            static constexpr int32_t CMD_DIRECTCOMMAND_GET_OBC_RESOURCE{3003};
            static constexpr int32_t CMD_DIRECTCOMMAND_EVENT_CONNECT{3004};
            static constexpr int32_t CMD_DIRECTCOMMAND_SEND_UDS{3005};
            static constexpr int32_t CMD_DIRECTCOMMAND_EVENT_UDS_RES{3006};
            static constexpr int32_t CMD_DIRECTCOMMAND_EVENT_DISCONNECT{3007};
            static constexpr int32_t CMD_CHANGE_IG_STATUS{3008};
            static constexpr int32_t CMD_DIRECTCOMMAND_CENTER_REQUEST{3009};
            static constexpr int32_t CMD_DIRECTCOMMAND_REQUEST_TO_PRIORITY_CONTROL{3010};
            static constexpr int32_t CMD_DIRECTCOMMAND_SEND_REQUEST{3011};
            static constexpr int32_t CMD_DIRECTCOMMAND_RECEIVE_UNDER_REPAIR_FLAG_CHANGE{3012};
            static constexpr int32_t CMD_DIRECTCOMMAND_COLLECTION_CONDITION{3013};
            static constexpr int32_t CMD_DIRECTCOMMAND_FINISH_TRANSMISSION{3014};
            static constexpr int32_t CMD_DIRECTCOMMAND_MAX{3999};
            explicit MainHandler(android::sp<sl::SLLooper> &privateLooper, RemoteDirectCommand &obj) noexcept;
            ~MainHandler() override = default;
            MainHandler(const MainHandler &) = default;
            MainHandler(MainHandler &&) = default;
            MainHandler &operator=(const MainHandler &) = default;
            MainHandler &operator=(MainHandler &&) = default;
            void handleMessage(const android::sp<sl::Message> &handlemsg) override;

        private:
            RemoteDirectCommand &mDirectCommand;
        };

    public:
        class DirectCommandTimerHandler : public TimerTimeoutHandler
        {
        public:
            static constexpr int32_t TIMER_ID_DIRECTCOMMAND_IG_ON_TRIGGER{4000};
            static constexpr int32_t TIMER_ID_DIRECTCOMMAND_TRANSMISSION_TIMEOUT{4001};
            static constexpr int32_t TIMER_ID_DIRECTCOMMAND_IG_OFF_TRIGGER{4002};
            explicit DirectCommandTimerHandler(const RemoteDirectCommand &obj) noexcept : TimerTimeoutHandler(), mDirectCommand(obj){};
            ~DirectCommandTimerHandler(void) override = default;
            DirectCommandTimerHandler(const DirectCommandTimerHandler &) = default;
            DirectCommandTimerHandler(DirectCommandTimerHandler &&) = default;
            DirectCommandTimerHandler &operator=(const DirectCommandTimerHandler &) = default;
            DirectCommandTimerHandler &operator=(DirectCommandTimerHandler &&) = default;

            void handlerFunction(const int32_t timerId) override;

        private:
            const RemoteDirectCommand &mDirectCommand;
        };

    private:
        class DirectCommandTransmission : public RefBase
        {
        public:
            enum class State : uint8_t
            {
                DIRECTCOMMAND_TRANS_INIT = 0U,
                DIRECTCOMMAND_TRANS_CONNECT,
                DIRECTCOMMAND_TRANS_SEND_UDS,
                DIRECTCOMMAND_TRANS_DISCONNECT,
                DIRECTCOMMAND_TRANS_DONE
            };
            DirectCommandTransmission(const RemoteDirectCommand &obj, const CommonDefine::EcuInformation ecuInformation, const android::sp<::Buffer> udsReq);
            ~DirectCommandTransmission() override = default;

            void connect(void);
            void disconnect(void);
            void sendUds(void);
            uint16_t connectId() const noexcept { return mConnectId; };
            State getState() const noexcept { return mState; };
            bool getDefaultSession() const noexcept { return bDefaultSession; };
            void setDefaultSession(const bool isDefault) noexcept { bDefaultSession = isDefault; };
            void setState(const State st) noexcept { mState = st; };
            void setUdsReq(const android::sp<Buffer> udsReq);
            CommonDefine::EcuInformation ecuInformation() const noexcept { return mEcuInformation; };
            std::deque<android::sp<::Buffer>> getUdsReqList() noexcept { return udsReqList; };
            void pushUdsReqList(const sp<::Buffer> uds) { udsReqList.push_back(uds); };
            android::sp<Buffer> getUdsReq() const noexcept;

        private:
            android::sp<::Buffer> mUdsReq;
            android::sp<::Buffer> mSentUds;
            // static constexpr uint32_t TRANSMISSION_TIME_OUT_DURATION{60U * 5U};
            const RemoteDirectCommand &mDirectCommand;
            uint16_t mConnectId;
            State mState;
            bool bDefaultSession;
            CommonDefine::EcuInformation mEcuInformation;
            std::deque<android::sp<Buffer>> udsReqList;
        };

    private:
        void createDiagnosticsData(vccomif::rdg::v1::interfaces::DiagnosticsMessage &diagMessage, const android::sp<DirectCommandTransmission> directCommandReq, const android::sp<OBCResponseEventInfo>) const;

    private:
        android::sp<sl::Handler> mRemoteDirectCommandHandler;
        static RemoteDirectCommand *mRemoteDirectCommand;
        android::sp<CommonDefine::RDGLocationData> mLocationData;
        uint8_t isDirectAppRunning;
        int64_t mDirectCommandAcquisitionTime;
        uint32_t odo_value;
        uint32_t odo_unit;
        std::unique_ptr<DirectCommandTimerHandler> mDirectCommandTimerHandlerTrans;
        std::unique_ptr<DirectCommandTimerHandler> mDirectCommandTimerHandlerAbort;
        android::sp<Timer> mTimeOutTrans;
        android::sp<Timer> mTimeOutAbort;
        std::shared_ptr<::vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequest> mDirectCommandUploadData;
        // std::list<::vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequest> l_DirectCommandUploadData;
        std::shared_ptr<::vccomif::rdg::v1::interfaces::UploadErrorDataRequest> mDirectCommandUploadErrorData;
        std::list<CommonDefine::EcuInformation> l_EcuInformation;
        std::unordered_map<uint64_t, android::sp<DirectCommandTransmission>> l_DirectCommandTrans;

        android::sp<DirectCommandTransmission> mCurrDirectCommandTrans;
        std::deque<android::sp<DirectCommandTransmission>> mDirectCommandTransQueue;
        std::vector<UploadDirectCommandDataRequest> dataUploadDirectCommandList;

        google::protobuf::RepeatedPtrField<vccomif::rdg::v1::interfaces::DirectCommand> l_DirectcommandRequest;
        uint64_t mCurrentTransmissionId;
        std::unordered_map<uint32_t, android::sp<DiagTrigger>> mSaveReq;
        // std::unordered_map<uint64_t, android::sp<UdsMessage>> mDirectCommandResponse;
        std::vector<vccomif::rdg::v1::interfaces::DiagnosticsMessage> mDirectCommandResponse;
        std::vector<UploadDirectCommandDataRequest> l_UploadDirectCommandDataRequest;
        android::sp<OccurrentRobNotification> mRobNotificationReq;
        std::vector<uint32_t> vErrorIdx;
        const Remotediag &mApp;
        int32_t mTriggerId;
        uint64_t mColId;
        uint32_t mPriority;
        uint32_t mTotalSize;
        bool bUploadDirectCommandData;
        bool bUploadErrorData;
        bool bAbortDirectCommand;
        bool bsrvc_ac_flag;
        DiagTrigger::DiagTriggerType mTriggerType;
        bool mSuspended;
        mutable Mutex mMutexAppState;
    };
}
#endif // REMOTE_DIRECTCOMMAND_H
