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

RemoteOTA::RemoteOTA()
        : android::RefBase(), RemoteDelegate()
        , mOtaTimerHandler(*this)
        , mUdsResponseTimerHandler(*this)
        , mOtaTimer(&mOtaTimerHandler, TimerHandler::OTA_SESSION_TIMEOUT_TIMER_ID)
        , mUdsResponseTimer(&mUdsResponseTimerHandler, TimerHandler::UDS_RESPONSE_TIMEOUT_TIMER_ID)
        , mReqSeqNum(0U)
        , mCurrentTriggerId(0U)
        , mOtaEnableState(false)
        , mIsWaitingObcResource(false)
        , mIsActive(true) // Communication between FA and RemoteDiag is enable by default
        , mCurrentConnectId(0U)
        , mCurrentUdsResTimeout(0U)
        , mOtaPriority(OTAPriorityType::UNKNOWN)
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
    const int32_t whatCmd {handlemsg->what};

    switch (whatCmd) {
        case CMD_OTA_EXECULTE_REQ:
        {
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
            LOG_I("CMD_OTA_FA_CLIENT_DISCONNECTED");
            mOTA.handleFaClientDisconnectEvent();
            break;
        }
        case CMD_OTA_ENABLE_STATE_TIMEOUT:
        {
            // RDG30-R-0535
            LOG_I("CMD_OTA_ENABLE_STATE_TIMEOUT");
            mOTA.handleOtaEnableStateTimeout();
            break;
        }
        case CMD_OTA_USD_RESPONSE_TIMEOUT_EVENT:
        {
            LOG_I("CMD_OTA_USD_RESPONSE_TIMEOUT_EVENT");
            mOTA.handleUdsResponseTimeout();
            break;
        }
        case CMD_OTA_DISCARD:
        {
            LOG_I("CMD_OTA_DISCARD");
            mOTA.handleDiscardedEvent();
            break;
        }
        case CMD_OTA_WAITING_FRAGMENT_DATA:
        {
            LOG_I("CMD_OTA_WAITING_FRAGMENT_DATA");
            mOTA.updateOtaEnableStateTimeout();
            break;
        }
        case CMD_OTA_FA_CLIENT_TIMEOUT:
        {
            LOG_I("CMD_OTA_FA_CLIENT_TIMEOUT");
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
        default:
            break;
    }
}

void RemoteOTA::execulteOtaReq(const android::sp<::Buffer> reqData)
{
    if (mIsActive == true) {
        LOG_I("execulte Ota Request");
        const android::sp<OtaMessage> otaReqMessage {new OtaMessage()};
        if (otaReqMessage->Parser(reqData) != E_OK)
        {
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
        LOG_I("Request is rejected. Because, FirewallManager disabled OTA function");
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
            if (mFaServer->notify(resData) == E_ERROR)
            {
                LOG_D("Retry %d", count);
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
    mFaServer = new FaServer(ParamsDef::FA_SERVER_DEFAULT_PORT, mHandler);
    mOtaTimer.setDuration(TimerHandler::OTA_SESSION_TIMEOUT, 0U);
    mFaServer->startup();
}

void RemoteOTA::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
    if ((mOtaEnableState == true) 
        && (mCurrentConnectId == responseEventInfo->resInfo()->connectId())
        && (responseEventInfo->errCode() != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_TIMEOUT) )
        // filter by SID
        && ((udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER)) // Req: 0x22, Resp: 0x62
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_ROUTINE_CONTROL)) // Req: 0x31, Resp: 0x71
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_SECURITY_ACCESS)) // Req: 0x27, Resp: 0x67
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_REQUEST_DOWNLOAD)) // Req: 0x34, Resp: 0x74
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_TRANSFER_DATA)) // Req: 0x36, Resp: 0x76
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_REQUEST_TRANSFER_EXIT)) // Req: 0x37, Resp: 0x77
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_WRITE_DATA_BY_IDENTIFIER)) // Req: 0x2E, Resp: 0x6E
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_SESSION_CONTROL)) // Req: 0x10, Resp: 0x50
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_TESTER_PRESENT)) // Req: 0x3E, Resp: 0x7E
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_ECU_RESET)) // Req: 0x11, Resp: 0x51 DCM24MON-1790
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION)) // Req: 0x19, Resp: 0x59 DCM24MON-1790
        || (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE))))  // Negative response
    {
        LOG_D("canId = 0x%02x", responseEventInfo->resInfo()->canInfo()->canId());
        LOG_D("connectId = %d", responseEventInfo->resInfo()->connectId());
        stopUdsResponseTimeout();
        responseEventNotify(responseEventInfo);
    } 
    else if ((mOtaEnableState == true) 
            && (mCurrentConnectId == responseEventInfo->resInfo()->connectId())
            && (responseEventInfo->errCode() == static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_TIMEOUT))) 
    {
        stopUdsResponseTimeout();
        responseEventNotify(responseEventInfo);
    } else {
        LOG_E("Not support connectId = %d, sid = 0x%02x", responseEventInfo->resInfo()->connectId(), udsResponse->getSID());
    }
}

