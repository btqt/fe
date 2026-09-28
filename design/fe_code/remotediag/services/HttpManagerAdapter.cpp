#include "HttpManagerAdapter.h"

namespace rdgapp {

HttpManagerAdapter::HttpManagerAdapter() {
    LOG_I("HttpManagerAdapter Constructor");
    connectionAvail = true;
    mServiceDeathRecipient = new ServiceDeathRecipient( [this] ( const android::wp<android::IBinder>& who ){
        this->onBinderDied(who);
    });
}

HttpManagerAdapter::~HttpManagerAdapter() {
    if(HttpManagerAdapter::instance != nullptr) {
        HttpManagerAdapter::instance = nullptr;
    }
    mServiceDeathRecipient = nullptr;
    mHandler               = nullptr;
    mGRPCReceiver          = nullptr;
    mHTTPMgrService        = nullptr;
}

std::shared_ptr<HttpManagerAdapter> HttpManagerAdapter::instance{nullptr};
std::shared_ptr<HttpManagerAdapter> HttpManagerAdapter::getInstance() {
    if (instance == nullptr) {
        instance = std::make_shared<HttpManagerAdapter>();
    }
    return instance;
}

void HttpManagerAdapter::registerService() {
    LOG_I("HttpManagerAdapter registerService");
    mHandler = RemotediagHandler::getInstance();
    
    if(mGRPCReceiver == nullptr) {
        mGRPCReceiver = android::sp<IGRPCReceiver>{new GRPCReceiver(*this)};
    }
    
    if (mHTTPMgrService != nullptr) {
        mHTTPMgrService = nullptr;
    }

    mHTTPMgrService = android::interface_cast<IHttpManagerService>(android::defaultServiceManager()->getService(android::String16("service_layer.HttpManagerService")));

    bool error {true};
    if (mHTTPMgrService != nullptr) {
        if (android::OK == android::IInterface::asBinder(mHTTPMgrService)->linkToDeath(mServiceDeathRecipient)) {
            const error_t ret {registerReceiver()};
            if(ret != E_OK) {
                LOG_E("Register receiver failed, ret = %d", ret);
            } else {
                error = false;
            }
            error = false;
        }
    }
    
    if(error) {
        LOG_I("Cannot register HTPPS Manager Service");
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_HTTP_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        } else {
            LOG_E("mHandler = nullptr");
        }
    }

}

error_t HttpManagerAdapter::registerReceiver() {
    error_t result {E_ERROR};

    if ((mHTTPMgrService != nullptr) && (mGRPCReceiver != nullptr))
    {
        constexpr GRPC_APP_TYPE pAppType {GRPC_APP_TYPE::RMT_DIAG};
        
        result = mHTTPMgrService->registerReceiverGRPCReceive(mGRPCReceiver, pAppType);
    }
    else
    {
        LOG_E("mHTTPMgrService is nullptr");
    }

    return result;
}

void HttpManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
    LOG_I("HttpManagerAdapter::onBinderDied");
    NOTUSED(who);
    mHTTPMgrService = nullptr;
    mGRPCReceiver = nullptr;
    (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_HTTP_MGR), 
        static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
}

void HttpManagerAdapter::testTriggerReceive(const android::sp<GrpcResData>& pGrpcResData)
{
    if (pGrpcResData->getAppType() == GRPC_APP_TYPE::RMT_DIAG)
    {
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RPC_MESSAGE_RECEIVED, pGrpcResData)->sendToTarget();
    } else {

    }
}

void HttpManagerAdapter::onReceive(const android::sp<GrpcResData> pGrpcResData) {
    // handleReceive(pGrpcResData);
    if(pGrpcResData != nullptr) 
    {
        const android::sp<GrpcResData> localGrpcResData {new GrpcResData()};
        localGrpcResData->setTo(*pGrpcResData);
        if (localGrpcResData->getAppType() == GRPC_APP_TYPE::RMT_DIAG)
        {
            (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RPC_MESSAGE_RECEIVED, localGrpcResData)->sendToTarget();
        } else {

        }
    } else {
        LOG_E("Receive GrpcResData is empty");
    }
}

void HttpManagerAdapter::onDataConnStateChange(const GRPC_APP_TYPE pAppType, const bool pIsConnected)
{
    LOG_I("GRPC Communication change apptype = %d", static_cast<int32_t>(pAppType));
    if(pAppType == GRPC_APP_TYPE::RMT_DIAG)
    {
        if(pIsConnected == true)
        {
            connectionAvail = true;
            (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_GRPC_COMMUNICATION_RECONNECT)->sendToTarget();
        } else
        {
            connectionAvail = false;
            (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_GRPC_COMMUNICATION_DISCONNECT)->sendToTarget();
        }
    }
}

bool HttpManagerAdapter::getConnectionAvail() {
    if(connectionAvail) {
        LOG_D("Check connectionAvail OK ");
    } else {
        LOG_D("Check connectionAvail NOT OK ");
    }
    return connectionAvail;
}

int32_t HttpManagerAdapter::sendGrpcMessage(const android::sp<GrpcReqData>& pGrpcReqData)
{
    int32_t result {-1};
    if (mHTTPMgrService != nullptr)
    {
        /*http://10.158.7.45:8100/xref/toyota_24dcm_release/nad/LGE/24dcm-src/services/http-manager/interface/include/services/HttpManagerService/IHttpManagerService.h#356*/
        result = mHTTPMgrService->sendOverGRPC(pGrpcReqData);
        if(result < 0)
        {
            LOG_E("Send Over gRPC data failed");
        } else {
            LOG_D("Send successful. API Call ID: %d", result);
        }
    } 
    else 
    {
        LOG_E("mHTTPMgrService is nullptr");
    }
    return result;
}

}
