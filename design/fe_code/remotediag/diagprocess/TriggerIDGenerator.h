#ifndef TRIGGER_ID_GENERATOR_H
#define TRIGGER_ID_GENERATOR_H

#include <utils/Singleton.h>

namespace rdgapp {

class TriggerIDGenerator : public android::Singleton<TriggerIDGenerator> {
    friend class android::Singleton<TriggerIDGenerator>;
public:
    uint32_t getNextId() noexcept;
    void setInit() noexcept;

private:
    static constexpr uint32_t MAX_ID {10000U};

    TriggerIDGenerator() noexcept;
    ~TriggerIDGenerator() = default;
    TriggerIDGenerator(const TriggerIDGenerator&) = default;
    TriggerIDGenerator(TriggerIDGenerator&&) = default;
    TriggerIDGenerator& operator=(const TriggerIDGenerator&) = default;
    TriggerIDGenerator& operator=(TriggerIDGenerator&&) = default;

    uint32_t mCurrentId;
    mutable android::Mutex mLock;
};
}
#endif //TRIGGER_ID_GENERATOR_H
