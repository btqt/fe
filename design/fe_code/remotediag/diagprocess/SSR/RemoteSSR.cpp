#include "RemoteSSR.h"

#include "utils/Logger.h"
#include "utils/CollectionCondition.h"
#include <services/OnboardclientManagerAdapter.h>
#include <services/TimeManagerService/TimeManager.h>
#include <unordered_map>

namespace rdgapp {

android::sp<RemoteSSR> RemoteSSR::mSSR_instance {nullptr};

RemoteSSR::~RemoteSSR() = default;

RemoteSSR::RemoteSSR(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper):
        android::RefBase(), RemoteDelegate()
        , mApp(app)
        , mHandler(new MainHandler(privateLooper, *this))
        , mSaveReq{}
        , mCurrentTransmissionId(0U)
        , mDiagnosticsAcquisitionTime(0U)
        , mWarningTriggerOccurrenceTime(0U)
        , mIsSsrRunning(false)
        , mIsSuspending(false)
        , mSession(EcuSession::SESSION_DEFAULT)
        , mIsEndOfAcquisition(false)
        , discardResponseCode(vccomif::rdg::v1::interfaces::ResponseCode_MIN)
        , mIsNeedtoUploadSSRData(true)
        , mIsSkipEcu(false)
        , mPriority{0U}
        , mColId{0U}
        , mCurrentDiagTriggerType(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)
        , mCurrentTriggerTimestamp(0)
        , mCurrentTriggerLocation{nullptr}
        , mSsrTransList{}
        , mCurrentReqID{0U}
        , mTransmissioIdListByID{}
        , mSsrTransListByID{}
        , mResSID22ByID{}
        , mReqIDIsSuspended{}
        , isSsrAcquireAborted(false)
        , isSsrAcquiredRefData(false)
{
    mSSR_instance = this;
    SSR_UPLOAD_DATA_SIZE_MAX = 2097152U;
    mCurrentTriggerLocation = new CommonDefine::RDGLocationData;
    (void)mHandler->obtainMessage(MainHandler::CMD_INIT_SSR)->sendToTarget();
}

android::sp<RemoteSSR> RemoteSSR::getInstance(void)
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

void RemoteSSR::onRdgStop(const bool isStop) const noexcept {
    //obtain message to stop rdg
    (void)mHandler->obtainMessage(MainHandler::CMD_STOP_RDG, static_cast<int32_t>(isStop))->sendToTarget();
}

void RemoteSSR::onChangedRemoteInfo(const int32_t what, const int32_t info)
{
    LOG_I("onChangedRemoteInfo");
    (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE, what, info)->sendToTarget();
}

void RemoteSSR::changeIGStatus(const bool status) 
{
    LOG_I("onReceiveIG status = %s", status ? "IGN_ON" : "IGN_OFF");
    if ((status == false) && (mIsSsrRunning == true))
    {
        mIsSsrRunning = false;
        //RDG30-R-0247, RDG30-R-0689
        LOG_I("mCurrentDiagTriggerType = %u", mCurrentDiagTriggerType);
        LOG_I("mCurrentReqID = %u", mCurrentReqID);
        discardResponseCode = RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION;
        const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans{mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
        android::sp<SsrUdsTransmission> transPtr{nullptr};
        if (itTrans != mSsrTransListByID[mCurrentReqID].end())
        {
            transPtr = itTrans->second;
        }
        if (transPtr != nullptr)
        {
            if (isSsrAcquireAborted  == false){
                isSsrAcquireAborted = true;
            }
            if (transPtr->getState() == SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST)
            {
                transPtr->stopTimeout();
                transPtr->disconnect();
                LOG_E("IG OFF, store undefined");
                mResSID22ByID[mCurrentReqID][transPtr->getEcuInformation().getTargetAddress()].setEcuMakerCode(0xFFFFU);
                mResSID22ByID[mCurrentReqID][transPtr->getEcuInformation().getTargetAddress()].setSwPartNumber("");
                (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
            }
            else
            {
                abortSSRAcquisition(discardResponseCode);
            }
        }
    }
}

void RemoteSSR::onSSRFlagChangeOFF() {
    LOG_D("Clear CRC");
    mCRCManager->requestRemoveCRCFile();
}

void RemoteSSR::handleUnderRepairStatusChange(const int32_t& what, const int32_t status)
{
    LOG_D("handleUnderRepairStatusChange");
    switch (what)
    {
    case WHAT_CHANGED_REPAIR_SATUS:
    {
        const uint8_t under_repair_status{mApp.getUnderRepair()};
        LOG_D("under_repair_status changed = %u, status %d", under_repair_status, status);
        if ( (mIsSsrRunning == true) && (status == 1))
        {
            mIsSsrRunning = false;
            isSsrAcquireAborted = true;
            discardResponseCode = RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR;
            LOG_D("isSsrAcquireAborted: %d", isSsrAcquireAborted);
            const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans{mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
            android::sp<SsrUdsTransmission> transPtr{nullptr};
            if (itTrans != mSsrTransListByID[mCurrentReqID].end())
            {
                transPtr = itTrans->second;
            }
            if (transPtr != nullptr)
            {
                if (transPtr->getState() == SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST)
                {
                    transPtr->stopTimeout();
                    transPtr->disconnect();
                    LOG_E("Discard, store undefined");
                    mResSID22ByID[mCurrentReqID][transPtr->getEcuInformation().getTargetAddress()].setEcuMakerCode(0xFFFFU);
                    mResSID22ByID[mCurrentReqID][transPtr->getEcuInformation().getTargetAddress()].setSwPartNumber("");
                    (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                }
                else {
                    abortSSRAcquisition(discardResponseCode);
                }
            }
        }
        (void)under_repair_status;
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
    mSSRUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
    if (checkPrecondition() == true)
    {
        // Get OBC resource
        const OBCResourceEventCode resEventInfo {OnboardclientAdapter::getInstance()->GetObcResource()};
        if ( resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK )
        {
            LOG_E("SSR wait OBC resource");

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

            mSsrTransListByID[mCurrentReqID].clear();
            mTransmissioIdListByID[mCurrentReqID] = {};
            LOG_V("Size of mSsrTargetECUList: %d", mSsrTargetECUList.size());
            for (uint32_t i{0U}; i < mSsrTargetECUList.size(); i++)
            {
                std::list<CommonDefine::EcuInformation> ecuInfoList{};
                RemoteEcuInformation::getInstance()->getEcuInformationList(ecuInfoList);
                if (ecuInfoList.size() == 0U)
                {
                    LOG_E("ECU list is none");
                    DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
                }
                std::list<CommonDefine::EcuInformation>::iterator ecu_it{};
                for (ecu_it = ecuInfoList.begin(); ecu_it != ecuInfoList.end(); ++ecu_it)
                {
                    if ( mSsrTargetECUList[i].first == ecu_it->getTargetAddress() )
                    {
                        break;
                    }
                }

                if (ecu_it == ecuInfoList.end())
                {
                    LOG_E("ECU is not in the list");
                } 
                else
                {
                    if ((ecu_it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6) || (ecu_it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5))
                    {
                        const android::sp<SsrUdsTransmission> transmission {new SsrUdsTransmission( *this, *ecu_it, mSsrTargetECUList[i].second)};
                        transmission->transInit();
                        const uint64_t tempTransID{transmission->getTransmissionId()};
                        if (tempTransID <= UINT64_MAX)
                        {
                            mSsrTransListByID[mCurrentReqID][tempTransID] = transmission;
                            mTransmissioIdListByID[mCurrentReqID].push(tempTransID);
                        }
                        (void)tempTransID;
                    }
                    else
                    {
                        LOG_E("ECU diag phase is invalid");
                    }
                }
            }
            
            LOG_D("mTransmissioIdListByID[mCurrentReqID] size = %zu", mTransmissioIdListByID[mCurrentReqID].size());
            if ( mTransmissioIdListByID[mCurrentReqID].size() > 0U )
            {
                mCurrentTransmissionId = static_cast<uint64_t>(mTransmissioIdListByID[mCurrentReqID].front());
                const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans {mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
                if (itTrans != mSsrTransListByID[mCurrentReqID].end())
                {
                    itTrans->second->connect();
                }
            } else {
                // Release OBC resource
                onFinishSSRAcquisition(mCurrentReqID);
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
            }
        }
    }
    else
    {
        LOG_E("conditions not match");
        // Release OBC resource
        // TMCDCMTF-35635
        // OnboardclientAdapter::getInstance()->ReleaseObcResource();
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(mCurrentReqID)};
        if(it != mSaveReq.end())
        {
            if(mCurrentReqID<=static_cast<uint32_t>(INT32_MAX))
            {
                const DiagTrigger::DiagTriggerType tmp_type {it->second->getType()};
                if ((tmp_type >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
                        (tmp_type <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)) {
                    LOG_I("Trigger type is valid");
                } else {
                    LOG_I("Trigger type is out of range");
                }
                PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mCurrentReqID), tmp_type);
            }
            (void)mSaveReq.erase(it);
        }
    }
}

bool RemoteSSR::checkPrecondition()
{
    //get RDG active flag
    const uint8_t RDG_flag{DiagManagerAdapter::getInstance()->getRDGFlag()};
    //get SSR flag
    const uint8_t SSR_flag {DiagManagerAdapter::getInstance()->getSSRFlag()};
    //get IG status
    const IG_STATUS IG_status {PowerManagerAdapter::getInstance()->getIgnitionStatus()};
    //get data upload consent status
    const bool consent_status {DiagManagerAdapter::getInstance()->getAllUploadConsent()};
    //get Under Repair Status
    const uint8_t under_repair_status{mApp.getUnderRepair()};
    LOG_D("Precondition: SSR_flag = %d, IG_status = %d, consent_status = %d, under_repair_status = %d", SSR_flag, IG_status, consent_status, under_repair_status);
    //const bool ret{(SSR_flag == 0x01U) && (IG_status == IG_STATUS_ON) && (consent_status == true)&&(under_repair_status != 0x01U)};
    bool  ret{false};
    if((mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER))
    {
        if ((RDG_flag == 1U) && (IG_status == IG_STATUS_ON) && (SSR_flag == 1U))
        {
            if(under_repair_status == 0x00U) {
                LOG_D("Under repair status is false");
                LOG_I("SSR executes Conditions is PASS");
                ret = true;
            } else {
                LOG_D("Under repair status is true");
                LOG_D("SSR is restricted due to under repair status");
                LOG_D("RC_OBE_ERROR_UNDER_REPAIR");
                constexpr RdgProtoInterface::ResponseCode resCode{RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR};
                abortSSRAcquisition(resCode);
            }
        } else {
            LOG_I("AllDiag executes Conditions is FAIL");
            RdgProtoInterface::ResponseCode resCode{RdgProtoInterface::ResponseCode::RC_OTHER_ERROR};
            if ((RDG_flag != 1U) || (SSR_flag != 1U))
            {
                LOG_I("RC_REQUEST_ERROR_UNPROVIDED_VEHICLE");
                resCode = RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_UNPROVIDED_VEHICLE;
            }
            if (IG_status != IG_STATUS_ON)
            {
                LOG_I("RC_VEHICLE_ERROR_POWER_CONDITION");
                if(resCode > RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION) {
                    resCode = RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION;
                }
            }
            abortSSRAcquisition(resCode);
        }
    } else if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
        LOG_I("SSR running by IG ON trigger");
        if(under_repair_status == 0x00U) {
            LOG_D("Under repair status is false");
            LOG_I("AllDiag executes Conditions is PASS");
            ret = true;
        } else {
            LOG_D("Under repair status is true");
            LOG_D("SSR IG ON Trigger is restricted due to under repair status");
            constexpr RdgProtoInterface::ResponseCode resCode{RdgProtoInterface::ResponseCode::RC_OTHER_ERROR};
            abortSSRAcquisition(resCode);
        }
    } else if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
        /* Handle warning trigger*/
        LOG_I("SSR running by warning trigger");
        if(under_repair_status == 0x00U) {
            LOG_D("Under repair status is false, conditions is PASS");
            ret = true;
        } else {
            LOG_D("Under repair status is true");
            LOG_D("SSR Warning trigger is restricted due to under repair status");
            constexpr RdgProtoInterface::ResponseCode resCode{RdgProtoInterface::ResponseCode::RC_OTHER_ERROR};
            abortSSRAcquisition(resCode);
        }
    } else {
        LOG_D("Undefined DiagTriggerType");
    }
    (void) RDG_flag;
    (void) SSR_flag;
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
        // PriorityControl::getInstance()->notifyTriggerNoFound(pTriggerId);
        /*Forward to RoB*/
        mApp.forwardSsrToRob(pState, pTriggerId, dueToIgOff);
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
            mCurrentReqID = tmpTriggerId;
            mCurrentDiagTriggerType = pTriggerType;
            mCurrentTriggerLocation->setLatitude(it->second->getLatitude());
            mCurrentTriggerLocation->setLongitude(it->second->getLongtitude());
            mSsrTargetECUList = mSsrTargetECUListByID[mCurrentReqID];
            mPriority = it->second->getPriority();
            mColId = it->second->getCollectionID();
            if (pTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
            {
                const int64_t triggerTime{it->second->getWarningTriggerTime()};
                if(triggerTime >= 0)
                {
                    mWarningTriggerOccurrenceTime = static_cast<uint64_t>(triggerTime);
                    LOG_I("Get mWarningTriggerOccurrenceTime: %llu", mWarningTriggerOccurrenceTime);
                }
            } else {
                const int64_t triggerTime{it->second->getTriggerTime()};
                if(triggerTime >= 0)
                {
                    mDiagnosticsAcquisitionTime = static_cast<uint64_t>(triggerTime);
                    LOG_I("Get Diagnostics Acquisition Time: %llu", mDiagnosticsAcquisitionTime);
                }
            }
            for ( uint32_t i{0U}; i < mSsrTargetECUList.size(); i++ )
            {
                LOG_V("Target ECU %u: 0x%02lx - DTC 0x%02X", i, mSsrTargetECUList[i].first, mSsrTargetECUList[i].second);
            }
            if (mReqIDIsSuspended[mCurrentReqID] == true)
            {
                mIsSsrRunning = true;
                LOG_I("TRIGGER_RESUME");
                LOG_V("Resume mCurrentReqID = %u", mCurrentReqID);
                isSsrAcquireAborted = false;
                mReqIDIsSuspended[mCurrentReqID] = false;
                LOG_D("mTransmissioIdList size = %d", mTransmissioIdListByID[mCurrentReqID].size());
                bool ssrFlag{false};
                ssrFlag = DiagManagerAdapter::getInstance()->getSSRFlag()>0U;
                if (ssrFlag == false)
                {
                    LOG_I("SSRFlag is OFF. Finish SSR Acquisition.");
                }
                if ((mTransmissioIdListByID[mCurrentReqID].size() > 0U) && (ssrFlag))
                {
                    mCurrentTransmissionId = static_cast<uint64_t>(mTransmissioIdListByID[mCurrentReqID].front());
                    const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans {mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
                    if (itTrans != mSsrTransListByID[mCurrentReqID].end())
                    {
                        itTrans->second->transInit();
                        itTrans->second->connect();
                    }
                } else {
                    // Release OBC resource
                    onFinishSSRAcquisition(mCurrentReqID);
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                }
            }
            else
            {
                LOG_I("TRIGGER_PROCESSING");
                // const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
                const android::sp<sl::Message> handlemsg {mHandler->obtainMessage(MainHandler::CMD_START_SSR, mCurrentTriggerLocation)};
                handlemsg->arg1 = static_cast<int32_t>(pTriggerType);
                (void)handlemsg->sendToTarget();
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED:
        {
            LOG_I("TRIGGER_SUSPENDED");
            if(mIsSsrRunning == true) {
                LOG_V("mCurrentReqID = %u", mCurrentReqID);
                mReqIDIsSuspended[mCurrentReqID] = true;
                LOG_V("mIsSuspending = %d", mReqIDIsSuspended[mCurrentReqID]);

            }
            const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans{mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
            android::sp<SsrUdsTransmission> transPtr{nullptr};
            if (itTrans != mSsrTransListByID[mCurrentReqID].end())
            {
                transPtr = itTrans->second;
            }
            if (transPtr != nullptr)
            {
                if (transPtr->getState() == SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST)
                {
                    isSsrAcquiredRefData = true;
                    LOG_E("suspend, store undefined");
                    mResSID22ByID[mCurrentReqID][transPtr->getEcuInformation().getTargetAddress()].setEcuMakerCode(0xFFFFU);
                    mResSID22ByID[mCurrentReqID][transPtr->getEcuInformation().getTargetAddress()].setSwPartNumber("");
                } else if (transPtr->getState() != SsrUdsTransmission::State::SSR_TRANS_INIT){
                    LOG_E("suspend, wait UDS response");
                } else {
                    finishCurrentTransmission();
                    PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
                }
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
        {
            LOG_I("TRIGGER_DISCARDED");
            if (mIsSsrRunning == true)
            {
                isSsrAcquireAborted = true;
                LOG_D("isSsrAcquireAborted: %d", isSsrAcquireAborted);
            }
            const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans{mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
            android::sp<SsrUdsTransmission> transPtr{nullptr};
            if (itTrans != mSsrTransListByID[mCurrentReqID].end())
            {
                transPtr = itTrans->second;
            }
            if (transPtr != nullptr)
            {
                if(dueToIgOff == true) {
                    discardResponseCode = RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION;
                } else {
                    discardResponseCode = RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED;
                }
                if (transPtr->getState() == SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST)
                {
                    isSsrAcquiredRefData = true;
                    LOG_E("Discard, store undefined");
                    mResSID22ByID[mCurrentReqID][transPtr->getEcuInformation().getTargetAddress()].setEcuMakerCode(0xFFFFU);
                    mResSID22ByID[mCurrentReqID][transPtr->getEcuInformation().getTargetAddress()].setSwPartNumber("");
                }
                else {
                    abortSSRAcquisition(discardResponseCode);
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
            if (location == nullptr)
            {
                LOG_E("location data is null");
                break;
            } else {
                LOG_I("Get location data success, 0x%08X 0x%08X", location->getLatitude(), location->getLongtitude());
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
            if (pTrigger != nullptr)
            {
                LOG_I("Trigger Request have Type: %d Func: %d Prio: %d ID: %d", 
                    pTrigger->getType(), pTrigger->getFunc(), pTrigger->getPriority(), pTrigger->getTriggerId());
                PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
            } else {
                LOG_E("pTrigger is nullptr");
            }
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
            mSSR.handleUnderRepairStatusChange(handlemsg->arg1, handlemsg->arg2);
            break;
        }
        case CMD_SSR_FINISH_TRANSMISSION:
        {
            LOG_I("CMD_SSR_FINISH_TRANSMISSION");
            mSSR.finishCurrentTransmission();
            break;
        }
        /*Handle message uds receive*/
        case CMD_RECEIVE_UDS_RESPONSE:
        {
            LOG_I("CMD_RECEIVE_UDS_RESPONSE");
            android::sp<OBCResponseEventInfo> responseEventInfo{nullptr};
            handlemsg->getObject(responseEventInfo);
            if (responseEventInfo != nullptr)
            {
                const android::sp<::Buffer> udsData{responseEventInfo->resInfo()->udsData()};
                const android::sp<UdsMessage> udsResponse{new UdsMessage()};
                if (udsData->size() > 0U)
                {
                    (void)udsResponse->Parser(udsData);
                }
                mSSR.onHandleUDSResponse(responseEventInfo, udsResponse);
            } else {
                LOG_E("responseEventInfo is nullptr");
            }
            break;
        }
        case CMD_TRANSMISSION_TIMEOUT:
        {
            LOG_I("CMD_TRANSMISSION_TIMEOUT");
            mSSR.handleTransmissionTimeout();
            break;
        }
        case CMD_STOP_RDG:
        {
            const int32_t isStop{handlemsg->arg1};
            if(isStop == 1) {
                LOG_I("CMD_STOP_RDG");
                mSSR.handleStopRDG();
            } else {
                LOG_I("Power source enabled: RDG functionality is now active");
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void RemoteSSR::onHandleUDSResponse(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
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
        const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans{mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
        android::sp<SsrUdsTransmission> transPtr{nullptr};
        if (itTrans != mSsrTransListByID[mCurrentReqID].end())
        {
            transPtr = itTrans->second;
        }
        if ((transPtr != nullptr) && (mTransmissioIdListByID[mCurrentReqID].empty() != true) && (transPtr->getConnectId() == responseEventInfo->resInfo()->connectId()))
        {
            if ((isSsrAcquireAborted == false) && (mReqIDIsSuspended[mCurrentReqID] == false))
            {
                transPtr->setRxAdd(centerRxAdd);
                transPtr->handleUdsResponse(responseEventInfo, udsResponse);
            } else
            {
                transPtr->stopTimeout();
                LOG_D("SSR is suspend or discard");
                if(transPtr->getState() == SsrUdsTransmission::State::SSR_TRANS_CLOSESESSION_REQUEST)
                {
                    LOG_D("Change to default session");
                    mIsSsrRunning = false;
                    if(mReqIDIsSuspended[mCurrentReqID])
                    {
                        transPtr->disconnect();
                        if (isSsrAcquiredRefData)
                        {
                            (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                        } else {
                            finishCurrentTransmission();
                            if (mCurrentReqID < static_cast<uint32_t>(INT32_MAX))
                            {
                                PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mCurrentReqID), mCurrentDiagTriggerType);
                            }
                        }
                    } else {
                        if (isSsrAcquiredRefData)
                        {
                            transPtr->disconnect();
                            (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                        } else {
                            abortSSRAcquisition(discardResponseCode);
                        }
                    }
                } else {
                    const CommonDefine::DiagPhase diagPhase{transPtr->getEcuInformation().getDiagPhase()};
                    if (diagPhase == CommonDefine::DiagPhase::DP_PHASE_5)
                    {
                        transPtr->changeToDefaultSession();
                    } else {
                        mIsSsrRunning = false;
                        if(mReqIDIsSuspended[mCurrentReqID])
                        {
                            transPtr->disconnect();
                            if (isSsrAcquiredRefData)
                            {
                                (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                            } else {
                                finishCurrentTransmission();
                                if (mCurrentReqID < static_cast<uint32_t>(INT32_MAX))
                                {
                                    PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mCurrentReqID), mCurrentDiagTriggerType);
                                }
                            }
                        } else {
                            if (discardResponseCode == vccomif::rdg::v1::interfaces::ResponseCode_MIN)
                            {
                                transPtr->disconnect();
                                (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                            } else {
                                abortSSRAcquisition(discardResponseCode);
                            }
                        }
                    }
                    (void)diagPhase;
                }
            }
        }
        (void)centerRxAdd;
    }
    return;
}

void RemoteSSR::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
   /*Check If DTC is running*/
    if (mIsSsrRunning == true)
    {
        (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UDS_RESPONSE, responseEventInfo)->sendToTarget();
        (void)udsResponse;
    }
}

RemoteSSR::SsrUdsTransmission::SsrUdsTransmission( RemoteSSR& ssr
                                                    , const CommonDefine::EcuInformation& ecuInformation, const uint32_t DTC)
: android::RefBase()
, mSSR(ssr)
, DTCNumber(0U)
, mTimerHandler(ssr)
, mTimeOut(&mTimerHandler, TimerHandler::ID_TRANSMISSION_TIMEOUT)
, ecuInfo(ecuInformation)
, connectId(0U)
, transmissionId(0U)
, m_state(SsrUdsTransmission::State::SSR_TRANS_INIT)
, mIsAcquireRefData(false)
, centerRxAdd(0U)

{
    mTimeOut.setDuration(TRANSMISSION_TIME_OUT_DURATION, 0U);
    const uint32_t targetEcu{ecuInfo.getTargetAddress()};
    DTCNumber = DTC;
    transmissionId = static_cast<uint64_t>(targetEcu) << 32U | static_cast<uint64_t>(DTCNumber);
    LOG_D("Init transmissionId = 0x%02llx", transmissionId);
}

void RemoteSSR::SsrUdsTransmission::transInit()
{
    //add SID 19 SubFunction 03 request
    this->udsReqList = std::queue<android::sp<UdsMessage>>();
    mSSR.mResSID22ByID[mSSR.mCurrentReqID].clear();
    const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
    if (this->ecuInfo.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6)
    {
        pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION));
        pUdsReq->setSFID(static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_03_REPORT_DTC_SNAPSHOT_IDENTIFICATION));
        udsReqList.push(pUdsReq);
    } else
    {
        pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL));
        pUdsReq->setSFID(static_cast<uint8_t>(EcuSession::SESSION_REMOTE));
        udsReqList.push(pUdsReq);
    }
}

void RemoteSSR::SsrUdsTransmission::connect()
{
    LOG_I("Start connect, transmissionID = 0x%02llx", this->transmissionId);
    const android::sp<OBCTransportInfo> mtransportInfo{new OBCTransportInfo()};
    const android::sp<OBCConnectInfo> mConnectInfo{new OBCConnectInfo()};
    const uint8_t mprotocolType{static_cast<uint8_t>(RemoteEcuInformation::getInstance()->convertObcProtocolType(this->ecuInfo.getCommProtocol(), this->ecuInfo.getCommType(), this->ecuInfo.getTargetAddress()))};
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
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << nTa;
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
    if (mConnectInfo != nullptr)
    {
        if (mConnectInfo->getResponse() != OBCEnum::OBCErrCode::OBC_OK)
        {
            LOG_E("can't connect to canid = %d", this->ecuInfo.getTargetAddress());
            (void)mSSR.mHandler->obtainMessage(MainHandler::CMD_SSR_FINISH_TRANSMISSION)->sendToTarget();

        } else {
            LOG_D("connect success transmissionID = 0x%02llx", this->transmissionId);
            this->connectId = mConnectInfo->getConnectId();
            // TMCDCMTF-35635
            // OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
            this->sendNextUdsRequest();
        }
    }
}

void RemoteSSR::SsrUdsTransmission::stopTimeout()
{
    LOG_I("Stop Timeout");
    this->mTimeOut.stop();
}

void RemoteSSR::SsrUdsTransmission::disconnect()
{
    LOG_D("disconnect, transmissionId = 0x%02llx", this->transmissionId);
    (void)OnboardclientAdapter::getInstance()->disconnectECU(this->connectId);
    // TMCDCMTF-35635
    // OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
}

void RemoteSSR::SsrUdsTransmission::sendNextUdsRequest()
{
    if (udsReqList.empty() != true)
    {
        const android::sp<UdsMessage> currentUdsReq {this->udsReqList.front()};
        if (currentUdsReq != nullptr)
        {
            const android::sp<::Buffer> udsData{currentUdsReq->ToUdsData()};
            if ((currentUdsReq->getSID() == static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL)) &&  (currentUdsReq->getSFID() == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_01_DEFAULT_SESSION_CONTROL))){
                this->setState(SsrUdsTransmission::State::SSR_TRANS_CLOSESESSION_REQUEST);
            }
            else if ((currentUdsReq->getSID() == static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL)) &&  (currentUdsReq->getSFID() == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_40_REMOTE_SESSION_CONTROL))){
                this->setState(SsrUdsTransmission::State::SSR_TRANS_SESSIONCONTROL_REQUEST);
            }
            else if ((currentUdsReq->getSID() == static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION)) &&  (currentUdsReq->getSFID() == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_03_REPORT_DTC_SNAPSHOT_IDENTIFICATION))){
                this->setState(SsrUdsTransmission::State::SSR_TRANS_SSRFRAMENO_REQUEST);
            }
            else if ((currentUdsReq->getSID() == static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION)) &&  (currentUdsReq->getSFID() == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_04_REPORT_DTC_SNAPSHOT_RECORD_BY_DTC_NUMBER))){
                this->setState(SsrUdsTransmission::State::SSR_TRANS_SSRDATA_REQUEST);
            }
            else if (currentUdsReq->getSID() == static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER)){
                this->setState(SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST);
            } else {
                LOG_E("Not support this SID");
            }
            const uint8_t err {OnboardclientAdapter::getInstance()->sendUdsData(this->connectId, udsData)};
            if (err != 0x00U) //E_OK 
            {
                LOG_E("Send SID 0x%02x, SFID 0x%02x for transmissionId 0x%02llx error = %d -> Disconnect", currentUdsReq->getSID(), currentUdsReq->getSFID(), this->transmissionId, err);
                this->handleNegativeResponse(nullptr, nullptr);
            }
            else {
                LOG_D("Send SID 0x%02x, SFID 0x%02x for transmissionId 0x%02llx success", currentUdsReq->getSID(),currentUdsReq->getSFID(), this->transmissionId);
                LOG_V("State of SsrUdsTransmission = %d", this->getState());
                mTimeOut.start();
            }
        }
    }
    else
    {
        this->disconnect();
        mSSR.finishCurrentTransmission();
        mSSR.mDiagNegativeResp.clear();
    }
}

void RemoteSSR::SsrUdsTransmission::reqAcquireRefData()
{
    this->m_state = SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST;
    if (this->ecuInfo.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6)
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
    else {
        mSSR.mResSID22ByID[mSSR.mCurrentReqID][this->ecuInfo.getTargetAddress()].setEcuMakerCode(0xFFFFU);
        //send SID22 DID$F181
        const android::sp<UdsMessage> pUdsReq1 {new UdsMessage()};
        pUdsReq1->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
        pUdsReq1->setSFID(0xF1U); //start routine
        constexpr uint8_t tmp_did1 {0x81U};
        pUdsReq1->GetUdsPayload()->setTo(&tmp_did1, sizeof(uint8_t));
        udsReqList.push(pUdsReq1);
    }
}

void RemoteSSR::SsrUdsTransmission::handleUdsResponse(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
    const uint8_t sid {udsResponse->getSID()};
    const uint8_t sfid {udsResponse->getSFID()};

    LOG_I("sid: 0x%02x, sfid: 0x%02x", sid, sfid);
    LOG_V("Current state of transmission = %d", this->getState());

    switch (sid)
    {
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_SESSION_CONTROL):
        {
            this->stopTimeout();
            //add SID 19 req to read SSR Frame
            LOG_I("Session control response sfid: 0x%x", sfid);
            if ((sfid == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_40_REMOTE_SESSION_CONTROL)) 
            && (this->getState() == SsrUdsTransmission::State::SSR_TRANS_SESSIONCONTROL_REQUEST))
            {
                mSSR.mSession = RemoteSSR::EcuSession::SESSION_REMOTE;
                LOG_I("UDS_PR_SESSION_CONTROL ECU PHASE 5 - Changed to remote session.");
                const android::sp<UdsMessage> currentUdsReq {this->udsReqList.front()};
                if ((currentUdsReq != nullptr) && (currentUdsReq->getSID() == static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL)))
                {
                    const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                    pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION));
                    pUdsReq->setSFID(static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_03_REPORT_DTC_SNAPSHOT_IDENTIFICATION));
                    udsReqList.push(pUdsReq);
                    this->udsReqList.pop();
                    this->sendNextUdsRequest();
                } else {
                    this->sendNextUdsRequest();
                }
            } else if ((sfid == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_01_DEFAULT_SESSION_CONTROL)) 
            && (this->getState() == SsrUdsTransmission::State::SSR_TRANS_CLOSESESSION_REQUEST))
            {
                mSSR.mSession = RemoteSSR::EcuSession::SESSION_DEFAULT;
                LOG_I("UDS_PR_SESSION_CONTROL ECU PHASE 5 - Changed to default session.");
                if (mSSR.isSsrAcquireAborted == false)
                {
                    android::sp<UdsMessage> currentUdsReq {};
                    if(!this->udsReqList.empty())
                    {
                        currentUdsReq = this->udsReqList.front();
                    } else {
                        LOG_E("udsReqList is empty");
                    }
                    if ((currentUdsReq != nullptr) && (currentUdsReq->getSID() == static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL)))
                    {
                        if (!this->udsReqList.empty())
                        {
                            this->udsReqList.pop();
                        } else {
                            LOG_E("udsReqList is empty!");
                        }
                        this->sendNextUdsRequest();
                    } else {
                        this->disconnect();
                        mSSR.finishCurrentTransmission();
                    }
                }
            } else {
                LOG_E("Wrong SFID");
                this->stopTimeout();
                mSSR.onTransmissionTimeout();
            }
            break;
        }
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION):
        {
            if ((sfid == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_03_REPORT_DTC_SNAPSHOT_IDENTIFICATION))
                && (this->getState() == SsrUdsTransmission::State::SSR_TRANS_SSRFRAMENO_REQUEST))
            {
                this->stopTimeout();
                //handle response of SID$19 SFID$03
                /*  Response message definition for SID$19 SFID$03
                 *   0     1       2        3      4        5         6        7       8         9
                 *  |----|----|--------|-------|-------|----------|--------|-------|-------|----------|---------
                 *  | 59 | 03 |DTC High|DTC Mid|DTC Low|SSR Number|DTC High|DTC Mid|DTC Low|SSR Number|---------
                 */
                android::sp<::Buffer> udsData {nullptr};
                if (udsResponse != nullptr) {
                    udsData = udsResponse->GetUdsPayload();
                }
                uint32_t numSSRNo{0U};
                const uint32_t udsDataSize{udsData->size()};
                std::vector<uint32_t> ssrNoList{};
                numSSRNo = udsDataSize/4U;
                for (uint32_t idx {0U}; idx < numSSRNo; idx++) 
                {
                    if (udsData->data() != nullptr)
                    {
                    /*  Format of one ssrNoData
                    *       0       1       2        3      
                    *  |--------|-------|-------|----------|
                    *  |DTC High|DTC Mid|DTC Low|SSR Number|
                    */
                        const uint32_t ssrNoData {static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[idx*4U]) << 24U) | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[idx*4U+1U]) << 16U)
                                            | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[idx*4U+2U]) << 8U) | static_cast<uint32_t>(udsData->data()[idx*4U+3U])};
                        const uint32_t dtcNoData {static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[idx*4U]) << 16U) | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[idx*4U+1U]) << 8U)
                                            | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[idx*4U+2U]))};
                        // RDG30-R-1303: For Phase 6, validate SSRNo for Confirmed DTCs
                        // For Phase 5, always accept all SSRNo values
                        // Extract SSRNo from the 4th byte of the 4-byte SSR entry
                        // SSR entry format: [DTC_BYTE1][DTC_BYTE2][DTC_BYTE3][SSR_NO]
                        // idx*4U+3U points to the SSRNo byte (4th byte of current entry)
                        const uint8_t ssrNo{static_cast<uint8_t>(udsData->data()[idx*4U+3U])};
                        const CommonDefine::DiagPhase diagPhase{this->ecuInfo.getDiagPhase()};
                        // Validate diagPhase before using it
                        if ((diagPhase >= CommonDefine::DiagPhase::DP_PHASE_5) && 
                            (diagPhase <= CommonDefine::DiagPhase::DP_PHASE_6) &&
                            (dtcNoData == this->DTCNumber) && 
                            mSSR.isValidSSRNoForPhase6(ssrNo, diagPhase)){
                            LOG_D("Push valid SSR to list: DTC 0x%06X, SSRNo 0x%02X", dtcNoData, ssrNo);
                            ssrNoList.push_back(ssrNoData);
                        }
                        (void)ssrNo;
                        (void)ssrNoData;
                        (void)dtcNoData;
                        (void)diagPhase;
                    }
                    else {
                        LOG_E("udsData->data() is empty");
                    }
                }
                sort(ssrNoList.begin(), ssrNoList.end(), greater<uint32_t>());

                //create UDS request message for SID$19 SFID$04
                /*  Format of one request SID$19 SFID$04
                *   0     1       2        3      4        5      
                *  |----|----|--------|-------|-------|----------|
                *  | 19 | 04 |DTC High|DTC Mid|DTC Low|SSR Number|
                */
                for (size_t idx {0U}; idx < ssrNoList.size(); idx++) 
                {
                    const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                    pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION));
                    pUdsReq->setSFID(static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_04_REPORT_DTC_SNAPSHOT_RECORD_BY_DTC_NUMBER));
                    uint8_t ssrNo_Data[sizeof(ssrNoList[idx])];
                    (void)memcpy(&ssrNo_Data[0], &ssrNoList[idx], sizeof(ssrNo_Data));
                    const android::sp<::Buffer> udsReqPayload{pUdsReq->GetUdsPayload()};
                    udsReqPayload->setTo(&ssrNo_Data[3], 1);
                    udsReqPayload->append(&ssrNo_Data[2], 1);
                    udsReqPayload->append(&ssrNo_Data[1], 1);
                    udsReqPayload->append(&ssrNo_Data[0], 1);
                    udsReqList.push(pUdsReq);
                }
                LOG_D("size of udsReqList: %d", udsReqList.size());
                //remove the request in front and send next request
                if (!this->udsReqList.empty())
                {
                    this->udsReqList.pop();
                } else {
                    LOG_E("udsReqList is empty!");
                }
                if(udsReqList.size() == 0U)
                {
                    if (this->ecuInfo.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) {
                        this->changeToDefaultSession();
                    } else {
                        this->disconnect();
                        mSSR.finishCurrentTransmission();
                    }
                } else {
                    this->sendNextUdsRequest();
                }
            }
            else if ((sfid == static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_04_REPORT_DTC_SNAPSHOT_RECORD_BY_DTC_NUMBER))
            && (this->getState() == SsrUdsTransmission::State::SSR_TRANS_SSRDATA_REQUEST))
            {
                this->stopTimeout();
                //handle response of SID$19 SFID$04
                /*  Response message definition for SID$19 SFID$04
                 *   0     1       2        3      4        5         6       n bytes of SSR Data
                 *  |----|----|--------|-------|-------|-----------|---------|-------------------|
                 *  | 59 | 04 |DTC High|DTC Mid|DTC Low|statusOfDTC|SSR Frame|      SSR data     |
                 */
                this->mIsAcquireRefData = true;
                android::sp<::Buffer> udsData {nullptr};
                if (udsResponse != nullptr) {
                    udsData = udsResponse->ToUdsData();
                }
                if ((udsData != nullptr) && (udsData->size() > 2U)) 
                {
                    if(udsData->data() != nullptr) {
                        uint8_t* const ssrUserData{&(udsData->data()[0])};
                        if(ssrUserData != nullptr) {
                            if(udsData->size() > 5U)
                            {
                                ssrUserData[5] &=  0x0CU; // masking StatusOfDTC  with 0x0C (keep pendingDTC,confirmedDTC bits, other set to 0)
                            }
                        } else {
                            LOG_D("ssrUserData is nullptr");
                        }
                        const android::sp<UdsMessage> udsResponse_crc{new UdsMessage(udsResponse)};
                        (void)udsResponse_crc->Parser(udsData);
                        mSSR.mListDiagResp_CRC[transmissionId].push_back(udsResponse_crc);
                        mSSR.mTransDiagResp[transmissionId].push_back(udsResponse_crc);
                    } else {
                        LOG_I("udsResponse_crc is nullptr");
                    }
                    //get reference data
                    if (udsReqList.size() == 1U)
                    {
                        if (mSSR.mResSID22ByID[mSSR.mCurrentReqID].find(this->ecuInfo.getTargetAddress()) == mSSR.mResSID22ByID[mSSR.mCurrentReqID].end())
                        {
                            this->reqAcquireRefData();
                        }
                        if (this->ecuInfo.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) {
                            LOG_D("Last uds request, add request change to default session.");
                            const android::sp<UdsMessage> defaultSessionReq {new UdsMessage()};
                            defaultSessionReq->setSID(static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL));
                            defaultSessionReq->setSFID(static_cast<uint8_t>(RemoteSSR::EcuSession::SESSION_DEFAULT));
                            udsReqList.push(defaultSessionReq);
                        }
                    }
                }
                (void)DTCNumber;
                if (!this->udsReqList.empty())
                {
                    this->udsReqList.pop();
                } else {
                    LOG_E("udsReqList is empty!");
                }
                this->sendNextUdsRequest();
            }
            else
            {
                LOG_E("Wrong sfid");
                this->stopTimeout();
                mSSR.onTransmissionTimeout();
            }
            break;
        }

        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER):
        {
            // const RefData refData;
            /*  Response message definition for SID$22 SFID$04
            *   0      1       2   n bytes of Reference Data
            *  |----|------|------|-------------------------|
            *  | 62 |   resDid    |     Reference Data      |
            */
            if (this->getState() == SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST)
            {
                this->stopTimeout();
                android::sp<::Buffer> udsData {nullptr};
                if (udsResponse != nullptr) {
                    udsData = udsResponse->ToUdsData();
                }
                if ((udsData != nullptr) && (udsData->data() != nullptr))
                {
                    const uint16_t resDid {static_cast<uint16_t>(static_cast<uint16_t>(static_cast<uint16_t>(udsData->data()[1]) << 8U) | static_cast<uint16_t>(udsData->data()[2]))};
                    if (resDid == 0xF18AU)
                    {
                        mSSR.mResSID22ByID[mSSR.mCurrentReqID][this->ecuInfo.getTargetAddress()].setEcuMakerCode(static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[3]) << 8U) | static_cast<uint32_t>(udsData->data()[4]));

                    }
                    else if ((resDid == 0xF188U) || (resDid == 0xF181U))
                    {
                        const uint32_t udsDataSize{udsData->size()};
                        if (udsDataSize > 3U)
                        {
                            std::string stSwPtNum{};
                            stSwPtNum.resize(udsDataSize - 3U);
                            if (stSwPtNum.size() > 0U)
                            {
                                (void)memcpy(&stSwPtNum[0], &udsData->data()[3], static_cast<uint32_t>(stSwPtNum.size()));
                            }
                            mSSR.mResSID22ByID[mSSR.mCurrentReqID][this->ecuInfo.getTargetAddress()].setSwPartNumber(stSwPtNum);
                        }
                    }
                    else
                    {
                        LOG_E("Wong DID, transmission timeout");
                    }
                }
                else {
                    LOG_E("udsData->data() is nullptr");
                }
                if (!this->udsReqList.empty())
                {
                    this->udsReqList.pop();
                } else {
                    LOG_E("udsReqList is empty!");
                }
                this->sendNextUdsRequest();
            } else {
                LOG_E("Mismatch State");
                this->stopTimeout();
                mSSR.onTransmissionTimeout();
            }
            break;
        }

        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE):
        {
            LOG_E("Negative response");
            this->stopTimeout();
            this->handleNegativeResponse(responseEventInfo, udsResponse);
            break;
        }

        default:
        {
            if (responseEventInfo->getErrCode() != OBCEnum::OBCErrCode::OBC_OK)
            {
                LOG_E("OBC_TIMEOUT");
                this->stopTimeout();
                mSSR.onTransmissionTimeout();
            }
            else
            {
                LOG_E("unsupported SID 0x%02x", sid);
                this->stopTimeout();
                mSSR.onTransmissionTimeout();
            }
            break;
        }
    }
}

