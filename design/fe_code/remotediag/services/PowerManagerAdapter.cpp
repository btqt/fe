#include "PowerManagerAdapter.h"

#include <exception>
#include <sstream>

namespace rdgapp {

PowerManagerAdapter::PowerManagerAdapter() {
    // Create and register IPC callback handler
    mCallbackHandler = std::make_shared<CallbackHandler>(this);
    mCallbackHandler->initialize();
}

PowerManagerAdapter::~PowerManagerAdapter() noexcept {
    if(PowerManagerAdapter::instance != nullptr) {
        PowerManagerAdapter::instance = nullptr;
    } 
}

std::shared_ptr<PowerManagerAdapter> PowerManagerAdapter::instance{nullptr};
android::Mutex PowerManagerAdapter::mInstanceLock{};
std::shared_ptr<PowerManagerAdapter> PowerManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<PowerManagerAdapter>();
        }
    }
    return instance;
}

void PowerManagerAdapter::registerService(void)
{
    mHandler = RemotediagHandler::getInstance();
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::PowerGetIgnitionStatus, {}, response)) {
        LOG_W("PowerManagerAdapter register through proxy failed");
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_POWER_MODE_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        } else {
            LOG_E("mHandler is nullptr");
        }
    }
}

void PowerManagerAdapter::onPowerStateChanged(const int32_t newState, const int32_t reason) {
    //TBD
    LOG_I("newState: %d reason: %d",newState, reason);
    if (newState == POWER_STATE_WAITING_SHUTDOWN) {
        LOG_I("PowerMng will be reboot in next few second!");
        // MSG_PREPARE_TO_SHUTDOWN
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_PREPARE_TO_SHUTDOWN)->sendToTarget();
    }
}
void PowerManagerAdapter::onErrControlPower(const int32_t err_reason, const int32_t errPowerID, const int32_t currPowerID) {
    //TBD
    LOG_D("err_reason: %d errPowerID: %d currPowerID: %d",err_reason, errPowerID, currPowerID);
}
void PowerManagerAdapter::onPowerModeChanged(const int32_t newMode) {
    LOG_I("onPowerModeChanged newMode = %d", newMode);    
}

void PowerManagerAdapter::onExtValueChanged(const int32_t listenIndex, const int32_t value) {
    switch (listenIndex)
    {
        case POWER_IDX::LISTEN_INDEX_MCU_STATUS_IG:
        {
            LOG_D("POWER_IDX::LISTEN_INDEX_MCU_STATUS_IG");
            if ((value >= static_cast<int32_t>(IG_STATUS::IG_STATUS_UNDECIDED)) && (value < static_cast<int32_t>(IG_STATUS::IG_STATUS_MAX)))
            {
                IGN_changedHandler(static_cast<IG_STATUS>(value));
            }
            break;
        }
        case POWER_IDX::LISTEN_INDEX_POWER_SOURCE:
        {
            LOG_I("Power source change to: %s", value == POWER_SOURCE::POWER_SOURCE_BUB ? "POWER_SOURCE_BUB" : "POWER_SOURCE_MAIN");
    
            if (value == static_cast<int32_t>(POWER_SOURCE::POWER_SOURCE_BUB))
            { /* POWER_SOURCE_BUB */
                bubTrigger(true);
            }
            else if (value == static_cast<int32_t>(POWER_SOURCE::POWER_SOURCE_MAIN))
            {
                bubTrigger(false);
            }
            else
            {
                LOG_I("Power source is invalid");
            }
            break;
        }
        default:
            break;
    }
}

void PowerManagerAdapter::onPowerLockRelease() {
    //TBD
    LOG_I("onPowerLockRelease");
}

void PowerManagerAdapter::acquirePowerLock() {
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::PowerAcquireLock, {}, response)) {
        LOG_W("Power acquire lock through proxy failed");
    }
}

void PowerManagerAdapter::releasePowerLock() {
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::PowerReleaseLock, {}, response)) {
        LOG_W("Power release lock through proxy failed");
    }
}

