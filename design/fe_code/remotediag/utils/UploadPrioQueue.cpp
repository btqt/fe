#include "UploadPrioQueue.h"

namespace rdgapp {

bool UploadPrioQueue::insertQueue(const android::sp<UploadTask>& pMsgData) {
    const android::Mutex::Autolock _l{mLock};
    bool res{false};
    LOG_I("UploadPrioQueue::insertQueue ++");
    if (this->isQueueFull()) {
        LOG_I("UploadPrioQueue::insertQueue() Queue full, New request is discarded");
        res = false;
    } else {
        try {
            (void)mQueue.insert(pMsgData);
            res = true;
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
    const bool isEmpty{mQueue.empty()};
    if (!isEmpty) {
        const PriorityQueueIter iIter {mQueue.begin()};
        (void)mQueue.erase(iIter);
    }
}

bool UploadPrioQueue::isQueueEmpty() const noexcept {
    bool empty{false};
    {
        const android::Mutex::Autolock _l{mLock};
        empty = mQueue.empty();
    }
    return empty;
}

bool UploadPrioQueue::isQueueFull() const noexcept{
    const uint32_t iMaxSize {this->getMaxSize()};
    const uint32_t iCurrentSize {this->getQueueSize()};
    return (iMaxSize == iCurrentSize);
}

uint32_t UploadPrioQueue::getQueueSize() const noexcept{
    return mQueue.size();
}

android::sp<UploadTask> UploadPrioQueue::pop() {
    // const android::sp<UploadTask> iResult {this->getTop()};
    // this->removeTop();
    // return iResult;
    android::sp<UploadTask> res{nullptr};
    const android::Mutex::Autolock _l{mLock};
    const bool isEmpty{mQueue.empty()};
    if (!isEmpty) {
        PriorityQueueIter iIter{};
        iIter = mQueue.begin();
        res = *iIter;
        (void)mQueue.erase(iIter);
    } else {
        LOG_E("Queue is empty, nothing to get");
    }
    return res;
}

android::sp<UploadTask> UploadPrioQueue::getTop() {
    android::sp<UploadTask> res{nullptr};
    const android::Mutex::Autolock _l{mLock};
    const bool isEmpty{mQueue.empty()};
    if (!isEmpty) {
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

void UploadPrioQueue::clearQueue() noexcept {
    const android::Mutex::Autolock _l{mLock};
    mQueue.clear();
}

void UploadPrioQueue::dumpUploadQueue() {
    PriorityQueueIter iIter{};
    LOG_I("===================");
    for (iIter = mQueue.begin(); iIter != mQueue.end(); ++iIter) {
        LOG_I("Id: %llu - Priority: %u", (*iIter)->getUploadId(), (*iIter)->getUploadPrio());
    }
    LOG_I("===================");
}
}
