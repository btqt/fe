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
std::shared_ptr<LocationManagerAdapter> LocationManagerAdapter::getInstance() {
    if (instance == nullptr) {
        instance = std::make_shared<LocationManagerAdapter>();
    }
    return instance;
}

void LocationManagerAdapter::registerService() {
    LOG_I("registerService");
    mHandler = RemotediagHandler::getInstance();
    if (mLocationservice != nullptr) {
        LOG_E("mLocationService nullptr");
        mLocationservice = nullptr;
    }

    mLocationservice = android::interface_cast<ILocationManagerService> (android::defaultServiceManager()->getService(android::String16("service_layer.LocationManagerService")));
    if (mLocationservice != nullptr) {
        if (android::OK == android::IInterface::asBinder(mLocationservice)->linkToDeath(mServiceDeathRecipient)) {
            if (mLocationservice->IsLocationStarted() == static_cast<uint8_t>(false))
            {
                (void)mLocationservice->setLocationStatus(LOCATION_ENABLE);
            }
            else
            {
                LOG_I("Location already enable");
            }
        }
    }
    else {
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_LOCATION_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        }
    }
}

android::sp<ILocationManagerService> LocationManagerAdapter::getLocationManagerService() {
    if (mLocationservice == nullptr) {
        mLocationservice = android::interface_cast<ILocationManagerService> (android::defaultServiceManager()->getService(android::String16("service_layer.LocationManagerService")));
    }
    return mLocationservice;
}

void LocationManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
    LOG_I("");
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
    const int32_t tempLat {std::lround(lLocationData->locationData.latitude * 3600.0)}; //degree to second conversion
    const int32_t tempLong {std::lround(lLocationData->locationData.longitude * 3600.0)}; //degree to second conversion
    error_t error{E_OK};

    if (mLocationservice != nullptr) {
        error = mLocationservice->getLocationData(lLocationData);
    } else {
        error = E_ERROR;
    }

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
    } else if ((tempLat < -324000) || (tempLat > 324000)) //outside of [-90 deg, 90 deg]
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
        rdgLoc->setLatitude(tempLat);
        rdgLoc->setLongitude(tempLong);
    }

    LOG_I("latitude = 0x%08X", rdgLoc->getLatitude());
    LOG_I("longitude = 0x%08X", rdgLoc->getLongtitude());
    (void)tempLat;
    (void)tempLong;
    return rdgLoc;
}

}
