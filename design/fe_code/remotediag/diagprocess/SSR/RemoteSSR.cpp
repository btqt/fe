#include "RemoteSSR.h"

#include "utils/Logger.h"
#include "utils/CollectionCondition.h"
#include <services/OnboardclientManagerAdapter.h>
#include <services/TimeManagerService/TimeManager.h>
#include <unordered_map>

namespace rdgapp {

RemoteSSR* RemoteSSR::mSSR_instance {nullptr};

RemoteSSR::~RemoteSSR() = default;

RemoteSSR::RemoteSSR(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper):
        android::RefBase(), RemoteDelegate()
        , mApp(app)
        , mHandler(new MainHandler(privateLooper, *this))
        , mCurrentTransmissionId(0U)
        , mIsSsrRunning(false)
        , mIsSuspending(false)
        , mDiagnosticsAcquisitionTime(0U)
        , mIsNeedUploadErrorData(false)
        , mIsNeedtoUploadSSRData(false)
        , mCurrentDiagTriggerType(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)
        , mWarningTriggerOccurrenceTime(0U)
        , mCurrentTriggerTimestamp(0)
        , mColId{0U}
        , mPriority{0U}
        , mSaveReq{}
        , mTriggerId(0U)
{
    mSSR_instance = this;
    SSR_UPLOAD_DATA_SIZE_MAX = 2097152U;
    (void)mHandler->obtainMessage(MainHandler::CMD_INIT_SSR)->sendToTarget();
}

RemoteSSR* RemoteSSR::getInstance(void)
{
    if (mSSR_instance == nullptr)
    {
        LOG_E("getInstance error, mRemoteSSR is null");
    }
    return mSSR_instance;
}

void RemoteSSR::init() 
{
    LOG_I("Init success");
    mCRCManager = new CRCManager(*this);
    (void)mHandler->obtainMessage(MainHandler::CMD_READ_SAVED_CRC)->sendToTarget();
}

void RemoteSSR::onReceiveIG(const bool status) const {
    (void)mHandler->obtainMessage(MainHandler::CMD_CHANGE_IG_STATUS,
        static_cast<int32_t>(status))->sendToTarget();
}

void RemoteSSR::onChangedRemoteInfo(const int32_t what, const int32_t info)
{
    LOG_I("onChangedRemoteInfo");
    (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE, what, info)->sendToTarget();
}

void RemoteSSR::changeIGStatus(const bool status) 
{
    LOG_I("onReceiveIG status = %s", status ? "IGN_ON" : "IGN_OFF");
    LOG_I("mCurrentDiagTriggerType = %d", mCurrentDiagTriggerType);
    LOG_I("mTriggerId = %d", mTriggerId);
    if ((status == false) && (mIsSsrRunning == true) )
    {
        //RDG30-R-0247, RDG30-R-0689
        const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans{mSsrTransList.find(mCurrentTransmissionId)};
        if (itTrans != mSsrTransList.end())
        {
            if (mSsrTransList[mCurrentTransmissionId]->getState() == SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST)
            {
                LOG_E("IG change to OFF, store undefined value to reference data");
                mResSID22[mSsrTransList[mCurrentTransmissionId]->getEcuInformation().getTargetAddress()].setEcuMakerCode(0xFFFFU);
                mResSID22[mSsrTransList[mCurrentTransmissionId]->getEcuInformation().getTargetAddress()].setSwPartNumber("");
                (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
            }
            else
            {
                if ( (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) || (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) )
                {
                    LOG_I("IG changed to OFF. Finish SSR acquisition.");
                    mIsSsrRunning = false;
                    itTrans->second->disconnect();
                    mTransmissioIdList = {};
                    mDiagResp_CRC.clear();
                    mDiagResp.clear();
                    mTransDTCNo.clear();
                    mDTCNoResp.clear();
                    onFinishSSRAcquisition(mTriggerId);
                    mSsrTransList.clear();
                    // Release OBC resource
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                }
                else
                {
                    mSSRUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
                    abortSSRAcquisition(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
                }
            }
        }
    }
    else
    {
        //Do nothing   
    }
}

void RemoteSSR::onSSRFlagChangeOFF() {
    LOG_D("Clear CRC");
    mCRCManager->requestRemoveCRCFile();
}

void RemoteSSR::handleUnderRepairStatusChange(const int32_t& what)
{
    LOG_D("handleUnderRepairStatusChange");
    switch (what)
    {
    case WHAT_CHANGED_REPAIR_SATUS:
    {
        if ( mIsSsrRunning == true )
        {
            //RDG30-R-0247
            const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans{mSsrTransList.find(mCurrentTransmissionId)};
            if (itTrans != mSsrTransList.end())
            {
                if (mSsrTransList[mCurrentTransmissionId]->getState() == SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST)
                {
                    LOG_E("IG change to OFF, store undefined value to reference data");
                    mResSID22[mSsrTransList[mCurrentTransmissionId]->getEcuInformation().getTargetAddress()].setEcuMakerCode(0xFFFFU);
                    mResSID22[mSsrTransList[mCurrentTransmissionId]->getEcuInformation().getTargetAddress()].setSwPartNumber("");
                    (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                }
                else
                {
                    abortSSRAcquisition(RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR);
                }
            }
        }
        else {
            //Do nothing
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

uint8_t RemoteSSR::getAppId(void) const noexcept 
{
    return RDG_APPID::SSR;
}

void RemoteSSR::startUp(const DiagTrigger::DiagTriggerType type, const uint64_t timestamp, const android::sp<CommonDefine::RDGLocationData> location)
{
    LOG_I("SSR_Startup");

    const uint8_t SSR_flag{DiagManagerAdapter::getInstance()->getSSRFlag()};
    if (SSR_flag == 0x00U) 
    {
        //RDG30-R-0260
        LOG_D("Clear CRC");
        mCRCManager->requestRemoveCRCFile();
    }

    if (checkPrecondition() == true)
    {
        mSSRUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
        // Get OBC resource
        const OBCResourceEventCode resEventInfo {OnboardclientAdapter::getInstance()->GetObcResource()};
        if ( resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK )
        {
            LOG_E("GetObcResource OBC_GET_RESOURCE_WAIT -> wait and check after");

            const android::sp<sl::Message> handlemsg {mHandler->obtainMessage(MainHandler::CMD_START_SSR, location)};
            uint8_t timerPtr[sizeof(timestamp)];
            (void)memcpy(&timerPtr[0], &timestamp, sizeof(timestamp));
            handlemsg->arg1 = static_cast<int32_t>(type);
            handlemsg->buffer.setTo(&timerPtr[0], static_cast<int32_t>(sizeof(timestamp)));
            (void)mHandler->sendMessageDelayed(handlemsg, 5000U);

        } else {
            LOG_D("Get obc resource success");
            // Lock OBC resource
            OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
            this->mIsSsrRunning = true;

            mSsrTransList.clear();
            mTransmissioIdList = {};
            LOG_V("Size of mSsrTargetECUList: %d", mSsrTargetECUList.size());
            for (std::vector<uint32_t>::iterator it {mSsrTargetECUList.begin()}; it != mSsrTargetECUList.end(); it++)
            {
                std::list<CommonDefine::EcuInformation> ecuInfoList{};
                RemoteEcuInformation::getInstance()->getEcuInformationList(ecuInfoList);
                std::list<CommonDefine::EcuInformation>::iterator ecu_it{};
                for (ecu_it = ecuInfoList.begin(); ecu_it != ecuInfoList.end(); ++ecu_it)
                {
                    if ((*it == ecu_it->getTargetAddress()) && (ecu_it->getecuActiveFlag() == true))
                    {
                        break;
                    }
                }

                if (ecu_it == ecuInfoList.end())
                {
                    LOG_E("ECU is not active in the list");
                } 
                else
                {
                    if ((ecu_it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6) || (ecu_it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5))
                    {
                        const android::sp<SsrUdsTransmission> transmission {new SsrUdsTransmission( *this, *ecu_it)};
                        transmission->transInit();
                        mSsrTransList[transmission->getTransmissionId()] = transmission;
                        mTransmissioIdList.push(transmission->getTransmissionId());
                    }
                    else
                    {
                        LOG_E("ECU diag phase is invalid");
                    }
                }
            }
            
            LOG_D("mTransmissioIdList size = %d", mTransmissioIdList.size());
            if ( mTransmissioIdList.size() > 0U )
            {
                mCurrentTransmissionId = mTransmissioIdList.front();
                const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans {mSsrTransList.find(mCurrentTransmissionId)};
                if (itTrans != mSsrTransList.end())
                {
                    itTrans->second->connect();
                }
            } else {
                // Release OBC resource
                DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
                onFinishSSRAcquisition(mTriggerId);
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
            }
        }
    }
    else
    {
        LOG_E("conditions not match");
        if (mTriggerId < static_cast<uint32_t>(INT32_MAX))
        {
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mCurrentDiagTriggerType);
        }
        else
        {
            LOG_D("triggerId value error: %d in mTriggerList", mTriggerId);
        }
    }
}

bool RemoteSSR::checkPrecondition()
{
    //get RDG active flag
    const uint8_t SSR_flag {DiagManagerAdapter::getInstance()->getSSRFlag()};
    //get IG status
    const IG_STATUS IG_status {PowerManagerAdapter::getInstance()->getIgnitionStatus()};
    //get data upload consent status
    const bool consent_status {DiagManagerAdapter::getInstance()->getAllUploadConsent()};
    //get Under Repair Status
    const uint8_t under_repair_status{mApp.getUnderRepair()};
    LOG_D("Precondition: SSR_flag = %d, IG_status = %d, consent_status = %d, under_repair_status = %d", SSR_flag, IG_status, consent_status, under_repair_status);
    const bool ret{(SSR_flag == 0x01U) && (IG_status == IG_STATUS_ON) && (consent_status == true)&&(under_repair_status != 0x01U)};
    (void) IG_status;
    (void) consent_status;
    (void) under_repair_status;
    return ret;
}

void RemoteSSR::notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff)
{
    /* Check if trigger is in the saved trigger*/
    const uint32_t tmpTriggerId {(pTriggerId >= 0) ? static_cast<uint32_t>(pTriggerId) : 0U};
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(tmpTriggerId)};
    if(it != mSaveReq.end()) {
        LOG_I("notify Trigger state: %d TriggerID: %d TriggerTime: %lld", pState, pTriggerId, it->second->getTriggerTime());
        handleTrigger(pState, pTriggerId, dueToIgOff);
    } else {
        LOG_I("Can not find triggerID in saved list");
        PriorityControl::getInstance()->notifyTriggerNoFound(pTriggerId);
    }
}

void RemoteSSR::handleTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff) 
{
    const uint32_t tmpTriggerId {(pTriggerId >= 0) ? static_cast<uint32_t>(pTriggerId) : 0U};
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(tmpTriggerId)};
    const android::sp<DiagTrigger> trigger {nullptr};
    if(it != mSaveReq.end()) {
        const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
        LOG_I("notify Trigger state: %d, TriggerID: %d, TriggerType: %d", pState, pTriggerId, pTriggerType);
        switch (pState) {
        case DiagTrigger::DiagTriggerState::TRIGGER_STATE_MIN:
        {
            LOG_I("TRIGGER_STATE_MIN");
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_PENDING:
        {
            LOG_I("TRIGGER_PENDING");
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING:
        {
            if (mIsSuspending == true)
            {
                mIsSsrRunning = true;
                mIsSuspending = false;

                LOG_D("mTransmissioIdList size = %d", mTransmissioIdList.size());
                if ( mTransmissioIdList.size() > 0U )
                {
                    mCurrentTransmissionId = mTransmissioIdList.front();
                    const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans {mSsrTransList.find(mCurrentTransmissionId)};
                    if (itTrans != mSsrTransList.end())
                    {
                        itTrans->second->connect();
                    }
                } else {
                    // Release OBC resource
                    DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
                    onFinishSSRAcquisition(mTriggerId);
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                }
            }
            else
            {
                LOG_I("TRIGGER_PROCESSING");
                // const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
                const android::sp<CommonDefine::RDGLocationData> location {LocationManagerAdapter::getInstance()->getLocationData()};
                mTriggerId = (pTriggerId >= 0) ? static_cast<uint32_t>(pTriggerId) : 0U;

                const android::sp<sl::Message> handlemsg {mHandler->obtainMessage(MainHandler::CMD_START_SSR, location)};
                handlemsg->arg1 = static_cast<int32_t>(pTriggerType);
                (void)handlemsg->sendToTarget();
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED:
        {
            LOG_I("TRIGGER_SUSPENDED");
            if (mIsSsrRunning == true)
            {
                mIsSsrRunning = false;
                mIsSuspending = true;
            }
            PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
        {
            LOG_I("TRIGGER_DISCARDED - DueToIGOff = %d", dueToIgOff);
            const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans{mSsrTransList.find(mCurrentTransmissionId)};
            if (itTrans != mSsrTransList.end())
            {
                if (mSsrTransList[mCurrentTransmissionId]->getState() == SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST)
                {
                    LOG_E("IG change to OFF, store undefined value to reference data");
                    mResSID22[mSsrTransList[mCurrentTransmissionId]->getEcuInformation().getTargetAddress()].setEcuMakerCode(0xFFFFU);
                    mResSID22[mSsrTransList[mCurrentTransmissionId]->getEcuInformation().getTargetAddress()].setSwPartNumber("");
                    (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                }
                else {
                    //Abort acquisition
                    mIsSsrRunning = false;
                    while(mTransmissioIdList.empty() != true) {
                        mTransmissioIdList.pop();
                    }
                    mSsrTransList.clear();

                    // Release OBC resource
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();

                    mSSRUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
                    if ( mSSRUploadErrorData != nullptr ) 
                    {
                        //RDG30-R-0691
                        if ( (pTriggerType != DiagTrigger::DiagTriggerType::IGON_TRIGGER) 
                        && (pTriggerType != DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
                        && (dueToIgOff == false))
                        {
                            mSSRUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED);
                            LOG_I("The SSR acquisition sequence is aborted -> push error upload data package");
                            mIsNeedUploadErrorData = true;
                            makeErrorUploadDataRequest(*mSSRUploadErrorData, pTriggerType, mColId);
                        }
                        else if (pTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER)
                        {
                            LOG_D("IGON_TRIGGER -> error upload data shall not be created");
                        }
                        else if(pTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
                        {
                            LOG_D("WARNING_TRIGGER -> error upload data shall not be created");
                        } else {
                            // do nothing
                        }
                    }
                    else
                    {
                        LOG_E("mSSRUploadErrorData is null");
                    }
                    PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
                }
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DONE:
        {
            LOG_I("TRIGGER_DONE");
            PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_STATE_MAX:
        {
            LOG_I("TRIGGER_STATE_MAX");
            break;
        }
        }
    }
    else {
        //Do nothing
    }
}

void RemoteSSR::MainHandler::handleMessage (const android::sp<sl::Message>& handlemsg) {
    const int32_t what {handlemsg->what};
    LOG_I({"handler is processing with what: %d"}, what);
    switch (what) {
        case CMD_INIT_SSR:
        {
            LOG_I("CMD_INIT_SSR");
            mSSR.init();
            break;
        }
        case CMD_START_SSR:
        {
            LOG_I("CMD_START_SSR");
            android::sp<CommonDefine::RDGLocationData> location {nullptr};
            uint64_t timestamp {0U};
            (void)timestamp;
            if (handlemsg->buffer.size() > 0U ) {
                if (handlemsg->buffer.data() != nullptr)
                {
                    (void)memcpy(&timestamp, handlemsg->buffer.data(), sizeof(timestamp));
                } else {
                    LOG_E("buffer.data() is empty");
                }
            } else {
                LOG_E("timestamp data is empty");
            }
            handlemsg->getObject(location);
            LOG_I("Get location data success, 0x%08X 0x%08X", location->getLatitude(), location->getLongtitude());
            if (location == nullptr)
            {
                LOG_E("location data is null");
                break;
            }
            LOG_I("Trigger Type: %d", handlemsg->arg1);
            if ((handlemsg->arg1 > static_cast<int32_t>(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)) && (handlemsg->arg1 < static_cast<int32_t>(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)))
            {
                LOG_I("Enter StartUp");
                mSSR.startUp(static_cast<DiagTrigger::DiagTriggerType>(handlemsg->arg1), timestamp, location);
            }
            break;
        }
        case CMD_CHANGE_IG_STATUS:
        {
            LOG_I("CMD_CHANGE_IG_STATUS");
            mSSR.changeIGStatus((handlemsg->arg1 != 0) ? true : false);
            break;
        }
        case CMD_READ_SAVED_CRC:
        {
            mSSR.readCRC();
            break;
        }
        case CMD_REQUEST_TO_PRIORITY_CONTROL:
        {
            LOG_I("CMD_REQUEST_TO_PRIORITY_CONTROL");
            sp<DiagTrigger> pTrigger {nullptr};
            handlemsg->getObject(pTrigger);
            LOG_I("Trigger Request have Type: %d Func: %d Prio: %d ID: %d", 
                pTrigger->getType(), pTrigger->getFunc(), pTrigger->getPriority(), pTrigger->getTriggerId());
            PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
            break;
        }
        case CMD_MAKE_UPLOAD_REQUEST:
        {
            LOG_I("CMD_MAKE_UPLOAD_REQUEST");
            mSSR.makeUploadSSRRequest();
            break;
        }
        case CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE:
        {
            mSSR.handleUnderRepairStatusChange(handlemsg->arg1);
            break;
        }
        case CMD_SSR_FINISH_TRANSMISSION:
        {
            LOG_I("CMD_SSR_FINISH_TRANSMISSION");
            mSSR.finishCurrentTransmission();
            break;
        }
        default:
        {
            break;
        }
    }
}

void RemoteSSR::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
    const android::sp<OBCCanInfo> canInfo {responseEventInfo->resInfo()->canInfo()};
    // uint16_t connectId = responseEventInfo->resInfo->connectId;
    // uint8_t protocolType = responseEventInfo->resInfo->protocolType;

    if (this->mIsSsrRunning == true) {
        LOG_D("onReceiveUDS canId = 0x%02x", canInfo->canId());
        LOG_D("onReceiveUDS connectId = %d", responseEventInfo->resInfo()->connectId());
        uint32_t centerRxAdd{0U};
        const uint32_t tmpCenterRxAdd{responseEventInfo->getResInfo()->getCanInfo()->getCanId()};
        if (tmpCenterRxAdd <= static_cast<uint32_t>(UINT32_MAX))
        {
            centerRxAdd = tmpCenterRxAdd;
        }
        else
        {
            centerRxAdd = static_cast<uint32_t>(UINT32_MAX);
        }
        if ((mTransmissioIdList.empty() != true) && (mSsrTransList[mCurrentTransmissionId]->getConnectId() == responseEventInfo->resInfo()->connectId()))
        {
            mSsrTransList[mCurrentTransmissionId]->handleUdsResponse(udsResponse);
            mSsrTransList[mCurrentTransmissionId]->setRxAdd(centerRxAdd);
        }
        (void)centerRxAdd;
    }
    return;
}

RemoteSSR::SsrUdsTransmission::SsrUdsTransmission( RemoteSSR& ssr
                                                    , const CommonDefine::EcuInformation& ecuInformation)
: android::RefBase()
, ecuInfo(ecuInformation)
, connectId(0U)
, mSSR(ssr)
, transmissionId(0U)
, centerRxAdd(0U)
, mTimerHandler(ssr)
, m_state(SsrUdsTransmission::State::SSR_TRANS_INIT)
, mTimeOut(&mTimerHandler, TimerHandler::ID_TRANSMISSION_TIMEOUT)

{
    mTimeOut.setDuration(TRANSMISSION_TIME_OUT_DURATION, 0U);
    transmissionId = (static_cast<uint64_t>(ecuInfo.getTargetAddress()) << 32U);
}

void RemoteSSR::SsrUdsTransmission::transInit()
{
    //add SID 19 SubFunction 03 request
    this->setState(SsrUdsTransmission::State::SSR_TRANS_SESSIONCONTROL_REQUEST);
    const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
    pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION));
    pUdsReq->setSFID(static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_03_REPORT_DTC_SNAPSHOT_IDENTIFICATION));
    udsReqList.push(pUdsReq);
}

void RemoteSSR::SsrUdsTransmission::connect()
{
    LOG_I("Start connect, transmissionID = 0x%02llx", this->transmissionId);
    // this->state = RobSsrUdsTransmission::State::ROBSSR_TRANS_CONNECT;
    const android::sp<OBCTransportInfo> mtransportInfo{new OBCTransportInfo()};
    const android::sp<OBCConnectInfo> mConnectInfo{new OBCConnectInfo()};
    const uint8_t mprotocolType{RemoteEcuInformation::getInstance()->convertObcProtocolType(this->ecuInfo.getCommProtocol(), this->ecuInfo.getCommType(), this->ecuInfo.getTargetAddress())};
    // const android::sp<OBCTransportInfo> obcTransportInfo = new OBCTransportInfo();
    OBCCanInfo mcanInfo{};
    std::vector<std::string> ntaArray{};
    ntaArray.clear();
    if ((mprotocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
        (mprotocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
        (mprotocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
    {
        const uint16_t nTa{static_cast<uint16_t>(((this->ecuInfo.getTargetAddress() >> 8U) & 0xFFU))};
        std::stringstream ss{};
        ss << &std::hex << &std::uppercase << std::setw(2) << std::setfill('0') << nTa;
        const std::string hexString{ss.str()}; // Convert to string
        for (size_t i{0U}; i < hexString.size(); i++)
        {
            const std::string tmp{std::string(1U, hexString[i])};
            ntaArray.push_back(tmp);
        }
    }
    mcanInfo.setData(this->ecuInfo.getTargetAddress(), ntaArray);
    constexpr bool mperiodicRes{false};
    constexpr uint16_t mudsResTimeout{0U};
    mtransportInfo->setData(mprotocolType, mcanInfo, mperiodicRes, mudsResTimeout);
    (void)OnboardclientAdapter::getInstance()->connect(mtransportInfo, APP_NAME, mConnectInfo);
    
    if (mConnectInfo->getResponse() != OBCEnum::OBCErrCode::OBC_OK)
    {
        LOG_E("can't connect to canid = %d", this->ecuInfo.getTargetAddress());
        (void)mSSR.mHandler->obtainMessage(MainHandler::CMD_SSR_FINISH_TRANSMISSION)->sendToTarget();

    } else {
        LOG_D("connect success transmissionID = 0x%02llx", this->transmissionId);
        this->connectId = mConnectInfo->getConnectId();
        
        OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
        this->sendNextUdsRequest();
    }
}

void RemoteSSR::SsrUdsTransmission::stopTimeout()
{
    this->mTimeOut.stop();
}

void RemoteSSR::SsrUdsTransmission::disconnect()
{
    LOG_D("disconnect, transmissionId = 0x%02llx", this->transmissionId);
    (void)OnboardclientAdapter::getInstance()->disconnectECU(this->connectId);
    OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
}

void RemoteSSR::SsrUdsTransmission::sendNextUdsRequest()
{
    if (udsReqList.empty() != true)
    {
        const android::sp<::Buffer> udsData {this->udsReqList.front()->ToUdsData()};

        const uint8_t err {OnboardclientAdapter::getInstance()->sendUdsData(this->connectId, udsData)};
        if (err != 0x00U) //E_OK 
        {
            LOG_E("Send SID 0x%02x for transmissionId 0x%02llx error = %d -> Disconnect", this->udsReqList.front()->getSID(), this->transmissionId, err);
            this->udsReqList.pop();
            LOG_V("Size of udsReqList = %d", udsReqList.size());
            if (udsReqList.empty() == true)
            {
                this->disconnect();
            } else
            {
                this->sendNextUdsRequest();
            }
        }
        else {
            LOG_D("Send SID 0x%02x for transmissionId 0x%02llx success", this->udsReqList.front()->getSID(), this->transmissionId);
            mTimeOut.start();
        }
    }
    else
    {
        this->disconnect();
        mSSR.finishCurrentTransmission();
        mSSR.mDiagNegativeResp.clear();
    }
}

void RemoteSSR::SsrUdsTransmission::handleUdsResponse(android::sp<UdsMessage> udsResponse)
{
    const uint8_t sid {udsResponse->getSID()};
    const uint8_t sfid {udsResponse->getSFID()};

    LOG_I("sid: 0x%02x, sfid: 0x%02x", sid, sfid);

    switch (sid)
    {
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION):
        {
            if (sfid == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_03_REPORT_DTC_SNAPSHOT_IDENTIFICATION))
            {
                this->stopTimeout();
                //handle response of SID$19 SFID$03
                const android::sp<::Buffer> udsData {udsResponse->GetUdsPayload()};
                uint32_t numSSRNo{0U};
                const uint32_t udsDataSize{udsData->size()};
                std::vector<uint32_t> ssrNoList{};
                numSSRNo = udsDataSize/4U;
                for (uint32_t idx {0U}; idx < numSSRNo; idx++) 
                {
                    if (udsData->data() != nullptr)
                    {
                        const uint32_t ssrNoData {static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[idx*4U]) << 24U) | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[idx*4U+1U]) << 16U)
                                            | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[idx*4U+2U]) << 8U) | static_cast<uint32_t>(udsData->data()[idx*4U+3U])};
                        ssrNoList.push_back(ssrNoData);
                    }
                    else {
                        LOG_E("udsData->data() is empty");
                    }
                }
                sort(ssrNoList.begin(), ssrNoList.end(), greater<uint32_t>());

                //create UDS request message for SID$19 SFID$042
                for (size_t idx {0U}; idx < ssrNoList.size(); idx++) 
                {
                    const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                    pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION));
                    pUdsReq->setSFID(static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_04_REPORT_DTC_SNAPSHOT_RECORD_BY_DTC_NUMBER));
                    uint8_t ssrNo_Data[sizeof(ssrNoList[idx])];
                    (void)memcpy(&ssrNo_Data[0], &ssrNoList[idx], sizeof(ssrNo_Data));
                    pUdsReq->GetUdsPayload()->setTo(&ssrNo_Data[3], 1);
                    pUdsReq->GetUdsPayload()->append(&ssrNo_Data[2], 1);
                    pUdsReq->GetUdsPayload()->append(&ssrNo_Data[1], 1);
                    pUdsReq->GetUdsPayload()->append(&ssrNo_Data[0], 1);
                    udsReqList.push(pUdsReq);
                }
                //remove the request in front and send next request
                this->udsReqList.pop();
                this->sendNextUdsRequest();
            }
            else if (sfid == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_04_REPORT_DTC_SNAPSHOT_RECORD_BY_DTC_NUMBER))
            {
                this->stopTimeout();
                //handle response of SID$19 SFID$04
                const android::sp<::Buffer> udsData {udsResponse->GetUdsPayload()};
                uint32_t DTCNumber{0U};
                if (udsData->data() != nullptr)
                {
                    DTCNumber = static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[0U]) << 16U) | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[1U]) << 8U)
                                | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[2U]));
                    LOG_D("DTC Numbber Data: 0x%02X", DTCNumber);
                } else {
                    LOG_E("udsData->data() is null");
                }
                if (udsData->size() > 0U) 
                {
                    mSSR.mDiagResp.push_back({transmissionId, udsResponse});
                    mSSR.mDiagResp_CRC[transmissionId] = udsResponse;
                    mSSR.mTransDTCNo[transmissionId].push_back(DTCNumber);
                    mSSR.mDTCNoResp[DTCNumber] = udsResponse;
                    //get reference data
                    if ((udsReqList.size() == 1U) && (mSSR.mResSID22.find(this->ecuInfo.getTargetAddress()) == mSSR.mResSID22.end()))
                    {
                        // send SID22 DID$F18A
                        this->setState(SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST);
                        const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                        pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
                        pUdsReq->setSFID(0xF1U); //start routine
                        constexpr uint8_t tmp_did {0x8AU};
                        pUdsReq->GetUdsPayload()->setTo(&tmp_did, sizeof(uint8_t));
                        udsReqList.push(pUdsReq);

                        //send SID22 DID$F188
                        const android::sp<UdsMessage> pUdsReq1 {new UdsMessage()};
                        pUdsReq1->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
                        pUdsReq1->setSFID(0xF1U); //start routine
                        constexpr uint8_t tmp_did1 {0x88U};
                        pUdsReq1->GetUdsPayload()->setTo(&tmp_did1, sizeof(uint8_t));
                        udsReqList.push(pUdsReq1);
                    }
                }
                (void)DTCNumber;
                this->udsReqList.pop();
                this->sendNextUdsRequest();
            }
            else
            {
                LOG_E("Wrong sfid");
            }
            break;
        }

        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER):
        {
            this->stopTimeout();
            //TODO: handle SID 22 response
            // const RefData refData;
            const android::sp<::Buffer> udsData {udsResponse->ToUdsData()};
            if (udsData->data() != nullptr)
            {
                const uint16_t resDid {static_cast<uint16_t>(static_cast<uint16_t>(udsData->data()[1]) << 8U) | static_cast<uint16_t>(udsData->data()[2])};
                if (resDid == 0xF18AU)
                {
                    mSSR.mResSID22[this->ecuInfo.getTargetAddress()].setEcuMakerCode(static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[3]) << 8U) | static_cast<uint32_t>(udsData->data()[4]));

                }
                else if (resDid == 0xF188U)
                {
                    const uint32_t udsDataSize{udsData->size()};
                    if (udsDataSize > 3U)
                    {
                        std::string stSwPtNum{};
                        stSwPtNum.resize(udsDataSize - 3U);
                        if (stSwPtNum.size() > 0U)
                        {
                            (void)memcpy(&stSwPtNum[0], &udsData->data()[3], stSwPtNum.size());
                        }
                        mSSR.mResSID22[this->ecuInfo.getTargetAddress()].setSwPartNumber(stSwPtNum);
                    }
                }
                else
                {
                    LOG_E("Wong DID");
                }
            }
            else {
                LOG_E("udsData->data() is nullptr");
            }
            this->udsReqList.pop();
            this->sendNextUdsRequest();
            break;
        }
        
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE):
        {
            mSSR.mDiagResp_CRC[transmissionId] = udsResponse;
            const android::sp<UdsMessage> currentUdsReq {this->udsReqList.front()};
            if (currentUdsReq->getSFID() == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_03_REPORT_DTC_SNAPSHOT_IDENTIFICATION))
            {
                this->stopTimeout();
                //discard response and shift to next ECU
                this->disconnect();
                mSSR.finishCurrentTransmission();
            }
            else if (currentUdsReq->getSFID() == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_04_REPORT_DTC_SNAPSHOT_RECORD_BY_DTC_NUMBER))
            {
                this->stopTimeout();
                //store negative response message and shift to next SSRNo
                LOG_I("Negative response, store and execute next SSRNo");
                mSSR.mDiagNegativeResp.push_back({transmissionId, udsResponse});
                this->udsReqList.pop();
                LOG_V("Size of udsReqList = %d", udsReqList.size());
                this->sendNextUdsRequest();
            }
            else
            {
                LOG_E("Wrong sfid");
            }
            break;
        }

        default:
        {
            LOG_E("unsupported SID 0x%02x", sid);
            break;
        }
    }
}

