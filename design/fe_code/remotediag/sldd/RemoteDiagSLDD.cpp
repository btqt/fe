#include "RemoteDiagSLDD.h"
#include <sstream>
#include <iomanip>
#ifdef ENABLE_LGE_LXC
#include <services/DcemqttproxyManagerService/DceNotification.h>
#include "..//services/OnboardclientManagerAdapter.h"
#else
#include "../services_org/services/OnboardclientManagerAdapter.h"
#endif /* ENABLE_LGE_LXC */

#include "diagprocess/CenterReqData.h"
#include "diagprocess/OTA/RemoteOTA.h"
#include "diagprocess/RoBOccurrence/RoBOccurrence.h"
#include "utils/CollectionCondition.h"
#include "utils/UploadManager.h"

namespace rdgapp {
    // CMD TEST---BEGIN----
    constexpr static int32_t MSG_SLDD_TEST_INIT{10000};
    constexpr static int32_t MSG_SLDD_TEST_START_DIRECTCOMMAND{10001}; // CMD TEST START DIRECT_COMMAND
    constexpr static int32_t MSG_SLDD_TEST_SEND_CONNECT{10002};
    constexpr static int32_t MSG_SLDD_TEST_SEND_DISCONNECT{10003};
    constexpr static int32_t MSG_SLDD_TEST_DIAGTRIGGER{10004}; // MSG_SLDD_TEST_DIAGTRIGGER + Priority + DiagTriggerFunc
    constexpr static int32_t MSG_SLDD_TEST_OTA_GET_OCB_RESOURCE_REQ{10005};
    constexpr static int32_t MSG_SLDD_TEST_OTA_CONNECT_REQ{10006};
    constexpr static int32_t MSG_SLDD_TEST_OTA_DISCONNECT_REQ{10007};
    constexpr static int32_t MSG_SLDD_TEST_OTA_SEND_UDS_DATA_REQ{10008};
    constexpr static int32_t MSG_SLDD_TEST_OTA_RELEASE_OBC_RESOURCE_REQ{10009};
    constexpr static int32_t MSG_SLDD_TEST_DIAGTRIGGER_DONE{10010}; // arg1: int32_t pTriggerId, arg2: DiagTrigger::DiagTriggerType type)
    constexpr static int32_t MSG_SLDD_TEST_SEND_UDS{10011};
    constexpr static int32_t MSG_SLDD_TEST_ROB_PROCESSING{10012};
    constexpr static int32_t MSG_SLDD_TEST_COLLECTION_CONDITION{10013};
    constexpr static int32_t MSG_SLDD_TEST_PRINT_COLLECTION_CONDITION{10014};
    constexpr static int32_t MSG_SLDD_TEST_INSERT_ECU_INFO{10015}; // arg1: uint32_t targetAddress, arg2: uint16_t canId
    constexpr static int32_t MSG_SLDD_TEST_COLLECTION_CONDITION_NEW{10016};
    constexpr static int32_t MSG_SLDD_TEST_CLEAR_WARNING_COUNTER{10017};
    constexpr static int32_t MSG_SLDD_TEST_START_ECU_INFORMATION{10018};
    constexpr static int32_t MSG_SLDD_TEST_MSG_POWR_ON_IGN_ON{10019};
    constexpr static int32_t MSG_SLDD_TEST_MSG_POWR_ON_IGN_OFF{10020};
    constexpr static int32_t MSG_SLDD_TEST_LOAD_ECU_LIST{10021};
    constexpr static int32_t MSG_SLDD_TEST_ADD_UPLOADTASK{10022};
    constexpr static int32_t MSG_SLDD_TEST_CENTER_PUSH_NOTIFICATION{10023};
    constexpr static int32_t MSG_SLDD_TEST_CLEAR_ROB_MONITORING{10024};
    constexpr static int32_t MSG_SLDD_TEST_NOTIFY_OCCURRENT_ROB_DETECTION_PROCESS_DONE{10025};
    constexpr static int32_t MSG_SLDD_TEST_SELFDIAG_IGON_OFF{10026};
    constexpr static int32_t MSG_SLDD_TEST_SELFDIAG_FAILURE_COLLECTION_CONDITION{10027};
    constexpr static int32_t MSG_SLDD_TEST_SELFDIAG_SUCCESS_READ_NOTIFICATION_TRIGGER{10028};
    constexpr static int32_t MSG_SLDD_TEST_SELFDIAG_SUCCESSFULLY_FILE_CREATION{10029};
    constexpr static int32_t MSG_SLDD_TEST_SELFDIAG_STOP_OPERATION{10030};
    constexpr static int32_t MSG_SLDD_TEST_SELFDIAG_NO_CENTER_RESPONSE{10031};
    constexpr static int32_t MSG_SLDD_TEST_STORE_COLLECTION_CONDITION{10032};
    constexpr static int32_t MSG_SLDD_TEST_SELFDIAG_ECU_USER_DEF_MEMORY_DTC{10033};
    constexpr static int32_t MSG_SLDD_TEST_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR_PROCESS_DONE{10034};
    constexpr static int32_t MSG_SLDD_TEST_WARNING_TRIGGER_ROB{10035};
    constexpr static int32_t MSG_SLDD_TEST_SAVE_ECU_LIST{10036};
    constexpr static int32_t MSG_SLDD_TEST_CHANGE_EXCEEDED_UPLOAD_SIZE{10037};
    constexpr static int32_t MSG_SLDD_TEST_COLLECTION_CONDITION_BIN_DATA{10038};
    constexpr static int32_t MSG_SLDD_TEST_COLLECTION_CONDITION_UPDLOAD_END{10039};
    constexpr static int32_t MSG_SLDD_TEST_SET_UPLOAD_STORAGE{10040};
    constexpr static int32_t MSG_SLDD_TEST_GET_UPLOAD_STORAGE{10041};
    constexpr static int32_t MSG_SLDD_TEST_SET_COUNTER_VALUE{10042};

