#ifndef REMOTEDIAG_UDS_MESSAGE_DEFINE_H
#define REMOTEDIAG_UDS_MESSAGE_DEFINE_H

#define SID_BYTE_MASK 0U
#define SFID_BYTE_MASK 1U
#define UDS_DATA_BYTE_MASK 2U
#define UDS_MEMORY_SELECTION_BYTE_MASK UDS_DATA_BYTE_MASK
#define UDS_DTC_STATUS_AVAILABILITY_MASK UDS_MEMORY_SELECTION_BYTE_MASK + 1U
#define UDS_DTC_ROB_USER_DATA_MASK UDS_DTC_STATUS_AVAILABILITY_MASK + 1U
#define UDS_DTC_AND_STATUS_RECORD_MASK_FOR_DTC 3U

#define NRC_BYTE_MASK 2U

#define SFID_LENGHT 1U
#define UDS_MESSAGE_HEADER_LENGHT 2U

namespace rdgapp {

enum class UDS_SID : uint8_t
{
    SID_10_SESSION_CONTROL                                                      = 0x10U,
    SID_11_ECU_RESET                                                            = 0x11U,
    SID_13_READ_DTC_PH4                                                         = 0x13U,
    SID_14_CLEAR_DIAG_INFORMATION                                               = 0x14U,
    SID_19_READ_DTC_INFORMATION                                                 = 0x19U,
    SID_22_READ_DATA_BY_IDENTIFIER                                              = 0x22U,
    SID_23_READ_MEMORY_BY_ADDRESS                                               = 0x23U,
    SID_27_SECURITY_ACCESS                                                      = 0x27U,
    SID_28_COMMUNICATION_CONTROL                                                = 0x28U,
    SID_29_AUTHENTICATION                                                       = 0x29U,
    SID_2A_READ_DATA_BY_PERIODIC_IDENTIFIER                                     = 0x2AU,
    SID_2C_DYNAMICALLY_DEFINE_DATA_IDENTIFIER                                   = 0x2CU,
    SID_2E_WRITE_DATA_BY_IDENTIFIER                                             = 0x2EU,
    SID_2F_IO_CONTROL_BY_IDENTIFIER                                             = 0x2FU,
    SID_31_ROUTINE_CONTROL                                                      = 0x31U,
    SID_34_REQUEST_DOWNLOAD                                                     = 0x34U,
    SID_36_TRANSFER_DATA                                                        = 0x36U,
    SID_37_REQUEST_TRANSFER_EXIT                                                = 0x37U,
    SID_38_REQUEST_FILE_TRANSFER                                                = 0x38U,
    SID_3E_TESTER_PRESENT                                                       = 0x3EU,
    SID_85_CONTROL_DTC_SETTING                                                  = 0x85U,
    SID_86_RESPONSE_ON_EVENT                                                    = 0x86U,
    SID_AB_READ_ROB_INFORMATION                                                 = 0xABU,
};

enum class UDS_RESPONSE_CODE: uint8_t {
    UDS_POSITIVE_RESPONSE_OFFSET                                                = 0x40U,
    UDS_NEGATIVE_RESPONSE                                                       = 0x7FU,

