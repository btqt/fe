#ifndef RDG_SCHED_TYPE_H
#define RDG_SCHED_TYPE_H

#include <rdg/v1/interfaces/RDG_common_message_definition.pb.h>

namespace rdgapp {

namespace Rdg_Sched_Type{

constexpr static int64_t IG_ON_TRIGGER_ROUTINE_DURATION {70}; /*RDG30-R-1074*/
constexpr static int64_t IG_OFF_TRIGGER_ROUTINE_DURATION {3}; /*RDG30-R-1083*/
// constexpr static int64_t IG_OFF_PROCESS_EXPRIED_DURATION {15}; /*RDG30-R-1086*/
constexpr static int64_t IG_ON_PERIOD_TRIGGER_DURATION {90}; /*RDG30-R-1080*/

constexpr static int64_t SEC_PER_DAY {86400};
constexpr static int64_t SEC_PER_HOUR {3600};
constexpr static int64_t SEC_PER_MIN {60};
constexpr static int64_t MILLIS_PER_SEC {1000};
// This enum definition for Type of Schedule
enum class SchedType : int32_t {
    ST_UNKNOWN                                  = vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType::ScheduleInformation_ScheduleType_ST_UNKNOWN,
    ST_IMMEDIATE_IG_ON_ONE_SHOT                 = vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT,
    ST_IMMEDIATE_ANY_POWER_STATUS_ONE_SHOT      = vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_ANY_POWER_STATUS_ONE_SHOT,
    ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT  = vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType::ScheduleInformation_ScheduleType_ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT,
    ST_IG_ON_TRIGGER_ROUTINE                    = vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType::ScheduleInformation_ScheduleType_ST_IG_ON_TRIGGER_ROUTINE,
    ST_PERIOD_TRIGGER_ROUTINE                   = vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType::ScheduleInformation_ScheduleType_ST_PERIOD_TRIGGER_ROUTINE,
    ST_IG_OFF_TRIGGER_ROUTINE                   = vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType::ScheduleInformation_ScheduleType_ST_IG_OFF_TRIGGER_ROUTINE,

    ST_SCHE_INT_MIN                             = vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType::ScheduleInformation_ScheduleType_ScheduleInformation_ScheduleType_INT_MIN_SENTINEL_DO_NOT_USE_,
    ST_SCHE_INT_MAX                             = vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType::ScheduleInformation_ScheduleType_ScheduleInformation_ScheduleType_INT_MAX_SENTINEL_DO_NOT_USE_
};

// This enum definition is base on 19CY remote service specification
enum class SchedFuncType : uint8_t {
    FUNC_UNKNOWN = 0x00,
    CENTER_RQ_ALLDTCSSR = 0x01,
    CENTER_RQ_ALLROB = 0x02,
    CENTER_RQ_ROBSSR = 0x03,
    CENTER_RQ_ECUINFORMATION = 0x04,
    CENTER_RQ_DIRECTCOMMAND = 0x05,
    COLLECTION_COND_DIRECTCOMMAND = 0x06,
    COLLECTION_COND_ECUINFORMATION = 0x07,
    COLLECTION_COND_ROBROBSSRDIDEVENT = 0x08,
    FUNC_TYPE_INVALID=0xFF
};

// enum SchedDataError {
//     E_NO_ERROR=0,
//     E_OUT_OF_RANGE,
//     E_OTHERS
// };
}
}
#endif /* RDG_SCHED_TYPE_H */