    // CMD TEST---END------
android::sp<RemoteDiagSLDD> RemoteDiagSLDD::mRemoteDiagSLDD{nullptr};
android::Mutex RemoteDiagSLDD::mInstanceLock{};
RemoteDiagSLDD::RemoteDiagSLDD() : android::RefBase()
{
    mRemoteDiagSLDD = this;
    mHandler = RemotediagHandler::getInstance();
    mCurrentConnectId = 0U;
}

RemoteDiagSLDD::~RemoteDiagSLDD()
{
    if (mRemoteDiagSLDD != nullptr)
    {
        mRemoteDiagSLDD.clear();
    }
}

android::sp<RemoteDiagSLDD> RemoteDiagSLDD::getInstance()
{
    if (mRemoteDiagSLDD == nullptr)
    {
        const android::AutoMutex _l{mInstanceLock};
        if (mRemoteDiagSLDD == nullptr)
        {
            mRemoteDiagSLDD = new RemoteDiagSLDD();
        }
    }
    return mRemoteDiagSLDD;
}

void RemoteDiagSLDD::runRemoteDiagSlddTesting(const int32_t what, const int32_t arg1, const int32_t arg2)
{
    LOG_I("Run SLDD testing: cmd: %d, arg1: %d, arg2: %d", what, arg1, arg2);
    switch (what)
    {
    case MSG_SLDD_TEST_INIT:
    {
        LOG_I("Init testing");
        break;
    }
    case MSG_SLDD_TEST_START_DIRECTCOMMAND:
    {
        LOG_I("Start testing DirectCommand");
        // (void)RemoteDirectCommand::getInstance()->forSLDDTesting(what, arg1, arg2);
        break;
    }
    case MSG_SLDD_TEST_SEND_CONNECT:
    {
        const android::sp<OBCTransportInfo> transportInfo{new OBCTransportInfo()};
        const android::sp<OBCConnectInfo> connectInfo{new OBCConnectInfo()};
        OBCCanInfo mcanInfo{};
        const uint32_t mcanId{0U};
        std::vector<std::string> mnTa;
        mnTa.push_back("F");
        mnTa.push_back("E");
        mcanInfo.setData(mcanId, mnTa);
        transportInfo->setData(1U, mcanInfo, false, 1000U);
        const std::string applicationName{"remotediag"};
        (void)OnboardclientAdapter::getInstance()->connect(transportInfo, applicationName, connectInfo);
        break;
    }
    case MSG_SLDD_TEST_SEND_DISCONNECT:
    {
        (void)OnboardclientAdapter::getInstance()->disconnectECU(2U);
        break;
    }
    case MSG_SLDD_TEST_DIAGTRIGGER:
    {
        const uint32_t TriggerId{TriggerIDGenerator::getInstance().getNextId()};
        const android::sp<DiagTrigger> pTrigger{new DiagTrigger(DiagTrigger::DiagTriggerType::CUSTOMIZE_TRIGGER,
                                                                static_cast<uint32_t>(arg1), static_cast<DiagTrigger::DiagTriggerFunc>(arg2), TriggerId)};
       //TimeManager &mTimeManagerService = TimeManager::getInstance();
        int64_t current_time{0};
        current_time = CommonUtils::getCurrentAcquisiteTime();
        pTrigger->setTriggerTime(current_time);
        LOG_I("check time: %lld sec", pTrigger->getTriggerTime());
        PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
        break;
    }
    case MSG_SLDD_TEST_OTA_GET_OCB_RESOURCE_REQ:
    {
        LOG_I("Start testing OTA get OBC resource request");
        const OTAPriorityType priority{static_cast<OTAPriorityType>(arg1)};
        uint8_t payload[] = {0x01U, 0x01U, 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x01U, 0x01U};
        const android::sp<::Buffer> rawMessage{new ::Buffer()};
        // set OTA MID
        payload[1] = static_cast<uint8_t>(OTA_MID::OTA_MID_1_GET_OBC_RESOURCE_REQ);
        payload[8] = static_cast<uint8_t>(priority);

        rawMessage->setTo(payload, sizeof(payload));
        mFa.startup();
        const android::sp<OtaMessage> res = mFa.sendData(rawMessage);
        if (res->Payload()->empty() == false)
        {
            const OBCResourceEventCode resCode = static_cast<OBCResourceEventCode>(res->Payload()->data()[0]);
            if (resCode == OBCResourceEventCode::OBC_GET_RESOURCE_OK)
            {
                LOG_I("Received get OBC resource response, OBCResourceEventCode = OBC_GET_RESOURCE_OK");
            }
            else if (resCode == OBCResourceEventCode::OBC_GET_RESOURCE_WAIT)
            {
                LOG_I("Received get OBC resource response, OBCResourceEventCode = OBC_GET_RESOURCE_WAIT");
            }
            else
            {
                LOG_E("Received wrong get OBC resource response, OBCResourceEventCode = %d", resCode);
            }
        }
        break;
    }
    case MSG_SLDD_TEST_OTA_CONNECT_REQ:
    {
        uint32_t canId{0U};
        uint8_t protocolType{0U};

        canId = static_cast<uint32_t>(arg1);
        protocolType = static_cast<uint8_t>(arg2);

        LOG_I("Start testing OTA connect request, target canId = 0x%02X, protocoltype = 0x%02X", canId, protocolType);
        std::vector<uint8_t> payload {};
        // uint8_t payloadSendUDS[] = { 0x01,0x05,0x00,0x01,0x00,0x00,0x00,0x07,0x00,0x01,0x00,0x03,0x22,0xF1,0x88 };
        // uint8_t payloadDisconnect[] = { 0x01,0x04,0x00,0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00 };
        // uint8_t payloadReleaseUDS[] = { 0x01,0x02,0x00,0x01, 0x00, 0x00, 0x00, 0x00 };
        const android::sp<OtaMessage> otaResMessage{new OtaMessage(OTA_MID::OTA_MID_3_CONNECT_REQ)};
        // FaClient fa;
        //=================================================================
        otaResMessage->SetSequenceNumber(1234U);
        payload.push_back(protocolType); 
        // add canid
        payload.push_back(0xFFU & static_cast<uint8_t>(canId >> 24U));
        payload.push_back(0xFFU & static_cast<uint8_t>(canId >> 16U));
        payload.push_back(0xFFU & static_cast<uint8_t>(canId >> 8U));
        payload.push_back(0xFFU & static_cast<uint8_t>(canId));

        const uint16_t nTa{static_cast<uint16_t>(((canId >> 8U) & 0xFFU))};
        std::stringstream ss{};
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << nTa;
        const std::string hexString{ss.str()}; // Convert to string
        const uint32_t numberChar {static_cast<uint32_t>(static_cast<uint32_t>(hexString.length()) + 1U)};
        // set nta lenght
        payload.push_back(0xFFU & static_cast<uint8_t>(numberChar >> 8U));
        payload.push_back(0xFFU & static_cast<uint8_t>(numberChar));

        payload.push_back(static_cast<uint8_t>(hexString[0]));
        payload.push_back(static_cast<uint8_t>(hexString[1]));
        payload.push_back(0X00U);
        // add periodicRes
        payload.push_back(0X00U);
        // add udsResTimeout;
        const uint32_t udsResTimeout {30U};
        payload.push_back(0xFFU & static_cast<uint8_t>(udsResTimeout >> 8U));
        payload.push_back(0xFFU & static_cast<uint8_t>(udsResTimeout));

        const uint32_t payloadSize {static_cast<uint32_t>(payload.size())};

        if (payloadSize <= static_cast<uint32_t>(INT32_MAX))
        {
            otaResMessage->Payload()->setTo(payload.data(), static_cast<int32_t>(payloadSize));
        } else {
            LOG_E("payloadSize out of range with int32_t, payloadSize = %d", payloadSize);
        }

        const android::sp<OtaMessage> res{mFa.sendData(otaResMessage->ToRaw())};
        if (res->Payload()->empty() == false)
        {
            const uint8_t resCode = res->Payload()->data()[0];
            LOG_I("Connect response success, response code = 0x%02X", resCode);
            if (resCode == static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK))
            {
                mCurrentConnectId = (res->Payload()->data()[1] << 8U) | res->Payload()->data()[2];
                LOG_I("Connected success, connectId = %d", mCurrentConnectId);
            }
        }
        break;
    }
    case MSG_SLDD_TEST_OTA_SEND_UDS_DATA_REQ:
    {
        LOG_I("Start testing OTA send UDS data request");
        const android::sp<::Buffer> rawMessage{new ::Buffer()};
        // uint8_t payload[] = {0x01U, 0x05U, 0x00U, 0x03U, 0x00U, 0x00U, 0x00U, 0x07U, 0x00U, 0x01U, 0x00U, 0x03U, 0x22U, 0xF1U, 0x88U};
        // std::vector<uint8_t> payload {0x01U, 0x05U, 0x00U, 0x03U, 0x00U, 0x00U, 0x04U, 0x06U, 0x00U, 0x01U, 0x04U, 0x02U};
        // //=================================================================
        // uint16_t connectId{static_cast<uint16_t>(arg1)};
        // if ((connectId <= 0U) && (mCurrentConnectId > 0U))
        // {
        //     connectId = mCurrentConnectId;
        // }
        // // set OTA MID
        // payload[1] = static_cast<uint8_t>(OTA_MID::OTA_MID_5_SEND_UDS_DATA_REQ);

        // payload[8] = 0xFFU & static_cast<uint8_t>(connectId >> 8U);
        // payload[9] = 0xFFU & (connectId);
        // for (uint32_t i {0U}; i < 1026U; i++)
        // {
        //     payload.push_back(0x09U);
        // }


        std::vector<uint8_t> payload {0x01U, 0x05U, 0x00U, 0x03U, 0x00U, 0x00U, 0x07U, 0xF8U, 0x00U, 0x01U, 0x07U, 0xF4U};
        //=================================================================
        uint16_t connectId{static_cast<uint16_t>(arg1)};
        if ((connectId <= 0U) && (mCurrentConnectId > 0U))
        {
            connectId = mCurrentConnectId;
        }
        // set OTA MID
        payload[1] = static_cast<uint8_t>(OTA_MID::OTA_MID_5_SEND_UDS_DATA_REQ);

        payload[8] = 0xFFU & static_cast<uint8_t>(connectId >> 8U);
        payload[9] = 0xFFU & (connectId);
        for (uint32_t i {0U}; i < 2036U; i++)
        {
            payload.push_back(0x09U);
        }


        rawMessage->setTo(payload.data(), static_cast<int32_t>(payload.size()));
        const android::sp<OtaMessage> res{mFa.sendData(rawMessage)};
        if (res->Payload()->empty() == false)
        {
            LOG_I("Response Event notify Data:");
            for (uint32_t i = 0U; i < res->Payload()->size(); i++)
            {
                LOG_I("usdData[%d] = 0x%02x", i, res->Payload()->data()[i]);
            }
        }
        break;
    }
    case MSG_SLDD_TEST_OTA_DISCONNECT_REQ:
    {
        uint8_t payload[] = {0x01U, 0x04U, 0x00U, 0x04U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U};
        const android::sp<::Buffer> rawMessage{new ::Buffer()};
        uint16_t connectId{static_cast<uint16_t>(arg1)};
        if ((connectId <= 0U) && (mCurrentConnectId > 0U))
        {
            connectId = mCurrentConnectId;
        }
        // set OTA MID
        payload[1] = static_cast<uint8_t>(OTA_MID::OTA_MID_4_DISCONNECT_REQ);

        payload[8] = 0xFFU & static_cast<uint8_t>(connectId >> 8U);
        payload[9] = 0xFFU & static_cast<uint8_t>(connectId);
        LOG_I("Start testing OTA disconnect request, target connectId = %d", connectId);
        rawMessage->setTo(payload, sizeof(payload));
        const android::sp<OtaMessage> res = mFa.sendData(rawMessage);
        if (res->Payload()->empty() == false)
        {
            const uint8_t resCode = res->Payload()->data()[0];
            LOG_I("Disconnect response success, response code = 0x%02X", resCode);
        }

        break;
    }
    case MSG_SLDD_TEST_OTA_RELEASE_OBC_RESOURCE_REQ:
    {
        LOG_I("Start testing OTA release OBC resource request notify");
        uint8_t payload[] = {0x01U, 0x02U, 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U};
        const android::sp<::Buffer> rawMessage{new ::Buffer()};
        // set OTA MID
        payload[1] = static_cast<uint8_t>(OTA_MID::OTA_MID_2_RELEASE_OBC_RESOURCE_REQ);

        rawMessage->setTo(payload, sizeof(payload));
        const android::sp<OtaMessage> res{mFa.sendData(rawMessage)};
        if (res->Payload()->empty() == false)
        {
            const OBCResourceEventCode resCode = static_cast<OBCResourceEventCode>(res->Payload()->data()[0]);
            if (resCode == OBCResourceEventCode::OBC_RELEASE_RESOURCE_OK)
            {
                LOG_I("Received release OBC resource response, OBCResourceEventCode = OBC_RELEASE_RESOURCE_OK");
            }
            else
            {
                LOG_E("Received wrong release OBC resource response, OBCResourceEventCode = %d", resCode);
            }
        }
        mFa.stop();
        break;
    }
    case MSG_SLDD_TEST_DIAGTRIGGER_DONE:
    {
        PriorityControl::getInstance()->notifyTriggerProcessDone(arg1, static_cast<DiagTrigger::DiagTriggerType>(arg2));
        break;
    }
    case MSG_SLDD_TEST_SEND_UDS:
    {
        LOG_I("MSG_SLDD_TEST_SEND_UDS");
        const uint16_t connectId{static_cast<uint16_t>(arg1)};
        const android::sp<::Buffer> udsRequest{new ::Buffer()};
        const android::sp<::Buffer> nextUdsRequest{new ::Buffer()};
        uint8_t data[3];
        data[0] = 0x22U;
        data[1] = 0xF1U;
        data[1] = 0xF1U;
        udsRequest->setTo(data, 3);
        uint8_t reqData2[2];
        reqData2[0] = 0x10U;
        reqData2[1] = 0x01U;
        nextUdsRequest->setTo(reqData2, 2);
        (void)OnboardclientAdapter::getInstance()->sendUdsData(connectId, udsRequest);
        (void)usleep(static_cast<useconds_t>(arg2));
        (void)OnboardclientAdapter::getInstance()->sendUdsData(connectId, nextUdsRequest);
        break;
    }
    case MSG_SLDD_TEST_ROB_PROCESSING:
    {
        LOG_I("MSG_SLDD_TEST_ROB_PROCESSING");
        const android::sp<CenterReqData> req{new CenterReqData(12345678U, 12U, 50U, DiagTrigger::DiagTriggerType::CENTER_TRIGGER)};
        RemoteRoB::getInstance().onCenterCommandForward(req);
        break;
    }
    case MSG_SLDD_TEST_COLLECTION_CONDITION:
    {
        LOG_I("MSG_SLDD_TEST_COLLECTION_CONDITION");
        CollectionCondition::getInstance().testReceivedCollectionConditionResponse();
        break;
    }
    case MSG_SLDD_TEST_COLLECTION_CONDITION_BIN_DATA:
    {
        LOG_I("MSG_SLDD_TEST_COLLECTION_CONDITION_BIN_DATA");
        CollectionCondition::getInstance().testReceivedCollectionConditionResponseBinData();
        break;
    }
    case MSG_SLDD_TEST_PRINT_COLLECTION_CONDITION:
    {
        LOG_I("MSG_SLDD_TEST_PRINT_COLLECTION_CONDITION");
        CollectionCondition::getInstance().testPrintCollectionConditionData();

        break;
    }
    case MSG_SLDD_TEST_INSERT_ECU_INFO:
    {
        LOG_I("MSG_SLDD_TEST_INSERT_ECU_INFO");
        CommonDefine::EcuInformation ecuInformation;
        ecuInformation.setTargetAddress(static_cast<uint32_t>(arg1));
        ecuInformation.setCanId(static_cast<uint32_t>(arg2));
        ecuInformation.setCommProtocol(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN);
        ecuInformation.setCommType(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS);
        ecuInformation.setDiagPhase(CommonDefine::DiagPhase::DP_PHASE_6);
        ecuInformation.setecuActiveFlag(true);
        ecuInformation.setNTa(0x00U);
        RemoteEcuInformation::getInstance()->insertEcuInformation(ecuInformation);
        break;
    }
    case MSG_SLDD_TEST_START_ECU_INFORMATION:
    {
        LOG_I("MSG_SLDD_TEST_START_ECU_INFORMATION");
        RemoteEcuInformation::getInstance()->testStartDiagTask();
        break;
    }
    case MSG_SLDD_TEST_COLLECTION_CONDITION_NEW:
    {
        LOG_I("MSG_SLDD_TEST_COLLECTION_CONDITION_NEW");
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_NEW_COLLECTION_CONDITION)->sendToTarget();
        break;
    }
    case MSG_SLDD_TEST_CLEAR_WARNING_COUNTER:
    {
        LOG_I("MSG_SLDD_TEST_CLEAR_WARNING_COUNTER");
        RemoteWarning::getInstance()->testClearWarningCounter();
        break;
    }
    case MSG_SLDD_TEST_MSG_POWR_ON_IGN_ON:
    {
        LOG_I("MSG_SLDD_TEST_MSG_POWR_ON_IGN_ON");
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_ON)->sendToTarget();
        break;
    }
    case MSG_SLDD_TEST_MSG_POWR_ON_IGN_OFF:
    {
        LOG_I("MSG_SLDD_TEST_MSG_POWR_ON_IGN_OFF");
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_OFF)->sendToTarget();
        break;
    }
    case MSG_SLDD_TEST_LOAD_ECU_LIST:
    {
        LOG_I("MSG_SLDD_TEST_LOAD_ECU_LIST");
        RemoteEcuInformation::getInstance()->testLoadEcuInformationFromFile();
        break;
    }
    case MSG_SLDD_TEST_SAVE_ECU_LIST:
    {
        LOG_I("MSG_SLDD_TEST_SAVE_ECU_LIST");
        RemoteEcuInformation::getInstance()->testSaveEcuInformationToFile();
        break;
    }
    case MSG_SLDD_TEST_ADD_UPLOADTASK:
    {
        LOG_I("MSG_SLDD_TEST_ADD_UPLOADTASK");
        break;
    }
    case MSG_SLDD_TEST_CENTER_PUSH_NOTIFICATION:
    {
        LOG_I("MSG_SLDD_TEST_CENTER_PUSH_NOTIFICATION");
        const android::sp<DceNotification> pDataEvent{new DceNotification()};
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_CENTER_PUSH_RECEIVED, pDataEvent)->sendToTarget();
        break;
    }
    case MSG_SLDD_TEST_CLEAR_ROB_MONITORING:
    {
        LOG_I("MSG_SLDD_TEST_CLEAR_ROB_MONITORING");
        (void)RoBMonitoring::getInstance()->clearRoBInformationList();
        break;
    }
    case MSG_SLDD_TEST_NOTIFY_OCCURRENT_ROB_DETECTION_PROCESS_DONE:
    {
        LOG_I("MSG_NOTIFY_OCCURRENT_ROB_DETECTION_PROCESS_DONE");
        RoBOccurrence::getInstance()->onDirectCommandCompleted(true, 0U);
        break;
    }
    case MSG_SLDD_TEST_SELFDIAG_IGON_OFF:
    {
        LOG_I("MSG_SLDD_TEST_SELFDIAG_IGON_OFF");
        bool isIgOn{false};
        const uint8_t type{static_cast<uint8_t>(arg2)};
        if (arg1 == 1)
        {
            isIgOn = true;
        }
        const android::sp<::Buffer> timeData{new ::Buffer()};
        CommonUtils::convertCurrentTimeToBuffer(timeData);
        DiagManagerAdapter::getInstance()->selfDiagIgOnOffTimes(isIgOn, type, timeData);
        break;
    }
    case MSG_SLDD_TEST_SELFDIAG_FAILURE_COLLECTION_CONDITION:
    {
        LOG_I("MSG_SLDD_TEST_SELFDIAG_FAILURE_COLLECTION_CONDITION");
        const uint64_t collectionCondition_data = 0xFEDCBA9876543210U;
        DiagManagerAdapter::getInstance()->selfDiagCollectionCondition(collectionCondition_data);
        break;
    }
    case MSG_SLDD_TEST_SELFDIAG_SUCCESS_READ_NOTIFICATION_TRIGGER:
    {
        LOG_I("MSG_SLDD_TEST_SELFDIAG_SUCCESS_READ_NOTIFICATION_TRIGGER");
        const uint8_t triggerType{static_cast<uint8_t>(arg1)};
        DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(triggerType);
        break;
    }
    case MSG_SLDD_TEST_SELFDIAG_SUCCESSFULLY_FILE_CREATION:
    {
        LOG_I("MSG_SLDD_TEST_SELFDIAG_SUCCESSFULLY_FILE_CREATION");
        const uint8_t triggerType{static_cast<uint8_t>(arg1)};
        DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(triggerType);
        break;
    }
    case MSG_SLDD_TEST_SELFDIAG_STOP_OPERATION:
    {
        LOG_I("MSG_SLDD_TEST_SELFDIAG_STOP_OPERATION");
        const uint8_t operation{static_cast<uint8_t>(arg1)};
        DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(operation);
        break;
    }
    case MSG_SLDD_TEST_SELFDIAG_NO_CENTER_RESPONSE:
    {
        LOG_I("MSG_SLDD_TEST_SELFDIAG_NO_CENTER_RESPONSE");
        DiagManagerAdapter::getInstance()->selfDiagNoCenterResponse();
        break;
    }
    case MSG_SLDD_TEST_STORE_COLLECTION_CONDITION:
    {
        LOG_I("MSG_SLDD_TEST_STORE_COLLECTION_CONDITION");
        const uint64_t collectionCondition_data = 0xFEDCBA9876543210U;
        DiagManagerAdapter::getInstance()->storeCollectionConditionId(collectionCondition_data);
        break;
    }
    case MSG_SLDD_TEST_SELFDIAG_ECU_USER_DEF_MEMORY_DTC:
    {
        LOG_I("MSG_SLDD_TEST_SELFDIAG_ECU_USER_DEF_MEMORY_DTC");
        // DiagManagerAdapter::getInstance()->selfDiagNoCenterResponse();
        break;
    }
    case MSG_SLDD_TEST_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR_PROCESS_DONE:
    {
        LOG_I("MSG_SLDD_TEST_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR_PROCESS_DONE");
        break;
    }
    case MSG_SLDD_TEST_WARNING_TRIGGER_ROB:
    {
        LOG_I("MSG_SLDD_TEST_WARNING_TRIGGER_ROB");
        RemoteRoB::getInstance().testHandleWarningTrigger(arg1, arg2);
        break;
    }
    case MSG_SLDD_TEST_CHANGE_EXCEEDED_UPLOAD_SIZE:
    {
        LOG_I("MSG_SLDD_TEST_CHANGE_EXCEEDED_UPLOAD_SIZE");
        if ((arg2 >=0) && (arg2 <= static_cast<int32_t>(UINT16_MAX)))
        {
            if(arg1 == 1)
            {
                RemoteDirectCommand::getInstance()->testingMaxFileSize(static_cast<uint16_t>(arg2));
            }
            else if (arg1 == 2)
            {
                RemoteRoBSSR::getInstance()->testingMaxFileSize(static_cast<uint16_t>(arg2));
            }
            else if (arg1 == 3)
            {
                RemoteSSR::getInstance()->testingMaxFileSize(static_cast<uint16_t>(arg2));
            }
            else if ((arg1 == 4) && (arg2 > 0))
            {
                RemoteRoB::getInstance().testingMaxFileSize(arg2);
            }
            else if (arg1 == 5)
            {
                RemoteDTC::getInstance()->testingMaxFileSize(static_cast<uint32_t>(arg2));
            }
            else
            {

            }
        }
        break;
    }
    case MSG_SLDD_TEST_COLLECTION_CONDITION_UPDLOAD_END:
    {
        LOG_I("MSG_SLDD_TEST_COLLECTION_CONDITION_UPDLOAD_END");
        mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_COLLECTION_CONDITION_UPDLOAD_END)->sendToTarget();
        break;
    }
    case MSG_SLDD_TEST_SET_UPLOAD_STORAGE:
    {
        LOG_I("MSG_SLDD_TEST_SET_UPLOAD_STORAGE");
        UploadManager::getInstance()->testSetUploadStorage(arg1, arg2);
        break;
    }
    case MSG_SLDD_TEST_GET_UPLOAD_STORAGE:
    {
        LOG_I("MSG_SLDD_TEST_GET_UPLOAD_STORAGE");
        UploadManager::getInstance()->testGetUploadStorage();
        break;
    }
    case MSG_SLDD_TEST_SET_COUNTER_VALUE:
    {
        LOG_I("MSG_SLDD_TEST_SET_COUNTER_VALUE");
        uint32_t testVal {0U};
        if (arg1 >= 0)
        {
            testVal = static_cast<uint32_t>(arg1);
        }
        else
        {
            LOG_E("Invalid params, assign to 0U");
        }
        UploadManager::getInstance()->testCounterValue(testVal);
        break;
    }
    default:
    {
        break;
    }
    }
}
}
