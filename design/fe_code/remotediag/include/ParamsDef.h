#ifndef REMOTEDIAG_PARAMSDEF_H
#define REMOTEDIAG_PARAMSDEF_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <Typedef.h>
#include <rdg/v1/interfaces/RDG_common_message_definition.pb.h>
namespace rdgapp {
namespace ParamsDef {
        constexpr static uint16_t FA_SERVER_DEFAULT_PORT{51000U};
        constexpr static char_t FA_SERVER_LOCAL_HOST[]{"192.168.10.16"};
        constexpr static uint32_t FA_SERVER_MAX_PACKET_SIZE {1024U};
        constexpr static uint32_t FA_SERVER_MAX_PACKET_SEND_SIZE {4096U};
        constexpr static char_t ECU_INFORMATION_LIST_PATH_TEST[]{"/data/rdg/ecu_info_list_test"};
        constexpr static int32_t OCCURRENT_ROB_NOTIFICATION_LOW_PRIORITY {150};
        constexpr static int32_t OCCURRENT_ROB_NOTIFICATION_HIGH_PRIORITY {50};
        constexpr static uint64_t GET_OBC_RESOURCE_RETRY_TIME {5000U}; // 5 seconds
        constexpr static int32_t UNKNOWN{5};
};


static constexpr uint32_t PROTOBUF_MESSAGE_DEFINITION_VERSION{0x0600U}; // https://toyota-11f.rickcloud.jp/jira/browse/DCM24SPEC-17142
static constexpr uint8_t NUMBER_UPLOAD_DATA_IS_MADE_AFTER_IG_ON_MAX{99U};
static constexpr uint32_t ROB_UPLOAD_DATA_SIZE_MAX{4194304U}; // bytes = 4MB  //CID 8246662
static constexpr uint32_t DTC_UPLOAD_DATA_SIZE_MAX{4194304U}; // bytes = 4MB
static constexpr int32_t WHAT_CHANGED_REPAIR_SATUS{1};
const static std::string CFG_MGR_BRAND_TOYOTA{"Toyota"};
const static std::string CFG_MGR_BRAND_LEXUS{"Lexus"};

static constexpr char_t APP_NAME[]{"RDG"};
static const std::string DATA_PATH{"/data/appdata/rdg/"};
static const std::string UPLOAD_PATH{"/data/rdg/"};

static const std::string GET_COLLECTION_CONDITION_REQ_DB_NAME{"get_collection_condition_req.db"};

static const std::string COLLECTION_CONDITION_DIAG_COMMON_DB_NAME{"collection_condition_diag_common.db"};
// static constexpr char COLLECTION_CONDITION_DIRECT_CMD_DB_NAME[]{"collection_condition_direct_command.db"}; // CID 9300337
static const std::string COLLECTION_CONDITION_ROB_SSR_DID_DB_NAME{"collection_condition_rob_ssr_did.db"};
static const std::string COLLECTION_CONDITION_ECU_INFO_DB_NAME{"collection_condition_ecu_info.db"};
static const std::string COLLECTION_CONDITION_WARN_INFO_DB_NAME{"collection_condition_warn_info.db"};

static const std::string OCCURRENCE_ROB_MONITORING_DB_NAME{"rob_monitoring_information.db"};

#ifdef PF_TYPE_CY
static constexpr vccomif::common::v1::AppCommonHeaderVehicleToCenter_ElectronicPf EPF_19EPF{vccomif::common::v1::AppCommonHeaderVehicleToCenter_ElectronicPf::AppCommonHeaderVehicleToCenter_ElectronicPf_EPF_19EPF_V3};
#endif

#ifdef PF_TYPE_LC
static constexpr vccomif::common::v1::AppCommonHeaderVehicleToCenter_ElectronicPf EPF_19EPF{vccomif::common::v1::AppCommonHeaderVehicleToCenter_ElectronicPf::AppCommonHeaderVehicleToCenter_ElectronicPf_EPF_19EPF_V2};
#endif

// static constexpr char PREVIOUS_TRIP_OCCURRENCE_ROBS_DB_NAME[]{"previous_trip_occurrent_robs.db"}; //CID 9301632

// static constexpr char CENTER_REQ_ALL_DTC_SSR_DB_NAME[]{"center_request_all_dtc_ssr.db"}; // CID 9301890
// static constexpr char CENTER_REQ_ALL_ROB_DB_NAME[]{"center_request_all_rob.db"}; // CID 9301135
// static constexpr char CENTER_REQ_ECU_INFO_DB_NAME[]{"center_request_ecu_information.db"}; //CID 9301384


//const std::string RDG_DID_ECU_USER_DEF_MEMORY_DTC_INDEX_PROP{"remotediag.prop.UserDefDTCIdx"};

//typedef uint32_t remoteDiagBrand_t;



/* Ignore warning about unused value */
template <typename Type>
inline void NOTUSED(const Type &val) noexcept
{
    static_cast<void>(val);
}

namespace remotediag_region
{
    constexpr static uint8_t LGE_REGION_NONE{0x0000U};
    constexpr static uint8_t LGE_REGION_JP{0x0001U};
    constexpr static uint8_t LGE_REGION_CN{0x0002U};
    constexpr static uint8_t LGE_REGION_NA{0x0003U};
    constexpr static uint8_t LGE_REGION_AU{0x0004U};
    constexpr static uint8_t LGE_REGION_SA{0x0005U};
    constexpr static uint8_t LGE_REGION_AE{0x0006U};
    constexpr static uint8_t LGE_REGION_IN{0x0007U};
    constexpr static uint8_t LGE_REGION_BH{0x0008U};
    constexpr static uint8_t LGE_REGION_QA{0x0009U};
    constexpr static uint8_t LGE_REGION_KW{0x000AU};
    constexpr static uint8_t LGE_REGION_NZ{0x000BU};
    constexpr static uint8_t LGE_REGION_KR{0x000CU};
    constexpr static uint8_t LGE_REGION_TW{0x000DU};

