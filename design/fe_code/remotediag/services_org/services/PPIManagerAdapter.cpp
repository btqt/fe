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
android::Mutex PPIManagerAdapter::mInstanceLock{};
std::shared_ptr<PPIManagerAdapter> PPIManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<PPIManagerAdapter>();
        }
    }
    return instance;
}

android::sp<IPPIManagerService> PPIManagerAdapter::getService() {
   return android::interface_cast<IPPIManagerService>(
                android::defaultServiceManager()->getService(
                    android::String16("service_layer.PPIManagerService")
                    )
                );
}

void PPIManagerAdapter::onStatusChanged(android::sp<::Buffer> &name) const
{
    LOG_I("PPIManagerAdapter::onStatusChanged");
    const android::sp<sl::Message> msg {mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_PPI_INFO_RECEIVED)};
    msg->buffer = *name;
    (void)msg->sendToTarget();
}

void PPIManagerAdapter::registerService(){
    LOG_I("Start register PPIManagerAdapter");
    mHandler = RemotediagHandler::getInstance();
    mPPIManagerService = getService();
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

uint32_t PPIManagerAdapter::receivePPIErase(const char_t* const buf){
    uint32_t PPIFlag {0U};
    if(strncmp(buf, "PPIFlag", strlen("PPIFlag")) == 0) {
        LOG_I("PPIManagerAdapter::receivePPIErase process");
        if (mPPIManagerService != nullptr) {
            int32_t tmp_flag {0};
            const std::string strFlag {mPPIManagerService->getPropertyPPIValue("PPIFlag")};
            try
            {
                tmp_flag = std::stoi(strFlag);
            }
            catch (std::exception& e)
            {
                LOG_E("Exception: %s", e.what());
            }
            if(tmp_flag >=0)
            {
                PPIFlag = static_cast<uint32_t>(tmp_flag);
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
    return PPIFlag;
}

void PPIManagerAdapter::responsePPIErase(const uint32_t appType, const uint32_t appState)
{
    LOG_I("appType = %u, appState = %u", appType, appState);
    const android::sp<IPPIManagerService> ppiMgr{getService()};
    if (ppiMgr != nullptr)
    {
        (void)ppiMgr->responseDeletePPInformation(appType, appState);
    }
    else
    {
        LOG_E("PPIManagerService is not ready");
    }
}

void PPIManagerAdapter::onBinderDied(const android::wp<android::IBinder>& who)
{
    LOG_I("PPI die, try again after 500ms");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
    NOTUSED(who);
    mPPIManagerService = nullptr;
    if (mHandler != nullptr)
    {
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_PPI_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);       
    }
}
}
