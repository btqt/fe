#ifndef REMOTEDIAGPROXY_LOCATIONMANAGER_H
#define REMOTEDIAGPROXY_LOCATIONMANAGER_H

#include <memory>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>

#include <services/LocationManagerService/ILocationManagerService.h>
#include <services/LocationManagerService/ILocationManagerServiceType.h>

#include "Error.h"
#include "ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../include/IpcMessageHandler.h"
#include "../utils/Logger.h"
#include "../utils/RemotediagProxyHandler.h"

class RemotediagProxyHandler;
class LocationManagerAdapter
{
public:
    LocationManagerAdapter();
    virtual ~LocationManagerAdapter() noexcept;
    LocationManagerAdapter(const LocationManagerAdapter &) = delete;
    LocationManagerAdapter &operator=(const LocationManagerAdapter &) = delete;
    LocationManagerAdapter(LocationManagerAdapter &&) = delete;
    LocationManagerAdapter &operator=(LocationManagerAdapter &&) = delete;
    static std::shared_ptr<LocationManagerAdapter> getInstance();

    virtual void registerService();
    bool getLocationData(int32_t &latitude, int32_t &longitude);

private:
    virtual android::sp<ILocationManagerService> getLocationManagerService();
    void onBinderDied(const android::wp<android::IBinder> &who);

private:
    // Nested command handler for IPC
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(LocationManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        LocationManagerAdapter* mAdapter;
        rdgipc::CommandResponse handleGetLocation(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CommandHandler> mCommandHandler;

private:
    static std::shared_ptr<LocationManagerAdapter> instance;
    android::sp<RemotediagProxyHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<ILocationManagerService> mLocationservice = nullptr;
    mutable android::Mutex mDiedLock;
    static android::Mutex mInstanceLock;
};

#endif // REMOTEDIAGPROXY_LOCATIONMANAGER_H
