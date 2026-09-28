#include "UploadPrioQueue.h"

namespace rdgapp {

bool UploadPrioQueue::insertQueue(const android::sp<UploadTask>& pMsgData) {
    const android::Mutex::Autolock _l{mLock};
    bool res{true};
    LOG_I("UploadPrioQueue::insertQueue ++");
    if (this->isQueueFull()) {
        LOG_I("UploadPrioQueue::insertQueue() Queue full, New request is discarded");
        res = false;
    } else {
        try {
            (void)mQueue.insert(pMsgData);
        } catch (const std::bad_alloc&) {
            LOG_E("UploadPrioQueue::insertQueue() Insert to queue fail");
            res = false;
        }
        this->dumpUploadQueue();
    }
    return res;
}

void UploadPrioQueue::removeTop() {
    const android::Mutex::Autolock _l{mLock};
    if (!isQueueEmpty()) {
        const PriorityQueueIter iIter {mQueue.begin()};
        (void)mQueue.erase(iIter);
    }
}

void UploadPrioQueue::discardWithRequestId(const uint32_t pRequestId) {
    const android::Mutex::Autolock _l{mLock};
    if (!isQueueEmpty()) {
        PriorityQueueIter iIter{};
        for (iIter = mQueue.begin(); iIter != mQueue.end(); ++iIter) {
            if ((*iIter)->getUploadId() == pRequestId) {
                (void)mQueue.erase(iIter);
                break;
            }
        }
    }
}

void UploadPrioQueue::discardPriorityIter(const PriorityQueueIter pPriorityIter) {
    const android::Mutex::Autolock _l{mLock};
    (void)mQueue.erase(pPriorityIter);
}

bool UploadPrioQueue::isQueueEmpty() const noexcept{
    return (mQueue.empty());
}

bool UploadPrioQueue::isQueueFull() const noexcept{
    const uint32_t iMaxSize {this->getMaxSize()};
    const uint32_t iCurrentSize {this->getQueueSize()};
    return (iMaxSize == iCurrentSize);
}

bool UploadPrioQueue::isQueueOverFlow() const noexcept{
    const uint32_t iMaxSize {this->getMaxSize()};
    const uint32_t iCurrentSize {this->getQueueSize()};
    return (iMaxSize < iCurrentSize);
}

uint32_t UploadPrioQueue::getQueueSize() const noexcept{
    return mQueue.size();
}

android::sp<UploadTask> UploadPrioQueue::pop() {
    const android::sp<UploadTask> iResult {this->getTop()};
    this->removeTop();
    return iResult;
}

android::sp<UploadTask> UploadPrioQueue::getTop() {
    android::sp<UploadTask> res{nullptr};
    if (!isQueueEmpty()) {
        PriorityQueueIter iIter{};
        iIter = mQueue.begin();
        res = *iIter;
    } else {
        LOG_E("UploadPrioQueue::getTop() Queue is empty, nothing to get");
    }
    return res;
}

uint32_t UploadPrioQueue::getMaxSize() noexcept {
    return QUEUE_SIZE_MAX;
}

// uint32_t UploadPrioQueue::discardOnOverFlow() {
//     LOG_I("UploadPrioQueue::discardOnOverFlow() ++");
//     uint32_t iDiscardedId = 0U;
//     PriorityQueueIter iIter;

//     uint32_t iLowestPriority = this->getLowestPriority();

//     for (iIter = mQueue.begin(); iIter != mQueue.end(); ++iIter) {
//         if ((*iIter)->getUploadPrio() == iLowestPriority) {
//             iDiscardedId = (*iIter)->getUploadId();
//             this->discardPriorityIter(iIter);
//             break;
//         }
//     }

//     LOG_I("UploadPrioQueue::discardOnOverFlow() -- iDiscardedId = %ld", iDiscardedId);
//     return iDiscardedId;
// }

void UploadPrioQueue::clearQueue() noexcept {
    const android::Mutex::Autolock _l{mLock};
    mQueue.clear();
}

uint32_t UploadPrioQueue::getLowestPriority() {
    uint32_t iLowestPriority {255U};
    if (!mQueue.empty()) {
        std::multiset<android::sp<UploadTask>, UploadPrioQueueCompare>::reverse_iterator iLastItem{};
        iLastItem = mQueue.rbegin();
        iLowestPriority = (*iLastItem)->getUploadPrio();
    }
    return iLowestPriority;
}

void UploadPrioQueue::dumpUploadQueue() {
    PriorityQueueIter iIter{};
    LOG_I("===================");
    for (iIter = mQueue.begin(); iIter != mQueue.end(); ++iIter) {
        LOG_I("Id: %lld - Priority: %d", (*iIter)->getUploadId(), (*iIter)->getUploadPrio());
    }
    LOG_I("===================");
}
}
