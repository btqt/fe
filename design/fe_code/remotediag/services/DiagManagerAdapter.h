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

    android::sp<IDiagManagerService> getService();
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
    void selfDiagIgOnOffTimes(const bool isIgOn, const uint16_t type);
    void selfDiagCollectionCondition(const uint64_t collectionConditionId);
    void selfDiagSuccessReadNotification(const uint8_t triggerType);
    void selfDiagSuccessCreateFile(const uint8_t triggerType);
    void selfDiagStopOpeartion(const uint8_t operation);
    void selfDiagNoCenterResponse();
    void storeCollectionConditionId(const uint64_t collectionConditionId);
    void selfDiagEcuUserDefMemoryDTC(const android::sp<OccurrentRobNotification> notification);
    void setSRVC(const bool isACFlag, const bool flag);

private:
    //static DiagManagerAdapter *instance;
    static std::shared_ptr<DiagManagerAdapter> instance;
    android::sp<RemotediagHandler> mHandler = nullptr;
    android::sp<ServiceDeathRecipient> mServiceDeathRecipient = nullptr;
    android::sp<IDiagManagerReceiver> mDiagReceiver = nullptr;
    android::sp<IDiagManagerService> mDiagMService = nullptr;

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
    //constexpr static uint8_t IG_ON_TRIGGER{0x05U};
    //constexpr static uint8_t MEMORY_ACCESS_FAILURE{0x11U};
    //constexpr static uint8_t STORAGE_ACCESS_FAILURE{0x12U};
    constexpr static uint8_t CORRUPT_DATA_ACQUISITION{0x13U};
    constexpr static uint8_t FAILURE_ACQUIRE_ECU_LIST{0x14U};

    // RDG owner's DIDs
    constexpr static uint16_t PARAM_DID_IG_ON_OFF_TIME{0x3000U};
    constexpr static uint16_t PARAM_DID_ALL_IG_ON_OFF_TIMES{0x3001U};

    constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_1{0x3002U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_2{0x3003U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_3{0x3004U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_4{0x3005U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_5{0x3006U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_6{0x3007U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_7{0x3008U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_8{0x3009U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_9{0x300AU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_10{0x300BU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_11{0x300CU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_12{0x300DU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_13{0x300EU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_14{0x300FU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_15{0x3010U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_16{0x3011U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_17{0x3012U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_18{0x3013U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_19{0x3014U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_20{0x3015U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_21{0x3016U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_22{0x3017U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_23{0x3018U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_24{0x3019U};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_25{0x301AU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_26{0x301BU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_27{0x301CU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_28{0x301DU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_29{0x301EU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_30{0x301FU};
    // constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_31{0x3020U};
    constexpr static uint16_t PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_32{0x3021U};

    constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_1{0x3102U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_2{0x3103U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_3{0x3104U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_4{0x3105U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_5{0x3106U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_6{0x3107U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_7{0x3108U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_8{0x3109U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_9{0x310AU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_10{0x310BU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_11{0x310CU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_12{0x310DU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_13{0x310EU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_14{0x310FU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_15{0x3110U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_16{0x3111U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_17{0x3112U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_18{0x3113U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_19{0x3114U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_20{0x3115U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_21{0x3116U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_22{0x3117U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_23{0x3118U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_24{0x3119U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_25{0x311AU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_26{0x311BU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_27{0x311CU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_28{0x311DU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_29{0x311EU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_30{0x311FU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_31{0x3120U};
    constexpr static uint16_t PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_32{0x3121U};

    constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_1{0x3202U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_2{0x3203U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_3{0x3204U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_4{0x3205U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_5{0x3206U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_6{0x3207U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_7{0x3208U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_8{0x3209U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_9{0x320AU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_10{0x320BU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_11{0x320CU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_12{0x320DU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_13{0x320EU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_14{0x320FU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_15{0x3210U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_16{0x3211U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_17{0x3212U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_18{0x3213U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_19{0x3214U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_20{0x3215U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_21{0x3216U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_22{0x3217U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_23{0x3218U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_24{0x3219U};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_25{0x321AU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_26{0x321BU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_27{0x321CU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_28{0x321DU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_29{0x321EU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_30{0x321FU};
    // constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_31{0x3220U};
    constexpr static uint16_t PARAM_DID_SUCCESSFULLY_FILE_CREATION_32{0x3221U};

    constexpr static uint16_t PARAM_DID_STOP_OPERATION_1{0x3302U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_2{0x3303U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_3{0x3304U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_4{0x3305U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_5{0x3306U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_6{0x3307U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_7{0x3308U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_8{0x3309U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_9{0x330AU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_10{0x330BU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_11{0x330CU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_12{0x330DU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_13{0x330EU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_14{0x330FU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_15{0x3310U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_16{0x3311U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_17{0x3312U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_18{0x3313U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_19{0x3314U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_20{0x3315U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_21{0x3316U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_22{0x3317U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_23{0x3318U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_24{0x3319U};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_25{0x331AU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_26{0x331BU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_27{0x331CU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_28{0x331DU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_29{0x331EU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_30{0x331FU};
    // constexpr static uint16_t PARAM_DID_STOP_OPERATION_31{0x3320U};
    constexpr static uint16_t PARAM_DID_STOP_OPERATION_32{0x3321U};

    constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_1{0x3402U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_2{0x3403U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_3{0x3404U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_4{0x3405U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_5{0x3406U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_6{0x3407U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_7{0x3408U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_8{0x3409U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_9{0x340AU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_10{0x340BU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_11{0x340CU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_12{0x340DU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_13{0x340EU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_14{0x340FU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_15{0x3410U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_16{0x3411U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_17{0x3412U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_18{0x3413U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_19{0x3414U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_20{0x3415U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_21{0x3416U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_22{0x3417U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_23{0x3418U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_24{0x3419U};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_25{0x341AU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_26{0x341BU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_27{0x341CU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_28{0x341DU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_29{0x341EU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_30{0x341FU};
    // constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_31{0x3420U};
    constexpr static uint16_t PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_32{0x3421U};

    constexpr static uint16_t PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG{0x3500U};
    constexpr static uint16_t PARAM_DID_COLLECTION_CONDITIONS{0x3501U};
    constexpr static uint16_t PARAM_DID_DATETIME_OF_ECU_USERDEF_MEMORY_DTC_1{0x3502U};
    // constexpr static uint16_t PARAM_DID_DATETIME_OF_ECU_USERDEF_MEMORY_DTC_2{0x3503U};
    constexpr static uint16_t PARAM_DID_DATETIME_OF_ECU_USERDEF_MEMORY_DTC_3{0x3504U};
};
}
#endif // REMOTEDIAG_DIAGMANAGERADAPTER_H
