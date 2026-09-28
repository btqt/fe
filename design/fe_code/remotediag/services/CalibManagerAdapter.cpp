#include "CalibManagerAdapter.h"

#include <vector>
#include <utils/Message.h>

#include "utils/ProxyIpcServer.h"
#include "../remotediagproxy/include/ProxyIpcProtocol.h"

namespace rdgapp {

// CallbackHandler implementation
CalibManagerAdapter::CallbackHandler::CallbackHandler(CalibManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("CalibManagerAdapter::CallbackHandler: created");
}

void CalibManagerAdapter::CallbackHandler::initialize() {
    // Register this handler for Calib callbacks
    std::vector<uint32_t> callbackIds = {
        static_cast<uint32_t>(rdgipc::CallbackId::CalibServiceFlagChanged),
        static_cast<uint32_t>(rdgipc::CallbackId::CalibPpiFlagChanged),
        static_cast<uint32_t>(rdgipc::CallbackId::CalibVinChanged)
    };
    
    rdgapp::ProxyIpcServer& server = rdgapp::ProxyIpcServer::getInstance();
    server.registerCallbackHandler(shared_from_this(), callbackIds);
    LOG_I("CalibManagerAdapter::CallbackHandler: registered %zu callbacks", callbackIds.size());
}

void CalibManagerAdapter::CallbackHandler::handle(uint32_t callbackId, const std::vector<uint8_t>& payload) {
    LOG_I("CalibManagerAdapter::CallbackHandler::handle id=%u payloadSize=%zu",
          callbackId, payload.size());

    const android::sp<RemotediagHandler> handler{mAdapter->mHandler};
    if (handler == nullptr) {
        LOG_W("CalibManagerAdapter: callback dropped no handler callbackId=%u", callbackId);
        return;
    }

    if (callbackId == static_cast<uint32_t>(rdgipc::CallbackId::CalibServiceFlagChanged)) {
        if (payload.size() != 2U) {
            LOG_W("CalibManagerAdapter: invalid service flag payload size=%zu", payload.size());
            return;
        }
        const android::sp<Buffer> spBuf{new Buffer()};
        spBuf->setTo(payload.data(), static_cast<int32_t>(payload.size()));
        (void)handler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_SERVICE_FLAG_CHANGE, spBuf)->sendToTarget();
        return;
    }

    if (callbackId == static_cast<uint32_t>(rdgipc::CallbackId::CalibPpiFlagChanged)) {
        if (payload.size() != 1U) {
            LOG_W("CalibManagerAdapter: invalid ppi flag payload size=%zu", payload.size());
            return;
        }
        const android::sp<Buffer> spBuf{new Buffer()};
        spBuf->setTo(payload.data(), static_cast<int32_t>(payload.size()));
        (void)handler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_PPI_FLAG_CHANGE, spBuf)->sendToTarget();
        return;
    }

    if (callbackId == static_cast<uint32_t>(rdgipc::CallbackId::CalibVinChanged)) {
        if (payload.size() != 17U) {
            LOG_W("CalibManagerAdapter: invalid vin payload size=%zu", payload.size());
            return;
        }
        const android::sp<Buffer> spBuf{new Buffer()};
        spBuf->setTo(payload.data(), static_cast<int32_t>(payload.size()));
        (void)handler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_VIN_CHANGE, spBuf)->sendToTarget();
        return;
    }
}

CalibManagerAdapter::CalibManagerAdapter() noexcept {
    mCallbackHandler = std::make_shared<CallbackHandler>(this);
    mCallbackHandler->initialize();
    LOG_I("CalibManagerAdapter Constructor");
}

CalibManagerAdapter::~CalibManagerAdapter() noexcept {
    if(CalibManagerAdapter::instance != nullptr) {
        CalibManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<CalibManagerAdapter> CalibManagerAdapter::instance{nullptr};
android::Mutex CalibManagerAdapter::mInstanceLock{};
std::shared_ptr<CalibManagerAdapter> CalibManagerAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<CalibManagerAdapter>();
        }
    }
    return instance;
}

android::sp<ICalibManagerService> CalibManagerAdapter::getService() {
    return nullptr;
}

void CalibManagerAdapter::registerService() {
    LOG_I("Start register CalibManagerAdapter");
    mHandler = RemotediagHandler::getInstance();

    std::vector<uint8_t> response{};
    const bool requestOk{ProxyIpcServer::getInstance().requestAPICall(
        rdgipc::CommandId::CalibRegisterDidWatch,
        {},
        response,
        2000U)};
    if (!requestOk) {
        LOG_E("Cannot register CalibM Service, try again");
        if (mHandler != nullptr) {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CALIB_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        }
        return;
    }

    const bool AC_Flag{DiagManagerAdapter::getInstance()->getSRVC_AC()};
    const bool STT_Flag{DiagManagerAdapter::getInstance()->getSRVC_STT()};
    LOG_I("AC flag: %d STT flag: %d", AC_Flag, STT_Flag);
    DiagManagerAdapter::getInstance()->setSRVC(true, AC_Flag);
    DiagManagerAdapter::getInstance()->setSRVC(false, STT_Flag);
}

void CalibManagerAdapter::onBinderDied(const android::wp<android::IBinder>& who) {
    LOG_I("CalibManagerAdapter::onBinderDied");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
    NOTUSED(who);
    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CALIB_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}
}
