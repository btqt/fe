#include "RemoteRoB.h"
#include "Remotediag.h"
#include "CRCManager.h"
#include "utils/UploadTask.h"
#include "utils/Logger.h"
#include "utils/CommonUtils.h"
#include "utils/CollectionCondition.h"
#include <services/OnboardclientManagerAdapter.h>
#include <services/TimeManagerService/TimeManager.h>
#include <climits>

namespace rdgapp {

ANDROID_SINGLETON_STATIC_INSTANCE(rdgapp::RemoteRoB)


RemoteRoB::RemoteRoB()
    : android::RefBase()
    , RemoteDelegate()
    , mApp(nullptr)
    , mCurrentTransmissionId(0U)
    , mTriggerId(0)
    , mIsRobRunning(false)
    , mIsAcquisitionAbort(false)
    , mAcquisitionTime(0)
    , mMaxUploadFileSize(ROB_UPLOAD_DATA_SIZE_MAX)
{}

void RemoteRoB::MainHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
{
    const int32_t whatCmd{handlemsg->what};

    switch (whatCmd)
    {
    case CMD_ROB_START_UP:
    {
        mRoB.startUp();
        break;
    }
    case CMD_READ_SAVED_CRC:
    {
        mRoB.readCRC();
        break;
    }
    case CMD_TRIGGER_FROM_CENTER:
    {
        LOG_D("CMD_TRIGGER_FROM_CENTER");
        /* Get Diagnostic Acquisition Start Time*/
        // RDG30-R-0545
        // RDG30-R-1044
        // RDG30-R-1045
        // int64_t current_time{0};
        // current_time = MS_TO_SEC(TimeManager::getInstance().getCurrentMilliSec());
        // if ((current_time < static_cast<int64_t>(0x00)) || (current_time > static_cast<int64_t>(0x00000000FFFFFFFF)))
        // {

        //     LOG_D("Time information out of range");
        //     current_time = 0;
        // }
        // else
        // {
        //     LOG_D("Time information valid");
        // }
        /* Get location*/
        // RDG30-R-0052
        const android::sp<CommonDefine::RDGLocationData> loc{LocationManagerAdapter::getInstance()->getLocationData()};
        LOG_D("Save location information, 0x%08X 0x%08X", loc->getLatitude(), loc->getLongtitude());
        mRoB.mCurrentTriggerLocation = loc;

        /* Get priority from Center Request */
        //const uint32_t prio_tmp{static_cast<uint32_t>(msg->arg1)};
        uint32_t prio_tmp{0U};
        if(handlemsg->arg1 >= 0)
        {
            prio_tmp = static_cast<uint32_t>(handlemsg->arg1);
        }

        /* Get collection condition ID*/
        const android::sp<::Buffer> buf{new ::Buffer(handlemsg->buffer)};
        uint64_t colId{0U};
        if(buf->size()>0U)
        {
            if(buf->data() != nullptr){
                (void)memcpy(&colId,buf->data(),sizeof(colId));
            } else {
                LOG_D("Data is nullptr");
            }
        }
        LOG_D("Check Col ID: %llu", colId);
        mRoB.triggerAllRoB(DiagTrigger::DiagTriggerType::CENTER_TRIGGER, loc, prio_tmp, colId, 0);
        break;
    }
    case CMD_SIMULATE_WARNING_TRIGGER_EVENT:
    {
        LOG_D("CMD_SIMULATE_WARNING_TRIGGER_EVENT");
        /* Get Diagnostic Acquisition Start Time*/
        // RDG30-R-0545
        // RDG30-R-1044
        // RDG30-R-1045
        int64_t warningTriggerOccurrenceTime{0};
        warningTriggerOccurrenceTime = ParamsDef::getCurrentAcquisiteTime();
        if ((warningTriggerOccurrenceTime < static_cast<int64_t>(0x00)) || (warningTriggerOccurrenceTime > static_cast<int64_t>(0x00000000FFFFFFFF)))
        {
            /*RDG30-R-0063*/
            LOG_D("Time information out of range");
            warningTriggerOccurrenceTime = 0;
        }
        else
        {
            LOG_D("Time information valid");
        }
        /* Get location*/
        // RDG30-R-0052
        const android::sp<CommonDefine::RDGLocationData> loc{LocationManagerAdapter::getInstance()->getLocationData()};
        mRoB.mCurrentTriggerLocation = loc;

        /* Get priority from Center Request */
        uint32_t prio_tmp {200U};
        if (handlemsg->arg1 >= 0)
        {
            prio_tmp = static_cast<uint32_t>(handlemsg->arg1);
        }
        mRoB.triggerFromSSR(DiagTrigger::DiagTriggerType::WARNING_TRIGGER, warningTriggerOccurrenceTime, mRoB.mCurrentTriggerLocation, 123456U, prio_tmp);
        break;
    }
    case CMD_REQUEST_TO_PRIORITY_CONTROL:
    {
        LOG_I("CMD_REQUEST_TO_PRIORITY_CONTROL");
        android::sp<DiagTrigger> pTrigger{nullptr};
        handlemsg->getObject(pTrigger);
        LOG_I("Trigger Request have Type: %d Func: %d Prio: %d ID: %d ColID: %llu",
              pTrigger->getType(), pTrigger->getFunc(), pTrigger->getPriority(), pTrigger->getTriggerId(), pTrigger->getCollectionID());
        PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
        break;
    }
    case CMD_MAKE_UPLOAD_REQUEST:
    {
        android::sp<DiagTrigger> trigger{nullptr};
        handlemsg->getObject(trigger);
        mRoB.makeUploadRoBRequest(trigger);
        break;
    }
    case CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE:
    {
        mRoB.handleUnderRepairStatusChange(handlemsg->arg1, handlemsg->arg2);
        break;
    }
    case CMD_CHANGE_IG_STATUS:
    {
        mRoB.changeIGStatus((handlemsg->arg1 != 0) ? true : false);
        break;
    }
    case CMD_ROB_FINISH_TRANSMISSION:
    {
        LOG_I("CMD_ROB_FINISH_TRANSMISSION");
        mRoB.finishCurrentTransmission();
        break;
    }
    default:
        break;
    }
}

void RemoteRoB::onReceiveIG(const bool status) const
{
    LOG_I("onReceiveIG status = %d", status);
    (void)mHandler->obtainMessage(MainHandler::CMD_CHANGE_IG_STATUS, static_cast<int32_t>(status))->sendToTarget();
}

void RemoteRoB::changeIGStatus(const bool status) 
{
    android::sp<DiagTrigger> trigger{nullptr};
    error_t error {E_ERROR};
    error = getCurrentTrigger(trigger);
    if (error == E_OK) 
    {
        if ((status == false) && (mIsRobRunning == true) && (trigger != nullptr))
        {
            const DiagTrigger::DiagTriggerType type{trigger->getType()};
            if (type != DiagTrigger::DiagTriggerType::IGON_TRIGGER)
            {
                // RDG30-R-0342, RDG30-R-0693
                abortRobAcquisition(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION, trigger);

                // DID 330C
                if (type == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) {
                    DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::COLLECTION_CONDITIONS);
                } 
                else if (type == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
                {
                    DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::WARINING_TRIGGER);
                } else {
                    // Do nothing
                }
            }
        }
    }
}

void RemoteRoB::onChangedRemoteInfo(const int32_t what, const int32_t info)
{
    LOG_I("onChangedRemoteInfo");
    (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE, what, info)->sendToTarget();
}

void RemoteRoB::triggerFromSSR(const DiagTrigger::DiagTriggerType triggerType
        , const int64_t acquisitionStartTime
        , const android::sp<CommonDefine::RDGLocationData>& location
        , const uint64_t collectionId
        , const uint32_t priority
        , const int64_t warningOccurrenceTime = 0)
{
    mCurrentTriggerLocation = location;
    LOG_I("RoB notified with TriggerType: %d", triggerType);
    LOG_I("RoB notified with acquisition start time: %lld", acquisitionStartTime);
    LOG_I("RoB notified with Location: 0x%08X 0x%08X", location->getLatitude(), location->getLongtitude());
    LOG_I("RoB notified with Collection ID: %llu ", collectionId);
    LOG_I("RoB notified with priority: %d ", priority);
    /* Get trigger ID*/
    const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
    /* Create NewDiag Trigger*/
    const android::sp<DiagTrigger> pTrigger{new DiagTrigger(triggerType, priority, DiagTrigger::DiagTriggerFunc::ROB, nextTriggerId)};
    /* set acquisition Start time*/
    pTrigger->setTriggerTime(acquisitionStartTime);
    pTrigger->setWarningTriggerTime(warningOccurrenceTime);
    pTrigger->setLatitude(location->getLatitude());
    pTrigger->setLongitude(location->getLongtitude());
    
    /* Set collection id*/
    pTrigger->setCollectionId(collectionId);
    /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
    /* Save request to local*/
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret{mTriggerList.emplace(nextTriggerId, pTrigger)};
    LOG_D("Check mTriggerList size: %d", mTriggerList.size());
    if (!ret.second)
    {
        ret.first->second = pTrigger;
    }
    LOG_D("Check saved RoB trigger ID: %d", ret.first->first);
}