void RemoteOTA::onFirewallActionDiagDisable()
{
    mIsActive = false;
    mFaServer->active(false);
    if (mOtaEnableState == true) {
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        mCurrentConnectId = 0U;
        mOtaEnableState = false;
        mIsWaitingObcResource = false;
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
    LOG_I("Handle Connect request");
    const uint32_t payloadSize {otaReqMessage->PayloadSize()};
    if(payloadSize == 0U) 
    {
        LOG_E("Connect Request, Invalid Playload");
    } else {
        uint8_t protocolType {0U};
        (void)std::memcpy(&protocolType, otaReqMessage->Payload()->data() + OtaMessageDefs::PROTOCOL_TYPE_BYTE_MASK, 1U);
        const uint32_t targetAddress {CommonUtils::makeSerializeUint32(otaReqMessage->Payload(), OtaMessageDefs::CANID_BYTE_MASK)};
        // RDG30-R-1266
        protocolType = static_cast<uint8_t>(RemoteEcuInformation::getInstance()->getObcProtocolType(targetAddress));
        const uint16_t nTaLeght {CommonUtils::makeSerializeUint16(otaReqMessage->Payload(), OtaMessageDefs::NTA_LENGHT_BYTE_MASK)};

        std::vector<std::string> ntaArray{};
        LOG_D("nTA info, nTaLeght = %d", nTaLeght);
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
        LOG_D("OBCProtocolType: %d, udsResTimeout: %d, targetAddress: 0x%02x"
            , protocolType
            , udsResTimeout
            , targetAddress);
        updateOtaEnableStateTimeout();
        const android::sp<OBCTransportInfo> obcTransportInfo {new OBCTransportInfo()};
        OBCCanInfo canInfo{};
        canInfo.setData(targetAddress, ntaArray);
        obcTransportInfo->setData(protocolType
                                , canInfo
                                , true  // Not using on OBC spec
                                , udsResTimeout);
        const android::sp<OBCConnectInfo> obj {new OBCConnectInfo()};
        
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
                LOG_D("ECU connected, connectId = %d", mCurrentConnectId);
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
    }
}

void RemoteOTA::handleDisconnectReq(const android::sp<OtaMessage>& otaReqMessage)
{
    LOG_I("Handle Disconnect request");
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
        }
        disconnectResultNotify(otaReqMessage->SequenceNumber(), errorCode);
        if (errorCode == OBCEnum::OBCErrCode::OBC_ERR_NOT_CONNECTED) {
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
        }
        (void)tmp;
    }
}

