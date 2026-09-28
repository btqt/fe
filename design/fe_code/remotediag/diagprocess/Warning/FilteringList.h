#ifndef FILTERING_LIST_H
#define FILTERING_LIST_H

#include <vector>
#include <utils/RefBase.h>

#include "utils/Logger.h"
#include "WarningSignal.h"
#include "services/LocationManagerAdapter.h"

namespace rdgapp {

class FilteringList : public android::RefBase {
public:
    FilteringList();
    virtual ~FilteringList() override = default;
    FilteringList(FilteringList const&) = default;
    FilteringList(FilteringList&&) = default;
    FilteringList& operator=(const FilteringList&) = default;
    FilteringList& operator=(FilteringList&&) = default;


    int32_t isWarningInFilterList(const android::sp<WarningSignal> warning);
    void addToFilterList(const android::sp<WarningSignal> warning);
    void removeFromFilterList(const android::sp<WarningSignal> warning);
    void reset();
    bool getTriggerDiag();
    uint32_t getSize() const noexcept;
    bool checkAllFilterCounterExpired();
    bool isUpdateWarningCounter();
    void revertWarningCounter();
    void setFilteringTimeAndLocation(const uint64_t timeData, const android::sp<CommonDefine::RDGLocationData> location, const uint32_t odoValue,const  uint32_t odoUnit);
    void getFilteringTimeAndLocation(uint64_t &timeData, android::sp<CommonDefine::RDGLocationData> &location, uint32_t &odoValue, uint32_t &odoUnit);
    int64_t getTriggerTime() const noexcept;
    void setUploadStatus(const bool isUploading) noexcept;
    const bool isUploading() const noexcept;
private:
    std::vector<android::sp<WarningSignal>> mList;   //List warning signal after done verification progress
    uint64_t mTimeData;
    android::sp<CommonDefine::RDGLocationData> mLocation;
    uint32_t mOdoUnit;
    uint32_t mOdoValue;
    int64_t mTriggerTime;
    bool mIsUploading;
};
}
#endif // !defined(FILTERING_LIST_H)