void RemoteRoB::triggerAllRoB(const DiagTrigger::DiagTriggerType type, const android::sp<CommonDefine::RDGLocationData> location, const uint32_t prio, const uint64_t colId, const int64_t time)
{
    LOG_D("All RoB trigger");
    /* TBD: check ppi flag*/
    /* Get trigger ID*/
    const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
    /* Create NewDiag Trigger*/
    const android::sp<DiagTrigger> pTrigger{new DiagTrigger(type, prio, DiagTrigger::DiagTriggerFunc::ALLROB, nextTriggerId)};
    /* set trigger time*/
    pTrigger->setTriggerTime(time);
    LOG_D("Check RoB trigger time: %lld sec", time);
    /* Set collection id*/
    pTrigger->setCollectionId(colId);
    LOG_D("Check RoB Collection ID: %llu ", colId);

    pTrigger->setLatitude(location->getLatitude());
    pTrigger->setLongitude(location->getLongtitude());

    /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
    /* Save request to local*/
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret{mTriggerList.emplace(nextTriggerId, pTrigger)};
    LOG_D("Check mTriggerList size: %d", mTriggerList.size());
    if (!ret.second)
    {
        ret.first->second = pTrigger;
    }
    LOG_D("Check saved RoB trigger ID: %d", ret.first->first);
}

bool RemoteRoB::notifyTrigger(const DiagTrigger::DiagTriggerState &pState,
                              const int32_t &pTriggerId, const bool dueToIgOff)
{
    bool isRobTrigger{false};
    /* Check if trigger is in the saved trigger*/
    android::sp<DiagTrigger> trigger{nullptr};
    error_t error{E_ERROR};
    if(pTriggerId>=0)
    {
        error = getDiagTrigger(static_cast<uint32_t>(pTriggerId), trigger);
    }
    else
    {
        //Nothing
    }
    if ((error == E_OK) && (trigger != nullptr))
    {
        LOG_I("notify All RoB Trigger state: %d TriggerID: %d TriggerTime: %lld", pState, pTriggerId, trigger->getTriggerTime());
        isRobTrigger = true;
        handleTrigger(pState, pTriggerId, dueToIgOff);
    }
    else
    {
        LOG_I("Can not find triggerID in saved list");
        isRobTrigger = false;
        PriorityControl::getInstance()->notifyTriggerNoFound(pTriggerId);
    }
    return isRobTrigger;
}

void RemoteRoB::finishRoBAcquisition(const int32_t &triggerId)
{
    mTransmissioIdList.clear();

    mCrcCheck.clear();

    mRobTransList.clear();

    mDiagResp.clear();

    OnboardclientAdapter::getInstance()->ReleaseObcResource();
    /* Check if pTriggerId is in savedID then notify done to priorityControl*/
    LOG_I("Finish RoB Acquisition triggerID: %d", triggerId);
    LOG_D("Check mTriggerList size before: %d", mTriggerList.size());
    if(triggerId>=0)
    {
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mTriggerList.find(static_cast<uint32_t>(triggerId))};
        if (it != mTriggerList.end())
        {
            const DiagTrigger::DiagTriggerType temp_type {it->second->getType()};
            if ((temp_type < DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX) &&  (temp_type > DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN))
            {
                PriorityControl::getInstance()->notifyTriggerProcessDone(triggerId, temp_type);
            }
            (void)mTriggerList.erase(it);
        }
        else
        {
            LOG_D("Can not find triggerId: %d in mTriggerList", triggerId);
        }
        LOG_D("Check mTriggerList size after: %d", mTriggerList.size());
    }
    else{}
}

