#include <iostream>
#include <sstream>
#include <random>
#include <cmath>
#include <cstdlib>
#include <utils/StrongPointer.h>
#include <utils/Singleton.h>
#include <utils/Mutex.h>

#include "ParamsDef.h"
#include "RemoteOTA.h"
#include "utils/Logger.h"
#include "Remotediag.h"
#include "OtaMessage.h"
#include "UdsMessage.h"
#include "FaServer.h"
#include "services/OnboardclientManagerAdapter.h"
#include "diagprocess/EcuInformation/EcuInformationDataType.h"

namespace rdgapp {

ANDROID_SINGLETON_STATIC_INSTANCE(rdgapp::RemoteOTA)
std::vector<uint8_t> RemoteOTA::SUPPORTED_SID { static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER), // Req: 0x22, Resp: 0x62
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_ROUTINE_CONTROL), // Req: 0x31, Resp: 0x71
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_SECURITY_ACCESS), // Req: 0x27, Resp: 0x67
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_REQUEST_DOWNLOAD), // Req: 0x34, Resp: 0x74
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_TRANSFER_DATA), // Req: 0x36, Resp: 0x76
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_REQUEST_TRANSFER_EXIT), // Req: 0x37, Resp: 0x77
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_WRITE_DATA_BY_IDENTIFIER), // Req: 0x2E, Resp: 0x6E
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_SESSION_CONTROL), // Req: 0x10, Resp: 0x50
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_TESTER_PRESENT), // Req: 0x3E, Resp: 0x7E
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_ECU_RESET), // Req: 0x11, Resp: 0x51 DCM24MON-1790
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION), // Req: 0x19, Resp: 0x59 DCM24MON-1790
                                                static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE), // Negative response
                                            };

RemoteOTA::RemoteOTA()
        : android::RefBase(), RemoteDelegate()
        , mOtaTimerHandler(*this)
        , mUdsResponseTimerHandler(*this)
        , mOtaTimer(&mOtaTimerHandler, TimerHandler::OTA_SESSION_TIMEOUT_TIMER_ID)
        , mUdsResponseTimer(&mUdsResponseTimerHandler, TimerHandler::UDS_RESPONSE_TIMEOUT_TIMER_ID)
        , mReqSeqNum(0U)
        , mCurrentTriggerId(0U)
        , mIsActive(true) // Communication between FA and RemoteDiag is enable by default
        , mCurrentConnectId(0U)
        , mCurrentUdsResTimeout(0U)
        , mOtaPriority(OTAPriorityType::UNKNOWN)
        , mState(RemoteOTA::State::REMOTE_OTA_STATE_IDLE)
{}

RemoteOTA::~RemoteOTA() 
{
    if ((mFaServer != nullptr) && mFaServer->isRunning())
    {
        mFaServer->stop();
    }
}

void RemoteOTA::MainHandler::handleMessage(const android::sp<sl::Message>& handlemsg)
{
    if (handlemsg != nullptr) {
        const int32_t whatCmd {handlemsg->what};
        switch (whatCmd) {
            case CMD_OTA_EXECULTE_REQ:
            {
                LOG_W("CMD_OTA_EXECULTE_REQ");
                android::sp<::Buffer> reqData {nullptr};
                handlemsg->getObject(reqData);
                if (reqData == nullptr)
                {
                    LOG_E("reqData is invalid");
                    break;
                }
                mOTA.execulteOtaReq(reqData);
            }
                break;
            case CMD_OTA_FA_CLIENT_DISCONNECTED:
            {
                LOG_W("CMD_OTA_FA_CLIENT_DISCONNECTED");
                break;
            }
            case CMD_OTA_ENABLE_STATE_TIMEOUT:
            {
                // RDG30-R-0535
                LOG_W("CMD_OTA_ENABLE_STATE_TIMEOUT");
                mOTA.handleOtaEnableStateTimeout();
                break;
            }
            case CMD_OTA_USD_RESPONSE_TIMEOUT_EVENT:
            {
                LOG_W("CMD_OTA_USD_RESPONSE_TIMEOUT_EVENT");
                mOTA.handleUdsResponseTimeout();
                break;
            }
            case CMD_OTA_DISCARD:
            {
                LOG_W("CMD_OTA_DISCARD");
                mOTA.handleDiscardedEvent();
                break;
            }
            case CMD_OTA_TRIGGER_PROCESSING:
            {
                LOG_W("CMD_OTA_TRIGGER_PROCESSING");
                mOTA.handleTriggerProcessing(handlemsg->arg1);
                break;
            }
            case CMD_OTA_WAITING_FRAGMENT_DATA:
            {
                LOG_W("CMD_OTA_WAITING_FRAGMENT_DATA");
                mOTA.updateOtaEnableStateTimeout();
                break;
            }
            case CMD_OTA_FA_CLIENT_TIMEOUT:
            {
                LOG_E("CMD_OTA_FA_CLIENT_TIMEOUT");
                android::sp<::Buffer> reqData {nullptr};
                handlemsg->getObject(reqData);
                if (reqData == nullptr)
                {
                    LOG_E("reqData is invalid");
                    break;
                }
                mOTA.handleSendUdsDataReqTimeout(reqData);
                break;
            }
            case CMD_STOP_RDG:
            {
                LOG_I("CMD_STOP_RDG");
                mOTA.handleStop();
                break;
            }
            case CMD_STOP_OLD_CONNECTION:
            {
                LOG_I("CMD_STOP_OLD_CONNECTION");
                mOTA.releaseObcResource();
                break;
            }
            default:
                break;
        }
    }
}