bool RemoteSSR::calculateCRC() 
{
    bool isDifferent{false};
    if(mCRCManager != nullptr) {
        /*TBD: caculate CRC*/
        mCRCManager->calculateCRC16Data();
        isDifferent = mCRCManager->compareCrc16Value();
    } else {
        isDifferent = true;
    }
    return isDifferent;
}

void RemoteSSR::readCRC() {
    /*TBD: read dtc flag*/
    if(mCRCManager == nullptr) {
        mCRCManager = new CRCManager(*this);
    }
    mCRCManager->readCRC16FromFile();
} 

void RemoteSSR::onTransmissionTimeout(void)
{
    LOG_I("Event Transmission Timeout, currentTrans = 0x%02llx", mCurrentTransmissionId);
    const TransmissionInter it {mSsrTransList.find(mCurrentTransmissionId)};
    if (it != mSsrTransList.end())
    {
        it->second->disconnect();
    }
    mSsrTransList[mCurrentTransmissionId]->handleTimeout();
    mSsrTransList[mCurrentTransmissionId]->sendNextUdsRequest();
}

void RemoteSSR::startNextTransmission(void)
{
    if (this->mIsSsrRunning == true)
    {
        mCurrentTransmissionId = mTransmissioIdList.front();
        LOG_I("mCurrentTransmissionId = 0x%02llx", mCurrentTransmissionId);
        const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator it {mSsrTransList.find(mCurrentTransmissionId)};
        if (it != mSsrTransList.end())
        {
            it->second->connect();
        }
    }
}

