#ifndef REMOTEDIAG_REG_ADAPTER_CALIB_MANAGER_H
#define REMOTEDIAG_REG_ADAPTER_CALIB_MANAGER_H

// Calib Manager
#include <iostream>
#include <memory>
#include <services/CalibManagerService/ICalibManagerReceiver.h>
#include <services/CalibManagerService/ICalibManagerService.h>
#include <services/CalibManagerService/ICalibManagerServiceType.h>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <Error.h>

#include "DiagManagerAdapter.h"
#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"

namespace rdgapp {

class RemotediagHandler;
class CalibManagerAdapter
{
    // constexpr static uint16_t  DID {0U};
    class CalibReceiver : public BnCalibManagerReceiver {
    public:
        CalibReceiver(CalibManagerAdapter& pr) noexcept : parent(pr) {}
        virtual ~CalibReceiver() = default;
        CalibReceiver(CalibReceiver const&) = default;
        CalibReceiver& operator=(CalibReceiver const&) = default;
        CalibReceiver(CalibReceiver&&) = delete;
        CalibReceiver& operator=(CalibReceiver&&) = delete;
        virtual int32_t onCalibDidChanged(uint16_t const DID, size_t const bufLen, uint8_t* const buf) {
            return parent.onCalibDidChanged(DID, bufLen, buf);
        }
    private:
        CalibManagerAdapter& parent;
    };

public:
    CalibManagerAdapter() noexcept ;
    virtual ~CalibManagerAdapter() noexcept;
    CalibManagerAdapter(CalibManagerAdapter const&) = default;
    CalibManagerAdapter& operator=(CalibManagerAdapter const&) = default;
    CalibManagerAdapter(CalibManagerAdapter&&) = delete;
    CalibManagerAdapter& operator=(CalibManagerAdapter&&) = delete;
    static std::shared_ptr<CalibManagerAdapter> getInstance();

    int32_t onCalibDidChanged(const uint16_t DID, const size_t bufLen, const uint8_t * const buf);
    android::sp<ICalibManagerService> getService();

    void registerService();
    void onBinderDied(const android::wp<android::IBinder>& who);
private:
    //static CalibManagerAdapter *instance;
    static std::shared_ptr<CalibManagerAdapter> instance;
    android::sp<ICalibManagerService> mCalibMgrService;
    android::sp<RemotediagHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<ICalibManagerReceiver> mCalibReceiver = nullptr;

};
}
#endif
