#ifndef REMOTEDIAG_REGIONMANAGER_ADAPTER_H
#define REMOTEDIAG_REGIONMANAGER_ADAPTER_H

#include <iostream>
#include <memory>
#include <services/RegionManagerService/IRegionManagerService.h>
#include <services/RegionManagerService/IRegionManagerServiceType.h>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <Error.h>

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"

namespace rdgapp {

class RegionManagerAdapter {
public:
    RegionManagerAdapter(const RegionManagerAdapter& ) = default;
    RegionManagerAdapter& operator=(const RegionManagerAdapter& ) = default;

    RegionManagerAdapter(RegionManagerAdapter&& ) = default;
    RegionManagerAdapter& operator=(RegionManagerAdapter&& ) = default;

    static std::shared_ptr<RegionManagerAdapter> getInstance();
 
    uint8_t getNation();
    void registerService();
    android::sp<IRegionManagerService> getService();
    void onBinderDied(const android::wp<android::IBinder>& who);
    RegionManagerAdapter();
    virtual ~RegionManagerAdapter();
private:
    static std::shared_ptr<RegionManagerAdapter> instance;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient {nullptr};
    android::sp<IRegionManagerService> mRegionMService;
    android::sp<RemotediagHandler> mHandler = nullptr;
    mutable android::Mutex mDiedLock;
    static android::Mutex mInstanceLock;
};
}
#endif /* REMOTEDIAG_REGIONMANAGER_ADAPTER_H */
