#ifndef REMOTE_ROB_OCCURRENCE_H
#define REMOTE_ROB_OCCURRENCE_H

#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <list>
#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "RemoteDelegate.h"
#include "Remotediag.h"
#include "DiagTrigger.h"
#include "PriorityControl.h"
#include "services/HttpManagerAdapter.h"
#include "services/LocationManagerAdapter.h"
#include "services/OnboardclientManagerAdapter.h"
#include "services/DiagManagerAdapter.h"
#include "services/OnboardclientManagerAdapter.h"
#include "diagprocess/EcuInformation/EcuInformationDataType.h"
#include "diagprocess/EcuInformation/RemoteEcuInformation.h"
#include "diagprocess/RoBOccurrence/OccurrentRobNotification.h"
#include <services/TimeManagerService/TimeManager.h>
#include "utils/CollectionCondition.h"
#include "utils/Database.h"

#define OCCURRENT_ROB_DID 0xA006U
#define SID22_PAYLOAD_SIZE_AT_LEAST 8U
#define SID22_DID_BYTE_MASK 1U
#define SID22_NUMBER_OF_DID_EVENT SID22_DID_BYTE_MASK + 2U
#define SID22_USER_DEFINED_DTC SID22_NUMBER_OF_DID_EVENT + 1U
#define SID22_MEMORY_SELECTION SID22_USER_DEFINED_DTC + 3U

namespace rdgapp {

class Remotediag;
class Database;
class OccurrentRobNotification;

class RoBOccurrence : public RemoteDelegate, public android::RefBase
{
public:
    static constexpr uint8_t APP_ID{RDG_APPID::ROB_OCCURRENCE};
    constexpr static char_t PREVIOUS_TRIP_DB_STRUCT[]{"(id INTEGER PRIMARY KEY\
                                                    , collection_condition_id BLOB\
                                                    , notification_id BLOB\
                                                    , location_latitude INTEGER\
                                                    , location_longitude INTEGER\
                                                    , diag_accqui_time INTEGER\
                                                    , priority INTEGER\
                                                    , target_collection_data BLOB\
                                                )"};
    constexpr static char_t ROBSSR_CRC_DB_STRUCT[]{"(id INTEGER PRIMARY KEY\
                                                , trans_id BLOB\
                                                , occurred_rob_ssr_crc INTEGER\
                                                , time_series_rob_ssr_crc INTEGER\
                                                )"};
    constexpr static char_t ROBSSR_CRC_DB_NAME[]{"robssr_crc"};
    constexpr static char_t PREVIOUS_TRIP_DB_NAME[]{"occurrent_rob_notification"};

    RoBOccurrence(const Remotediag &app, android::sp<sl::SLLooper> &privateLooper);
    ~RoBOccurrence() final = default;
    RoBOccurrence(const RoBOccurrence &) = default;
    RoBOccurrence(RoBOccurrence &&) = default;
    RoBOccurrence &operator=(const RoBOccurrence &) = default;
    RoBOccurrence &operator=(RoBOccurrence &&) = default;
    static RoBOccurrence *getInstance(void);
    bool checkPrecondition(void) noexcept final {return false;}
    void notifyBootComplete(void) const noexcept final {}
    const uint32_t getOccurredCRC(const uint64_t transId);
    const uint32_t getTimeSeriesCRC(const uint64_t transId);
    void onReceiveIG(const bool status) const final;
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) noexcept override;
    void onOccurrenceRoBReceived(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse);
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0) noexcept final {}
    void onCenterCommandForward(const android::sp<CenterReqData> &pCenterReqData) noexcept final {}
    void onRdgStop(const bool isStop) const noexcept override;
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final {return std::map<uint64_t, android::sp<UdsMessage>>();}
    uint8_t getAppId(void) const noexcept final;

    void onRobSsrAcquisitionCompleted(const bool result, const uint64_t notificationId, const uint64_t transId, const uint32_t occurredrRobSsrCRC, const uint32_t timeSeriesRobSsrCRC);
    void onDirectCommandCompleted(const bool result, const uint64_t notificationId);
    void onServiceFlagChange();
    bool notifyTrigger(const DiagTrigger::DiagTriggerState &pState, const int32_t &pTriggerId, const bool dueToIgOff) const noexcept {
        NOTUSED(pState);
        NOTUSED(pTriggerId);
        NOTUSED(dueToIgOff);
        return true;
    }
    const android::sp<OccurrentRobNotification> getNotification(const uint64_t notificationId);

