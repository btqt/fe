#pragma once
#include <common_def.h>
#include <vector>

#include <services/OnboardclientManagerService/OBCEnum.h>

namespace rdgapp {

enum class OTA_MID: uint8_t {
    // Request
    OTA_MID_UNKNOW                                          = 0x00U,
    OTA_MID_1_GET_OBC_RESOURCE_REQ                          = 0x01U,
    OTA_MID_2_RELEASE_OBC_RESOURCE_REQ                      = 0x02U,
    OTA_MID_3_CONNECT_REQ                                   = 0x03U,
    OTA_MID_4_DISCONNECT_REQ                                = 0x04U,
    OTA_MID_5_SEND_UDS_DATA_REQ                             = 0x05U,
    // Response
    OTA_MID_6_GET_OBC_RESOURCE_RES                          = 0x06U,
    OTA_MID_7_CONNECT_RES                                   = 0x07U,
    OTA_MID_8_DISCONNECT_RES                                = 0x08U,
    OTA_MID_9_RESPONSE_EVENT                                = 0x09U,
    OTA_MID_MAX                                             = 0x0AU
};

enum class OBCResourceEventCode : uint8_t {
    OBC_GET_RESOURCE_OK = 0U,                           /* Get OBC Resource OK */
    OBC_RELEASE_RESOURCE_OK = 1U,                       /* Release OBC Resource OK */
    OBC_GET_RESOURCE_WAIT = 2U,                         /* Wait for Get OBC ResourceK */
    OBC_RELEASE_RESOURCE_COMPLETE = 3U,                 /* Completition of Release OBC Resource */
    OBC_ACCESS_TIMEOUT = 4U,                            /* OBC access timeout */
    OBC_FORCE_RESET = 5U                                /* OBC forced reset */
};

enum class OTAPriorityType : uint8_t {
    UNKNOWN = 0U,                                       /* Unknown */     
    LOW = 1U,                                           /* Low priority */
    HIGH = 2U                                           /* High priority */
};

namespace OtaMessageDefs {
        constexpr static int32_t MID_LENGHT {1};
        constexpr static int32_t FA_PROTO_VERSION_LENGHT {1};

        constexpr static uint32_t PROTOCOL_TYPE_BYTE_LENGHT {1U};

        constexpr static uint32_t CANID_BYTE_LENGHT {4U};

        constexpr static uint32_t PERIODIC_RES_BYTE_LENGHT {1U};

        constexpr static uint32_t CONNECT_ID_BYTE_LENGHT {2U};
        constexpr static uint32_t UDS_REQUEST_BYTE_LEGHT {2U};

        constexpr static uint32_t FA_PROTOCOL_HEADER_LENGHT {8U};

        constexpr static uint32_t FA_PROTOCOL_MAX_DATA {65575U};

        constexpr static uint32_t FA_PROTO_VERSION_BYTE_MASK {0U};
        constexpr static uint32_t MID_BYTE_MASK {1U};
        constexpr static uint32_t SEQUENCE_NUMBER_BYTE_MASK {2U};
        constexpr static uint32_t PAYLOAD_SIZE_BYTE_MASK {5U};
        constexpr static uint32_t PAYLOAD_BYTE_MASK {FA_PROTOCOL_HEADER_LENGHT};

        constexpr static uint32_t PROTOCOL_TYPE_BYTE_MASK {0U};
        constexpr static uint32_t CANID_BYTE_MASK {PROTOCOL_TYPE_BYTE_MASK + PROTOCOL_TYPE_BYTE_LENGHT};
        constexpr static uint32_t NTA_LENGHT_BYTE_MASK { CANID_BYTE_MASK + CANID_BYTE_LENGHT };
        constexpr static uint32_t CONNECT_ID_BYTE_MASK {PROTOCOL_TYPE_BYTE_MASK};
        constexpr static uint32_t UDS_REQUEST_BYTE_MASK {CONNECT_ID_BYTE_MASK + CONNECT_ID_BYTE_LENGHT};
        constexpr static uint32_t UDS_REQUEST_DATA_BYTE_MASK {UDS_REQUEST_BYTE_MASK + UDS_REQUEST_BYTE_LEGHT};
};
} // namespace rdgapp
