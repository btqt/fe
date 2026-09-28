#ifndef REMOTEDIAG_REG_ADAPTER_MQTT_MANAGER_H
#define REMOTEDIAG_REG_ADAPTER_MQTT_MANAGER_H

#include <memory>
#include <string>
#include <map>
#include <functional>

#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include <services/DcemqttproxyManagerService/IDcemqttproxyManagerService.h>
#include <services/DcemqttproxyManagerService/IDcemqttproxyReceiver.h>
#include <services/DcemqttproxyManagerService/MessageIdType.h>
#include <services/DcemqttproxyManagerService/DceNotification.h>

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"

namespace rdgapp {

class RemotediagHandler;
class DcemqttproxyReceiver;

class MqttManagerAdapter : public android::RefBase
{
public:
    MqttManagerAdapter();
    virtual ~MqttManagerAdapter();
    MqttManagerAdapter(MqttManagerAdapter const &) = default;
    MqttManagerAdapter &operator=(MqttManagerAdapter const &) = default;
    MqttManagerAdapter(MqttManagerAdapter &&) = delete;
    MqttManagerAdapter &operator=(MqttManagerAdapter &&) = delete;
    static android::sp<MqttManagerAdapter> getInstance();

    /* Register a service for handle event */
    void registerService();

private:
    /* For checking service died */
    void onBinderDied(const android::wp<android::IBinder> &who);

private:
    static android::sp<MqttManagerAdapter> mMqttManagerAdapter;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient{nullptr};
    android::sp<RemotediagHandler> mHandler = nullptr;
    android::sp<IDcemqttproxyManagerService> mDcemqttproxy;
    android::sp<DcemqttproxyReceiver> mDcemqttProxyReceiver;
    mutable android::Mutex mDiedLock;
};

class DcemqttproxyReceiver : public BnDcemqttproxyReceiver
{
private:
    MqttManagerAdapter &mParent;
    bool isMQTTSimulate{false};
    android::sp<sl::Handler> mHandler;

public:
    DcemqttproxyReceiver(MqttManagerAdapter &parent) noexcept;
    void onNotifyCb(const android::sp<DceNotification> message) override;
};
}
#endif /* REMOTEDIAG_REG_ADAPTER_MQTT_MANAGER_H */
