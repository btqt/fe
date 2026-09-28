#ifndef RDG_PROXY_IPC_PROTOCOL_H
#define RDG_PROXY_IPC_PROTOCOL_H

#include <cstdint>
#include <string>
#include <vector>

namespace rdgipc {

enum class CommandId : uint32_t {
    ApplicationGetBootCompleted = 1001U,
    ApplicationQueryAction = 1002U,
    ApplicationSetFeatureStatus = 1003U,
    ApplicationGetFeatureStatus = 1004U,

    DiagWriteDid = 2001U,
    DiagReadDid = 2002U,

    CalibRegisterDidWatch = 2101U,

    LocationGetLocation = 3001U,

    MqttSubscribeTopic = 4001U,

    HttpSendGrpc = 5001U,

    PowerGetIgnitionStatus = 6001U,
    PowerAcquireLock = 6002U,
    PowerReleaseLock = 6003U,

    PpiGetFlag = 7001U,
    PpiResponseDelete = 7002U,

    RegionGetNation = 8001U,

    VehicleGetTimeCounter = 9001U,
    VehicleGetTripCounter = 9002U,

    TimeGetCurrentTime = 9101U,
    TimeGetCurrentMilliSec = 9102U,
    TimeGetOffset = 9103U,

    TelephonyGetNetworkTime = 9201U,
    TelephonyGetImei = 9202U,
    TelephonyGetEidDefault = 9203U,