void RemoteRoB::handleTrigger(const DiagTrigger::DiagTriggerState &pState,
                              const int32_t &pTriggerId, const bool dueToIgOff)
{
    (void)dueToIgOff;
    android::sp<DiagTrigger> trigger{nullptr};
    error_t error{E_ERROR};
    if(pTriggerId>=0)
    {
        error = getDiagTrigger(static_cast<uint32_t>(pTriggerId), trigger);
    }
    else
    {
        //Nothing
    }
    if ((error == E_OK) && (trigger != nullptr))
    {

        LOG_I("notify RoB Trigger state: %d TriggerID: %d", pState, pTriggerId);
        switch (pState)
        {
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
            LOG_I("TRIGGER_PROCESSING");

            mTriggerId = pTriggerId;
            (void)mHandler->obtainMessage(MainHandler::CMD_ROB_START_UP)->sendToTarget();
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED:
        {
            LOG_I("TRIGGER_SUSPENDED");
            if (mIsRobRunning == true)
            { // RDG30-R-0411
                suspendRobAcquisition();
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
        {
            LOG_I("TRIGGER_DISCARDED");
            const DiagTrigger::DiagTriggerType type{trigger->getType()};
            if ((type != DiagTrigger::DiagTriggerType::IGON_TRIGGER) && (type != DiagTrigger::DiagTriggerType::WARNING_TRIGGER))
            {
                // Release OBC resource
                if (mIsRobRunning) {
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                }
                // RDG30-R-0693
                // RDG30-R-0407
                // RDG30-R-0694
                abortRobAcquisition(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED, trigger);
                DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::RD_SCHEDULE_TRIGGER);
            }
            else if (type == DiagTrigger::DiagTriggerType::IGON_TRIGGER)
            {
                // RDG30-R-0754
                LOG_D("IGON_TRIGGER -> error upload data shall not be created");
            }
            else if(type == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
            {
                // RDG30-R-0755
                LOG_D("WARNING_TRIGGER -> error upload data shall not be created");
            } else {
                // do nothing
            }
            const DiagTrigger::DiagTriggerType temp_type {trigger->getType()};
            if ((temp_type < DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX) && (temp_type > DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN))
            {
                PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, temp_type);
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DONE:
        {
            LOG_I("TRIGGER_DONE");
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_STATE_MAX:
        {
            LOG_I("TRIGGER_STATE_MAX");
            break;
        }
        }
    }
}

void RemoteRoB::handleUnderRepairStatusChange(const int32_t what, const int32_t status)
{
    LOG_D("handleUnderRepairStatusChange");
    switch (what)
    {
    case WHAT_CHANGED_REPAIR_SATUS:
    {
        android::sp<DiagTrigger> trigger{nullptr};
        error_t error{E_ERROR};
        error = getCurrentTrigger(trigger);
        if ((mIsRobRunning == true) && (error == E_OK) && (trigger != nullptr))
        {
            const DiagTrigger::DiagTriggerType type{trigger->getType()};
            if (type != DiagTrigger::DiagTriggerType::IGON_TRIGGER)
            {
                // RDG30-R-0693, RDG30-R-1175
                abortRobAcquisition(RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR, trigger);
            }
        }
        (void)error;
        break;
    }
    default:
    {
        break;
    }
    }
    (void)status;
}

void RemoteRoB::onCenterCommandForward(const android::sp<CenterReqData> &pCenterReqData)
{
    LOG_I("RoB receive Center request");
    /*TBD: Process center data*/
    const uint32_t prio_data{pCenterReqData->getCenterReq_prio()};
    /*TBD: get warning trigger occurence*/
    uint64_t colID{0U};
    colID = pCenterReqData->getCenterReq_CollectionID();
    //const uint8_t *const colId_ptr{reinterpret_cast<const uint8_t *>(&colID)};
    uint8_t colId_ptr[sizeof(colID)];
    (void)memcpy(&colId_ptr[0], &colID, sizeof(colID));
    const android::sp<::Buffer> colId_sp{new ::Buffer()};
    colId_sp->setTo(&colId_ptr[0], sizeof(colID));
    //const sp<sl::Message> msg{mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_CENTER, static_cast<int32_t>(prio_data))};
    //msg->buffer.setTo(colId_sp->data(), static_cast<int32_t>(colId_sp->size()));
    if(prio_data < static_cast<uint32_t>(INT32_MAX))
    {
        sp<sl::Message> msg{};
        msg =  mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_CENTER, static_cast<int32_t>(prio_data));
        const uint32_t tmp{colId_sp->size()};
        if(tmp < static_cast<uint32_t>(INT32_MAX))
        {
            msg->buffer.setTo(colId_sp->data(), static_cast<int32_t>(tmp));
        }
        else
        {
            LOG_D("Error codeing");
        }

        (void)msg->sendToTarget();
    }
    else
    {
        LOG_D("Error codeing");
    }
}

void RemoteRoB::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
    if (this->mIsRobRunning == true)
    {
        const uint8_t responseCode{responseEventInfo->errCode()};
        const uint8_t protocolType{responseEventInfo->resInfo()->protocolType()};
        const uint32_t canIdRx {responseEventInfo->resInfo()->canInfo()->canId()};
        const uint32_t targetAddress{CommonUtils::calTargetAddressFromCanIdRx(protocolType, canIdRx)};
        if (mIsAcquisitionAbort == false) {
            if (mCurrentTransmission == nullptr) {
                LOG_E("An Error occurenced, mCurrentTransmission is null");
            } else {
                const uint16_t connectId {responseEventInfo->resInfo()->connectId()};
                if (mCurrentTransmission->getConnectId() == connectId)
                {
                    const std::unordered_map<uint32_t, uint64_t>::iterator phase6It{mEcuDiagPhase6List.find(targetAddress)};
                    if (phase6It != mEcuDiagPhase6List.end())
                    {
                        LOG_D("ECU phase 6 detected");
                        handleUdsResponsePhase6(targetAddress, canIdRx, connectId, responseCode, udsResponse);
                    } else {
                        handleUdsResponsePhase5(targetAddress, canIdRx, connectId, responseCode, udsResponse);
                    }
                }
            }
        } else {
            LOG_D("Received ECU response while RoB is suspending => stop RoB acquisition process");
            stopRobAcquisition();
        }
        (void)canIdRx;
        (void)targetAddress;
        (void)responseCode;

    } else {
        LOG_D("RoB acquisition isn't running");
    }
    return;
}

void RemoteRoB::handleUdsResponsePhase5(const uint32_t targetAddress, const uint32_t canIdRx, const uint16_t connectId, const uint8_t responseCode, const android::sp<UdsMessage> udsResponse)
{
    switch(udsResponse->getSID()) {
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_RESPONSE_READ_ROB_INFORMATION):
        {
            uint64_t transmissionId{0U};
            transmissionId |= (0xFFFFFFFFFFFFFFFF & targetAddress) << 8U;
            transmissionId |= (0xFFFFFFFFFFFFFFFF & udsResponse->getSFID());

            const TransmissionInter it{mRobTransList.find(transmissionId)};
            LOG_I("Finding transmissionId = 0x%02llx", transmissionId);
            if ((it != mRobTransList.end()) && (it->second->getState() == RobUdsTransmission::State::ROB_TRANS_SEND_UDS))
            {
                LOG_D("ECU phase 5 detected");
                mDiagResp.push_back({transmissionId, udsResponse});
                LOG_D("Calulating CRC of UDS user data of 0x%02X", targetAddress);
                mCrcCheck[transmissionId] = udsResponse;
                it->second->setCanIdRx(canIdRx);

                if (transmissionId == mCurrentTransmissionId)
                {
                    it->second->stopTimeout();
                    it->second->disconnect();
                    it->second->setState(RobUdsTransmission::State::ROB_TRANS_DISCONNECT);
                    finishCurrentTransmission();
                }
            } else {
                LOG_I("transmissionId = 0x%02llx not found", transmissionId);
            }
            
            break;
        }
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE):
        {
            const TransmissionInter it{mRobTransList.find(mCurrentTransmissionId)};
            if ((it != mRobTransList.end()) && (it->second->getState() == RobUdsTransmission::State::ROB_TRANS_SEND_UDS))
            {
                if (it->second->getConnectId() == connectId)
                {
                    it->second->setCanIdRx(canIdRx);
                    it->second->stopTimeout();
                    it->second->disconnect();
                    mDiagResp.push_back({mCurrentTransmissionId, udsResponse});
                    mCrcCheck[mCurrentTransmissionId] = udsResponse;
                    it->second->setState(RobUdsTransmission::State::ROB_TRANS_DISCONNECT);
                    finishCurrentTransmission();
                }
            }
            break;
        }
        default:
        {
            // RDG30-R-1176
            mCurrentTransmission->stopTimeout();
            mCurrentTransmission->disconnect();
            mDiagResp.push_back({mCurrentTransmissionId, nullptr});
            mCurrentTransmission->setState(RobUdsTransmission::State::ROB_TRANS_DISCONNECT);
            finishCurrentTransmission();
            switch(responseCode) {
                case static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_TIMEOUT):
                {
                    LOG_D("Received no response from OBC, targetAddress = 0x%02X.", targetAddress);
                    break;
                }
                case static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_FAILED):
                {
                    LOG_D("Received upon failure to send a request message from OBC, targetAddress = 0x%02X.", targetAddress);
                    break;
                }
                default:
                {
                    LOG_D("Received response from OBC, responseCode = %d", responseCode);
                    break;
                }
            }
            break;
        }
    }
}
void RemoteRoB::handleUdsResponsePhase6(const uint32_t targetAddress, const uint32_t canIdRx, const uint16_t connectId, const uint8_t responseCode, const android::sp<UdsMessage> udsResponse)
{
    switch(udsResponse->getSID()) {
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION):
        {
            uint64_t transmissionId{0U};
            // check if UDS message send by ECU diag phase 6
            if(udsResponse->GetUdsPayload()->data() != nullptr) {
                const uint8_t memorySelection{udsResponse->GetUdsPayload()->data()[0U]};
                transmissionId |= (0xFFFFFFFFFFFFFFFFU & targetAddress) << 16U;
                transmissionId |= (0xFFFFFFFFFFFFFFFFU & udsResponse->getSFID()) << 8U;
                transmissionId |= (0xFFFFFFFFFFFFFFFFU & memorySelection);
            } else {
                LOG_D("UDS data is null");
            }

            const TransmissionInter it{mRobTransList.find(transmissionId)};
            LOG_I("Finding transmissionId = 0x%02llx", transmissionId);
            if ((it != mRobTransList.end()) && (it->second->getState() == RobUdsTransmission::State::ROB_TRANS_SEND_UDS))
            {
                mDiagResp.push_back({transmissionId, udsResponse});
                LOG_D("Calulating CRC of UDS user data of 0x%02X", targetAddress);
                mCrcCheck[transmissionId] = udsResponse;
                it->second->setCanIdRx(canIdRx);

                if (transmissionId == mCurrentTransmissionId)
                {
                    it->second->stopTimeout();
                    it->second->disconnect();
                    it->second->setState(RobUdsTransmission::State::ROB_TRANS_DISCONNECT);
                    finishCurrentTransmission();
                }
            } else {
                LOG_I("transmissionId = 0x%02llx not found", transmissionId);
            }
            
            break;
        }
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE):
        {
            const TransmissionInter it{mRobTransList.find(mCurrentTransmissionId)};
            if ((it != mRobTransList.end()) && (it->second->getState() == RobUdsTransmission::State::ROB_TRANS_SEND_UDS))
            {
                if (it->second->getConnectId() == connectId)
                {
                    it->second->setCanIdRx(canIdRx);
                    it->second->stopTimeout();
                    it->second->disconnect();
                    mDiagResp.push_back({mCurrentTransmissionId, udsResponse});
                    mCrcCheck[mCurrentTransmissionId] = udsResponse;
                    it->second->setState(RobUdsTransmission::State::ROB_TRANS_DISCONNECT);
                    finishCurrentTransmission();
                }
            }
            break;
        }
        default:
        {
            // RDG30-R-1176
            mCurrentTransmission->stopTimeout();
            mCurrentTransmission->disconnect();
            mDiagResp.push_back({mCurrentTransmissionId, nullptr});
            mCurrentTransmission->setState(RobUdsTransmission::State::ROB_TRANS_DISCONNECT);
            finishCurrentTransmission();
            switch(responseCode) {
                case static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_TIMEOUT):
                {
                    LOG_D("Received no response from OBC, targetAddress = 0x%02X.", targetAddress);
                    break;
                }
                case static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_FAILED):
                {
                    LOG_D("Received upon failure to send a request message from OBC, targetAddress = 0x%02X.", targetAddress);
                    break;
                }
                default:
                {
                    LOG_D("Received response from OBC, responseCode = %d", responseCode);
                    break;
                }
            }
            break;
        }
    }
}

void RemoteRoB::init(const Remotediag* const app, android::sp<sl::SLLooper>& privateLooper)
{
    LOG_I("Init success");
    mApp = app;
    mHandler = new MainHandler(privateLooper, *this);
    mCRCManager = new CRCManager(*this);
    (void)mHandler->obtainMessage(MainHandler::CMD_READ_SAVED_CRC)->sendToTarget();
}

