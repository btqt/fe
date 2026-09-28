#ifndef WARNING_SIGNAL_H
#define WARNING_SIGNAL_H

#include <Typedef.h>
#include <utils/RefBase.h>
#include <utils/Timer.h>
#include "utils/Logger.h"

namespace rdgapp {

enum class FilteringMode : int32_t {
    FILTERING_MODE_CANCEL,
    FILTERING_MODE_DETECT,
    FILTERING_MODE_NOTHING
};

///////////////////////////////////////////////////////////
//  Implementation of the Class WarningSignal
///////////////////////////////////////////////////////////
class WarningSignal : public android::RefBase {

public:
    /**
     * Constructor for Warning Signal.
     */
    explicit WarningSignal(const uint8_t pWarningID);
    /**
     * Destructor for Warning Signal.
     */
    virtual ~WarningSignal() override;
    WarningSignal(const WarningSignal&) = default;
    WarningSignal(WarningSignal&&) = default;
    WarningSignal& operator=(const WarningSignal&) = default;
    WarningSignal& operator=(WarningSignal&&) = default;


    /**
     * @brief get byte position in 18byte warning data.
     */
    uint8_t getByteNumber() const noexcept;   ///??????

    /**
     * @brief get bit position in the byte of warning data.
     */
    uint8_t getBitNumber() const noexcept;   ///????????

    const uint8_t getWarningID() const noexcept;                //CanID
    const bool getWarningCounter() const noexcept;              //warning Counter
    const bool getNotifyCenter() const noexcept;
    const uint8_t getWarningProperty() const noexcept;
    const bool getTriggerDiag() const noexcept;
    const uint8_t getFilterCounter() const noexcept;
    const uint8_t getTableValue() const noexcept;
    const uint8_t getFilteringMode() const noexcept;
    const android::sp<Timer> getConfirmTimer() const noexcept;

    void setWarningCounter(const bool warningCounter) noexcept;
    void setNotifyCenter(const bool notifyCenter) noexcept;
    void setWarningProperty(const uint8_t warningProperty) noexcept;
    void setTriggerDiag(const bool triggerDiag) noexcept;
    void setFilterCounter(const uint8_t filterCounter) noexcept;
    void setTableValue(const uint8_t value) noexcept;
    void setFilteringMode(const uint8_t mode) noexcept;
    void setConfirmTimer(const android::sp<Timer> confirmTimer) noexcept;

private:
    /**
     * Index of warning signal in array.
     */
    uint8_t mWarningID;   //range from 0->255

    /**
     * Warning Counter WC for warning filtering
     */
    bool mWarningCounter;

    /**
     * Necessity to notify to center.
     */
    bool mNotifyCenter;

    /**
     * Time for filtering warning signal. If warning is detected or resolved in WP
     * time, warning signal is defined or canceled.
     */
    uint8_t mWarningProperty;

    /**
     * Check if needed to triger diagnostic DTC/FFD
     */
    bool mTriggerDiag;

    uint8_t mFilterCounter;

    uint8_t mTableValue;

    uint8_t mFilteringMode;

    /*
    TimerID for each signal. It is key for WP 
    */
    android::sp<Timer> mConfirmTimer; 
};
}
#endif //!defined(WARNING_SIGNAL_H)
