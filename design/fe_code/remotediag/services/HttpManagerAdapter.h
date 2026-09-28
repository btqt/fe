#ifndef RDG_HTTP_MANAGER_ADAPTER_H
#define RDG_HTTP_MANAGER_ADAPTER_H

#include <string>
#include <cstdint>
#include <memory>

#include <services/HttpManagerService/GrpcReqData.h>
#include <services/HttpManagerService/GrpcResData.h>
#include <services/HttpManagerService/IGRPCReceiver.h>
#include <services/RegionManagerService/IRegionManagerServiceType.h>
#include <services/RegionManagerService/RegionManager.h>

#include <binder/Parcel.h>
#include <utils/Mutex.h>
#include <Error.h>

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../include/ParamsDef.h"
#include "../remotediagproxy/include/ProxyIpcProtocol.h"
#include "../remotediagproxy/include/IpcMessageHandler.h"

namespace rdgapp {

class RemotediagHandler;

class HttpManagerAdapter {
    
    public:
        HttpManagerAdapter();
        ~HttpManagerAdapter();
        static std::shared_ptr<HttpManagerAdapter> getInstance();
        void registerService();
        int32_t sendGrpcMessage(const android::sp<GrpcReqData>& pGrpcReqData);
        void testTriggerReceive(const android::sp<GrpcResData>& pGrpcResData);
        void onReceive(android::sp<GrpcResData> pGrpcResData);
        void onDataConnStateChange(const GRPC_APP_TYPE pAppType, const bool pIsConnected);
        bool getConnectionAvail();
        uint32_t getProtoTextVersion() const noexcept;
    private:
        // Nested CallbackHandler for self-registering callback handling
        class CallbackHandler : public rdgipc::ICallbackHandler,
                               public std::enable_shared_from_this<CallbackHandler> {
        public:
            explicit CallbackHandler(HttpManagerAdapter* adapter);
            ~CallbackHandler() override = default;
            void initialize();
            void handle(uint32_t callbackId, const std::vector<uint8_t>& payload) override;
        private:
            HttpManagerAdapter* mAdapter;
        };
        std::shared_ptr<CallbackHandler> mCallbackHandler;

        HttpManagerAdapter(const HttpManagerAdapter& ) = delete;
        HttpManagerAdapter& operator=(const HttpManagerAdapter& ) = delete;
        HttpManagerAdapter(HttpManagerAdapter&& ) = delete;
        HttpManagerAdapter& operator=(HttpManagerAdapter&& ) = delete;

    private:
        static std::shared_ptr<HttpManagerAdapter> instance;
        android::sp<RemotediagHandler> mHandler = nullptr;
        bool connectionAvail;
        uint32_t mProtoTextVer;
        static android::Mutex mInstanceLock;
};
}
#endif /* RDG_HTTP_MANAGER_ADAPTER_H */
