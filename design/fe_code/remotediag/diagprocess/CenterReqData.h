#ifndef CENTER_REQ_DATA_H_
#define CENTER_REQ_DATA_H_

#include "utils/external/mindroid/lang/String.h"

#include <utils/Buffer.h>
#include <utils/RefBase.h>
#include <binder/Parcel.h>
#include "utils/SchedulerType.h"
#include "CenterReqDataType.h"
#include "DiagTrigger.h"
// #include "utils/CollectionCondition.h"

namespace rdgapp {

class CenterReqData : public android::RefBase
{
public:
  CenterReqData() noexcept;
  CenterReqData(const uint64_t ColID, const uint8_t messID, const uint32_t Prio, const DiagTrigger::DiagTriggerType type) noexcept;
  CenterReqData(const uint64_t ColID, const uint8_t messID, const uint32_t Prio, const DiagTrigger::DiagTriggerType type, const Rdg_Sched_Type::SchedType schedType) noexcept;
  ~CenterReqData() override = default;

  uint64_t getCenterReq_CollectionID() const noexcept {return centerReq_CollectionID;}
  uint8_t getCenterReq_messageID() const noexcept {return centerReq_messageID;}
  uint32_t getCenterReq_prio() const noexcept {return centerReq_prio;}
  Rdg_Sched_Type::SchedType getScheduleType() const noexcept {return mScheduleType;}
  DiagTrigger::DiagTriggerType getTrigger_type() const noexcept {return Trigger_type;}
  bool operator<(const CenterReqData& other) const noexcept {
    bool res{false};
    res = (centerReq_prio >= other.getCenterReq_prio());
    return res;
  }
private:
  uint64_t centerReq_CollectionID;
  uint8_t centerReq_messageID;
  uint32_t centerReq_prio;
  Rdg_Sched_Type::SchedType mScheduleType;
  DiagTrigger::DiagTriggerType Trigger_type;
};
}
#endif /* CENTER_REQ_DATA_H_ */