void PowerManagerAdapter::IGN_changedHandler(const IG_STATUS status)
{
    LOG_I("IG status changed to %s", status == IG_STATUS::IG_STATUS_ON ? "IGN_ON" : "IGN_OFF");

    if (mHandler != nullptr)
    {
        switch (status) 
        {
            case IG_STATUS::IG_STATUS_ON:
                (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_ON)->sendToTarget();
                break;
            case IG_STATUS::IG_STATUS_OFF:
                (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_OFF)->sendToTarget();
                break;
            default:
                LOG_W("IG unknow status, value = %d", static_cast<int32_t>(status));
                break;
        }
    }
    else {
        // do nothing
        LOG_E("mHandler is nullptr");
    }
}

IG_STATUS PowerManagerAdapter::getIgnitionStatus(void)
{
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::PowerGetIgnitionStatus, {}, response)) {
        LOG_E("Power ignition request through proxy failed");
        return IG_STATUS::IG_STATUS_MAX;
    }

    try {
        const int32_t parsed{std::stoi(rdgipc::toString(response))};
        if ((parsed >= static_cast<int32_t>(IG_STATUS::IG_STATUS_UNDECIDED)) &&
            (parsed <= static_cast<int32_t>(IG_STATUS::IG_STATUS_MAX))) {
            return static_cast<IG_STATUS>(parsed);
        }
    } catch (const std::exception &) {
        LOG_E("Power ignition response is invalid");
    }

    return IG_STATUS::IG_STATUS_MAX;
}

void PowerManagerAdapter::bubTrigger(const bool value)
{
    LOG_I("BUB Trigger: %s", value ? "ON" : "OFF");
    if (mCurBubStatus != value)
    {
        mCurBubStatus = value;
        if (mHandler != nullptr)
        {
            int32_t message{0};
            if (mCurBubStatus)
            {
                message = HANDLE_MESSAGE_REQUEST::HANDLE_MSG_BUB_ON;
            }
            else
            {
                message = HANDLE_MESSAGE_REQUEST::HANDLE_MSG_BUB_OFF;
            }
            (void)mHandler->obtainMessage(message)->sendToTarget();
        }
        else
        {
            LOG_E("mHandler is nullptr");
        }
    }
    else
    {
        LOG_I("BUB status unchanged");
    }
}

// ============================================================================
// NESTED CallbackHandler IMPLEMENTATION
// ============================================================================

PowerManagerAdapter::CallbackHandler::CallbackHandler(PowerManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("PowerManagerAdapter::CallbackHandler: created");
}

void PowerManagerAdapter::CallbackHandler::initialize() {    
    // Register with the specific callback IDs this handler supports
    std::vector<uint32_t> callbackIds = {
        static_cast<uint32_t>(rdgipc::CallbackId::PowerStateChanged),
        static_cast<uint32_t>(rdgipc::CallbackId::PowerErrControl),
        static_cast<uint32_t>(rdgipc::CallbackId::PowerModeChanged),
        static_cast<uint32_t>(rdgipc::CallbackId::PowerExtValueChanged),
        static_cast<uint32_t>(rdgipc::CallbackId::PowerLockReleased)
    };
    
    rdgapp::ProxyIpcServer& server = rdgapp::ProxyIpcServer::getInstance();
    server.registerCallbackHandler(shared_from_this(), callbackIds);
    LOG_I("PowerManagerAdapter::CallbackHandler: registered %zu callbacks", callbackIds.size());
}

PowerManagerAdapter::CallbackHandler::~CallbackHandler() {
    LOG_I("PowerManagerAdapter::CallbackHandler: destroyed");
}

