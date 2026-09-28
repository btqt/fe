#include "TelephonyManagerAdapter.h"

#include <TelephonyManager.hpp>

#include "../utils/ProxyIpcClient.h"

TelephonyManagerAdapter::TelephonyManagerAdapter() {
    // Create and register IPC command handler
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
}

TelephonyManagerAdapter::~TelephonyManagerAdapter() noexcept {
    if (TelephonyManagerAdapter::instance != nullptr) {
        TelephonyManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<TelephonyManagerAdapter> TelephonyManagerAdapter::instance{nullptr};
android::Mutex TelephonyManagerAdapter::mInstanceLock{};
std::shared_ptr<TelephonyManagerAdapter> TelephonyManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<TelephonyManagerAdapter>();
        }
    }
    return instance;
}

std::string TelephonyManagerAdapter::getNetworkTime() {
    return telephony::TelephonyManager::getNetworkTime();
}

std::string TelephonyManagerAdapter::getImei() {
    return telephony::TelephonyManager::getImei();
}

std::string TelephonyManagerAdapter::getEidDefault() {
    return telephony::TelephonyManager::getEid(telephony::TelephonyManager::SlotIdType::DEFAULT_ID);
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

TelephonyManagerAdapter::CommandHandler::CommandHandler(TelephonyManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("TelephonyManagerAdapter::CommandHandler: created");
}

void TelephonyManagerAdapter::CommandHandler::initialize() {
    LOG_I("TelephonyManagerAdapter::CommandHandler: registering with ProxyIpcClient");

    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::TelephonyGetNetworkTime),
        static_cast<uint32_t>(rdgipc::CommandId::TelephonyGetImei),
        static_cast<uint32_t>(rdgipc::CommandId::TelephonyGetEidDefault)
    };

    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("TelephonyManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("TelephonyManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

TelephonyManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("TelephonyManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse TelephonyManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);

    LOG_I("TelephonyManagerAdapter::CommandHandler: handling command=%s id=%u payloadSize=%zu",
          rdgipc::commandName(cmdId), commandId, payload.size());

    switch (cmdId) {
    case rdgipc::CommandId::TelephonyGetNetworkTime:
        return handleGetNetworkTime(payload);
    case rdgipc::CommandId::TelephonyGetImei:
        return handleGetImei(payload);
    case rdgipc::CommandId::TelephonyGetEidDefault:
        return handleGetEidDefault(payload);
    default:
        LOG_W("TelephonyManagerAdapter::CommandHandler: unsupported command id=%u", commandId);
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse TelephonyManagerAdapter::CommandHandler::handleGetNetworkTime(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    return rdgipc::CommandResponse::ok(mAdapter->getNetworkTime());
}

rdgipc::CommandResponse TelephonyManagerAdapter::CommandHandler::handleGetImei(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    return rdgipc::CommandResponse::ok(mAdapter->getImei());
}

rdgipc::CommandResponse TelephonyManagerAdapter::CommandHandler::handleGetEidDefault(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    return rdgipc::CommandResponse::ok(mAdapter->getEidDefault());
}
