#ifndef REMOTEDIAGPROXY_REGIONMANAGER_H
#define REMOTEDIAGPROXY_REGIONMANAGER_H

#include <memory>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>

#include <services/RegionManagerService/IRegionManagerService.h>
#include <services/RegionManagerService/IRegionManagerServiceType.h>

#include "Error.h"
#include "ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../include/IpcMessageHandler.h"
#include "../utils/Logger.h"
#include "../utils/RemotediagProxyHandler.h"

class RemotediagProxyHandler;
class RegionManagerAdapter
{
public:
    RegionManagerAdapter();
    virtual ~RegionManagerAdapter() noexcept;
    RegionManagerAdapter(const RegionManagerAdapter &) = delete;
    RegionManagerAdapter &operator=(const RegionManagerAdapter &) = delete;
    RegionManagerAdapter(RegionManagerAdapter &&) = delete;
    RegionManagerAdapter &operator=(RegionManagerAdapter &&) = delete;
    static std::shared_ptr<RegionManagerAdapter> getInstance();

    virtual void registerService();
    uint8_t getNation();

private:
    virtual android::sp<IRegionManagerService> getRegionManagerService();
    void onBinderDied(const android::wp<android::IBinder> &who);

private:
    // Nested command handler for IPC
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(RegionManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        RegionManagerAdapter* mAdapter;
        rdgipc::CommandResponse handleGetNation(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CommandHandler> mCommandHandler;

private:
    static std::shared_ptr<RegionManagerAdapter> instance;
    android::sp<RemotediagProxyHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<IRegionManagerService> mRegionMService = nullptr;
    mutable android::Mutex mDiedLock;
    static android::Mutex mInstanceLock;
};

#endif // REMOTEDIAGPROXY_REGIONMANAGER_H