    UDS_PR_SESSION_CONTROL                                                      = 0x50U,
    UDS_PR_CLEAR_DIAG_INFO                                                      = 0x54U,
    UDS_PR_READ_DTC_INFORMATION_PHASE4                                          = 0x53U,
    UDS_PR_READ_DTC_INFORMATION                                                 = 0x59U,
    UDS_PR_READ_DATA_BY_IDENTIFIER                                              = 0x62U,
    UDS_PR_SECURITY_ACCESS                                                      = 0x67U,
    UDS_PR_COMMUNICATION_CONTROL                                                = 0x68U,
    UDS_PR_AUTHENTICATION                                                       = 0x69U,
    UDS_PR_READ_DATA_BY_PERIODIC_IDENTIFIER                                     = 0x6AU,
    UDS_PR_DYNAMICALLY_DEFINE_DATA_IDENTIFIER                                   = 0x6CU,
    UDS_PR_WRITE_DATA_BY_IDENTIFIER                                             = 0x6EU,
    UDS_PR_IO_CONTROL                                                           = 0x6FU,
    UDS_PR_ROUTINE_CONTROL                                                      = 0x71U,
    UDS_PR_REQUEST_DOWNLOAD                                                     = 0x74U,
    UDS_PR_TRANSFER_DATA                                                        = 0x76U,
    UDS_PR_REQUEST_TRANSFER_EXIT                                                = 0x77U,
    UDS_PR_REQUEST_FILE_TRANSFER                                                = 0x78U,
    UDS_PR_TESTER_PRESENT                                                       = 0x7EU,
    UDS_PR_CONTROL_DTC_SETTING                                                  = 0xC5U,
    UDS_PR_RESPONSE_ON_EVENT                                                    = 0xC6U,
    UDS_PR_ECU_RESET                                                            = 0x51U,
    UDS_PR_RESPONSE_READ_ROB_INFORMATION                                        = 0xEBU,
};

enum class UDS_READ_DTC_INFO_SFID : uint8_t 
{
    SFID_01_REPORT_NUMBER_OF_DTC_BY_STATUS_MASK                                 = 0x01U,
    SFID_02_REPORT_DTC_BY_STATUS_MASK                                           = 0x02U,
    SFID_03_REPORT_DTC_SNAPSHOT_IDENTIFICATION                                  = 0x03U,
    SFID_04_REPORT_DTC_SNAPSHOT_RECORD_BY_DTC_NUMBER                            = 0x04U,
    SFID_05_REPORT_DTC_STORED_DATA_BY_RECORD_NUMBER                             = 0x05U,
    SFID_06_REPORT_DTC_EXT_DATA_RECORD_BY_DTC_NUMBER                            = 0x06U,
    SFID_07_REPORT_NUMBER_OF_DTC_BY_SEVERITY_MASK_RECORD                        = 0x07U,
    SFID_08_REPORT_DTC_BY_SEVERITY_MASK_RECORD                                  = 0x08U,
    SFID_09_REPORT_SEVERITY_INFO_OF_DTC                                         = 0x09U,
    SFID_0A_REPORT_SUPPORTED_DTC                                                = 0x0AU,
    SFID_0B_REPORT_FIRST_TEST_FAILED_DTC                                        = 0x0BU,
    SFID_0D_REPORT_MOST_RECENT_TEST_FAILED_DTC                                  = 0x0DU,
    SFID_0C_REPORT_FIRST_CONFIRMED_DTC                                          = 0x0CU,
    SFID_0E_REPORT_MOST_RECENT_CONFIRMED_DTC                                    = 0x0EU,
    SFID_14_REPORT_DTC_FAULT_DETECTION_COUNTER                                  = 0x14U,
    SFID_15_REPORT_DTC_WITH_PERMANENT_STATUS                                    = 0x15U,
    SFID_16_REPORT_DTC_EXT_DATA_RECORD_BY_RECORD_NUMBER                         = 0x16U,
    SFID_17_REPORT_USER_DEF_MEMORY_DTC_BY_STATUS_MASK                           = 0x17U,
    SFID_18_REPORT_USER_DEF_MEMORY_DTC_SNAPSHOT_RECORD_BY_DTC_NUMBER            = 0x18U,
    SFID_19_REPORT_USER_DEF_MEMORY_DTC_EXT_DATA_RECORD_BY_DTC_NUMBER            = 0x19U,
    SFID_1A_REPORT_SUPPORTED_DTC_EXT_DATA_RECORD                                = 0x1AU,
    SFID_42_REPORT_REPORT_WWHOBDDTC_BY_MASK_RECORD                              = 0x42U,
    SFID_56_REPORT_DTC_INFO_BY_DTC_READINESS_GROUP_IDENTIFIER                   = 0x56U,
    SFID_80 = 0x80U,
    SFID_81 = 0x81U,
    SFID_B0_READ_DTC = 0xB0U,
};

// typedef struct{
//     unsigned TestFailed : 1;
//     unsigned TestFailedThisOperationCycle : 1;
//     unsigned PendingDtc : 1;
//     unsigned ConfirmedDtc : 1;
//     unsigned TestNotCompletedSinceLastClear : 1;
//     unsigned TestFailedSinceLastClear : 1;
//     unsigned TestNotCompletedThisOperationCycle : 1;
//     unsigned WarningIndicatorRequested : 1;
// } DtcStatusMask_t;
}
#endif /* REMOTEDIAG_UDS_MESSAGE_DEFINE_H */
