#ifndef ONBOARDCLIENT_COMMON_DEF_H
#define ONBOARDCLIENT_COMMON_DEF_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace OBC{
namespace OBC_COMMON
{
    constexpr static uint64_t TIME_SEND_RETRY_DELAY_MS{500U};
    enum COMM_RESPONSE_TYPE : uint8_t
    {
        COMM_RES_CONNECT,
        COMM_RES_DISCONNECT,
        COMM_RES_ACK_UDS_REQUEST,
        COMM_RES_UDS_REQUEST
    };
}

namespace OBC_INPUT_HANDLER
{
    constexpr static int32_t MSG_RECEIVE_BOOT_COMPLETE{1};
    constexpr static int32_t MSG_RECEIVE_WATCH_DOG{2};
    // constexpr static int32_t MSG_BOOT_COMPLETE_DID_START{3};
    // constexpr static int32_t MSG_SIGNAL_INTERNAL_IGNITION2_STATUS{4};
    constexpr static int32_t MSG_CONNECT_TO_APPMGR{5};
    constexpr static int32_t MSG_OBC_RECEIVE_BOOT_PRE{6};
    constexpr static int32_t MSG_OBC_CONNECT_TO_COMMMGR{7};
    constexpr static int32_t MSG_OBC_CONNECT_TO_DIAGMGR{8};
}

namespace OBC_RX_HANDLER
{
    constexpr static int32_t CMD_RX_INIT{100};
    constexpr static int32_t MSG_OBC_RECEIVE_FROM_COMM{101};
    constexpr static int32_t MSG_OBC_RECEIVE_FROM_DIAG{102};
}

namespace OBC_TX_HANDLER
{
    constexpr static int32_t CMD_TX_INIT{200};
    constexpr static int32_t CMD_TX_SEND_UDS{201};
    // constexpr static int32_t CMD_TX_SEND_UDS_RESPONSE{202};
    constexpr static int32_t CMD_TX_SEND_ACK_ROB{203};
    constexpr static int32_t CMD_TX_SEND_UDS_TO_DIAG{204};
    constexpr static int32_t CMD_TX_START_MONITORING_WIRE_CONNECTION{205};
}

namespace OBC_COMM_ENUM
{
    constexpr static uint8_t CATEGORY_OBC{0x1BU};
    // constexpr static uint8_t REQUEST_INITIALIZE{0x10U};
    // constexpr static uint8_t RESPONSE_INITIALIZE{0x11U};
    // constexpr static uint8_t REQUEST_CONNECT{0x20U};
    constexpr static uint8_t RESPONSE_CONNECT{0x21U};
    // constexpr static uint8_t REQUEST_DISCONNECT{0x30U};
    constexpr static uint8_t RESPONSE_DISCONNECT{0x31U};
    // constexpr static uint8_t SEND_UDS_REQUEST{0x40U};
    constexpr static uint8_t ACK_UDS_REQUEST{0x41U};
    constexpr static uint8_t ACK_ROB_DETECT{0x50U};
    constexpr static uint8_t SEND_UDS_RESPONSE{0x51U};
    // constexpr static uint8_t WIRED_CONNECTION_PROGRESS{0x61U};
    constexpr static uint8_t CMD2_OBC_DEFAULT{0x01U};

}

namespace OBC_DIAG_ENUM
{
    // constexpr static uint8_t MIN_DATA_BLOCK_SIZE{2U};
    constexpr static uint32_t BLOCK_SIZE{4095U};
    // constexpr static uint32_t BLOCK_PAYLOAD_SIZE{4094U};
}
// namespace OBC_ERROR_HANDLE
// {
//     constexpr static uint8_t MAX_RETRY_COUNT_TRANS_ERR{3U};
//     constexpr static uint8_t MAX_RETRY_COUNT_BUSY_ERR{10U};
//     constexpr static uint32_t MAX_UDS_REQ_SIZE{65536U};
//     // constexpr static uint32_t MAX_UDS_RES_SIZE{3276800U};
// }
}
#endif // ONBOARDCLIENT_COMMON_DEF_H