void RemoteSSR::abortSSRAcquisition(const RdgProtoInterface::ResponseCode code)
{
    LOG_D("abortSSRAcquisition, responsecode = %d", code);
    // Abort aquisition if SSR Data
    mIsSsrRunning = false;
    mTransmissioIdList = {};
    mDiagResp_CRC.clear();
    mSsrTransList.clear();
    mDiagResp.clear();
    mTransDTCNo.clear();
    mDTCNoResp.clear();
    // Release OBC resource
    OnboardclientAdapter::getInstance()->ReleaseObcResource();

    makeErrorUploadDataRequest(*mSSRUploadErrorData, mCurrentDiagTriggerType, mColId);
}

void RemoteSSR::makeErrorUploadDataRequest(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colID)
{
    LOG_I("Make SSR Error data");
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);

    vccomif::rdg::v1::interfaces::RdgCommonRequestHeader *const mRdgCommonRequestHeader{errorData.mutable_rdg_common_request_header()};
    vccomif::common::v1::AppCommonHeaderVehicleToCenter *const mAppCommonHeaderVehicleToCenter{mRdgCommonRequestHeader->mutable_app_common_header()};

    mAppCommonHeaderVehicleToCenter->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    mAppCommonHeaderVehicleToCenter->set_electronic_pf(EPF_19EPF);
    mAppCommonHeaderVehicleToCenter->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);

    mRdgCommonRequestHeader->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
    // mess id
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    mRdgCommonRequestHeader->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, counterValue));

    const sp<CommonDefine::RDGLocationData> mLocation{LocationManagerAdapter::getInstance()->getLocationData()};
    errorData.set_collection_condition_id(colID); /*TBD*/
    errorData.set_counter_value(counterValue);
    (void)counterValue;
    vccomif::rdg::v1::interfaces::TriggerType errorTriggerType{vccomif::rdg::v1::interfaces::TriggerType::TT_UNKNOWN};
    if ((type == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) || (type == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER))
    {
        errorTriggerType = vccomif::rdg::v1::interfaces::TriggerType::TT_OTHER_TRIGGER;
    }
    else if (type == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
    {
        errorTriggerType = vccomif::rdg::v1::interfaces::TriggerType::TT_OCCURRENCE_NOTIFICATION_TRIGGER;
    }
    else
    {
        errorTriggerType = vccomif::rdg::v1::interfaces::TriggerType::TT_UNKNOWN;
    }
    errorData.set_trigger_type(errorTriggerType);
    const bool bOBDFlag{mApp.getOBDStatus()};
    errorData.set_obd2_installed_flag(bOBDFlag);
    errorData.set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
    errorData.set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ALL_DIAG);
    std::string str_err_robssr{""};
    google::protobuf::util::JsonOptions option{};
    option.always_print_primitive_fields = true;
    option.preserve_proto_field_names = true;
    (void)google::protobuf::util::MessageToJsonString(errorData, &str_err_robssr, option);
    printData(str_err_robssr);

    const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
    std::string file_dir{std::to_string(uploadId)};
    (void)file_dir.append("_UploadErrorSSRDataRequest.dat");
    (void)DataModel<UploadErrorDataRequest>::save(file_dir, errorData);
    const android::sp<UploadTask> task{new UploadTask(uploadId)};

    task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
    task->setUploadPatch(file_dir);
    task->setUploadId(static_cast<uint64_t>(uploadId));
    /*Set priority*/
    task->setUploadPrio(mPriority);
    const uint64_t fileSize{static_cast<uint64_t>(errorData.ByteSizeLong())};
    task->setFileSize(fileSize);
    UploadManager::getInstance()->requestUploadTask(task);
    mSSRUploadErrorData->Clear();
}

