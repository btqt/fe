#include "FilteringList.h"
#include <limits>

namespace rdgapp {

FilteringList::FilteringList() : android::RefBase(), mList{}, mTimeData{0U} , mLocation{new CommonDefine::RDGLocationData()}, mTriggerTime{0}, mIsUploading{false} {
    mOdoValue = 0U;
    mOdoUnit = 0U;
    LOG_D("Init FilteringList");
}

int32_t FilteringList::isWarningInFilterList(const android::sp<WarningSignal> warning) {
    int32_t ret {-1};
    for (uint32_t i {0U}; i < mList.size(); i++) {
        if (warning == mList[i]) {
            ret = static_cast<int32_t>(i);
            break;
        }
    }
    return ret;
}

void FilteringList::addToFilterList(const android::sp<WarningSignal> warning) {
    mList.push_back(warning);
}

uint32_t FilteringList::getSize() const noexcept {
    return mList.size();
}

void FilteringList::removeFromFilterList(const android::sp<WarningSignal> warning) {
    const int32_t tempPos {isWarningInFilterList(warning)};
    if (tempPos >= 0) {
        const std::vector<android::sp<WarningSignal>>::iterator it {mList.begin()};
        if (it != mList.end()) {
            (void)mList.erase(mList.cbegin() + tempPos);
        }

        if (getSize() == 0U) {
            mLocation = new CommonDefine::RDGLocationData();
        }
    }
}

void FilteringList::reset() {
    for (uint8_t i {0U}; i < mList.size(); i++) {
        mList[i]->setFilteringMode(static_cast<uint8_t>(FilteringMode::FILTERING_MODE_NOTHING));
        mList[i]->getConfirmTimer()->stop();
        mList[i]->setFilterCounter(mList[i]->getWarningProperty() / 2U);
    }
    mList.clear();
    // mTimeLocBuffer->clear();
    mTimeData = 0U;
    mOdoValue = 0U;
    mOdoUnit = 0U;
    mLocation = new CommonDefine::RDGLocationData();
    mTriggerTime = 0;
    LOG_I({"FilteringList reset - number of signal in filter list %d"}, mList.size());
}

bool FilteringList::getTriggerDiag() {
    bool ret {false};
    for (uint8_t i {0U}; i < mList.size(); i++) {
        if (mList[i]->getTriggerDiag()) {
            ret = true;
            break;
        }
    }
    return ret;
}

bool FilteringList::checkAllFilterCounterExpired() {
    bool ret {true};
    for (uint8_t i {0U}; i < mList.size(); i++) {
        if (mList[i]->getFilterCounter() != 0U) {
            ret = false;
            break;
        }
    }
    return ret;
}

bool FilteringList::isUpdateWarningCounter() {
    bool isNeedUpdate{false};
    for (size_t i {0U}; i < mList.size(); i++) {
        const bool preWC{mList[i]->getWarningCounter()};
        const bool newWC{mList[i]->getFilteringMode() == static_cast<uint8_t>(FilteringMode::FILTERING_MODE_DETECT)};
        if(preWC != newWC)
        {
            isNeedUpdate = true;
            mList[i]->setWarningCounter(newWC);
            LOG_D("Index(%d) - Warning counter(%d)", i, newWC);
        }
    }
    return isNeedUpdate;
}

void FilteringList::revertWarningCounter() {
    for (size_t i {0U}; i < mList.size(); i++) {
        const bool warningCounter {mList[i]->getFilteringMode() == static_cast<uint8_t>(FilteringMode::FILTERING_MODE_CANCEL)};
        mList[i]->setWarningCounter(warningCounter);
        LOG_D("Index(%d) - Warning counter(%d)", i, warningCounter);
    }
}

void FilteringList::setFilteringTimeAndLocation(const uint64_t timeData, const android::sp<CommonDefine::RDGLocationData> location, const uint32_t odoValue, const uint32_t odoUnit) {
    mTimeData = timeData;
    mLocation = location;
    mOdoValue = odoValue;
    mOdoUnit = odoUnit;
    if(timeData < static_cast<uint64_t>(INT64_MAX)) //condition to handle exception case
    {
        mTriggerTime = static_cast<int64_t>(timeData);
    }
    else
    {
        mTriggerTime = 0;
    }
}

void FilteringList::getFilteringTimeAndLocation(uint64_t& timeData, android::sp<CommonDefine::RDGLocationData>& location, uint32_t& odoValue, uint32_t& odoUnit) {
    timeData = mTimeData;
    location = mLocation;
    odoValue = mOdoValue;
    odoUnit = mOdoUnit;
    LOG_D("Warning data : time(%llu) - odoValue(%lu) - odoUnit(%lu)",timeData, odoValue, odoUnit);
}

int64_t FilteringList::getTriggerTime() const noexcept {
    return mTriggerTime;
}

void FilteringList::setUploadStatus(const bool isUploading) noexcept {
    mIsUploading = isUploading;
}

const bool FilteringList::isUploading() const noexcept {
    return mIsUploading;
}
}