bool RemoteSSR::isValidSSRNoForPhase6(const uint8_t ssrNo, const CommonDefine::DiagPhase diagPhase) const noexcept
{
    bool ret{false};
    // RDG30-R-1303: SSRNo is valid only for Confirmed DTCs: 01h, 20h-2Fh [Phase6]
    if (diagPhase == CommonDefine::DiagPhase::DP_PHASE_5)
    {
        LOG_D("Phase 5: SSRNo always valid");
        ret = true;
    } else {
        ret = (ssrNo == 0x01U) || ((ssrNo >= 0x20U) && (ssrNo <= 0x2FU));
        LOG_D("Phase 6: SSRNo 0x%02X validation result: %s", ssrNo, ret ? "valid" : "invalid");
    }
    return ret;
}

bool RemoteSSR::calculateCRC(const uint64_t keyCompare) 
{
    bool isDifferent{false};
    if(mCRCManager != nullptr) {
        /*TBD: caculate CRC*/
        mCRCManager->calculateCRC16Data();
        isDifferent = mCRCManager->compareCrc16ValueSSR(keyCompare);
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
    (void)mHandler->obtainMessage(MainHandler::CMD_TRANSMISSION_TIMEOUT)->sendToTarget();
}

void RemoteSSR::handleTransmissionTimeout()
{
    LOG_I("Event Transmission Timeout, currentTrans = 0x%02llx", mCurrentTransmissionId);
    const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans{mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
    android::sp<SsrUdsTransmission> transPtr{nullptr};
    if (itTrans != mSsrTransListByID[mCurrentReqID].end())
    {
        transPtr = itTrans->second;
    }
    if (transPtr != nullptr)
    {
        android::sp<UdsMessage> currentUdsReq {};
        if(!transPtr->udsReqList.empty())
        {
            currentUdsReq = transPtr->udsReqList.front();
        } else {
            LOG_E("udsReqList is empty");
        }
        if (transPtr->getState() == SsrUdsTransmission::State::SSR_TRANS_SSRFRAMENO_REQUEST)
        {
            LOG_I("NEG or no response while acquiring SSR data in SubFunction$03");
            //discard response and shift to next ECU
            mIsSkipEcu = true;
            mIsNeedtoUploadSSRData = false;
            const CommonDefine::DiagPhase diagPhase{transPtr->getEcuInformation().getDiagPhase()};
            if (diagPhase == CommonDefine::DiagPhase::DP_PHASE_5)
            {
                transPtr->changeToDefaultSession();
            } else {
                transPtr->disconnect();
                finishCurrentTransmission();
            }
            (void)diagPhase;
        }
        else if (transPtr->getState() == SsrUdsTransmission::State::SSR_TRANS_SSRDATA_REQUEST)
        {
            //store negative response message and shift to next SSRNo
            LOG_I("SSR_TRANS_SSRDATA_REQUEST timeout, store request message.");
            mTransDiagResp[mCurrentTransmissionId].push_back(currentUdsReq);
            if (transPtr->udsReqList.size() == 1U)
            {
                if ((mResSID22ByID[mCurrentReqID].find(transPtr->ecuInfo.getTargetAddress()) == mResSID22ByID[mCurrentReqID].end())
                    && (transPtr->mIsAcquireRefData))
                {
                    transPtr->reqAcquireRefData();
                }
                if (transPtr->ecuInfo.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) {
                    LOG_D("Last uds request, add request change to default session.");
                    const android::sp<UdsMessage> defaultSessionReq {new UdsMessage()};
                    defaultSessionReq->setSID(static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL));
                    defaultSessionReq->setSFID(static_cast<uint8_t>(RemoteSSR::EcuSession::SESSION_DEFAULT));
                    (void)transPtr->udsReqList.push(defaultSessionReq);
                }
            }
            if (!transPtr->udsReqList.empty())
            {
                transPtr->udsReqList.pop();
            } else {
                LOG_E("udsReqList is empty!");
            }
            LOG_V("Size of udsReqList = %zu", transPtr->udsReqList.size());
            transPtr->sendNextUdsRequest();
        } else if (transPtr->getState() == SsrUdsTransmission::State::SSR_TRANS_SESSIONCONTROL_REQUEST){
            LOG_I("SSR_TRANS_SSRDATA_REQUEST timeout, store request message.");
            mTransDiagResp[mCurrentTransmissionId].push_back(currentUdsReq);
            mIsSkipEcu = true;
            transPtr->disconnect();
            finishCurrentTransmission();
        }
        else if (transPtr->getState() == SsrUdsTransmission::State::SSR_TRANS_CLOSESESSION_REQUEST){
            LOG_I("SSR_TRANS_CLOSESESSION_REQUEST timeout, store request message.");
            transPtr->disconnect();
            finishCurrentTransmission();
        }
        else if (transPtr->getState() == SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST) {
            if (!transPtr->udsReqList.empty())
            {
                transPtr->udsReqList.pop();
            } else {
                LOG_E("udsReqList is empty!");
            }
            transPtr->sendNextUdsRequest();
        } else {
            LOG_E("Wrong State of transmission");
            transPtr->disconnect();
            finishCurrentTransmission();
        }
    }
}

void RemoteSSR::SsrUdsTransmission::handleNegativeResponse(const android::sp<OBCResponseEventInfo> responseEvent, const android::sp<UdsMessage> udsResponse)
{
    LOG_V("Current state of transmission = %d", this->getState());

    android::sp<UdsMessage> currentUdsReq {};
    uint8_t sid {0U};
    uint8_t sfid {0U};
    if(!this->udsReqList.empty())
    {
        currentUdsReq = this->udsReqList.front();
    } else {
        LOG_E("udsReqList is empty");
    }
    if (udsResponse != nullptr)
    {
        sid = udsResponse->getSID();
        sfid = udsResponse->getSFID();
        LOG_V("NEG - sid: 0x%02x, sfid: 0x%02x", sid, sfid);
    } else {
        LOG_E("udsResponse is nullptr");
        sfid = currentUdsReq->getSID();
    }
    if (sfid == static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION))
    {
        if (this->getState() == SsrUdsTransmission::State::SSR_TRANS_SSRFRAMENO_REQUEST)
        {
            LOG_I("NEG or no response while acquiring SSR data in SubFunction$03");
            //discard response and shift to next ECU
            mSSR.mIsSkipEcu = true;
            mSSR.mIsNeedtoUploadSSRData = false;
            const CommonDefine::DiagPhase diagPhase{this->getEcuInformation().getDiagPhase()};
            if (diagPhase == CommonDefine::DiagPhase::DP_PHASE_5)
            {
                this->changeToDefaultSession();
            } else {
                this->disconnect();
                mSSR.finishCurrentTransmission();
            }
            (void)diagPhase;
        }
        else if (this->getState() == SsrUdsTransmission::State::SSR_TRANS_SSRDATA_REQUEST)
        {
            //store negative response message and shift to next SSRNo
            if ((udsResponse != nullptr) && (udsResponse->ToUdsData()->size() > 0U)) 
            {
                if (udsResponse->getNRC() == 0x78U)
                {
                    LOG_I("NRC78, store request message.");
                    mSSR.mTransDiagResp[transmissionId].push_back(currentUdsReq);
                } else {
                    LOG_I("Negative response, store and execute next SSRNo");
                    mSSR.mTransDiagResp[transmissionId].push_back(udsResponse);
                }
            }
            if (udsResponse == nullptr)
            {
                mSSR.mTransDiagResp[transmissionId].push_back(currentUdsReq);
            }
            if (udsReqList.size() == 1U)
            {
                if ((mSSR.mResSID22ByID[mSSR.mCurrentReqID].find(this->ecuInfo.getTargetAddress()) == mSSR.mResSID22ByID[mSSR.mCurrentReqID].end())
                    && (mIsAcquireRefData))
                {
                    this->reqAcquireRefData();
                }
                if (this->ecuInfo.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) {
                    LOG_D("Last uds request, add request change to default session.");
                    const android::sp<UdsMessage> defaultSessionReq {new UdsMessage()};
                    defaultSessionReq->setSID(static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL));
                    defaultSessionReq->setSFID(static_cast<uint8_t>(RemoteSSR::EcuSession::SESSION_DEFAULT));
                    (void)udsReqList.push(defaultSessionReq);
                }
            }
            if (!this->udsReqList.empty())
            {
                this->udsReqList.pop();
            } else {
                LOG_E("udsReqList is empty!");
            }
            LOG_V("Size of udsReqList = %zu", udsReqList.size());
            this->sendNextUdsRequest();
        }
        else
        {
            LOG_E("Wrong sfid");
            mSSR.onTransmissionTimeout();
        }
    } else if (sfid == static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL)){
        if (this->getState() == SsrUdsTransmission::State::SSR_TRANS_SESSIONCONTROL_REQUEST)
        {
            //Store negative response
            //store negative response message and shift to next SSRNo
            if ((udsResponse != nullptr) && (udsResponse->ToUdsData()->size() > 0U)) 
            {
                if (udsResponse->getNRC() == 0x78U)
                {
                    LOG_I("Unresponse SID_10_SESSION_CONTROL, store request message.");
                    mSSR.mTransDiagResp[transmissionId].push_back(currentUdsReq);
                } else {
                    LOG_I("Negative response SID_10_SESSION_CONTROL, store and execute next SSRNo.");
                    mSSR.mTransDiagResp[transmissionId].push_back(udsResponse);
                }
            }
            if (udsResponse == nullptr)
            {
                mSSR.mTransDiagResp[transmissionId].push_back(currentUdsReq);
            }
            mSSR.mIsSkipEcu = true;
            mSSR.mIsNeedtoUploadSSRData = true;
            this->disconnect();
            mSSR.finishCurrentTransmission();
        } else if (this->getState() == SsrUdsTransmission::State::SSR_TRANS_CLOSESESSION_REQUEST) 
        {
            this->disconnect();
            mSSR.finishCurrentTransmission();
        }
        else {
            LOG_E("Wrong sid");
            mSSR.onTransmissionTimeout();
        }
    }
    else if (sfid == static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER)) {
        if (this->getState() == SsrUdsTransmission::State::SSR_TRANS_REFERENCEDATA_REQUEST)
        {
            if (!this->udsReqList.empty())
            {
                this->udsReqList.pop();
            } else {
                LOG_E("udsReqList is empty!");
            }
            this->sendNextUdsRequest();
        } else {
            LOG_E("Wrong sid");
            mSSR.onTransmissionTimeout();
        }
    } else {
        LOG_E("Wrong SID");
        mSSR.onTransmissionTimeout();
    }
    (void)responseEvent;
    (void)sfid;
    (void)sid;
}

