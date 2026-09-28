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
    mCalibMgrService = android::interface_cast<ICalibManagerService>(
                android::defaultServiceManager()->getService(
                    android::String16("service_layer.CalibManagerService")
                    )
                );
    return mCalibMgrService;
}

void CalibManagerAdapter::registerService() {
    LOG_I("Start register CalibManagerAdapter");
    mHandler = RemotediagHandler::getInstance();

    if (mCalibReceiver == nullptr) {
        mCalibReceiver = android::sp<CalibReceiver>(new CalibReceiver(*this));
    }

    (void)getService();
    error_t res {0};
    if (mCalibMgrService != nullptr) {
        LOG_D("CalibManagerAdapter registered");
        const android::status_t result{android::IInterface::asBinder(mCalibMgrService)->linkToDeath(mServiceDeathRecipient)};
        if (result == android::OK) {
            LOG_D("Link to death success");
            // register did below
            // Remove OEM_DID_Under_repair_status DCM24MON-7971
            res |= mCalibMgrService->registerReceiverCalibManagerReceiverOnCalibReceiveDID(mCalibReceiver, 0x1022U);
            const bool AC_Flag{DiagManagerAdapter::getInstance()->getSRVC_AC()};
            const bool STT_Flag{DiagManagerAdapter::getInstance()->getSRVC_STT()};
            LOG_I("AC flag: %d STT flag: %d", AC_Flag, STT_Flag);
            DiagManagerAdapter::getInstance()->setSRVC(true, AC_Flag);
            DiagManagerAdapter::getInstance()->setSRVC(false, STT_Flag);
            res |= mCalibMgrService->registerReceiverCalibManagerReceiverOnCalibReceiveDID(mCalibReceiver, 0x3500U);
            res |= mCalibMgrService->registerReceiverCalibManagerReceiverOnCalibReceiveDID(mCalibReceiver, 0xF190U);
        }
    }
    if (res != 0) {
        LOG_E("Cannot register CalibM Service, try again");
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CALIB_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}

int32_t CalibManagerAdapter::onCalibDidChanged(const uint16_t DID, const size_t bufLen, const uint8_t *const buf) {
    LOG_I("CalibManager DID name: %04X with size: %d", DID, bufLen);
    // handle did changed below
    switch(DID) {
        case 0x3500U:
        {
            LOG_I("Receive Service changed");
            if ((buf != nullptr) && (bufLen == 2U))
            {
                const android::sp<Buffer> spBuf{new Buffer()};
                spBuf->setTo(buf, static_cast<int32_t>(bufLen));
                (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_SERVICE_FLAG_CHANGE, spBuf)->sendToTarget();
            }
            else
            {
                LOG_E("Invalid data");
            }
            break;
        }
        case 0x1022U:
        {
            LOG_I("Receive PPI flag changed");
            if ((buf != nullptr) && (bufLen == 1U))
            {
                const android::sp<Buffer> spBuf{new Buffer()};
                spBuf->setTo(buf, static_cast<int32_t>(bufLen));
                (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_PPI_FLAG_CHANGE, spBuf)->sendToTarget();
            }
            else
            {
                LOG_E("Invalid data");
            }
            break;
        }
        case 0xF190U:
        {
            if((buf != nullptr) && (bufLen == 17U))
            {
                const android::sp<Buffer> spBuf{new Buffer()};
                spBuf->setTo(buf, static_cast<int32_t>(bufLen));
                (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_VIN_CHANGE, spBuf)->sendToTarget();
            }
            else
            {
                LOG_E("Invalid data");
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
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
    NOTUSED(who);
    mCalibMgrService = nullptr;
    mCalibReceiver = nullptr;
    if (mHandler != nullptr) {
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CALIB_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}
}
