#include "RegionManagerAdapter.h"

#include "../utils/ProxyIpcClient.h"

RegionManagerAdapter::RegionManagerAdapter()
{
    mServiceDeathRecipient = new ServiceDeathRecipient( [this] ( const android::wp<android::IBinder>& who ){
        this->onBinderDied(who);
    });
    
    // Create and register IPC command handler
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
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

android::sp<IRegionManagerService> RegionManagerAdapter::getRegionManagerService()
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
    mHandler = RemotediagProxyHandler::getInstance();

    mRegionMService = getRegionManagerService();

    if (mRegionMService != nullptr)
    {
        const android::status_t result{android::IInterface::asBinder(mRegionMService)->linkToDeath(mServiceDeathRecipient)};
        if (result != android::OK)
        {
            LOG_E("Cannot register Region Service");
            if (mHandler != nullptr)
            {
                (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_REGION_MGR),
                                                   RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
            }
        }
    }
    else
    {
        if (mHandler != nullptr)
        {
            LOG_E("RegionManagerService is null, retrying registration");
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_REGION_MGR),
                                               RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        }
    }
}

void RegionManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who)
{
    LOG_I("RegionManagerAdapter::onBinderDied");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
    NOTUSED(who);
    mRegionMService = nullptr;
    if (mHandler != nullptr)
    {
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_REGION_MGR),
                                           RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}

uint8_t RegionManagerAdapter::getNation()
{
    uint8_t region{LGE_REGION::LGE_REGION_NONE};

    android::sp<IRegionManagerService> regionMgr{getRegionManagerService()};

    if (regionMgr == nullptr) {
        regionMgr = getRegionManagerService();
        mRegionMService = regionMgr;
    }

    if (regionMgr != nullptr) {
        const error_t result{regionMgr->getNation(region)};
        if (result != E_OK) {
            LOG_E("Proxy RegionManagerAdapter::getNation fail!");
        }
    }
    else
    {
        LOG_E("Proxy RegionManagerAdapter::getNation - RegionManagerService is null");
    }

    return region;
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

RegionManagerAdapter::CommandHandler::CommandHandler(RegionManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("RegionManagerAdapter::CommandHandler: created");
}

void RegionManagerAdapter::CommandHandler::initialize() {
    LOG_I("RegionManagerAdapter::CommandHandler: registering with ProxyIpcClient");
    
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::RegionGetNation)
    };
    
    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("RegionManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("RegionManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

RegionManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("RegionManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse RegionManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);
    
    LOG_I("RegionManagerAdapter::CommandHandler: handling command=%s id=%u payloadSize=%zu",
          rdgipc::commandName(cmdId), commandId, payload.size());
    
    switch (cmdId) {
    case rdgipc::CommandId::RegionGetNation:
        return handleGetNation(payload);
    default:
        LOG_W("RegionManagerAdapter::CommandHandler: unsupported command id=%u", commandId);
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse RegionManagerAdapter::CommandHandler::handleGetNation(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    const uint8_t nation = mAdapter->getNation();
    return rdgipc::CommandResponse::ok(nation);
}
