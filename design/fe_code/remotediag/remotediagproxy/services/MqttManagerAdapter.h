#ifndef REMOTEDIAGPROXY_MQTTMANAGERADAPTER_H
#define REMOTEDIAGPROXY_MQTTMANAGERADAPTER_H

#include <memory>
#include <string>
#include <vector>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <utils/Mutex.h>

#include <services/DcemqttproxyManagerService/DceNotification.h>
#include <services/DcemqttproxyManagerService/IDcemqttproxyManagerService.h>
#include <services/DcemqttproxyManagerService/IDcemqttproxyReceiver.h>

#include "Error.h"
#include "ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../include/ProxyIpcProtocol.h"
#include "../include/IpcMessageHandler.h"
#include "../utils/Logger.h"
#include "../utils/RemotediagProxyHandler.h"

class RemotediagProxyHandler;

class MqttManagerAdapter {
private:
    class MqttNotifyReceiver : public BnDcemqttproxyReceiver {
    public:
        explicit MqttNotifyReceiver(MqttManagerAdapter &adapter) noexcept : mParent(adapter) {}
        ~MqttNotifyReceiver() override = default;

        void onNotifyCb(const android::sp<DceNotification> message) override {
            mParent.onNotifyReceived(message);
        }

    private:
        MqttManagerAdapter &mParent;
    };

public:
    MqttManagerAdapter();
    ~MqttManagerAdapter() noexcept;
    MqttManagerAdapter(const MqttManagerAdapter &) = delete;
    MqttManagerAdapter &operator=(const MqttManagerAdapter &) = delete;
    MqttManagerAdapter(MqttManagerAdapter &&) = delete;
    MqttManagerAdapter &operator=(MqttManagerAdapter &&) = delete;

    static std::shared_ptr<MqttManagerAdapter> getInstance();

    void registerService();
    bool subscribeTopic(const std::string &vinNum);

    void onBinderDied(const android::wp<android::IBinder> &who);

private:
    android::sp<IDcemqttproxyManagerService> getService() const;
    bool registerServiceLocked();
    void onNotifyReceived(const android::sp<DceNotification> &message);

private:
    // Nested command handler for IPC
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(MqttManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        MqttManagerAdapter* mAdapter;
        rdgipc::CommandResponse handleSubscribeTopic(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CommandHandler> mCommandHandler;

private:
    static std::shared_ptr<MqttManagerAdapter> instance;
    static android::Mutex mInstanceLock;

    android::sp<RemotediagProxyHandler> mHandler = nullptr;
    android::sp<MqttNotifyReceiver> mReceiver = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<IDcemqttproxyManagerService> mDcemqttproxy = nullptr;
    mutable android::Mutex mServiceLock;
};

#endif // REMOTEDIAGPROXY_MQTTMANAGERADAPTER_H
