#ifndef RMT_SCHED_TIME_H
#define RMT_SCHED_TIME_H

#include "utils/Logger.h"
#include "services/AlarmManagerService/IAlarmManagerService.h"
#include "services/AlarmManagerService/IAlarmExpireListener.h"
#include "SchedulerType.h"
#include "DiagTrigger.h"

namespace rdgapp {

class SchedulerTime : public android::RefBase {
public:
  SchedulerTime(const Rdg_Sched_Type::SchedType pSchedType, const uint64_t request_id, 
        const uint8_t interval_d, const uint8_t interval_h, const uint8_t interval_m);

  ~SchedulerTime() override = default;

  virtual Rdg_Sched_Type::SchedType getType() const noexcept;
  virtual Rdg_Sched_Type::SchedFuncType getFuncType() const noexcept;
  virtual uint64_t getRequestID() const noexcept;
  virtual int64_t getDuration() const noexcept;
  uint8_t getDayInterval() const noexcept;
  uint8_t getHourInterval() const noexcept;
  uint8_t getMinuteInterval() const noexcept;
  uint32_t getPrio() const noexcept;

  void setFuncType(const Rdg_Sched_Type::SchedFuncType funcType) noexcept;
  void setPrio(const uint32_t data) noexcept;
  void setNewSchedState(const bool data) noexcept;
  bool getNewSchedState() const noexcept;
private:
  uint8_t mIntervalDay;
  uint8_t mIntervalHour;
  uint8_t mIntervalMin;
  int64_t mDuration;
  Rdg_Sched_Type::SchedType mType;
  Rdg_Sched_Type::SchedFuncType mFuncType;
  uint64_t mRequestId;
  uint32_t mPrio;
  bool newSchedState;
};
}
#endif /* RMT_SCHED_TIME_H */