void RemoteOTA::execulteOtaReq(const android::sp<::Buffer> reqData)
{
    if (mIsActive == true) {
        const android::sp<OtaMessage> otaReqMessage {new OtaMessage()};
        if (otaReqMessage->Parser(reqData) != E_OK)
        {
            LOG_E("execulteOtaReq otaReqMessage parse error");
            goto exit;
        }

        if(otaReqMessage->FaProtoVer() > 1U)
        {
            LOG_E("FA protocol version %d isn't supported", otaReqMessage->FaProtoVer());
            goto exit;
        }

        switch(otaReqMessage->Mid())
        {
            case OTA_MID::OTA_MID_1_GET_OBC_RESOURCE_REQ:
                handleGetObcResourceReq(otaReqMessage);
                break;
            case OTA_MID::OTA_MID_2_RELEASE_OBC_RESOURCE_REQ:
                handleReleaseObcResourceReq(otaReqMessage);
                break;
            case OTA_MID::OTA_MID_3_CONNECT_REQ:
                handleConnectReq(otaReqMessage);
                break;
            case OTA_MID::OTA_MID_4_DISCONNECT_REQ:
                handleDisconnectReq(otaReqMessage);
                break;
            case OTA_MID::OTA_MID_5_SEND_UDS_DATA_REQ:
                handleSendUdsDataReq(otaReqMessage);
                break;
            default:
                LOG_E("OTA message id %d isn't supported", static_cast<uint8_t>(otaReqMessage->Mid()));
                break;
        }
    } else {
        LOG_E("FirewallManager disabled OTA function");
    }
exit:
    return;
}

void RemoteOTA::sendOtaRes(const android::sp<::Buffer> resData)
{
    // Use TCP response function to send response resData to FA
    uint8_t count {0U};
    while (count < 2U)
    {
        if (mFaServer->isRunning() == true) {
            if (mFaServer->notify(resData) == E_PENDING)
            {
                LOG_E("Retry %d", count);
                (void)sleep(1U);
                count++;
            } else {
                count = 2U;
            }
        } else {
            break;
        }
    }
}

void RemoteOTA::init(android::sp<sl::SLLooper>& privateLooper)
{
    mHandler = new MainHandler(privateLooper, *this);
    mOtaTimer.setDuration(TimerHandler::OTA_SESSION_TIMEOUT, 0U);
}

void RemoteOTA::startFaServer()
{
    LOG_E("Start FA Server");
    mFaServer = new FaServer(ParamsDef::FA_SERVER_DEFAULT_PORT, mHandler);
    mFaServer->startup();
}

void RemoteOTA::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
    if ((udsResponse != nullptr) && (responseEventInfo != nullptr))
    {
        const uint8_t SID {udsResponse->getSID()};
        const android::sp<OBCUDSResInfo> udsRespInfo {responseEventInfo->resInfo()};
        if ((mState == State::REMOTE_OTA_STATE_ENABLE) 
            && (udsRespInfo != nullptr)
            && (mCurrentConnectId == udsRespInfo->connectId())
            && (responseEventInfo->errCode() != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_TIMEOUT) )
            // filter by SID
            && (SID < static_cast<uint8_t>(UINT8_MAX))
            && (CheckSupportedSID(SID)))
        {
            LOG_D("canId = 0x%02x, connectId = %u", udsRespInfo->canInfo()->canId(), udsRespInfo->connectId());
            stopUdsResponseTimeout();
            responseEventNotify(responseEventInfo);
        } 
        else if ((mState == State::REMOTE_OTA_STATE_ENABLE) 
                && (mCurrentConnectId == udsRespInfo->connectId())
                && (responseEventInfo->errCode() == static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_TIMEOUT))) 
        {
            stopUdsResponseTimeout();
            responseEventNotify(responseEventInfo);
        } else {
            LOG_D("Not support connectId = %d, sid = 0x%02x", udsRespInfo->connectId(), SID);
        }
        (void)SID;
    }
}

void RemoteOTA::onFirewallActionDiagDisable()
{
    mIsActive = false;
    mFaServer->active(false);
    if (mState == State::REMOTE_OTA_STATE_ENABLE) {
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        mCurrentConnectId = 0U;
        mState = State::REMOTE_OTA_STATE_IDLE;
        mOtaPriority = OTAPriorityType::UNKNOWN;
    }
}

void RemoteOTA::onFirewallActionDiagEnable()
{
    mIsActive = true;
    mFaServer->active(true);
}

