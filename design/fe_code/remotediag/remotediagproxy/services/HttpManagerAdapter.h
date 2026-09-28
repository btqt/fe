#ifndef REMOTEDIAGPROXY_HTTPMANAGERADAPTER_H
#define REMOTEDIAGPROXY_HTTPMANAGERADAPTER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <utils/Mutex.h>

#include <services/HttpManagerService/IGRPCReceiver.h>
#include <services/HttpManagerService/IHttpManagerService.h>
#include <services/HttpManagerService/IHttpManagerServiceType.h>

#include "Error.h"
#include "ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "../include/ProxyIpcProtocol.h"
#include "../include/IpcMessageHandler.h"
#include "../utils/Logger.h"
#include "../utils/RemotediagProxyHandler.h"

class RemotediagProxyHandler;

class HttpManagerAdapter {
private:
    class HttpGrpcReceiver : public BnGRPCReceiver {
    public:
        explicit HttpGrpcReceiver(HttpManagerAdapter &adapter) noexcept : mParent(adapter) {}
        ~HttpGrpcReceiver() override = default;

        void onReceive(const android::sp<GrpcResData> pGrpcResData) override {
            mParent.onReceive(pGrpcResData);
        }
        void onDataConnStateChange(const GRPC_APP_TYPE pAppType, const bool pIsConnected) override {
            mParent.onDataConnStateChange(pAppType, pIsConnected);
        }

    private:
        HttpManagerAdapter &mParent;
    };

public:
    HttpManagerAdapter();
    ~HttpManagerAdapter() noexcept;
    HttpManagerAdapter(const HttpManagerAdapter &) = delete;
    HttpManagerAdapter &operator=(const HttpManagerAdapter &) = delete;
    HttpManagerAdapter(HttpManagerAdapter &&) = delete;
    HttpManagerAdapter &operator=(HttpManagerAdapter &&) = delete;

    static std::shared_ptr<HttpManagerAdapter> getInstance();

    void registerService();
    int32_t sendGrpcMessageRaw(const std::vector<uint8_t> &encodedReq);

    void onBinderDied(const android::wp<android::IBinder> &who);
    void onReceive(const android::sp<GrpcResData> pGrpcResData);
    void onDataConnStateChange(const GRPC_APP_TYPE pAppType, const bool pIsConnected);

private:
    android::sp<IHttpManagerService> getService() const;
    bool registerServiceLocked();

private:
    static std::shared_ptr<HttpManagerAdapter> instance;
    static android::Mutex mInstanceLock;

    android::sp<RemotediagProxyHandler> mHandler = nullptr;
    android::sp<HttpGrpcReceiver> mGrpcReceiver = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<IHttpManagerService> mHttpMgrService = nullptr;
    mutable android::Mutex mServiceLock;
    
    // ========================================================================
    // NESTED IPC COMMAND HANDLER
    // ========================================================================
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(HttpManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        HttpManagerAdapter* mAdapter;
    };
    std::shared_ptr<CommandHandler> mCommandHandler;
};

#endif // REMOTEDIAGPROXY_HTTPMANAGERADAPTER_H
