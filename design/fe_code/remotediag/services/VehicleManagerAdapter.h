#ifndef REMOTEDIAG_VEHICLEMANAGERADAPTER_H
#define REMOTEDIAG_VEHICLEMANAGERADAPTER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <utils/Message.h>
#include <services/CommunicationManagerService/ICommunicationManagerServiceType.h>
#include <services/CommunicationManagerService/Toyota_24dcmDataIndex.h>

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../include/ParamsDef.h"
#include "../remotediagproxy/include/IpcMessageHandler.h"

namespace rdgapp {

/* Refer to [24ABG-24DCM-LOCAL_CAN-S00-02.xlsx] */
// enum class LOCAL_CAN_ID : uint32_t
// {
//     ABG1S02 = 0x058B,
//     ABG1S03 = 0x058C,
//     ABG1S05 = 0x03AB,
//     ABG1S06 = 0x03AC,
//     ABG1S07 = 0x03AD,
// };

/* Refer to [6-DCF-10-02_24DCM Global CAN Data Specification_V.01.01.00.xlsx] */
// enum class GLOBAL_CAN_ID : uint32_t
// {
//     VSC1H14 = 0x006D,
//     EHV1H49 = 0x020B,
//     EHV1H84 = 0x03D0,
//     ABG1H05 = 0x029F,
//     ABG1H06 = 0x02A9,
//     ADU11H59 = 0x03B6,
//     ZN11H25 = 0x0306,
// };

static std::unordered_map<uint32_t, uint32_t> CAN_DATA_TABLE{
    /* Message label                                  CAN ID */
    /* Global CAN */
    // {TOYOTA_24DCM::SigInd_Rx::SigInd_CanRx_TAIL, static_cast<uint32_t>(GLOBAL_CAN_ID::ZN11H25)},
    // {TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ABG1D72, static_cast<uint32_t>(GLOBAL_CAN_ID::VSC1H14)},
    // { TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_EHV1H49,  static_cast<uint32_t>(GLOBAL_CAN_ID::EHV1H49)  },
    // {TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_EHV1H84, static_cast<uint32_t>(GLOBAL_CAN_ID::EHV1H84)},
    // { TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ABG1H05,  static_cast<uint32_t>(GLOBAL_CAN_ID::ABG1H05)  },
    // { TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ABG1H06,  static_cast<uint32_t>(GLOBAL_CAN_ID::ABG1H06)  },
    // { TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ADU11H59, static_cast<uint32_t>(GLOBAL_CAN_ID::ADU11H59) },

    /* Local CAN */
    // {TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ABG1S02, static_cast<uint32_t>(LOCAL_CAN_ID::ABG1S02)},
    // {TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ABG1S03, static_cast<uint32_t>(LOCAL_CAN_ID::ABG1S03)},
    // {TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ABG1S05, static_cast<uint32_t>(LOCAL_CAN_ID::ABG1S05)},
    // {TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ABG1S06, static_cast<uint32_t>(LOCAL_CAN_ID::ABG1S06)},
    // {TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ABG1S07, static_cast<uint32_t>(LOCAL_CAN_ID::ABG1S07)},
};

class RemotediagHandler;
class VehicleManagerAdapter
{
public:
    VehicleManagerAdapter();
    ~VehicleManagerAdapter();

    VehicleManagerAdapter(const VehicleManagerAdapter &) = delete;
    VehicleManagerAdapter &operator=(const VehicleManagerAdapter &) = delete;

    VehicleManagerAdapter(VehicleManagerAdapter &&) = delete;
    VehicleManagerAdapter &operator=(VehicleManagerAdapter &&) = delete;

    static std::shared_ptr<VehicleManagerAdapter>getInstance();
    void registerService();
    error_t getOdoInformation(uint32_t &odo_value, uint32_t &odo_unit);
    uint32_t getTimeCounter();
    uint16_t getTripCounter();
    void setTimeoutOdoInfo(const uint32_t odoValue, const uint32_t odoUnit);
    void handleOdoSignal(const android::sp<VehicleData> vehicleData);
    void onSignalReceived(uint32_t channel, uint32_t sigId, const std::vector<uint8_t> &bufferBytes);
    void onSignalTimeout(uint32_t channel, uint32_t sigId, const std::vector<uint8_t> &bufferBytes);

private:
    // Nested callback handler for IPC
    class CallbackHandler : public rdgipc::ICallbackHandler,
                           public std::enable_shared_from_this<CallbackHandler> {
    public:
        explicit CallbackHandler(VehicleManagerAdapter* adapter);
        ~CallbackHandler() override;
        void initialize();
        void handle(uint32_t callbackId, const std::vector<uint8_t> &payload) override;
    private:
        VehicleManagerAdapter* mAdapter;
        void handleSignalReceived(const std::vector<uint8_t> &payload);
        void handleSignalTimeout(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CallbackHandler> mCallbackHandler;

private:
    static std::shared_ptr<VehicleManagerAdapter> instance;
    android::sp<RemotediagHandler> mHandler = nullptr;
    mutable android::Mutex mOdoInfo;
    uint32_t mOdoValue;
    uint32_t mOdoUnit;
    static android::Mutex mInstanceLock;
};
}
#endif // REMOTEDIAG_VEHICLEMANAGERADAPTER_H
