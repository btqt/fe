#include "MqttManagerAdapter.h"

#include <services/DcemqttproxyManagerService/DceNotification.h>
#include <binder/Parcel.h>
#include "services/DiagManagerAdapter.h"
#include "utils/ProxyIpcServer.h"

namespace rdgapp {

namespace {
constexpr uint32_t kMqttSubscribeTimeoutMs{2000U};
constexpr size_t kVinLength{17U};
} // namespace

// CallbackHandler implementation
MqttManagerAdapter::CallbackHandler::CallbackHandler(MqttManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("MqttManagerAdapter::CallbackHandler: created");
}

void MqttManagerAdapter::CallbackHandler::initialize() {
    // Register this handler for MQTT callbacks
    std::vector<uint32_t> callbackIds = {
        static_cast<uint32_t>(rdgipc::CallbackId::MqttOnNotifyReceived)
    };
    
    rdgapp::ProxyIpcServer& server = rdgapp::ProxyIpcServer::getInstance();
    server.registerCallbackHandler(shared_from_this(), callbackIds);
    LOG_I("MqttManagerAdapter::CallbackHandler: registered %zu callbacks", callbackIds.size());
}

void MqttManagerAdapter::CallbackHandler::handle(uint32_t callbackId, const std::vector<uint8_t>& payload) {
    LOG_I("MqttManagerAdapter::CallbackHandler::handle id=%u payloadSize=%zu",
          callbackId, payload.size());

    const android::sp<RemotediagHandler> handler{mAdapter->mHandler};
    if (handler == nullptr) {
        LOG_W("MqttManagerAdapter: callback dropped no handler callbackId=%u", callbackId);
        return;
    }

    if (callbackId == static_cast<uint32_t>(rdgipc::CallbackId::MqttOnNotifyReceived)) {
        if (payload.empty()) {
            LOG_W("MqttManagerAdapter: empty MQTT notify callback payload");
            return;
        }

        android::Parcel parcel{};
        parcel.setData(payload.data(), payload.size());
        android::sp<DceNotification> dataEvent{new DceNotification()};
        if ((dataEvent == nullptr) || (dataEvent->readFromParcel(parcel) != E_OK)) {
            LOG_E("MqttManagerAdapter: failed to deserialize MQTT notify payload");
            return;
        }
        (void)handler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_CENTER_PUSH_RECEIVED, dataEvent)->sendToTarget();
        return;
    }
}

std::shared_ptr<MqttManagerAdapter> MqttManagerAdapter::instance{nullptr};
android::Mutex MqttManagerAdapter::mInstanceLock{};
MqttManagerAdapter::MqttManagerAdapter() {
    mCallbackHandler = std::make_shared<CallbackHandler>(this);
    mCallbackHandler->initialize();
}

MqttManagerAdapter::~MqttManagerAdapter()
{
    if (MqttManagerAdapter::instance != nullptr)
    {
        instance = nullptr;
    }
}

std::shared_ptr<MqttManagerAdapter> MqttManagerAdapter::getInstance()
{
    if (instance == nullptr)
    {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr)
        {
            instance = std::make_shared<MqttManagerAdapter>();
        }
    }
    return instance;
}

void MqttManagerAdapter::registerService()
{
    LOG_I("MqttManagerAdapter::registerService");
    mHandler = RemotediagHandler::getInstance();
    const std::string vinNum{DiagManagerAdapter::getInstance()->getVinNumber()};
    subscribeTopic(vinNum);
}

void MqttManagerAdapter::subscribeTopic(const std::string vinNum)
{
    if (vinNum.size() != kVinLength)
    {
        LOG_W("MqttManagerAdapter::subscribeTopic invalid vin length=%zu", vinNum.size());
        return;
    }

    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::MqttSubscribeTopic,
                                                       rdgipc::toBytes(vinNum),
                                                       response,
                                                       kMqttSubscribeTimeoutMs))
    {
        LOG_E("MqttManagerAdapter::subscribeTopic request through proxy failed");
        return;
    }

    LOG_D("MqttManagerAdapter::subscribeTopic success vin=%s", vinNum.c_str());
}
}
