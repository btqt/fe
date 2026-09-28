#include "PPIManagerAdapter.h"

namespace rdgapp {

PPIManagerAdapter::PPIManagerAdapter() {
    LOG_I("constructor PPIManagerAdapter");
    mServiceDeathRecipient = new ServiceDeathRecipient( [this] ( const android::wp<android::IBinder>& who ) {
        this->onBinderDied(who);
    });
}

PPIManagerAdapter::~PPIManagerAdapter() noexcept {
    if(PPIManagerAdapter::instance != nullptr) {
        PPIManagerAdapter::instance = nullptr;
    } 
}

std::shared_ptr<PPIManagerAdapter> PPIManagerAdapter::instance{nullptr};
std::shared_ptr<PPIManagerAdapter> PPIManagerAdapter::getInstance() {
    if (instance == nullptr) {
        instance = std::make_shared<PPIManagerAdapter>();
    }
    return instance;
}

android::sp<IPPIManagerService> PPIManagerAdapter::getService() {
    mPPIManagerService = android::interface_cast<IPPIManagerService>(
                android::defaultServiceManager()->getService(
                    android::String16("service_layer.PPIManagerService")
                    )
                );
    return mPPIManagerService;
}

void PPIManagerAdapter::onStatusChanged(android::sp<::Buffer> &name) const
{
    LOG_I("PPIManagerAdapter::onStatusChanged");
    const android::sp<sl::Message> msg {mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_PPI_INFO_RECEIVED)};
    if(name->size() > 0U){
        const uint32_t tmp_size{name->size()};
        if(tmp_size <= static_cast<uint32_t>(INT32_MAX))
        {
            msg->buffer.setTo(name->data(), static_cast<int32_t>(tmp_size)+1);
        }
        else
        {
            // do nothing
        }
    }
    (void)msg->sendToTarget();
}

void PPIManagerAdapter::registerService(){
    LOG_I("Start register PPIManagerAdapter");
    (void)getService();
    mHandler = RemotediagHandler::getInstance_2();
    
    if(mPPIManagerService != nullptr)
    {
        LOG_I("PPIManagerAdapter Registed");
        (void)android::IInterface::asBinder(mPPIManagerService)->linkToDeath(mServiceDeathRecipient);
        mPPIStatusReceiver = android::sp<PPIMgrReceiver>(new PPIMgrReceiver(*this));
        (void)mPPIManagerService->registerReceiverPPIStatusReceiverOnStatusChanged(mPPIStatusReceiver);
    }
    else
    {
        LOG_I("connectToPPIMgr:: wait mPPIManager 500ms and retry");
        if(mHandler != nullptr)
        {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_PPI_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        }
    }
}

void PPIManagerAdapter::receivePPIErase(const uint8_t* const buf){
    LOG_I("");
    const string flag{"PPIFlag"};
    if(memcmp(buf, "PPIFlag", std::min(flag.size(),sizeof(buf))) == 0){
        LOG_I("PPIManagerAdapter::receivePPIErase process");
        if(mPPIManagerService != nullptr){
            const int32_t tmp_flag{std::stoi(mPPIManagerService->getPropertyPPIValue("PPIFlag"))};
            if(tmp_flag >=0)
            {
                const uint32_t ppiFlag {static_cast<uint32_t>(tmp_flag)};
                deleteData(ppiFlag);
            }
            else
            {
                LOG_E("ppiFlag is negative value");
            }
        } else {
            LOG_E("mPPIManagerService is nullptr");
            LOG_E("PPIManagerService is not ready");
        }
    }
    else{
        LOG_I("PPIManagerAdapter::receivePPIErase do not process");
    }
}

void PPIManagerAdapter::deleteData(const uint32_t ppiFlag){
    LOG_I("ppiFlag = %d", ppiFlag);
    switch(ppiFlag){
        case IPPIManagerServiceType::PPI_OPERATING:
        case IPPIManagerServiceType::PPI_RETRY_1ST:
        case IPPIManagerServiceType::PPI_RETRY_2ND:
        {
            if(mPPIManagerService != nullptr) {
                // if successful
                LOG_I("PPI_HISTORY_ECALL_FILE : PPI_APP_STATUS_CLEANUP");
                (void)mPPIManagerService->responseDeletePPInformation(IPPIManagerServiceType::PPI_HISTORY_ECALL_FILE, IPPIManagerServiceType::PPI_APP_STATUS_CLEANUP);
                // // if failure
                // mPPIManagerService->responseDeletePPInformation(IPPIManagerServiceType::PPI_HISTORY_ECALL_FILE, IPPIManagerServiceType::PPI_APP_STATUS_NON_CLEANUP);
            } else {
                LOG_E("mPPIManagerService is nullptr");
                LOG_E("PPIManagerService is not ready");
            }
            break;
        }

        case IPPIManagerServiceType::PPI_NO_RESPONSE:
        case IPPIManagerServiceType::PPI_FORMATTING:
        {
            break;
        }
        
        case IPPIManagerServiceType::PPI_FORMAT_COMPLETE:
        case IPPIManagerServiceType::PPI_FORMAT_UNCOMPLETE:
        case IPPIManagerServiceType::PPI_END:
        {
            if(mPPIManagerService != nullptr) {
                // When data deletion is finished, be sure to set "PPI_APP_STATUS_RE_RUNNING" and call responseDeletePPInformation().
                // Post-action in PPI Mgr and change to "PPI_APP_STATUS_INIT".
                LOG_I("PPI_HISTORY_ECALL_FILE : PPI_APP_STATUS_RE_RUNNING");
                (void)mPPIManagerService->responseDeletePPInformation(IPPIManagerServiceType::PPI_HISTORY_ECALL_FILE, IPPIManagerServiceType::PPI_APP_STATUS_RE_RUNNING);
            } else {
                LOG_E("mPPIManagerService is nullptr");
                LOG_E("PPIManagerService is not ready");
            }
            break;
        }

        default:
            break;
    }
}

void PPIManagerAdapter::onBinderDied(const android::wp<android::IBinder>& who) {
    LOG_I("PPI die, try again after 500ms");
    NOTUSED(who);
    mPPIManagerService = nullptr;
    (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_PPI_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
}
}
