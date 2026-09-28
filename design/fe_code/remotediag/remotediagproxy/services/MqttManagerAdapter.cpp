#include "MqttManagerAdapter.h"

#include <binder/Parcel.h>
#include <cstring>

#include "../utils/ProxyIpcClient.h"

namespace {
constexpr size_t kVinLength{17U};
constexpr const char *kMqttServiceName{"service_layer.DcemqttproxyManagerService"};
constexpr const char *kMqttTopicSuffix{"/C2V/DESTSW/remotediagnostics/DCIF-RDG015"};
} // namespace

std::shared_ptr<MqttManagerAdapter> MqttManagerAdapter::instance{nullptr};
android::Mutex MqttManagerAdapter::mInstanceLock{};

MqttManagerAdapter::MqttManagerAdapter() {
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who) {
        this->onBinderDied(who);
    });
    
    // Create and register IPC command handler
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
}

MqttManagerAdapter::~MqttManagerAdapter() noexcept {
    if (MqttManagerAdapter::instance != nullptr) {
        MqttManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<MqttManagerAdapter> MqttManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<MqttManagerAdapter>();
        }
    }
    return instance;
}

void MqttManagerAdapter::registerService() {
    LOG_I("MqttManagerAdapter registerService");
    mHandler = RemotediagProxyHandler::getInstance();

    if (!registerServiceLocked()) {
        LOG_W("MqttManagerAdapter: register service failed, retry");
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(
                mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_MQTT_MGR),
                static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        } else {
            LOG_E("MqttManagerAdapter: mHandler is nullptr");
        }
    }
}

bool MqttManagerAdapter::registerServiceLocked() {
    android::sp<IDcemqttproxyManagerService> service{getService()};
    if (service == nullptr) {
        LOG_W("MqttManagerAdapter: DcemqttproxyManagerService not available");
        return false;
    }

    const android::status_t linkResult{android::IInterface::asBinder(service)->linkToDeath(mServiceDeathRecipient)};
    if (linkResult != android::OK) {
        LOG_E("MqttManagerAdapter: linkToDeath failed: %d", static_cast<int32_t>(linkResult));
        return false;
    }

    if (mReceiver == nullptr) {
        mReceiver = android::sp<MqttNotifyReceiver>{new MqttNotifyReceiver(*this)};
    }

    const error_t registerResult{service->registerReceiverDcemqttproxyOnNotifyCb(mReceiver, APP_NAME)};
    if (registerResult != E_OK) {
        LOG_E("MqttManagerAdapter: registerReceiverDcemqttproxyOnNotifyCb failed: %d",
              static_cast<int32_t>(registerResult));
        return false;
    }

    {
        const android::Mutex::Autolock lock{mServiceLock};
        mDcemqttproxy = service;
    }

    LOG_I("MqttManagerAdapter: registered successfully");
    return true;
}

bool MqttManagerAdapter::subscribeTopic(const std::string &vinNum) {
    if (vinNum.size() != kVinLength) {
        LOG_W("MqttManagerAdapter: invalid VIN length=%zu", vinNum.size());
        return false;
    }

    android::sp<IDcemqttproxyManagerService> service{nullptr};
    {
        const android::Mutex::Autolock lock{mServiceLock};
        service = mDcemqttproxy;
    }

    if (service == nullptr) {
        LOG_W("MqttManagerAdapter: subscribeTopic called while service unavailable");
        return false;
    }

    std::vector<std::string> topicList{};
    topicList.push_back(vinNum + kMqttTopicSuffix);
    const error_t invokeResult{service->invokeSubscribeAdd(APP_NAME, topicList)};
    if (invokeResult != E_OK) {
        LOG_E("MqttManagerAdapter: invokeSubscribeAdd failed: %d", static_cast<int32_t>(invokeResult));
        return false;
    }

    LOG_I("MqttManagerAdapter: subscribeTopic success vin=%s", vinNum.c_str());
    return true;
}

void MqttManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
    NOTUSED(who);
    LOG_I("MqttManagerAdapter::onBinderDied");
    {
        const android::Mutex::Autolock lock{mServiceLock};
        mDcemqttproxy = nullptr;
        mReceiver = nullptr;
    }

    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(
            mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_MQTT_MGR),
            static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
    }
}

void MqttManagerAdapter::onNotifyReceived(const android::sp<DceNotification> &message) {
    if (message == nullptr) {
        LOG_E("MqttManagerAdapter: onNotifyReceived called with nullptr");
        return;
    }

    android::Parcel parcel{};
    if (message->writeToParcel(&parcel) != E_OK) {
        LOG_E("MqttManagerAdapter: failed to serialize DceNotification");
        return;
    }

    std::vector<uint8_t> payload(parcel.dataSize());
    if (parcel.dataSize() > 0) {
        std::memcpy(payload.data(), parcel.data(), parcel.dataSize());
    }

    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::MqttOnNotifyReceived), payload);
}

android::sp<IDcemqttproxyManagerService> MqttManagerAdapter::getService() const {
    return android::interface_cast<IDcemqttproxyManagerService>(
        android::defaultServiceManager()->getService(android::String16(kMqttServiceName)));
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

MqttManagerAdapter::CommandHandler::CommandHandler(MqttManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("MqttManagerAdapter::CommandHandler: created");
}

void MqttManagerAdapter::CommandHandler::initialize() {
    LOG_I("MqttManagerAdapter::CommandHandler: registering with ProxyIpcClient");
    
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::MqttSubscribeTopic)
    };
    
    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("MqttManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("MqttManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

MqttManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("MqttManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse MqttManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);
    
    LOG_I("MqttManagerAdapter::CommandHandler: handling command=%s id=%u payloadSize=%zu",
          rdgipc::commandName(cmdId), commandId, payload.size());
    
    switch (cmdId) {
    case rdgipc::CommandId::MqttSubscribeTopic:
        return handleSubscribeTopic(payload);
    default:
        LOG_W("MqttManagerAdapter::CommandHandler: unsupported command id=%u", commandId);
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse MqttManagerAdapter::CommandHandler::handleSubscribeTopic(const std::vector<uint8_t> &payload) {
    const std::string vinNum = rdgipc::toString(payload);
    const bool success = mAdapter->subscribeTopic(vinNum);
    return success ? rdgipc::CommandResponse::ok() : rdgipc::CommandResponse::err(rdgipc::ErrorCode::OperationFailed);
}