void RemoteRoB::startUp()
{
    LOG_I("startup");
    bool isAbort{true};
    bool robFlag{false};

    const std::shared_ptr<CollectionConditionDiagCommon> diagCommon{CollectionCondition::getInstance().getCollectionConditionDiagCommon()};
    if (diagCommon != nullptr)
    {
        robFlag = diagCommon->rob_flag();
    }

    // check precondtion
    android::sp<DiagTrigger> trigger{nullptr};
    error_t error{E_ERROR};
    error = getCurrentTrigger(trigger);
    if ((error == E_OK) && (trigger != nullptr))
    {
        const DiagTrigger::DiagTriggerType triggerType{trigger->getType()};
        if (triggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
        {
            // RDG30-R-0050
            // RDG30-R-0729
            if (DiagManagerAdapter::getInstance()->getRDGFlag() == 0U)
            {
                abortRobAcquisition(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_UNPROVIDED_VEHICLE, trigger);
                
                LOG_W("Abort AllRoB center request because Rdg active flag is OFF");
                DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::CORRUPT_DATA_ACQUISITION);
            }
            else if (PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_OFF)
            {
                abortRobAcquisition(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION, trigger);
                LOG_W("Abort AllRoB center request because IG status is OFF");
            }
            else if (DiagManagerAdapter::getInstance()->getAllUploadConsent() == false)
            {
                abortRobAcquisition(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_UNPROVIDED_VEHICLE, trigger);
                LOG_W("Abort AllRoB center request because All data upload consent status isn't 10b");
                DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::CORRUPT_DATA_ACQUISITION);
            }
            else if (mApp->getUnderRepair() == 1U)
            {
                abortRobAcquisition(RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR, trigger);
                LOG_W("Abort AllRoB center request because under repair status is on");
                DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::CORRUPT_DATA_ACQUISITION);
            }
            else
            {
                LOG_D("All conditions are met -> Exec AllRoB");
                mAcquisitionTime = ParamsDef::getCurrentAcquisiteTime();
                if ((mAcquisitionTime < static_cast<int64_t>(0x00)) || (mAcquisitionTime > static_cast<int64_t>(0x00000000FFFFFFFF)))
                {
                    /*RDG30-R-0063*/
                    LOG_D("Time information out of range -> set as undefine");
                    mAcquisitionTime = 0;
                }
                else
                {
                    LOG_D("Time information valid");
                }
                LOG_D("Set Diagnostic Acquisition Start Time %lld", mAcquisitionTime);
                isAbort = false;
            }
        }
        else if ((robFlag == true) && ((triggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) || (triggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)))
        {
            isAbort = false;
            mAcquisitionTime = trigger->getTriggerTime();
        }
        else
        {
            // Do nothing
        }

        if (isAbort == false)
        {
            LOG_D("Execulting RoB Acquistion");
            // Get OBC resource
            const OBCResourceEventCode resEventInfo{OnboardclientAdapter::getInstance()->GetObcResource()};
            if (resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK)
            {
                LOG_E("GetObcResource OBC_GET_RESOURCE_WAIT -> wait and check after");

                const android::sp<sl::Message> msg{mHandler->obtainMessage(MainHandler::CMD_ROB_START_UP)};
                (void)mHandler->sendMessageDelayed(msg, 5000U);
            }
            else
            {
                LOG_D("Get obc resource success");
                // Lock OBC resource
                OnboardclientAdapter::getInstance()->TakeObcResource();
                this->mIsRobRunning = true;

                std::list<CommonDefine::EcuInformation> ecuInformationList{};
                RemoteEcuInformation::getInstance()->getEcuInformationList(ecuInformationList);
                LOG_D("Check ecuInformationList size = %d", ecuInformationList.size());
                std::list<CommonDefine::EcuInformation>::iterator it{ecuInformationList.begin()};

                mRobTransList.clear();
                mEcuDiagPhase6List.clear();
                // Clear Queue
                mTransmissioIdList.clear();

                while (it != ecuInformationList.end())
                {
                    LOG_I("Check ecuInformationList target_address = 0x%02X", it->getTargetAddress());


                    if (it->getecuActiveFlag() == true)
                    {
                        uint8_t sid{static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION)};
                        constexpr uint8_t sfid{static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_17_REPORT_USER_DEF_MEMORY_DTC_BY_STATUS_MASK)};
                        // RDG30-R-0972: The DTCStatusMask of the request message is set to 08h.[Phase6]
                        if (it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6)
                        {
                            constexpr uint8_t dtcStatusMask {0x08U};
                            LOG_D("TargetAddress: 0x%02X, Phase6, Add dtcStatusMask = 0x%x",it->getTargetAddress(),  dtcStatusMask);
                            // RDG30-R-0756
                            const android::sp<RobUdsTransmission> transmission1{new RobUdsTransmission(*this, *it, sid, sfid, static_cast<uint8_t>(UDS_MEMORY_SELECTION::OccurrenceDTC), dtcStatusMask)};
                            LOG_D("TargetAddress: 0x%02X, Phase6, Add MemorySelection OccurrenceDTC success", it->getTargetAddress());
                            const android::sp<RobUdsTransmission> transmission2{new RobUdsTransmission(*this, *it, sid, sfid, static_cast<uint8_t>(UDS_MEMORY_SELECTION::MaintenanceDTC), dtcStatusMask)};
                            LOG_D("TargetAddress: 0x%02X, Phase6, Add MemorySelection MaintenanceDTC success", it->getTargetAddress());
                            const android::sp<RobUdsTransmission> transmission3{new RobUdsTransmission(*this, *it, sid, sfid, static_cast<uint8_t>(UDS_MEMORY_SELECTION::SystemOperationDTC), dtcStatusMask)};
                            LOG_D("TargetAddress: 0x%02X, Phase6, Add MemorySelection SystemOperationDTC success", it->getTargetAddress());
                            const android::sp<RobUdsTransmission> transmission4{new RobUdsTransmission(*this, *it, sid, sfid, static_cast<uint8_t>(UDS_MEMORY_SELECTION::SecurityEventDTC), dtcStatusMask)};
                            LOG_D("TargetAddress: 0x%02X, Phase6, Add MemorySelection SecurityEventDTC success", it->getTargetAddress());

                            mRobTransList[transmission1->getTransmissionId()] = transmission1;
                            LOG_D("Adding transmissionId1 = 0x%02llx", transmission1->getTransmissionId());
                            mTransmissioIdList.push_back(transmission1->getTransmissionId());

                            mRobTransList[transmission2->getTransmissionId()] = transmission2;
                            mTransmissioIdList.push_back(transmission2->getTransmissionId());

                            mRobTransList[transmission3->getTransmissionId()] = transmission3;
                            mTransmissioIdList.push_back(transmission3->getTransmissionId());

                            mRobTransList[transmission4->getTransmissionId()] = transmission4;
                            mTransmissioIdList.push_back(transmission4->getTransmissionId());

                            const std::unordered_map<uint32_t, uint64_t>::iterator phase6It{mEcuDiagPhase6List.find(it->getTargetAddress())};

                            if (phase6It == mEcuDiagPhase6List.end())
                            {
                                mEcuDiagPhase6List[it->getTargetAddress()] = transmission1->getTransmissionId();
                            }

                            it++;
                            (void)sid;
                            (void)sfid;
                        }
                        else if (it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5)
                        {
                            sid = static_cast<uint8_t>(UDS_SID::SID_AB_READ_ROB_INFORMATION);
                            // RDG30-R-1194
                            const android::sp<RobUdsTransmission> transmission1{new RobUdsTransmission(*this, *it, sid, 0x01U)};
                            LOG_D("TargetAddress: 0x%02X, Phase5, Add subfunction = 0x01 success", it->getTargetAddress());
                            const android::sp<RobUdsTransmission> transmission2{new RobUdsTransmission(*this, *it, sid, 0x11U)};
                            LOG_D("TargetAddress: 0x%02X, Phase5, Add subfunction = 0x11 success", it->getTargetAddress());
                            const android::sp<RobUdsTransmission> transmission3{new RobUdsTransmission(*this, *it, sid, 0x21U)};
                            LOG_D("TargetAddress: 0x%02X, Phase5, Add subfunction = 0x21 success", it->getTargetAddress());
                            const android::sp<RobUdsTransmission> transmission4{new RobUdsTransmission(*this, *it, sid, 0x31U)};
                            LOG_D("TargetAddress: 0x%02X, Phase5, Add subfunction = 0x31 success", it->getTargetAddress());

                            mRobTransList[transmission1->getTransmissionId()] = transmission1;
                            mTransmissioIdList.push_back(transmission1->getTransmissionId());

                            mRobTransList[transmission2->getTransmissionId()] = transmission2;
                            mTransmissioIdList.push_back(transmission2->getTransmissionId());

                            mRobTransList[transmission3->getTransmissionId()] = transmission3;
                            mTransmissioIdList.push_back(transmission3->getTransmissionId());

                            mRobTransList[transmission4->getTransmissionId()] = transmission4;
                            mTransmissioIdList.push_back(transmission4->getTransmissionId());

                            it++;
                            (void)sid;
                            (void)sfid;
                            //continue;
                        }
                        else
                        {
                            LOG_D("RoB function doesn't support phase 4");
                            it++;
                            (void)sid;
                            (void)sfid;
                        }
                    }
                    else
                    {
                        it++;
                    }
                }

                LOG_D("Finish generation RoB request message");
                LOG_D("mRobTransList size = %d, mEcuDiagPhase6List size = %d", mTransmissioIdList.size(), mEcuDiagPhase6List.size());
                if (mTransmissioIdList.size() > 0U)
                {
                    mCurrentTransmissionId = mTransmissioIdList.front();
                    const TransmissionInter itTrans{mRobTransList.find(mCurrentTransmissionId)};
                    if (itTrans != mRobTransList.end())
                    {
                        mCurrentTransmission = itTrans->second;
                        LOG_D("Start RoB acquisition sequence");
                        itTrans->second->connect();
                    }
                }
                else
                {
                    DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
                    const uint32_t tmp{trigger->getTriggerId()};
                    if(tmp<static_cast<uint32_t>(INT32_MAX))
                    {
                        finishRoBAcquisition(static_cast<int32_t>(tmp));
                    }
                    else
                    {
                        LOG_I("Error coding");
                    }
                    
                    // Release OBC resource
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                }
            }
        }
    }
    (void)isAbort;
    (void)robFlag;
}

