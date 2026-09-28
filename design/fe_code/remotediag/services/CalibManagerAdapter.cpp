#include "CalibManagerAdapter.h"

namespace rdgapp {

CalibManagerAdapter::CalibManagerAdapter() noexcept {
    LOG_I("CalibManagerAdapter Constructor");
    mServiceDeathRecipient = new ServiceDeathRecipient( [this] ( const android::wp<android::IBinder>& who ){
        this->onBinderDied(who);
    });
}

CalibManagerAdapter::~CalibManagerAdapter() noexcept {
    if(CalibManagerAdapter::instance != nullptr) {
        CalibManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<CalibManagerAdapter> CalibManagerAdapter::instance{nullptr};
std::shared_ptr<CalibManagerAdapter> CalibManagerAdapter::getInstance() {
    if (instance == nullptr) {
        instance = std::make_shared<CalibManagerAdapter>();
    }
    return instance;
}

android::sp<ICalibManagerService> CalibManagerAdapter::getService() {
    if (mCalibMgrService == nullptr) {
        mCalibMgrService = android::interface_cast<ICalibManagerService>(
                    android::defaultServiceManager()->getService(
                        android::String16("service_layer.CalibManagerService")
                        )
                    );
    }
    return mCalibMgrService;
}

void CalibManagerAdapter::registerService() {
    LOG_I("Start register CalibManagerAdapter");
    mHandler = RemotediagHandler::getInstance_2();

    if (mCalibReceiver == nullptr) {
        mCalibReceiver = android::sp<CalibReceiver>(new CalibReceiver(*this));
    }

    (void)getService();
    bool error{false};
    if (mCalibMgrService != nullptr) {
        LOG_D("CalibManagerAdapter registered");
        const android::status_t result{android::IInterface::asBinder(mCalibMgrService)->linkToDeath(mServiceDeathRecipient)};
        if (result == android::OK) {
            LOG_D("Link to death success");
            // register did below
            error_t res{E_ERROR};
            res = mCalibMgrService->registerReceiverCalibManagerReceiverOnCalibReceiveDID(mCalibReceiver, static_cast<uint16_t>(OEM_DID_Under_repair_status));
            if ( res!= E_OK) {
                error = true;
            }
            res = mCalibMgrService->registerReceiverCalibManagerReceiverOnCalibReceiveDID(mCalibReceiver, 0x1022U);
            const bool AC_Flag{DiagManagerAdapter::getInstance()->getSRVC_AC()};
            const bool STT_Flag{DiagManagerAdapter::getInstance()->getSRVC_STT()};
            LOG_I("AC flag: %d STT flag: %d", AC_Flag, STT_Flag);
            DiagManagerAdapter::getInstance()->setSRVC(true, AC_Flag);
            DiagManagerAdapter::getInstance()->setSRVC(false, STT_Flag);
            (void)AC_Flag;
            (void)STT_Flag;
            if( res!= E_OK) {
                error = true;
            }
            res = mCalibMgrService->registerReceiverCalibManagerReceiverOnCalibReceiveDID(mCalibReceiver, 0x3500U);
            if ( res!= E_OK) {
                error = true;
            }
        }
    }
    if (error) {
        LOG_E("Cannot register CalibM Service, try again after ms: %d", RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CALIB_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}

int32_t CalibManagerAdapter::onCalibDidChanged(const uint16_t DID, const size_t bufLen, const uint8_t *const buf) {
    LOG_I("CalibManager DID name: %04X with size: %d", DID, bufLen);
    // handle did changed below
    switch(DID) {
        case OEM_DID_Under_repair_status:
        {
            LOG_I("OEM_DID_Under_repair_status");
            const uint8_t data{buf[0]};
            (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_SERVICE_MODE_STATUS, static_cast<int32_t>(data))->sendToTarget();
            break;
        }
        case 0x3500U:
        {
            LOG_I("Receive Service changed");
            if (bufLen == 2U)
            {
                const android::sp<Buffer> spBuf{new Buffer()};
                spBuf->setTo(buf, static_cast<int32_t>(bufLen));
                (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_SERVICE_FLAG_CHANGE, spBuf)->sendToTarget();
            }
            else
            {
                LOG_E("Invalid data length %lu", bufLen);
            }
            break;
        }
        case 0x1022U:
        {
            LOG_I("Receive PPI flag changed");
            if (bufLen == 1U)
            {
                const android::sp<Buffer> spBuf{new Buffer()};
                spBuf->setTo(buf, static_cast<int32_t>(bufLen));
                (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_PPI_FLAG_CHANGE, spBuf)->sendToTarget();
            }
            else
            {
                LOG_E("Invalid data length %lu", bufLen);
            }
            break;
        }
        default:
        {
            break;
        }
    }
    return 0;
}

void CalibManagerAdapter::onBinderDied(const android::wp<android::IBinder>& who) {
    LOG_I("CalibManagerAdapter::onBinderDied");
    NOTUSED(who);
    mCalibMgrService = nullptr;
    mCalibReceiver = nullptr;
    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CALIB_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}
}
