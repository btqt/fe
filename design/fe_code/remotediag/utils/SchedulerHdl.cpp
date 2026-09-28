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
            android::sp<SchedulerManager::SchedCompleteInfo> schedData{nullptr};
            handlemsg->getObject(schedData);
            if (schedData != nullptr) {
                mSchedManager->handleSchedComplete(schedData);
            } else {
                LOG_E("schedData is nullptr");
            }
            break;
        }
        case CMD_CHANGE_IG_STATUS:
        {
            LOG_I("CMD_CHANGE_IG_STATUS");
            const int32_t status{handlemsg->arg1};
            if ((status == 0) || (status == 1)) {
                mSchedManager->handleReceiveIG(static_cast<bool>(status));
            } else {
                LOG_E("Invalid status received: %d", status);
            }
            break;
        }
        case MSG_NEW_SCHED_DATA:
        {
            LOG_I("MSG_NEW_SCHED_DATA");
            const bool isOnlyLoadSched{static_cast<bool>(handlemsg->arg1)};
            const bool isBooting{static_cast<bool>(handlemsg->arg2)};
            mSchedManager->handleNewSchedData(isOnlyLoadSched, isBooting);
            break;
        }
        case CMD_IG_ON_ROUTINE_EXPIRED:
        {
            LOG_I("CMD_IG_ON_ROUTINE_EXPIRED");
            mSchedManager->handleIgOnRoutineExpired();
            break;
        }
        case MSG_RECEIVE_NEW_CENTERCOMMNAD:
        {
            LOG_I("MSG_RECEIVE_NEW_CENTERCOMMNAD");
            android::sp<CenterReqData> centerCommand{nullptr};
            handlemsg->getObject(centerCommand);
            if (centerCommand != nullptr) {
                mSchedManager->handleNewCenterReqData(centerCommand);
            } else {
                LOG_E("centerCommand is nullptr");
            }
            break;
        }
        default: {
            LOG_I("default");
            break;
        }
    }
}
}
