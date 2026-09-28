#include "SomeipManagerAdapter.h"

#include <cstring>

#include "../utils/ProxyIpcClient.h"

std::shared_ptr<SomeipManagerAdapter> SomeipManagerAdapter::instance{nullptr};
android::Mutex SomeipManagerAdapter::mInstanceLock{};

SomeipManagerAdapter::SomeipManagerAdapter()
{
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who)
                                                       { this->onBinderDied(who); });
}

SomeipManagerAdapter::~SomeipManagerAdapter() noexcept
{
    if (SomeipManagerAdapter::instance != nullptr)
    {
        SomeipManagerAdapter::instance = nullptr;
    }
}

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
    LOG_I("SomeipManagerAdapter::onBinderDied");
    NOTUSED(who);
    const android::sp<RemotediagProxyHandler> handler{RemotediagProxyHandler::getInstance()};
    if (handler != nullptr)
    {
        (void)handler->sendMessageDelayed(handler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_SOMEIP_MGR),
                                          static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
    }
}

void SomeipManagerAdapter::registerService()
{
    mSomeipProviderMgr = SomeipProviderManager::instance();
    error_t errCode{E_ERROR};
    if (mSomeipProviderMgr != nullptr)
    {
        const android::sp<ISomeipProviderManagerService> someipProviderMgr{android::interface_cast<ISomeipProviderManagerService>(
            android::defaultServiceManager()->getService(android::String16(SOMEIPPROVIDER_SRV_NAME)))};
        if (someipProviderMgr != nullptr)
        {
            if (android::OK == android::IInterface::asBinder(someipProviderMgr)->linkToDeath(mServiceDeathRecipient))
            {
                mProviderReceiver = new SomeipProviderReceiver();
                errCode = mSomeipProviderMgr->registerReceiverTidlSomeipProviderRxUint8ArrDataToApp(
                    mProviderReceiver,
                    EnumSomeipRxMsgID::kRxInformServiceModeStatusNotification);
                LOG_I("SomeipManagerAdapter: registered SomeipProvider receiver");
            }
        }
    }

    if (errCode != E_OK)
    {
        LOG_E("SomeipManagerAdapter: cannot bind SomeipProviderMgr service, try again");
        const android::sp<RemotediagProxyHandler> handler{RemotediagProxyHandler::getInstance()};
        if (handler != nullptr)
        {
            (void)handler->sendMessageDelayed(handler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_SOMEIP_MGR),
                                              static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
    }
}

void SomeipProviderReceiver::RxUint8ArrDataToApp(EnumSomeipRxMsgID &kSomeipRxMsgID,
                                                 size_t &bufSomeipDataLen,
                                                 uint8_t *bufSomeipData) noexcept
{
    if (kSomeipRxMsgID != EnumSomeipRxMsgID::kRxInformServiceModeStatusNotification)
    {
        return;
    }

    LOG_I("SomeipProviderReceiver: received service mode status notification");
    if (bufSomeipData == nullptr)
    {
        LOG_E("SomeipProviderReceiver: bufSomeipData is nullptr");
        return;
    }

    std::vector<uint8_t> payload(bufSomeipDataLen);
    if (bufSomeipDataLen > 0U)
    {
        std::memcpy(payload.data(), bufSomeipData, bufSomeipDataLen);
    }

    (void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::SomeipServiceModeStatus), payload);
}
