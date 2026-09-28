#ifndef REMOTEDIAG_PPIMANAGERADAPTER_H
#define REMOTEDIAG_PPIMANAGERADAPTER_H

#include <iostream>
#include <cstring>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include "services/PPIManagerService/IPPIManagerServiceType.h"
#include "services/PPIManagerService/IPPIManagerService.h"
#include "services/PPIManagerService/IPPIStatusReceiver.h"
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <Error.h>

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"

namespace rdgapp {

class RemotediagHandler;
class PPIManagerAdapter
{
    class PPIMgrReceiver: public BnPPIStatusReceiver{
    public:
        PPIMgrReceiver(PPIManagerAdapter &pr) noexcept : parent(pr){}
        virtual ~PPIMgrReceiver() = default;
        virtual void onStatusChanged(android::sp<::Buffer>& name) override
        {
            return parent.onStatusChanged(name);
        }
    private:
        PPIManagerAdapter& parent;
    };

    public:
        PPIManagerAdapter();
        ~PPIManagerAdapter();
        PPIManagerAdapter(const PPIManagerAdapter& ) = delete;
        PPIManagerAdapter& operator=(const PPIManagerAdapter& ) = delete;
        PPIManagerAdapter(PPIManagerAdapter&& ) = delete;
        PPIManagerAdapter& operator=(PPIManagerAdapter&& ) = delete;
        
        static std::shared_ptr<PPIManagerAdapter> getInstance();
        android::sp<IPPIManagerService> getService();

        void onStatusChanged(android::sp<::Buffer>& name) const;
        void registerService();
        void onBinderDied(const android::wp<android::IBinder>& who);
        // void handleMessage(const sp<sl::Message>& msg);
        uint32_t receivePPIErase(const char_t* const buf);
        void responsePPIErase(const uint32_t appType, const uint32_t appState);

    private:
        static std::shared_ptr<PPIManagerAdapter> instance;
        android::sp<IPPIManagerService>         mPPIManagerService      {nullptr};
        android::sp<IPPIStatusReceiver>         mPPIStatusReceiver      {nullptr};
        android::sp<ServiceDeathRecipient>      mServiceDeathRecipient  {nullptr};
        android::sp<RemotediagHandler> mHandler = nullptr;
        mutable android::Mutex mDiedLock;
        static android::Mutex mInstanceLock;

};
}
#endif //REMOTEDIAG_PPIMANAGERADAPTER_H
