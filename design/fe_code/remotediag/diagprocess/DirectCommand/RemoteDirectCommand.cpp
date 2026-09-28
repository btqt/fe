#include <google/protobuf/util/json_util.h>
#include "RemoteDirectCommand.h"
#include "DataModel.h"
#include "diagprocess/UDS/SidFilter.h"

namespace rdgapp
{

    RemoteDirectCommand *RemoteDirectCommand::mRemoteDirectCommand{nullptr};
    RemoteDirectCommand::RemoteDirectCommand(const Remotediag &app, android::sp<sl::SLLooper> &privateLooper) : android::RefBase(), mApp(app)
    {
        mRemoteDirectCommand = this;
        isDirectAppRunning = RemoteDirectCommand::DIRECTCOMMAND_STOP;
        mDirectCommandAcquisitionTime = 0;
        odo_value = 0U;
        odo_unit = 0U;
        mCurrentTransmissionId = 0U;
        mTriggerId = 0;
        mColId = 0U;
        mPriority = 0U;
        mTotalSize = 0U;
        bUploadDirectCommandData = false;
        bUploadErrorData = false;
        bsrvc_ac_flag = false;
        mSuspended = false;
        bAbortUnderRepair = false;
        mOccurrenceNotificationId = 0U;
        mDirectCommandTimerHandlerTrans = nullptr;
        mDirectCommandTimerHandlerAbort = nullptr;
        mTriggerType = DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN;
        DIRECTCOMMAND_TOTAL_UPLOAD_DATA_SIZE_MAX = 10485760U; // bytes = 10MB
        DIRECTCOMMAND_UPLOAD_DATA_SIZE_MAX = 4194304U;        // bytes = 4MB
        DIRECTCOMMAND_LAST_UPLOAD_DATA_SIZE_MAX = 2097152U;   // bytes = 2MB
        mDirectCommandUploadData = std::shared_ptr<vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequest>(new vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequest());
        mDirectCommandUploadErrorData = std::shared_ptr<vccomif::rdg::v1::interfaces::UploadErrorDataRequest>(new vccomif::rdg::v1::interfaces::UploadErrorDataRequest());
        mRemoteDirectCommandHandler = new MainHandler(privateLooper, *this);
        (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_INIT)->sendToTarget();
    }

    RemoteDirectCommand::~RemoteDirectCommand(void) = default;

    RemoteDirectCommand *RemoteDirectCommand::getInstance(void)
    {
        if (mRemoteDirectCommand == nullptr)
        {
            LOG_E("RemoteDirectCommand is not created");
        }
        return mRemoteDirectCommand;
    }

    void RemoteDirectCommand::init(void) const
    {
        LOG_I("Init RemoteDirectCommand");
    }

    void RemoteDirectCommand::printData(const std::string data) const
    {
        std::string data_tmp{generateJson(data)};
        std::string::iterator ptr{data_tmp.begin()};
        std::string a{""};
        while (ptr != data_tmp.end())
        {
            if (*ptr != '\n')
            {
                a += *ptr;
            }
            else
            {
                LOG_V(a.c_str());
                a.clear();
            }
            ptr++;
        }
    }

    bool RemoteDirectCommand::checkPrecondition(void) noexcept
    {
        return false;
    }

