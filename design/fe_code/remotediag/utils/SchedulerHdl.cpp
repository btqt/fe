#include "SchedulerHdl.h"
#include "SchedulerManager.h"

namespace rdgapp {

SchedulerHdl::SchedulerHdl(android::sp<sl::SLLooper>& privateLooper, const android::sp<SchedulerManager> SchedManager) noexcept
        : android::RefBase(), sl::Handler(privateLooper), mSchedManager(SchedManager) {}

SchedulerHdl::~SchedulerHdl() = default;

void SchedulerHdl::handleMessage (const android::sp<sl::Message>& handlemsg) {
    const int32_t what {handlemsg->what};
    LOG_I({"handler is processing with what: %d"}, what);

    switch(what) {
        case CMD_INIT_SCHEDULERHDL:
        {
            LOG_I("CMD_INIT_SCHEDULERHDL");
            break;
        }
        case MSG_NOTIFY_SCHED_COMPLETE:
        {
            LOG_I("MSG_NOTIFY_SCHED_COMPLETE");
            break;
        }
        default: {
            LOG_I("default");
            break;
        }
    }
}
}
