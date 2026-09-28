#include "SomeipManagerAdapter.h"

namespace rdgapp
{
    SomeipManagerAdapter::SomeipManagerAdapter()
    {
        mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who)
                                                           { this->onBinderDied(who); });
    }
    SomeipManagerAdapter::~SomeipManagerAdapter()
    {
    }
    std::shared_ptr<SomeipManagerAdapter> SomeipManagerAdapter::instance{nullptr};
    android::Mutex SomeipManagerAdapter::mInstanceLock{};
    std::shared_ptr<SomeipManagerAdapter> SomeipManagerAdapter::getInstance()
    {
        if (instance == nullptr)
        {
            const android::AutoMutex _l{mInstanceLock};
            if (instance == nullptr)
            {
                instance = std::make_shared<SomeipManagerAdapter>();
            }
        }
        return instance;
    }

    void SomeipManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who)
    {
        LOG_I("SomeIpMgr is died");
        NOTUSED(who);
        const android::sp<RemotediagHandler> handler{RemotediagHandler::getInstance()};
        if (handler != nullptr)
        {
            (void)handler->sendMessageDelayed(handler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_SOMEIP_MGR), static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
        else
        {
            LOG_E("handler = nullptr");
        }
    }

    void SomeipManagerAdapter::registerService()
    {
        /* Register SomeipProviderMgr receiver */
        mSomeipProviderMgr = SomeipProviderManager::instance();
        error_t errCode{E_ERROR};
        if (mSomeipProviderMgr != nullptr)
        {
            const android::sp<ISomeipProviderManagerService> someipProviderMgr{android::interface_cast<ISomeipProviderManagerService>(
                android::defaultServiceManager()->getService(
                    android::String16(SOMEIPPROVIDER_SRV_NAME)
                    )
                )};
            if(someipProviderMgr != nullptr)
            {
                if (android::OK == android::IInterface::asBinder(someipProviderMgr)->linkToDeath(mServiceDeathRecipient)) {
                        /* Create receiver */
                    mProviderReceiver = new SomeipProviderReceiver();
                    errCode = mSomeipProviderMgr->registerReceiverTidlSomeipProviderRxUint8ArrDataToApp(mProviderReceiver, EnumSomeipRxMsgID::kRxInformServiceModeStatusNotification);
                    LOG_I("Registered SomeipProviderMgr receiver");
                }
            }
        }
        if (errCode != E_OK)
        {
            LOG_E("Cannot bind SomeipProviderMgr Service, try again");
            const android::sp<RemotediagHandler> handler{RemotediagHandler::getInstance()};
            if (handler != nullptr)
            {
                (void)handler->sendMessageDelayed(handler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_SOMEIP_MGR), static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
            }
        }
    }

    SomeipProviderReceiver::SomeipProviderReceiver()
    {
        LOG_I("SomeipProviderReceiver created");
    }

    void SomeipProviderReceiver::RxUint8ArrDataToApp(EnumSomeipRxMsgID &kSomeipRxMsgID, size_t &bufSomeipDataLen, uint8_t *bufSomeipData) noexcept
    {
        if (kSomeipRxMsgID == EnumSomeipRxMsgID::kRxInformServiceModeStatusNotification)
        {
            LOG_I("Received kRxInformServiceModeStatusNotification notification");
            if (bufSomeipData != nullptr)
            {
                const android::sp<Buffer> bufStatus{new Buffer()};
                if (bufSomeipDataLen < static_cast<size_t>(INT32_MAX))
                {
                    bufStatus->setTo(bufSomeipData, bufSomeipDataLen);
                }
                else
                {
                    LOG_E("Received data length is too large: %zu", bufSomeipDataLen);
                }
                const android::sp<RemotediagHandler> handler{RemotediagHandler::getInstance()};
                if (handler != nullptr)
                {
                    (void)handler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_SERVICE_MODE_STATUS, bufStatus)->sendToTarget();
                }
            }
            else
            {
                LOG_E("bufSomeipData is nullptr");
            }
        }
    }

} /* End: namespace rdgapp */
