#include "SomeipManagerAdapter.h"
#include <utils/Message.h>
#include "utils/ProxyIpcServer.h"

namespace rdgapp
{
    // CallbackHandler implementation
    SomeipManagerAdapter::CallbackHandler::CallbackHandler(SomeipManagerAdapter* adapter)
        : mAdapter(adapter) {
        LOG_I("SomeipManagerAdapter::CallbackHandler: created");
    }

    void SomeipManagerAdapter::CallbackHandler::initialize() {
        // Register this handler for SOMEIP callbacks
        std::vector<uint32_t> callbackIds = {
            static_cast<uint32_t>(rdgipc::CallbackId::SomeipServiceModeStatus)
        };
        
        rdgapp::ProxyIpcServer& server = rdgapp::ProxyIpcServer::getInstance();
        server.registerCallbackHandler(shared_from_this(), callbackIds);
        LOG_I("SomeipManagerAdapter::CallbackHandler: registered %zu callbacks", callbackIds.size());
    }

    void SomeipManagerAdapter::CallbackHandler::handle(uint32_t callbackId, const std::vector<uint8_t>& payload) {
        LOG_I("SomeipManagerAdapter::CallbackHandler::handle id=%u payloadSize=%zu",
              callbackId, payload.size());

        if (callbackId == static_cast<uint32_t>(rdgipc::CallbackId::SomeipServiceModeStatus)) {
            if (payload.empty()) {
                LOG_W("SomeipManagerAdapter: empty SOMEIP service mode callback payload");
                return;
            }

            const android::sp<RemotediagHandler> handler{RemotediagHandler::getInstance()};
            if (handler == nullptr) {
                LOG_W("SomeipManagerAdapter: callback dropped no handler");
                return;
            }

            const android::sp<Buffer> spBuf{new Buffer()};
            spBuf->setTo(payload.data(), static_cast<int32_t>(payload.size()));
            (void)handler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_SERVICE_MODE_STATUS, spBuf)->sendToTarget();
            return;
        }
    }

    SomeipManagerAdapter::SomeipManagerAdapter()
    {
        mCallbackHandler = std::make_shared<CallbackHandler>(this);
        mCallbackHandler->initialize();
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
        LOG_I("SomeipManagerAdapter::registerService skipped (proxy side handles SOMEIP registration)");
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
