#ifndef REMOTEDIAG_REG_ADAPTER_ONBOARDCLIENT_MANAGER_H
#define REMOTEDIAG_REG_ADAPTER_ONBOARDCLIENT_MANAGER_H

#include <memory>
#include <string>
#include <map>
#include <functional>

#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include <services/OnboardclientManagerService/IOnboardclientManagerService.h>
#include <services/OnboardclientManagerService/IOnboardClientReceiver.h>
#include <services/OnboardclientManagerService/OBCResponseEventInfo.h>
#include <services/OnboardclientManagerService/OBCUDSResInfo.h>
#include <services/OnboardclientManagerService/OBCCanInfo.h>
#include <services/OnboardclientManagerService/OBCTransportInfo.h>
#include <services/OnboardclientManagerService/OBCConnectInfo.h>
#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "diagprocess/OTA/OtaMessageDefine.h"

namespace rdgapp {

class RemotediagHandler;
class OnboardClientReceiver;

class OnboardclientAdapter : public android::RefBase
{
public:
    OnboardclientAdapter();
    virtual ~OnboardclientAdapter() noexcept;
    OnboardclientAdapter(OnboardclientAdapter const &) = default;
    OnboardclientAdapter &operator=(OnboardclientAdapter const &) = default;
    OnboardclientAdapter(OnboardclientAdapter &&) = delete;
    OnboardclientAdapter &operator=(OnboardclientAdapter &&) = delete;
    static  android::sp<OnboardclientAdapter> getInstance();
    /* Register a service for handle event */
    void registerService();
    uint8_t sendUdsData(const uint16_t connectId, const android::sp<::Buffer> udsRequest);
    uint8_t disconnectECU(const uint16_t connectionID);
    error_t connect(const android::sp<OBCTransportInfo> transportInfo, const std::string appNames, android::sp<OBCConnectInfo> connectInfo) noexcept;
    OBCResourceEventCode GetObcResource() const noexcept { return obcResourceStatus; };
    void SetObcResource(const OBCResourceEventCode status);
    void TakeObcResource(void);
    void ReleaseObcResource(void);
    void startTimer(const uint32_t timerId);
    void stopTimer(const uint32_t timerId);

public:
    class OnboardclientManagerAdapterTimer : public TimerTimeoutHandler
    {
    public:
        static constexpr uint32_t OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION{1U};
        OnboardclientManagerAdapterTimer() noexcept;
        void handlerFunction(const int32_t timerId) override;
    };

private:
    OnboardclientManagerAdapterTimer *mOnboardclientManagerAdapterTimer;
    android::sp<Timer> mTimeOutWireConenction;
    void initTimer(void);

private:
    /* For checking service died */
    void onBinderDied(const android::wp<android::IBinder> &who);

private:
    static  android::sp<OnboardclientAdapter> mOnboardclientAdapter;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient{nullptr};
    android::sp<RemotediagHandler> mHandler = nullptr;
    android::sp<IOnboardclientManagerService> mOnboardclient;
    android::sp<OnboardClientReceiver> mOnboardClientReceiver;
    mutable android::Mutex mDiedLock;
    OBCResourceEventCode obcResourceStatus{OBCResourceEventCode::OBC_GET_RESOURCE_OK};
};

class OnboardClientReceiver : public BnOnboardClientReceiver
{
private:
    OnboardclientAdapter &mParent;
    android::sp<sl::Handler> mHandler;

public:
    OnboardClientReceiver(OnboardclientAdapter &parent) noexcept;
    void onNotifyOBD2Event() noexcept override;
    void onResponseEvent(const android::sp<OBCResponseEventInfo> message) noexcept override;
};
}
#endif /* REMOTEDIAG_REG_ADAPTER_ONBOARDCLIENT_MANAGER_H */
