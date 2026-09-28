#include "RegionManagerAdapter.h"

namespace rdgapp {

RegionManagerAdapter::RegionManagerAdapter()
{
    mServiceDeathRecipient = new ServiceDeathRecipient( [this] ( const android::wp<android::IBinder>& who ){
        this->onBinderDied(who);
    });
}

RegionManagerAdapter::~RegionManagerAdapter()
{
    mRegionMService = nullptr;
    mServiceDeathRecipient = nullptr;

    if(RegionManagerAdapter::instance != nullptr) {
        RegionManagerAdapter::instance = nullptr;
    }   
}

std::shared_ptr<RegionManagerAdapter> RegionManagerAdapter::instance{nullptr};
std::shared_ptr<RegionManagerAdapter> RegionManagerAdapter::getInstance()
{
    if (instance == nullptr)
    {
        instance = std::make_shared<RegionManagerAdapter>();
    }
    return instance;
}

android::sp<IRegionManagerService> RegionManagerAdapter::getService()
{
    mRegionMService = android::interface_cast<IRegionManagerService> (
            android::defaultServiceManager()->getService(
                android::String16("service_layer.RegionManagerService")
                )
            );
    return mRegionMService;
}

void RegionManagerAdapter::registerService()
{
    LOG_I("RegionManagerAdapter::registerService");
    mHandler = RemotediagHandler::getInstance_2();

    if (mRegionMService != nullptr)
    {
        mRegionMService = nullptr;
    }

    (void)getService();

    if (mRegionMService != nullptr)
    {
        const android::status_t result{android::IInterface::asBinder(mRegionMService)->linkToDeath(mServiceDeathRecipient)};
        if (result != android::OK)
        {
            LOG_E("Cannot register RegionM Service, try again after ms: %d", RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_REGION_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        }
    }
}

void RegionManagerAdapter::onBinderDied(const android::wp<android::IBinder>& who)
{
    LOG_I("RegionManagerAdapter::onBinderDied");
    NOTUSED(who);
    mRegionMService = nullptr;
    (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_REGION_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
}

uint8_t RegionManagerAdapter::getNation()
{
    uint8_t region{LGE_REGION::LGE_REGION_NONE};
    if (mRegionMService != nullptr) {
        const error_t result{mRegionMService->getNation(region)};
        if (result != E_OK) {
            LOG_E("RegionManagerAdapter::getNation fail!");
        }
    }
    return region;
}
}
