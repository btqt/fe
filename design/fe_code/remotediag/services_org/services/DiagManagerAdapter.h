#ifndef REMOTEDIAG_DIAGMANAGERADAPTER_H
#define REMOTEDIAG_DIAGMANAGERADAPTER_H

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <functional>
#include <utils/Message.h>
#include <services/DiagManagerService/IDiagManagerService.h>
#include <services/DiagManagerService/IDiagManagerServiceType.h>
#include <services/DiagManagerService/IDiagManagerReceiver.h>
#include <services/DiagManagerService/DiagDtcItemList.h>
#include <services/DiagManagerService/DiagType.h>
#include <services/DiagManagerService/OEM_DiagData_Config.h>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <Error.h>

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../utils/ServiceDeathRecipient.h"
#include "../include/ParamsDef.h"
#include "CommonUtils.h"
#include "diagprocess/RoBOccurrence/RoBOccurrence.h"
#include "diagprocess/RoBOccurrence/OccurrentRobNotification.h"

namespace rdgapp {

class RemotediagHandler;
class DiagManagerAdapter
{
    class DiagMReceiver : public BnDiagManagerReceiver
    {
    public:
        DiagMReceiver(DiagManagerAdapter &sDiagMRC) noexcept : diagMRC(sDiagMRC) {}
        virtual ~DiagMReceiver() = default;
        DiagMReceiver(DiagMReceiver const &) = default;
        DiagMReceiver &operator=(DiagMReceiver const &) = default;
        DiagMReceiver(DiagMReceiver &&) = delete;
        DiagMReceiver &operator=(DiagMReceiver &&) = delete;
        virtual void onReceive(sp<DiagData> &diagData)
        {
            diagMRC.onReceivedDiagReq(diagData);
            LOG_I("DiagMReceiver::onReceive");
        }
        virtual void onClearDiagInfo(uint8_t const order)
        {
            diagMRC.onClearDiagInfo(order);
            LOG_I("DiagMReceiver::onClearDiagInfo");
        }

    private:
        DiagManagerAdapter &diagMRC;
    };

public:
    DiagManagerAdapter();
    virtual ~DiagManagerAdapter();
    DiagManagerAdapter(DiagManagerAdapter const &) = default;
    DiagManagerAdapter &operator=(DiagManagerAdapter const &) = default;
    DiagManagerAdapter(DiagManagerAdapter &&) = delete;
    DiagManagerAdapter &operator=(DiagManagerAdapter &&) = delete;
    static std::shared_ptr<DiagManagerAdapter> getInstance();
    void registerService();

    // DID
    void writeDidData(const uint16_t did, sp<::Buffer> didData);
    void readDidData(const uint16_t did, sp<::Buffer> &didData);