void RemoteSSR::triggerFromDTC(const DiagTrigger::DiagTriggerType triggerType
    , const int64_t timeData
    , const android::sp<CommonDefine::RDGLocationData> location
    , const uint64_t collectionId
    , const uint32_t priority
    , const std::vector<uint32_t> v_targetEcu)
{
    LOG_I("triggerFromDTC");
    bool ssrFlag{false};
    ssrFlag = DiagManagerAdapter::getInstance()->getSSRFlag()>0U;
    if (ssrFlag == false)
    {
        LOG_I("SSRFlag is OFF. Trigger to RoB.");
        mApp.triggerSSRToRoB(triggerType, timeData, location, collectionId, priority);
    }
    else
    {
        mCurrentDiagTriggerType = triggerType;
        mCurrentTriggerLocation = location;
        mSsrTargetECUList = v_targetEcu;
        mPriority = priority;
        mColId = collectionId;
        if (triggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
        {
            if (timeData >= 0)
            {
                mWarningTriggerOccurrenceTime = static_cast<uint64_t>(timeData);
            }
        } else {
            if (timeData >= 0)
            {
                mDiagnosticsAcquisitionTime = static_cast<uint64_t>(timeData);
            }
        }
        for ( uint32_t i{0U}; i < mSsrTargetECUList.size(); i++ )
        {
            LOG_V("Target ECU %d: 0x%02lx", i, mSsrTargetECUList[i]);
        }
        LOG_D("Check SSR location information: 0x%08x 0x%08x", mCurrentTriggerLocation->getLatitude(), mCurrentTriggerLocation->getLongtitude());
        /* Get trigger ID*/
        const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
        /* Create NewDiag Trigger*/
        /* DiagTrigger(const DiagTrigger::DiagTriggerType type, const uint32_t priority, const DiagTrigger::DiagTriggerFunc func, const int32_t triggerId)*/
        const android::sp<DiagTrigger> pTrigger{new DiagTrigger(triggerType, priority, DiagTrigger::DiagTriggerFunc::SSR, nextTriggerId)};
        /* set trigger time*/
        pTrigger->setTriggerTime(timeData);
        LOG_D("Check SSR trigger time: %lld sec", timeData);
        /* Set collection id*/
        pTrigger->setCollectionId(collectionId);
        LOG_D("Check Collection ID: %llu ", collectionId);
        /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
        (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
        /* Save request to local*/
        const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
        LOG_D("Check mSaveReq size: %d", mSaveReq.size());
        if(!ret.second) {
            ret.first->second = pTrigger;
        }
        LOG_D("Check saved SSR trigger ID: %d", ret.first->first);
    }
}

void RemoteSSR::triggerToNextService(const bool hasHistoryFile) {
    LOG_I("trigger next service with triggerId: %d, hasHistoryFile: %d", mTriggerId, hasHistoryFile);
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(mTriggerId)};
    if(it != mSaveReq.end()) {
        LOG_I("Trigger SSR to RoB");
        mApp.triggerSSRToRoB(it->second->getType(),it->second->getTriggerTime(), mCurrentTriggerLocation, mColId, it->second->getPriority());
    } else {
        LOG_I("Can not find triggerID in saved list");
    }
}

void RemoteSSR::finishCurrentTransmission(void)
{
    if (this->mIsSsrRunning == true) {
        LOG_I("finish Current transmission ID: 0x%02llx, mTransmissioIdList size = %d", mCurrentTransmissionId, mTransmissioIdList.size());
        if ( mTransmissioIdList.size() > 1U ) 
        {
            mTransmissioIdList.pop();
            mCurrentTransmissionId = mTransmissioIdList.front();
            const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator it {mSsrTransList.find(mCurrentTransmissionId)};
            if (it != mSsrTransList.end())
            {
                mSsrTransList[mCurrentTransmissionId]->setState(SsrUdsTransmission::State::SSR_TRANS_DONE);
                it->second->connect();
            }
        } else  {
            if(mTransmissioIdList.empty() != true) {
                mTransmissioIdList.pop();
            }
            // Release OBC resource
            OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
            (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
            mIsSsrRunning = false;
        }
    }
}

void RemoteSSR::makeUploadSSRRequest()
{
    UploadSsrDataRequest ssrDataReq{};
    mIsNeedUploadErrorData = false;
    mIsNeedtoUploadSSRData = false;
    uint32_t sizeOfFileCounter {0U};
    ssrDataReq.set_collection_condition_id(mColId);
    ssrDataReq.mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_SSR_DATA);
    ssrDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    ssrDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    ssrDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(AppCommonHeaderVehicleToCenterGeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
    const int32_t timezoneOffSet_hour {TimeManager::getInstance().getOffset() / 60};
    const int32_t timezoneOffSet_minute {TimeManager::getInstance().getOffset() % 60};
    LOG_D("Get timezone offset: %d:%d ", timezoneOffSet_hour, timezoneOffSet_minute);
    ssrDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(timezoneOffSet_hour);
    ssrDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(timezoneOffSet_minute);

    LOG_D("location data, 0x%08x 0x%08x", mCurrentTriggerLocation->getLatitude(), mCurrentTriggerLocation->getLongtitude());
    /*obd2_installed_flag*/
    ssrDataReq.set_obd2_installed_flag(mApp.getOBDStatus());
    /*oneof_odo_information*/
    uint32_t odo_value;
    uint32_t odo_unit;
    (void)VehicleManagerAdapter::getInstance()->getOdoInformation(odo_value, odo_unit); // odo value
    if (odo_unit == 0x2U)
    {
        ssrDataReq.set_odo_information_mile(odo_value); // odo_unit = 10b ~ Mile
    }
    else if (odo_unit == 0x1U)
    {
        ssrDataReq.set_odo_information_km(odo_value); // odo_unit = 01b ~ km
    }
    else
    {
        ssrDataReq.set_odo_information_km(0xFFFFFFFF); // RDG30-R-1050 undefined value
    }
    /*under_repair_flag*/
    ssrDataReq.set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true:false);

    if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
        ssrDataReq.set_trigger_type(RdgProtoInterface::TriggerType::TT_WARNING_TRIGGER);
    } else if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
        ssrDataReq.set_trigger_type(RdgProtoInterface::TriggerType::TT_IG_ON_TRIGGER);
    } else if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER) {
        ssrDataReq.set_trigger_type(RdgProtoInterface::TriggerType::TT_OCCURRENCE_NOTIFICATION_TRIGGER);
    } else if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) {
        ssrDataReq.set_trigger_type(RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER);
    } else {
        ssrDataReq.set_trigger_type(RdgProtoInterface::TriggerType::TT_UNKNOWN);
    }

    if (mCurrentDiagTriggerType != DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        ssrDataReq.set_diagnostics_acquisition_time(mDiagnosticsAcquisitionTime);
    } else {
        ssrDataReq.set_warning_trigger_occurrence_time(mWarningTriggerOccurrenceTime);
    }

    ssrDataReq.mutable_location()->set_latitude(mCurrentTriggerLocation->getLatitude());
    ssrDataReq.mutable_location()->set_longitude(mCurrentTriggerLocation->getLongtitude());

    sizeOfFileCounter = ssrDataReq.ByteSizeLong();
    std::unordered_map<uint64_t, std::vector<uint32_t>>::iterator item{};
    for (item = mTransDTCNo.begin(); item != mTransDTCNo.end(); item++) {
        const uint64_t transId{item->first};
        const TransmissionInter transIt {mSsrTransList.find(transId)};
        if (transIt != mSsrTransList.end()) {
            std::vector<uint32_t> listDTCNo {item->second};
            for(const uint32_t dtc : listDTCNo) 
            {
                ssrDataReq.set_dtc(dtc);
                RdgProtoInterface::DiagnosticsMessage tmpDiagMessage{};
                LOG_I("Generating data for transmissionId = 0x%02llx - DTCNo = 0x%02X", item->first, dtc);
                const android::sp<UdsMessage> udsData {mDTCNoResp[dtc]};
                if (udsData == nullptr) {
                    // RDG30-R-1168
                    // RDG30-R-1170
                    // RDG30-R-0251
                    RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                    tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_UNRESPONSIVE);
                    const android::sp<::Buffer> reqData {transIt->second->udsReqList.front()->ToUdsData()};
                    if ((reqData != nullptr) && (reqData->data() != nullptr))
                    {
                        tmpDiagMessage.set_user_data(reqData->data(), reqData->size());
                    } else {
                        LOG_D("reqData or reqData->data() is null");
                    }
                    ecuAddressInfo->set_communication_protocol(transIt->second->getEcuInformation().getCommProtocol());
                    ecuAddressInfo->set_communication_type(transIt->second->getEcuInformation().getCommType());
                    ecuAddressInfo->set_target_address(transIt->second->getEcuInformation().getTargetAddress());
                }
                else if (udsData->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION)) 
                {
                    RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                    tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL);
                    ecuAddressInfo->set_communication_protocol(transIt->second->getEcuInformation().getCommProtocol());
                    ecuAddressInfo->set_communication_type(transIt->second->getEcuInformation().getCommType());
                    ecuAddressInfo->set_target_address(transIt->second->getRxAdd());
                    if (udsData->ToUdsData()->data() != nullptr)
                    {
                        tmpDiagMessage.set_user_data(udsData->ToUdsData()->data(), udsData->ToUdsData()->size());
                        const android::sp<::Buffer> ssrUserData {udsData->ToUdsData()};
                        for (uint32_t i{UDS_DTC_ROB_USER_DATA_MASK + 3U} ; i < ssrUserData->size(); i += 4U)
                        {
                            if (ssrUserData->data() != nullptr)
                            {
                                ssrUserData->data()[i] &=  0x0DU; // masking StatusOfDTC  with 0xF2 (pendingDTC,confirmedDTC and testFailed bits have to be 0)
                            } else {
                                LOG_E("ssrUserData->data() is null");
                            }
                        }
                        for(uint32_t i{0U}; i < udsData->ToUdsData()->size(); i++)
                        {
                            if(ssrUserData->data() != nullptr)
                            {
                                LOG_D("Check UDS_PR_READ_DTC_INFORMATION, byte %d = 0x%02llx", i, ssrUserData->data()[i]);
                            } else {
                                LOG_E("ssrUserData->data() is null");
                            }
                        }
                    } else {
                        LOG_E("ToUdsData()->data() is nullptr");
                    }
                } 
                else if (udsData->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE)) 
                {
                    RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                    tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL_WITH_NEGATIVE);
                    ecuAddressInfo->set_communication_protocol(transIt->second->getEcuInformation().getCommProtocol());
                    ecuAddressInfo->set_communication_type(transIt->second->getEcuInformation().getCommType());
                    ecuAddressInfo->set_target_address(transIt->second->getRxAdd());
                    if (udsData->ToUdsData()->data() != nullptr)
                    {
                        tmpDiagMessage.set_user_data(udsData->ToUdsData()->data(), udsData->ToUdsData()->size());
                        for(uint32_t i{0U}; i < udsData->ToUdsData()->size(); i++)
                        {
                            LOG_D("Check UDS_NEGATIVE_RESPONSE, byte %d = 0x%02llx", i, udsData->ToUdsData()->data()[i]);
                        }
                        mSSRUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_OTHER_ERROR);
                        mSSRUploadErrorData->add_diag_messages()->CopyFrom(tmpDiagMessage);

                        mIsNeedUploadErrorData = true;
                    }  else {
                        LOG_E("item.second->ToUdsData()->data() is nullptr");
                    }
                } else {
                    // Do nothing
                }
                if ((mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER)
                    || (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER))
                {
                    if(this->calculateCRC() == true) {
                        mIsNeedtoUploadSSRData = true;
                        /* generte upload data */
                        LOG_D("CRC Have change in response list");
                        const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
                        ssrDataReq.set_counter_value(counterValue);
                        ssrDataReq.mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_SSR_DATA, counterValue));
                        /* TBD: Call to Uploader to make upload data*/
                    } else {
                        LOG_D("CRC NO change in response list");
                        mIsNeedtoUploadSSRData = false;
                    }
                } else {
                    mIsNeedtoUploadSSRData = true;
                }
                // RDG30-R-0265
                uint32_t tmpSum {0U};
                const uint32_t tmpDiagMessageSize {tmpDiagMessage.ByteSizeLong()};
                if (sizeOfFileCounter <= static_cast<uint32_t>(UINT32_MAX) - tmpDiagMessageSize)
                {
                    tmpSum = sizeOfFileCounter + tmpDiagMessage.ByteSizeLong();
                }
                else
                {
                    LOG_E("total size overflow");
                }
                if (tmpSum > SSR_UPLOAD_DATA_SIZE_MAX) 
                {
                    LOG_D("SSR upload data exceeds 4MB -> discard exceeds data");
                    if (tmpDiagMessage.ByteSizeLong() > 0U)
                    {
                        // RDG30-R-1171
                        tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL_WITH_EXCEEDED_SIZE);
                        uint32_t sizeOfStatusCode {0U};
                        sizeOfStatusCode = sizeof(vccomif::rdg::v1::interfaces::StatusCode);
                        if (sizeOfFileCounter > (UINT32_MAX - sizeOfStatusCode)) {
                            LOG_E("May wrap error");
                        }
                        if (udsData != nullptr) {
                            const uint32_t sizeOfEcuAddressInformation {tmpDiagMessage.ecu_address_information().ByteSizeLong()};
                            const uint32_t tmp{sizeOfFileCounter + sizeOfStatusCode};
                            if (sizeOfEcuAddressInformation > (UINT32_MAX - tmp))
                            {
                                LOG_E("May wrap error");
                            }
                            const uint32_t remainingBytes {SSR_UPLOAD_DATA_SIZE_MAX - (tmp + sizeOfEcuAddressInformation)};
                            if (udsData->GetUdsPayload()->data() != nullptr)
                            {
                                tmpDiagMessage.set_user_data(udsData->GetUdsPayload()->data(), remainingBytes < udsData->GetUdsPayload()->size() ? remainingBytes : udsData->GetUdsPayload()->size());
                            } else {
                                LOG_E("udsData->GetUdsPayload()->data() is nullptr");
                            }
                            LOG_D("Assigne status_code = SC_SUCCESSFUL_WITH_EXCEEDED_SIZE");
                        }
                    }
                    break;
                } else {
                    sizeOfFileCounter += tmpDiagMessageSize;
                    const std::unordered_map<uint32_t, RefData>::iterator itRef{mResSID22.find(mSsrTransList[mCurrentTransmissionId]->getEcuInformation().getTargetAddress())};
                    if (itRef != mResSID22.end())
                    {
                        ssrDataReq.set_software_part_number(itRef->second.getSwPartNumber());
                        ssrDataReq.set_ecu_maker_code(itRef->second.getEcuMakerCode());
                    }
                    else
                    {
                        //RDG30-R-0777
                        constexpr uint32_t undefinedEcuMakerCode {0xFFFFU};
                        const std::string undefinedSwPartNumber {""};
                        ssrDataReq.set_software_part_number(undefinedSwPartNumber);
                        ssrDataReq.set_ecu_maker_code(undefinedEcuMakerCode);
                    }
                    ssrDataReq.add_ssr_messages()->CopyFrom(tmpDiagMessage);
                }

                if (mIsNeedtoUploadSSRData == true) {
                    LOG_D("Send SSR upload data to Center:");
                    // TODO: Need to change for using Uploader's API
                    /*Save UploadSSRDataRequest to file*/ 
                    std::string str{};
                    google::protobuf::util::JsonOptions option{};
                    option.always_print_primitive_fields = true;
                    option.preserve_proto_field_names = true;
                    (void)google::protobuf::util::MessageToJsonString(ssrDataReq, &str, option);
                    printData(str);
                    const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
                    std::string file_dir {std::to_string(uploadId)};
                    (void)file_dir.append("_UploadSsrDataRequest.dat");
                    (void)DataModel<UploadSsrDataRequest>::save(file_dir, ssrDataReq);
                    const android::sp<UploadTask> task {new UploadTask(uploadId)};
                    task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG040);
                    task->setUploadPatch(file_dir);
                    // task->setUploadId(uploadId);
                    /*Set priority*/
                    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(mTriggerId)};
                    if(it != mSaveReq.end()) {
                        task->setUploadPrio(it->second->getPriority());
                        const uint64_t fileSize{static_cast<uint64_t>(ssrDataReq.ByteSizeLong())};
                        task->setFileSize(fileSize);
                        UploadManager::getInstance()->requestUploadTask(task);  
                    } else {
                        LOG_D("Can not find triggerID in saved list");
                    }

                }
            }
        }
    }
    (void)sizeOfFileCounter;

    if ((mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER)
        || (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER))
    {
        if(this->calculateCRC() == true) {
            mIsNeedtoUploadSSRData = true;
            /* generte upload data */
            LOG_D("CRC Have change in response list");
            /* TBD: Call to Uploader to make upload data*/
        } else {
            LOG_D("CRC NO change in response list");
            mIsNeedtoUploadSSRData = false;
        }
    } else {
        mIsNeedtoUploadSSRData = true;
    }

    if (mIsNeedtoUploadSSRData == true) {
        LOG_D("Send SSR upload data to Center:");
        // TODO: Need to change for using Uploader's API
        /*Save UploadSSRDataRequest to file*/ 
        std::string str{};
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        (void)google::protobuf::util::MessageToJsonString(ssrDataReq, &str, option);
        printData(str);
        const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
        std::string file_dir {std::to_string(uploadId)};
        (void)file_dir.append("_UploadSsrDataRequest.dat");
        (void)DataModel<UploadSsrDataRequest>::save(file_dir, ssrDataReq);
        const android::sp<UploadTask> task {new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG040);
        task->setUploadPatch(file_dir);
        // task->setUploadId(uploadId);
        /*Set priority*/
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(mTriggerId)};
        if(it != mSaveReq.end()) {
            task->setUploadPrio(it->second->getPriority());
            const uint64_t fileSize{static_cast<uint64_t>(ssrDataReq.ByteSizeLong())};
            task->setFileSize(fileSize);
            UploadManager::getInstance()->requestUploadTask(task);  
        } else {
            LOG_D("Can not find triggerID in saved list");
        }

    }

    mSsrTransList.clear();
    mCurrentDiagTriggerType = DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN;
    mIsSsrRunning = false;
    onFinishSSRAcquisition(mTriggerId);
}

