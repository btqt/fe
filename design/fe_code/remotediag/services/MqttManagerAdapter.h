#ifndef REMOTEDIAG_REG_ADAPTER_MQTT_MANAGER_H
#define REMOTEDIAG_REG_ADAPTER_MQTT_MANAGER_H

#include <memory>
#include <string>
#include <utils/Mutex.h>

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../include/ParamsDef.h"
#include "../remotediagproxy/include/ProxyIpcProtocol.h"
#include "../remotediagproxy/include/IpcMessageHandler.h"

namespace rdgapp {

class RemotediagHandler;

class MqttManagerAdapter
{
public:
    MqttManagerAdapter();
    ~MqttManagerAdapter();
    MqttManagerAdapter(MqttManagerAdapter const &) = delete;
    MqttManagerAdapter &operator=(MqttManagerAdapter const &) = delete;
    MqttManagerAdapter(MqttManagerAdapter &&) = delete;
    MqttManagerAdapter &operator=(MqttManagerAdapter &&) = delete;
    static std::shared_ptr<MqttManagerAdapter> getInstance();

    void registerService();
    void subscribeTopic(const std::string vinNum);

private:
    // Nested CallbackHandler for self-registering callback handling
    class CallbackHandler : public rdgipc::ICallbackHandler,
                           public std::enable_shared_from_this<CallbackHandler> {
    public:
        explicit CallbackHandler(MqttManagerAdapter* adapter);
        ~CallbackHandler() override = default;
        void initialize();
        void handle(uint32_t callbackId, const std::vector<uint8_t>& payload) override;
    private:
        MqttManagerAdapter* mAdapter;
    };
    std::shared_ptr<CallbackHandler> mCallbackHandler;

    static std::shared_ptr<MqttManagerAdapter> instance;
    static android::Mutex mInstanceLock;

    android::sp<RemotediagHandler> mHandler = nullptr;
};
}
#endif /* REMOTEDIAG_REG_ADAPTER_MQTT_MANAGER_H */