private:
    int32_t saveOccurrentRob(void);
    int32_t saveCRC(void);
    int32_t restoreOccurrentRob(void);
    void changeIGStatus(const bool status);
    error_t removeNotification(const uint64_t notificationId);
    void addNotification(const android::sp<OccurrentRobNotification>& aNotification);
    void handleStopRDG(const int32_t isStop);

private:
    class MainHandler : public sl::Handler
    {
    public:
        static constexpr int32_t CMD_ROB_OCCURRENCE_DETECTING_ROB_EVENT{5001};
        static constexpr int32_t CMD_ROB_OCCURRENCE_IG_OFF{5002};
        static constexpr int32_t CMD_ROB_OCCURRENCE_PREVIOUS_TRIP{5003};
        static constexpr int32_t CMD_ROB_OCCURRENCE_CHANGE_IG_STATUS{5004};
        static constexpr int32_t CMD_STOP_RDG{5005};

        explicit MainHandler(android::sp<sl::SLLooper> &privateLooper, RoBOccurrence &RoBOccurrence) noexcept;
        ~MainHandler() override = default;
        MainHandler(const MainHandler &) = default;
        MainHandler(MainHandler &&) = default;
        MainHandler &operator=(const MainHandler &) = default;
        MainHandler &operator=(MainHandler &&) = default;
        void handleMessage(const android::sp<sl::Message> &handlemsg) override;

    private:
        RoBOccurrence &mRoBOccurrence;
    };


    class TimerHandler : public TimerTimeoutHandler {
    public:
        static constexpr uint32_t ID_IG_ON_STATE_MAINTAINED_CHECK_TIMEOUT {60U}; // 1 minute

        static constexpr int32_t IG_ON_STATE_MAINTAINED_CHECK_ID  {3000};

        explicit TimerHandler (RoBOccurrence& aRoBOccurrence) noexcept 
            : TimerTimeoutHandler(), mRoBOccurrence(aRoBOccurrence) {}
        ~TimerHandler() override = default;
        TimerHandler(const TimerHandler&) = default;
        TimerHandler(TimerHandler&&) = default;
        TimerHandler& operator=(const TimerHandler&) = default;
        TimerHandler& operator=(TimerHandler&&) = default;

        void handlerFunction (const int32_t timerId) override {
            switch(timerId) 
            {
                case IG_ON_STATE_MAINTAINED_CHECK_ID:
                    (void)mRoBOccurrence.mRoBOccurrenceHandler->obtainMessage(MainHandler::CMD_ROB_OCCURRENCE_PREVIOUS_TRIP)->sendToTarget();
                    break;
                default:
                    break;
            }
        }
    private:
        RoBOccurrence& mRoBOccurrence;
    };

    void handleOccurrentRobDetection(const uint32_t targetAddress, const android::sp<UdsMessage> udsResponse);
    void handleIgOffEvent();
    void handleOccurrentRobPreviousTrip();

private:

    android::sp<sl::Handler> mRoBOccurrenceHandler;
    static RoBOccurrence *mRoBOccurrence;
    const Remotediag &mApp;
    DiagTrigger::DiagTriggerType mTriggerType;
    std::unordered_map<uint64_t, android::sp<OccurrentRobNotification>> mNotificationList;
    std::unordered_map<uint64_t, std::shared_ptr<CrcInformation>> mCrcList;
    std::unique_ptr<Database> mOccurrentRobInfoDB;
    std::unique_ptr<Database> mRobSsrCrcDB;
    TimerHandler mTimerHandler;
    Timer mIgOnCheckTimer;
    mutable android::Mutex mLock;
    bool mIsAbortOccurrenceEvent;
};
}
#endif // REMOTE_ROB_OCCURRENCE_H
