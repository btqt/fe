#ifndef REMOTEDIAG_SOMEIP_MNG_ADAPTER_H
#define REMOTEDIAG_SOMEIP_MNG_ADAPTER_H

#include <memory>
#include <string>
#include <map>
#include <queue>
#include <mutex>
#include <functional>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Handler.h>

/* Someip Provider */
#include <services/SomeipProviderManagerService/SomeipProviderManager.h>
#include <services/SomeipProviderManagerService/SomeipProviderCommand.h>
#include <services/SomeipProviderManagerService/EnumSomeipRxMsgID.h>
#include <services/SomeipProviderManagerService/EnumSomeipTxMsgID.h>
#include <services/SomeipProviderManagerService/ITidlSomeipProvider.h>

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"

namespace rdgapp
{
    class SomeipProviderReceiver : public BnTidlSomeipProvider
    {
    public:
        SomeipProviderReceiver();
        SomeipProviderReceiver(const SomeipProviderReceiver &) = default;
        SomeipProviderReceiver &operator=(const SomeipProviderReceiver &) = default;
        SomeipProviderReceiver(SomeipProviderReceiver &&) = default;
        SomeipProviderReceiver &operator=(SomeipProviderReceiver &&) = default;
        virtual ~SomeipProviderReceiver() override = default;

        virtual void RxUint8ArrDataToApp(EnumSomeipRxMsgID &kSomeipRxMsgID, size_t &bufSomeipDataLen, uint8_t *bufSomeipData) noexcept override;
    };

    class SomeipManagerAdapter
    {
    public:
        SomeipManagerAdapter();
        SomeipManagerAdapter(const SomeipManagerAdapter &) = default;
        SomeipManagerAdapter &operator=(const SomeipManagerAdapter &) = default;

        SomeipManagerAdapter(SomeipManagerAdapter &&) = default;
        SomeipManagerAdapter &operator=(SomeipManagerAdapter &&) = default;

        static std::shared_ptr<SomeipManagerAdapter> getInstance();

        void registerService();
        void onBinderDied(const android::wp<android::IBinder> &who);
        virtual ~SomeipManagerAdapter();

    private:
        static std::shared_ptr<SomeipManagerAdapter> instance;
        android::sp<ServiceDeathRecipient> mServiceDeathRecipient;
        android::sp<SomeipProviderReceiver> mProviderReceiver;
        android::sp<SomeipProviderManager> mSomeipProviderMgr;
        static android::Mutex mInstanceLock;
    };

} /* End: namespace rdgapp */
#endif // REMOTEDIAG_SOMEIP_MNG_ADAPTER_H