    /* Upload file store hosted by the proxy on the EP partition */
    FileSaveUpload = 9301U,
    FileGetUpload = 9302U,
    FileRemoveUpload = 9303U,
    FileExistsUpload = 9304U,
};

enum class Status : uint32_t {
    Ok = 0U,
    Err = 1U,
};

enum class MessageType : uint32_t {
    Request = 0U,
    Response = 1U,
    Callback = 2U,
};

enum class CallbackId : uint32_t {
    ApplicationOnBootCompleted = 2000U,
    ApplicationOnFeatureStatusChanged = 2001U,
    ApplicationOnFeatureActionDelivered = 2002U,
    ApplicationPostAppStatusChanged = 2005U,
    PowerStateChanged = 9001U,
    PowerErrControl = 9002U,
    PowerModeChanged = 9003U,
    PowerExtValueChanged = 9004U,
    PowerLockReleased = 9005U,
    HttpGrpcResponse = 9006U,
    HttpGrpcConnState = 9007U,
    CalibServiceFlagChanged = 9008U,
    CalibPpiFlagChanged = 9009U,
    CalibVinChanged = 9010U,
    MqttOnNotifyReceived = 9011U,
    VehicleSignalReceived = 9012U,
    VehicleSignalTimeout = 9013U,
    PpiStatusChanged = 9014U,
    SomeipServiceModeStatus = 9015U,
};

inline const char *commandName(const CommandId commandId) {
    switch (commandId) {
    case CommandId::ApplicationGetBootCompleted:
        return "ApplicationGetBootCompleted";
    case CommandId::ApplicationQueryAction:
        return "ApplicationQueryAction";
    case CommandId::ApplicationSetFeatureStatus:
        return "ApplicationSetFeatureStatus";
    case CommandId::ApplicationGetFeatureStatus:
        return "ApplicationGetFeatureStatus";
    case CommandId::DiagWriteDid:
        return "DiagWriteDid";
    case CommandId::DiagReadDid:
        return "DiagReadDid";
    case CommandId::CalibRegisterDidWatch:
        return "CalibRegisterDidWatch";
    case CommandId::LocationGetLocation:
        return "LocationGetLocation";
    case CommandId::MqttSubscribeTopic:
        return "MqttSubscribeTopic";
    case CommandId::HttpSendGrpc:
        return "HttpSendGrpc";
    case CommandId::PowerGetIgnitionStatus:
        return "PowerGetIgnitionStatus";
    case CommandId::PowerAcquireLock:
        return "PowerAcquireLock";
    case CommandId::PowerReleaseLock:
        return "PowerReleaseLock";
    case CommandId::PpiGetFlag:
        return "PpiGetFlag";
    case CommandId::PpiResponseDelete:
        return "PpiResponseDelete";
    case CommandId::RegionGetNation:
        return "RegionGetNation";
    case CommandId::VehicleGetTimeCounter:
        return "VehicleGetTimeCounter";
    case CommandId::VehicleGetTripCounter:
        return "VehicleGetTripCounter";
    case CommandId::TimeGetCurrentTime:
        return "TimeGetCurrentTime";
    case CommandId::TimeGetCurrentMilliSec:
        return "TimeGetCurrentMilliSec";
    case CommandId::TimeGetOffset:
        return "TimeGetOffset";
    case CommandId::TelephonyGetNetworkTime:
        return "TelephonyGetNetworkTime";
    case CommandId::TelephonyGetImei:
        return "TelephonyGetImei";
    case CommandId::TelephonyGetEidDefault:
        return "TelephonyGetEidDefault";
    case CommandId::FileSaveUpload:
        return "FileSaveUpload";
    case CommandId::FileGetUpload:
        return "FileGetUpload";
    case CommandId::FileRemoveUpload:
        return "FileRemoveUpload";
    case CommandId::FileExistsUpload:
        return "FileExistsUpload";
    default:
        return "UnknownCommand";
    }
}

inline const char *callbackName(const CallbackId callbackId) {
    switch (callbackId) {
    case CallbackId::ApplicationOnBootCompleted:
        return "ApplicationOnBootCompleted";
    case CallbackId::ApplicationOnFeatureStatusChanged:
        return "ApplicationOnFeatureStatusChanged";
    case CallbackId::ApplicationOnFeatureActionDelivered:
        return "ApplicationOnFeatureActionDelivered";
    case CallbackId::ApplicationPostAppStatusChanged:
        return "ApplicationPostAppStatusChanged";
    case CallbackId::PowerStateChanged:
        return "PowerStateChanged";
    case CallbackId::PowerErrControl:
        return "PowerErrControl";
    case CallbackId::PowerModeChanged:
        return "PowerModeChanged";
    case CallbackId::PowerExtValueChanged:
        return "PowerExtValueChanged";
    case CallbackId::PowerLockReleased:
        return "PowerLockReleased";
    case CallbackId::HttpGrpcResponse:
        return "HttpGrpcResponse";
    case CallbackId::HttpGrpcConnState:
        return "HttpGrpcConnState";
    case CallbackId::CalibServiceFlagChanged:
        return "CalibServiceFlagChanged";
    case CallbackId::CalibPpiFlagChanged:
        return "CalibPpiFlagChanged";
    case CallbackId::CalibVinChanged:
        return "CalibVinChanged";
    case CallbackId::MqttOnNotifyReceived:
        return "MqttOnNotifyReceived";
    case CallbackId::VehicleSignalReceived:
        return "VehicleSignalReceived";
    case CallbackId::VehicleSignalTimeout:
        return "VehicleSignalTimeout";
    case CallbackId::PpiStatusChanged:
        return "PpiStatusChanged";
    case CallbackId::SomeipServiceModeStatus:
        return "SomeipServiceModeStatus";
    default:
        return "UnknownCallback";
    }
}

inline const char *callbackName(const uint32_t callbackId) {
    return callbackName(static_cast<CallbackId>(callbackId));
}

inline const char *statusName(const Status status) {
    switch (status) {
    case Status::Ok:
        return "OK";
    case Status::Err:
        return "ERR";
    default:
        return "UNKNOWN";
    }
}

inline const char *messageTypeName(const MessageType type) {
    switch (type) {
    case MessageType::Request:
        return "Request";
    case MessageType::Response:
        return "Response";
    case MessageType::Callback:
        return "Callback";
    default:
        return "UnknownMessageType";
    }
}

inline std::vector<uint8_t> toBytes(const std::string &value) {
    return std::vector<uint8_t>(value.begin(), value.end());
}

inline std::string toString(const std::vector<uint8_t> &value) {
    return std::string(value.begin(), value.end());
}

} // namespace rdgipc

#endif // RDG_PROXY_IPC_PROTOCOL_H
