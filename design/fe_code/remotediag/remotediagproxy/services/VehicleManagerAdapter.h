#ifndef REMOTEDIAGPROXY_VEHICLEMANAGERADAPTER_H
#define REMOTEDIAGPROXY_VEHICLEMANAGERADAPTER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <utils/Mutex.h>

#include <services/CommunicationManagerService/ICommunicationManagerService.h>
#include <services/CommunicationManagerService/ICommunicationManagerServiceType.h>
#include <services/CommunicationManagerService/IVehicleReceiver.h>
#include <services/CommunicationManagerService/Toyota_24dcmDataIndex.h>

#include "Error.h"
#include "ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../include/ProxyIpcProtocol.h"
#include "../include/IpcMessageHandler.h"
#include "../utils/Logger.h"
#include "../utils/RemotediagProxyHandler.h"

class RemotediagProxyHandler;

class VehicleManagerAdapter {
private:
    class VCMReceiver : public BnVehicleReceiver {
    public:
        explicit VCMReceiver(VehicleManagerAdapter &adapter) noexcept : mParent(adapter) {}
        ~VCMReceiver() override = default;

        void onReceived(const uint32_t channel, const android::sp<VehicleData> &vehicleData) override {
            mParent.onReceived(channel, vehicleData);
        }

        void onReceiveTimeout(const uint32_t channel, const android::sp<VehicleData> &vehicleData) override {
            mParent.onReceiveTimeout(channel, vehicleData);
        }

    private:
        VehicleManagerAdapter &mParent;
    };

public:
    VehicleManagerAdapter();
    ~VehicleManagerAdapter() noexcept;
    VehicleManagerAdapter(const VehicleManagerAdapter &) = delete;
    VehicleManagerAdapter &operator=(const VehicleManagerAdapter &) = delete;
    VehicleManagerAdapter(VehicleManagerAdapter &&) = delete;
    VehicleManagerAdapter &operator=(VehicleManagerAdapter &&) = delete;

    static std::shared_ptr<VehicleManagerAdapter> getInstance();

    void registerService();
    uint32_t getTimeCounter();
    uint16_t getTripCounter();

    void onBinderDied(const android::wp<android::IBinder> &who);
    void onReceived(uint32_t channel, const android::sp<VehicleData> &vehicleData);
    void onReceiveTimeout(uint32_t channel, const android::sp<VehicleData> &vehicleData);

private:
    android::sp<ICommunicationManagerService> getService() const;
    bool registerServiceLocked();

private:
    // Nested command handler for IPC
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(VehicleManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        VehicleManagerAdapter* mAdapter;
        rdgipc::CommandResponse handleGetTimeCounter(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleGetTripCounter(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CommandHandler> mCommandHandler;

private:
    static std::shared_ptr<VehicleManagerAdapter> instance;
    static android::Mutex mInstanceLock;

    android::sp<RemotediagProxyHandler> mHandler = nullptr;
    android::sp<IVehicleReceiver> mVehicleReceiver = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<ICommunicationManagerService> mVCMService = nullptr;
    mutable android::Mutex mServiceLock;
};

#endif // REMOTEDIAGPROXY_VEHICLEMANAGERADAPTER_H
