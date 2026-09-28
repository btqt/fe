#include "VehicleManagerAdapter.h"

namespace rdgapp {

VehicleManagerAdapter::VehicleManagerAdapter()
{
    mVehicleReceiver = std::move(android::sp<IVehicleReceiver>{new VCMReceiver(*this)});
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who)
                                                       { this->onBinderDied(who); });
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
std::shared_ptr<VehicleManagerAdapter> VehicleManagerAdapter::getInstance()
{
    if (instance == nullptr)
    {
        instance = std::make_shared<VehicleManagerAdapter>();
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
        LOG_E("Cannot register VCM Service, try again after ms: %d", RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_VEHICLE_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
    }
}

error_t VehicleManagerAdapter::getOdoInformation(uint32_t &odo_value, uint32_t &odo_unit)
{
    LOG_V("Get ODO information from Vehicle ManagerService");
    sp<VehicleData> vehicle_data{new VehicleData()};
    uint32_t result_odo{0xFFFFFFFFU};
    uint32_t result_odo_unit{0xFFFFFFFFU};
    if (mVCMService == nullptr)
    {
        mVCMService = android::interface_cast<ICommunicationManagerService>(android::defaultServiceManager()->getService(android::String16("service_layer.CommunicationManagerService")));
    }
    // 2. Getting ODO from vehicle manager
    vehicle_data = mVCMService->getVehicleValue(TOYOTA_24DCM::SigInd_CanRx_ODO);

    if (vehicle_data->valType == TYPE_UINT32)
    {
        result_odo = vehicle_data->valUint32;
    }

    // 2. Getting ODO_UNIT from vehicle manager
    vehicle_data = mVCMService->getVehicleValue(TOYOTA_24DCM::SigInd_CanRx_ODO_UNIT);
    if (vehicle_data->valType == TYPE_UINT32)
    {
        result_odo_unit = vehicle_data->valUint32;
    }

    // If the odometer unit received from odometer master is “Mile”, it shall be converted
    // to “km” with 1mile = 1.61 km or higher accuracy (from time stamp requirement specification)
    if (result_odo_unit == 0x2U)
    {
        /* 01 : km, 10 : mile*/
        // result_odo = result_odo * 1.6;
        LOG_V("ODO info is Mile");
        result_odo = result_odo;
    }
    else if (result_odo_unit == 0x1U)
    {
        LOG_V("ODO info is Km");
        result_odo = result_odo;
    }
    else
    {
        LOG_V("Undefined odo info, set to 0xFFFFFFFF");
        result_odo = 0xFFFFFFFFU;
    }
    odo_value = result_odo;
    odo_unit = result_odo_unit;
    return E_OK;
}

uint32_t VehicleManagerAdapter::getTimeCounter()
{
    uint32_t timeCounter{0U};
    sp<VehicleData> vehicle_data{new VehicleData()};
    if (mVCMService == nullptr)
    {
        mVCMService = android::interface_cast<ICommunicationManagerService>(android::defaultServiceManager()->getService(android::String16("service_layer.CommunicationManagerService")));
    }
    vehicle_data = mVCMService->getVehicleValue(TOYOTA_24DCM::SigInd_CanRx_TIME_CNT);
    if (vehicle_data->valType == TYPE_UINT32)
    {
        timeCounter = vehicle_data->valUint32;
    }
    LOG_V("valType %d, timeCounter: %llu", vehicle_data->valType, timeCounter);
    return timeCounter;
}

uint16_t VehicleManagerAdapter::getTripCounter()
{
    uint16_t tripCounter{0U};
    sp<VehicleData> vehicle_data{new VehicleData()};
    if (mVCMService == nullptr)
    {
        mVCMService = android::interface_cast<ICommunicationManagerService>(android::defaultServiceManager()->getService(android::String16("service_layer.CommunicationManagerService")));
    }
    vehicle_data = mVCMService->getVehicleValue(TOYOTA_24DCM::SigInd_CanRx_TRIP_CNT);
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

// void VehicleManagerAdapter::onReceivedCanSignal(const uint32_t channel, const android::sp<VehicleData> &vehicleData) const noexcept
// {
//     // NOTUSED(channel);
//     // const sp<sl::Message> message { mHandler->obtainMessage(ENUM_TO_INT(msg::INTERNAL_MSG_VEHICLE_MANAGER_SRV::RECV_MSG_VIF), vehicleData) };
//     // (void)(message->sendToTarget());
// }

// void VehicleManagerAdapter::sendVehicleData(const uint32_t channel, const android::sp<VehicleData> data) const noexcept
// {
//     // if (mVCMService != nullptr) {
//     //     (void)(mVCMService->sendVehicleData(channel, data));
//     // }
// }

void VehicleManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who)
{
    LOG_I("VehicleManagerAdapter::onBinderDied");
    NOTUSED(who);
    mVCMService = nullptr;
    mVehicleReceiver = nullptr;
    (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_VEHICLE_MGR), RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
}

void VCMReceiver::onReceived(const uint32_t channel, const sp<VehicleData> &vehicleData)
{
    LOG_I("Forward CAN signal to RemoteWarning");
    if ((channel == data_channel::CAN) || (channel == data_channel::ETHERNET))
    {
        RemoteWarning::getInstance()->onReceivedCanSignal(channel, vehicleData);
    }
}

void VCMReceiver::onReceiveTimeout(const uint32_t channel, const sp<VehicleData> &vehicleData)
{
    LOG_I("Forward CAN signal timerout to RemoteWarning");
    if ((channel == data_channel::CAN) || (channel == data_channel::ETHERNET))
    {
        RemoteWarning::getInstance()->onReceivedCanSignalTimeout(channel, vehicleData);
    }
}

}
