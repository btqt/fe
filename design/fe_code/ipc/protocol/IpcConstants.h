// ipc/protocol/IpcConstants.h
#pragma once
#include <cstdint>

// Callback IDs (EP → IP, one-way)
enum DiagCallbackId : uint16_t {
    CB_DIAG_STATUS_CHANGED          = 0x0100,
    CB_DIAG_UNDER_REPAIR_CHANGED    = 0x0101,
    CB_DIAG_SERVICE_FLAG_CHANGED    = 0x0102,
    CB_DIAG_DID_READ_RESPONSE       = 0x0103,
};

enum CalibCallbackId : uint16_t {
    CB_CALIB_STATUS_CHANGED         = 0x0200,
};

enum PowerCallbackId : uint16_t {
    CB_POWER_IG_CHANGED             = 0x0300,
    CB_POWER_BATTERY_STATUS         = 0x0301,
};

enum PPICallbackId : uint16_t {
    CB_PPI_RECEIVED                 = 0x0400,
};

enum LocationCallbackId : uint16_t {
    CB_LOCATION_UPDATE              = 0x0500,
};

enum RegionCallbackId : uint16_t {
    CB_REGION_CHANGED               = 0x0600,
};

enum MqttCallbackId : uint16_t {
    CB_MQTT_NOTIFICATION            = 0x0700,
};

enum HttpCallbackId : uint16_t {
    CB_HTTP_GRPC_RESPONSE           = 0x0800,
};

enum OnboardClientCallbackId : uint16_t {
    CB_OBC_RESPONSE_EVENT           = 0x0900,
    CB_OBC_OBD2_EVENT               = 0x0901,
    CB_OBC_RESOURCE_EVENT           = 0x0902,
};

enum VehicleCallbackId : uint16_t {
    CB_VEHICLE_EVENT                = 0x0A00,
};

enum AppCallbackId : uint16_t {
    CB_APP_BOOT_COMPLETED           = 0x0B00,
    CB_APP_FEATURE_STATUS_CHANGED   = 0x0B01,
    CB_APP_POST_RECEIVED            = 0x0B02,
};

// Command IDs (IP → EP, synchronous request/response)
enum DiagCommandId : uint16_t {
    CMD_DIAG_GET_RDG_FLAG           = 0x8100,
    CMD_DIAG_GET_DTC_FLAG           = 0x8101,
    CMD_DIAG_GET_SSR_FLAG           = 0x8102,
    CMD_DIAG_GET_WAR_FLAG           = 0x8103,
    CMD_DIAG_GET_ROB_FLAG           = 0x8104,
    CMD_DIAG_GET_DDR_FLAG           = 0x8105,
    CMD_DIAG_GET_UNDER_REPAIR       = 0x8106,
    CMD_DIAG_GET_SRVC_AC            = 0x8107,
    CMD_DIAG_GET_SRVC_VC            = 0x8108,
    CMD_DIAG_GET_SRVC_PC            = 0x8109,
    CMD_DIAG_GET_SRVC_STT           = 0x810A,
    CMD_DIAG_READ_DID               = 0x810B,
    CMD_DIAG_WRITE_DID              = 0x810C,
};

enum CalibCommandId : uint16_t {
    CMD_CALIB_GET_VARIANT           = 0x8200,
};

enum PowerCommandId : uint16_t {
    CMD_POWER_GET_IG_STATUS         = 0x8300,
};

enum PPICommandId : uint16_t {
    CMD_PPI_WRITE_DID               = 0x8400,
};

enum LocationCommandId : uint16_t {
    CMD_LOCATION_GET_CURRENT        = 0x8500,
};

enum RegionCommandId : uint16_t {
    CMD_REGION_GET_CURRENT          = 0x8600,
};

enum MqttCommandId : uint16_t {
    CMD_MQTT_PUBLISH                = 0x8700,
};

enum HttpCommandId : uint16_t {
    CMD_HTTP_SEND_GRPC              = 0x8800,
};

enum OnboardClientCommandId : uint16_t {
    CMD_OBC_SEND_UDS                = 0x8900,
    CMD_OBC_CONNECT                 = 0x8901,
    CMD_OBC_DISCONNECT              = 0x8902,
    CMD_OBC_TAKE_RESOURCE           = 0x8903,
    CMD_OBC_RELEASE_RESOURCE        = 0x8904,
};

enum VehicleCommandId : uint16_t {
    CMD_VEHICLE_GET_STATUS          = 0x8A00,
};

enum AppCommandId : uint16_t {
    CMD_APP_REGISTER                = 0x8B00,
};