void RemoteSSR::SsrUdsTransmission::changeToDefaultSession()
{
    //change session to default
    const android::sp<UdsMessage> pUdsSessionReq {new UdsMessage()};
    pUdsSessionReq->setSID(static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL));
    pUdsSessionReq->setSFID(static_cast<uint8_t>(EcuSession::SESSION_DEFAULT)); //default session
    this->setState(SsrUdsTransmission::State::SSR_TRANS_CLOSESESSION_REQUEST);
    const uint8_t ret {OnboardclientAdapter::getInstance()->sendUdsData(this->connectId, pUdsSessionReq->ToUdsData())};
 
    if (ret != static_cast<uint8_t>(OBCEnum::OBC_OK))
    {
        LOG_E("Request change to Default Session for transmissionId 0x%02llx error = %d -> Disconnect", this->transmissionId, ret);
        this->disconnect();
        mSSR.finishCurrentTransmission();
    }
    else
    {
        LOG_D("Request change to Default Session for transmissionId 0x%02llx success", this->transmissionId);
        mTimeOut.start();
        LOG_I("Start timeout timer for transmissionId 0x%02llx", this->transmissionId);
    }
}

void RemoteSSR::startNextTransmission(void)
{
    LOG_D("startNextTransmission");
    mIsNeedtoUploadSSRData = true;
    if (this->mIsSsrRunning == true)
    {
        if(mIsEndOfAcquisition){
            mTransmissioIdListByID[mCurrentReqID].pop();
            mSsrTransListByID[mCurrentReqID].clear();
            onFinishSSRAcquisition(mCurrentReqID);
            OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
            // if (mCurrentReqID < static_cast<uint32_t>(INT32_MAX))
            // {
            //     PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mCurrentReqID), mCurrentDiagTriggerType);
            // }
        } else {
            if(mIsSkipEcu)
            {
                while(!mTransmissioIdListByID[mCurrentReqID].empty())
                    {
                        const uint64_t removeTrans{static_cast<uint64_t>(mTransmissioIdListByID[mCurrentReqID].front())};
                        const uint32_t tempAddress{static_cast<uint32_t>(removeTrans >> 32U)};
                        const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator it {mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
                        if (it != mSsrTransListByID[mCurrentReqID].end())
                        {
                            LOG_D("current ECU address = 0x%02x; targetEcu = 0x%02x", tempAddress, it->second->ecuInfo.getTargetAddress());
                            if(tempAddress == it->second->ecuInfo.getTargetAddress())
                            {
                                LOG_D("Remove from transmission queue");
                                mTransmissioIdListByID[mCurrentReqID].pop();
                            } else {
                                break;
                            }
                        }
                        (void)removeTrans;
                        (void)tempAddress;
                    }
                mIsSkipEcu = false;
            } else {
                mTransmissioIdListByID[mCurrentReqID].pop();
            }
            mTransDiagResp[mCurrentTransmissionId].clear();
            if(mTransmissioIdListByID[mCurrentReqID].empty())
            {
                LOG_D("Queue empty");
                mSsrTransListByID[mCurrentReqID].clear();
                onFinishSSRAcquisition(mCurrentReqID);
                OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
                // if (mCurrentReqID < static_cast<uint32_t>(INT32_MAX))
                // {
                //     PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mCurrentReqID), mCurrentDiagTriggerType);
                // }
            } else
            {
                mCurrentTransmissionId = static_cast<uint64_t>(mTransmissioIdListByID[mCurrentReqID].front());
                const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator it {mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
                if (it != mSsrTransListByID[mCurrentReqID].end())
                {
                    it->second->connect();
                }
            }
        }
    }  else {
        LOG_E("SSR is not running");
        if(mReqIDIsSuspended[mCurrentReqID])
        {
            mTransmissioIdListByID[mCurrentReqID].pop();
            finishCurrentTransmission();
            if (mCurrentReqID < static_cast<uint32_t>(INT32_MAX))
            {
                PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mCurrentReqID), mCurrentDiagTriggerType);
            }
        } else {
            onFinishSSRAcquisition(mCurrentReqID);
        }
    }
}

