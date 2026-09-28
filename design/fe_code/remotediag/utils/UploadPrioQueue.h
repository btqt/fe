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
    void removeTop();

    bool isQueueEmpty() const noexcept;
    bool isQueueFull() const noexcept;
    bool isQueueOverFlow() const noexcept;

    uint32_t getQueueSize() const noexcept;
    uint32_t getLowestPriority();
    // virtual TscCommQueueType getQueueType()=0;
    static uint32_t getMaxSize() noexcept;
    void clearQueue() noexcept;
    void dumpUploadQueue();

protected:
    void discardPriorityIter(const PriorityQueueIter pPriorityIter);
    void discardWithRequestId(const uint32_t pRequestId);
private:
    static constexpr uint32_t QUEUE_SIZE_MAX {300U};
    RdgUploadPrioQueue mQueue;
    mutable android::Mutex mLock;
};
}
#endif // RDG_UPLOAD_PRIO_QUEUE_H
