#ifndef DIAG_TRIGGER_H
#define DIAG_TRIGGER_H

#include <cstdint>
#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>

#include "common_def.h"
#include "ParamsDef.h"
#include "utils/Logger.h"
namespace rdgapp {

class DiagTrigger : public android::RefBase {
public:
    enum class DiagTriggerType : int32_t {
    DIAG_TRIGGER_TYPE_MIN,
    WARNING_TRIGGER,        /* Warning trigger of all diag */
    IGON_TRIGGER,           /* Ig on trigger of all diag */
    OCCURRENCE_NOTIFICATION_TRIGGER,      /* OCCURRENCE_NOTIFICATION_TRIGGER */
    CENTER_TRIGGER,         /* Center request */
    ONE_SHOT_TRIGGER,       /* RS Schedule trigger one shot of monitoring or all diag */
    IGOFF_TRIGGER,          /* Ig off trigger */
    ROUTINE_TRIGGER,        /* RS Schedule trigger routine of monitoring */
    OTA_TRIGGER,
    CUSTOMIZE_TRIGGER,      /* Customize (user) trigger */
    DIAG_TRIGGER_TYPE_MAX
    };
    constexpr static uint32_t PRIO_MIN{0U};
    constexpr static uint32_t PRIO_OTA_HIGH{10U};
    constexpr static uint32_t PRIO_OTA_LOW{200U};
    constexpr static uint32_t PRIO_WARNING_TRIGGER{20U};
    //constexpr static uint32_t PRIO_OCCUR_ROB_NOTI_HIGH{50U}; // CID 9302055 
    //constexpr static uint32_t PRIO_OCCUR_ROB_NOTI_LOW{150U}; //CID 9302026
    constexpr static uint32_t PRIO_IG_ON_TRIGGER{220U};
    constexpr static uint32_t PRIO_MAX{255U};

    constexpr static uint32_t DEFAULT_DIAG_TRIGGER_ID{0U};
    constexpr static int64_t  DEFAULT_TIME_UNDEFINED_VALUE{0};

    enum class DiagTriggerFunc : int32_t
    {
        DIAG_FUNC_MIN,
        WARNING,
        ALLDIAG,
        ALLROB,
        DIRECT_COMMAND,
        ROBSSR,
        ROB_MORNITOR,
        ECU_UPDATE_INFO,
        OTA,
        LAST_UPLOAD,
        DTC,
        SSR,
        ROB,
        DIAG_FUNC_MAX
    };

    enum class DiagTriggerState : int32_t
    {
        TRIGGER_STATE_MIN,
        TRIGGER_PENDING,    /* Trigger is waiting for being processed (initial) */
        TRIGGER_PROCESSING, /* Trigger is being processed. (stand at top of priority queue) */
        TRIGGER_SUSPENDED,  /* Higher priority trigger occurs, current trigger will be suspended */
        TRIGGER_DISCARDED,
        TRIGGER_DONE,
        TRIGGER_STATE_MAX
    };
    explicit DiagTrigger(const DiagTriggerType type, const uint32_t priority, 
                        const DiagTriggerFunc func, const uint32_t triggerId) noexcept;
    virtual ~DiagTrigger() = default;
    DiagTrigger(const DiagTrigger& other) noexcept;
    DiagTrigger(DiagTrigger&&) = default;
    DiagTrigger& operator=(DiagTrigger&& other) noexcept {
        mWarningTriggerTime = other.mWarningTriggerTime;
        mType = other.mType;
        mState = other.mState;
        mFunc = other.mFunc;
        mPriority = other.mPriority;
        mTriggerId = other.mTriggerId;
        mTriggerTime = other.mTriggerTime;
        mCollectionId = other.mCollectionId;
        mNotificationId = other.mNotificationId;
        latitude = other.latitude;
        longitude = other.longitude;
        return *this;
    }
    DiagTrigger& operator=(const DiagTrigger& other) noexcept {
        mWarningTriggerTime = other.mWarningTriggerTime;
        mType = other.mType;
        mState = other.mState;
        mFunc = other.mFunc;
        mPriority = other.mPriority;
        mTriggerId = other.mTriggerId;
        mTriggerTime = other.mTriggerTime;
        mCollectionId = other.mCollectionId;
        mNotificationId = other.mNotificationId;
        latitude = other.latitude;
        longitude = other.longitude;
        return *this;
    }

    bool operator<(const DiagTrigger& other) const noexcept {
        bool res{false};
        if(mPriority == other.getPriority()) {
            res = (mTriggerId > other.getTriggerId());
        } else {
            res = (mPriority > other.getPriority());
        }
        return res;
    }

    void changeState(const DiagTriggerState state) noexcept;

    inline uint32_t getPriority() const noexcept {return mPriority;}
    inline DiagTriggerType getType() const noexcept {return mType;}
    inline DiagTriggerState getState() const noexcept {return mState;}
    inline void setState(const DiagTriggerState val) noexcept {mState = val;}
    inline DiagTriggerFunc getFunc() const noexcept {return mFunc;}
    inline uint32_t getTriggerId() const noexcept {return mTriggerId;}
    inline int64_t getTriggerTime() const noexcept {return mTriggerTime;}
    inline int64_t getWarningTriggerTime() const noexcept {return mWarningTriggerTime;}
    inline uint64_t getCollectionID() const noexcept {return mCollectionId;}
    inline uint64_t getNotificationID() const noexcept {return mNotificationId;}
    inline int32_t getLatitude()const noexcept {return latitude;};
    inline int32_t getLongtitude()const noexcept {return longitude;};
    inline void setTriggerTime(const int64_t data) noexcept {mTriggerTime = data;} //CID 9301132 
    inline void setWarningTriggerTime(const int64_t data) noexcept {mWarningTriggerTime = data;} //CID 9301132 
    void setCollectionId(const uint64_t id) noexcept;
    void setNotificationId(const uint64_t notiId) noexcept;
    void setLatitude(const int32_t data)noexcept {latitude = data;}; //CID 9374639 
    void setLongitude(const int32_t data)noexcept {longitude = data;}; //CID 9374498

private:
    int64_t mWarningTriggerTime;
    DiagTriggerType mType;
    DiagTriggerState mState;
    DiagTriggerFunc mFunc;
    uint32_t mPriority;
    uint32_t mTriggerId;
    int64_t mTriggerTime;
    uint64_t mCollectionId;
    uint64_t mNotificationId;
    int32_t latitude;
    int32_t longitude;
};
}
#endif // DIAG_TRIGGER_H
