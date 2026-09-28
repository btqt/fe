#include "TriggerIDGenerator.h"

namespace rdgapp {

using android::Mutex;
using android::Singleton;

TriggerIDGenerator::TriggerIDGenerator() noexcept : android::Singleton<TriggerIDGenerator>() { //CID 9301238
    mCurrentId = 0U;
}

uint32_t TriggerIDGenerator::getNextId() noexcept {
    const android::AutoMutex _l{mLock};
    if(mCurrentId < static_cast<uint32_t>(UINT32_MAX))
    {
        ++mCurrentId;
    }
    else{
        // do nothing
    }
    mCurrentId %= MAX_ID;
    return mCurrentId;
}

void TriggerIDGenerator::setInit() noexcept {
    mCurrentId = 0U; //CID 9301471
}

ANDROID_SINGLETON_STATIC_INSTANCE(rdgapp::TriggerIDGenerator)
}
