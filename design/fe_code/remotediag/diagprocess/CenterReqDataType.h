#ifndef CENTERREQDATATYPE_H_
#define CENTERREQDATATYPE_H_

namespace rdgapp {

//Define check validate request command header
// static const uint32_t P_VERSION                                     (0x09U);
// static const uint32_t BIT_MASK_START_DATA_COMMAND                          (0x05U);

//Define message id
inline static constexpr uint8_t MSG_ID_CENTERREQUESTALLDTCSSR{0x01U};
inline static constexpr uint8_t MSG_ID_CENTERREQUESTALLROB{0x02U};
inline static constexpr uint8_t MSG_ID_CENTERREQUESTROBSSR{0x03U};
inline static constexpr uint8_t MSG_ID_CENTERREQUESTECUINFORMATION{0x04U};
inline static constexpr uint8_t MSG_ID_CENTERREQUESTDIRECTCOMMAND{0x05U};
inline static constexpr uint8_t MSG_ID_COLLECTIONCONDITIONDIRECTCOMMAND{0x06U};
inline static constexpr uint8_t MSG_ID_COLLECTIONCONDITIONECUINFORMATION{0x07U};
inline static constexpr uint8_t MSG_ID_COLLECTIONCONDITIONROBROBSSRDIDEVENT{0x08U};
inline static constexpr uint8_t MSG_ID_COLLECTIONCONDITIONWARNINGINFORMATION{0x09U};

// static const int8_t messageIdTbl[] = {
//     MSG_ID_DTC_FFD,
//     MSG_ID_RMT_MONITOR,
//     MSG_ID_DIAG_REC,
//     MSG_ID_VEHICLE_STATE_NOTIFY,
//     MSG_ID_RMT_CTRL_15PF,
//     MSG_ID_RMT_CTRL_19PF,
//     MSG_ID_RMT_IMMOBILIZER,
//     MSG_ID_E_VEHICLE_STATE,
//     MSG_ID_E_RMT_CTRL,
//     MSG_ID_HISTORY_UPLOAD,
//     MSG_ID_REJECT_RELEASE,
//     MSG_ID_CMD_STOP,
//     MSG_ID_UBI_NOTIFY,
//     MSG_ID_USB_NOTIFY,
//     MSG_ID_AIR_DATA_FEED
// };

// typedef enum _Trigger_Data_Type_ {
//   TRIGGER_TYPE_UNKNOWN            = 0,
//   TRIGGER_TYPE_SMS                = 3,
//   TRIGGER_TYPE_CONTROL_CODE       = 5,
//   TRIGGER_TYPE_SCHEDULER          = 10,
//   TRIGGER_TYPE_SCHEDULER_ONE_SHOT = 4,
//   TRIGGER_TYPE_SCHEDULER_ROUTINE  = 8,
//   TRIGGER_TYPE_MAX                =11
// }eTriggerType;

// typedef enum _eRespCode
// {
//     RESPONSE_CODE_NORMAL                       = 0x00,
//     RESPONSE_CODE_DURING_REPAIR                = 0x01,
//     RESPONSE_CODE_SCR_DISPLAY_CANCEL           = 0x02,
//     RESPONSE_CODE_NO_CORRESPONDING_FILE        = 0x03,
//     RESPONSE_CODE_UNSPECIFIED_COMMAND          = 0x10,
//     RESPONSE_CODE_NO_CORRESPONDING_DATA        = 0x11,
//     RESPONSE_CODE_DATA_OVER_CAPACITY           = 0x12,
//     RESPONSE_CODE_INTERRUPT_NOT_POSSIBLE       = 0x13,
//     RESPONSE_CODE_VERHICLE_SERVICE_NOT_PROVIDE = 0x14,
//     RESPONSE_CODE_MEMORY_READ_FAILURE          = 0x30,
//     RESPONSE_CODE_REJECT_DURING_REPAIR         = 0x32,
//     RESPONSE_CODE_OUT_COMMUNICATION_RANK       = 0x40,
//     RESPONSE_CODE_POWER_STATE_ERROR            = 0x50,
//     RESPONSE_CODE_SCR_DISPLAY_FAILURE          = 0x51,
//     RESPONSE_CODE_OTHERS                       = 0x90
// }eResponseCode;


// typedef enum _HeaderErrorType_ {
//   HEADER_VALID = 0,
//   HEADER_EMPTY,
//   MSG_ID_OUTSIDE,
//   INVALID_PROTOCOL_VERSION,
//   REQUEST_ID_NON_ALPHANUMERIC,
//   REQUEST_ID_MISMATCH,
//   ERROR_OTHER
// }eHeaderErrorType;
}
#endif /* CENTERREQDATATYPE_H_ */