void RemoteSSR::abortSSRAcquisition(const RdgProtoInterface::ResponseCode code) {
    LOG_D("abortSSRAcquisition, responsecode = %d", code);
    /*Do abort process*/
    LOG_D("Check isSsrAcquireAborted: %d", isSsrAcquireAborted);
    LOG_D("mCurrentDiagTriggerType = %d", mCurrentDiagTriggerType);
    mIsSsrRunning = false;
    /*Disconnect current ECU*/
    LOG_I("Disconnect current TransmitionID: 0x%llx", mCurrentTransmissionId);
    const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans{mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
    android::sp<SsrUdsTransmission> transPtr{nullptr};
    if (itTrans != mSsrTransListByID[mCurrentReqID].end())
    {
        transPtr = itTrans->second;
    }
    if (transPtr != nullptr)
    {
        transPtr->disconnect();
    } else {
        LOG_I("NOT match transmissionId or ECU was disconnected");
    }
    OnboardclientAdapter::getInstance()->ReleaseObcResource();
    if((mCurrentDiagTriggerType != DiagTrigger::DiagTriggerType::IGON_TRIGGER) &&
        (mCurrentDiagTriggerType != DiagTrigger::DiagTriggerType::WARNING_TRIGGER)) {
        /*Notify make error upload data*/
        mSSRUploadErrorData->set_response_code(((code>=vccomif::rdg::v1::interfaces::ResponseCode_MIN) && (code<=vccomif::rdg::v1::interfaces::ResponseCode_MAX))
                                            ? static_cast<vccomif::rdg::v1::interfaces::ResponseCode>(code)
                                            : vccomif::rdg::v1::interfaces::ResponseCode::RC_OTHER_ERROR);
        makeErrorUploadDataRequest(*mSSRUploadErrorData, mCurrentDiagTriggerType, mColId);
    } else {
        if((mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) && 
        (code == vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION)) {
            /*RDG30-R-0687: IG change to OFF while IG ON trigger running*/
            LOG_D("Do not generate error upload data due to trigger name is IG ON");
            onFinishSSRAcquisition(mCurrentReqID);
        } else if((mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
        && (code == vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION)){
            mSSRUploadErrorData->set_response_code(static_cast<vccomif::rdg::v1::interfaces::ResponseCode>(code));
            makeErrorUploadDataRequest(*mSSRUploadErrorData, mCurrentDiagTriggerType, mColId);
        } else {
            LOG_D("Do not generate error upload data");
            onFinishSSRAcquisition(mCurrentReqID);
        }
        
    }
    if (mCurrentReqID < static_cast<uint32_t>(INT32_MAX))
    {
        PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mCurrentReqID), mCurrentDiagTriggerType);
    }
    else
    {
        LOG_D("triggerId value error: %d in mTriggerList", mCurrentReqID);
    }
}

