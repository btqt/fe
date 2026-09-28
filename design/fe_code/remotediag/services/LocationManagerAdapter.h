#ifndef REMOTEDIAG_LOCATIONMANAGER_H
#define REMOTEDIAG_LOCATIONMANAGER_H

#include <iostream>
#include <fstream>
#include <memory>

#include <services/LocationManagerService/ILocationManagerService.h>
#include <services/LocationManagerService/ILocationManagerServiceType.h>


#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include "Error.h"

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"

namespace rdgapp {

class RemotediagHandler;
class LocationManagerAdapter
{
public:
    LocationManagerAdapter();
    ~LocationManagerAdapter() noexcept;
    LocationManagerAdapter(const LocationManagerAdapter& ) = delete;
    LocationManagerAdapter& operator=(const LocationManagerAdapter& ) = delete;
    LocationManagerAdapter(LocationManagerAdapter&& ) = delete;
    LocationManagerAdapter& operator=(LocationManagerAdapter&& ) = delete;
    static std::shared_ptr<LocationManagerAdapter> getInstance();
    
    virtual void registerService();
    sp<CommonDefine::RDGLocationData> getLocationData();

private:
    // Interfaces from ILocationManager
    virtual android::sp<ILocationManagerService> getLocationManagerService();
    // int32_t getGPSStatus();
    void setMaxLocationNumber(const uint32_t& num_of_location) noexcept {mMaxLocationNumberTobeSaved = num_of_location;}

    void onBinderDied(const android::wp<android::IBinder>& who);

private:
    static std::shared_ptr<LocationManagerAdapter> instance;
    android::sp<RemotediagHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient>   mServiceDeathRecipient = nullptr;
    android::sp<ILocationManagerService> mLocationservice = nullptr;
    uint32_t                             mMaxLocationNumberTobeSaved {3U};
    std::vector<sp<LocationData>>        mCurrentLocationData;
    mutable android::Mutex mDiedLock;
    static android::Mutex mInstanceLock;
};
}
#endif // REMOTEDIAG_LOCATIONMANAGER_H
