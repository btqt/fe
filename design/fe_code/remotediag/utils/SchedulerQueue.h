#ifndef RDG_SCHED_QUEUE_H
#define RDG_SCHED_QUEUE_H

#include <utils/RefBase.h>
#include <set>
#include <services/TimeManagerService/TimeManager.h>
#include "SchedulerType.h"
#include "SchedulerTime.h"
#include "SchedulerManager.h"
#include "SchedulerQueueCompare.h"
#include "utils/Logger.h"
#include "SchedulerHdl.h"
#include <services/AlarmManagerService/AlarmManager.h>
#include "services/PowerManagerAdapter.h"

namespace rdgapp {

// class SchedulerManager;
// class SchedulerHdl;

class SchedulerQueue : public android::RefBase {
public:
    using SchedQueue_Sto = std::multiset<android::sp<SchedulerTime>, SchedulerQueueCompare>;
    using SchedQueue_Sto_It = std::multiset<android::sp<SchedulerTime>, SchedulerQueueCompare>::iterator;
    SchedulerQueue(const android::sp<SchedulerHdl> mScheduleHdl, const android::sp<SchedulerManager> mScheduleMngr
                ,const android::sp<SchedulerTime> mScheduleTime, const uint64_t schedIndex);
    virtual ~SchedulerQueue();
    SchedulerQueue(const SchedulerQueue& other) = default;
    SchedulerQueue& operator=(const SchedulerQueue& other) = default;
    SchedulerQueue(SchedulerQueue&& other) noexcept = default;
    SchedulerQueue& operator=(SchedulerQueue&& other) noexcept = default;
    bool insert(const android::sp<SchedulerTime> SchedTime_data);
    void clearAll();
    bool isSameIGON();
    bool isSameIGOFF();
    uint64_t getSchedIndex() const noexcept;
    void readyToStart();
    void startTime();
    void startTime(const int64_t duration);
    void stopTime();
    void updateTime();
    Rdg_Sched_Type::SchedFuncType getFirstFuncType();
    Rdg_Sched_Type::SchedType getFirstSchedType();
    void executeQueue(const bool processNewOnly);
    void saveLastOpComplTime(const int64_t completeTime, const bool isDiagComplete);
    void setLastOpComplTime(const int64_t completeTime) noexcept;
    void setDiagCompleted(const uint8_t data) noexcept;
    int64_t getLastOpComlTime() const noexcept;
    uint8_t getIsExecuted() const noexcept;
    void setIsExecuted(const uint8_t data) noexcept;
    int64_t getIntervalDuration() const noexcept;
    uint32_t getPrioSchedQue() const noexcept;
    bool getIsDiagCompleted() const noexcept;
private:
    class AlarmExpireListener : public AlarmListener {
    public:
        AlarmExpireListener(SchedulerQueue &mQueue) noexcept : mSchedulerQueue(mQueue) {}
        AlarmExpireListener(const AlarmExpireListener& other) = default;
        AlarmExpireListener& operator=(const AlarmExpireListener& other) = default;
        AlarmExpireListener(AlarmExpireListener&& other) noexcept = default;
        AlarmExpireListener& operator=(AlarmExpireListener&& other) noexcept = default;
        virtual ~AlarmExpireListener() = default;
        void onAlarmExpired(const int32_t AlarmID) override;
    private:
        SchedulerQueue &mSchedulerQueue;
    };
private:
    void init(const android::sp<SchedulerTime> SchedTime_data);
    void restartAlarm(const int64_t mDuration);
    // static int64_t getLastOpComplTime();

    SchedQueue_Sto mSchedQueue_Sto;
    SchedQueue_Sto_It mSchedQueue_Sto_It;
    android::sp<SchedulerHdl> mpSchedHdl;
    android::sp<SchedulerManager> mpSchedulerMngr;
    android::sp<SchedulerTime> mpSchedulerTime;
    android::sp<AlarmExpireListener> mListener;
    android::sp<AlarmManager> mAlarmservice;
    uint64_t mSchedIndex;
    int32_t mTimerID;
    // std::string mpPropName;
    int64_t mLastOpComlTime;
    uint8_t mIsExecuted;
    uint32_t mPioShedQue;
    bool mIsDiagCompleted;
};
}
#endif /* RDG_SCHED_QUEUE_H */