void RemoteOTA::handleConnectReq(const android::sp<OtaMessage>& otaReqMessage)
{
    const uint32_t payloadSize {otaReqMessage->PayloadSize()};
    if(payloadSize == 0U) 
    {
        LOG_E("Connect Request, Invalid Playload");
    } else {
        uint8_t protocolType {0U};
        (void)std::memcpy(&protocolType, otaReqMessage->Payload()->data() + OtaMessageDefs::PROTOCOL_TYPE_BYTE_MASK, 1U);
        const uint32_t targetAddress {CommonUtils::makeSerializeUint32(otaReqMessage->Payload(), OtaMessageDefs::CANID_BYTE_MASK)};
        const android::sp<OBCConnectInfo> obj {new OBCConnectInfo()};
        if (RemoteEcuInformation::getInstance()->getCanId(targetAddress) != 0U)
        {
            // RDG30-R-1266
            protocolType = static_cast<uint8_t>(RemoteEcuInformation::getInstance()->getObcProtocolType(targetAddress));
            const uint16_t nTaLeght {CommonUtils::makeSerializeUint16(otaReqMessage->Payload(), OtaMessageDefs::NTA_LENGHT_BYTE_MASK)};

            std::vector<std::string> ntaArray{};
            for (uint32_t i {0U}; i < nTaLeght; i += 3U)
            {
                uint8_t tempText1 {0U};
                uint8_t tempText2 {0U};
                if(otaReqMessage->Payload()->data() != nullptr) {
                    tempText1 = otaReqMessage->Payload()->data()[OtaMessageDefs::NTA_LENGHT_BYTE_MASK + 2U + i];
                    tempText2 = otaReqMessage->Payload()->data()[OtaMessageDefs::NTA_LENGHT_BYTE_MASK + 2U + i + 1U];
                }
                if ((tempText1 > 0U) && (tempText1 < 127U))
                {
                    const char_t text1 {static_cast<char_t>(tempText1)};
                    ntaArray.push_back(std::string(&text1, 1U));
                }

                if ((tempText2 > 0U) && (tempText2 < 127U))
                {
                    const char_t text2 {static_cast<char_t>(tempText2)};
                    ntaArray.push_back(std::string(&text2, 1U));
                }
            }

            const uint16_t udsResTimeout {CommonUtils::makeSerializeUint16(otaReqMessage->Payload(), OtaMessageDefs::NTA_LENGHT_BYTE_MASK + 2U + nTaLeght + OtaMessageDefs::PERIODIC_RES_BYTE_LENGHT)};
            mCurrentUdsResTimeout = udsResTimeout;
            LOG_D("Handle Connect request: protocolType %u, udsResTimeout: %d, targetAddress: 0x%02x, nTaLeght %u"
                , protocolType
                , udsResTimeout
                , targetAddress
                , nTaLeght);
            updateOtaEnableStateTimeout();
            const android::sp<OBCTransportInfo> obcTransportInfo {new OBCTransportInfo()};
            OBCCanInfo canInfo{};
            canInfo.setData(targetAddress, ntaArray);
            obcTransportInfo->setData(protocolType
                                    , canInfo
                                    , true  // Not using on OBC spec
                                    , udsResTimeout);
            
            const error_t res {OnboardclientAdapter::getInstance()->connect(obcTransportInfo, APP_NAME, obj)};

            if (res == E_OK) {
                if (obj->getResponse() == static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK)) 
                {
                    mCurrentConnectId = obj->getConnectId();
                    mCurrentResInfo.canInfo.canId = targetAddress;
                    if ((protocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN)) 
                        || (protocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT))
                        || (protocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX))
                        || (protocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
                    {
                        mCurrentResInfo.protocolType = static_cast<OBCEnum::OBCProtocolType>(protocolType);
                    }  else {
                        mCurrentResInfo.protocolType = OBCEnum::OBCProtocolType::UNKNOWN;
                    }
                    mCurrentResInfo.responseType = OBCEnum::OBCUdsResponseType::EVENT;
                    mCurrentResInfo.connectId = obj->getConnectId();
                } else {
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                    mCurrentConnectId = 0U;
                }
            } else {
                obj->setData(static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_FAILED), 0U);
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
                mCurrentConnectId = 0U;
            }
            connectResultNotify(otaReqMessage->SequenceNumber(), obj);
        } else {
            obj->setData(static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_INVALID_PARAMETERS), 0U);
            connectResultNotify(otaReqMessage->SequenceNumber(), obj);
        }
    }
}

void RemoteOTA::handleDisconnectReq(const android::sp<OtaMessage>& otaReqMessage)
{
    bool isOK {true};
    OBCEnum::OBCErrCode errorCode {OBCEnum::OBCErrCode::OBC_ERR_FAILED};
    updateOtaEnableStateTimeout();
    const uint32_t payloadSize {otaReqMessage->PayloadSize()};
    if((payloadSize > 2U) || (payloadSize == 0U))
    {
        LOG_E("Disconnect Request, Invalid Playload");
        errorCode = OBCEnum::OBCErrCode::OBC_ERR_INVALID_PARAMETERS;
        disconnectResultNotify(otaReqMessage->SequenceNumber(), errorCode);
        isOK = false;
    }

    if (isOK == true) 
    {
        const uint16_t connectId {CommonUtils::makeSerializeUint16(otaReqMessage->Payload(), OtaMessageDefs::CONNECT_ID_BYTE_MASK)};

        LOG_D("Disconnect Request, connectId: %d", connectId);

        const uint8_t tmp {OnboardclientAdapter::getInstance()->disconnectECU(connectId)};
        if (tmp > static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_MAX))
        {
            errorCode = OBCEnum::OBCErrCode::OBC_ERR_MAX;
        } else {
            errorCode = static_cast<OBCEnum::OBCErrCode>(tmp);
            if (errorCode == OBCEnum::OBCErrCode::OBC_OK)
            {
                mCurrentConnectId = 0U;
            }
        }
        disconnectResultNotify(otaReqMessage->SequenceNumber(), errorCode);
        (void)tmp;
    }
}