void RemoteSSR::makeErrorUploadDataRequest(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colID)
{
    LOG_I("Make SSR Error data");
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());

    vccomif::rdg::v1::interfaces::RdgCommonRequestHeader *const mRdgCommonRequestHeader{errorData.mutable_rdg_common_request_header()};
    vccomif::common::v1::AppCommonHeaderVehicleToCenter *const mAppCommonHeaderVehicleToCenter{mRdgCommonRequestHeader->mutable_app_common_header()};

    mAppCommonHeaderVehicleToCenter->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
    mAppCommonHeaderVehicleToCenter->set_electronic_pf(EPF_19EPF);
    mAppCommonHeaderVehicleToCenter->set_geodesy_information(CommonUtils::getGeodesyInfo());
    mRdgCommonRequestHeader->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
    mRdgCommonRequestHeader->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes()); 

    mRdgCommonRequestHeader->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
    // mess id
    mRdgCommonRequestHeader->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, UploadManager::getInstance()->getCounterMessage()));

    const sp<CommonDefine::RDGLocationData> mLocation{LocationManagerAdapter::getInstance()->getLocationData()};
    errorData.set_collection_condition_id(colID); /*TBD*/
    errorData.set_counter_value(UploadManager::getInstance()->getCounterValue());
    //const int64_t currentTime {static_cast<int64_t>(CommonUtils::getCurrentAcquisiteTime())};
    //errorData.set_data_creation_date(currentTime > 0 ? static_cast<uint64_t>(currentTime) : 0U);
    vccomif::rdg::v1::interfaces::TriggerType errorTriggerType{vccomif::rdg::v1::interfaces::TriggerType::TT_UNKNOWN};
    if ((type == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) || (type == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER))
    {
        errorTriggerType = vccomif::rdg::v1::interfaces::TriggerType::TT_OTHER_TRIGGER;
        errorData.set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ALL_DIAG);
        errorData.set_data_creation_date(mDiagnosticsAcquisitionTime);
    }
    else if (type == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        errorTriggerType = vccomif::rdg::v1::interfaces::TriggerType::TT_WARNING_TRIGGER;
        errorData.set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_WARNING_TRIGGER);
        errorData.set_data_creation_date(mWarningTriggerOccurrenceTime);
    }
    else
    {
        LOG_E("Invalid trigger type");
        errorTriggerType = vccomif::rdg::v1::interfaces::TriggerType::TT_UNKNOWN;
        errorData.set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_UNKNOWN);
        errorData.set_data_creation_date(mDiagnosticsAcquisitionTime);
    }
    errorData.set_trigger_type(errorTriggerType);
    const bool bOBDFlag{mApp.getOBDStatus()};
    errorData.set_obd2_installed_flag(bOBDFlag);
    errorData.set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
    std::string str_err_robssr{""};
    google::protobuf::util::JsonOptions option{};
    option.always_print_primitive_fields = true;
    option.preserve_proto_field_names = true;
    (void)google::protobuf::util::MessageToJsonString(errorData, &str_err_robssr, option);
    printData(str_err_robssr);

    const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
    const uint64_t uploadCount {UploadManager::getInstance()->genCountUpload()};
    std::string file_dir {std::to_string(uploadCount)};
    (void)file_dir.append("_UploadErrorSSRDataRequest.dat");
    error_t bSaved {E_ERROR};
    uint32_t fileSize {0U};
    const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
    if (region == LGE_REGION::LGE_REGION_CN)
    {
        bSaved = DataModel<UploadErrorDataRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG160, file_dir, errorData, fileSize);
    }
    else
    {
        fileSize = errorData.ByteSizeLong();
        bSaved = DataModel<UploadErrorDataRequest>::saveUpload(file_dir, errorData);
    }
    if (bSaved == E_OK)
    {
        const uint8_t operation{CommonUtils::getOperation(mCurrentDiagTriggerType)};
        DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        const android::sp<UploadTask> task{new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
        task->setUploadPatch(file_dir);
        task->setUploadId(static_cast<uint64_t>(uploadId));
        /*Set priority*/
        task->setUploadPrio(mPriority);
        task->setFileSize(static_cast<uint64_t>(fileSize));
        UploadManager::getInstance()->requestUploadTask(task);
    }
    else
    {
        LOG_E("Save UploadErrorSSRDataRequest file error");
    }
    mSSRUploadErrorData->Clear();
    LOG_I("Finish SSR, mCurrentReqID = %d", mCurrentReqID);
    onFinishSSRAcquisition(mCurrentReqID);
}