    static std::unordered_map<uint32_t, std::string> name{
        {LGE_REGION_NONE, "LGE_REGION_NONE"},
        {LGE_REGION_JP, "LGE_REGION_JP"},
        {LGE_REGION_CN, "LGE_REGION_CN"},
        {LGE_REGION_NA, "LGE_REGION_NA"},
        {LGE_REGION_AU, "LGE_REGION_AU"},
        {LGE_REGION_SA, "LGE_REGION_SA"},
        {LGE_REGION_AE, "LGE_REGION_AE"},
        {LGE_REGION_IN, "LGE_REGION_IN"},
        {LGE_REGION_BH, "LGE_REGION_BH"},
        {LGE_REGION_QA, "LGE_REGION_QA"},
        {LGE_REGION_KW, "LGE_REGION_KW"},
        {LGE_REGION_NZ, "LGE_REGION_NZ"},
        {LGE_REGION_KR, "LGE_REGION_KR"},
        {LGE_REGION_TW, "LGE_REGION_TW"}};
};

union SerializeUint16_t {
    uint16_t value;
    uint8_t data[2];
};

union SerializeUint32_t {
    uint32_t value;
    uint8_t data[4];
};

namespace HANDLE_MESSAGE_REQUEST
{
    /* Register service*/
    constexpr static int32_t MSG_REGISTER_APPLICATION_MGR{1000}; /* Application Manager Service*/
    // constexpr static int32_t MSG_REGISTER_ALARM_MGR{1001};       /* Alam Manager Service*/
    // constexpr static int32_t MSG_REGISTER_AUDIO_MGR{1002};         /* Audio Manager Service*/  // CID 9337964
    // constexpr static int32_t MSG_REGISTER_COMMUNICATION_MGR{1003}; /* Communication Manager Service*/
    // constexpr static int32_t MSG_REGISTER_HMI_MGR{1004};           /* HMI Manager Service*/ // CID 9338254
    constexpr static int32_t MSG_REGISTER_LOCATION_MGR{1005};   /* Loaction Manager Service*/
    constexpr static int32_t MSG_REGISTER_POWER_MODE_MGR{1006}; /* Power Manager Service*/
    // constexpr static int32_t MSG_REGISTER_TELEPHONY_MGR{1007};     /* Telephony Manager Service*/ //CID 9337573
    constexpr static int32_t MSG_REGISTER_VEHICLE_MGR{1008}; /* Vehicle Manager Service*/
    constexpr static int32_t MSG_REGISTER_CONFIG_MGR{1009};  /* Vehicle Manager Service*/
    // constexpr static int32_t MSG_REGISTER_EMLJSON_MGR{1010};       /* EMLJSON Manager Service*/ //CID 9337184
    constexpr static int32_t MSG_REGISTER_MQTT_MGR{1011};     /* EMLJSON Manager Service*/
    constexpr static int32_t MSG_REGISTER_SOMEIP_MGR{1012};   /* SomeIP Manager Service*/
    constexpr static int32_t MSG_REGISTER_CALIB_MGR{1013};    /* Calib Manager Service*/
    constexpr static int32_t MSG_REGISTER_DIAG_MGR{1014};     /* Diag Manager Service*/
    constexpr static int32_t MSG_REGISTER_HTTP_MGR{1015};     /* Http Manager Service*/
    constexpr static int32_t MSG_REGISTER_OBC_MGR{1016};      /* Onboard client Service*/
    constexpr static int32_t MSG_REGISTER_PPI_MGR{1017};      /* PPI service*/
    constexpr static int32_t MSG_REGISTER_REGION_MGR{1018};   /* Region Manager service*/
    // constexpr static int32_t MSG_REGISTER_FIREWALL_MGR{1019}; /* Firewall Manager Service*/
    constexpr static int32_t MSG_NOTIFY_DIAG_TRIGGER_DISCARD_IGOFF{2006}; /* Notify discard diag due to IG OFF*/
    /* System message*/
    constexpr static int32_t MSG_APPL_ON_BOOT_COMPLETED{2000};           /* On Boot Completed */
    constexpr static int32_t MSG_APPL_ON_FEATURE_STATUS_CHANGED{2001};   /* On Feature Status Changed */
    constexpr static int32_t MSG_APPL_ON_FEATURE_ACTION_DELIVERED{2002}; /* On Request Action */
    // constexpr static int32_t MSG_APPL_ON_ACTIVE{2003};                    /* Active Callback */ //CID 9337129
    // constexpr static int32_t MSG_APPL_ON_INACTIVE{2004};                  /* Inactive Callback */ //CID 9337854
    constexpr static int32_t MSG_APPL_POST_APP_STATUS_CHANGED{2005};      /* App active status changed*/