void RemoteOTA::handleGetObcResourceReq(const android::sp<OtaMessage>& otaReqMessage)
{
    bool isOK {true};
    if((otaReqMessage->PayloadSize() != 1U)) 
    {
        LOG_E("GetObcResource Request, Invalid Playload");
        isOK = false;
    }

    uint8_t priorityTmp {0U};
    if(otaReqMessage->Payload()->data() != nullptr) {
        (void)std::memcpy(&priorityTmp, otaReqMessage->Payload()->data(), 1U);
    } else {
        LOG_D("otaReqMessage->Payload() is null");
    }
    
    OTAPriorityType priority {OTAPriorityType::UNKNOWN};
    if ((priorityTmp < static_cast<uint8_t>(OTAPriorityType::LOW)) ||
    (priorityTmp > static_cast<uint8_t>(OTAPriorityType::HIGH)))
    {
        LOG_E("Wrong priorityTmp: %u", priorityTmp);
        isOK = false;
        
    } else {
        priority = static_cast<OTAPriorityType>(priorityTmp);
    }

    if (isOK == true) {
        LOG_D("Handle GetObcResource request, Priority = %d", priorityTmp);
        mReqSeqNum = otaReqMessage->SequenceNumber();
        switch(mState) {
            case RemoteOTA::State::REMOTE_OTA_STATE_IDLE:
            {
                LOG_D("REMOTE_OTA_STATE_IDLE");
                handleGetObcIdleState(priority);
                break;
            }
            case RemoteOTA::State::REMOTE_OTA_STATE_GET_OBC_RESOURCE_COMPLETE:
            {
                LOG_D("REMOTE_OTA_STATE_GET_OBC_RESOURCE_COMPLETE");
                handleGetObcResourceComplete();
                break;
            }
            case RemoteOTA::State::REMOTE_OTA_STATE_REQUEST_PRIOTIRY:
            {
                LOG_D("REMOTE_OTA_STATE_REQUEST_PRIOTIRY");
                ocbResourceEventNotify(mReqSeqNum, OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
                break;
            }
            default:
            {
                ocbResourceEventNotify(mReqSeqNum, OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
                break;
            }
        }
    }
    (void)priorityTmp;
    (void)priority;
}

void RemoteOTA::handleGetObcIdleState(const OTAPriorityType priority)
{
    const OBCResourceEventCode code {OnboardclientAdapter::getInstance()->GetObcResource()};
    if (code != OBCResourceEventCode::OBC_GET_RESOURCE_OK)
    {
        mState = State::REMOTE_OTA_STATE_WAITING_OBC_RESOURCE;
        ocbResourceEventNotify(mReqSeqNum, OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
    } else {
        mState = State::REMOTE_OTA_STATE_REQUEST_PRIOTIRY;
        LOG_I("Get OBC Resource OK, state change to REMOTE_OTA_STATE_REQUEST_PRIOTIRY");
    }
    requestPriorityControll(priority);
}

void RemoteOTA::handleGetObcResourceComplete(void)
{
    OnboardclientAdapter::getInstance()->TakeObcResource();
    mState = State::REMOTE_OTA_STATE_ENABLE;
    ocbResourceEventNotify(mReqSeqNum, OBCResourceEventCode::OBC_GET_RESOURCE_OK);
}

void RemoteOTA::handleReleaseObcResourceReq(const android::sp<OtaMessage>& otaReqMessage)
{
    LOG_W("handleReleaseObcResourceReq");
    mReqSeqNum = otaReqMessage->SequenceNumber();
    releaseObcResource();
    ocbResourceEventNotify(mReqSeqNum, OBCResourceEventCode::OBC_RELEASE_RESOURCE_OK);
    stopOtaEnableStateTimeout();
}

void RemoteOTA::requestPriorityControll(const OTAPriorityType priority)
{
    mOtaPriority = priority;
    const uint32_t prio {(priority == OTAPriorityType::HIGH) ? DiagTrigger::PRIO_OTA_HIGH : DiagTrigger::PRIO_OTA_LOW};
    /* TBD: check ppi flag*/
    /* Get trigger ID*/
    const uint32_t nextTriggerId {TriggerIDGenerator::getInstance().getNextId()};
    /* Create NewDiag Trigger*/
    const android::sp<DiagTrigger> pTrigger{new DiagTrigger(DiagTrigger::DiagTriggerType::OTA_TRIGGER, prio, DiagTrigger::DiagTriggerFunc::OTA, nextTriggerId)};
    /* Save request to local*/
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret{mTriggerList.emplace(nextTriggerId, pTrigger)};
    if (!ret.second)
    {
        ret.first->second = pTrigger;
    }
    /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
    PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
    LOG_D("requestPriorityControll priority = %d, mTriggerList size: %d,  trigger ID: %d", priority, mTriggerList.size(), ret.first->first);
    mCurrentTriggerId = nextTriggerId;
}


void RemoteOTA::releaseObcResource()
{
    LOG_D("Released OTA enable state, currentConnectId: %lu", mCurrentConnectId);

    if (mCurrentConnectId != 0U)
    {
        (void)OnboardclientAdapter::getInstance()->disconnectECU(mCurrentConnectId);
    }

    if (mState == State::REMOTE_OTA_STATE_ENABLE) {
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mTriggerList.find(mCurrentTriggerId)};
        if (it != mTriggerList.end())
        {
            if (mCurrentTriggerId <= static_cast<uint32_t>(INT32_MAX)) {
                PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mCurrentTriggerId), DiagTrigger::DiagTriggerType::OTA_TRIGGER);
            } else {
                LOG_E("mCurrentTriggerId value out of range");
            }
            
            (void)mTriggerList.erase(it);
        }
        else
        {
            LOG_D("Can not find triggerId: %d in mTriggerList", mCurrentTriggerId);
        }
    } 
    else if (mState == State::REMOTE_OTA_STATE_IDLE)
    {
        // Do nothing
    }
    else {
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mTriggerList.find(mCurrentTriggerId)};
        if (it != mTriggerList.end())
        {
            (void)mTriggerList.erase(it);
            if (mCurrentTriggerId <= static_cast<uint32_t>(INT32_MAX)) {
                PriorityControl::getInstance()->notifyTriggerNoFound(static_cast<int32_t>(mCurrentTriggerId));
            } else {
                LOG_E("mCurrentTriggerId value out of range");
            }
        }
        else
        {
            LOG_D("Can not find triggerId: %d in mTriggerList", mCurrentTriggerId);
        }
    }
    mCurrentTriggerId = 0U;
    mCurrentConnectId = 0U;
    mState = State::REMOTE_OTA_STATE_IDLE;
    mOtaPriority = OTAPriorityType::UNKNOWN;
}

