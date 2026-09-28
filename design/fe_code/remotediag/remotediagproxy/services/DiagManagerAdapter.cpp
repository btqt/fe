#include "DiagManagerAdapter.h"

#include <utils/Message.h>
#include "../utils/ProxyIpcClient.h"

std::shared_ptr<DiagManagerAdapter> DiagManagerAdapter::instance{nullptr};
android::Mutex DiagManagerAdapter::mInstanceLock{};

DiagManagerAdapter::DiagManagerAdapter() {
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who) {
        this->onBinderDied(who);
    });
    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
}

DiagManagerAdapter::~DiagManagerAdapter() noexcept {
    if (DiagManagerAdapter::instance != nullptr) {
        DiagManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<DiagManagerAdapter> DiagManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<DiagManagerAdapter>();
        }
    }
    return instance;
}

void DiagManagerAdapter::registerService() {
    mHandler = RemotediagProxyHandler::getInstance();
    if (!ensureServiceReady()) {
        LOG_W("DiagManagerAdapter: register failed, retry");
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(
                mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_DIAG_MGR),
                static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
    }
}

android::sp<IDiagManagerService> DiagManagerAdapter::getService() const {
    return android::interface_cast<IDiagManagerService>(
        android::defaultServiceManager()->getService(android::String16(DIAG_SRV_NAME)));
}

bool DiagManagerAdapter::ensureServiceReady() {
    android::sp<IDiagManagerService> service{nullptr};
    {
        const android::Mutex::Autolock lock{mServiceLock};
        service = mDiagMService;
    }

    if (service == nullptr) {
        service = getService();
        if (service == nullptr) {
            return false;
        }

        const android::status_t result{android::IInterface::asBinder(service)->linkToDeath(mServiceDeathRecipient)};
        if (result != android::OK) {
            LOG_E("DiagManagerAdapter: linkToDeath failed: %d", static_cast<int32_t>(result));
            return false;
        }

        const android::Mutex::Autolock lock{mServiceLock};
        mDiagMService = service;
    }

    return true;
}

bool DiagManagerAdapter::writeDidData(const uint16_t did, const std::vector<uint8_t> &didData) {
    for (int32_t i{0}; i < 3; ++i) {
        if (!ensureServiceReady()) {
            continue;
        }

        android::sp<IDiagManagerService> service{nullptr};
        {
            const android::Mutex::Autolock lock{mServiceLock};
            service = mDiagMService;
        }
        if (service == nullptr) {
            continue;
        }

        android::sp<Buffer> spBuf{new Buffer()};
        if (!didData.empty()) {
            spBuf->setTo(didData.data(), static_cast<int32_t>(didData.size()));
        }

        if (service->writeDidInternalBySource(1U, did, spBuf) == 0x00U) {
            return true;
        }
    }

    return false;
}

bool DiagManagerAdapter::readDidData(const uint16_t did, std::vector<uint8_t> &didData) {
    didData.clear();

    for (int32_t i{0}; i < 3; ++i) {
        if (!ensureServiceReady()) {
            continue;
        }

        android::sp<IDiagManagerService> service{nullptr};
        {
            const android::Mutex::Autolock lock{mServiceLock};
            service = mDiagMService;
        }
        if (service == nullptr) {
            continue;
        }

        android::sp<Buffer> spBuf{new Buffer()};
        service->readDidInternalBySource(1U, did, spBuf);
        if ((spBuf != nullptr) && (spBuf->data() != nullptr) && (spBuf->size() > 0U)) {
            didData.assign(spBuf->data(), spBuf->data() + spBuf->size());
            return true;
        }
    }

    return false;
}

void DiagManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
    NOTUSED(who);
    LOG_I("DiagManagerAdapter::onBinderDied");
    const android::Mutex::Autolock lock{mDiedLock};
    {
        const android::Mutex::Autolock serviceLock{mServiceLock};
        mDiagMService = nullptr;
    }

    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(
            mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_DIAG_MGR),
            static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
    }
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

DiagManagerAdapter::CommandHandler::CommandHandler(DiagManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("DiagManagerAdapter::CommandHandler: created");
}

void DiagManagerAdapter::CommandHandler::initialize() {
    LOG_I("DiagManagerAdapter::CommandHandler: registering with ProxyIpcClient");
    
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::DiagWriteDid),
        static_cast<uint32_t>(rdgipc::CommandId::DiagReadDid)
    };
    
    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("DiagManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    }
}

DiagManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("DiagManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse DiagManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);
    
    if (cmdId == rdgipc::CommandId::DiagWriteDid) {
        if (payload.size() < 2U) {
            return rdgipc::CommandResponse::err(rdgipc::ErrorCode::InvalidPayload);
        }
        const uint16_t did = static_cast<uint16_t>((static_cast<uint16_t>(payload[0]) << 8U) |
                                                     static_cast<uint16_t>(payload[1]));
        std::vector<uint8_t> didPayload;
        if (payload.size() > 2U) {
            didPayload.assign(payload.begin() + 2, payload.end());
        }
        if (!mAdapter->writeDidData(did, didPayload)) {
            return rdgipc::CommandResponse::err(rdgipc::ErrorCode::OperationFailed);
        }
        return rdgipc::CommandResponse::ok("1");
    }
    
    if (cmdId == rdgipc::CommandId::DiagReadDid) {
        if (payload.size() != 2U) {
            return rdgipc::CommandResponse::err(rdgipc::ErrorCode::InvalidPayload);
        }
        const uint16_t did = static_cast<uint16_t>((static_cast<uint16_t>(payload[0]) << 8U) |
                                                     static_cast<uint16_t>(payload[1]));
        std::vector<uint8_t> didValue;
        if (!mAdapter->readDidData(did, didValue)) {
            return rdgipc::CommandResponse::err(rdgipc::ErrorCode::OperationFailed);
        }
        return rdgipc::CommandResponse::ok(didValue);
    }
    
    return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
}