void RemoteSSR::triggerFromDTC(
    const uint32_t triggerID
    , const DiagTrigger::DiagTriggerType triggerType
    , const int64_t timeData
    , const android::sp<CommonDefine::RDGLocationData> location
    , const uint64_t collectionId
    , const uint32_t priority
    , const std::vector<pair<uint32_t, uint32_t>> v_targetEcu)
{
    LOG_I("triggerFromDTC");
    bool ssrFlag{false};
    ssrFlag = DiagManagerAdapter::getInstance()->getSSRFlag()>0U;
    if (ssrFlag == false)
    {
        LOG_I("SSRFlag is OFF. Trigger to RoB.");
        LOG_I("SSR Done");
        mApp.notifyDiagDoneToLastUpload(triggerID);
        mApp.triggerSSRToRoB(triggerID, triggerType, timeData, location, collectionId, priority);
    }
    else
    {
        LOG_D("Check SSR location information: 0x%08x 0x%08x", location->getLatitude(), location->getLongtitude());
        /* Get trigger ID*/
        // const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
        const uint32_t nextTriggerId{triggerID};
        mSsrTargetECUListByID[nextTriggerId] = v_targetEcu;
        /* Create NewDiag Trigger*/
        /* DiagTrigger(const DiagTrigger::DiagTriggerType type, const uint32_t priority, const DiagTrigger::DiagTriggerFunc func, const int32_t triggerId)*/
        const android::sp<DiagTrigger> pTrigger{new DiagTrigger(triggerType, priority, DiagTrigger::DiagTriggerFunc::SSR, nextTriggerId)};
        /* set trigger time*/
        if (triggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
        {
            if (timeData >= 0)
            {
                pTrigger->setWarningTriggerTime(timeData);
                LOG_I("Save mWarningTriggerOccurrenceTime: %lld", timeData);
            }
        } else {
            if (timeData >= 0)
            {
                pTrigger->setTriggerTime(timeData);
                LOG_I("Save Diagnostics Acquisition Time: %lld", timeData);
            }
        }
        /* Set collection id*/
        pTrigger->setCollectionId(collectionId);
        LOG_D("Check Collection ID: %llu ", collectionId);
        pTrigger->setLongitude(location->getLongtitude());
        pTrigger->setLatitude(location->getLatitude());
        /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
        // (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();

        /* Save request to local*/
        const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
        LOG_D("Check mSaveReq size: %d", mSaveReq.size());
        if(!ret.second) {
            ret.first->second = pTrigger;
        }
        LOG_D("Check saved SSR trigger ID: %d", ret.first->first);
        /*Process SSR*/
        if(nextTriggerId > static_cast<uint32_t>(INT32_MAX))
        {
            LOG_E("Out of range");
        }
        (void)notifyTrigger(DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING, static_cast<int32_t>(nextTriggerId), false);
    }
}

void RemoteSSR::triggerToNextService(const bool hasHistoryFile) {
    LOG_I("trigger next service with triggerId: %d, hasHistoryFile: %d", mCurrentReqID, hasHistoryFile);
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(mCurrentReqID)};
    if(it != mSaveReq.end()) {
        LOG_I("Trigger SSR to RoB");
        const DiagTrigger::DiagTriggerType tempTriggerType {it->second->getType()};
        if ( (tempTriggerType > DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) && (tempTriggerType < DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX))
        {
            int64_t triggerTime{0};
            if (tempTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
            {
                triggerTime = it->second->getWarningTriggerTime();
            }
            else
            {
                triggerTime = it->second->getTriggerTime();
            }
            if(triggerTime >= 0)
            {
                mApp.triggerSSRToRoB(mCurrentReqID, tempTriggerType, triggerTime, mCurrentTriggerLocation, mColId, it->second->getPriority());
            }
            else
            {
                LOG_E("triggerTime is invalid");
            }
        } else {
            LOG_E("it->second->getType() invalid");
        }
        (void)tempTriggerType;
    } else {
        LOG_I("Can not find triggerID in saved list");
    }
}

void RemoteSSR::handleStopRDG() {
    LOG_I("handleStopRDG called. Stopping RDG...");

    bool isError{false};

    if (!mIsSsrRunning) {
        LOG_D("SSR is not running");
        isError = true;
    } else {
        mIsSsrRunning = false;

        LOG_I("mCurrentDiagTriggerType = %u", mCurrentDiagTriggerType);
        LOG_I("mCurrentReqID = %u", mCurrentReqID);

        const std::unordered_map<uint32_t, std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>>::iterator itReq {mSsrTransListByID.find(mCurrentReqID)};
        if (itReq == mSsrTransListByID.end()) {
            LOG_E("Request ID not found in mSsrTransListByID");
            isError = true;
        } else {
            const std::unordered_map<uint64_t, android::sp<SsrUdsTransmission>>::iterator itTrans {itReq->second.find(mCurrentTransmissionId)};
            if (itTrans == itReq->second.end()) {
                LOG_E("Transmission not found");
                isError = true;
            } else {
                const android::sp<SsrUdsTransmission> transPtr {itTrans->second};
                if (transPtr != nullptr) {
                    transPtr->stopTimeout();
                    transPtr->disconnect();
                } else {
                    LOG_E("Transmission pointer is null");
                    isError = true;
                }
            }
        }
    }

    // Final return point
    if (isError) {
        LOG_D("handleStopRDG encountered an error.");
    }
}

void RemoteSSR::finishCurrentTransmission(void)
{
    if (this->mIsSsrRunning == true) {
        LOG_I("finish Current transmission ID: 0x%02llx, mTransmissioIdListByID[mCurrentReqID] size = %d", mCurrentTransmissionId, mTransmissioIdListByID[mCurrentReqID].size());
        if ( mTransmissioIdListByID[mCurrentReqID].size() == 1U ) 
        {
            mIsEndOfAcquisition = true;
        }
        const std::unordered_map<uint64_t, std::vector<android::sp<UdsMessage>>>::iterator it{mTransDiagResp.find(mCurrentTransmissionId)};
        if(it == mTransDiagResp.end())
        {
            startNextTransmission();
        } else {
            (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
        }
    } else {
        LOG_I("SSR is aborted => dont process next ECU");
        mTransDiagResp[mCurrentTransmissionId].clear();
        mListDiagResp_CRC[mCurrentTransmissionId].clear();
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
    }
}

void RemoteSSR::makeUploadSSRRequest()
{
    if (mIsNeedtoUploadSSRData)
    {
        LOG_I("Generating data for transmissionId = 0x%02llx", mCurrentTransmissionId);
        for (std::map<uint64_t, vector<android::sp<UdsMessage>>>::iterator it{mListDiagResp_CRC.begin()}; it != mListDiagResp_CRC.end(); it++)
        {
            const android::sp<UdsMessage> crcUdsReq {new UdsMessage()};
            const android::sp<::Buffer> crcUdsReqBBuf{new ::Buffer()};
            const uint64_t crcKey{it->first};
            const vector<android::sp<UdsMessage>> listUdsCRC{it->second};
            for(uint32_t i{0U}; i < listUdsCRC.size(); i++)
            {
                if(i == 0U)
                {
                    if (listUdsCRC[i]->ToUdsData()->data() != nullptr)
                    {
                        const uint32_t dataSize{listUdsCRC[i]->ToUdsData()->size()};
                        if (dataSize <= static_cast<uint32_t>(INT32_MAX))
                        {
                            crcUdsReqBBuf->setTo(&listUdsCRC[i]->ToUdsData()->data()[0U], static_cast<int32_t>(dataSize));
                        } else {
                            LOG_E("Out of range");
                        }
                        (void)dataSize;
                    } else {
                        LOG_E("listUdsCRC[i]->ToUdsData()->data() is nullptr");
                    }
                } else {
                    if (listUdsCRC[i]->GetUdsPayload()->data() != nullptr)
                    {
                        const uint32_t dataSize{listUdsCRC[i]->GetUdsPayload()->size()};
                        const uint32_t tmpSize{crcUdsReqBBuf->size()};
                        if (tmpSize <= UINT32_MAX - dataSize)
                        {
                            if (dataSize <= static_cast<uint32_t>(INT32_MAX))
                            {
                                crcUdsReqBBuf->append(&listUdsCRC[i]->GetUdsPayload()->data()[0U], static_cast<int32_t>(dataSize));
                            } else {
                                LOG_E("Out of range");
                            }
                        } else {
                            LOG_E("Out of range");
                        }
                        (void)dataSize;
                        (void)tmpSize;
                    } else {
                        LOG_E("listUdsCRC[i]->GetUdsPayload()->data() is nullptr");
                    }
                }
            }
            (void)crcUdsReq->Parser(crcUdsReqBBuf);
            mDiagResp_CRC[crcKey] = crcUdsReq;
        }
        mListDiagResp_CRC.clear();
        const TransmissionInter transIt {mSsrTransListByID[mCurrentReqID].find(mCurrentTransmissionId)};
        if (transIt != mSsrTransListByID[mCurrentReqID].end()) {
            UploadSsrDataRequest ssrDataReq{};
            ssrDataReq.set_collection_condition_id(mColId);
            ssrDataReq.mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_SSR_DATA);
            ssrDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
            ssrDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
            ssrDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(CommonUtils::getGeodesyInfo());
            ssrDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
            ssrDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());

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
            uint32_t sizeOfFileCounter {0U};
            sizeOfFileCounter = ssrDataReq.ByteSizeLong();

            ssrDataReq.set_dtc(transIt->second->getDTC());
            RdgProtoInterface::DiagnosticsMessage tmpDiagMessage{};
            const vector<android::sp<UdsMessage>> listUdsData {mTransDiagResp[mCurrentTransmissionId]};
            if ((mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER)
                || (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER))
            {
                if(this->calculateCRC(mCurrentTransmissionId) == true) {
                    mIsNeedtoUploadSSRData = true;
                    /* generte upload data */
                    LOG_D("CRC Have change in response list");
                    /* TBD: Call to Uploader to make upload data*/
                }
                else {
                    LOG_D("CRC NO change in response list");
                    mIsNeedtoUploadSSRData = false;
                }
            } else {
                mIsNeedtoUploadSSRData = true;
            }
            for (const android::sp<UdsMessage>& udsData : listUdsData)
            {
                if ((udsData->ToUdsData()->size() == 0U) 
                || (udsData->getSID() == static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION)) 
                || (udsData->getSID() == static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL))) {
                    // RDG30-R-1168
                    // RDG30-R-1170
                    // RDG30-R-0251
                    RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                    tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_UNRESPONSIVE);
                    if (udsData->ToUdsData()->data() != nullptr)
                    {
                        tmpDiagMessage.set_user_data(udsData->ToUdsData()->data(), udsData->ToUdsData()->size());
                    } else {
                        LOG_E("ToUdsData()->data() is nullptr");
                    }
                    const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{transIt->second->getEcuInformation().getCommProtocol()};
                    ecuAddressInfo->set_communication_protocol(protocolType);
                    if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                    {
                        vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{transIt->second->getEcuInformation().getCommType()};
                        if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                            (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                        {
                            commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                        }
                        ecuAddressInfo->set_communication_type(commType);
                    }
                    ecuAddressInfo->set_target_address(transIt->second->getEcuInformation().getTargetAddress());
                }
                else if (udsData->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION)) 
                {
                    RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                    tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL);
                    const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{transIt->second->getEcuInformation().getCommProtocol()};
                    ecuAddressInfo->set_communication_protocol(protocolType);
                    if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                    {
                        vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{transIt->second->getEcuInformation().getCommType()};
                        if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                            (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                        {
                            commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                        }
                        ecuAddressInfo->set_communication_type(commType);
                    }
                    ecuAddressInfo->set_target_address(transIt->second->getRxAdd());
                    if (udsData->ToUdsData()->data() != nullptr)
                    {
                        tmpDiagMessage.set_user_data(udsData->ToUdsData()->data(), udsData->ToUdsData()->size());
                    } else {
                        LOG_E("ToUdsData()->data() is nullptr");
                    }
                } 
                else if (udsData->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE)) 
                {
                    RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                    tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL_WITH_NEGATIVE);
                    const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{transIt->second->getEcuInformation().getCommProtocol()};
                    ecuAddressInfo->set_communication_protocol(protocolType);
                    if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                    {
                        vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{transIt->second->getEcuInformation().getCommType()};
                        if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                            (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                        {
                            commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                        }
                        ecuAddressInfo->set_communication_type(commType);
                    }
                    ecuAddressInfo->set_target_address(transIt->second->getRxAdd());
                    if (udsData->ToUdsData()->data() != nullptr)
                    {
                        tmpDiagMessage.set_user_data(udsData->ToUdsData()->data(), udsData->ToUdsData()->size());
                    }  else {
                        LOG_E("udsData->ToUdsData()->data() is nullptr");
                    }
                } else {
                    // Do nothing
                }
                // RDG30-R-0265
                uint32_t tmpSum {0U};
                const uint32_t tmpDiagMessageSize {static_cast<uint32_t>(tmpDiagMessage.ByteSizeLong())};
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
                            const uint32_t sizeOfEcuAddressInformation {static_cast<uint32_t>(tmpDiagMessage.ecu_address_information().ByteSizeLong())};
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
                    const uint32_t targetAdr{transIt->second->getEcuInformation().getTargetAddress()};
                    const std::unordered_map<uint32_t, RefData>::iterator itRef{mResSID22ByID[mCurrentReqID].find(targetAdr)};
                    if (itRef != mResSID22ByID[mCurrentReqID].end())
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
            }

            if (mIsNeedtoUploadSSRData == true) {
                ssrDataReq.set_counter_value(UploadManager::getInstance()->getCounterValue());
                ssrDataReq.mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_SSR_DATA, UploadManager::getInstance()->getCounterMessage()));
                LOG_D("Send SSR upload data to Center:");
                /*Save UploadSSRDataRequest to file*/ 
                std::string str{};
                google::protobuf::util::JsonOptions option{};
                option.always_print_primitive_fields = true;
                option.preserve_proto_field_names = true;
                (void)google::protobuf::util::MessageToJsonString(ssrDataReq, &str, option);
                printData(str);
                const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
                const uint64_t uploadCount {UploadManager::getInstance()->genCountUpload()};
                std::string file_dir {std::to_string(uploadCount)};
                (void)file_dir.append("_UploadSsrDataRequest.dat");
                uint32_t fileSize{0U};
                error_t bSaved{E_ERROR};
                const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    bSaved = DataModel<UploadSsrDataRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG040, file_dir, ssrDataReq, fileSize);
                }
                else
                {
                    fileSize = ssrDataReq.ByteSizeLong();
                    bSaved = DataModel<UploadSsrDataRequest>::saveUpload(file_dir, ssrDataReq);
                }
                if (bSaved == E_OK)
                {
                    const uint8_t operation{CommonUtils::getOperation(mCurrentDiagTriggerType)};
                    DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
                    const android::sp<UploadTask> task {new UploadTask(uploadId)};
                    task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG040);
                    task->setUploadPatch(file_dir);
                    // task->setUploadId(uploadId);
                    /*Set priority*/
                    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(mCurrentReqID)};
                    if(it != mSaveReq.end()) {
                        task->setUploadPrio(it->second->getPriority());
                        task->setFileSize(static_cast<uint64_t>(fileSize));
                        UploadManager::getInstance()->requestUploadTask(task);  
                    } else {
                        LOG_D("Can not find triggerID in saved list");
                    }
                }
                else
                {
                    LOG_E("Save UploadSsrDataRequest file error");
                }
                
                (void)fileSize;
            }
            ssrDataReq.Clear();
            (void)sizeOfFileCounter;
        }
        mDiagResp_CRC.clear();
        if(mCRCManager != nullptr) {
            /*Caculate CRC*/
            mCRCManager->saveCRC16();
        } else {
            LOG_I("mCRCManager is null");
        }
    } else {
        LOG_D("Do not make SSR Upload data");
    }
    startNextTransmission();
}