    constexpr static int32_t MSG_NOTIFY_DIAG_TRIGGER{2007};               /* Notify discard diag trigger*/

    constexpr static int32_t MSG_POWR_ON_IGN_ON{2010};  /* IGN ON */
    constexpr static int32_t MSG_POWR_ON_IGN_OFF{2011}; /* IGN OFF */
    constexpr static int32_t MSG_POWRER_WRITE_DID_ON{2012}; /* Write DID for IG ON */
    constexpr static int32_t MSG_POWRER_WRITE_DID_OFF{2013}; /* Write DID for IG OFF */
    // constexpr static int32_t MSG_POWR_MODE_CHANGE{2012}; /* PowerMode change */  CID 9338508
    // constexpr static int32_t MSG_BUB_ON{2013};           /* BUB ON */ //CID 9338442
    // constexpr static int32_t MSG_BUB_OFF{2014};          /* BUB OFF */ CID 9338584

    constexpr static int32_t MSG_ALAM_ON_EXPIRED{2015};   /* Timer alarm expired */
    constexpr static int32_t MSG_PPI_INFO_RECEIVED{2016}; /* Receive PPI information */
    constexpr static int32_t MSG_CMD_INIT_APP{2017};      /* Do command init app */

    constexpr static int32_t MSG_CENTER_PUSH_RECEIVED{2030};             /* Receive Center request */
    
