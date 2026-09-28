#include "SchedulerTime.h"

namespace rdgapp {

SchedulerTime::SchedulerTime(const Rdg_Sched_Type::SchedType pSchedType, const uint64_t request_id, 
        const uint8_t interval_d, const uint8_t interval_h, const uint8_t interval_m): android::RefBase() {

    this->mIntervalDay = interval_d;
    this->mIntervalHour = interval_h;
    this->mIntervalMin = interval_m;
    this->mType = pSchedType;
    this->mFuncType = Rdg_Sched_Type::SchedFuncType::FUNC_TYPE_INVALID;
    this->mRequestId = request_id;
    this->mPrio = DiagTrigger::PRIO_MIN;
    newSchedState = true;
    if (Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE == mType) {
        LOG_I("ST_IG_ON_TRIGGER_ROUTINE Ignore collection condition interval data");
        mDuration = Rdg_Sched_Type::IG_ON_TRIGGER_ROUTINE_DURATION * Rdg_Sched_Type::MILLIS_PER_SEC;
    } else if ((Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_ROUTINE == mType) || 
        (Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT == mType)) {
        mDuration = Rdg_Sched_Type::IG_OFF_TRIGGER_ROUTINE_DURATION * Rdg_Sched_Type::MILLIS_PER_SEC;
    } else {
        mDuration = ((static_cast<int64_t>(interval_d) * Rdg_Sched_Type::SEC_PER_DAY) 
                    + (static_cast<int64_t>(interval_h) * Rdg_Sched_Type::SEC_PER_HOUR) 
                    + (static_cast<int64_t>(interval_m) * Rdg_Sched_Type::SEC_PER_MIN)) * Rdg_Sched_Type::MILLIS_PER_SEC;
    }
    LOG_D("day: %d hour: %d min: %d duration: %lld", interval_d, interval_h, interval_m, mDuration);
}

Rdg_Sched_Type::SchedType SchedulerTime::getType() const noexcept {
    return mType;
}

Rdg_Sched_Type::SchedFuncType SchedulerTime::getFuncType() const noexcept {
    return mFuncType;
}

uint64_t SchedulerTime::getRequestID() const noexcept {
    return mRequestId;
}

int64_t SchedulerTime::getDuration() const noexcept {
  return mDuration;
}

uint8_t SchedulerTime::getDayInterval() const noexcept {
    return mIntervalDay;
}

uint8_t SchedulerTime::getHourInterval() const noexcept {
    return mIntervalHour;
}

uint8_t SchedulerTime::getMinuteInterval() const noexcept {
    return mIntervalMin;
}

uint32_t SchedulerTime::getPrio() const noexcept {
    return mPrio;
}

void SchedulerTime::setFuncType(const Rdg_Sched_Type::SchedFuncType funcType) noexcept {
    this->mFuncType = funcType;
}

void SchedulerTime::setPrio(const uint32_t data) noexcept {
    this->mPrio = data;
}
void SchedulerTime::setNewSchedState(const bool data) noexcept {
    newSchedState = data;
}
bool SchedulerTime::getNewSchedState() const noexcept {
    return newSchedState;
}
}
