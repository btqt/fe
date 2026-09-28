#include "VehicleManagerAdapter.h"

namespace rdgapp {

VehicleManagerAdapter::VehicleManagerAdapter()
{
    mVehicleReceiver = std::move(android::sp<IVehicleReceiver>{new VCMReceiver(*this)});
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who)
                                                       { this->onBinderDied(who); });
    mOdoValue = 0xFFFFFFFFU;
    mOdoUnit = 0x01U;
}

VehicleManagerAdapter::~VehicleManagerAdapter()
{
    mVCMService = nullptr;
    mVehicleReceiver = nullptr;
    mServiceDeathRecipient = nullptr;
    if (VehicleManagerAdapter::instance != nullptr)
    {
        VehicleManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<VehicleManagerAdapter> VehicleManagerAdapter::instance{nullptr};
android::Mutex VehicleManagerAdapter::mInstanceLock{};
std::shared_ptr<VehicleManagerAdapter> VehicleManagerAdapter::getInstance()
{
    if (instance == nullptr)
    {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr)
        {
            instance = std::make_shared<VehicleManagerAdapter>();
        }
    }
    return instance;
}

void VehicleManagerAdapter::registerService()
{
    mHandler = RemotediagHandler::getInstance();

    if (mVCMService != nullptr)
    {
        mVCMService = nullptr;
    }

    if (mVehicleReceiver == nullptr)
    {
        mVehicleReceiver = std::move(android::sp<IVehicleReceiver>{new VCMReceiver(*this)});
    }
    mVCMService = android::interface_cast<ICommunicationManagerService>(
        android::defaultServiceManager()->getService(
            android::String16("service_layer.CommunicationManagerService")));

    bool error{true};
    if (mVCMService != nullptr)
    {
        if (android::OK == android::IInterface::asBinder(mVCMService)->linkToDeath(mServiceDeathRecipient))
        {
            const uint32_t moduleId{static_cast<uint32_t>(getpid())}; // Change to getpid to avoid dupplicate with another module
            if (mVCMService->registerVehicleReceiver(moduleId, mVehicleReceiver) == android::OK)
            {
                LOG_I("Registed Vehicle Manager Service with PID: %d", moduleId);
                error = false;
                if (mVCMService->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S02) == android::OK)
                {
                    LOG_I("Registed MsgInd_CanRx_MET1S02");
                }
                if (mVCMService->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S34) == android::OK)
                {
                    LOG_I("Registed MsgInd_CanRx_MET1S34");
                }

                if (mVCMService->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S35) == android::OK)
                {
                    LOG_I("Registed MsgInd_CanRx_MET1S35");
                }

                if (mVCMService->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S36) == android::OK)
                {
                    LOG_I("Registed MsgInd_CanRx_MET1S36");
                }

                if (mVCMService->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S37) == android::OK)
                {
                    LOG_I("Registed MsgInd_CanRx_MET1S37");
                }

                if (mVCMService->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ENG1G90) == android::OK)
                {
                    // ENG STATE : ASTNRM / RDYIND
                    LOG_I("Registed MsgInd_CanRx_ENG1G90");
                }

                //Register Timeout
                if (mVCMService->addFrameIndexForTimeout(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S02) == android::OK)
                {
                    LOG_I("Registed MsgInd_CanRx_MET1S02");
                }

                if (mVCMService->addFrameIndexForTimeout(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S34) == android::OK)
                {
                    LOG_I("Registed Timeout MsgInd_CanRx_MET1S34");
                }

                if (mVCMService->addFrameIndexForTimeout(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S35) == android::OK)
                {
                    LOG_I("Registed Timeout MsgInd_CanRx_MET1S35");
                }

                if (mVCMService->addFrameIndexForTimeout(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S36) == android::OK)
                {
                    LOG_I("Registed Timeout MsgInd_CanRx_MET1S36");
                }

                if (mVCMService->addFrameIndexForTimeout(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S37) == android::OK)
                {
                    LOG_I("Registed Timeout MsgInd_CanRx_MET1S37");
                }

            }
        }
    }
    if (error)
    {
        LOG_E("Cannot register VCM Service, try again after 500ms");
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_VEHICLE_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}

android::sp<ICommunicationManagerService> VehicleManagerAdapter::getService() const
{
    return android::interface_cast<ICommunicationManagerService>(android::defaultServiceManager()->getService(android::String16("service_layer.CommunicationManagerService")));
}

error_t VehicleManagerAdapter::getOdoInformation(uint32_t &odo_value, uint32_t &odo_unit)
{
    const Mutex::Autolock lock{Mutex::Autolock(mOdoInfo)};
    odo_value = mOdoValue;
    odo_unit = mOdoUnit;
    LOG_V("getOdoInformation, mOdoValue %u, mOdoUnit %u", mOdoValue, mOdoUnit);
    return E_OK;
}