void RemoteOTA::handleGetObcResourceReq(const android::sp<OtaMessage>& otaReqMessage)
{
    LOG_I("Handle GetObcResource request");
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
    if (priorityTmp > static_cast<uint8_t>(OTAPriorityType::HIGH))
    {
        isOK = false;
        
    } else {
        priority = static_cast<OTAPriorityType>(priorityTmp);
    }

    if (isOK == true) {
        LOG_D("Handle GetObcResource request, Priority = %d", priorityTmp);
        mReqSeqNum = otaReqMessage->SequenceNumber();
        
        if (mIsWaitingObcResource == false) {
            mIsWaitingObcResource = true;

            requestPriorityControll(priority);
        } else {
            LOG_D("mIsWaitingObcResource = true");
            updateOtaEnableStateTimeout();
            const OBCResourceEventCode resEventInfo {OnboardclientAdapter::getInstance()->GetObcResource()};
            if (resEventInfo == OBCResourceEventCode::OBC_GET_RESOURCE_OK) {
                mIsWaitingObcResource = false;
                ocbResourceEventNotify(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
            } else if (resEventInfo == OBCResourceEventCode::OBC_GET_RESOURCE_WAIT) {
                mIsWaitingObcResource = true;
                ocbResourceEventNotify(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
            } else {
                // do nothing
            }
        }
    }
    (void)priorityTmp;
    (void)priority;
}

void RemoteOTA::handleReleaseObcResourceReq(const android::sp<OtaMessage>& otaReqMessage)
{
    LOG_I("handleReleaseObcResourceReq");
    updateOtaEnableStateTimeout();
    releaseObcResource();
    ocbResourceEventNotify(otaReqMessage->SequenceNumber(), OBCResourceEventCode::OBC_RELEASE_RESOURCE_OK);
}

void RemoteOTA::requestPriorityControll(const OTAPriorityType priority)
{
    LOG_D("priority = %d", priority);
    mOtaPriority = priority;
    const uint32_t prio {(priority == OTAPriorityType::HIGH) ? DiagTrigger::PRIO_OTA_HIGH : DiagTrigger::PRIO_OTA_LOW};
    /* TBD: check ppi flag*/
    /* Get trigger ID*/
    const uint32_t nextTriggerId {TriggerIDGenerator::getInstance().getNextId()};
    /* Create NewDiag Trigger*/
    const android::sp<DiagTrigger> pTrigger{new DiagTrigger(DiagTrigger::DiagTriggerType::OTA_TRIGGER, prio, DiagTrigger::DiagTriggerFunc::OTA, nextTriggerId)};
    /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
    PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
    /* Save request to local*/
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret{mTriggerList.emplace(nextTriggerId, pTrigger)};
    LOG_D("Check mTriggerList size: %d", mTriggerList.size());
    if (!ret.second)
    {
        ret.first->second = pTrigger;
    }
    LOG_D("Check saved RoB trigger ID: %d", ret.first->first);
}


void RemoteOTA::releaseObcResource()
{
    if (mOtaEnableState == true) {
        if (mCurrentConnectId != 0U)
        {
            LOG_D("Disconnect current connectId: %d", mCurrentConnectId);
            (void)OnboardclientAdapter::getInstance()->disconnectECU(mCurrentConnectId);
        }
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        mCurrentConnectId = 0U;
        mOtaEnableState = false;
        LOG_D("Released OTA enable state");
        mIsWaitingObcResource = false;
        mOtaPriority = OTAPriorityType::UNKNOWN;
    }

    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mTriggerList.find(mCurrentTriggerId)};
    if (it != mTriggerList.end())
    {
        if (mCurrentTriggerId <= static_cast<uint32_t>(INT32_MAX)) {
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mCurrentTriggerId), DiagTrigger::DiagTriggerType::OTA_TRIGGER);
        } else {
            LOG_E("mCurrentTriggerId value out of range");
        }
        
        (void)mTriggerList.erase(it);
        mCurrentTriggerId = 0U;
    }
    else
    {
        LOG_D("Can not find triggerId: %d in mTriggerList", mCurrentTriggerId);
    }
}

