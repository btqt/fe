#ifndef PRIORITY_QUEUE_COMPARE_H
#define PRIORITY_QUEUE_COMPARE_H

#include <utils/RefBase.h>
#include "UploadTask.h"

namespace rdgapp {

class UploadPrioQueueCompare {
    public:
        bool operator()(const android::sp<UploadTask> lhs, const android::sp<UploadTask> rhs) const;
};
}
#endif  // PRIORITY_QUEUE_COMPARE_H