RemoteRoB::RobUdsTransmission::RobUdsTransmission(
        RemoteRoB &rob
        , const CommonDefine::EcuInformation &mecuInformation
        , const uint8_t aSID
        , const uint8_t aSFID
        , const uint8_t aMemorySelection
        , const uint8_t aDtcStatusMask)
    : android::RefBase()
    , connectId(0U)
    , transmissionId(0U)
    , canIdRx(0U)
    , mstate(State::ROB_TRANS_INIT)
    , mRoB(rob)
    , mTimerHandler(rob)
    , mTimeOut(&mTimerHandler, TimerHandler::ID_TRANSMISSION_TIMEOUT)
    , udsReq(aSID, aSFID, aDtcStatusMask, aMemorySelection)

{
    mTimeOut.setDuration(TRANSMISSION_TIME_OUT_DURATION, 0U);
    this->ecuInformation.setCanId(mecuInformation.getCanId());
    this->ecuInformation.setCommProtocol(mecuInformation.getCommProtocol());
    this->ecuInformation.setCommType(mecuInformation.getCommType());
    const CommonDefine::DiagPhase diagPhase{mecuInformation.getDiagPhase()};
    if((diagPhase >= CommonDefine::DiagPhase::DP_UNKNOW) && (diagPhase <= CommonDefine::DiagPhase::DP_PHASE_6))
    {
        this->ecuInformation.setDiagPhase(diagPhase);
    } else {
        LOG_E("Unknown diag phase");
    }
    this->ecuInformation.setecuActiveFlag(mecuInformation.getecuActiveFlag() == true ? true : false);
    this->ecuInformation.setTargetAddress(mecuInformation.getTargetAddress());

    if (this->ecuInformation.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6)
    {
        transmissionId |= (0xFFFFFFFFFFFFFFFFU & this->ecuInformation.getTargetAddress()) << 16U;
        transmissionId |= (0xFFFFFFFFFFFFFFFFU & this->udsReq.getSFID()) << 8U;
        transmissionId |= (0xFFFFFFFFFFFFFFFFU & aMemorySelection);
    }
    else
    {
        transmissionId |= (0xFFFFFFFFFFFFFFFFU & this->ecuInformation.getTargetAddress()) << 8U;
        transmissionId |= (0xFFFFFFFFFFFFFFFFU & this->udsReq.getSFID());
    }
}

void RemoteRoB::RobUdsTransmission::connect()
{
    LOG_I("Start connect, transmissionID = 0x%02llx", this->transmissionId);
    this->mstate = RobUdsTransmission::State::ROB_TRANS_CONNECT;

    const android::sp<OBCTransportInfo> obcTransportInfo{new OBCTransportInfo()};
    OBCCanInfo canInfo{};
    std::vector<std::string> ntaArray{};
    canInfo.setData(this->ecuInformation.getTargetAddress(), ntaArray);
    ntaArray.push_back("\0");
    obcTransportInfo->setData(static_cast<uint8_t>(RemoteEcuInformation::getInstance()->convertObcProtocolType(this->ecuInformation.getCommProtocol(), this->ecuInformation.getCommType(), this->ecuInformation.getTargetAddress())), canInfo, false, 0U);
    const android::sp<OBCConnectInfo> obj{new OBCConnectInfo()};
    (void)OnboardclientAdapter::getInstance()->connect(obcTransportInfo, APP_NAME, obj);

    tOBCConnectInfo info;
    obj->setDataFormat(info);
    if (info.response != OBCEnum::OBCErrCode::OBC_OK)
    {
        LOG_E("can't connect to target_address = 0x%02X", this->ecuInformation.getTargetAddress());
        mRoB.mDiagResp.push_back({this->transmissionId, nullptr});
        (void)mRoB.mHandler->obtainMessage(MainHandler::CMD_ROB_FINISH_TRANSMISSION)->sendToTarget();
    }
    else
    {
        LOG_D("connect success transmissionID = 0x%02llx", this->transmissionId);
        this->connectId = info.connectId;
        this->send();
    }
}

void RemoteRoB::RobUdsTransmission::stopTimeout()
{
    LOG_D("stopTimeout, transmissionId = 0x%02llx", this->transmissionId);
    this->mTimeOut.stop();
}

void RemoteRoB::RobUdsTransmission::disconnect()
{
    LOG_D("disconnect, transmissionId = 0x%02llx", this->transmissionId);
    (void)OnboardclientAdapter::getInstance()->disconnectECU(this->connectId);
    mstate = RobUdsTransmission::State::ROB_TRANS_DONE;
}

void RemoteRoB::RobUdsTransmission::send()
{
    const android::sp<::Buffer> usdData{this->udsReq.ToUdsData()};
    const uint8_t ret {OnboardclientAdapter::getInstance()->sendUdsData(this->connectId, usdData)};

    if (ret != static_cast<uint8_t>(OBCEnum::OBC_OK))
    {
        LOG_E("SendUdsData for transmissionId 0x%02llx error = %d -> Disconnect", this->transmissionId, ret);
        mRoB.mDiagResp.push_back({this->transmissionId, nullptr});
        this->disconnect();
        mRoB.finishCurrentTransmission();
    }
    else
    {
        LOG_D("SendUdsData for transmissionId 0x%02llx success", this->transmissionId);
        this->mstate = RobUdsTransmission::State::ROB_TRANS_SEND_UDS;
        mTimeOut.start();
        LOG_D("start timeout for transmissionId = 0x%02llx", this->transmissionId);
    }
}

bool RemoteRoB::calculateCRC()
{
    bool isDifferent{false};
    if (mCRCManager != nullptr)
    {
        /*TBD: caculate CRC*/
        mCRCManager->calculateCRC16Data();
        isDifferent = mCRCManager->compareCrc16Value();
    }
    else
    {
        isDifferent = true;
    }
    return isDifferent;
}

void RemoteRoB::readCRC()
{
    /*TBD: read dtc flag*/
    if (mCRCManager == nullptr)
    {
        mCRCManager = new CRCManager(*this);
    }
    mCRCManager->readCRC16FromFile();
}

void RemoteRoB::finishCurrentTransmission(void)
{
    if (mIsRobRunning == true)
    {
        LOG_D("finish Current transmission, mTransmissioIdList size = %d", mTransmissioIdList.size());
        if (mTransmissioIdList.size() > 1U)
        {
            mTransmissioIdList.pop_front();
            LOG_D("finish Current transId = 0x%02llx", mCurrentTransmissionId);
            mCurrentTransmissionId = mTransmissioIdList.front();
            const TransmissionInter it{mRobTransList.find(mCurrentTransmissionId)};
            if (it != mRobTransList.end())
            {
                mCurrentTransmission = it->second;
                it->second->connect();
            }
        }
        else
        {
            if (mTransmissioIdList.empty() != true)
            {
                mTransmissioIdList.pop_front();
            }
            mIsRobRunning = false;
            // Release OBC resource
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            android::sp<DiagTrigger> trigger{nullptr};
            error_t error {E_ERROR};
            error = getCurrentTrigger(trigger);
            if ((error == E_OK) && (trigger != nullptr))
            {
                (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST, trigger)->sendToTarget();
            }
        }
    }
}

void RemoteRoB::onTransmissionTimeout(void)
{
    LOG_D("Event Transmission Timeout, currentTrans = 0x%02llx", mCurrentTransmissionId);
    if (mIsRobRunning) 
    {
        if (mIsAcquisitionAbort)
        {
            LOG_D("Timeout expired while RoB is suspending => stop RoB acquisition process");
            stopRobAcquisition();
        } else {
            mDiagResp.push_back({mCurrentTransmissionId, nullptr});
            const TransmissionInter it{mRobTransList.find(mCurrentTransmissionId)};
            if (it != mRobTransList.end())
            {
                it->second->disconnect();
            }
            finishCurrentTransmission();
        }

    } else {
        LOG_D("Timeout expired while RoB isn't in acquisition process");
    }
}

void RemoteRoB::printDataDebug(const std::string data) const
{
    std::string data_tmp{generateJson(data)};
    std::string::iterator ptr {data_tmp.begin()};
    std::string a{""};
    while(ptr != data_tmp.end() ) {
        if(*ptr != '\n' ) {
            a += *ptr;
        }
        else {
            LOG_D(a.c_str());
            a.clear();
        }
        ptr++;
    }
}

