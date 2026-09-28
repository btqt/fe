#ifndef SCHED_QUEUE_COMPARE_H
#define SCHED_QUEUE_COMPARE_H

#include <utils/RefBase.h>

namespace rdgapp {

class SchedulerQueueCompare {

public:
    bool operator()(const android::sp<SchedulerTime>& lhs, const android::sp<SchedulerTime>& rhs) const{
        return (lhs->getPrio() < rhs->getPrio());
    }
};
}
#endif /* SCHED_QUEUE_COMPARE_H */
