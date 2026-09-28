#ifndef RDG_HTTP_MANAGER_ADAPTER_H
#define RDG_HTTP_MANAGER_ADAPTER_H

#include <string>
#include <cstdint>
#include <memory>

#include <services/HttpManagerService/IHttpManagerService.h>
#include <services/HttpManagerService/IHttpManagerServiceType.h>
#include <services/HttpManagerService/IGRPCReceiver.h>
#include <services/HttpManagerService/IHTTPReceiver.h>
#include <services/RegionManagerService/IRegionManagerService.h>
#include <services/RegionManagerService/IRegionManagerServiceType.h>
#include <services/RegionManagerService/RegionManager.h>

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
// class HTTPMgrEventData : public android::RefBase 
// {
//     public:
//         HTTPMgrEventData() = delete;
//         HTTPMgrEventData(const uint32_t responseCode, const std::string data)
//         {
//             mResponseCode = responseCode;
//             mDownloadData = data;

//         }

//         std::string getResponseData() noexcept
//         {
//             return mDownloadData;
//         }

//         uint32_t getResponseCode() const noexcept
//         {
//             return mResponseCode;
//         }
//     private:
//         uint32_t mResponseCode;
//         std::string mDownloadData;
// };

class HttpManagerAdapter 
{
    class GRPCReceiver : public BnGRPCReceiver {
    public:
        GRPCReceiver(HttpManagerAdapter& pr) noexcept : parent(pr) {}
        GRPCReceiver(const GRPCReceiver& ) = default;
        GRPCReceiver& operator=(const GRPCReceiver& ) = default;
        GRPCReceiver(GRPCReceiver&& ) = default;
        GRPCReceiver& operator=(GRPCReceiver&& ) = default;
        virtual ~GRPCReceiver() override = default;

        void onReceive(const android::sp<GrpcResData> pGrpcResData)
        {
            parent.onReceive(pGrpcResData);
        }
        void onDataConnStateChange(const GRPC_APP_TYPE pAppType, const  bool pIsConnected)
        {
            parent.onDataConnStateChange(pAppType, pIsConnected);
        }

    private:
        HttpManagerAdapter& parent;
    };
    
    public:
        HttpManagerAdapter();
        ~HttpManagerAdapter();
        static std::shared_ptr<HttpManagerAdapter> getInstance();
        void registerService();
        error_t registerReceiver();
        int32_t sendGrpcMessage(const android::sp<GrpcReqData>& pGrpcReqData);
        void testTriggerReceive(const android::sp<GrpcResData>& pGrpcResData);
        void onReceive(android::sp<GrpcResData> pGrpcResData);
        void onDataConnStateChange(const GRPC_APP_TYPE pAppType, const bool pIsConnected);
        bool getConnectionAvail();
    private:
        HttpManagerAdapter(const HttpManagerAdapter& ) = delete;
        HttpManagerAdapter& operator=(const HttpManagerAdapter& ) = delete;
        HttpManagerAdapter(HttpManagerAdapter&& ) = delete;
        HttpManagerAdapter& operator=(HttpManagerAdapter&& ) = delete;

        void onBinderDied(const android::wp<android::IBinder>& who);
        // void handleReceive(const android::sp<GrpcResData>& pGrpcResData);

    private:
        static std::shared_ptr<HttpManagerAdapter> instance;
        android::sp<RemotediagHandler> mHandler = nullptr;
        android::sp<ServiceDeathRecipient> mServiceDeathRecipient {nullptr};
        android::sp<IGRPCReceiver> mGRPCReceiver                  {nullptr};
        android::sp<IHttpManagerService> mHTTPMgrService          {nullptr};
        bool connectionAvail;
};
}
#endif /* RDG_HTTP_MANAGER_ADAPTER_H */
