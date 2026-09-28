#include "SchedulerQueue.h"
#include "SchedulerHdl.h"

namespace rdgapp {

SchedulerQueue::SchedulerQueue(const android::sp<SchedulerHdl> mScheduleHdl, const android::sp<SchedulerManager> mScheduleMngr
            , const android::sp<SchedulerTime> mScheduleTime, const uint64_t schedIndex) 
            : android::RefBase()
            , mpSchedHdl(mScheduleHdl)
            , mpSchedulerMngr(mScheduleMngr)
            , mpSchedulerTime(mScheduleTime){
    mListener = new AlarmExpireListener(*this);
    mAlarmservice = new AlarmManager(mListener);
    mSchedIndex = schedIndex;
    mPioShedQue = mScheduleTime->getPrio();
    // if(schedIndex <= static_cast<uint32_t>(INT32_MAX)){
    //     mTimerID = static_cast<int32_t>(schedIndex);
    // }
    // else{
        // const std::hash<uint64_t> hasher;
        uint64_t tmp_TimerID{0ULL};
        tmp_TimerID = (schedIndex >> 32U) ^ (schedIndex & 0xFFFFFFFF);
        if(tmp_TimerID > static_cast<uint64_t>(INT32_MAX)) {
            LOG_D("out of range");
        }
        uint32_t tmp{0U};
        tmp = static_cast<uint32_t>(tmp_TimerID) % static_cast<uint32_t>(INT32_MAX);
        if(tmp > static_cast<uint32_t>(INT32_MAX)) {
            LOG_D("out of range");
        }
        mTimerID = static_cast<int32_t>(tmp);
    // }
    LOG_D("Check sched index: %llu Timer ID: %d", schedIndex, mTimerID);
    mLastOpComlTime = 0;
    mIsExecuted = 0U;
    init(mScheduleTime);
}
SchedulerQueue::~SchedulerQueue() {
    mListener = nullptr;
}

void SchedulerQueue::AlarmExpireListener::onAlarmExpired(const int32_t AlarmID) {
    LOG_I("Timeout - Timer ID: %d - Sched index: %llu", AlarmID, mSchedulerQueue.getSchedIndex());
    if(mSchedulerQueue.getFirstFuncType() != Rdg_Sched_Type::SchedFuncType::FUNC_TYPE_INVALID) {
        /* If function is IG ON routine => stop timer*/
        /* IF function is IG OFF rountine => stop timer*/
        /* IF function is other than above -> updateTime and execute function*/
        if(mSchedulerQueue.getFirstSchedType() == Rdg_Sched_Type::SchedType::ST_PERIOD_TRIGGER_ROUTINE) {
            if(PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS_ON) {
                LOG_I("ST_PERIOD_TRIGGER_ROUTINE expired and IG is ON");
                LOG_I("Execute ST_PERIOD_TRIGGER_ROUTINE diagprocess");
                /*RDG30-R-1220*/
                mSchedulerQueue.executeQueue(false);
            } else {
                LOG_I("ST_PERIOD_TRIGGER_ROUTINE expired and IG is OFF");
            }

        } else {
            LOG_I("Don't restart timer");
            if((mSchedulerQueue.getFirstSchedType() != Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE) 
                || (mSchedulerQueue.getIsExecuted() == 0U) ) {           
                mSchedulerQueue.executeQueue(false);
            } else {
                LOG_I("Do not execute Sched index: %llu", mSchedulerQueue.getSchedIndex());
            }
        }
    } else {
        LOG_I("FUNC_TYPE_INVALID");
    } 
}

bool SchedulerQueue::insert(const android::sp<SchedulerTime> SchedTime_data) {
    bool result{false};
    LOG_D("SchedulerQueue index: %llu", mSchedIndex);
    LOG_D("SchedulerQueue prio: %d", mPioShedQue);
    try {
        (void)mSchedQueue_Sto.insert(SchedTime_data);
        mSchedQueue_Sto_It = mSchedQueue_Sto.begin();
        if(SchedTime_data->getPrio()<mPioShedQue) {
            mPioShedQue = SchedTime_data->getPrio();
        }
        result = true;
    } catch (const std::bad_alloc&) {
        LOG_I("Insert to queue fail");
    }
    return result;
}

bool SchedulerQueue::isSameIGON() {
    bool result {false};
    if(mSchedQueue_Sto_It == mSchedQueue_Sto.begin()) {
        if (Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE == (*mSchedQueue_Sto_It)->getType()) {
            LOG_I("Schedule type ST_IG_ON_TRIGGER_ROUTINE = 0x%02x", (*mSchedQueue_Sto_It)->getType());
            result = true;
        }
    }
    return result;
}

bool SchedulerQueue::isSameIGOFF() {
    bool result {false};
    if(mSchedQueue_Sto_It == mSchedQueue_Sto.begin()) {
        if ((Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT == (*mSchedQueue_Sto_It)->getType())
        || (Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_ROUTINE == (*mSchedQueue_Sto_It)->getType())) {
            LOG_I("Schedule type IG OFF = 0x%02x", (*mSchedQueue_Sto_It)->getType());
            result = true;
        }
    }
    return result;
}

void SchedulerQueue::readyToStart() {
    LOG_D("Start readyToStart");
    mSchedQueue_Sto_It = mSchedQueue_Sto.begin();
    LOG_D("Queue first function type 0x%02x", (*mSchedQueue_Sto_It)->getFuncType());
    if(false == isSameIGOFF()) {
        if ((getFirstSchedType() !=  Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE) 
            || (mpSchedulerMngr->getIgOnRoutineExpired() == 1U)) {
            LOG_I("ExecuteQueue ShedIndex %llu", mSchedIndex);
            /*RDG30-R-1219 RDG30-R-1075*/
            executeQueue(true);
        } else {
            /* Do nothing */
        }
    } else {
        LOG_I("SchedIndex %llu is same IG_OFF", mSchedIndex);
    }
}

void SchedulerQueue::startTime() {
    LOG_I("Start time Sched index: %llu Duration: %lld", mSchedIndex, mpSchedulerTime->getDuration());
    const int64_t mDuration {mpSchedulerTime->getDuration()};
    restartAlarm(mDuration);
    mpSchedulerTime->setNewSchedState(false);
    //TBD: check isElapsed()
}

void SchedulerQueue::startTime(const int64_t duration) {
    LOG_I("Start time Sched index: %llu Duration: %lld", mSchedIndex, duration);
    restartAlarm(duration);
    mpSchedulerTime->setNewSchedState(false);
    //TBD: check isElapsed()
}
void SchedulerQueue::stopTime() {
    LOG_I("STOP TIMER with TimerID: %d Sched index: %llu", mTimerID, mSchedIndex);
    if(mAlarmservice != nullptr) {
        const error_t res {mAlarmservice->cancel(mTimerID)};
        if(res == E_OK) {
            LOG_I("Cancel TimerID: %d success", mTimerID);
        } else {
            LOG_I("Cancel TimerID: %d FAIL", mTimerID);
        }
    } else {
        LOG_I("AlarmManager is null");
    }
}

void SchedulerQueue::updateTime() {
    const int64_t mDuration {mpSchedulerTime->getDuration()};
    restartAlarm(mDuration);
}

Rdg_Sched_Type::SchedFuncType SchedulerQueue::getFirstFuncType() {
    Rdg_Sched_Type::SchedFuncType res{Rdg_Sched_Type::SchedFuncType::FUNC_TYPE_INVALID};
    if(mSchedQueue_Sto_It == mSchedQueue_Sto.begin()) {
        LOG_I("First item function type: 0x%02x", (*mSchedQueue_Sto_It)->getFuncType());
        res = ((*mSchedQueue_Sto_It)->getFuncType());
    } else {
        res = Rdg_Sched_Type::SchedFuncType::FUNC_TYPE_INVALID;
    }
    return res;
}

Rdg_Sched_Type::SchedType SchedulerQueue::getFirstSchedType() {
    Rdg_Sched_Type::SchedType res{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
    if(mSchedQueue_Sto_It == mSchedQueue_Sto.begin()) {
        LOG_I("First item sched type: 0x%02x", (*mSchedQueue_Sto_It)->getType());
        res = ((*mSchedQueue_Sto_It)->getType());
    } else {
        res = Rdg_Sched_Type::SchedType::ST_UNKNOWN;
    }
    return res;
}

void SchedulerQueue::init(const android::sp<SchedulerTime> SchedTime_data) {
    bool isInserted{false};
    isInserted = insert(SchedTime_data);
    if(true == isInserted) {
        mSchedQueue_Sto_It = mSchedQueue_Sto.begin();
    }
}

void SchedulerQueue::restartAlarm(const int64_t mDuration) {
    if(mAlarmservice != nullptr) {
        // mAlarmservice->cancel(mTimerID);
        if(mDuration < 0) {
            LOG_I("shcedule is elapsed!!");
            //TBD: isElapsed = true;
        } else {
            // TBD: isElapsed = false;
           const int32_t res{mAlarmservice->setAlarmFromNow(mDuration, mTimerID)};
           if(res == E_OK){
            LOG_D("set alarm successfull");
           } else {
            LOG_D("set alarm fail");
           }
        }
    } else {
        LOG_I("AlarmManager is null");
    }
}

// static int64_t SchedulerQueue::getLastOpComplTime() {
//     constexpr int64_t lastTime{0};
//     return lastTime;
// }

void SchedulerQueue::executeQueue(const bool processNewOnly) {
    mIsExecuted = 1U;
    SchedQueue_Sto_It it{};
    for(it = mSchedQueue_Sto.begin(); it != mSchedQueue_Sto.end(); it++) {
        const android::sp<SchedulerTime> pSchedTime {*it};
        LOG_I("Broadcast SchedID: %llu Prio: %d FuncType: 0x%02x", 
            pSchedTime->getRequestID(), pSchedTime->getPrio(), pSchedTime->getFuncType());
        if(processNewOnly) {
            LOG_D("Process new SchedulerTime only");
            if(pSchedTime->getNewSchedState()) {
                mpSchedulerMngr->executeFunction(pSchedTime);
            } else {
                LOG_D("SchedulerTime is old");
            }
        } else {
            LOG_D("Process all SchedulerTime");
            mpSchedulerMngr->executeFunction(pSchedTime);
        }
        pSchedTime->setNewSchedState(false);
    }
}

void SchedulerQueue::saveLastOpComplTime(const int64_t completeTime) {
    mLastOpComlTime = completeTime;
    /*Caculate expired duration for routine sched base on completeTime
    */
    const int64_t interval_duration {mpSchedulerTime->getDuration()};
    /* Get current time */
    const int64_t current_time{ParamsDef::getCurrentAcquisiteTime()};
    int64_t remain_time{0};
    int64_t mDuration{0};
    if(((completeTime < 0) && (current_time > completeTime + INT64_MAX)) ||
        ((completeTime > 0) && (current_time < completeTime + INT64_MIN))){
        // print error log
    }
    else{
        remain_time = current_time - completeTime;
    }
    if(((remain_time < 0) && (interval_duration > remain_time + INT64_MAX)) ||
        ((remain_time > 0) && (interval_duration < remain_time + INT64_MIN))){
        // print error log
    }
    else{
        mDuration = interval_duration - remain_time;
    }
    startTime(mDuration);
    mIsExecuted = 0U;
    mpSchedulerMngr->saveComplTimeToFile(mSchedIndex, completeTime);
    LOG_I("Check completeTime: %lld currentTime: %lld duration: %lld", completeTime, current_time, mDuration);
}

void SchedulerQueue::setLastOpComplTime(const int64_t completeTime) noexcept {
    mLastOpComlTime = completeTime;
}

int64_t SchedulerQueue::getLastOpComlTime() const noexcept {
    return mLastOpComlTime;
}

uint8_t SchedulerQueue::getIsExecuted() const noexcept{
    return mIsExecuted;
}

void SchedulerQueue::setIsExecuted(const uint8_t data) noexcept {
    mIsExecuted = data;
}

uint64_t SchedulerQueue::getSchedIndex() const noexcept{
    return mSchedIndex;
}

int64_t SchedulerQueue::getIntervalDuration() const noexcept {
    const int64_t interval_duration {mpSchedulerTime->getDuration()};
    return interval_duration;
}

uint32_t SchedulerQueue::getPrioSchedQue() const noexcept {
    return mPioShedQue;
}

}
