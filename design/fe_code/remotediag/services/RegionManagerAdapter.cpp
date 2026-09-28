#include "RegionManagerAdapter.h"
#include "../utils/ProxyIpcServer.h"
#include "../remotediagproxy/include/ProxyIpcProtocol.h"

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
}

void RegionManagerAdapter::onBinderDied(const android::wp<android::IBinder>& who)
{
    LOG_I("RegionManagerAdapter::onBinderDied (no-op, service access is via proxy)");
    NOTUSED(who);
}

uint8_t RegionManagerAdapter::getNation()
{
    uint8_t region{LGE_REGION::LGE_REGION_NONE};

    std::vector<uint8_t> reponsePayload{};
    const bool requestOK{ProxyIpcServer::getInstance().requestAPICall(
        rdgipc::CommandId::RegionGetNation,
        {},
        reponsePayload,
        5000U)
    };

    if (requestOK && reponsePayload.size() >= 1U) {
        region = reponsePayload[0];
        LOG_I("RegionManagerAdapter::getNation from proxy: %u", region);
        return region;
    }

    LOG_E("fail to get nation from proxy, request=%d payloadSize=%zu",
        requestOK ? 1 : 0,
        reponsePayload.size());

    return region;
#if 0
    const android::sp<IRegionManagerService> regionMgr{getService()};
    if (regionMgr != nullptr) {
        const error_t result{regionMgr->getNation(region)};
        if (result != E_OK) {
            LOG_E("RegionManagerAdapter::getNation fail!");
        }
    }
    return region;
#endif
}
}
