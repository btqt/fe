#ifndef REMOTE_OCCURRENCE_ROB_NOTIFICATION_H
#define REMOTE_OCCURRENCE_ROB_NOTIFICATION_H
#include <binder/Parcel.h>
#include <utils/Buffer.h>
#include "services/LocationManagerAdapter.h"
#include "services/TimeManagerService/TimeManager.h"
#include "diagprocess/EcuInformation/RemoteEcuInformation.h"
#include "diagprocess/DiagTrigger.h"
#include "utils/CollectionCondition.h"

namespace rdgapp {

class OccurrentRobNotification : public android::RefBase
{
    public:
        OccurrentRobNotification();
        ~OccurrentRobNotification() = default;
        inline const int64_t& triggerTime(void) const noexcept {return mTriggerTimestamp;}
        inline const android::sp<CommonDefine::RDGLocationData>& triggerLocation(void) const noexcept {return mTriggerLocation;}
        inline DiagTrigger::DiagTriggerType triggerType(void) const noexcept {return mTriggerType;}
        inline int32_t priority(void) const noexcept {return mPriority;}
        inline Uint64 collectionConditionId() const noexcept {return mCollectionConditionId;}
        inline DirectCommandList* const directCommands() noexcept {return &mDirectCommand;}
        inline TargetCollectionDataRobSsr* const getTargetCollectionDataRobSsr() noexcept {return &mTargetCollectionConditionRobSsr;}
        inline void setCollectionConditionId(const Uint64 id) noexcept {mCollectionConditionId = id;}
        inline void setPriority(const int32_t priority) noexcept {mPriority = priority;}
        inline void setTriggerTime(const int64_t time) noexcept {mTriggerTimestamp = time;}
        inline void setTriggerLocation(const android::sp<CommonDefine::RDGLocationData> location) {mTriggerLocation = location;}
    private:
        DiagTrigger::DiagTriggerType mTriggerType;
        Uint64 mCollectionConditionId;
        int32_t mPriority;
        int64_t mTriggerTimestamp;
        android::sp<CommonDefine::RDGLocationData> mTriggerLocation;
        DirectCommandList mDirectCommand;
        TargetCollectionDataRobSsr mTargetCollectionConditionRobSsr;
};
}
#endif // REMOTE_OCCURRENCE_ROB_NOTIFICATION_H
