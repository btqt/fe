#ifndef REMOTEDIAGPROXY_SOMEIPMANAGERADAPTER_H
#define REMOTEDIAGPROXY_SOMEIPMANAGERADAPTER_H

#include <memory>
#include <string>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <utils/Mutex.h>

#include <services/SomeipProviderManagerService/SomeipProviderManager.h>
#include <services/SomeipProviderManagerService/SomeipProviderCommand.h>
#include <services/SomeipProviderManagerService/EnumSomeipRxMsgID.h>
#include <services/SomeipProviderManagerService/EnumSomeipTxMsgID.h>
#include <services/SomeipProviderManagerService/ITidlSomeipProvider.h>

#include "Error.h"
#include "ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../include/ProxyIpcProtocol.h"
#include "../utils/Logger.h"
#include "../utils/RemotediagProxyHandler.h"

class RemotediagProxyHandler;

class SomeipProviderReceiver : public BnTidlSomeipProvider
{
public:
    SomeipProviderReceiver() = default;
    SomeipProviderReceiver(const SomeipProviderReceiver &) = default;
    SomeipProviderReceiver &operator=(const SomeipProviderReceiver &) = default;
    SomeipProviderReceiver(SomeipProviderReceiver &&) = default;
    SomeipProviderReceiver &operator=(SomeipProviderReceiver &&) = default;
    ~SomeipProviderReceiver() override = default;

    void RxUint8ArrDataToApp(EnumSomeipRxMsgID &kSomeipRxMsgID,
                             size_t &bufSomeipDataLen,
                             uint8_t *bufSomeipData) noexcept override;
};

class SomeipManagerAdapter {
public:
    SomeipManagerAdapter();
    ~SomeipManagerAdapter() noexcept;
    SomeipManagerAdapter(const SomeipManagerAdapter &) = delete;
    SomeipManagerAdapter &operator=(const SomeipManagerAdapter &) = delete;
    SomeipManagerAdapter(SomeipManagerAdapter &&) = delete;
    SomeipManagerAdapter &operator=(SomeipManagerAdapter &&) = delete;

    static std::shared_ptr<SomeipManagerAdapter> getInstance();

    void registerService();
    void onBinderDied(const android::wp<android::IBinder> &who);

private:
    static std::shared_ptr<SomeipManagerAdapter> instance;
    static android::Mutex mInstanceLock;

    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<SomeipProviderReceiver> mProviderReceiver = nullptr;
    android::sp<SomeipProviderManager> mSomeipProviderMgr = nullptr;
};

#endif // REMOTEDIAGPROXY_SOMEIPMANAGERADAPTER_H
