#include "LocationManagerAdapter.h"
#include "../utils/ProxyIpcServer.h"
#include "../remotediagproxy/include/ProxyIpcProtocol.h"

namespace rdgapp {

// using namespace android;

LocationManagerAdapter::LocationManagerAdapter()
{
    mMaxLocationNumberTobeSaved = 3U;   // Default is same with EU 
    mServiceDeathRecipient = new ServiceDeathRecipient( [this] ( const android::wp<android::IBinder>& who ){
        this->onBinderDied(who);
    });
}

LocationManagerAdapter::~LocationManagerAdapter() noexcept
{
    if(LocationManagerAdapter::instance != nullptr) {
        LocationManagerAdapter::instance = nullptr;
    }   
}

std::shared_ptr<LocationManagerAdapter> LocationManagerAdapter::instance{nullptr};
android::Mutex LocationManagerAdapter::mInstanceLock{};
std::shared_ptr<LocationManagerAdapter> LocationManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<LocationManagerAdapter>();
        }
    }
    return instance;
}

void LocationManagerAdapter::registerService() {
    LOG_I("LocationManagerAdapter::registerService");
    mHandler = RemotediagHandler::getInstance();
}

android::sp<ILocationManagerService> LocationManagerAdapter::getLocationManagerService()
{
    return android::interface_cast<ILocationManagerService> (android::defaultServiceManager()->getService(android::String16("service_layer.LocationManagerService")));
}

void LocationManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
    LOG_I("LocationManagerAdapter::onBinderDied (no-op, service access is via proxy)");
    NOTUSED(who);
}

sp<CommonDefine::RDGLocationData> LocationManagerAdapter::getLocationData()
{
    const sp<CommonDefine::RDGLocationData> rdgLoc {new CommonDefine::RDGLocationData()};
    std::vector<uint8_t> responsePayload{};
    const bool requestOk{ProxyIpcServer::getInstance().requestAPICall(
        rdgipc::CommandId::LocationGetLocation,
        {},
        responsePayload,
        5000U)};

    if (requestOk && (responsePayload.size() >= 8U))
    {
        const int32_t lat{(static_cast<int32_t>(responsePayload[0]) << 24) |
                          (static_cast<int32_t>(responsePayload[1]) << 16) |
                          (static_cast<int32_t>(responsePayload[2]) << 8) |
                          static_cast<int32_t>(responsePayload[3])};
        const int32_t lon{(static_cast<int32_t>(responsePayload[4]) << 24) |
                          (static_cast<int32_t>(responsePayload[5]) << 16) |
                          (static_cast<int32_t>(responsePayload[6]) << 8) |
                          static_cast<int32_t>(responsePayload[7])};

        LOG_I("lat: %d, lon: %d", lat, lon);
        
        rdgLoc->setLatitude(lat);
        rdgLoc->setLongitude(lon);
        return rdgLoc;
    }

    LOG_E("Failed to get location from proxy: requestOk=%d payloadSize=%zu",
          requestOk ? 1 : 0,
          responsePayload.size());
    rdgLoc->setLatitude(0x7FFFFFFE);
    rdgLoc->setLongitude(0x7FFFFFFE);
    return rdgLoc;

#if 0
    const sp<LocationData> lLocationData {new LocationData()};
    
    error_t error{E_OK};
    uint8_t isLocationAvailable {0U};
    const android::sp<ILocationManagerService> locMgr{getLocationManagerService()};
    if (locMgr != nullptr) {
        isLocationAvailable = locMgr->IsLocationDataAvailable();
        error = locMgr->getLocationData(lLocationData);
    } else {
        error = E_ERROR;
    }
    
    const int32_t tempLat {static_cast<int32_t>(std::lround(lLocationData->locationData.latitude * 3600.0))}; //degree to second conversion
    const int32_t tempLong {static_cast<int32_t>(std::lround(lLocationData->locationData.longitude * 3600.0))}; //degree to second conversion

    if (DiagManagerAdapter::getInstance()->getLocationUploadConsent() == false)
    {
        LOG_V("Consenst status is false");
        error = E_ERROR;
    }

    if (error != E_OK)    //RDG30-R-1046
    {
        LOG_I("UNDEFINED LOCATION");
        rdgLoc->setLatitude(0x7FFFFFFE);
        rdgLoc->setLongitude(0x7FFFFFFE);
    }
    else if (isLocationAvailable == 0U)
    {
        LOG_I("INVALID LOCATION");
        rdgLoc->setLatitude(0x7FFFFFFF);
        rdgLoc->setLongitude(0x7FFFFFFF);
    }
    else if ((tempLat < -324000) || (tempLat > 324000)) //outside of [-90 deg, 90 deg]
    {
        LOG_I("INVALID LATITUDE");
        rdgLoc->setLatitude(0x7FFFFFFF);
        rdgLoc->setLongitude(0x7FFFFFFF);
    } 
    else if ((tempLong < -648000) || (tempLong > 648000)) //outside of [-180 deg, 180 deg]
    {
        LOG_I("INVALID LONGTITUDE");
        rdgLoc->setLatitude(0x7FFFFFFF);
        rdgLoc->setLongitude(0x7FFFFFFF);
    } 
    else 
    {
        LOG_I("normal location");
        rdgLoc->setLatitude(tempLat * 128); // resolution is 1/128 second
        rdgLoc->setLongitude(tempLong * 128); // resolution is 1/128 second
    }

    LOG_I("latitude = 0x%08X", rdgLoc->getLatitude());
    LOG_I("longitude = 0x%08X", rdgLoc->getLongtitude());
    (void)tempLat;
    (void)tempLong;
    (void)isLocationAvailable;
    return rdgLoc;
#endif
}

}