void PowerManagerAdapter::CallbackHandler::handle(uint32_t callbackId, const std::vector<uint8_t> &payload) {
    const rdgipc::CallbackId cbId = static_cast<rdgipc::CallbackId>(callbackId);
    
    LOG_I("PowerManagerAdapter::CallbackHandler: handling callback=%s id=%u payloadSize=%zu",
          rdgipc::callbackName(cbId), callbackId, payload.size());
    
    switch (cbId) {
    case rdgipc::CallbackId::PowerStateChanged:
        handlePowerStateChanged(payload);
        break;
    case rdgipc::CallbackId::PowerErrControl:
        handlePowerErrControl(payload);
        break;
    case rdgipc::CallbackId::PowerModeChanged:
        handlePowerModeChanged(payload);
        break;
    case rdgipc::CallbackId::PowerExtValueChanged:
        handlePowerExtValueChanged(payload);
        break;
    case rdgipc::CallbackId::PowerLockReleased:
        handlePowerLockReleased(payload);
        break;
    default:
        LOG_W("PowerManagerAdapter::CallbackHandler: unsupported callback id=%u", callbackId);
        break;
    }
}

void PowerManagerAdapter::CallbackHandler::handlePowerStateChanged(const std::vector<uint8_t> &payload) {
    const std::string payloadStr = rdgipc::toString(payload);
    std::vector<int32_t> values;
    
    if (!parseIntListPayload(payloadStr, values) || (values.size() != 2U)) {
        LOG_E("PowerManagerAdapter::CallbackHandler: invalid power state payload");
        return;
    }
    
    const int32_t state = values[0];
    const int32_t value = values[1];
    mAdapter->onPowerStateChanged(state, value);
}

void PowerManagerAdapter::CallbackHandler::handlePowerErrControl(const std::vector<uint8_t> &payload) {
    const std::string payloadStr = rdgipc::toString(payload);
    std::vector<int32_t> values;
    
    if (!parseIntListPayload(payloadStr, values) || (values.size() != 3U)) {
        LOG_E("PowerManagerAdapter::CallbackHandler: invalid power error payload");
        return;
    }
    
    const int32_t error = values[0];
    const int32_t type = values[1];
    const int32_t value = values[2];
    PowerManagerAdapter::onErrControlPower(error, type, value);
}

void PowerManagerAdapter::CallbackHandler::handlePowerModeChanged(const std::vector<uint8_t> &payload) {
    const std::string payloadStr = rdgipc::toString(payload);
    std::vector<int32_t> values;
    
    if (!parseIntListPayload(payloadStr, values) || (values.size() != 1U)) {
        LOG_E("PowerManagerAdapter::CallbackHandler: invalid power mode payload");
        return;
    }
    
    const int32_t mode = values[0];
    PowerManagerAdapter::onPowerModeChanged(mode);
}

void PowerManagerAdapter::CallbackHandler::handlePowerExtValueChanged(const std::vector<uint8_t> &payload) {
    const std::string payloadStr = rdgipc::toString(payload);
    std::vector<int32_t> values;
    
    if (!parseIntListPayload(payloadStr, values) || (values.size() != 2U)) {
        LOG_E("PowerManagerAdapter::CallbackHandler: invalid ext value payload");
        return;
    }
    
    const int32_t type = values[0];
    const int32_t value = values[1];
    mAdapter->onExtValueChanged(type, value);
}

void PowerManagerAdapter::CallbackHandler::handlePowerLockReleased(const std::vector<uint8_t> &payload) {
    PowerManagerAdapter::onPowerLockRelease();
}

bool PowerManagerAdapter::CallbackHandler::parseIntListPayload(const std::string &value, std::vector<int32_t> &numbers) {
    numbers.clear();
    if (value.empty()) {
        return false;
    }
    
    std::stringstream ss(value);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) {
            return false;
        }
        char *endPtr = nullptr;
        const long parsed = std::strtol(token.c_str(), &endPtr, 10);
        if ((endPtr == nullptr) || (*endPtr != '\0')) {
            return false;
        }
        numbers.push_back(static_cast<int32_t>(parsed));
    }
    return !numbers.empty();
}

}
