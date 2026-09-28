#include "CenterReqData.h"

namespace rdgapp {
  
CenterReqData::CenterReqData() noexcept : android::RefBase() 
{
  centerReq_CollectionID = 0U;
  centerReq_messageID = 0U;
  centerReq_prio = 0U;
  Trigger_type = DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN;
  mScheduleType = Rdg_Sched_Type::SchedType::ST_UNKNOWN;
}

CenterReqData::CenterReqData(const uint64_t ColID,
                             const uint8_t messID,
                             const uint32_t Prio,
                             const DiagTrigger::DiagTriggerType type) noexcept
  : CenterReqData()
{
  centerReq_CollectionID = ColID;
  centerReq_messageID = messID;
  centerReq_prio = Prio;
  Trigger_type = type;
  mScheduleType = Rdg_Sched_Type::SchedType::ST_UNKNOWN;
}

CenterReqData::CenterReqData(const uint64_t ColID,
                             const uint8_t messID,
                             const uint32_t Prio,
                             const DiagTrigger::DiagTriggerType type,
                             const Rdg_Sched_Type::SchedType schedType) noexcept
  : CenterReqData(ColID, messID, Prio, type)
{
  mScheduleType = schedType;
}
}
