#include "WarningSignal.h"

namespace rdgapp {

WarningSignal::WarningSignal(const uint8_t pWarningID)
    : android::RefBase(),    
    mWarningID{pWarningID},  
    mWarningCounter{false},    
    mNotifyCenter{false},
    mWarningProperty{0U},
    mTriggerDiag{false},
    mFilterCounter{0U},
    mTableValue{0U},
    mFilteringMode{static_cast<uint8_t>(FilteringMode::FILTERING_MODE_NOTHING)},
    mConfirmTimer{} {
}

WarningSignal::~WarningSignal() {
    if (mConfirmTimer != nullptr) {
        mConfirmTimer.clear();
    }
}

const uint8_t WarningSignal::getWarningID() const noexcept {
    return mWarningID;
}

const bool WarningSignal::getWarningCounter() const noexcept {
    return mWarningCounter;
}

const bool WarningSignal::getNotifyCenter() const noexcept {
    return mNotifyCenter;
}

const uint8_t WarningSignal::getWarningProperty() const noexcept {
    return mWarningProperty;
}

const bool WarningSignal::getTriggerDiag() const noexcept {
    return mTriggerDiag;
}

const uint8_t WarningSignal::getFilterCounter() const noexcept {
    return mFilterCounter;
}

const uint8_t WarningSignal::getTableValue() const noexcept {
    return mTableValue;
}

const uint8_t WarningSignal::getFilteringMode() const noexcept {
    return mFilteringMode;
}

const android::sp<Timer> WarningSignal::getConfirmTimer() const noexcept {
    return mConfirmTimer;
}

void WarningSignal::setWarningCounter(const bool warningCounter) noexcept {
    mWarningCounter = warningCounter;
}

void WarningSignal::setNotifyCenter(const bool notifyCenter) noexcept {
    mNotifyCenter = notifyCenter;
}

void WarningSignal::setWarningProperty(const uint8_t warningProperty) noexcept {
    mWarningProperty = warningProperty;
}

void WarningSignal::setTriggerDiag(const bool triggerDiag) noexcept {
    mTriggerDiag = triggerDiag;
}

void WarningSignal::setFilterCounter(const uint8_t filterCounter) noexcept {
    mFilterCounter = filterCounter;
}

void WarningSignal::setTableValue(const uint8_t value) noexcept {
    mTableValue = value;
}

void WarningSignal::setFilteringMode(const uint8_t mode) noexcept {
    mFilteringMode = mode;
}

void WarningSignal::setConfirmTimer(const android::sp<Timer> confirmTimer) noexcept {
    mConfirmTimer = confirmTimer;
}

uint8_t WarningSignal::getByteNumber() const noexcept {
    return (mWarningID / 8U);   // get current block in CAN (Warning propty table) of warning signal (0 - 31)
}

uint8_t WarningSignal::getBitNumber() const noexcept {
    return (mWarningID % 8U);    // get current byte in block. ex: block 31, byte 2nd (0 - 7)
}

}
