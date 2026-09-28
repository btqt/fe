#include "ApplicationManagerAdapter.h"

#include <exception>
#include <vector>
#include <sstream>

#include "utils/ProxyIpcServer.h"

namespace rdgapp {

namespace {

bool parsePostPayload(const std::string &value, int32_t &arg1, std::vector<uint8_t> &bufferBytes) {
    const std::size_t comma{value.find(',')};  
    if (comma == std::string::npos) {
        return false;
    }

    const std::string argStr{value.substr(0U, comma)};
    const std::string hex{value.substr(comma + 1U)};
    arg1 = static_cast<int32_t>(std::strtol(argStr.c_str(), nullptr, 10));
    
    bufferBytes.clear();
    if ((hex.size() % 2U) != 0U) {
        return false;
    }

    bufferBytes.reserve(hex.size() / 2U);
    for (size_t i{0U}; i < hex.size(); i += 2U) {
        uint32_t value{0U};
        std::stringstream ss{};
        ss << std::hex << hex.substr(i, 2U);
        ss >> value;
        bufferBytes.push_back(static_cast<uint8_t>(value & 0xFFU));
    }
    return true;
}

} // namespace

// CallbackHandler implementation
ApplicationManagerAdapter::CallbackHandler::CallbackHandler(ApplicationManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("ApplicationManagerAdapter::CallbackHandler: created");
}

void ApplicationManagerAdapter::CallbackHandler::initialize() {
    // Register this handler for Application callbacks
    std::vector<uint32_t> callbackIds = {
        static_cast<uint32_t>(rdgipc::CallbackId::ApplicationOnBootCompleted),
        static_cast<uint32_t>(rdgipc::CallbackId::ApplicationOnFeatureStatusChanged),
        static_cast<uint32_t>(rdgipc::CallbackId::ApplicationOnFeatureActionDelivered),
        static_cast<uint32_t>(rdgipc::CallbackId::ApplicationPostAppStatusChanged)
    };
    
    rdgapp::ProxyIpcServer& server = rdgapp::ProxyIpcServer::getInstance();
    server.registerCallbackHandler(shared_from_this(), callbackIds);
    LOG_I("ApplicationManagerAdapter::CallbackHandler: registered %zu callbacks", callbackIds.size());
}

void ApplicationManagerAdapter::CallbackHandler::handle(uint32_t callbackId, const std::vector<uint8_t>& payload) {
    LOG_I("ApplicationManagerAdapter::CallbackHandler::handle id=%u payloadSize=%zu",
          callbackId, payload.size());

    const android::sp<RemotediagHandler> handler{mAdapter->mHandler};
    if (handler == nullptr) {
        LOG_W("ApplicationManagerAdapter: callback dropped no handler callbackId=%u", callbackId);
        return;
    }

    if (callbackId == static_cast<uint32_t>(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_BOOT_COMPLETED)) {
        (void)handler->obtainMessage(static_cast<int32_t>(callbackId))->sendToTarget();
        return;
    }

    if ((callbackId == static_cast<uint32_t>(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_STATUS_CHANGED)) ||
        (callbackId == static_cast<uint32_t>(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_ACTION_DELIVERED)) ||
        (callbackId == static_cast<uint32_t>(HANDLE_MESSAGE_REQUEST::MSG_APPL_POST_APP_STATUS_CHANGED))) {
        int32_t arg1{0};
        std::vector<uint8_t> bytes{};
        if (!parsePostPayload(std::string(payload.begin(), payload.end()), arg1, bytes)) {
            LOG_W("ApplicationManagerAdapter: invalid callback payload callbackId=%u", callbackId);
            return;
        }

        const android::sp<Post> post{new Post()};
        post->arg1 = arg1;
        if (!bytes.empty()) {
            post->buffer.setTo(bytes.data(), static_cast<int32_t>(bytes.size()));
        }
        (void)handler->obtainMessage(static_cast<int32_t>(callbackId), post)->sendToTarget();
        return;
    }
}

ApplicationManagerAdapter::ApplicationManagerAdapter() {
    mCallbackHandler = std::make_shared<CallbackHandler>(this);
    mCallbackHandler->initialize();
}

ApplicationManagerAdapter::~ApplicationManagerAdapter() noexcept {
    if(ApplicationManagerAdapter::instance != nullptr) {
        ApplicationManagerAdapter::instance = nullptr;
    }
}
std::shared_ptr<ApplicationManagerAdapter> ApplicationManagerAdapter::instance{nullptr};
android::Mutex ApplicationManagerAdapter::mInstanceLock{};
std::shared_ptr<ApplicationManagerAdapter> ApplicationManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<ApplicationManagerAdapter>();
        }
    }
    return instance;
}

void ApplicationManagerAdapter::registerService() {
    LOG_I("Start register ApplicationManagerAdapter");
    mHandler = RemotediagHandler::getInstance();
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::ApplicationGetBootCompleted, {}, response)) {
        LOG_W("ApplicationManagerAdapter: boot status request through proxy failed");
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(
                mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_APPLICATION_MGR),
                static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
        return;
    }

    const bool isBootCompleted{rdgipc::toString(response) == "1"};
    if (isBootCompleted && (mHandler != nullptr) && !mIsBootCompleted) {
        LOG_I("ApplicationManagerAdapter: proxy reported boot completed");
        mIsBootCompleted = true;
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_BOOT_COMPLETED)->sendToTarget();
    }
}

int32_t ApplicationManagerAdapter::queryActionForFeature(const std::string name) {
    LOG_I("Query action for feature name : %s", name.c_str());
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::ApplicationQueryAction,
                                                       rdgipc::toBytes(name),
                                                       response)) {
        return ParamsDef::UNKNOWN;
    }

    try {
        return std::stoi(rdgipc::toString(response));
    } catch (const std::exception &) {
        LOG_E("Query action response is invalid");
        return ParamsDef::UNKNOWN;
    }
}

int32_t ApplicationManagerAdapter::setFeatureStatus(const std::string appNames, const std::string feaName, const bool onOf) {
    const std::string payload{appNames + "," + feaName + "," + (onOf ? "1" : "0")};
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::ApplicationSetFeatureStatus,
                                                       rdgipc::toBytes(payload),
                                                       response)) {
        return TIGER_ERR::E_ERROR;
    }

    try {
        return std::stoi(rdgipc::toString(response));
    } catch (const std::exception &) {
        LOG_E("Set feature status response is invalid");
        return TIGER_ERR::E_ERROR;
    }
}

FeatureStatus ApplicationManagerAdapter::getFeatureStatus(const std::string name) {
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::ApplicationGetFeatureStatus,
                                                       rdgipc::toBytes(name),
                                                       response)) {
        return FeatureStatus::OFF;
    }

    try {
        const int32_t status{std::stoi(rdgipc::toString(response))};
        return (status == static_cast<int32_t>(FeatureStatus::ON)) ? FeatureStatus::ON : FeatureStatus::OFF;
    } catch (const std::exception &) {
        LOG_E("Get feature status response is invalid");
        return FeatureStatus::OFF;
    }
}
}