void RemoteOTA::handleSendUdsDataReq(const android::sp<OtaMessage>& otaReqMessage)
{
    stopOtaEnableStateTimeout();
    mReqSeqNum = otaReqMessage->SequenceNumber();
    const android::sp<OBCResponseEventInfo> responseEventInfo {new OBCResponseEventInfo()};
    bool isOK {true};
    const uint32_t payloadSize {otaReqMessage->PayloadSize()};
    if(payloadSize == 0U)
    {
        LOG_E("Invalid Playload");
        isOK = false;
        responseEventInfo->errCode() = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_INVALID_PARAMETERS);
        responseEventNotify(responseEventInfo);
    }

    if (isOK == true) 
    {
        const uint16_t connectId {CommonUtils::makeSerializeUint16(otaReqMessage->Payload(), OtaMessageDefs::CONNECT_ID_BYTE_MASK)};
        LOG_D("handleSendUdsDataReq: connectId = %u", connectId);
        if ((mState == State::REMOTE_OTA_STATE_ENABLE) && (connectId == mCurrentConnectId) ) {
            const uint16_t udsDataLeght {CommonUtils::makeSerializeUint16(otaReqMessage->Payload(), OtaMessageDefs::UDS_REQUEST_BYTE_MASK)};

            const android::sp<::Buffer> udsData {new ::Buffer()};
            
            const uint8_t ObcProtocol {static_cast<uint8_t>(RemoteEcuInformation::getInstance()->getObcProtocolType(mCurrentResInfo.canInfo.canId))};
            responseEventInfo->resInfo()->canInfo()->canId() = RemoteEcuInformation::getInstance()->getCanId(mCurrentResInfo.canInfo.canId); // get canId from targetAddress
            responseEventInfo->resInfo()->canInfo()->nTa().clear();
            if ((ObcProtocol == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
                (ObcProtocol == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
                (ObcProtocol == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
            {
                const uint16_t nTa{static_cast<uint16_t>(((responseEventInfo->resInfo()->canInfo()->canId() >> 8U) & 0xFFU))};
                std::stringstream ss{};
                ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << nTa;
                const std::string hexString{ss.str()}; // Convert to string
                for (size_t i {0U}; i < hexString.size(); i++)
                {
                    const std::string tmp{std::string(1U, hexString[i])};
                    responseEventInfo->resInfo()->canInfo()->nTa().push_back(tmp);
                }
            }
            responseEventInfo->resInfo()->connectId() = connectId;           
            responseEventInfo->resInfo()->responseType() = static_cast<uint8_t>(OBCEnum::OBCUdsResponseType::UNKOWN);
            
            if (udsDataLeght > 0U)
            {
                if(otaReqMessage->Payload()->data() != nullptr) {
                    udsData->setTo(&otaReqMessage->Payload()->data()[OtaMessageDefs::UDS_REQUEST_DATA_BYTE_MASK], static_cast<int32_t>(udsDataLeght));
                } else {
                    LOG_E("otaReqMessage->Payload()->data() is null");
                }
                // TODO: Send UDS data to OBC
                const uint8_t error {OnboardclientAdapter::getInstance()->sendUdsData(connectId, udsData)};
                if (error != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK)) {
                    if (error <= static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_MAX))
                    {
                        responseEventInfo->errCode() = error;
                    } else {
                        responseEventInfo->errCode() = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_MAX);
                    }
                    responseEventNotify(responseEventInfo);
                }
                else {
                    updateUdsResponseTimeout(static_cast<uint32_t>(mCurrentUdsResTimeout));
                }
                (void)error;
            } else {
                responseEventInfo->errCode() = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_INVALID_PARAMETERS);
                responseEventNotify(responseEventInfo);
            }
        } else {
            responseEventInfo->errCode() = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_NOT_CONNECTED);
            responseEventInfo->resInfo()->connectId() = connectId;
            responseEventNotify(responseEventInfo);
        }
    }
}