void RemoteOTA::handleSendUdsDataReq(const android::sp<OtaMessage>& otaReqMessage)
{
    LOG_I("handleSendUdsDataReq");
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
        LOG_I("Send UDS Data: connectId = %d", connectId);
        if ((mOtaEnableState == true) && (connectId == mCurrentConnectId) ) {
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
                ss << &std::hex << &std::uppercase << std::setw(2) << std::setfill('0') << nTa;
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
                    LOG_E("SendUdsData error = %d", error);
                    if (error <= static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_MAX))
                    {
                        responseEventInfo->errCode() = error;
                    } else {
                        responseEventInfo->errCode() = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_MAX);
                    }
                    responseEventNotify(responseEventInfo);
                }
                else {
                    LOG_I("SendUdsData success");
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
    LOG_I("Handle send UDS data request timeout");
    /* responseEvent 
    const android::sp<OBCResponseEventInfo> responseEventInfo {new OBCResponseEventInfo()};
    responseEventInfo->errCode() = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_TIMEOUT);
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
                    
                    const uint32_t actualPayloadsize {reqData->size() - OtaMessageDefs::FA_PROTOCOL_HEADER_LENGHT};

                    mReqSeqNum = CommonUtils::makeSerializeUint16(reqData, OtaMessageDefs::SEQUENCE_NUMBER_BYTE_MASK);

                    const uint32_t actualPayloadSize {reqData->size() - OtaMessageDefs::PAYLOAD_BYTE_MASK};
                    if (actualPayloadSize <= static_cast<uint32_t>(INT32_MAX)) {
                        const android::sp<::Buffer> payload {new ::Buffer()};
                        payload->setTo((reqData->data() + OtaMessageDefs::PAYLOAD_BYTE_MASK), static_cast<int32_t>(actualPayloadSize));
                        LOG_I("Ota Request parsering success, FA protocol version %d, mid = %d, sequenceNumber = %d"
                            , faProtoVer
                            , mid
                            , mReqSeqNum);
                        
                        if (actualPayloadSize >= 2U)
                        {
                            responseEventInfo->resInfo()->connectId()  = CommonUtils::makeSerializeUint16(payload, OtaMessageDefs::CONNECT_ID_BYTE_MASK);
                        }
                    } else {
                        LOG_E("actualPayloadsize out of range INT32");
                    }
                }
            }
        }
    }
    
    if (responseEventInfo->resInfo()->connectId() == mCurrentConnectId) 
    {
        const uint8_t ObcProtocol {static_cast<uint8_t>(RemoteEcuInformation::getInstance()->getObcProtocolType(mCurrentResInfo.canInfo.canId))};
        responseEventInfo->resInfo()->canInfo()->canId() = RemoteEcuInformation::getInstance()->getCanId(mCurrentResInfo.canInfo.canId); // get canId from targetAddress
        responseEventInfo->resInfo()->canInfo()->nTa().clear();
        if ((ObcProtocol == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
            (ObcProtocol == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
            (ObcProtocol == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
        {
            const uint16_t nTa{static_cast<uint16_t>(((responseEventInfo->resInfo()->canInfo()->canId() >> 8U) & 0xFFU))};
            std::stringstream ss{};
            ss << &std::hex << &std::uppercase << std::setw(2) << std::setfill('0') << nTa;
            const std::string hexString{ss.str()}; // Convert to string
            for (size_t i {0U}; i < hexString.size(); i++)
            {
                const std::string tmp{std::string(1U, hexString[i])};
                responseEventInfo->resInfo()->canInfo()->nTa().push_back(tmp);
            }
        }          
    }
    responseEventNotify(responseEventInfo); 
    */
    if (mOtaEnableState)
    {
        updateOtaEnableStateTimeout();
    }

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
    if (mOtaEnableState) {
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
            ss << &std::hex << &std::uppercase << std::setw(2) << std::setfill('0') << nTa;
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
    LOG_I("connectResultNotify");
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
    LOG_I("disconnectResultNotify");
    const android::sp<OtaMessage> otaResMessage{new OtaMessage(OTA_MID::OTA_MID_8_DISCONNECT_RES)};
    uint8_t data {0U};
    data = static_cast<uint8_t>(code);
    otaResMessage->Payload()->append(&data, 1);
    otaResMessage->SetSequenceNumber(sequenceNumber);
    sendOtaRes(otaResMessage->ToRaw());
}

void RemoteOTA::ocbResourceEventNotify(const OBCResourceEventCode& event)
{
    LOG_I("ocbResourceEventNotify, OBCResourceEventCode = %d", static_cast<uint8_t>(event));
    const android::sp<OtaMessage> otaResMessage {new OtaMessage(OTA_MID::OTA_MID_6_GET_OBC_RESOURCE_RES)};
    
    uint8_t data {0U};
    data = static_cast<uint8_t>(event);
    otaResMessage->Payload()->append(&data, 1);

    otaResMessage->SetSequenceNumber(mReqSeqNum);
    sendOtaRes(otaResMessage->ToRaw());
}

void RemoteOTA::ocbResourceEventNotify(const uint16_t sequenceNumber, const OBCResourceEventCode& event)
{
    LOG_I("ocbResourceEventNotify, sequenceNumber =%d, OBCResourceEventCode = %d", sequenceNumber, static_cast<uint8_t>(event));
    const android::sp<OtaMessage> otaResMessage{new OtaMessage(OTA_MID::OTA_MID_6_GET_OBC_RESOURCE_RES)};
    uint8_t data {0U};
    data = static_cast<uint8_t>(event);
    otaResMessage->Payload()->append(&data, 1);
    otaResMessage->SetSequenceNumber(sequenceNumber);
    sendOtaRes(otaResMessage->ToRaw());
}

void RemoteOTA::responseEventNotify(const android::sp<OBCResponseEventInfo> responseEventInfo)
{
    LOG_I("responseEventNotify");
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
    const uint32_t nTaLenghtTmp {responseEventInfo->resInfo()->canInfo()->nTa().size() + 1U};
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

    vector<uint8_t> otaPayload{};
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

    const uint32_t payloadSize {otaPayload.size()};

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

    if (pTriggerId >= 0) 
    {
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mTriggerList.find(static_cast<uint32_t>(pTriggerId))};
        if (it != mTriggerList.end())
        {
            switch(pState) 
            {
                case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
                {
                    if ((mOtaEnableState == true) && (mOtaPriority == OTAPriorityType::LOW))
                    {
                        (void)mHandler->obtainMessage(MainHandler::CMD_OTA_DISCARD)->sendToTarget();
                    } 
                    else if ((mOtaEnableState == true) && (mOtaPriority == OTAPriorityType::HIGH)) 
                    {
                        LOG_D("Can't discard OTA process, because OTA Priority is HIGH");
                    } else {
                        // Do nothing
                    }
                    break;
                }
                case DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING:
                {
                    LOG_D("DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING");
                    if (pTriggerId >= 0) {
                        mCurrentTriggerId = static_cast<uint32_t>(pTriggerId);
                    } else {
                        mCurrentTriggerId = 0U;
                        LOG_E("pTriggerId out of range");
                    }
                    updateOtaEnableStateTimeout();
                    const OBCResourceEventCode resEventInfo {OnboardclientAdapter::getInstance()->GetObcResource()}; 
                    if (resEventInfo == OBCResourceEventCode::OBC_GET_RESOURCE_OK) {
                        mIsWaitingObcResource = false;
                        OnboardclientAdapter::getInstance()->TakeObcResource();
                        mOtaEnableState = true;
                    } else if (resEventInfo == OBCResourceEventCode::OBC_GET_RESOURCE_WAIT) {
                        mIsWaitingObcResource = true;
                        mOtaEnableState = false;
                    } 
                    else {
                        // Do nothing
                    }
                
                    ocbResourceEventNotify(mReqSeqNum, resEventInfo);
                    break;
                }
                case DiagTrigger::DiagTriggerState::TRIGGER_PENDING:
                {
                    LOG_D("DiagTrigger::DiagTriggerState::TRIGGER_PENDING");
                    ocbResourceEventNotify(mReqSeqNum, OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
                    mIsWaitingObcResource = true;
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
    return true;
}

void RemoteOTA::handleDiscardedEvent(void)
{   
    // RDG30-R-0533
    releaseObcResource();
    ocbResourceEventNotify(OBCResourceEventCode::OBC_FORCE_RESET);
    stopOtaEnableStateTimeout();
    stopUdsResponseTimeout();
}

void RemoteOTA::handleFaClientDisconnectEvent(void)
{
  if (mOtaEnableState == true) 
  {
    mFaServer->closeCurrentClient();
  }
}

void RemoteOTA::handleOtaEnableStateTimeout(void)
{
    if (mOtaEnableState == true) 
    {
        releaseObcResource();
        ocbResourceEventNotify(OBCResourceEventCode::OBC_FORCE_RESET);
    }
    mFaServer->closeCurrentClient();
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
}
