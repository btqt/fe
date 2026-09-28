#pragma once
#include <google/protobuf/util/json_util.h>
#include "diagprocess/DiagTrigger.h"
#include "diagprocess/CenterReqData.h"

namespace rdgapp {
class CenterRequestJob: public android::RefBase
{
public:

    CenterRequestJob(const uint64_t aId
                    , const uint8_t messID
                    , const uint32_t prio
                    , const DiagTrigger::DiagTriggerType triggerType
                    , const Rdg_Sched_Type::SchedType schedType
                    , const std::shared_ptr<google::protobuf::Message>& aCenterRequest)
    : mPayload(aCenterRequest) 
    {
        mCenterRequestHeader = new CenterReqData(aId, messID, prio, triggerType, schedType);
    }
    
    ~CenterRequestJob() final = default;
    inline const std::shared_ptr<google::protobuf::Message> getPayload(void) const noexcept {return mPayload;}
    inline const uint64_t getId(void) const noexcept {return mCenterRequestHeader->getCenterReq_CollectionID();}
    inline const Rdg_Sched_Type::SchedType getScheduleType(void) const noexcept {return mCenterRequestHeader->getScheduleType();}
    inline const uint8_t getMessageId(void) const noexcept {return mCenterRequestHeader->getCenterReq_messageID();}
    inline const android::sp<CenterReqData> getCenterRequestHeader(void) const noexcept {return mCenterRequestHeader;}

private:
    android::sp<CenterReqData> mCenterRequestHeader;
    std::shared_ptr<google::protobuf::Message> mPayload;
    // android::sp<::Buffer> mPayload;
};

}