void RemoteSSR::onFinishSSRAcquisition(const uint32_t& triggerId)
{
    /* Check if pTriggerId is in savedID then notify done to priorityControl*/
    LOG_I("Finish SSR Acquisition triggerID: %d", triggerId);
    LOG_D("Check mSaveReq size before: %d", mSaveReq.size());
    for (const auto it1 : mSaveReq ){
        LOG_V("triggerId: %d", it1.first);
    }
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(triggerId)};
    if(it != mSaveReq.end()) {
        if (triggerId <= static_cast<uint32_t>(INT32_MAX))
        {
            triggerToNextService(true);
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(triggerId), it->second->getType());
        }
        else
        {
            LOG_E("triggerId is out of range int32_t");
        }
        (void)mSaveReq.erase(it);
    } else {
        LOG_D("Can not find triggerId: %d in mSaveReq", triggerId);
    }
    mSsrTargetECUList.clear();
    LOG_D("Check mSaveReq size after: %d", mSaveReq.size());
}

void RemoteSSR::testingMaxFileSize(const uint16_t fileSize) noexcept
{
    SSR_UPLOAD_DATA_SIZE_MAX = static_cast<uint32_t>(fileSize);
}
void RemoteSSR::printData(const std::string data) const
{
    std::string data_tmp{generateJson(data)};
    std::string::iterator ptr {data_tmp.begin()};
    std::string a{""};
    while(ptr != data_tmp.end() ) {
        if(*ptr != '\n' ) {
            a += *ptr;
        }
        else {
            LOG_V(a.c_str());
            a.clear();
        }
        ptr++;
    }
}
}