void RemoteOTA::handleSendUdsDataReqTimeout(const android::sp<::Buffer> reqData)
{
    LOG_W("Handle send UDS data request timeout");
    if (reqData->empty())
    {
        LOG_E("OTA request data is empty");
    } 
    else 
    {
        const uint32_t rawDataSize {reqData->size()};
        if (rawDataSize < OtaMessageDefs::FA_PROTOCOL_HEADER_LENGHT)
        {
            LOG_E("OTA request data not enough (size < 8 bytes)");
        } else {
            uint8_t faProtoVer {0U};
            (void)std::memcpy(&faProtoVer, reqData->data() + OtaMessageDefs::FA_PROTO_VERSION_BYTE_MASK, 1U);
            if (faProtoVer > 1U)
            {
                LOG_E("FA Protocol Version: %d is not supported", faProtoVer);
            } else {
                uint8_t mid {0U};
                (void)std::memcpy(&mid, reqData->data() + OtaMessageDefs::MID_BYTE_MASK, 1U);

                if (mid != static_cast<uint8_t>(OTA_MID::OTA_MID_5_SEND_UDS_DATA_REQ))
                {
                    LOG_E("OTA Message ID = %d is not supported", mid);
                } else {

                    mReqSeqNum = CommonUtils::makeSerializeUint16(reqData, OtaMessageDefs::SEQUENCE_NUMBER_BYTE_MASK);
                }
            }
        }
    }

    ocbResourceEventNotify(OBCResourceEventCode::OBC_ACCESS_TIMEOUT);
}

