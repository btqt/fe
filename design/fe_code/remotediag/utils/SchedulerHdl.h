#ifndef RDG_SCHEDULE_HANDLER_H
#define RDG_SCHEDULE_HANDLER_H

#include <string>
#include <sstream>
#include <utils/Log.h>
#include "utils/Logger.h"
#include <utils/Handler.h>
#include <utils/SLLooper.h>
#include <utils/Buffer.h>
#include "utils/Timer.h"

namespace rdgapp {

class SchedulerManager;
static constexpr int32_t CMD_INIT_SCHEDULERHDL {2000};
static constexpr int32_t MSG_NOTIFY_SCHED_COMPLETE {2001};
static constexpr int32_t CMD_CHANGE_IG_STATUS {2002};
static constexpr int32_t MSG_NEW_SCHED_DATA {2003};
static constexpr int32_t CMD_IG_ON_ROUTINE_EXPIRED {2004};
static constexpr int32_t MSG_RECEIVE_NEW_CENTERCOMMNAD {2005};
class SchedulerHdl : public sl::Handler {
    public:
        explicit SchedulerHdl(android::sp<sl::SLLooper>& privateLooper, const android::sp<SchedulerManager> SchedManager) noexcept;
        ~SchedulerHdl() override;
        SchedulerHdl(const SchedulerHdl&) = default;
        SchedulerHdl(SchedulerHdl&&) = default;
        SchedulerHdl& operator=(const SchedulerHdl&) = default;
        SchedulerHdl& operator=(SchedulerHdl&&) = default;
        void handleMessage (const android::sp<sl::Message>& handlemsg) override;
    private:
        // void parseSchedData();
        android::sp<SchedulerManager> mSchedManager;
};
}
#endif /* RDG_SCHEDULE_HANDLER_H */
