#include "LocationManagerAdapter.h"

#include "../utils/ProxyIpcClient.h"
#include <cmath>

LocationManagerAdapter::LocationManagerAdapter()
{
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who) {
        this->onBinderDied(who);
    });
    
    // Create and register IPC command handler
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
}

LocationManagerAdapter::~LocationManagerAdapter() noexcept
{
    if (LocationManagerAdapter::instance != nullptr)
    {
        LocationManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<LocationManagerAdapter> LocationManagerAdapter::instance{nullptr};
android::Mutex LocationManagerAdapter::mInstanceLock{};
std::shared_ptr<LocationManagerAdapter> LocationManagerAdapter::getInstance()
{
    if (instance == nullptr)
    {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr)
        {
            instance = std::make_shared<LocationManagerAdapter>();
        }
    }
    return instance;
}

void LocationManagerAdapter::registerService()
{
    LOG_I("registerService");
    mHandler = RemotediagProxyHandler::getInstance();
    mLocationservice = getLocationManagerService();
    if (mLocationservice != nullptr)
    {
        if (android::OK != android::IInterface::asBinder(mLocationservice)->linkToDeath(mServiceDeathRecipient))
        {
            LOG_E("Fail to register Loc");
        }
    }
    else {
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_LOCATION_MGR),
                                            RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        }
    }
}

android::sp<ILocationManagerService> LocationManagerAdapter::getLocationManagerService()
{
    return android::interface_cast<ILocationManagerService>(
        android::defaultServiceManager()->getService(android::String16("service_layer.LocationManagerService")));
}

void LocationManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who)
{
    LOG_I("LocationManagerAdapter::onBinderDied");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
    NOTUSED(who);
    mLocationservice = nullptr;
    if (mHandler != nullptr)
    {
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_LOCATION_MGR),
                                           RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}

bool LocationManagerAdapter::getLocationData(int32_t &latitude, int32_t &longitude)
{
    const sp<LocationData> locationData{new LocationData()};

    error_t error{E_OK};
    uint8_t isLocationAvailable{0U};
    const android::sp<ILocationManagerService> locMgr{getLocationManagerService()};
    if (locMgr != nullptr)
    {
        isLocationAvailable = locMgr->IsLocationDataAvailable();
        error = locMgr->getLocationData(locationData);
    }
    else
    {
        error = E_ERROR;
    }

    const int32_t tempLat{static_cast<int32_t>(std::lround(locationData->locationData.latitude * 3600.0))};
    const int32_t tempLong{static_cast<int32_t>(std::lround(locationData->locationData.longitude * 3600.0))};

    if (error != E_OK)
    {
        latitude = 0x7FFFFFFE;
        longitude = 0x7FFFFFFE;
    }
    else if (isLocationAvailable == 0U)
    {
        latitude = 0x7FFFFFFF;
        longitude = 0x7FFFFFFF;
    }
    else if ((tempLat < -324000) || (tempLat > 324000))
    {
        latitude = 0x7FFFFFFF;
        longitude = 0x7FFFFFFF;
    }
    else if ((tempLong < -648000) || (tempLong > 648000))
    {
        latitude = 0x7FFFFFFF;
        longitude = 0x7FFFFFFF;
    }
    else
    {
        latitude = tempLat * 128;
        longitude = tempLong * 128;
    }

    return true;
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

LocationManagerAdapter::CommandHandler::CommandHandler(LocationManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("LocationManagerAdapter::CommandHandler: created");
}

void LocationManagerAdapter::CommandHandler::initialize() {
    LOG_I("LocationManagerAdapter::CommandHandler: registering with ProxyIpcClient");
    
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::LocationGetLocation)
    };
    
    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("LocationManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("LocationManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

LocationManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("LocationManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse LocationManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);
    
    LOG_I("LocationManagerAdapter::CommandHandler: handling command=%s id=%u payloadSize=%zu",
          rdgipc::commandName(cmdId), commandId, payload.size());
    
    switch (cmdId) {
    case rdgipc::CommandId::LocationGetLocation:
        return handleGetLocation(payload);
    default:
        LOG_W("LocationManagerAdapter::CommandHandler: unsupported command id=%u", commandId);
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse LocationManagerAdapter::CommandHandler::handleGetLocation(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    int32_t latitude = 0;
    int32_t longitude = 0;
    const bool success = mAdapter->getLocationData(latitude, longitude);
    
    if (!success) {
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::OperationFailed);
    }
    
    // Format response as "latitude,longitude"
    std::string response = std::to_string(latitude) + "," + std::to_string(longitude);
    return rdgipc::CommandResponse::ok(response);
}