void RemoteOTA::handleUdsResponseTimeout(void)
{
    if (mState == State::REMOTE_OTA_STATE_ENABLE) {
        LOG_D("handle Uds Response Timeout");
        const android::sp<OBCResponseEventInfo> responseEventInfo {new OBCResponseEventInfo()};
        responseEventInfo->errCode() = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_TIMEOUT);
        responseEventInfo->resInfo()->connectId() = mCurrentConnectId;
        responseEventInfo->resInfo()->responseType() = static_cast<uint8_t>(OBCEnum::OBCUdsResponseType::UNKOWN);

        const uint8_t ObcProtocol {static_cast<uint8_t>(RemoteEcuInformation::getInstance()->getObcProtocolType(mCurrentResInfo.canInfo.canId))};
        responseEventInfo->resInfo()->canInfo()->canId() = RemoteEcuInformation::getInstance()->getCanId(mCurrentResInfo.canInfo.canId); // get RX canId from targetAddress
        responseEventInfo->resInfo()->canInfo()->nTa().clear();
        if ((ObcProtocol == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
            (ObcProtocol == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
            (ObcProtocol == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
        {
            const uint16_t nTa{static_cast<uint16_t>(((responseEventInfo->resInfo()->canInfo()->canId() >> 8U) & 0xFFU))};
            std::stringstream ss{};
            ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << nTa;
            const std::string hexString{ss.str()}; // Convert to string
            for (size_t i {0U}; i < hexString.size(); i++)
            {
                const std::string tmp{std::string(1U, hexString[i])};
                responseEventInfo->resInfo()->canInfo()->nTa().push_back(tmp);
            }
        }
        responseEventNotify(responseEventInfo); 
    }
}

void RemoteOTA::connectResultNotify(const uint16_t sequenceNumber, const android::sp<OBCConnectInfo> info)
{
    LOG_W("connectResultNotify");
    uint8_t resCode {static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_FAILED)};
    if(info->getResponse() < OBCEnum::OBCErrCode::OBC_ERR_MAX) {
        resCode = static_cast<uint8_t>(info->getResponse());
    }
    const android::sp<OtaMessage> otaResMessage{new OtaMessage(OTA_MID::OTA_MID_7_CONNECT_RES)};
    otaResMessage->Payload()->append(&resCode, 1);
    SerializeUint16_t connectId {info->getConnectId()};
    otaResMessage->Payload()->append(&connectId.data[1], 1); // Byte High
    otaResMessage->Payload()->append(&connectId.data[0], 1); // Byte Low
    otaResMessage->SetSequenceNumber(sequenceNumber);
    sendOtaRes(otaResMessage->ToRaw());
}

void RemoteOTA::disconnectResultNotify(const uint16_t sequenceNumber, const OBCEnum::OBCErrCode& code)
{
    LOG_W("disconnectResultNotify");
    const android::sp<OtaMessage> otaResMessage{new OtaMessage(OTA_MID::OTA_MID_8_DISCONNECT_RES)};
    uint8_t data {0U};
    data = static_cast<uint8_t>(code);
    otaResMessage->Payload()->append(&data, 1);
    otaResMessage->SetSequenceNumber(sequenceNumber);
    sendOtaRes(otaResMessage->ToRaw());
}

void RemoteOTA::ocbResourceEventNotify(const OBCResourceEventCode& event)
{
    LOG_W("ocbResourceEventNotify, sequenceNumber = %u, OBCResourceEventCode = %u", mReqSeqNum, static_cast<uint8_t>(event));
    const android::sp<OtaMessage> otaResMessage {new OtaMessage(OTA_MID::OTA_MID_6_GET_OBC_RESOURCE_RES)};
    
    uint8_t data {0U};
    data = static_cast<uint8_t>(event);
    otaResMessage->Payload()->append(&data, 1);

    otaResMessage->SetSequenceNumber(mReqSeqNum);
    sendOtaRes(otaResMessage->ToRaw());
    if ((mState == State::REMOTE_OTA_STATE_ENABLE) || (mState == State::REMOTE_OTA_STATE_GET_OBC_RESOURCE_COMPLETE))
    {
        updateOtaEnableStateTimeout();
    }
}

void RemoteOTA::ocbResourceEventNotify(const uint16_t sequenceNumber, const OBCResourceEventCode& event)
{
    LOG_W("ocbResourceEventNotify, sequenceNumber = %u, OBCResourceEventCode = %u", sequenceNumber, static_cast<uint8_t>(event));
    const android::sp<OtaMessage> otaResMessage{new OtaMessage(OTA_MID::OTA_MID_6_GET_OBC_RESOURCE_RES)};
    uint8_t data {0U};
    data = static_cast<uint8_t>(event);
    otaResMessage->Payload()->append(&data, 1);
    otaResMessage->SetSequenceNumber(sequenceNumber);
    sendOtaRes(otaResMessage->ToRaw());
    if ((mState == State::REMOTE_OTA_STATE_ENABLE) || (mState == State::REMOTE_OTA_STATE_GET_OBC_RESOURCE_COMPLETE))
    {
        updateOtaEnableStateTimeout();
    }
}

void RemoteOTA::responseEventNotify(const android::sp<OBCResponseEventInfo> responseEventInfo)
{
    LOG_D("responseEventNotify");
    updateOtaEnableStateTimeout();
    const android::sp<OBCCanInfo> canInfo {responseEventInfo->resInfo()->canInfo()};
    const uint16_t connectId {responseEventInfo->resInfo()->connectId()};

    uint8_t protocolType {0U};
    const int32_t protocolTypeTmp {RemoteEcuInformation::getInstance()->getCommType(canInfo->canId())};
    if ((protocolTypeTmp >= 0) && (protocolTypeTmp <= static_cast<int32_t>(UINT8_MAX)))
    {
        protocolType = static_cast<uint8_t>(protocolTypeTmp);
    } else {
        LOG_E("protocolTypeTmp out of range");
    }

    uint16_t nTaLenght {0U};
    const uint32_t nTaLenghtTmp {static_cast<uint32_t>(responseEventInfo->resInfo()->canInfo()->nTa().size()) + 1U};
    if (nTaLenghtTmp <= static_cast<uint32_t>(UINT16_MAX))
    {
        nTaLenght = static_cast<uint16_t>(nTaLenghtTmp);
    } else {
        LOG_E("nTaLenghtTmp out of range");
    }


    uint16_t udsDataLenght {0U};
    const uint32_t udsDataLenghtTmp {responseEventInfo->resInfo()->udsData()->size()};
    if (udsDataLenghtTmp <= static_cast<uint32_t>(UINT16_MAX))
    {
        udsDataLenght = static_cast<uint16_t>(udsDataLenghtTmp);
    } else {
        LOG_E("responseEventInfo.resInfo.udsData.size() out of range");
    }

    std::vector<uint8_t> otaPayload{};
    const android::sp<OtaMessage> otaResMessage{new OtaMessage(OTA_MID::OTA_MID_9_RESPONSE_EVENT)};
    // Adding Error code
    otaPayload.push_back(static_cast<uint8_t>(responseEventInfo->errCode()));
    // Adding Protocol type
    otaPayload.push_back(protocolType);
    // Adding CanId
    const SerializeUint32_t serializedCanId {canInfo->canId()};
    // Adding CanId
    otaPayload.push_back(serializedCanId.data[3]);
    otaPayload.push_back(serializedCanId.data[2]);
    otaPayload.push_back(serializedCanId.data[1]);
    otaPayload.push_back(serializedCanId.data[0]);
    // Adding nTA Lenght
    const SerializeUint16_t serializedNtaLenght {nTaLenght};
    otaPayload.push_back(serializedNtaLenght.data[1]);
    otaPayload.push_back(serializedNtaLenght.data[0]);
    // Adding nTA Data
    if ((canInfo->nTa().size() >= 2U) 
        && (responseEventInfo->resInfo()->canInfo()->nTa()[0].size() >= 1U) 
        && (responseEventInfo->resInfo()->canInfo()->nTa()[1].size() >= 1U))
    {
        otaPayload.push_back(static_cast<uint8_t>(responseEventInfo->resInfo()->canInfo()->nTa()[0][0]));
        otaPayload.push_back(static_cast<uint8_t>(responseEventInfo->resInfo()->canInfo()->nTa()[1][0]));
        otaPayload.push_back(0x00U);
    } else {
        otaPayload.push_back(0x00U);
    }

    // Adding connectId
    const SerializeUint16_t serializedConnectId{connectId};
    otaPayload.push_back(serializedConnectId.data[1]);
    otaPayload.push_back(serializedConnectId.data[0]);
    // Adding responseType
    otaPayload.push_back(static_cast<uint8_t>(responseEventInfo->resInfo()->responseType()));
    // Adding udsDataLenght
    const SerializeUint16_t serializedConnectUdsDataLenght{udsDataLenght};
    otaPayload.push_back(serializedConnectUdsDataLenght.data[1]);
    otaPayload.push_back(serializedConnectUdsDataLenght.data[0]);
    // Adding UDS data
    (void)otaPayload.insert(otaPayload.cend(), responseEventInfo->resInfo()->udsData()->data(), responseEventInfo->resInfo()->udsData()->data() + udsDataLenght);

    const uint32_t payloadSize {static_cast<uint32_t>(otaPayload.size())};

    if (payloadSize <= static_cast<uint32_t>(INT32_MAX))
    {
        otaResMessage->Payload()->setTo(otaPayload.data(), static_cast<int32_t>(payloadSize));
    } else {
        LOG_E("payloadSize out of range with int32_t, payloadSize = %d", payloadSize);
    }

    otaResMessage->SetSequenceNumber(mReqSeqNum);
    sendOtaRes(otaResMessage->ToRaw());
    (void)udsDataLenghtTmp;
}

bool RemoteOTA::notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff) 
{
    (void)dueToIgOff;
    bool isOtaTriggered{false};
    if (pTriggerId >= 0) 
    {
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mTriggerList.find(static_cast<uint32_t>(pTriggerId))};
        if (it != mTriggerList.end())
        {
            isOtaTriggered = true;
            switch(pState) 
            {
                case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
                {
                    if ((mState == State::REMOTE_OTA_STATE_ENABLE) && (mOtaPriority == OTAPriorityType::LOW))
                    {
                        (void)mHandler->obtainMessage(MainHandler::CMD_OTA_DISCARD)->sendToTarget();
                    } 
                    else if ((mState == State::REMOTE_OTA_STATE_ENABLE) && (mOtaPriority == OTAPriorityType::HIGH)) 
                    {
                        LOG_D("Can't discard OTA process, because OTA Priority is HIGH");
                    } else {
                        // Do nothing
                    }
                    break;
                }
                case DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING:
                {
                    (void)mHandler->obtainMessage(MainHandler::CMD_OTA_TRIGGER_PROCESSING, pTriggerId)->sendToTarget();
                    break;
                }
                case DiagTrigger::DiagTriggerState::TRIGGER_PENDING:
                {
                    LOG_D("DiagTrigger::DiagTriggerState::TRIGGER_PENDING");
                    break;
                }
                default:
                    break;
            }
        }
        else
        {
            LOG_E("Can not find trigger id");
            PriorityControl::getInstance()->notifyTriggerNoFound(pTriggerId);
        }
    } else {
        LOG_E("Invalid triggerId");
    }
    return isOtaTriggered;
}