    bool RemoteDirectCommand::checkPrecondition(const DiagTrigger::DiagTriggerType diagTriggerType, const uint64_t collectionId)
    {
        bool res{false};
        int32_t RejectCode{0};
        DirectCommandList l_DirectCommand{};
        bool srvc_ac_disregard_flag{false};
        this->getDirectCommandRequest(diagTriggerType, collectionId, l_DirectCommand, srvc_ac_disregard_flag);
        if ((DiagManagerAdapter::getInstance()->getAllUploadConsent() == false) && (srvc_ac_disregard_flag == false))
        {
            res = false;
            RejectCode = 1;
            LOG_W("Reject DirectCommand function due to all upload consent status and srvc_ac_flag are false");
            if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
            {
                const int64_t current_time{CommonUtils::getCurrentAcquisiteTime()};
                mApp.notifyLastOpComplTime(collectionId, current_time, false);
            }
            CollectionCondition::getInstance().onFinishCenterRequestJob(collectionId);
        }
        else
        {
            res = true;
            if (mApp.getUnderRepair() == 1U)
            {
                res = false;
                RejectCode = 2;
                LOG_W("Reject DirectCommand function due to under repair");
                vccomif::rdg::v1::interfaces::UploadErrorDataRequest uploadErrorData{};
                uploadErrorData.set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR);
                this->makeErrorUploadData(uploadErrorData, diagTriggerType, collectionId, srvc_ac_disregard_flag, mDirectCommandAcquisitionTime);
            }
            else if (DiagManagerAdapter::getInstance()->getRDGFlag() == 0U)
            {
                res = false;
                RejectCode = 3;
                LOG_W("Reject DirectCommand function due to RDG flag is OFF");
                vccomif::rdg::v1::interfaces::UploadErrorDataRequest uploadErrorData{};
                uploadErrorData.set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_UNPROVIDED_VEHICLE);
                this->makeErrorUploadData(uploadErrorData, diagTriggerType, collectionId, srvc_ac_disregard_flag, mDirectCommandAcquisitionTime);
            }
            else
            {
                res = true;
            }
        }
        if (res == false)
        {
            LOG_E("RejectCode: %d", RejectCode);
        }
        return res;
    }

    void RemoteDirectCommand::notifyBootComplete(void) const noexcept
    {
    }

    void RemoteDirectCommand::onReceiveIG(const bool status) const noexcept
    {
        (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_CHANGE_IG_STATUS, static_cast<int32_t>(status))->sendToTarget();
    }

    void RemoteDirectCommand::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) noexcept
    {
        if (this->getAppStatus() == RemoteDirectCommand::DIRECTCOMMAND_RUNNING)
        {
            (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_EVENT_UDS_RES, responseEventInfo)->sendToTarget();
        }
    }

    void RemoteDirectCommand::onChangedRemoteInfo(const int32_t what, const int32_t info) noexcept
    {
        LOG_I("onChangedRemoteInfo");
        (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_RECEIVE_UNDER_REPAIR_FLAG_CHANGE, what, info)->sendToTarget();
    }

    void RemoteDirectCommand::onCenterCommandForward(const android::sp<CenterReqData> &pCenterReqData) noexcept
    {
        const IG_STATUS igStatus{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
        const uint64_t colID{pCenterReqData->getCenterReq_CollectionID()};
        const Rdg_Sched_Type::SchedType schedTypeInfo{pCenterReqData->getScheduleType()};
        LOG_I("RemoteDirectCommand receive Center request: igStatus %d, getScheduleType: %d, colID: %llu", igStatus, schedTypeInfo, colID);
        if ((((schedTypeInfo == Rdg_Sched_Type::SchedType::ST_IMMEDIATE_IG_ON_ONE_SHOT) ||
              (schedTypeInfo == Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE) ||
              (schedTypeInfo == Rdg_Sched_Type::SchedType::ST_PERIOD_TRIGGER_ROUTINE) ||
              (schedTypeInfo == Rdg_Sched_Type::SchedType::ST_IMMEDIATE_ANY_POWER_STATUS_ONE_SHOT)) &&
             (igStatus == IG_STATUS::IG_STATUS_ON)) ||
            (((schedTypeInfo == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT) ||
              (schedTypeInfo == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_ROUTINE)) &&
             (igStatus == IG_STATUS::IG_STATUS_OFF)))
        {
            uint32_t prio_data{pCenterReqData->getCenterReq_prio()};
            uint8_t colId_ptr[sizeof(colID)];
            (void)memcpy(&colId_ptr[0], &colID, sizeof(colId_ptr));
            const android::sp<::Buffer> colId_sp{new ::Buffer()};
            colId_sp->setTo(&colId_ptr[0], sizeof(colId_ptr));
            int32_t what{0};
            const DiagTrigger::DiagTriggerType triggerType{pCenterReqData->getTrigger_type()};
            if (triggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
            {
                what = MainHandler::CMD_DIRECTCOMMAND_CENTER_REQUEST;
            }
            else if (triggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
            {
                const uint8_t messId{pCenterReqData->getCenterReq_messageID()};
                if (messId == MSG_ID_CENTERREQUESTDIRECTCOMMAND)
                {
                    what = MainHandler::CMD_DIRECTCOMMAND_CENTER_REQUEST;
                }
                else
                {
                    what = MainHandler::CMD_DIRECTCOMMAND_COLLECTION_CONDITION;
                }
            }
            else
            {
                what = MainHandler::CMD_DIRECTCOMMAND_MAX;
            }

            prio_data = prio_data <= static_cast<uint32_t>(INT32_MAX) ? prio_data : 0U;
            const sp<sl::Message> msg{mRemoteDirectCommandHandler->obtainMessage(what, static_cast<int32_t>(prio_data))};
            const uint32_t coldIdSize{colId_sp->size()};
            if (coldIdSize <= static_cast<uint32_t>(INT32_MAX))
            {
                msg->buffer.setTo(colId_sp->data(), static_cast<int32_t>(coldIdSize));
            }
            else
            {
                LOG_E("colId overflow");
            }
            (void)msg->sendToTarget();
        }
        else
        {
            LOG_I("Reject DirectCommand function due to schedule type mismatch %d", schedTypeInfo);
            // TimeManager &mTimeManagerService{TimeManager::getInstance()};
            CollectionCondition::getInstance().onFinishCenterRequestJob(colID);
            const int64_t current_time{CommonUtils::getCurrentAcquisiteTime()};
            LOG_I("Collection ID: %llu - Last complete time: %lld - trigger type: %d", mColId, current_time, mTriggerType);
            if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
            {
                mApp.notifyLastOpComplTime(colID, current_time, false);
            }
        }
        (void)igStatus;
    }
    void RemoteDirectCommand::onRdgStop(const bool isStop) const noexcept
    {
        // obtain message to stop rdg
        (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_STOP_RDG, static_cast<int32_t>(isStop))->sendToTarget();
        (void)isStop;
    }

    uint8_t RemoteDirectCommand::getAppId(void) const noexcept
    {
        return RDG_APPID::DIRECT_COMMAND;
    }

    void RemoteDirectCommand::startUp(void)
    {
        // Get Time data
        mTotalSize = 0U;
        this->makeHeaderForUploading(mDirectCommandUploadData);

        const uint32_t uploadDataSize{static_cast<uint32_t>(mDirectCommandUploadData->ByteSizeLong())};
        if ((uploadDataSize < UINT32_MAX / 3U) && (mTotalSize <= UINT32_MAX - (3U * uploadDataSize)))
        {
            mTotalSize = mTotalSize + (3U * uploadDataSize);
        }
        else
        {
            LOG_E("total size overflow");
        }
        mDirectCommandUploadData->Clear();
        const OBCResourceEventCode resEventInfo{OnboardclientAdapter::getInstance()->GetObcResource()};
        if (resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK)
        {
            LOG_E("GetObcResource OBC_GET_RESOURCE_WAIT -> wait and check after");
            (void)mRemoteDirectCommandHandler->sendMessageDelayed(mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_START_UP), 1000U);
        }
        else
        {
            LOG_I("Set obc resource OBC_GET_RESOURCE_WAIT");
            OnboardclientAdapter::getInstance()->TakeObcResource();
            LOG_W("mDirectCommandTransQueue size = %u, mPriority %u, cocoId %llu", mDirectCommandTransQueue.size(), mPriority, mColId);
            mCurrDirectCommandTrans = mDirectCommandTransQueue.front();
            mCurrDirectCommandTrans->connect();
        }
    }

    void RemoteDirectCommand::getDirectCommandRequest(const DiagTrigger::DiagTriggerType triggerType, const uint64_t collectionId, google::protobuf::RepeatedPtrField<vccomif::rdg::v1::interfaces::DirectCommand> &commandList, bool &srvc_ac_flag)
    {
        std::string str_directCommand_req{""};
        if (triggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
        {
            srvc_ac_flag = false;
            if (mRobNotificationReq != nullptr)
            {
                commandList.CopyFrom(*(mRobNotificationReq->directCommands()));
            }
            else
            {
                LOG_E("mRobNotificationReq is nullptr");
            }
        }
        else
        {
            if (triggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
            {
                const std::shared_ptr<CenterRequestJob> job{CollectionCondition::getInstance().getCenterRequestJob(collectionId)};
                if ((job != nullptr) && (job->getMessageId() == MSG_ID_CENTERREQUESTDIRECTCOMMAND))
                {
                    const std::shared_ptr<google::protobuf::Message> message{job->getPayload()};
                    if (message != nullptr)
                    {
                        const std::shared_ptr<CenterRequestDirectCommand> centerReqDirectCommand{std::dynamic_pointer_cast<CenterRequestDirectCommand>(message)};
                        if (centerReqDirectCommand != nullptr)
                        {
                            LOG_I("Get Center request direct command, collectionId = %llu", collectionId);
                            google::protobuf::util::JsonOptions option{};
                            option.always_print_primitive_fields = true;
                            option.preserve_proto_field_names = true;
                            (void)google::protobuf::util::MessageToJsonString(*centerReqDirectCommand, &str_directCommand_req, option);
                            printData(str_directCommand_req);
                            srvc_ac_flag = centerReqDirectCommand->srvc_ac_disregard_flag();
                            commandList.CopyFrom(centerReqDirectCommand->direct_commands());
                        }
                    }
                }
            }
            else if (triggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
            {
                const CollectionConditionDirectCommandList &mCollectionConditionDirectCommandList{CollectionCondition::getInstance().getCollectionConditionDirectCommandRequest()};
                for (CollectionConditionDirectCommandIter it{mCollectionConditionDirectCommandList.begin()}; it != mCollectionConditionDirectCommandList.end(); it++)
                {
                    if ((*it).collection_condition_id() == collectionId)
                    {
                        LOG_I("Get CollectionCondition direct command, collectionId = %llu", collectionId);
                        google::protobuf::util::JsonOptions option{};
                        option.always_print_primitive_fields = true;
                        option.preserve_proto_field_names = true;
                        (void)google::protobuf::util::MessageToJsonString(*it, &str_directCommand_req, option);
                        printData(str_directCommand_req);
                        srvc_ac_flag = (*it).srvc_ac_disregard_flag();
                        commandList.CopyFrom((*it).direct_commands());
                        break;
                    }
                }
            }
            else
            {
                // DO nothing
            }
        }
    }

    bool RemoteDirectCommand::validateDirectCommand(const vccomif::rdg::v1::interfaces::DirectCommand bDirectCommandReq, CommonDefine::EcuInformation &ecuInformation)
    {
        // communication_protocol()
        bool res{false};
        const vccomif::rdg::v1::interfaces::EcuAddressInformation ecuReqInfo{bDirectCommandReq.ecu_address_information()};
        const CenterCommunicationProtocol protocol{ecuReqInfo.communication_protocol()};
        const uint32_t centerTargetAdd{ecuReqInfo.target_address()};
        const CenterCommunicationType reqCmdType{ecuReqInfo.communication_type()};
        if ((centerTargetAdd != 0U) &&
            ((protocol == CenterCommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD) ||
             ((protocol == CenterCommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN) &&
              ((reqCmdType == CenterCommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
               (reqCmdType == CenterCommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS)))))
        {
            std::list<CommonDefine::EcuInformation>::iterator it{l_EcuInformation.begin()};
            LOG_I("Ecu list size = %lu", l_EcuInformation.size());
            while (it != l_EcuInformation.end())
            {
                uint32_t targetAddCompare{centerTargetAdd};
                if ((reqCmdType == CenterCommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) &&
                    (centerTargetAdd < 0xFFFFFFU)) // 3bytes max (00 FF FF FF)
                {
                    targetAddCompare = ((targetAddCompare << 8U) | 0xFFU);
                }
                const uint32_t targetAdd{it->getTargetAddress()};
                if (targetAdd == targetAddCompare)
                {
                    const CommonDefine::DiagPhase ecuDiagPhase{it->getDiagPhase()};
                    if ((ecuDiagPhase > CommonDefine::DiagPhase::DP_UNKNOW) &&
                        (ecuDiagPhase <= CommonDefine::DiagPhase::DP_PHASE_6))
                    {
                        res = true;
                        // const bool ecuFlag{it->getecuActiveFlag()};
                        ecuInformation.setTargetAddress(targetAdd);
                        ecuInformation.setCanId(it->getCanId());
                        // ecuInformation.setecuActiveFlag(ecuFlag);
                        ecuInformation.setCommProtocol(it->getCommProtocol());
                        ecuInformation.setCommType(it->getCommType());
                        ecuInformation.setDiagPhase(ecuDiagPhase);
                        break;
                    }
                    (void)ecuDiagPhase;
                }
                it++;
            }
        }
        else
        {
            res = false;
            LOG_D("Invalid ECU specificed error targetAddr %u", centerTargetAdd);
        }
        (void)protocol;
        (void)reqCmdType;
        return res;
    }

    void RemoteDirectCommand::triggerDirectCommand(const DiagTrigger::DiagTriggerType type, const uint32_t prio, const uint64_t colId, const int64_t time, const uint64_t notificationId)
    {

        LOG_I("DirectCommand trigger");
        /* TBD: check ppi flag*/
        /* Get trigger ID*/
        const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
        /* Create NewDiag Trigger*/
        /* DiagTrigger(const DiagTrigger::DiagTriggerType type, const uint32_t priority, const DiagTrigger::DiagTriggerFunc func, const int32_t triggerId)*/
        const android::sp<DiagTrigger> pTrigger{new DiagTrigger(type, prio, DiagTrigger::DiagTriggerFunc::DIRECT_COMMAND, nextTriggerId)};
        /* set trigger time*/
        pTrigger->setTriggerTime(time);
        /* Set collection id*/
        pTrigger->setCollectionId(colId);
        pTrigger->setNotificationId(notificationId);
        LOG_I("Check DirectCommand: trigger time: %lld sec, Collection ID: %llu", time, colId);
        /* Save request to local*/
        const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret{mSaveReq.emplace(nextTriggerId, pTrigger)};
        LOG_I("Check mSaveReq size: %lu", mSaveReq.size());
        if (!ret.second)
        {
            ret.first->second = pTrigger;
        }
        /* Obtain message:CMD_DIRECTCOMMAND_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
        (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
        LOG_I("Check saved DirectCommand trigger ID: %d", ret.first->first);
    }

    void RemoteDirectCommand::initTimer(void)
    {
        mDirectCommandTimerHandlerTrans = std::make_unique<DirectCommandTimerHandler>(*this);
        mDirectCommandTimerHandlerAbort = std::make_unique<DirectCommandTimerHandler>(*this);
        this->mTimeOutTrans = android::sp<Timer>(new Timer(mDirectCommandTimerHandlerTrans.get(), DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_TRANSMISSION_TIMEOUT));
        this->mTimeOutAbort = android::sp<Timer>(new Timer(mDirectCommandTimerHandlerAbort.get(), DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_IG_OFF_TRIGGER));
    }

    void RemoteDirectCommand::startTimer(const int32_t timerId)
    {
        LOG_I("Start timer id: %ld ", timerId);
        switch (timerId)
        {
        case DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_TRANSMISSION_TIMEOUT:
            this->mTimeOutTrans->stop();
            this->mTimeOutTrans->setDuration(195U, 0U);
            this->mTimeOutTrans->start();
            break;
        case DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_IG_OFF_TRIGGER:
        {
            this->mTimeOutAbort->stop();
            this->mTimeOutAbort->setDuration(25U, 0U);
            this->mTimeOutAbort->start();
            break;
        }
        default:
            break;
        }
    }

    void RemoteDirectCommand::stopTimer(const int32_t timerId)
    {
        LOG_I("Start timer id: %ld ", timerId);
        switch (timerId)
        {
        case DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_TRANSMISSION_TIMEOUT:
            this->mTimeOutTrans->stop();
            break;
        case DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_IG_OFF_TRIGGER:
        {
            this->mTimeOutAbort->stop();
            break;
        }
        default:
            break;
        }
    }
    void RemoteDirectCommand::onTransmissionTimeout(void)
    {
        // TODO
        LOG_I("onTransmissionTimeout");
        this->mCurrDirectCommandTrans->disconnect();
        mCurrDirectCommandTrans->setState(DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DISCONNECT);
        if (mCurrDirectCommandTrans->getUdsReqList().size() != 0U)
        {
            mCurrDirectCommandTrans->connect();
        }
        else
        {
            this->finishCurrentTransmission();
        }
    }

    bool RemoteDirectCommand::notifyTrigger(const DiagTrigger::DiagTriggerState &pState,
                                            const int32_t &pTriggerId, const bool dueToIgOff)
    {
        bool isDirectCommandTrigger{false};
        /* Check if trigger is in the saved trigger*/
        const uint32_t u_pTriggerId{(pTriggerId >= 0) ? static_cast<uint32_t>(pTriggerId) : 0U};
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mSaveReq.find(u_pTriggerId)};
        if (it != mSaveReq.end())
        {
            LOG_I("notify DirectCommand Trigger state: %d TriggerID: %d TriggerTime: %lld", pState, pTriggerId, it->second->getTriggerTime());
            isDirectCommandTrigger = true;
            handleTrigger(pState, pTriggerId, dueToIgOff);
        }
        else
        {
            LOG_E("Can not find triggerID in saved list");
            isDirectCommandTrigger = false;
            PriorityControl::getInstance()->notifyTriggerNoFound(pTriggerId);
        }
        return isDirectCommandTrigger;
    }

    void RemoteDirectCommand::handleTrigger(const DiagTrigger::DiagTriggerState &pState,
                                            const int32_t &pTriggerId, const bool dueToIgOff)
    {
        const uint32_t u_pTriggerId{(pTriggerId >= 0) ? static_cast<uint32_t>(pTriggerId) : 0U};
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mSaveReq.find(u_pTriggerId)};
        if (it != mSaveReq.end())
        {
            const android::sp<DiagTrigger> pTrigger{it->second};
            const DiagTrigger::DiagTriggerType pTriggerType{pTrigger->getType()};
            const uint64_t colID{pTrigger->getCollectionID()};
            const uint64_t notiID{pTrigger->getNotificationID()};
            LOG_I("notify DirectCommand Trigger state: %d TriggerID: %d, due to IG off: %d", pState, pTriggerId, dueToIgOff);
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
                this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_RUNNING);
                const uint32_t prio_data{pTrigger->getPriority()};
                LOG_I("TRIGGER_PROCESSING");
                /* DirectCommand can run */
                mTriggerId = pTriggerId;
                mTriggerType = pTriggerType;
                mColId = colID;
                mPriority = prio_data;
                mOccurrenceNotificationId = notiID;
                if (mTriggerType != DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
                {
                    int64_t triggerTime{0};
                    triggerTime = pTrigger->getTriggerTime();
                    mDirectCommandAcquisitionTime = triggerTime;
                }
                else
                {
                    this->mRobNotificationReq = RoBOccurrence::getInstance()->getNotification(mOccurrenceNotificationId);
                    if (mRobNotificationReq != nullptr)
                    {
                        mDirectCommandAcquisitionTime = this->mRobNotificationReq->triggerTime();
                    }
                    else
                    {
                        LOG_E("mRobNotificationReq is nullptr");
                        mDirectCommandAcquisitionTime = 0;
                    }
                }
                if (this->checkPrecondition(pTriggerType, mColId))
                {
                    (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_SEND_REQUEST)->sendToTarget();
                }
                else
                {
                    this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_STOP);
                    this->notifyTriggerDone(this->mTriggerId, true, true, true);
                }

                break;
            }
            case DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED:
            {
                LOG_I("TRIGGER_SUSPENDED");
                if ((this->getAppStatus() == RemoteDirectCommand::DIRECTCOMMAND_RUNNING) && (pTriggerId == this->mTriggerId))
                {
                    mSuspended = true;
                }
                break;
            }
            case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
            {
                LOG_I("TRIGGER_DISCARDED, Trigger cannot be held in the queue");
                if ((this->getAppStatus() == RemoteDirectCommand::DIRECTCOMMAND_RUNNING) && (pTriggerId == this->mTriggerId))
                {
                    if (dueToIgOff == false)
                    {
                        LOG_I("TRIGGER_DISCARDED due to exceeded queue size");
                        RemoteDirectCommand::getInstance()->abortDirectCommand(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED);
                    }
                }
                else
                {
                    vccomif::rdg::v1::interfaces::UploadErrorDataRequest uploadErrorData{};
                    uploadErrorData.set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED);
                    DirectCommandList l_DirectCommand{};
                    bool srvc_ac_disregard_flag{false};
                    this->getDirectCommandRequest(pTriggerType, colID, l_DirectCommand, srvc_ac_disregard_flag);
                    const int64_t current_time{CommonUtils::getCurrentAcquisiteTime()};
                    this->makeErrorUploadData(uploadErrorData, pTriggerType, colID, srvc_ac_disregard_flag, current_time);
                    this->notifyTriggerDone(pTriggerId, true, false, true);
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
            default:
                break;
            }
            (void)pTriggerType;
            (void)colID;
            (void)notiID;
        }
    }

    void RemoteDirectCommand::sendDirectCommand(void)
    {
        //
        l_DirectcommandRequest.Clear();
        mDirectCommandUploadErrorData->Clear();
        mDirectCommandUploadData->Clear();
        this->getDirectCommandRequest(this->mTriggerType, this->mColId, this->l_DirectcommandRequest, this->bsrvc_ac_flag);
        l_EcuInformation.clear();
        l_DirectCommandTrans.clear();
        mDirectCommandTransQueue.clear();
        vErrorIdx.clear();
        // Get Location data
        mLocationData = LocationManagerAdapter::getInstance()->getLocationData();
        LOG_I("Get location data success, latitude: 0x%08X, longtitue 0x%08X", mLocationData->getLatitude(), mLocationData->getLongtitude());
        // Get ODO data
        (void)VehicleManagerAdapter::getInstance()->getOdoInformation(odo_value, odo_unit); // odo value
        RemoteEcuInformation::getInstance()->getEcuInformationList(l_EcuInformation);
        uint32_t cmdIdx{0U};
        for (CollectionCondition::DirectCommandIter it{l_DirectcommandRequest.begin()}; it != l_DirectcommandRequest.end(); it++)
        {
            CommonDefine::EcuInformation ecuInformation{};
            if (validateDirectCommand(*it, ecuInformation))
            {
                const sp<::Buffer> udsReq{new ::Buffer()};
                const std::string commandMess{(*it).command_messages()};
                if (commandMess.size() <= static_cast<uint32_t>(INT32_MAX))
                {
                    udsReq->setTo(commandMess.c_str(), static_cast<int32_t>(commandMess.size()));
                }
                const uint16_t maxUdsReqSize{(this->mTriggerType != DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER) ? static_cast<uint16_t>(4095U) /*4kB*/ : static_cast<uint16_t>(16U)};
                const bool sidAllowed{SidFilter::isAllowed(udsReq->data(), udsReq->size())};
                if ((udsReq->size() > static_cast<uint32_t>(maxUdsReqSize) /*test: 10 */) || (udsReq->size() == 0U) || (!sidAllowed))
                {
                    LOG_D("[sendDirectCommand] udsReq->size exceeded or SID blocked");
                    vErrorIdx.push_back(cmdIdx);
                    vccomif::rdg::v1::interfaces::DiagnosticsMessage error_diagMessage{};
                    error_diagMessage.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_COMMAND_ERROR);
                    vccomif::rdg::v1::interfaces::EcuAddressInformation *const ecuAddressInfo{error_diagMessage.mutable_ecu_address_information()};
                    const CenterCommunicationProtocol protocolType{ecuInformation.getCommProtocol()};
                    ecuAddressInfo->set_communication_protocol(protocolType);
                    if (protocolType != CenterCommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                    {
                        CenterCommunicationType commType{ecuInformation.getCommType()};
                        if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                            (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                        {
                            commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                        }
                        ecuAddressInfo->set_communication_type(commType);
                    }
                    ecuAddressInfo->set_target_address(it->ecu_address_information().target_address());
                    if ((udsReq->size() != 0U))
                    {
                        if (udsReq->data() != nullptr)
                        {
                            error_diagMessage.set_user_data(&udsReq->data()[0], 1U); // Store 1Byte (SID value)
                        }
                        else
                        {
                            LOG_E("udsReq->data() is null");
                        }
                    }
                    mDirectCommandResponse_order[cmdIdx] = std::pair<bool, vccomif::rdg::v1::interfaces::DiagnosticsMessage>(true, error_diagMessage);
                }
                else
                {
                    // for the case initial of mDirectCommandTransQueue
                    if (mDirectCommandTransQueue.empty())
                    {
                        if (ecuInformation.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5)
                        {
                            // add remote session
                            const sp<::Buffer> remoteSessionReq{new ::Buffer()};
                            constexpr uint8_t remoteReqArr[2]{0x10U, 0x40U};
                            remoteSessionReq->setTo(&remoteReqArr[0U], 2);
                            const sp<::Buffer> defaultSesison{new ::Buffer()};
                            constexpr uint8_t defaultArr[2]{0x10U, 0x01U};
                            defaultSesison->setTo(&defaultArr[0U], 2);
                            const android::sp<DirectCommandTransmission> mDirectCommandTransmission{new DirectCommandTransmission(*this, ecuInformation, remoteSessionReq)};
                            mDirectCommandTransmission->pushUdsReqList(std::make_pair(cmdIdx, remoteSessionReq));
                            mDirectCommandTransmission->pushUdsReqList(std::make_pair(cmdIdx, udsReq));
                            mDirectCommandTransmission->pushUdsReqList(std::make_pair(cmdIdx, defaultSesison));
                            mDirectCommandTransQueue.push_back(mDirectCommandTransmission);
                            LOG_I("Target ID: %02X, ECU phase 5, transmission list size: %lu", ecuInformation.getTargetAddress(), mDirectCommandTransQueue.size());
                        }
                        else
                        {
                            const android::sp<DirectCommandTransmission> mDirectCommandTransmission{new DirectCommandTransmission(*this, ecuInformation, udsReq)};
                            mDirectCommandTransmission->pushUdsReqList(std::make_pair(cmdIdx, udsReq));
                            mDirectCommandTransQueue.push_back(mDirectCommandTransmission);
                            LOG_I("Target ID: %02X, transmission list size: %lu", ecuInformation.getTargetAddress(), mDirectCommandTransQueue.size());
                        }
                    }
                    else
                    {
                        const android::sp<DirectCommandTransmission> prevCommand{mDirectCommandTransQueue.back()};
                        std::deque<std::pair<uint32_t, android::sp<Buffer>>> reqList{prevCommand->getUdsReqList()};
                        const android::sp<Buffer> reqFront{reqList.front().second};
                        if (reqFront->data() != nullptr)
                        {
                            LOG_I("Prev Target ID: %02X, Curr Target %02X, Prev SID %02X, Curr SID %02X", prevCommand->ecuInformation().getTargetAddress(), ecuInformation.getTargetAddress(), reqFront->data()[0], udsReq->data()[0]);
                            if ((prevCommand->ecuInformation().getTargetAddress() == ecuInformation.getTargetAddress()) &&
                                ((reqFront->size() >= 2U) &&
                                 (reqFront->data()[0] == 0x10U) &&
                                 (reqFront->data()[1] != 0x01U)))
                            {
                                const android::sp<Buffer> reqBack{reqList.back().second};
                                if (reqBack->data() != nullptr)
                                {
                                    if ((reqFront->size() >= 2U) &&
                                        (reqBack->data()[0] == 0x10U) &&
                                        (reqBack->data()[1] == 0x01U))
                                    {
                                        if (ecuInformation.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5)
                                        {
                                            prevCommand->insertBeforeEnd(std::make_pair(cmdIdx, udsReq));
                                        }
                                        else
                                        {
                                            const android::sp<DirectCommandTransmission> mDirectCommandTransmission{new DirectCommandTransmission(*this, ecuInformation, udsReq)};
                                            mDirectCommandTransmission->pushUdsReqList(std::make_pair(cmdIdx, udsReq));
                                            mDirectCommandTransQueue.push_back(mDirectCommandTransmission);
                                            LOG_I("Target ID: %02X, transmission list size: %lu", ecuInformation.getTargetAddress(), mDirectCommandTransQueue.size());
                                        }
                                    }
                                    else
                                    {
                                        prevCommand->pushUdsReqList(std::make_pair(cmdIdx, udsReq));
                                        LOG_I("Target ID: %02X, ecu phase %d, udsReqList size: %lu", ecuInformation.getTargetAddress(), ecuInformation.getDiagPhase(), reqList.size());
                                    }
                                }
                                else
                                {
                                    LOG_E("reqBack->data() is null");
                                }
                            }
                            else
                            {
                                if (ecuInformation.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5)
                                {
                                    // add remote session
                                    const sp<::Buffer> remoteSessionReq{new ::Buffer()};
                                    constexpr uint8_t remoteReqArr[2]{0x10U, 0x40U};
                                    remoteSessionReq->setTo(&remoteReqArr[0U], 2);
                                    const sp<::Buffer> defaultSesison{new ::Buffer()};
                                    constexpr uint8_t defaultArr[2]{0x10U, 0x01U};
                                    defaultSesison->setTo(&defaultArr[0U], 2);
                                    const android::sp<DirectCommandTransmission> mDirectCommandTransmission{new DirectCommandTransmission(*this, ecuInformation, remoteSessionReq)};
                                    mDirectCommandTransmission->pushUdsReqList(std::make_pair(cmdIdx, remoteSessionReq));
                                    mDirectCommandTransmission->pushUdsReqList(std::make_pair(cmdIdx, udsReq));
                                    mDirectCommandTransmission->pushUdsReqList(std::make_pair(cmdIdx, defaultSesison));
                                    mDirectCommandTransQueue.push_back(mDirectCommandTransmission);
                                    LOG_I("Target ID: %02X, ECU phase 5, transmission list size: %lu", ecuInformation.getTargetAddress(), mDirectCommandTransQueue.size());
                                }
                                else
                                {
                                    const android::sp<DirectCommandTransmission> mDirectCommandTransmission{new DirectCommandTransmission(*this, ecuInformation, udsReq)};
                                    mDirectCommandTransmission->pushUdsReqList(std::make_pair(cmdIdx, udsReq));
                                    mDirectCommandTransQueue.push_back(mDirectCommandTransmission);
                                    LOG_I("Target ID: %02X, transmission list size: %lu", ecuInformation.getTargetAddress(), mDirectCommandTransQueue.size());
                                }
                            }
                        }
                        else
                        {
                            LOG_E("reqList.front().second->data() is null");
                        }
                    }
                }
            }
            else
            {
                vccomif::rdg::v1::interfaces::DiagnosticsMessage diagMessage{};
                vccomif::rdg::v1::interfaces::EcuAddressInformation *const ecuAddressInfo{diagMessage.mutable_ecu_address_information()};
                diagMessage.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_ECU_SPECIFIED_ERROR);
                ecuAddressInfo->set_target_address(0xFFFFFFFFU);
                if ((it->command_messages().size() != 0U))
                {
                    if (it->command_messages().c_str() != nullptr)
                    {
                        diagMessage.set_user_data(it->command_messages().c_str(), it->command_messages().size()); // Store 1Byte (SID value)
                    }
                    else
                    {
                        LOG_E("bDirectCommandReq->data() is null");
                    }
                }
                else
                {
                    LOG_E("bDirectCommandReq is null");
                }
                // mDirectCommandResponse.push_back(diagMessage);
                mDirectCommandResponse_order[cmdIdx] = std::pair<bool, vccomif::rdg::v1::interfaces::DiagnosticsMessage>(true, diagMessage);
                // Remove invalid command
                vErrorIdx.push_back(cmdIdx);
            }
            if (cmdIdx < UINT32_MAX)
            {
                cmdIdx++;
            }
        }
        (void)cmdIdx;
        if (l_EcuInformation.size() == 0U)
        {
            LOG_E("ECU list is none");
            DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
        }
        if (mDirectCommandTransQueue.size() > 0U)
        {
            if (mSuspended == true)
            {
                suspendDirectCommand();
            }
            else
            {
                this->startUp();
            }
        }
        else
        {
            LOG_E("l_DirectCommandTrans is empty");
            this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_STOP);
            this->finishDirectCommand();
            this->notifyTriggerDone(this->mTriggerId, true, true, true);
        }
        if ((this->mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER) && (vErrorIdx.size() > 0U))
        {
            CollectionCondition::getInstance().removeErrorDirectCommand(this->mColId, vErrorIdx);
        }
        else
        {
            vErrorIdx.clear();
        }
    }

    void RemoteDirectCommand::finishCurrentTransmission(void)
    {
        LOG_D("finish Current transmission ID: 0x%02x, mDirectCommandTransQueue size = %lu", mCurrentTransmissionId, mDirectCommandTransQueue.size());
        /* TBD: Delete finished DTCUdsTransmission in mapl_DirectCommandTrans */
        if (mDirectCommandTransQueue.size() > 1U)
        {
            mDirectCommandTransQueue.pop_front();
            this->mCurrDirectCommandTrans = mDirectCommandTransQueue.front();
            this->mCurrDirectCommandTrans->connect();
        }
        else
        {
            if (!mDirectCommandTransQueue.empty())
            {
                mDirectCommandTransQueue.pop_front();
            }

            // Release OBC resource
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_STOP);
            LOG_I("mDirectCommandTransQueue size = %lu ->  finished the last ECU -> upload package", mDirectCommandTransQueue.size());
            this->finishDirectCommand();
            this->notifyTriggerDone(this->mTriggerId, true, true, true);
            // l_DirectCommandTrans.clear();
        }
    }

    void RemoteDirectCommand::finishDirectCommand(void)
    {
        /*Generate upload data*/
        // const bool isVehicleTrigger{(mTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) || (mTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER)};
        if (mDirectCommandResponse_order.size() != 0U)
        {
            makeUploadData();
        }
        if (bUploadErrorData == true)
        {
            this->makeErrorUploadData(*(this->mDirectCommandUploadErrorData), this->mTriggerType, this->mColId, this->bsrvc_ac_flag, this->mDirectCommandAcquisitionTime);
            bUploadErrorData = false;
        }
        mDirectCommandTransQueue.clear();

        if (!mDirectCommandResponse_order.empty())
        {
            mDirectCommandResponse_order.clear();
        }
        LOG_I("Finish DirectCommand Acquisition triggerID: %d", mTriggerId);
    }

    void RemoteDirectCommand::notifyTriggerDone(const int32_t pTriggerId, const bool isNotify, const bool isCompletedRob, const bool isRemoveTrigger)
    {
        mSuspended = false;
        setAbortingUnderRepair(false);
        const uint32_t u_pTriggerId{(pTriggerId >= 0) ? static_cast<uint32_t>(pTriggerId) : 0U};
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mSaveReq.find(u_pTriggerId)};
        if (it != mSaveReq.end())
        {
            /* Get current time */
            if (isNotify)
            {
                const DiagTrigger::DiagTriggerType pTriggerType{it->second->getType()};
                if ((pTriggerType >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
                    (pTriggerType <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX))
                {
                    LOG_I("Notify DTC Trigger done: %d", pTriggerId);
                }
                else
                {
                    LOG_I("Trigger type is out of range");
                }
                // TimeManager &mTimeManagerService{TimeManager::getInstance()};
                PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            }
            if ((isCompletedRob == true) && (it->second->getType() == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER))
            {
                RoBOccurrence::getInstance()->onDirectCommandCompleted(isCompletedRob, it->second->getNotificationID());
            }
            if (isRemoveTrigger)
            {
                (void)mSaveReq.erase(it);
            }
        }
        else
        {
            LOG_D("Can not find triggerId: %ld in mSaveReq", pTriggerId);
        }
        LOG_D("Check mSaveReq size after: %lu, trigger id: %ld", mSaveReq.size(), pTriggerId);
    }

    void RemoteDirectCommand::makeUploadData(void)
    {
        LOG_I("Make DirectCommand upload data: Size of response: %lu", mDirectCommandResponse_order.size());
        mDirectCommandUploadData->Clear();
        this->makeHeaderForUploading(mDirectCommandUploadData, true);
        // split file
        // std::vector<vccomif::rdg::v1::interfaces::DiagnosticsMessage>::iterator it_ResData{mDirectCommandResponse.begin()};
        std::map<uint32_t, std::pair<bool, vccomif::rdg::v1::interfaces::DiagnosticsMessage>>::iterator it_ResData{mDirectCommandResponse_order.begin()};
        for (uint32_t i{0U}; i < MAX_SPLIT_FILE; i++)
        {
            LOG_V("Current file: %u", i);
            UploadDirectCommandDataRequest pUploadDirectCommandDataRequest{};
            pUploadDirectCommandDataRequest.CopyFrom(*mDirectCommandUploadData);
            uint32_t sizeOfFile{0U};
            sizeOfFile = pUploadDirectCommandDataRequest.ByteSizeLong();
            while (it_ResData != mDirectCommandResponse_order.end())
            {
                if (it_ResData->second.first == true)
                {
                    vccomif::rdg::v1::interfaces::DiagnosticsMessage *const diagMessage{pUploadDirectCommandDataRequest.add_direct_command_messages()};
                    diagMessage->CopyFrom(it_ResData->second.second);
                    sizeOfFile = pUploadDirectCommandDataRequest.ByteSizeLong();
                    LOG_V("Current sizeOfFile %u", sizeOfFile);
                    if ((sizeOfFile <= DIRECTCOMMAND_UPLOAD_DATA_SIZE_MAX) /* test: 55 + X */ && (i < MAX_SPLIT_FILE - 1U)) // 4M - 1st and 2nd file //
                    {
                        it_ResData++;
                    }
                    else if ((sizeOfFile <= DIRECTCOMMAND_LAST_UPLOAD_DATA_SIZE_MAX) /* test: 55 + X */ && (i == MAX_SPLIT_FILE - 1U)) // 2M - last file
                    {
                        it_ResData++;
                    }
                    else
                    {
                        pUploadDirectCommandDataRequest.mutable_direct_command_messages()->RemoveLast();
                        LOG_V("Removed last message due to oversize, Size of pUploadDirectCommandDataRequest = %u", pUploadDirectCommandDataRequest.ByteSizeLong());
                        break;
                    }
                    (void)DIRECTCOMMAND_LAST_UPLOAD_DATA_SIZE_MAX;
                }
                else
                {
                    it_ResData++;
                    LOG_V(" it_ResData->second.first false");
                }
            }
            (void)sizeOfFile;
            l_UploadDirectCommandDataRequest.push_back(pUploadDirectCommandDataRequest);
            if (it_ResData == mDirectCommandResponse_order.end())
            {
                break;
            }
        }
        (void)it_ResData;
        for (size_t i{0U}; i < l_UploadDirectCommandDataRequest.size(); i++)
        {
            l_UploadDirectCommandDataRequest[i].set_split_counter(i + 1U);
            if (i == l_UploadDirectCommandDataRequest.size() - 1U)
            {
                l_UploadDirectCommandDataRequest[i].set_last_split_data_flag(true);
            }
            else
            {
                l_UploadDirectCommandDataRequest[i].set_last_split_data_flag(false);
            }
        }

        std::vector<UploadDirectCommandDataRequest>::iterator it{};
        for (it = l_UploadDirectCommandDataRequest.begin(); it != l_UploadDirectCommandDataRequest.end(); ++it)
        {
            std::string str_directCommand_upload{""};
            google::protobuf::util::JsonOptions option{};
            option.always_print_primitive_fields = true;
            option.preserve_proto_field_names = true;
            (void)google::protobuf::util::MessageToJsonString(*it, &str_directCommand_upload, option);
            printData(str_directCommand_upload);
        }

        /*Save UploadDirectCommandDataRequest to file*/
        for (it = l_UploadDirectCommandDataRequest.begin(); it != l_UploadDirectCommandDataRequest.end(); ++it)
        {
            const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
            const uint64_t uploadCount{UploadManager::getInstance()->genCountUpload()};
            std::string file_dir{std::to_string(uploadCount)};
            (void)file_dir.append("_UploadDirectCommandDataRequest.dat");
            uint32_t fileSize{0U};
            error_t bStored{E_OK};
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if (region == LGE_REGION::LGE_REGION_CN)
            {
                bStored = DataModel<UploadDirectCommandDataRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG070, file_dir, *it, fileSize);
            }
            else
            {
                fileSize = it->ByteSizeLong();
                bStored = DataModel<UploadDirectCommandDataRequest>::saveUpload(file_dir, *it);
            }

            if (bStored == E_OK)
            {
                const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
                // Self-Diag
                DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
                const android::sp<UploadTask> task{new UploadTask(uploadId)};
                task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG070);
                task->setUploadPatch(file_dir);
                task->setUploadId(static_cast<uint64_t>(uploadId));
                /*Set priority*/
                task->setUploadPrio(mPriority);
                task->setSrvcAcFlag(this->bsrvc_ac_flag);
                task->setFileSize(static_cast<uint64_t>(fileSize));
                UploadManager::getInstance()->requestUploadTask(task);
            }
            else
            {
                LOG_E("Failed to store");
            }
            (void)fileSize;
            (void)uploadId;
            // test_saveUploadData = task;
        }
        mDirectCommandUploadData->Clear();
        l_UploadDirectCommandDataRequest.clear();
        int64_t current_time{0};
        current_time = CommonUtils::getCurrentAcquisiteTime();
        LOG_W("Collection ID: %llu, number of response: %u, - Last complete time: %lld - trigger Type: %d", mColId, mDirectCommandResponse_order.size(), current_time, mTriggerType);
        CollectionCondition::getInstance().onFinishCenterRequestJob(this->mColId);
        if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
        {
            mApp.notifyLastOpComplTime(mColId, current_time, true);
        }
    }

    void RemoteDirectCommand::createDiagnosticsData(vccomif::rdg::v1::interfaces::DiagnosticsMessage &diagMessage, const android::sp<DirectCommandTransmission> directCommandReq, const android::sp<OBCResponseEventInfo> responseEventInfo) const
    {
        vccomif::rdg::v1::interfaces::EcuAddressInformation *const ecuAddressInfo{diagMessage.mutable_ecu_address_information()};
        const CenterCommunicationProtocol commProtocol{directCommandReq->ecuInformation().getCommProtocol()};
        ecuAddressInfo->set_communication_protocol(commProtocol);
        if (commProtocol != CenterCommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
        {
            const CenterCommunicationType diagResType{directCommandReq->ecuInformation().getCommType()};
            ecuAddressInfo->set_communication_type(diagResType);
        }
        const uint32_t centerTxAdd{directCommandReq->ecuInformation().getTargetAddress()};
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
        const android::sp<::Buffer> resData{responseEventInfo->resInfo()->udsData()};
        const android::sp<UdsMessage> udsResponse{new UdsMessage()};
        if (resData->size() > 0U)
        {
            (void)udsResponse->Parser(resData);
        }
        if (responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_OK)
        {
            (void)centerTxAdd;
            diagMessage.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL);
            ecuAddressInfo->set_target_address(centerRxAdd);
            if (resData->data() != nullptr)
            {
                diagMessage.set_user_data(resData->data(), resData->size());
            }
            else
            {
                LOG_E("udsData()->data() is nullptr");
            }
        }
        else if ((responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_NEGATIVE) && (udsResponse->getNRC() != 0x78U))
        {
            (void)centerTxAdd;
            diagMessage.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL_WITH_NEGATIVE);
            ecuAddressInfo->set_target_address(centerRxAdd);
            if (resData->data() != nullptr)
            {
                diagMessage.set_user_data(resData->data(), resData->size());
            }
            else
            {
                LOG_E("udsData()->data() is nullptr");
            }
        }
        else
        {
            (void)centerRxAdd;
            diagMessage.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_UNRESPONSIVE);
            ecuAddressInfo->set_target_address(centerTxAdd);
            const android::sp<::Buffer> udsRequest{directCommandReq->getUdsReq().second};
            if (udsRequest->data() != nullptr)
            {
                diagMessage.set_user_data(&udsRequest->data()[0], udsRequest->size());
            }
            else
            {
                LOG_E("udsReq->data() is null");
            }
        }

        // print data
        std::string std_diagMessage{""};
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        (void)google::protobuf::util::MessageToJsonString(diagMessage, &std_diagMessage, option);
        printData(std_diagMessage);
    }

    void RemoteDirectCommand::changedRemoteStatus(const uint8_t what, const uint32_t info)
    {
        switch (what)
        {
        case RemoteDirectCommand::WHAT_CHANGED_REPAIR_SATUS:
        {
            LOG_I("WHAT_CHANGED_REPAIR_SATUS");
            if ((this->getAppStatus() == RemoteDirectCommand::DIRECTCOMMAND_RUNNING) && (info == 1U))
            {
                setAbortingUnderRepair(true);
            }
            break;
        }
        default:
        {
            break;
        }
        }
    }

    void RemoteDirectCommand::makeErrorUploadData(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colId, const bool srvc_ac_flag, const int64_t occurredTime)
    {
        LOG_I("Make DirectCommand Error data");
        errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());

        vccomif::rdg::v1::interfaces::RdgCommonRequestHeader *const mRdgCommonRequestHeader{errorData.mutable_rdg_common_request_header()};
        vccomif::common::v1::AppCommonHeaderVehicleToCenter *const mAppCommonHeaderVehicleToCenter{mRdgCommonRequestHeader->mutable_app_common_header()};

        mAppCommonHeaderVehicleToCenter->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
        mAppCommonHeaderVehicleToCenter->set_electronic_pf(EPF_19EPF);
        mAppCommonHeaderVehicleToCenter->set_geodesy_information(CommonUtils::getGeodesyInfo());

        // Interface type
        mRdgCommonRequestHeader->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
        mRdgCommonRequestHeader->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
        mRdgCommonRequestHeader->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());

        const sp<CommonDefine::RDGLocationData> mLocation{LocationManagerAdapter::getInstance()->getLocationData()};
        errorData.set_collection_condition_id(colId); /*TBD*/
        mRdgCommonRequestHeader->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, UploadManager::getInstance()->getCounterMessage()));
        errorData.set_counter_value(UploadManager::getInstance()->getCounterValue());
        // set error trigger type
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

        // data_creation_date
        if (occurredTime >= 0)
        {
            errorData.set_data_creation_date(static_cast<uint64_t>(occurredTime));
        }
        else
        {
            errorData.set_data_creation_date(0xFFFFFFFFFFU);
        }
        // Set OBD2 flag
        const bool bOBDFlag{mApp.getOBDStatus()};
        errorData.set_obd2_installed_flag(bOBDFlag);
        errorData.set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
        errorData.set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_DIRECT_COMMAND);
        std::string str_err_directCommand_upload{""};
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        (void)google::protobuf::util::MessageToJsonString(errorData, &str_err_directCommand_upload, option);
        printData(str_err_directCommand_upload);

        const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
        const uint64_t uploadCount{UploadManager::getInstance()->genCountUpload()};
        std::string file_dir{std::to_string(uploadCount)};
        (void)file_dir.append("_UploadErrorDirectCommandDataRequest.dat");
        uint32_t fileSize{0U};
        error_t bStored{E_OK};
        const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
        if (region == LGE_REGION::LGE_REGION_CN)
        {
            bStored = DataModel<UploadErrorDataRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG160, file_dir, errorData, fileSize);
        }
        else
        {
            fileSize = errorData.ByteSizeLong();
            bStored = DataModel<UploadErrorDataRequest>::saveUpload(file_dir, errorData);
        }
        if (bStored == E_OK)
        {
            const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
            // Self-Diag
            DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
            const android::sp<UploadTask> task{new UploadTask(uploadId)};
            task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
            task->setUploadPatch(file_dir);
            task->setUploadId(static_cast<uint64_t>(uploadId));
            /*Set priority*/
            task->setUploadPrio(mPriority);
            task->setSrvcAcFlag(srvc_ac_flag);
            task->setFileSize(static_cast<uint64_t>(fileSize));
            UploadManager::getInstance()->requestUploadTask(task);
        }
        else
        {
            LOG_E("Failed to store");
        }
        (void)fileSize;
        (void)uploadId;
        mDirectCommandUploadErrorData->Clear();
        int64_t current_time{0};
        current_time = CommonUtils::getCurrentAcquisiteTime();
        LOG_W("Make error data: Collection ID: %llu - Last complete time: %lld - Trigger Type: %d", mColId, current_time, mTriggerType);
        if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
        {
            mApp.notifyLastOpComplTime(mColId, current_time, false);
        }
        CollectionCondition::getInstance().onFinishCenterRequestJob(colId);
    }

    void RemoteDirectCommand::abortDirectCommand(const vccomif::rdg::v1::interfaces::ResponseCode resCode)
    {
        if (this->getAppStatus() == RemoteDirectCommand::DIRECTCOMMAND_RUNNING)
        {
            this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_STOP);
            LOG_E("code: %d ", resCode);
            if ((mCurrDirectCommandTrans != nullptr) && (mCurrDirectCommandTrans->getState() != DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DONE))
            {
                this->mTimeOutTrans->stop();
                mCurrDirectCommandTrans->disconnect();
            }
            if (!mDirectCommandTransQueue.empty())
            {
                mDirectCommandTransQueue.clear();
            }
            if (mDirectCommandResponse_order.size() != 0U)
            {
                mDirectCommandResponse_order.clear();
            }
            bUploadErrorData = true;
            bool isNotifyRoBDone{false};
            if (resCode == vccomif::rdg::v1::interfaces::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR)
            {
                setAbortingUnderRepair(false);
                isNotifyRoBDone = true;
            }
            mDirectCommandUploadErrorData->set_response_code(resCode);
            this->finishDirectCommand();
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            this->notifyTriggerDone(this->mTriggerId, true, isNotifyRoBDone, true);
        }
    }

    void RemoteDirectCommand::makeHeaderForUploading(const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequest> &directCommandUploadData, const bool isIncreaseCount)
    {
        LOG_I("Make DirectCommand Header upload data");
        // RdgCommonRequestHeader

        vccomif::rdg::v1::interfaces::RdgCommonRequestHeader *const mRdgCommonRequestHeader{directCommandUploadData->mutable_rdg_common_request_header()};
        vccomif::common::v1::AppCommonHeaderVehicleToCenter *const mAppCommonHeaderVehicleToCenter{mRdgCommonRequestHeader->mutable_app_common_header()};

        mAppCommonHeaderVehicleToCenter->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
        mAppCommonHeaderVehicleToCenter->set_electronic_pf(EPF_19EPF);
        mAppCommonHeaderVehicleToCenter->set_geodesy_information(CommonUtils::getGeodesyInfo());
        // Interface type
        mRdgCommonRequestHeader->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_DIRECT_COMMAND_DATA);

        // Set collection condition ID of occurrence RoB (fixed value) in the case of occurrence RoB   directCommandUploadData->set_collection_condition_id(ixd);
        directCommandUploadData->set_collection_condition_id(mColId);
        // messageId
        // set message id use the time when creating file
        mRdgCommonRequestHeader->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
        mRdgCommonRequestHeader->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());
        if (isIncreaseCount)
        {
            mRdgCommonRequestHeader->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_DIRECT_COMMAND_DATA, UploadManager::getInstance()->getCounterMessage()));
            directCommandUploadData->set_counter_value(UploadManager::getInstance()->getCounterValue());
        }
        else
        {
            mRdgCommonRequestHeader->set_message_id("01-20231231154859-05"); // temporary set default
            directCommandUploadData->set_counter_value(0U);                  // temporary set default
        }
        // Set Time data diagnostics_acquisition_time
        // Set Location data *location

        vccomif::rdg::v1::interfaces::Location *const mLocation{directCommandUploadData->mutable_location()};
        mDirectCommandAcquisitionTime = (mDirectCommandAcquisitionTime >= 0) ? mDirectCommandAcquisitionTime : 0xFFFFFFFFFF;
        if (mTriggerType != DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
        {
            directCommandUploadData->set_diagnostics_acquisition_time(static_cast<uint64_t>(mDirectCommandAcquisitionTime));
            try
            {
                mLocation->set_latitude(mLocationData->getLatitude());
                mLocation->set_longitude(mLocationData->getLongtitude());
            }
            catch (const std::exception &e)
            {
                LOG_D("Exception caught: %s", e.what());
            }
            LOG_D("Get Current Time %lld, Lattitude: %d, Longtitude: %d", mDirectCommandAcquisitionTime, mLocationData->getLatitude(), mLocationData->getLongtitude());
        }
        else
        {
            directCommandUploadData->set_diagnostics_acquisition_time(static_cast<uint64_t>(mDirectCommandAcquisitionTime));
            try
            {
                mLocation->set_latitude(this->mRobNotificationReq->triggerLocation()->getLatitude());
                mLocation->set_longitude(this->mRobNotificationReq->triggerLocation()->getLongtitude());
            }
            catch (const std::exception &e)
            {
                LOG_D("Exception caught: %s", e.what());
            }
            LOG_D("Get Current Time success: %lld", this->mRobNotificationReq->triggerTime());
            LOG_D("Get location data success, latitude: 0x%08X, longitude: 0x%08X", this->mRobNotificationReq->triggerLocation()->getLatitude(), this->mRobNotificationReq->triggerLocation()->getLongtitude());
            vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequest_RoBInformation *const mRoBInformation{directCommandUploadData->mutable_rob_information()};
            mRoBInformation->set_rob(mRobNotificationReq->getTargetCollectionDataRobSsr().rob_information().rob());
            vccomif::rdg::v1::interfaces::EcuAddressInformation *const ecuInformation{mRoBInformation->mutable_ecu_address_information()};
            ecuInformation->CopyFrom(mRobNotificationReq->getTargetCollectionDataRobSsr().ecu_address_information());
            mRoBInformation->set_memory_selection(mRobNotificationReq->getTargetCollectionDataRobSsr().rob_information().memory_selection());
        }

        // Set OBD2 flag
        const bool bOBDFlag{mApp.getOBDStatus()};
        directCommandUploadData->set_obd2_installed_flag(bOBDFlag);
        // Set ODO data
        if (odo_unit == 0x2U)
        {
            directCommandUploadData->set_odo_information_mile(odo_value); // odo_unit = 10b ~ Mile
        }
        else if (odo_unit == 0x1U)
        {
            directCommandUploadData->set_odo_information_km(odo_value); // odo_unit = 01b ~ km
        }
        else
        {
            directCommandUploadData->set_odo_information_km(0xFFFFFFFFU); // RDG30-R-1050 undefined value
        }
        directCommandUploadData->set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
        directCommandUploadData->set_split_counter(1U);
        directCommandUploadData->set_last_split_data_flag(true);
    }

    void RemoteDirectCommand::suspendDirectCommand()
    {
        LOG_I("[suspendDirectCommand]");
        if (this->getAppStatus() == RemoteDirectCommand::DIRECTCOMMAND_RUNNING)
        {
            this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_STOP);

            if ((mCurrDirectCommandTrans != nullptr) && (mCurrDirectCommandTrans->getState() != DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DONE))
            {
                this->mTimeOutTrans->stop();
                mCurrDirectCommandTrans->disconnect();
            }
            mDirectCommandResponse_order.clear();
            bUploadErrorData = false;
            this->finishDirectCommand();
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            this->notifyTriggerDone(this->mTriggerId, true, false, false);
            mSuspended = false;
            int64_t current_time{0};
            current_time = CommonUtils::getCurrentAcquisiteTime();
            LOG_I("Collection ID: %llu - Last complete time: %lld - trigger type: %d", mColId, current_time, mTriggerType);
            if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
            {
                mApp.notifyLastOpComplTime(mColId, current_time, false);
            }
        }
    }

    RemoteDirectCommand::MainHandler::MainHandler(android::sp<sl::SLLooper> &privateLooper, RemoteDirectCommand &obj) noexcept : sl::Handler(privateLooper), mDirectCommand(obj)
    {
    }

    void RemoteDirectCommand::MainHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
    {
        const int32_t what{handlemsg->what};
        LOG_I("handler is processing with what: %d", what);
        switch (what)
        {
        case CMD_DIRECTCOMMAND_INIT:
        {
            LOG_I("CMD_INIT_DIRECTCOMMAND");
            RemoteDirectCommand::getInstance()->init();
            RemoteDirectCommand::getInstance()->initTimer();
            break;
        }
        case CMD_DIRECTCOMMAND_START_UP:
        {
            RemoteDirectCommand::getInstance()->startUp();
            break;
        }
        case CMD_DIRECTCOMMAND_UPLOAD_DATA:
        {
            break;
        }
        case CMD_DIRECTCOMMAND_GET_OBC_RESOURCE:
        {
            break;
        }
        case CMD_DIRECTCOMMAND_EVENT_CONNECT:
        {
            break;
        }
        case CMD_DIRECTCOMMAND_SEND_UDS:
        {
            RemoteDirectCommand::getInstance()->mCurrDirectCommandTrans->sendUds();
            break;
        }
        case CMD_DIRECTCOMMAND_EVENT_UDS_RES:
        {
            android::sp<OBCResponseEventInfo> responseEventInfo{nullptr};
            handlemsg->getObject(responseEventInfo);
            if (responseEventInfo != nullptr)
            {
                RemoteDirectCommand::getInstance()->handleUdsResponse(responseEventInfo);
            }
            break;
        }
        case CMD_DIRECTCOMMAND_EVENT_DISCONNECT:
        {
            break;
        }
        case CMD_CHANGE_IG_STATUS:
        {
            LOG_I("CMD_CHANGE_IG_STATUS: IG status: %d ", handlemsg->arg1);
            if (handlemsg->arg1 == 0)
            {
                RemoteDirectCommand::getInstance()->startTimer(DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_IG_OFF_TRIGGER);
                RemoteDirectCommand::getInstance()->abortDirectCommand(vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
            }
            else if (handlemsg->arg1 == 1)
            {
                RemoteDirectCommand::getInstance()->stopTimer(DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_IG_OFF_TRIGGER);
                RemoteDirectCommand::getInstance()->abortDirectCommand(vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
            }
            else
            {
                // Do nothing
            }
            break;
        }
        case CMD_DIRECTCOMMAND_CENTER_REQUEST:
        {
            LOG_V("CMD_DIRECTCOMMAND_CENTER_REQUEST");
            /* Get current time */
            // TimeManager &mTimeManagerService{TimeManager::getInstance()};
            int64_t current_time{0};
            current_time = CommonUtils::getCurrentAcquisiteTime();
            /* Get priority from Center Request */
            uint32_t prio_tmp{0U};
            if (handlemsg->arg1 > 0)
            {
                prio_tmp = static_cast<uint32_t>(handlemsg->arg1);
            }
            /* Get collection condition ID*/
            const android::sp<::Buffer> buf{new ::Buffer(handlemsg->buffer)};
            uint64_t colId{0U};
            if (buf->data() != nullptr)
            {
                (void)memcpy(&colId, buf->data(), sizeof(uint64_t));
            }
            else
            {
                LOG_E("buf->data() is null");
            }
            LOG_I("Check Col ID: %llu", colId);
            RemoteDirectCommand::getInstance()->triggerDirectCommand(DiagTrigger::DiagTriggerType::CENTER_TRIGGER, prio_tmp, colId, current_time);
            break;
        }
        case CMD_DIRECTCOMMAND_COLLECTION_CONDITION:
        {
            LOG_V("CMD_DIRECTCOMMAND_COLLECTION_CONDITION");
            /* Get current time */
            // TimeManager &mTimeManagerService{TimeManager::getInstance()};
            int64_t current_time{0};
            current_time = CommonUtils::getCurrentAcquisiteTime();
            /* Get priority from Center Request */
            uint32_t prio_tmp{0U};
            if (handlemsg->arg1 > 0)
            {
                prio_tmp = static_cast<uint32_t>(handlemsg->arg1);
            }
            /* Get collection condition ID*/
            const android::sp<::Buffer> buf{new ::Buffer(handlemsg->buffer)};
            uint64_t colId{0U};
            if (buf->data() != nullptr)
            {
                (void)memcpy(&colId, buf->data(), sizeof(uint64_t));
            }
            else
            {
                LOG_E("buf->data() is null");
            }
            LOG_I("Check Col ID: %llu", colId);
            RemoteDirectCommand::getInstance()->triggerDirectCommand(DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER, prio_tmp, colId, current_time);
            break;
        }
        case CMD_DIRECTCOMMAND_REQUEST_TO_PRIORITY_CONTROL:
        {
            LOG_V("CMD_DIRECTCOMMAND_REQUEST_TO_PRIORITY_CONTROL");
            // check trigger type ROUNTER or CENTER
            sp<DiagTrigger> pTrigger{nullptr};
            handlemsg->getObject(pTrigger);
            if (pTrigger != nullptr)
            {
                LOG_I("Trigger Request have Type: %d Func: %d Prio: %d ID: %d ColID: %llu",
                      pTrigger->getType(), pTrigger->getFunc(), pTrigger->getPriority(), pTrigger->getTriggerId(), pTrigger->getCollectionID());
                PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
            }
            else
            {
                LOG_E("CMD_DIRECTCOMMAND_REQUEST_TO_PRIORITY_CONTROL: pTrigger is null");
            }
            break;
        }
        case CMD_DIRECTCOMMAND_SEND_REQUEST:
        {
            LOG_I("CMD_DIRECTCOMMAND_SEND_REQUEST");
            RemoteDirectCommand::getInstance()->sendDirectCommand();
            break;
        }
        case CMD_DIRECTCOMMAND_RECEIVE_UNDER_REPAIR_FLAG_CHANGE:
        {
            LOG_I("CMD_DIRECTCOMMAND_RECEIVE_UNDER_REPAIR_FLAG_CHANGE");
            uint8_t id{0x00U};
            if ((handlemsg->arg1 > 0) && (handlemsg->arg1 <= UINT8_MAX))
            {
                id = static_cast<uint8_t>(handlemsg->arg1);
            }
            uint32_t info{0U};
            if (handlemsg->arg2 > 0)
            {
                info = static_cast<uint32_t>(handlemsg->arg2);
            }
            RemoteDirectCommand::getInstance()->changedRemoteStatus(id, info);
            break;
        }
        case CMD_DIRECTCOMMAND_FINISH_TRANSMISSION:
        {
            LOG_I("CMD_DIRECTCOMMAND_FINISH_TRANSMISSION");
            RemoteDirectCommand::getInstance()->finishCurrentTransmission();
            break;
        }
        case CMD_DIRECTCOMMAND_TIMEOUT_TRANSMISSION:
        {
            LOG_V("CMD_DIRECTCOMMAND_TIMEOUT_TRANSMISSION");
            RemoteDirectCommand::getInstance()->onTransmissionTimeout();
            break;
        }
        case CMD_DIRECTCOMMAND_TIMEOUT_IGOFF:
        {
            LOG_I("CMD_DIRECTCOMMAND_TIMEOUT_IGOFF");
            RemoteDirectCommand::getInstance()->abortDirectCommand(vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
            break;
        }
        case CMD_STOP_RDG:
        {
            const int32_t isStop{handlemsg->arg1};
            if (isStop == 1)
            {
                LOG_I("CMD_STOP_RDG");
                RemoteDirectCommand::getInstance()->handleStopRDG();
            }
            else
            {
                LOG_I("Power source enable RDG");
            }
            break;
        }
        case CMD_DIRECTCOMMAND_OCCURRENCE_REQUEST:
        {
            LOG_I("CMD_DIRECTCOMMAND_OCCURRENCE_REQUEST");
            android::sp<OccurrentRobNotification> notification{nullptr};
            handlemsg->getObject(notification);
            if (notification != nullptr)
            {
                const int32_t prio{notification->priority()};
                if (prio >= 0)
                {
                    RemoteDirectCommand::getInstance()->triggerDirectCommand(DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER, static_cast<uint32_t>(prio), notification->collectionConditionId(), notification->triggerTime(), notification->getId());
                }
                else
                {
                    LOG_E("prio < 0");
                }
            }
            else
            {
                LOG_E("CMD_DIRECTCOMMAND_OCCURRENCE_REQUEST: notification is null");
            }

            break;
        }
        default:
        {
            break;
        }
        }
    }

    void RemoteDirectCommand::DirectCommandTimerHandler::handlerFunction(const int32_t timerId)
    {
        LOG_I("Time out id %ld", timerId);
        switch (timerId)
        {
        case DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_IG_ON_TRIGGER:
        {
            // (void)RemoteDirectCommand::getInstance()->mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_IGON)->sendToTarget();
            break;
        }
        case DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_TRANSMISSION_TIMEOUT:
        {
            (void)RemoteDirectCommand::getInstance()->mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_TIMEOUT_TRANSMISSION)->sendToTarget();
            break;
        }
        case DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_IG_OFF_TRIGGER:
        {
            (void)RemoteDirectCommand::getInstance()->mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_TIMEOUT_IGOFF)->sendToTarget();
            break;
        }
        default:
        {
            break;
        }
        }
    }

    RemoteDirectCommand::DirectCommandTransmission::DirectCommandTransmission(const RemoteDirectCommand &obj, const CommonDefine::EcuInformation ecuInformation, const sp<::Buffer> udsReq)
        : android::RefBase(),
          mDirectCommand(obj),
          mConnectId(0U),
          mState(State::DIRECTCOMMAND_TRANS_INIT),
          bDefaultSession(false)
    {
        this->mEcuInformation.setTargetAddress(ecuInformation.getTargetAddress());
        this->mEcuInformation.setCanId(ecuInformation.getCanId());
        this->mEcuInformation.setecuActiveFlag(ecuInformation.getecuActiveFlag() == true ? true : false);
        this->mEcuInformation.setCommProtocol(ecuInformation.getCommProtocol());
        this->mEcuInformation.setCommType(ecuInformation.getCommType());
        const CommonDefine::DiagPhase diagPhase{ecuInformation.getDiagPhase()};

        if ((diagPhase >= CommonDefine::DiagPhase::DP_UNKNOW) && (diagPhase <= CommonDefine::DiagPhase::DP_PHASE_6))
        {
            this->mEcuInformation.setDiagPhase(diagPhase);
        }
        else
        {
            LOG_E("Unknown diag phase");
        }

        this->mUdsReq = new ::Buffer();
        if (udsReq->size() != 0U)
        {
            this->mUdsReq->setTo(*(udsReq.get()));
        }
        else
        {
            LOG_I("[DirectCommandTransmission] UDS req is nullptr");
        }
        LOG_I("[DirectCommandTransmission] DirectCommandcanId %02X, commProtocol %02X, commType %02X", this->mEcuInformation.getTargetAddress(), this->mEcuInformation.getCommProtocol(), this->mEcuInformation.getCommType());
    }

    void RemoteDirectCommand::DirectCommandTransmission::connect(void)
    {
        LOG_I("DirectCommandTransmission::connect()");
        const android::sp<OBCTransportInfo> transportInfo{new OBCTransportInfo()};
        const android::sp<OBCConnectInfo> ConnectInfo{new OBCConnectInfo()};
        const uint8_t protocolType{RemoteEcuInformation::getInstance()->convertObcProtocolType(this->mEcuInformation.getCommProtocol(), this->mEcuInformation.getCommType(), this->mEcuInformation.getTargetAddress())};
        OBCCanInfo canInfo{};
        std::vector<std::string> ntaArray{};
        ntaArray.clear();
        if ((protocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
            (protocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
            (protocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
        {
            const uint16_t nTa{static_cast<uint16_t>(((this->mEcuInformation.getTargetAddress() >> 8U) & 0xFFU))};
            std::stringstream ss{};
            ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << nTa;
            const std::string hexString{ss.str()}; // Convert to string
            for (size_t i{0U}; i < hexString.size(); i++)
            {
                const std::string tmp{std::string(1U, hexString[i])};
                ntaArray.push_back(tmp);
            }
        }
        canInfo.setData(this->mEcuInformation.getTargetAddress(), ntaArray);
        transportInfo->setData(protocolType, canInfo, false, 0U);
        const std::string applicationName{"remotediag"};
        (void)OnboardclientAdapter::getInstance()->connect(transportInfo, applicationName, ConnectInfo);
        if (ConnectInfo->getResponse() == OBCEnum::OBCErrCode::OBC_OK)
        {
            this->mConnectId = ConnectInfo->getConnectId();
            this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_CONNECT;
            OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
            this->sendUds();
        }
        else
        {
            LOG_E("Can't connect to canid = %02X", this->mEcuInformation.getTargetAddress());
            // TMCDCMTF-35635
            // OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
            this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DONE;
            (void)RemoteDirectCommand::getInstance()->mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_FINISH_TRANSMISSION)->sendToTarget();
        }
    }

    void RemoteDirectCommand::DirectCommandTransmission::disconnect(void)
    {
        LOG_I("DirectCommandTransmission::disconnect()");
        (void)OnboardclientAdapter::getInstance()->disconnectECU(this->mConnectId);
        // TMCDCMTF-35635
        // Only release OBC resouce after function complete
        // OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
        this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DONE;
    }

    void RemoteDirectCommand::DirectCommandTransmission::sendUds(void)
    {
        LOG_I("DirectCommandTransmission::sendUds()");
        mSentUds = this->mUdsReqList.front();
        this->mUdsReqList.pop_front();
        const uint8_t res{OnboardclientAdapter::getInstance()->sendUdsData(this->mConnectId, mSentUds.second)};
        if (res != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK))
        {
            // TODO Handle exeptional case
            RemoteDirectCommand::getInstance()->createUnresponsiveResData(this);
            if (!mUdsReqList.empty())
            {
                (void)RemoteDirectCommand::getInstance()->mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_SEND_UDS)->sendToTarget();
            }
            else
            {
                setState(DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DISCONNECT);
                disconnect();
                RemoteDirectCommand::getInstance()->finishCurrentTransmission();
            }
        }
        else
        {
            if (this->mEcuInformation.getDiagPhase() != CommonDefine::DiagPhase::DP_PHASE_5)
            {
                LOG_D("Diag Phase is not DP_PHASE_5, set state to DIRECTCOMMAND_TRANS_SEND_UDS");
                this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_UDS;
            }
            else
            {
                if ((this->mState == DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_CONNECT) &&
                    (mSentUds.second->size() >= 2U) &&
                    (mSentUds.second->data() != nullptr) &&
                    (mSentUds.second->data()[0] == 0x10U) &&
                    (mSentUds.second->data()[1] == 0x40U))
                {
                    LOG_I("Set state to SEND_REMOTE_SESSION");
                    this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_REMOTE_SESSION;
                }
                else if (this->mState == DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_REMOTE_SESSION)
                {
                    LOG_I("Set state to SEND_UDS");
                    this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_UDS;
                }
                else if ((this->mState == DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_UDS) &&
                         (mSentUds.second->size() >= 2U) &&
                         (mSentUds.second->data() != nullptr) &&
                         (mSentUds.second->data()[0] == 0x10U) &&
                         (mSentUds.second->data()[1] == 0x01U))
                {
                    LOG_I("Set state to SEND_DEFAULT_SESSION");
                    this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_DEFAULT_SESSION;
                }
                else
                {
                    LOG_I("Set state to SEND_UDS with multi request");
                    this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_UDS;
                }
            }
            RemoteDirectCommand::getInstance()->startTimer(DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_TRANSMISSION_TIMEOUT);
            LOG_I("Start timeout timer for connectId %lu isDefault %d", this->mConnectId, bDefaultSession);
        }
    }

    std::pair<uint32_t, android::sp<Buffer>> RemoteDirectCommand::DirectCommandTransmission::getUdsReq() const noexcept
    {
        return this->mSentUds;
    }
    void RemoteDirectCommand::DirectCommandTransmission::insertBeforeEnd(const std::pair<uint32_t, android::sp<Buffer>> uds)
    {
        if (mUdsReqList.empty())
        {
            mUdsReqList.push_back(uds);
        }
        else
        {
            std::deque<std::pair<uint32_t, android::sp<Buffer>>>::const_iterator itr{mUdsReqList.cend()};
            --itr;
            (void)mUdsReqList.insert(itr, uds);
        }
    };
    bool RemoteDirectCommand::DirectCommandTransmission::getCurrUdsRequest()
    {
        bool isNotDefault{false};
        if (mUdsReqList.size() > 1U) // to get UDS request, and ignore the last request which is for default session
        {
            isNotDefault = true;
            mSentUds = this->mUdsReqList.front();
            this->mUdsReqList.pop_front();
        }
        return isNotDefault;
    }

    void RemoteDirectCommand::onOccurrentRobDetected(const android::sp<OccurrentRobNotification> notification)
    {
        LOG_D("robDetected, notificationId = %llu, cocoId = %llu, time = %lld, latitude = 0x%08X, longitude = 0x%08X, targetAddress = 0x%02x, priority = %d", notification->getId(), notification->collectionConditionId(), notification->triggerTime(), notification->triggerLocation()->getLatitude(), notification->triggerLocation()->getLongtitude(), notification->getTargetCollectionDataRobSsr().ecu_address_information().target_address(), notification->priority());
        (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_OCCURRENCE_REQUEST, notification)->sendToTarget();
    }

    uint8_t RemoteDirectCommand::getAppStatus() const noexcept
    {
        const Mutex::Autolock mLock{Mutex::Autolock(mMutexAppState)};
        return this->isDirectAppRunning;
    }

    void RemoteDirectCommand::changeAppStatus(const uint8_t appStatus) noexcept
    {
        const Mutex::Autolock mLock{Mutex::Autolock(mMutexAppState)};
        this->isDirectAppRunning = appStatus;
    }

    bool RemoteDirectCommand::isAbortingUnderRepair() const noexcept
    {
        const Mutex::Autolock mLock{Mutex::Autolock(mMutexAppState)};
        return this->bAbortUnderRepair;
    }

    void RemoteDirectCommand::setAbortingUnderRepair(const bool isAbort) noexcept
    {
        const Mutex::Autolock mLock{Mutex::Autolock(mMutexAppState)};
        this->bAbortUnderRepair = isAbort;
    }

    void RemoteDirectCommand::testingMaxFileSize(const uint16_t fileSize) noexcept
    {
        DIRECTCOMMAND_UPLOAD_DATA_SIZE_MAX = static_cast<uint32_t>(fileSize);
        DIRECTCOMMAND_LAST_UPLOAD_DATA_SIZE_MAX = DIRECTCOMMAND_UPLOAD_DATA_SIZE_MAX / 2U;
        DIRECTCOMMAND_TOTAL_UPLOAD_DATA_SIZE_MAX = DIRECTCOMMAND_UPLOAD_DATA_SIZE_MAX * 2U + DIRECTCOMMAND_LAST_UPLOAD_DATA_SIZE_MAX;
    }
    void RemoteDirectCommand::handleUdsResponse(const android::sp<OBCResponseEventInfo> responseEventInfo)
    {
        if (this->getAppStatus() == RemoteDirectCommand::DIRECTCOMMAND_RUNNING)
        {
            const uint16_t connectId{responseEventInfo->resInfo()->connectId()};
            LOG_D("[onReceiveUDS] connectId: %u", connectId);
            if (mCurrDirectCommandTrans->connectId() == connectId)
            {
                bUploadDirectCommandData = true;
                vccomif::rdg::v1::interfaces::DiagnosticsMessage diagMessage{};
                if (mCurrDirectCommandTrans->ecuInformation().getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5)
                {

                    if (mCurrDirectCommandTrans->getState() == DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_REMOTE_SESSION)
                    {
                        LOG_I("Receive response for remote session");
                        const android::sp<::Buffer> resData{responseEventInfo->resInfo()->udsData()};
                        if ((responseEventInfo->errCode() != OBCEnum::OBCErrCode::OBC_OK) ||
                            (resData->size() < 2U) ||
                            ((resData->data() != nullptr) &&
                             ((resData->data()[0] != 0x50U) ||
                              (resData->data()[1] != 0x40U))))
                        {
                            LOG_E("Remote session set failed, create error response data");
                            // for handling mismatch SID of UDS response
                            if ((responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_OK))
                            {
                                responseEventInfo->errCode() = OBCEnum::OBCErrCode::OBC_TIMEOUT;
                            }
                            while (mCurrDirectCommandTrans->getCurrUdsRequest())
                            {
                                this->createDiagnosticsData(diagMessage, mCurrDirectCommandTrans, responseEventInfo);
                                addResponseToUpload(mTotalSize, diagMessage);
                            }
                            mCurrDirectCommandTrans->cleanRequestQueue();
                        }
                    }
                    else if (mCurrDirectCommandTrans->getState() == DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_UDS)
                    {
                        LOG_I("Receive response for UDS");
                        this->createDiagnosticsData(diagMessage, mCurrDirectCommandTrans, responseEventInfo);
                        addResponseToUpload(mTotalSize, diagMessage);
                    }
                    else
                    {
                        LOG_I("Receive response for UDS default session");
                    }
                }
                else
                {
                    this->createDiagnosticsData(diagMessage, mCurrDirectCommandTrans, responseEventInfo);
                    addResponseToUpload(mTotalSize, diagMessage);
                }
                LOG_V("total size: %u", mTotalSize);

                this->mTimeOutTrans->stop();
                if (mTotalSize > DIRECTCOMMAND_TOTAL_UPLOAD_DATA_SIZE_MAX)
                {
                    mCurrDirectCommandTrans->setState(DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DISCONNECT);
                    mCurrDirectCommandTrans->disconnect();
                    LOG_I("Abort directcommand due to total size exceeded");
                    this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_STOP);
                    this->finishDirectCommand();
                    this->notifyTriggerDone(this->mTriggerId, true, true, true);
                }
                else
                {
                    if (isAbortingUnderRepair())
                    {
                        abortDirectCommand(vccomif::rdg::v1::interfaces::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR);
                    }
                    else if (mSuspended)
                    {
                        suspendDirectCommand();
                    }
                    else
                    {
                        if (mCurrDirectCommandTrans->getUdsReqList().size() != 0U)
                        {
                            mCurrDirectCommandTrans->sendUds();
                        }
                        else
                        {
                            mCurrDirectCommandTrans->setState(DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DISCONNECT);
                            mCurrDirectCommandTrans->disconnect();
                            this->finishCurrentTransmission();
                        }
                    }
                }
            }
            else
            {
                LOG_I("Receive expired response");
            }
        }
    }

    void RemoteDirectCommand::handleStopRDG()
    {
        if (this->getAppStatus() == RemoteDirectCommand::DIRECTCOMMAND_RUNNING)
        {
            LOG_I("Stop Directcommand");
            this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_STOP);
            if ((mCurrDirectCommandTrans != nullptr) && (mCurrDirectCommandTrans->getState() != DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DONE))
            {
                this->mTimeOutTrans->stop();
                mCurrDirectCommandTrans->disconnect();
            }
            if (!mDirectCommandTransQueue.empty())
            {
                mDirectCommandTransQueue.clear();
            }
            if (mDirectCommandResponse_order.size() != 0U)
            {
                mDirectCommandResponse_order.clear();
            }
            mSaveReq.clear();
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
        }
        else
        {
            LOG_I("DirectCommand is not running");
        }
    }
    void RemoteDirectCommand::createUnresponsiveResData(const android::sp<DirectCommandTransmission> directCommandReq)
    {
        const android::sp<OBCResponseEventInfo> responseEventInfo{new OBCResponseEventInfo()};
        responseEventInfo->errCode() = OBCEnum::OBCErrCode::OBC_ERR_NOT_CONNECTED;
        vccomif::rdg::v1::interfaces::DiagnosticsMessage diagMessage{};
        createDiagnosticsData(diagMessage, directCommandReq, responseEventInfo);
        mDirectCommandResponse_order[directCommandReq->getRequestIdx()] = std::pair<bool, vccomif::rdg::v1::interfaces::DiagnosticsMessage>(true, diagMessage);
    }
    void RemoteDirectCommand::addResponseToUpload(uint32_t &currentUploadSize, const vccomif::rdg::v1::interfaces::DiagnosticsMessage directCommandUploadData)
    {
        if (currentUploadSize <= (UINT32_MAX - directCommandUploadData.ByteSizeLong()))
        {
            currentUploadSize += directCommandUploadData.ByteSizeLong();
        }
        else
        {
            LOG_E("Data size exceeded max");
        }
        if (currentUploadSize <= DIRECTCOMMAND_TOTAL_UPLOAD_DATA_SIZE_MAX)
        {
            mDirectCommandResponse_order[mCurrDirectCommandTrans->getRequestIdx()] = std::pair<bool, vccomif::rdg::v1::interfaces::DiagnosticsMessage>(true, directCommandUploadData);
        }
    }
}