void RemoteRoB::makeUploadRoBRequest(const android::sp<DiagTrigger> &trigger)
{
    UploadRobDataRequest robDataReq{};
    DiagnosticsMessageList errorDiagMsg{};
    uint32_t sizeOfFileCounter{0U};
    bool isNeedtoUploadRoBData{false};

    const DiagTrigger::DiagTriggerType type{trigger->getType()};
    robDataReq.set_collection_condition_id(trigger->getCollectionID());
    robDataReq.mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ROB_DATA);
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    robDataReq.mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ROB_DATA, counterValue));
    robDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);

    robDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    robDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(AppCommonHeaderVehicleToCenterGeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
    robDataReq.set_counter_value(counterValue);
    (void)counterValue;
    const int32_t timezoneOffSet_hour {TimeManager::getInstance().getOffset() / 60};
    const int32_t timezoneOffSet_minute {TimeManager::getInstance().getOffset() % 60};

    LOG_D("Get timezone offset: %d:%d ", timezoneOffSet_hour, timezoneOffSet_minute);

    robDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(timezoneOffSet_hour);
    robDataReq.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(timezoneOffSet_minute);

    LOG_D("Add location data to upload data, latitude = 0x%08X, longitude = 0x%08X", mCurrentTriggerLocation->getLatitude(), mCurrentTriggerLocation->getLongtitude());

    /*obd2_installed_flag*/
    robDataReq.set_obd2_installed_flag(mApp->getOBDStatus());
    /*oneof_odo_information*/
    uint32_t odo_value;
    uint32_t odo_unit;
    (void)VehicleManagerAdapter::getInstance()->getOdoInformation(odo_value, odo_unit); // odo value
    if (odo_unit == 0x2U)
    {
        robDataReq.set_odo_information_mile(odo_value); // odo_unit = 10b ~ Mile
    }
    else if (odo_unit == 0x1U)
    {
        robDataReq.set_odo_information_km(odo_value); // odo_unit = 01b ~ km
    }
    else
    {
        robDataReq.set_odo_information_km(0xFFFFFFFFU); // RDG30-R-1050 undefined value
    }
    /*under_repair_flag*/
    robDataReq.set_under_repair_flag((mApp->getUnderRepair() == 1U) ? true : false);

    if (type == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        robDataReq.set_trigger_type(RdgProtoInterface::TriggerType::TT_WARNING_TRIGGER);
    }
    else if (type == DiagTrigger::DiagTriggerType::IGON_TRIGGER)
    {
        robDataReq.set_trigger_type(RdgProtoInterface::TriggerType::TT_IG_ON_TRIGGER);
    }
    else if (type == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
    {
        robDataReq.set_trigger_type(RdgProtoInterface::TriggerType::TT_OCCURRENCE_NOTIFICATION_TRIGGER);
    }
    else if (type == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
    {
        robDataReq.set_trigger_type(RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER);
    }
    else
    {
        robDataReq.set_trigger_type(RdgProtoInterface::TriggerType::TT_UNKNOWN);
    }

    if (type == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {   
        LOG_D("retained warning trigger occurrence time %lld", trigger->getTriggerTime());
        const int64_t tmp_time{trigger->getTriggerTime()};
        if(tmp_time >0)
        {
            robDataReq.set_warning_trigger_occurrence_time(static_cast<uint64_t>(tmp_time));
        }
        else
        {
            LOG_D("retained warning trigger occurrence time as undefine");
            robDataReq.set_warning_trigger_occurrence_time(0U);
        }
       
    } else {
        if(mAcquisitionTime > 0)
        {
            LOG_D("retained diagnostics acquisition time: %lld", mAcquisitionTime);
            robDataReq.set_diagnostics_acquisition_time(static_cast<uint64_t>(mAcquisitionTime));
        }
        else
        {
            LOG_D("Invalid diagnostics acquisition time: %lld", mAcquisitionTime);
            LOG_D("retained diagnostics acquisition time as undefine");
            robDataReq.set_diagnostics_acquisition_time(0U);
        }
    }

    robDataReq.mutable_location()->set_latitude(trigger->getLatitude());
    robDataReq.mutable_location()->set_longitude(trigger->getLongtitude());

    sizeOfFileCounter = robDataReq.ByteSizeLong();

    for (std::pair<uint64_t, android::sp<UdsMessage>> &item : mDiagResp)
    {
        const TransmissionInter transIt{mRobTransList.find(item.first)};
        if (transIt != mRobTransList.end())
        {
            RdgProtoInterface::DiagnosticsMessage tmpDiagMessage{};
            LOG_D("Generating data for transmissionId = 0x%02llx", item.first);
            if (item.second == nullptr)
            {
                // RDG30-R-1168
                // RDG30-R-1170
                // RDG30-R-1144
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo{tmpDiagMessage.mutable_ecu_address_information()};
                tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_UNRESPONSIVE);
                const android::sp<::Buffer> reqData {transIt->second->udsReq.ToUdsData()};
                if ((reqData != nullptr) && (reqData->data() != nullptr))
                {
                    tmpDiagMessage.set_user_data(reqData->data(), reqData->size());
                } else {
                    LOG_D("reqData or reqData->data() is null");
                }
                ecuAddressInfo->set_communication_protocol(transIt->second->getEcuInformation().getCommProtocol());
                ecuAddressInfo->set_communication_type(transIt->second->getEcuInformation().getCommType());
                ecuAddressInfo->set_target_address(transIt->second->getEcuInformation().getTargetAddress());
                errorDiagMsg.Add()->CopyFrom(tmpDiagMessage);
                isNeedtoUploadRoBData = true;
            }
            else if (item.second->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION))
            {
                const android::sp<::Buffer> robUserData {item.second->ToUdsData()};
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo{tmpDiagMessage.mutable_ecu_address_information()};
                tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL);
                ecuAddressInfo->set_communication_protocol(transIt->second->getEcuInformation().getCommProtocol());
                ecuAddressInfo->set_communication_type(transIt->second->getEcuInformation().getCommType());
                ecuAddressInfo->set_target_address(transIt->second->getCanIdRx());
                if(robUserData->data() != nullptr) {
                    tmpDiagMessage.set_user_data(robUserData->data(), robUserData->size());
                } else {
                    LOG_D("robUserData is null");
                }

                if (transIt->second->getEcuInformation().getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6) {
                    LOG_D("Phase 6 - Masking StatusOfDTC(pendingDTC,confirmedDTC and testFailed) of the USD response");
                    // RDG30-R-0758, Change UDS response data in the list by masking StatusOfDTC
                    // with 0xF2 (pendingDTC,confirmedDTC and testFailed bits have to be 0)
                    for (uint32_t i{UDS_DTC_ROB_USER_DATA_MASK + 3U}; i < robUserData->size(); i +=4U)
                    {
                        if(robUserData->data() != nullptr) {
                            LOG_D("Phase 6 - Masking, byte %d = 0x%02X", i, robUserData->data()[i]);
                            robUserData->data()[i] &= 0x0DU; // masking StatusOfDTC  with 0x0D (00001101)(pendingDTC,confirmedDTC and testFailed bits have to be 0)
                            LOG_D("Phase 6 - Masking result, byte %d = 0x%02X result", i, robUserData->data()[i]);
                        } else {
                            LOG_D("robUserData is null");
                        }
                    }

                    for (uint32_t i{0U}; i < robUserData->size(); i++)
                    {
                        if(robUserData->data() != nullptr) {
                            LOG_D("Phase 6 - Check UDS_PR_READ_DTC_INFORMATION, byte %d = 0x%02X", i, robUserData->data()[i]);
                        } else {
                            LOG_D("robUserData is null");
                        }
                    }
                }
            }
            else if (item.second->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE))
            {
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo{tmpDiagMessage.mutable_ecu_address_information()};
                tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL_WITH_NEGATIVE);
                ecuAddressInfo->set_communication_protocol(transIt->second->getEcuInformation().getCommProtocol());
                ecuAddressInfo->set_communication_type(transIt->second->getEcuInformation().getCommType());
                ecuAddressInfo->set_target_address(transIt->second->getCanIdRx());
                tmpDiagMessage.set_user_data(item.second->ToUdsData()->data(), item.second->ToUdsData()->size());
                for(uint32_t i{0U}; i < item.second->ToUdsData()->size(); i++)
                {
                    if(item.second->ToUdsData()->data() != nullptr) {
                        LOG_D("Check UDS_NEGATIVE_RESPONSE, byte %d = 0x%02X", i, item.second->ToUdsData()->data()[i]);
                    } else {
                        LOG_D("robUserData is null");
                    }
                }
            }
            else
            {
                // Do nothing
            }
            // RDG30-R-0356
            if((UINT32_MAX - sizeOfFileCounter >= tmpDiagMessage.ByteSizeLong())&& ((sizeOfFileCounter + tmpDiagMessage.ByteSizeLong()) > mMaxUploadFileSize))
            {
                LOG_D("RoB upload data exceeds 4MB -> discard exceeds data");
                if (tmpDiagMessage.ByteSizeLong() > 0U)
                {
                    // RDG30-R-1171
                    tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL_WITH_EXCEEDED_SIZE);
                    LOG_D("Assigne status_code = SC_SUCCESSFUL_WITH_EXCEEDED_SIZE");
                    //const uint32_t sizeOfStatusCode{sizeof(tmpDiagMessage.status_code())};
                    
                    if (item.second != nullptr)
                    {
                        const uint32_t sizeOfEcuAddressInformation{tmpDiagMessage.ecu_address_information().ByteSizeLong()};
                        const uint32_t tmp_status{tmpDiagMessage.status_code()};
                        const uint32_t remainingBytes{mMaxUploadFileSize - (sizeOfFileCounter + sizeof(tmp_status) + sizeOfEcuAddressInformation)};
                        if(item.second->ToUdsData()->data() != nullptr) {
                            tmpDiagMessage.set_user_data(item.second->ToUdsData()->data(), remainingBytes < item.second->ToUdsData()->size() ? remainingBytes : item.second->ToUdsData()->size());
                        } else {
                            LOG_D("RoB data is null");
                        }
                        (void)remainingBytes;
                    }
                }
                break;
            }
            else
            {
                sizeOfFileCounter += tmpDiagMessage.ByteSizeLong();
                robDataReq.add_rob_messages()->CopyFrom(tmpDiagMessage);
            }
        }
    }


        // RDG30-R-0757
    if ((type == DiagTrigger::DiagTriggerType::IGON_TRIGGER) || (type == DiagTrigger::DiagTriggerType::WARNING_TRIGGER))
    {
        if (this->calculateCRC() == true)
        {
            isNeedtoUploadRoBData = true;
            /* generte upload data */
            LOG_D("CRC Have change in response list");
            /* TBD: Call to Uploader to make upload data*/
        }
        else
        {
            LOG_D("CRC NO change in response list");
            isNeedtoUploadRoBData = false;
        }
    }
    else
    {
        LOG_D("Abort CRC check for center request");
        isNeedtoUploadRoBData = true;
    }


    if (isNeedtoUploadRoBData == true)
    {

        LOG_D("Send RoB upload data to Center:");
        // TODO: Need to change for using Uploader's API
        /*Save UploadRobDataRequest to file*/
        std::string str{};
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        (void)google::protobuf::util::MessageToJsonString(robDataReq, &str, option);
        printDataDebug(str);
        const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
        std::string file_dir{std::to_string(uploadId)};
        (void)file_dir.append("_UploadRobDataRequest.dat");
        error_t error{E_ERROR};
        error = DataModel<UploadRobDataRequest>::save(file_dir, robDataReq);
        if (error == E_OK)
        {
            const android::sp<UploadTask> task{new UploadTask(uploadId)};
            task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG050);
            task->setUploadPatch(file_dir);
            // task->setUploadId(uploadId);
            /*Set priority*/
            task->setUploadPrio(trigger->getPriority());
            const uint64_t fileSize{static_cast<uint64_t>(robDataReq.ByteSizeLong())};
            task->setFileSize(fileSize);
            UploadManager::getInstance()->requestUploadTask(task);
        }
    }

    // RDG30-R-0718, RDG30-R-0722, RDG30-R-0088
    if ((type == DiagTrigger::DiagTriggerType::IGON_TRIGGER) || (type == DiagTrigger::DiagTriggerType::WARNING_TRIGGER))
    {
        mApp->triggerLastUpload(type
                                , trigger->getTriggerTime()
                                , mCurrentTriggerLocation
                                , trigger->getCollectionID()
                                , trigger->getPriority());
        LOG_D("Notify TriggerType %d, start time = %lld to Last Upload Data", type, trigger->getTriggerTime());
    } 
    else if ( (trigger->getFunc() != DiagTrigger::DiagTriggerFunc::ALLROB) && (type == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) ) 
    {
        // RDG30-R-1128
        mApp->triggerLastUpload(type
                        , mAcquisitionTime
                        , mCurrentTriggerLocation
                        , trigger->getCollectionID()
                        , trigger->getPriority());
        LOG_D("Notify trigger name: Other, start time = %lld to Last Upload Data", mAcquisitionTime);
    } else {
        // Do nothing
    }
    mRobTransList.clear();
    const uint32_t tmp{trigger->getTriggerId()};
    if(tmp<static_cast<uint32_t>(INT32_MAX))
    {
        finishRoBAcquisition(static_cast<int32_t>(tmp));
    }
    else
    {
        LOG_I("Error coding");
    }
    (void)sizeOfFileCounter;
}