    android::sp<IDiagManagerService> getService() const;
    uint8_t getUnderRepairStatus();
    uint8_t getRDGFlag();
    uint8_t getDTCFlag();
    uint8_t getSSRFlag();
    uint8_t getWARflag();
    uint8_t getRoBflag();
    uint8_t getDDRflag();
    bool getAllUploadConsent();
    std::string getVinNumber();
    bool getLocationUploadConsent();
    bool getSRVC_AC();
    bool getSRVC_VC();
    bool getSRVC_PC();
    bool getSRVC_STT();
    void saveRDGFlag(const bool isActive);
    void selfDiagIgOnOffTimes(const bool isIgOn, const uint16_t type, const android::sp<::Buffer> &timeData);
    void selfDiagCollectionCondition(const uint64_t collectionConditionId);
    void selfDiagSuccessReadNotification(const uint8_t triggerType);
    void selfDiagSuccessCreateFile(const uint8_t triggerType);
    void selfDiagStopOpeartion(const uint8_t operation);
    void selfDiagNoCenterResponse();
    void storeCollectionConditionId(const uint64_t collectionConditionId);
    void selfDiagEcuUserDefMemoryDTC(const android::sp<OccurrentRobNotification> notification);
    void setSRVC(const bool isACFlag, const bool flag);
    void setUnderRepairStatus(const android::sp<Buffer> status);

private:
    static std::shared_ptr<DiagManagerAdapter> instance;
    android::sp<RemotediagHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<IDiagManagerReceiver> mDiagReceiver = nullptr;
    android::sp<IDiagManagerService> mDiagMService = nullptr;
    mutable android::Mutex mDiedLock;
    static android::Mutex mInstanceLock;

public:
    void onBinderDied(const android::wp<android::IBinder> &who);
    static void onReceivedDiagReq(const sp<DiagData> &diagData);
    static void onSendDiagData(const sp<DiagData> diagData);
    static void onClearDiagInfo(const uint8_t order);
    constexpr static uint16_t SELFDIAG_DID_TYPE_10_TIMES{10U};
    constexpr static uint16_t SELFDIAG_DID_TYPE_280_TIMES{280U};
    constexpr static uint8_t UNDER_REPAIR_STATUS_DEFAULT{0U};
    // constexpr static uint8_t CAL_REMOTEDIAG_SERVICE_FLAG{0U};
    // constexpr static uint8_t CAL_TELEMATIC_FLAGS{1U};
    // constexpr static int8_t DTC_FULL_COUNT_PASS{-128};
    // constexpr static int8_t DTC_FULL_COUNT_FAIL{127};
    // constexpr static uint16_t PARAM_DID_SERVICE_FLAG{0x1021U};
    // constexpr static uint16_t PARAM_DID_SETTING_FLAG{0x1023U};
    constexpr static uint16_t PARAM_DID_VIN_NUMBER{0xF190U};
    // constexpr static uint16_t PARAM_DID_PHASE_5_SOFTWARE_PART_NO{0xF181U};
    // constexpr static uint16_t PARAM_DID_PHASE_6_SOFTWARE_PART_NO{0xF188U};
    // constexpr static uint16_t PARAM_DID_PHASE_5_HARDWARE_PART_NO{0x0105U};
    // constexpr static uint16_t PARAM_DID_PHASE_6_HARDWARE_PART_NO{0xF191U};
    constexpr static uint16_t PARAM_DID_PPI_CONSENT_STATE{0x1022U};

    // Operation
    constexpr static uint8_t COLLECTION_CONDITIONS{0x00U};
    constexpr static uint8_t WARINING_TRIGGER{0x01U};
    constexpr static uint8_t ROB_NOTIFICATION_TRIGGER{0x02U};
    // constexpr static uint8_t DDR_TRIGGER{0x03U};
    constexpr static uint8_t RD_SCHEDULE_TRIGGER{0x04U};
    constexpr static uint8_t IG_ON_TRIGGER{0x05U};
    // constexpr static uint8_t MEMORY_ACCESS_FAILURE{0x11U};
    // constexpr static uint8_t STORAGE_ACCESS_FAILURE{0x12U};
    // constexpr static uint8_t CORRUPT_DATA_ACQUISITION{0x13U};
    constexpr static uint8_t FAILURE_ACQUIRE_ECU_LIST{0x14U};

    // RDG owner's DIDs
    constexpr static uint16_t PARAM_DID_IG_ON_OFF_TIME{0x3000U};
    constexpr static uint16_t PARAM_DID_ALL_IG_ON_OFF_TIMES{0x3001U};

    constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_1{0x3002U};
    constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_32{0x3021U};

    constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_1{0x3102U};
    constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_32{0x3121U};

    constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_1{0x3202U};
    constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_32{0x3221U};

    constexpr static uint16_t PARAM_DID_STOP_OPERATION_1{0x3302U};
    constexpr static uint16_t PARAM_DID_STOP_OPERATION_32{0x3321U};

    constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_1{0x3402U};
    constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_32{0x3421U};

    constexpr static uint16_t PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG{0x3500U};
    constexpr static uint16_t PARAM_DID_COLLECTION_CONDITIONS{0x3501U};

    constexpr static uint16_t PARAM_DID_DATETIME_OF_ECU_USERDEF_MEMORY_DTC_1{0x3502U};
    constexpr static uint16_t PARAM_DID_DATETIME_OF_ECU_USERDEF_MEMORY_DTC_3{0x3504U};
    constexpr static uint16_t PARAM_DID_DATETIME_OF_ECU_USERDEF_MEMORY_DTC_SIZE{4080U};
};
}
#endif // REMOTEDIAG_DIAGMANAGERADAPTER_H