void RemoteSSR::onFinishSSRAcquisition(const uint32_t& triggerId)
{
    /* Check if pTriggerId is in savedID then notify done to priorityControl*/
    LOG_I("Finish SSR Acquisition triggerID: %d", triggerId);
    LOG_I("SSR Done");
    mApp.notifyDiagDoneToLastUpload(triggerId);
    LOG_D("Check mSaveReq size before: %d", mSaveReq.size());
    for (const auto& it1 : mSaveReq ){
        LOG_V("triggerId: %d", it1.first);
    }
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(triggerId)};
    if(it != mSaveReq.end()) {
        if (triggerId <= static_cast<uint32_t>(INT32_MAX))
        {
            if ((isSsrAcquireAborted == false) && (mReqIDIsSuspended[mCurrentReqID] == false))
            {
                triggerToNextService(true);
            } else {
                LOG_I("SSR was aborted");
            }
            // const DiagTrigger::DiagTriggerType tempTriggerType {it->second->getType()};
            // if ( (tempTriggerType > DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) && (tempTriggerType < DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX))
            // {
            //     // PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(triggerId), tempTriggerType);
            // } else {
            //     LOG_E("it->second->getType() invalid");
            // }
            // (void)tempTriggerType;
        }
        else
        {
            LOG_E("triggerId is out of range int32_t");
        }
        (void)mSaveReq.erase(it);
        (void)mTransmissioIdListByID.erase(triggerId);
        (void)mSsrTransListByID.erase(triggerId);
        (void)mResSID22ByID.erase(triggerId);
        (void)mReqIDIsSuspended.erase(triggerId);
    } else {
        LOG_D("Can not find triggerId: %d in mSaveReq", triggerId);
    }
    mSsrTargetECUList.clear();
    mListDiagResp_CRC.clear();
    mDiagResp_CRC.clear();
    mTransDiagResp.clear();
    mIsSsrRunning = false;
    isSsrAcquireAborted = false;
    mIsEndOfAcquisition = false;
    isSsrAcquiredRefData = false;
    mIsNeedtoUploadSSRData = true;
    discardResponseCode = vccomif::rdg::v1::interfaces::ResponseCode_MIN;
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
