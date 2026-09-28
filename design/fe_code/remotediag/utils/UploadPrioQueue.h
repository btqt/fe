#ifndef RDG_UPLOAD_PRIO_QUEUE_H
#define RDG_UPLOAD_PRIO_QUEUE_H

#include <set>
#include <utils/RefBase.h>
#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "UploadTask.h"
#include "UploadPrioQueueCompare.h"

namespace rdgapp {
class UploadPrioQueue : public android::RefBase {
public:
    using RdgUploadPrioQueue = std::multiset<android::sp<UploadTask>, UploadPrioQueueCompare>;
    using PriorityQueueIter = std::multiset<android::sp<UploadTask>, UploadPrioQueueCompare>::iterator;
    // using PriorityQueueReverseIter = std::multiset<android::sp<UploadTask>, UploadPrioQueueCompare>::reverse_iterator;
    UploadPrioQueue() = default;
    ~UploadPrioQueue() override = default;

    bool insertQueue(const android::sp<UploadTask>& pMsgData);
    android::sp<UploadTask> getTop();
    android::sp<UploadTask> pop();
    bool isQueueEmpty() const noexcept;
    void clearQueue() noexcept;
    void dumpUploadQueue();
private:
    void removeTop();
    bool isQueueFull() const noexcept;
    uint32_t getQueueSize() const noexcept;
    static uint32_t getMaxSize() noexcept;
    static constexpr uint32_t QUEUE_SIZE_MAX {1000U};
    RdgUploadPrioQueue mQueue;
    mutable android::Mutex mLock;
};
}
#endif // RDG_UPLOAD_PRIO_QUEUE_H
