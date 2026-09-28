#include "TimeManagerAdapter.h"

#include <sstream>

#include <services/TimeManagerService/TimeManager.h>

#include "../utils/ProxyIpcClient.h"

TimeManagerAdapter::TimeManagerAdapter() {
    // Create and register IPC command handler
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
}

TimeManagerAdapter::~TimeManagerAdapter() noexcept {
    if (TimeManagerAdapter::instance != nullptr) {
        TimeManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<TimeManagerAdapter> TimeManagerAdapter::instance{nullptr};
android::Mutex TimeManagerAdapter::mInstanceLock{};
std::shared_ptr<TimeManagerAdapter> TimeManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<TimeManagerAdapter>();
        }
    }
    return instance;
}

struct tm TimeManagerAdapter::getCurrentTime() {
    return TimeManager::getInstance().getCurrentTime();
}

int64_t TimeManagerAdapter::getCurrentMilliSec() {
    return TimeManager::getInstance().getCurrentMilliSec();
}

int32_t TimeManagerAdapter::getOffset() {
    return TimeManager::getInstance().getOffset();
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

TimeManagerAdapter::CommandHandler::CommandHandler(TimeManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("TimeManagerAdapter::CommandHandler: created");
}

void TimeManagerAdapter::CommandHandler::initialize() {
    LOG_I("TimeManagerAdapter::CommandHandler: registering with ProxyIpcClient");

    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::TimeGetCurrentTime),
        static_cast<uint32_t>(rdgipc::CommandId::TimeGetCurrentMilliSec),
        static_cast<uint32_t>(rdgipc::CommandId::TimeGetOffset)
    };

    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("TimeManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("TimeManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

TimeManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("TimeManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse TimeManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);

    LOG_I("TimeManagerAdapter::CommandHandler: handling command=%s id=%u payloadSize=%zu",
          rdgipc::commandName(cmdId), commandId, payload.size());

    switch (cmdId) {
    case rdgipc::CommandId::TimeGetCurrentTime:
        return handleGetCurrentTime(payload);
    case rdgipc::CommandId::TimeGetCurrentMilliSec:
        return handleGetCurrentMilliSec(payload);
    case rdgipc::CommandId::TimeGetOffset:
        return handleGetOffset(payload);
    default:
        LOG_W("TimeManagerAdapter::CommandHandler: unsupported command id=%u", commandId);
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse TimeManagerAdapter::CommandHandler::handleGetCurrentTime(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    const struct tm current{mAdapter->getCurrentTime()};
    // Encode as "tm_year,tm_mon,tm_mday,tm_hour,tm_min,tm_sec" (raw struct tm fields)
    std::ostringstream oss{};
    oss << current.tm_year << "," << current.tm_mon << "," << current.tm_mday << ","
        << current.tm_hour << "," << current.tm_min << "," << current.tm_sec;
    return rdgipc::CommandResponse::ok(oss.str());
}

rdgipc::CommandResponse TimeManagerAdapter::CommandHandler::handleGetCurrentMilliSec(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    const int64_t currentMs{mAdapter->getCurrentMilliSec()};
    return rdgipc::CommandResponse::ok(std::to_string(currentMs));
}

rdgipc::CommandResponse TimeManagerAdapter::CommandHandler::handleGetOffset(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    const int32_t offset{mAdapter->getOffset()};
    return rdgipc::CommandResponse::ok(offset);
}
