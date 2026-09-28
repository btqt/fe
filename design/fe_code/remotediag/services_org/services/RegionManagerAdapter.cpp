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
android::Mutex RegionManagerAdapter::mInstanceLock{};
std::shared_ptr<RegionManagerAdapter> RegionManagerAdapter::getInstance()
{
    if (instance == nullptr)
    {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr)
        {
            instance = std::make_shared<RegionManagerAdapter>();
        }
    }
    return instance;
}

android::sp<IRegionManagerService> RegionManagerAdapter::getService()
{
    return android::interface_cast<IRegionManagerService> (
            android::defaultServiceManager()->getService(
                android::String16("service_layer.RegionManagerService")
                )
            );
}

void RegionManagerAdapter::registerService()
{
    LOG_I("RegionManagerAdapter::registerService");
    mHandler = RemotediagHandler::getInstance();

    mRegionMService = getService();

    if (mRegionMService != nullptr)
    {
        const android::status_t result{android::IInterface::asBinder(mRegionMService)->linkToDeath(mServiceDeathRecipient)};
        if (result != android::OK)
        {
            LOG_E("Cannot register Region Service");
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_REGION_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        }
    }
}

void RegionManagerAdapter::onBinderDied(const android::wp<android::IBinder>& who)
{
    LOG_I("RegionManagerAdapter::onBinderDied");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
    NOTUSED(who);
    mRegionMService = nullptr;
    (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_REGION_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
}

uint8_t RegionManagerAdapter::getNation()
{
    uint8_t region{LGE_REGION::LGE_REGION_NONE};
    const android::sp<IRegionManagerService> regionMgr{getService()};
    if (regionMgr != nullptr) {
        const error_t result{regionMgr->getNation(region)};
        if (result != E_OK) {
            LOG_E("RegionManagerAdapter::getNation fail!");
        }
    }
    return region;
}
}
