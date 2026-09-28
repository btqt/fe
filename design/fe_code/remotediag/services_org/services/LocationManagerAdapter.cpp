#include "LocationManagerAdapter.h"

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
    LOG_I("registerService");
    mHandler = RemotediagHandler::getInstance();
    mLocationservice = getLocationManagerService();
    if (mLocationservice != nullptr) {
        if (android::OK != android::IInterface::asBinder(mLocationservice)->linkToDeath(mServiceDeathRecipient)) {
            LOG_E("Fail to register Loc");
        }
    }
    else {
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_LOCATION_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        }
    }
}

android::sp<ILocationManagerService> LocationManagerAdapter::getLocationManagerService()
{
    return android::interface_cast<ILocationManagerService> (android::defaultServiceManager()->getService(android::String16("service_layer.LocationManagerService")));
}

void LocationManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
    LOG_I("LocationManagerAdapter::onBinderDied");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
    NOTUSED(who);
    mLocationservice = nullptr;
    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_LOCATION_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}

sp<CommonDefine::RDGLocationData> LocationManagerAdapter::getLocationData()
{
    const sp<CommonDefine::RDGLocationData> rdgLoc {new CommonDefine::RDGLocationData()};
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
}

}
