#ifndef SCHED_MGR_H
#define SCHED_MGR_H

#include <map>
#include <vector>
#include <atomic>
#include <utils/RefBase.h>
#include <utils/Timer.h>

#include <services/TimeManagerService/TimeManager.h>
#include "services/HttpManagerAdapter.h"

#include "SchedulerType.h"
#include "SchedulerTime.h"
// #include "SchedulerQueue.h"
#include "utils/Logger.h"
#include "diagprocess/CenterReqData.h"
#include "Remotediag.h"
#include "DiagTrigger.h"

namespace rdgapp {

class Remotediag;
class SchedulerQueue;
class SchedulerHdl;

using ScheduleMap = std::map<uint64_t, android::sp<SchedulerQueue>>;
using ScheduleMapIt = std::map<uint64_t, android::sp<SchedulerQueue>>::iterator;

class SchedulerManager : public android::RefBase
{
public:
    class SchedCompleteInfo : public android::RefBase
    {
    public:
        SchedCompleteInfo(const uint64_t schedIndex, const int64_t completeTime, const bool isCompleted) noexcept
            : mSchedIndex(schedIndex), mCompleteTime(completeTime), mIsCompleted(isCompleted) {}
        ~SchedCompleteInfo() override = default;
        SchedCompleteInfo(const SchedCompleteInfo&) = default;
        SchedCompleteInfo(SchedCompleteInfo&&) = default;
        SchedCompleteInfo& operator=(const SchedCompleteInfo&) = default;
        SchedCompleteInfo& operator=(SchedCompleteInfo&&) = default;
        uint64_t getSchedIndex() const noexcept { return mSchedIndex; }
        int64_t getCompleteTime() const noexcept { return mCompleteTime; }
        bool getIsCompleted() const noexcept { return mIsCompleted; }
        // void setCompleteTime(int64_t time) { mCompleteTime = time; }
        // void setIsCompleted(bool completed) { mIsCompleted = completed; }
    private:
        uint64_t mSchedIndex;
        int64_t mCompleteTime;
        bool mIsCompleted;
    };
    SchedulerManager(const Remotediag &app, android::sp<sl::SLLooper> &privateLooper);
    ~SchedulerManager() override = default;

    void onReceiveIG(const bool status);
    void onRdgStop(const bool isStop) const;
    void insertToMap(const android::sp<SchedulerTime> SchedTime, uint64_t schedIndex);
    void deleteSchedInMap(const uint64_t schedIndex);
    void notifySchedComplete(const uint64_t schedIndex, const int64_t completeTime, const bool isCompleted);
    // ScheduleMap getScheduleMap();
    void clearMap();

    void applyChange();
    void executeSchedIGONRoutine();
    void executeFunction(const android::sp<SchedulerTime> executeSchedule);

    void setIgStatus(const bool IGStatus);
    // void setIgStatus_2(bool IGStatus);
    // bool getIgStatus();
    void setIgOnRoutineExpired(const bool data) noexcept;
    bool getIgOnRoutineExpired() const noexcept;

    // bool getIsReadFromMem();
    // void setIsReadFromMem(bool ReadFromMem);

    // void executeElapsed();
    // void onAlarmManagerDied();
    void applyNewCenterReqData(const android::sp<CenterReqData> aCenterReqData);
    void applyNewSchedData(const bool isOnlyLoadSched = false, const bool isBooting = false);
    // void discardIgOffProcess();
    uint32_t checkSchedMapSize() const noexcept;
    // android::sp<SchedulerQueue> getSameDuration(uint32_t duration);
    // android::sp<SchedulerQueue> getSameTypeIGON();
    android::sp<SchedulerQueue> getSameSchedType(const Rdg_Sched_Type::SchedType pSchedType);
    // int64_t getIgOnTimestamp() const {return mIgOnTimestamp;};
    void saveComplTimeToFile(const uint64_t schedIndex, const int64_t timeData, const bool isCompleted);
    void saveComplTimeToFile();
    void loadComplTimeFromFile();
    void handleSchedComplete(const android::sp<SchedCompleteInfo> schedData);
    void handleReceiveIG(const bool status);
    void handleNewSchedData(const bool isOnlyLoadSched, const bool isBooting);
    void onIgOnRoutineExpired();
    void handleIgOnRoutineExpired();
    void handleNewCenterReqData(const android::sp<CenterReqData> aCenterReqData);
private:
    static constexpr uint32_t IG_ON_ROUTINE_TIMER{70U};    /* 70 sec */
    static constexpr uint32_t IG_OFF_PROCESS_EXPIRED{15U}; /*RDG30-R-1086: 15s*/

    class TimerHandler : public TimerTimeoutHandler
    {
    public:
        static constexpr int32_t ID_IG_ON_TRIGGER_ROUTINE{3000};
        // static constexpr int32_t ID_TRANSMISSION_TIMEOUT{3001};
        static constexpr int32_t ID_IG_OFF_PROCESS_EXPIRED{3002};
        explicit TimerHandler(SchedulerManager &Sched) noexcept : TimerTimeoutHandler(), mSchedManager(Sched) {}
        ~TimerHandler() override = default;
        TimerHandler(const TimerHandler &) = default;
        TimerHandler(TimerHandler &&) = default;
        TimerHandler &operator=(const TimerHandler &) = default;
        TimerHandler &operator=(TimerHandler &&) = default;
        void handlerFunction(const int32_t timerId) override;

    private:
        SchedulerManager &mSchedManager;
    };
    const Remotediag &mApp;
    android::sp<SchedulerHdl> mpSchedulerHdl;
    ScheduleMap mSchedulerMap;
    volatile bool mpIGStatus;
    std::shared_ptr<TimerHandler> mTimerHandler;
    android::sp<Timer> mTickTimer;
    std::vector<android::sp<Timer>> mTimers;
    std::atomic<bool> isIgOnRoutineExpired;
    // int64_t mIgOnTimestamp;
    std::map<uint64_t, std::pair<int64_t,uint8_t>> mLastCompTime;
};
}
#endif /* SCHED_MGR_H */
