#include "diagprocess/DiagTrigger.h"

namespace rdgapp {

DiagTrigger::DiagTrigger(const DiagTriggerType type, const uint32_t priority, const DiagTriggerFunc func, const uint32_t triggerId) noexcept
    :android::RefBase()
    , mWarningTriggerTime(0)
    , mType(type)
    , mState(DiagTriggerState::TRIGGER_PENDING)
    , mFunc(func)
    , mPriority(priority)
    , mTriggerId(triggerId)
    , mTriggerTime(DEFAULT_TIME_UNDEFINED_VALUE)
    , mCollectionId(0U)
    , mNotificationId(0U)
    , latitude(0)
    , longitude(0) {
}

DiagTrigger::DiagTrigger(const DiagTrigger& other) noexcept: 
    android::RefBase()
    , mWarningTriggerTime(other.mWarningTriggerTime)
    , mType(other.mType)
    , mState(other.mState)
    , mFunc(other.mFunc)
    , mPriority(other.mPriority)
    , mTriggerId(other.mTriggerId)
    , mTriggerTime(other.mTriggerTime)
    , mCollectionId(other.mCollectionId)
    , mNotificationId(other.mNotificationId)
    , latitude(other.latitude)
    , longitude(other.longitude) {
}

void DiagTrigger::changeState(const DiagTriggerState state) noexcept {
    mState = state;
}

void DiagTrigger::setCollectionId(const uint64_t id) noexcept { //CID 9301986 
    mCollectionId = id;
}

void DiagTrigger::setNotificationId(const uint64_t notiId) noexcept {
    mNotificationId = notiId;
}
}