void RemoteOTA::handleTriggerProcessing(const int32_t triggerId)
{
    LOG_D("TRIGGER_PROCESSING");
    if (triggerId >= 0) {
        mCurrentTriggerId = static_cast<uint32_t>(triggerId);
    } else {
        mCurrentTriggerId = 0U;
        LOG_E("triggerId out of range");
    }
    const OBCResourceEventCode code {OnboardclientAdapter::getInstance()->GetObcResource()}; 
    if ( (code == OBCResourceEventCode::OBC_GET_RESOURCE_OK) 
            && (mState == State::REMOTE_OTA_STATE_REQUEST_PRIOTIRY) )
    {
        OnboardclientAdapter::getInstance()->TakeObcResource();
        mState = State::REMOTE_OTA_STATE_ENABLE;
        ocbResourceEventNotify(mReqSeqNum, OBCResourceEventCode::OBC_GET_RESOURCE_OK);
    } 
    else if ( (code == OBCResourceEventCode::OBC_GET_RESOURCE_OK) 
            && (mState == State::REMOTE_OTA_STATE_WAITING_OBC_RESOURCE) )
    {
        mState = State::REMOTE_OTA_STATE_GET_OBC_RESOURCE_COMPLETE;
        ocbResourceEventNotify(mReqSeqNum, OBCResourceEventCode::OBC_RELEASE_RESOURCE_COMPLETE);
    }
    else {
        mState = State::REMOTE_OTA_STATE_WAITING_OBC_RESOURCE;
        ocbResourceEventNotify(mReqSeqNum, OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
    }
}

void RemoteOTA::handleDiscardedEvent(void)
{   
    // RDG30-R-0533
    releaseObcResource();
    ocbResourceEventNotify(OBCResourceEventCode::OBC_FORCE_RESET);
    stopOtaEnableStateTimeout();
    stopUdsResponseTimeout();
}

void RemoteOTA::handleOtaEnableStateTimeout(void)
{
    releaseObcResource();
    ocbResourceEventNotify(OBCResourceEventCode::OBC_ACCESS_TIMEOUT);
}

void RemoteOTA::updateUdsResponseTimeout(const uint32_t duration) 
{
    mUdsResponseTimer.stop();
    if(duration > (UINT32_MAX - TimerHandler::UDS_RESPONSE_TIMEOUT_EXTEND)) {
        LOG_E("May wrap error");
    }
    mUdsResponseTimer.setDuration(duration + TimerHandler::UDS_RESPONSE_TIMEOUT_EXTEND, 0U);
    mUdsResponseTimer.start();
}

void RemoteOTA::onRdgStop(const bool isStop) const noexcept {
    //ontain message CMD_STOP_RDG to stop rdg
    (void)mHandler->obtainMessage(MainHandler::CMD_STOP_RDG)->sendToTarget();
    (void)isStop;
}

void RemoteOTA::onReceiveIG(const bool status) const noexcept  {
    
    if (status == false)
    {
        LOG_I("onReceiveIG OFF");
        (void)mHandler->obtainMessage(MainHandler::CMD_STOP_RDG)->sendToTarget();
    }
};


bool RemoteOTA::CheckSupportedSID(const uint8_t SID) noexcept
{
    bool result {false};
    std::vector<uint8_t>::iterator supportedSidIt {SUPPORTED_SID.begin()};
    for(; supportedSidIt != SUPPORTED_SID.end(); supportedSidIt++)
    {
        if(*supportedSidIt == SID)
        {
            result = true;
            break;
        }
    }
    return result;
}

void RemoteOTA::handleStop() noexcept
{
    LOG_I("Handle stop");
    releaseObcResource();
}
}