uint32_t VehicleManagerAdapter::getTimeCounter()
{
    uint32_t timeCounter{0U};
    sp<VehicleData> vehicle_data{new VehicleData()};
    const android::sp<ICommunicationManagerService> vehicleMgr {getService()};
    if  (vehicleMgr != nullptr)
    {
        vehicle_data = vehicleMgr->getVehicleValue(TOYOTA_24DCM::SigInd_CanRx_TIME_CNT);
    }
    if (vehicle_data->valType == TYPE_UINT32)
    {
        timeCounter = vehicle_data->valUint32;
    }
    LOG_V("valType %d, timeCounter: %u", vehicle_data->valType, timeCounter);
    return timeCounter;
}

uint16_t VehicleManagerAdapter::getTripCounter()
{
    uint16_t tripCounter{0U};
    sp<VehicleData> vehicle_data{new VehicleData()};
    const android::sp<ICommunicationManagerService> vehicleMgr {getService()};
    if  (vehicleMgr != nullptr)
    {
        vehicle_data = vehicleMgr->getVehicleValue(TOYOTA_24DCM::SigInd_CanRx_TRIP_CNT);
    }

    if (vehicle_data->valType == TYPE_UINT32)
    {
        const uint32_t tripCounter_t{vehicle_data->valUint32}; // CID 9391458
        if(tripCounter_t <= 65535U)
        {
            tripCounter = static_cast<uint16_t>(tripCounter_t);
        }
        else 
        {
            // do nothing
        }
    }
    LOG_V("valType %d, tripCounter: %u", vehicle_data->valType, tripCounter);
    return tripCounter;
}
void VehicleManagerAdapter::setTimeoutOdoInfo(const uint32_t odoValue, const uint32_t odoUnit)
{
    const Mutex::Autolock lock{Mutex::Autolock(mOdoInfo)};
    mOdoValue = odoValue;
    mOdoUnit = odoUnit;
    LOG_V("setTimeoutOdoInfo, mOdoValue %u, mOdoUnit %u, odoValue, %u, odoUnit %u", mOdoValue, mOdoUnit, odoValue, odoUnit);
}

void VehicleManagerAdapter::handleOdoSignal(const android::sp<VehicleData> vehicleData)
{
    if (vehicleData != nullptr)
    {
        const uint32_t signalId{vehicleData->sigID};
        if (signalId == 0x0611U) //6-DCF-10-02-24DCM Global CAN Data Specification
        {
            uint32_t odoUnit{0U};
            uint32_t odoValue{0U};
            const android::sp<::Buffer> mbuffer {vehicleData->buffer};
            if ((mbuffer != nullptr) && (mbuffer->data() != nullptr) && (mbuffer->size() == 8U))
            {

                odoUnit = static_cast<uint32_t>((static_cast<uint32_t>(mbuffer->data()[3]) >> 4U) & 0x3U);// 7 6 5 4 3 2 1 0 (0011)
                if ((odoUnit == 0x01U) || (odoUnit == 0x02U)) // 01b --> km, 02b --> mile
                {
                    odoValue = static_cast<uint32_t>(static_cast<uint32_t>(static_cast<uint32_t>(mbuffer->data()[4]) << 24U) |
                                                     static_cast<uint32_t>(static_cast<uint32_t>(mbuffer->data()[5]) << 16U) |
                                                     static_cast<uint32_t>(static_cast<uint32_t>(mbuffer->data()[6]) << 8U) |
                                                     static_cast<uint32_t>(mbuffer->data()[7]));
                }
                else
                {
                    odoUnit = 0x01U;  //undefined value
                    odoValue = 0xFFFFFFFFU; //undefined value
                }
            }
            else
            {
                odoUnit = 0x01U;  //undefined value
                odoValue = 0xFFFFFFFFU; //undefined value
            }
            setTimeoutOdoInfo(odoValue, odoUnit);
        }
    }
}

void VehicleManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who)
{
    LOG_I("VehicleManagerAdapter::onBinderDied");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
    NOTUSED(who);
    mVCMService = nullptr;
    mVehicleReceiver = nullptr;
    (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_VEHICLE_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
}

void VCMReceiver::onReceived(const uint32_t channel, const sp<VehicleData> &vehicleData)
{
    // LOG_I("Forward CAN signal to RemoteWarning");
    if ((channel == data_channel::CAN) || (channel == data_channel::ETHERNET))
    {
        RemoteWarning::getInstance()->onReceivedCanSignal(channel, vehicleData);
    }
}

void VCMReceiver::onReceiveTimeout(const uint32_t channel, const sp<VehicleData> &vehicleData)
{
    // LOG_I("Forward CAN signal timerout to RemoteWarning");
    if ((channel == data_channel::CAN) || (channel == data_channel::ETHERNET))
    {
        RemoteWarning::getInstance()->onReceivedCanSignalTimeout(channel, vehicleData);
    }
}

}