    constexpr static int32_t MSG_RPC_MESSAGE_RECEIVED{2032};             /* Receive RPC message from center */
    constexpr static int32_t MSG_OBC_OBD_EVENT_RECEIVED{2033};           /* Receive OBD event from Onboardclient */
    constexpr static int32_t MSG_OBC_UDS_RESPONSE_RECEIVED{2034};        /* Receive RPC message from center */
    constexpr static int32_t MSG_RECEIVE_NEW_COLLECTION_CONDITION{2035}; /* Save collection condition successful */
    constexpr static int32_t MSG_RECEIVE_CENTERCOMMNAD{2036};            /* Load collection condition from file*/

    constexpr static int32_t MSG_FIREWALL_ACTION_DIAG_DISABLE{2037};                  /* Receive FirewallMgr event diag disable */
    constexpr static int32_t MSG_FIREWALL_ACTION_DIAG_ENABLE{2038};                   /* Receive FirewallMgr event diag enable */
    constexpr static int32_t MSG_NOTIFY_RDG_UPDATED_STATUS{2039};                     /* Receive new status of RDG*/
    constexpr static int32_t MSG_NOTIFY_SERVICE_MODE_STATUS{2040};                    /* Receive service mode status */
    constexpr static int32_t MSG_NOTIFY_DELETE_COLLECTION_CONDITION{2041};            /* Delete collection condition */
    constexpr static int32_t MSG_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR{2042};         /* Occurrent RoB detected notification */
    constexpr static int32_t MSG_NOTIFY_OCCURRENT_ROB_DETECTION_DIRECT_COMMAND{2043}; /* Occurrent RoB detected notification */
    constexpr static int32_t MSG_NOTIFY_SERVICE_FLAG_CHANGE{2044};                    /* Receive service flag */
    constexpr static int32_t MSG_NOTIFY_PPI_FLAG_CHANGE{2045};                        /* Receive service flag */
    constexpr static int32_t MSG_RECEIVE_COLLECTION_CONDITION_UPDLOAD_END{2046};      /* Colection condition update processing end event*/
    constexpr static int32_t MSG_GRPC_COMMUNICATION_RECONNECT{2047};
    constexpr static int32_t MSG_GRPC_COMMUNICATION_DISCONNECT{2048};
    constexpr static int32_t MSG_NOTIFY_VIN_CHANGE{2049};
    constexpr static int32_t MSG_APPL_ON_FEATURE_ACTION_PERFORMED{2050};
    constexpr static int32_t HANDLE_MSG_BUB_ON{2051};
    constexpr static int32_t HANDLE_MSG_BUB_OFF{2052};
    constexpr static int32_t MSG_PREPARE_TO_SHUTDOWN{2053}; /* Prepare to shutdown */
};
namespace RDG_TIME
{
    constexpr static uint64_t TIME_OBTAIN_MSG_DELAY_500MS{500U};
}

namespace RDG_APPID
{
        constexpr static uint8_t DTC{1U};
        constexpr static uint8_t SSR{2U};
        constexpr static uint8_t ROB{3U};
        constexpr static uint8_t DIRECT_COMMAND{4U};
        constexpr static uint8_t ROBSSR{5U};
        constexpr static uint8_t WARNING{6U};
        constexpr static uint8_t PRIORITYCONTROLS{7U};
        constexpr static uint8_t ECUINFORMATION{8U};
        constexpr static uint8_t OTA{9U};
        constexpr static uint8_t ROB_OCCURRENCE{10U};
        constexpr static uint8_t ROB_MONITORING{11U};
};

}
#endif /* REMOTEDIAG_PARAMSDEF_H */