void RemoteRoB::makeErrorUploadDataRequest(const RdgProtoInterface::ResponseCode code, const android::sp<DiagTrigger> &trigger)
{
    // how many upload data is made after IG-ON and return to 00 after 99.
    const DiagTrigger::DiagTriggerType triggerType{trigger->getType()};
    const DiagTrigger::DiagTriggerFunc triggerFunc{trigger->getFunc()};
    const int64_t triggerOccurrenceTime{trigger->getTriggerTime()};
    UploadErrorDataRequestFunctionType errorfunctionType{UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ROB_ACQUISITION_REQUEST};
    if ((triggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) && (triggerFunc == DiagTrigger::DiagTriggerFunc::ROB))
    {
        errorfunctionType = UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ALL_DIAG;
    }
    else if ((triggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) && (triggerFunc == DiagTrigger::DiagTriggerFunc::ALLROB))
    {
        errorfunctionType = UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ALL_ROB;
    }
    else if ((triggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) || (triggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER))
    {
        errorfunctionType = UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ROB_ACQUISITION_REQUEST;
    }
    else
    {
        // Do nothing
    }
    (void)triggerFunc;
    UploadErrorDataRequest robUploadErrorData{};
    robUploadErrorData.mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    robUploadErrorData.mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, counterValue));
    robUploadErrorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    robUploadErrorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    robUploadErrorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(AppCommonHeaderVehicleToCenterGeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
    robUploadErrorData.set_counter_value(counterValue);
    (void)counterValue;
    const int32_t timezoneOffSet_hour {TimeManager::getInstance().getOffset() / 60};
    const int32_t timezoneOffSet_minute {TimeManager::getInstance().getOffset() % 60};

    LOG_D("Get timezone offset: %d:%d ", timezoneOffSet_hour, timezoneOffSet_minute);

    robUploadErrorData.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(timezoneOffSet_hour);
    robUploadErrorData.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(timezoneOffSet_minute);

    robUploadErrorData.set_function_type(errorfunctionType);
    robUploadErrorData.set_response_code(code);

    if (triggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        robUploadErrorData.set_trigger_type(RdgProtoInterface::TriggerType::TT_WARNING_TRIGGER);
    }
    else if (triggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER)
    {
        robUploadErrorData.set_trigger_type(RdgProtoInterface::TriggerType::TT_IG_ON_TRIGGER);
    }
    else if (triggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
    {
        robUploadErrorData.set_trigger_type(RdgProtoInterface::TriggerType::TT_OCCURRENCE_NOTIFICATION_TRIGGER);
    }
    else if (triggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
    {
        robUploadErrorData.set_trigger_type(RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER);
    }
    else
    {
        robUploadErrorData.set_trigger_type(RdgProtoInterface::TriggerType::TT_UNKNOWN);
    }

    if (triggerOccurrenceTime > 0)
    {
        robUploadErrorData.set_data_creation_date(static_cast<uint64_t>(triggerOccurrenceTime));
    }

    robUploadErrorData.set_obd2_installed_flag(mApp->getOBDStatus());

    /*under_repair_flag*/
    robUploadErrorData.set_under_repair_flag((mApp->getUnderRepair() == 1U) ? true : false);

    LOG_D("Send error upload data to Center");
    // TODO: Need to change for using Uploader's API
    /*Save UploadErrorDataRequest to file*/
    std::string str{};
    (void)google::protobuf::util::MessageToJsonString(robUploadErrorData, &str);
    printDataDebug(str);
    const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
    std::string file_dir{std::to_string(uploadId)};
    (void)file_dir.append("_UploadErrorDataRequest.dat");
    const error_t error{DataModel<UploadErrorDataRequest>::save(file_dir, robUploadErrorData)};
    if (error == E_OK)
    {
        const android::sp<UploadTask> task{new UploadTask(uploadId)};
        const uint32_t priority{trigger->getPriority()};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
        task->setUploadPatch(file_dir);
        // task->setUploadId(uploadId);
        /*Set priority*/
        (void)task->setUploadPrio(priority);
        const uint64_t fileSize{static_cast<uint64_t>(robUploadErrorData.ByteSizeLong())};
        task->setFileSize(fileSize);
        UploadManager::getInstance()->requestUploadTask(task);
    }
}

void RemoteRoB::makeErrorUploadDataRequest(const RdgProtoInterface::ResponseCode code, const DiagnosticsMessageList &diagMsg, const android::sp<DiagTrigger> &trigger)
{
    // how many upload data is made after IG-ON and return to 00 after 99.
    const DiagTrigger::DiagTriggerType triggerType{trigger->getType()};
    const DiagTrigger::DiagTriggerFunc triggerFunc{trigger->getFunc()};
    const int64_t triggerOccurrenceTime{trigger->getTriggerTime()};
    UploadErrorDataRequestFunctionType errorfunctionType{UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ROB_ACQUISITION_REQUEST};
    if ((triggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) && (triggerFunc == DiagTrigger::DiagTriggerFunc::ALLDIAG))
    {
        errorfunctionType = UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ALL_DIAG;
    }
    else if ((triggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) && (triggerFunc == DiagTrigger::DiagTriggerFunc::ALLROB))
    {
        errorfunctionType = UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ALL_ROB;
    }
    else if ((triggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) || (triggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER))
    {
        errorfunctionType = UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ROB_ACQUISITION_REQUEST;
    }
    else
    {
        // Do nothing
    }
    (void)triggerFunc;
    UploadErrorDataRequest robUploadErrorData{};
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    robUploadErrorData.mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
    robUploadErrorData.mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, counterValue));
    robUploadErrorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    robUploadErrorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(AppCommonHeaderVehicleToCenterGeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
    robUploadErrorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);

    const int32_t timezoneOffSet_hour {TimeManager::getInstance().getOffset() / 60};
    const int32_t timezoneOffSet_minute {TimeManager::getInstance().getOffset() % 60};

    LOG_D("Get timezone offset: %d:%d ", timezoneOffSet_hour, timezoneOffSet_minute);

    robUploadErrorData.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(timezoneOffSet_hour);
    robUploadErrorData.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(timezoneOffSet_minute);
    robUploadErrorData.set_counter_value(counterValue);
    (void)counterValue;
    robUploadErrorData.set_function_type(errorfunctionType);
    robUploadErrorData.set_response_code(code);
    if(triggerOccurrenceTime>=0)
    {
        robUploadErrorData.set_data_creation_date(static_cast<uint64_t>(triggerOccurrenceTime));
    }
    else
    {
        robUploadErrorData.set_data_creation_date(0U);
    }

    robUploadErrorData.set_obd2_installed_flag(mApp->getOBDStatus());

    /*under_repair_flag*/
    robUploadErrorData.set_under_repair_flag((mApp->getUnderRepair() == 1U) ? true : false);

    if (triggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        robUploadErrorData.set_trigger_type(RdgProtoInterface::TriggerType::TT_WARNING_TRIGGER);
    }
    else if (triggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER)
    {
        robUploadErrorData.set_trigger_type(RdgProtoInterface::TriggerType::TT_IG_ON_TRIGGER);
    }
    else if (triggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
    {
        robUploadErrorData.set_trigger_type(RdgProtoInterface::TriggerType::TT_OCCURRENCE_NOTIFICATION_TRIGGER);
    }
    else if (triggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
    {
        robUploadErrorData.set_trigger_type(RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER);
    }
    else
    {
        robUploadErrorData.set_trigger_type(RdgProtoInterface::TriggerType::TT_UNKNOWN);
    }

    robUploadErrorData.mutable_diag_messages()->CopyFrom(diagMsg);

    LOG_D("Send error upload data to Center");
    // TODO: Need to change for using Uploader's API
    /*Save UploadErrorDataRequest to file*/
    std::string str{};
    (void)google::protobuf::util::MessageToJsonString(robUploadErrorData, &str);
    printDataDebug(str);
    const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
    std::string file_dir{std::to_string(uploadId)};
    error_t error{E_ERROR};
    (void)file_dir.append("_UploadErrorDataRequest.dat");
    error = DataModel<UploadErrorDataRequest>::save(file_dir, robUploadErrorData);
    if (error == E_OK)
    {
        const android::sp<UploadTask> task{new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
        task->setUploadPatch(file_dir);
        // task->setUploadId(uploadId);
        /*Set priority*/
        const uint64_t fileSize{static_cast<uint64_t>(robUploadErrorData.ByteSizeLong())};
        task->setFileSize(fileSize);
        task->setUploadPrio(trigger->getPriority());
        UploadManager::getInstance()->requestUploadTask(task);
    }
}

void RemoteRoB::abortRobAcquisition(const RdgProtoInterface::ResponseCode code, const android::sp<DiagTrigger> &trigger)
{
    LOG_D("abortRobAcquisition, responsecode = %d", code);
    // Abort aquisition if RoB Data
    mIsRobRunning = false;
    mIsAcquisitionAbort = true;

    mTransmissioIdList.clear();

    mCrcCheck.clear();

    mRobTransList.clear();

    mDiagResp.clear();

    // Release OBC resource
    OnboardclientAdapter::getInstance()->ReleaseObcResource();

    makeErrorUploadDataRequest(code, trigger);
    const uint32_t triggerId{trigger->getTriggerId()};
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mTriggerList.find(triggerId)};
    if (it != mTriggerList.end())
    {
        if(triggerId<static_cast<uint32_t>(INT32_MAX))
        {
            const DiagTrigger::DiagTriggerType temp_type {it->second->getType()};
            if ((temp_type < DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX) &&  (temp_type > DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN))
            {
                PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(triggerId), temp_type);
            }
            (void)mTriggerList.erase(it);
        }
        else
        {
            LOG_D("triggerId value error: %d in mTriggerList", triggerId);
        }
    }
    else
    {
        LOG_D("Can not find triggerId: %d in mTriggerList", triggerId);
    }
    mIsAcquisitionAbort = false;
}

void RemoteRoB::suspendRobAcquisition(void)
{
    LOG_D("suspendRobAcquisition");
    mIsAcquisitionAbort = true;

    if (mIsRobRunning)
    {
        const TransmissionInter it{mRobTransList.find(mCurrentTransmissionId)};
        if ((it != mRobTransList.end()) && (it->second->getState() == RobUdsTransmission::State::ROB_TRANS_SEND_UDS))
        {
            LOG_D("Waiting ECU response for stoping RoB acquisition");
        } else {
            LOG_D("NOT match transmissionId or ECU was disconnected");
        }
    }
}

void RemoteRoB::stopRobAcquisition(void)
{
    LOG_D("Stoping RoB Acquisition");

    const TransmissionInter it{mRobTransList.find(mCurrentTransmissionId)};
    if ((it != mRobTransList.end()) && (it->second->getState() == RobUdsTransmission::State::ROB_TRANS_SEND_UDS))
    {
        LOG_D("Disconnect current transmision");
        it->second->stopTimeout();
        it->second->disconnect();
        it->second->setState(RobUdsTransmission::State::ROB_TRANS_DISCONNECT);

    } else {
        LOG_D("NOT match transmissionId or ECU was disconnected");
    }

    // suspend aquisition if RoB Data
    mIsRobRunning = false;
    // RDG30-R-0410: Aquired response data shoud be removed
    mTransmissioIdList.clear();

    mCrcCheck.clear();

    mRobTransList.clear();

    mDiagResp.clear();

    // Release OBC resource
    OnboardclientAdapter::getInstance()->ReleaseObcResource();

    if (mTriggerId >= 0)
    {
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it2{mTriggerList.find(static_cast<uint32_t>(mTriggerId))};
        if (it2 != mTriggerList.end())
        {
            const DiagTrigger::DiagTriggerType temp_type {it2->second->getType()};
            if ((temp_type < DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX) &&  (temp_type > DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN))
            {
                PriorityControl::getInstance()->notifyTriggerProcessDone(mTriggerId, temp_type);
            }
        }
        else
        {
            LOG_D("Can not find triggerId: %d in mTriggerList", mTriggerId);
        }
    } else {
        LOG_D("Can not find triggerId: mTriggerId < 0", mTriggerId);
    }
    mIsAcquisitionAbort = false;
}

error_t RemoteRoB::getDiagTrigger(const uint32_t triggerId, android::sp<DiagTrigger> &trigger)
{
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mTriggerList.find(triggerId)};
    error_t error{E_ERROR};
    if (it != mTriggerList.end())
    {
        trigger = it->second;
        error = E_OK;
        goto exit;
    }
    else
    {
        LOG_E("Can not find trigger id");
    }
exit:
    return error;
}

void RemoteRoB::testHandleWarningTrigger(const int32_t pri)
{
    (void)mHandler->obtainMessage(MainHandler::CMD_SIMULATE_WARNING_TRIGGER_EVENT, pri)->sendToTarget();
}

void RemoteRoB::testingMaxFileSize(const uint32_t fileSize) noexcept
{
    mMaxUploadFileSize = fileSize;
}

void RemoteRoB::onRobFlagChangeOFF()
{
    // RDG30-R-0759
    LOG_D("Clear CRC");
    mCRCManager->requestRemoveCRCFile();
}
} // namespace rdgapp
