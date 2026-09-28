#include <google/protobuf/util/json_util.h>
#include "RemoteDirectCommand.h"
#include "../../sid_filter/SIDFilter.h"

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
        bAbortDirectCommand = false;
        bsrvc_ac_flag = false;
        mSuspended = false;
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
        DirectCommandList l_DirectCommand{};
        bool srvc_ac_disregard_flag{false};
        this->getDirectCommandRequest(diagTriggerType, collectionId, l_DirectCommand, srvc_ac_disregard_flag);
        if ((DiagManagerAdapter::getInstance()->getAllUploadConsent() == false) && (srvc_ac_disregard_flag == false))
        {
            res = false;
            LOG_I("Reject DirectCommand function due to all upload consent status and srvc_ac_flag are false");
        }
        else
        {
            res = true;
            if (mApp.getUnderRepair() == 1U)
            {
                res = false;
                LOG_I("Reject DirectCommand function due to under repair");
            }
            else if (DiagManagerAdapter::getInstance()->getRDGFlag() == 0U)
            {
                res = false;
                LOG_I("Reject DirectCommand function due to RDG flag is OFF");
                vccomif::rdg::v1::interfaces::UploadErrorDataRequest mUploadErrorData{};
                mUploadErrorData.set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_UNPROVIDED_VEHICLE);
                this->makeErrorUploadData(mUploadErrorData, diagTriggerType, collectionId, srvc_ac_disregard_flag);
            }
            else
            {
                res = true;
            }
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
        const android::sp<OBCCanInfo> canInfo{responseEventInfo->resInfo()->canInfo()};
        const android::sp<::Buffer> udsData{responseEventInfo->resInfo()->udsData()};
        // const uint8_t protocolType{responseEventInfo->resInfo->protocolType};
        // const uint8_t bErrCode{responseEventInfo->errCode};
        const uint32_t sizeOfResponse{responseEventInfo->resInfo()->udsData()->size()};
        if (this->getAppStatus() == RemoteDirectCommand::DIRECTCOMMAND_RUNNING)
        {
            const uint16_t connectId{responseEventInfo->resInfo()->connectId()};
            LOG_D("[onReceiveUDS] connectId: %u", connectId);
            if (mCurrDirectCommandTrans->connectId() == connectId)
            {
                for (uint32_t i{0U}; i < sizeOfResponse; i++)
                {
                    if(responseEventInfo->resInfo()->udsData()->data() != nullptr){
                        LOG_V("UDS data %02X", responseEventInfo->resInfo()->udsData()->data()[i]);
                    } else {
                        LOG_V("responseEventInfo->resInfo()->udsData() is null");
                    }
                }
                bUploadDirectCommandData = true;
                vccomif::rdg::v1::interfaces::DiagnosticsMessage diagMessage{};
                if (!mCurrDirectCommandTrans->getDefaultSession())
                {
                    this->createDiagnosticsData(diagMessage, mCurrDirectCommandTrans, responseEventInfo);
                    if (mTotalSize <= (UINT32_MAX - diagMessage.ByteSizeLong()))
                    {
                        mTotalSize += diagMessage.ByteSizeLong();
                    }
                    else
                    {
                        LOG_E("file size overflow");
                    }
                    LOG_V("total size: %lu", mTotalSize);
                }

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
                    if (!mCurrDirectCommandTrans->getDefaultSession())
                    {
                        mDirectCommandResponse.push_back(diagMessage);
                    }
                    if (mSuspended)
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
        else
        {
            LOG_D("DirectCommand App is not running");
        }
    }

    void RemoteDirectCommand::onChangedRemoteInfo(const int32_t what, const int32_t info) noexcept
    {
        LOG_I("onChangedRemoteInfo");
        (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_RECEIVE_UNDER_REPAIR_FLAG_CHANGE, what, info)->sendToTarget();
    }

    void RemoteDirectCommand::onCenterCommandForward(const android::sp<CenterReqData> &pCenterReqData) noexcept
    {
        LOG_I("RemoteDirectCommand receive Center request");
        /*TODO: Process center data*/
        /*Check reject due to under repair*/
        const IG_STATUS igStatus{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
        const uint64_t colID{pCenterReqData->getCenterReq_CollectionID()};
        if ((((pCenterReqData->getScheduleType() == Rdg_Sched_Type::SchedType::ST_IMMEDIATE_IG_ON_ONE_SHOT) ||
              (pCenterReqData->getScheduleType() == Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE) ||
              (pCenterReqData->getScheduleType() == Rdg_Sched_Type::SchedType::ST_PERIOD_TRIGGER_ROUTINE) ||
              (pCenterReqData->getScheduleType() == Rdg_Sched_Type::SchedType::ST_IMMEDIATE_ANY_POWER_STATUS_ONE_SHOT)) &&
             (igStatus == IG_STATUS::IG_STATUS_OFF)) ||
            (((pCenterReqData->getScheduleType() == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT) ||
              (pCenterReqData->getScheduleType() == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_ROUTINE)) &&
             (igStatus == IG_STATUS::IG_STATUS_ON)))
        {
            LOG_I("Reject DirectCommand function due to schedule type mismatch %d", pCenterReqData->getScheduleType());
            // TimeManager &mTimeManagerService{TimeManager::getInstance()};
            const int64_t current_time{ParamsDef::getCurrentAcquisiteTime()};
            LOG_I("Collection ID: %llu - Last complete time: %lld", mColId, current_time);
            mApp.notifyLastOpComplTime(colID, current_time);
        }
        else
        {
            uint32_t prio_data{pCenterReqData->getCenterReq_prio()};
            uint8_t colId_ptr[sizeof(colID)];
            (void)memcpy(&colId_ptr[0], &colID, sizeof(colId_ptr));
            const android::sp<::Buffer> colId_sp{new ::Buffer()};
            colId_sp->setTo(&colId_ptr[0], sizeof(colId_ptr));
            int32_t what{0};
            if (pCenterReqData->getTrigger_type() == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
            {
                what = MainHandler::CMD_DIRECTCOMMAND_CENTER_REQUEST;
            }
            else if (pCenterReqData->getTrigger_type() == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
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
        (void)igStatus;
    }

    uint8_t RemoteDirectCommand::getAppId(void) const noexcept
    {
        return RDG_APPID::DIRECT_COMMAND;
    }

    void RemoteDirectCommand::startUp(void)
    {
        // Get Time data
        mTotalSize = 0U;
        this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_RUNNING);
        // acquisition time
        // TimeManager &mTimeManagerService{TimeManager::getInstance()};
        int64_t current_time{0};
        current_time = ParamsDef::getCurrentAcquisiteTime();
        mDirectCommandAcquisitionTime = current_time;
        // Get Location data
        mLocationData = LocationManagerAdapter::getInstance()->getLocationData();
        LOG_I("Get location data success, latitude: 0x%08X, longtitue 0x%08X", mLocationData->getLatitude(), mLocationData->getLongtitude());
        // Get ODO data
        (void)VehicleManagerAdapter::getInstance()->getOdoInformation(odo_value, odo_unit); // odo value
        this->makeHeaderForUploading(mDirectCommandUploadData);

        const uint32_t uploadDataSize{mDirectCommandUploadData->ByteSizeLong()};
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
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            LOG_I("mDirectCommandTransQueue size = %lu", mDirectCommandTransQueue.size());
            mCurrDirectCommandTrans = mDirectCommandTransQueue.front();
            mCurrDirectCommandTrans->connect();
        }
    }

    void RemoteDirectCommand::getDirectCommandRequest(const DiagTrigger::DiagTriggerType triggerType, const uint64_t collectionId, google::protobuf::RepeatedPtrField<vccomif::rdg::v1::interfaces::DirectCommand> &commandList, bool &srvc_ac_flag)
    {
        std::string str_directCommand_req{""};
        if (triggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
        {
            const CenterRequestDirectCommandList &mCenterRequestDirectCommandList{CollectionCondition::getInstance().getCenterRequestDirectCommand()};
            for (CenterRequestDirectCommandIter it{mCenterRequestDirectCommandList.begin()}; it != mCenterRequestDirectCommandList.end(); it++)
            {
                if ((*it).collection_condition_id() == collectionId)
                {
                    LOG_I("Get Center request direct command, collectionId = %llu", collectionId);
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
        else if (triggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
        {
            srvc_ac_flag = false;
            commandList.CopyFrom(*(mRobNotificationReq->directCommands()));
        }
        else
        {
            // DO nothing
        }
    }

    bool RemoteDirectCommand::validateDirectCommand(const vccomif::rdg::v1::interfaces::DirectCommand bDirectCommandReq, CommonDefine::EcuInformation &ecuInformation)
    {
        // communication_protocol()
        bool res{false};
        const CenterCommunicationProtocol bProtocol{bDirectCommandReq.ecu_address_information().communication_protocol()};
        const uint32_t centerTargetAdd{bDirectCommandReq.ecu_address_information().target_address()};
        const CenterCommunicationType reqCmdType{bDirectCommandReq.ecu_address_information().communication_type()};
        if ((centerTargetAdd != 0U) &&
            (((bProtocol == CenterCommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD) &&
              (reqCmdType != CenterCommunicationType::EcuAddressInformation_CommunicationType_CT_UNKNOWN)) ||
             ((bProtocol == CenterCommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN) &&
              ((reqCmdType == CenterCommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
               (reqCmdType == CenterCommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS)))))
        {
            uint32_t targetAddCompare{centerTargetAdd};
            if (reqCmdType == CenterCommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS)
            {
                targetAddCompare = ((targetAddCompare << 8U) | 0xFFU);
            }
            std::list<CommonDefine::EcuInformation>::iterator it{l_EcuInformation.begin()};
            LOG_I("Ecu list size = %lu", l_EcuInformation.size());
            while (it != l_EcuInformation.end())
            {
                if (it->getTargetAddress() == targetAddCompare)
                {
                    const bool ecuFlag{it->getecuActiveFlag()};
                    const CommonDefine::DiagPhase ecuDiagPhase{it->getDiagPhase()};
                    if ((ecuFlag == true) &&
                        (ecuDiagPhase > CommonDefine::DiagPhase::DP_UNKNOW) &&
                        (ecuDiagPhase <= CommonDefine::DiagPhase::DP_PHASE_6))
                    {
                        res = true;
                        ecuInformation.setTargetAddress(it->getTargetAddress());
                        ecuInformation.setCanId(it->getCanId());
                        ecuInformation.setecuActiveFlag(ecuFlag);
                        ecuInformation.setCommProtocol(it->getCommProtocol());
                        ecuInformation.setCommType(it->getCommType());
                        ecuInformation.setDiagPhase(ecuDiagPhase);
                        break;
                    }
                    (void)ecuDiagPhase;
                }
                it++;
            }
            (void)targetAddCompare;
        }
        else
        {
            res = false;
        }

        (void)bProtocol;

        if (res == true)
        {
            LOG_V("Valid targetAddr %02X, commProtocol %02X, commType %02X", centerTargetAdd, ecuInformation.getCommProtocol(), ecuInformation.getCommType());
        }
        else
        {
            // Make error upload data
            LOG_E("Invalid ECU specificed error targetAddr %02X", centerTargetAdd);
            bUploadErrorData = true;
            vccomif::rdg::v1::interfaces::DiagnosticsMessage *const errDiagMessage{mDirectCommandUploadErrorData->add_diag_messages()};
            errDiagMessage->set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_ECU_SPECIFIED_ERROR); // SC_ECU_SPECIFIED_ERROR
            vccomif::rdg::v1::interfaces::EcuAddressInformation *const errorEcuAddr{errDiagMessage->mutable_ecu_address_information()};
            vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{bDirectCommandReq.ecu_address_information().communication_protocol()};
            if ((protocolType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN) ||
                (protocolType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_IP))
            {
                protocolType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN;
            }
            errorEcuAddr->set_communication_protocol(protocolType);
            vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{bDirectCommandReq.ecu_address_information().communication_type()};
            if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
            {
                commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
            }
            errorEcuAddr->set_communication_type(commType);
            errorEcuAddr->set_target_address(centerTargetAdd);
        }
        (void)reqCmdType;
        return res;
    }

    void RemoteDirectCommand::triggerDirectCommand(const DiagTrigger::DiagTriggerType type, const uint32_t prio, const uint64_t colId, const int64_t time)
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
        LOG_I("Check DirectCommand: trigger time: %lld sec, Collection ID: %llu", time, colId);
        /* Obtain message:CMD_DIRECTCOMMAND_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
        (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
        /* Save request to local*/
        const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret{mSaveReq.emplace(nextTriggerId, pTrigger)};
        LOG_I("Check mSaveReq size: %lu", mSaveReq.size());
        if (!ret.second)
        {
            ret.first->second = pTrigger;
        }
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
            const DiagTrigger::DiagTriggerType pTriggerType{it->second->getType()};
            const uint64_t colID{it->second->getCollectionID()};
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
                const uint32_t prio_data{it->second->getPriority()};
                LOG_I("TRIGGER_PROCESSING");
                /* DirectCommand can run */
                mTriggerId = pTriggerId;
                mTriggerType = pTriggerType;
                mColId = colID;
                mPriority = prio_data;
                if (this->checkPrecondition(pTriggerType, mColId))
                {
                    (void)mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_SEND_REQUEST)->sendToTarget();
                }
                else
                {
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
                    vccomif::rdg::v1::interfaces::UploadErrorDataRequest mUploadErrorData{};
                    mUploadErrorData.set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED);
                    DirectCommandList l_DirectCommand{};
                    bool srvc_ac_disregard_flag{false};
                    this->getDirectCommandRequest(pTriggerType, colID, l_DirectCommand, srvc_ac_disregard_flag);
                    this->makeErrorUploadData(mUploadErrorData, pTriggerType, colID, srvc_ac_disregard_flag);
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
        RemoteEcuInformation::getInstance()->getEcuInformationList(l_EcuInformation);
        if (l_EcuInformation.size() > 0U)
        {
            for (CollectionCondition::DirectCommandIter it{l_DirectcommandRequest.begin()}; it != l_DirectcommandRequest.end(); it++)
            {
                uint32_t cmdIdx{0U};
                CommonDefine::EcuInformation ecuInformation{};
                if (validateDirectCommand(*it, ecuInformation))
                {
                    const sp<::Buffer> udsReq{new ::Buffer()};
                    const std::string commandMess{(*it).command_messages()};
                    if (commandMess.size() <= static_cast<uint32_t>(INT32_MAX))
                    {
                        udsReq->setTo(commandMess.c_str(), static_cast<int32_t>(commandMess.size()));
                    }
                    // for (uint32_t i{0U}; i < udsReq->size(); i++)
                    // {
                    //     LOG_V("[sendDirectCommand] uds data  = 0x%02X", udsReq->data()[i]);
                    // }
                    const uint16_t maxUdsReqSize{(this->mTriggerType != DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER) ? 4095U /*4kB*/ : 16U};
                    if ((udsReq->size() > static_cast<uint32_t>(maxUdsReqSize) /*test: 10 */) || (udsReq->size() == 0U))
                    {
                        LOG_E("[sendDirectCommand] udsReq->size exceeded ");
                        bUploadErrorData = true;
                        vErrorIdx.push_back(cmdIdx);
                        mDirectCommandUploadErrorData->set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_EXCEEDED_DATA_SIZE);
                        vccomif::rdg::v1::interfaces::DiagnosticsMessage *const error_diagMessage{mDirectCommandUploadErrorData->add_diag_messages()};
                        error_diagMessage->set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_COMMAND_ERROR);
                        vccomif::rdg::v1::interfaces::EcuAddressInformation *const ecuAddressInfo{error_diagMessage->mutable_ecu_address_information()};
                        ecuAddressInfo->set_communication_protocol(ecuInformation.getCommProtocol());
                        ecuAddressInfo->set_communication_type(ecuInformation.getCommType());
                        ecuAddressInfo->set_target_address(it->ecu_address_information().target_address());
                        if ((udsReq->size() != 0U))
                        {
                            if(udsReq->data() != nullptr) {
                                error_diagMessage->set_user_data(&udsReq->data()[0], 1U); // Store 1Byte (SID value)
                            } else {
                                LOG_E("udsReq->data() is null");
                            }
                        }
                        // TODO RDG30-R-0697 Remove DirectCommand in the data collection condition
                    }
                    else
                    {
                        if (mDirectCommandTransQueue.empty())
                        {
                            const android::sp<DirectCommandTransmission> mDirectCommandTransmission{new DirectCommandTransmission(*this, ecuInformation, udsReq)};
                            mDirectCommandTransmission->pushUdsReqList(udsReq);
                            mDirectCommandTransQueue.push_back(mDirectCommandTransmission);
                            LOG_I("Target ID: %02X, transmission list size: %lu", ecuInformation.getTargetAddress(), mDirectCommandTransQueue.size());
                        }
                        else
                        {
                            const android::sp<DirectCommandTransmission> prevCommand{mDirectCommandTransQueue.back()};
                            if(prevCommand->getUdsReqList().front()->data() != nullptr){
                            LOG_I("Prev Target ID: %02X, Curr Target %02X, Prev SID %02X, Curr SID %02X", prevCommand->ecuInformation().getTargetAddress(), ecuInformation.getTargetAddress(), prevCommand->getUdsReqList().front()->data()[0], udsReq->data()[0]);
                            if ((prevCommand->ecuInformation().getTargetAddress() == ecuInformation.getTargetAddress()) && ((prevCommand->getUdsReqList().front()->data()[0] == 0x10U) && (prevCommand->getUdsReqList().front()->data()[1] != 0x01U)))
                            {
                                if(prevCommand->getUdsReqList().back()->data() != nullptr) {
                                if ((prevCommand->getUdsReqList().back()->data()[0] == 0x10U) && (prevCommand->getUdsReqList().back()->data()[1] == 0x01U))
                                {
                                    const android::sp<DirectCommandTransmission> mDirectCommandTransmission{new DirectCommandTransmission(*this, ecuInformation, udsReq)};
                                    mDirectCommandTransmission->pushUdsReqList(udsReq);
                                    mDirectCommandTransQueue.push_back(mDirectCommandTransmission);
                                    LOG_I("Target ID: %02X, transmission list size: %lu", ecuInformation.getTargetAddress(), mDirectCommandTransQueue.size());
                                }
                                else
                                {
                                    prevCommand->pushUdsReqList(udsReq);
                                    LOG_I("Target ID: %02X, udsReqList size: %lu", ecuInformation.getTargetAddress(), prevCommand->getUdsReqList().size());
                                }
                                } else {
                                    LOG_E("prevCommand->getUdsReqList().back()->data() is null");
                                }
                            }
                            else
                            {
                                const android::sp<DirectCommandTransmission> mDirectCommandTransmission{new DirectCommandTransmission(*this, ecuInformation, udsReq)};
                                mDirectCommandTransmission->pushUdsReqList(udsReq);
                                mDirectCommandTransQueue.push_back(mDirectCommandTransmission);
                                LOG_I("Target ID: %02X, transmission list size: %lu", ecuInformation.getTargetAddress(), mDirectCommandTransQueue.size());
                            }
                            } else {
                                LOG_E("prevCommand->getUdsReqList().front()->data() is null");
                            }
                        }
                    }
                }
                else
                {
                    vErrorIdx.push_back(cmdIdx);
                    bUploadErrorData = true;
                    mDirectCommandUploadErrorData->set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_UNEXPECTED_COMMAND);
                }
                // Should be at the end
                (void)cmdIdx++;
            }
        }
        else
        {
            // Self-Diag
            LOG_E("ECU list is none");
            DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
        }
        if (mDirectCommandTransQueue.size() > 0U)
        {
            this->startUp();
        }
        else
        {
            LOG_I("l_DirectCommandTrans is empty");
            this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_STOP);
            this->finishDirectCommand();
            this->notifyTriggerDone(this->mTriggerId, true, false, true);
        }
        if ((this->mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER) && (vErrorIdx.size() > 0U))
        {
            CollectionCondition::getInstance().removeErrorDirectCommand(this->mColId, vErrorIdx);
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
        if (mDirectCommandResponse.size() != 0U)
        {
            makeUploadData();
        }
        if (bUploadErrorData == true)
        {
            this->makeErrorUploadData(*(this->mDirectCommandUploadErrorData), this->mTriggerType, this->mColId, this->bsrvc_ac_flag);
            bUploadErrorData = false;
        }
        mDirectCommandTransQueue.clear();

        if (!mDirectCommandResponse.empty())
        {
            mDirectCommandResponse.clear();
        }
        LOG_I("Finish DirectCommand Acquisition triggerID: %d", mTriggerId);
        bAbortDirectCommand = false;
    }

    void RemoteDirectCommand::notifyTriggerDone(const int32_t pTriggerId, const bool isNotify, const bool isCompletedRob, const bool isRemoveTrigger)
    {
        const uint32_t u_pTriggerId{(pTriggerId >= 0) ? static_cast<uint32_t>(pTriggerId) : 0U};
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mSaveReq.find(u_pTriggerId)};
        if (it != mSaveReq.end())
        {
            /* Get current time */
            if (isNotify)
            {
                // TimeManager &mTimeManagerService{TimeManager::getInstance()};
                int64_t current_time{0};
                current_time = ParamsDef::getCurrentAcquisiteTime();
                LOG_I("Collection ID: %llu - Last complete time: %lld", mColId, current_time);
                PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, it->second->getType());
                mApp.notifyLastOpComplTime(mColId, current_time);
            }
            if (it->second->getType() == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
            {
                RoBOccurrence::getInstance()->onDirectCommandCompleted(isCompletedRob);
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
        LOG_I("Make DirectCommand upload data");
        mDirectCommandUploadData->Clear();
        this->makeHeaderForUploading(mDirectCommandUploadData, true);
        // split file
        LOG_I("Size of response: %d", mDirectCommandResponse.size());
        std::vector<vccomif::rdg::v1::interfaces::DiagnosticsMessage>::iterator it_ResData{mDirectCommandResponse.begin()};
        for (uint32_t i{0U}; i < MAX_SPLIT_FILE; i++)
        {
            LOG_V("Current file: %lu", i);
            UploadDirectCommandDataRequest pUploadDirectCommandDataRequest{};
            pUploadDirectCommandDataRequest.CopyFrom(*mDirectCommandUploadData);
            uint32_t sizeOfFile{0U};
            sizeOfFile = pUploadDirectCommandDataRequest.ByteSizeLong();
            while (it_ResData != mDirectCommandResponse.end())
            {
                // -> sizeOfFile + getSize() > 4194304 --> ko push
                const uint32_t userDataFile{it_ResData->ByteSizeLong()};
                LOG_V("Size of user data: %lu, current sizeOfFile %lu", userDataFile, sizeOfFile);
                if (sizeOfFile <= UINT32_MAX - userDataFile)
                {
                    sizeOfFile += userDataFile;
                }
                else
                {
                    LOG_E("file size overflow");
                }
                if ((sizeOfFile <= DIRECTCOMMAND_UPLOAD_DATA_SIZE_MAX) /* test: 55 + X */ && (i < MAX_SPLIT_FILE - 1U)) // 4M - 1st and 2nd file //
                {
                    vccomif::rdg::v1::interfaces::DiagnosticsMessage *const diagMessage{pUploadDirectCommandDataRequest.add_direct_command_messages()};
                    diagMessage->CopyFrom(*it_ResData);
                    it_ResData++;
                }
                else if ((sizeOfFile <= DIRECTCOMMAND_LAST_UPLOAD_DATA_SIZE_MAX) /* test: 55 + X */ && (i == MAX_SPLIT_FILE - 1U)) // 2M - last file
                {
                    vccomif::rdg::v1::interfaces::DiagnosticsMessage *const diagMessage{pUploadDirectCommandDataRequest.add_direct_command_messages()};
                    diagMessage->CopyFrom(*it_ResData);
                    it_ResData++;
                }
                else
                {
                    break;
                }
                (void)DIRECTCOMMAND_LAST_UPLOAD_DATA_SIZE_MAX;
            }
            (void)sizeOfFile;
            l_UploadDirectCommandDataRequest.push_back(pUploadDirectCommandDataRequest);
            if (it_ResData == mDirectCommandResponse.end())
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
            std::string file_dir{std::to_string(uploadId)};
            (void)file_dir.append("_UploadDirectCommandDataRequest.dat");
            const error_t bStored{DataModel<UploadDirectCommandDataRequest>::save(file_dir, *it)};
            if (bStored == E_OK)
            {
                const uint8_t operation{getOperation()};
                // Self-Diag
                DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
            }
            const android::sp<UploadTask> task{new UploadTask(uploadId)};
            task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG070);
            task->setUploadPatch(file_dir);
            task->setUploadId(static_cast<uint64_t>(uploadId));
            /*Set priority*/
            task->setUploadPrio(mPriority);
            task->setSrvcAcFlag(this->bsrvc_ac_flag);
            const uint64_t fileSize{static_cast<uint64_t>(it->ByteSizeLong())};
            task->setFileSize(fileSize);
            UploadManager::getInstance()->requestUploadTask(task);
            // test_saveUploadData = task;
        }
        mDirectCommandUploadData->Clear();
        l_UploadDirectCommandDataRequest.clear();
    }

    void RemoteDirectCommand::createDiagnosticsData(vccomif::rdg::v1::interfaces::DiagnosticsMessage &diagMessage, const android::sp<DirectCommandTransmission> directCommandReq, const android::sp<OBCResponseEventInfo> responseEventInfo) const
    {
        vccomif::rdg::v1::interfaces::EcuAddressInformation *const ecuAddressInfo{diagMessage.mutable_ecu_address_information()};
        ecuAddressInfo->set_communication_protocol(directCommandReq->ecuInformation().getCommProtocol());
        const CenterCommunicationType diagResType{directCommandReq->ecuInformation().getCommType()};
        ecuAddressInfo->set_communication_type(diagResType);
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
        if (responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_OK)
        {
            (void)centerTxAdd;
            diagMessage.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL);
            ecuAddressInfo->set_target_address(centerRxAdd);
            if (responseEventInfo->resInfo()->udsData()->data() != nullptr)
            {
                diagMessage.set_user_data(responseEventInfo->resInfo()->udsData()->data(), responseEventInfo->resInfo()->udsData()->size());
            }
            else
            {
                LOG_E("udsData()->data() is nullptr");
            }
        }
        else if (responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_NEGATIVE)
        {
            (void)centerTxAdd;
            diagMessage.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL_WITH_NEGATIVE);
            ecuAddressInfo->set_target_address(centerRxAdd);
            if (responseEventInfo->resInfo()->udsData()->data() != nullptr)
            {
                diagMessage.set_user_data(responseEventInfo->resInfo()->udsData()->data(), responseEventInfo->resInfo()->udsData()->size());
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
            const android::sp<::Buffer> udsRequest{directCommandReq->getUdsReq()};
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
                abortDirectCommand(vccomif::rdg::v1::interfaces::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR);
            }
            break;
        }
        default:
        {
            break;
        }
        }
    }

    void RemoteDirectCommand::makeErrorUploadData(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colId, const bool srvc_ac_flag)
    {
        LOG_I("Make DirectCommand Error data");
        errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);

        vccomif::rdg::v1::interfaces::RdgCommonRequestHeader *const mRdgCommonRequestHeader{errorData.mutable_rdg_common_request_header()};
        vccomif::common::v1::AppCommonHeaderVehicleToCenter *const mAppCommonHeaderVehicleToCenter{mRdgCommonRequestHeader->mutable_app_common_header()};

        mAppCommonHeaderVehicleToCenter->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
        mAppCommonHeaderVehicleToCenter->set_electronic_pf(EPF_19EPF);
        mAppCommonHeaderVehicleToCenter->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);

        // Interface type
        mRdgCommonRequestHeader->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
        const int32_t timezoneOffSet_hour{TimeManager::getInstance().getOffset() / 60};
        const int32_t timezoneOffSet_minute{TimeManager::getInstance().getOffset() % 60};
        LOG_D("Get timezone offset: %d:%d ", timezoneOffSet_hour, timezoneOffSet_minute);
        mRdgCommonRequestHeader->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(timezoneOffSet_hour);
        mRdgCommonRequestHeader->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(timezoneOffSet_minute);

        const sp<CommonDefine::RDGLocationData> mLocation{LocationManagerAdapter::getInstance()->getLocationData()};
        errorData.set_collection_condition_id(colId); /*TBD*/
        const uint32_t counterValue{UploadManager::getInstance()->getCounterValue()};
        mRdgCommonRequestHeader->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, counterValue));
        errorData.set_counter_value(counterValue);
        (void)counterValue;
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

        // TODO data_creation_date
        int64_t current_time{0};
        current_time = ParamsDef::getCurrentAcquisiteTime();
        if (current_time >= 0)
        {
            errorData.set_data_creation_date(static_cast<uint64_t>(current_time));
        }
        else
        {
            // Do nothing
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
        std::string file_dir{std::to_string(uploadId)};
        (void)file_dir.append("_UploadErrorDirectCommandDataRequest.dat");
        (void)DataModel<UploadErrorDataRequest>::save(file_dir, errorData);
        const android::sp<UploadTask> task{new UploadTask(uploadId)};

        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
        task->setUploadPatch(file_dir);
        task->setUploadId(static_cast<uint64_t>(uploadId));
        /*Set priority*/
        task->setUploadPrio(mPriority);
        task->setSrvcAcFlag(srvc_ac_flag);
        const uint64_t fileSize{static_cast<uint64_t>(errorData.ByteSizeLong())};
        task->setFileSize(fileSize);
        UploadManager::getInstance()->requestUploadTask(task);
        mDirectCommandUploadErrorData->Clear();
    }

    void RemoteDirectCommand::abortDirectCommand(const vccomif::rdg::v1::interfaces::ResponseCode resCode)
    {
        if (this->getAppStatus() == RemoteDirectCommand::DIRECTCOMMAND_RUNNING)
        {
            this->changeAppStatus(RemoteDirectCommand::DIRECTCOMMAND_STOP);
            LOG_I("abortDirectCommand, code: %d ", resCode);
            bAbortDirectCommand = true;
            if (mCurrDirectCommandTrans->getState() != DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DONE)
            {
                this->mTimeOutTrans->stop();
                mCurrDirectCommandTrans->disconnect();
            }
            if (!mDirectCommandTransQueue.empty())
            {
                mDirectCommandTransQueue.clear();
            }
            if (mDirectCommandResponse.size() != 0U)
            {
                mDirectCommandResponse.clear();
            }
            bUploadErrorData = true;
            mDirectCommandUploadErrorData->set_response_code(resCode);
            this->finishDirectCommand();
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            this->notifyTriggerDone(this->mTriggerId, true, false, true);
        }
    }

    void RemoteDirectCommand::makeHeaderForUploading(const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequest> &directCommandUploadData, const bool isIncreaseCount)
    {
        LOG_I("Make DirectCommand Header upload data");
        // RdgCommonRequestHeader

        vccomif::rdg::v1::interfaces::RdgCommonRequestHeader *const mRdgCommonRequestHeader{directCommandUploadData->mutable_rdg_common_request_header()};
        vccomif::common::v1::AppCommonHeaderVehicleToCenter *const mAppCommonHeaderVehicleToCenter{mRdgCommonRequestHeader->mutable_app_common_header()};

        mAppCommonHeaderVehicleToCenter->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
        mAppCommonHeaderVehicleToCenter->set_electronic_pf(EPF_19EPF);
        mAppCommonHeaderVehicleToCenter->set_geodesy_information(::vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
        // Interface type
        mRdgCommonRequestHeader->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_DIRECT_COMMAND_DATA);

        // Set collection condition ID of occurrence RoB (fixed value) in the case of occurrence RoB   directCommandUploadData->set_collection_condition_id(ixd);
        directCommandUploadData->set_collection_condition_id(mColId);
        // messageId
        // set message id use the time when creating file
        const int32_t timezoneOffSet_hour{TimeManager::getInstance().getOffset() / 60};
        const int32_t timezoneOffSet_minute{TimeManager::getInstance().getOffset() % 60};

        LOG_D("Get timezone offset: %d:%d ", timezoneOffSet_hour, timezoneOffSet_minute);

        mRdgCommonRequestHeader->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(timezoneOffSet_hour);
        mRdgCommonRequestHeader->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(timezoneOffSet_minute);
        if (isIncreaseCount)
        {
            const uint32_t counterValue{UploadManager::getInstance()->getCounterValue()};
            mRdgCommonRequestHeader->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_DIRECT_COMMAND_DATA, counterValue));
            directCommandUploadData->set_counter_value(counterValue);
            (void)counterValue;
        }
        else
        {
            mRdgCommonRequestHeader->set_message_id("01-20231231154859-05"); // temporary set default
            directCommandUploadData->set_counter_value(0U); // temporary set default
        }
        // Set Time data diagnostics_acquisition_time
        // Set Location data *location

        vccomif::rdg::v1::interfaces::Location *const mLocation{directCommandUploadData->mutable_location()};

        if (mTriggerType != DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
        {
            mDirectCommandAcquisitionTime = (mDirectCommandAcquisitionTime >= 0) ? mDirectCommandAcquisitionTime : 0;
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
            LOG_D("Get Current Time success: %lld", mDirectCommandAcquisitionTime);
            LOG_D("Get location data success, %f %f ", mLocationData->getLatitude(), mLocationData->getLongtitude());
        }
        else
        {
            const int64_t acquisitionTime{this->mRobNotificationReq->triggerTime()};
            if (acquisitionTime >= 0)
            {
                directCommandUploadData->set_diagnostics_acquisition_time(static_cast<uint64_t>(acquisitionTime));
            }
            else
            {
                directCommandUploadData->set_diagnostics_acquisition_time(0U);
            }
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
            mRoBInformation->set_rob(mRobNotificationReq->getTargetCollectionDataRobSsr()->rob_information().rob());
            vccomif::rdg::v1::interfaces::EcuAddressInformation *const ecuInformation{mRoBInformation->mutable_ecu_address_information()};
            ecuInformation->CopyFrom(mRobNotificationReq->getTargetCollectionDataRobSsr()->ecu_address_information());
            mRoBInformation->set_memory_selection(mRobNotificationReq->getTargetCollectionDataRobSsr()->rob_information().memory_selection());
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
            bAbortDirectCommand = true;
            if (mCurrDirectCommandTrans->getState() != DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DONE)
            {
                this->mTimeOutTrans->stop();
                mCurrDirectCommandTrans->disconnect();
            }
            mDirectCommandResponse.clear();
            bUploadErrorData = false;
            this->finishDirectCommand();
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            this->notifyTriggerDone(this->mTriggerId, true, false, false);
            mSuspended = false;
        }
    }

    uint8_t RemoteDirectCommand::getOperation() const
    {
        uint8_t operation{DiagManagerAdapter::COLLECTION_CONDITIONS};
        if (RemoteDirectCommand::getInstance()->mTriggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
        {
            operation = DiagManagerAdapter::ROB_NOTIFICATION_TRIGGER;
        }
        else if (RemoteDirectCommand::getInstance()->mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
        {
            operation = DiagManagerAdapter::RD_SCHEDULE_TRIGGER;
        }
        else if (RemoteDirectCommand::getInstance()->mTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
        {
            operation = DiagManagerAdapter::COLLECTION_CONDITIONS;
        }
        else
        {
            operation = DiagManagerAdapter::COLLECTION_CONDITIONS;
        }
        return operation;
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
            break;
        }
        case CMD_DIRECTCOMMAND_EVENT_UDS_RES:
        {
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
            current_time = ParamsDef::getCurrentAcquisiteTime();
            /* Get priority from Center Request */
            uint32_t prio_tmp{0U};
            if (handlemsg->arg1 > 0)
            {
                prio_tmp = static_cast<uint32_t>(handlemsg->arg1);
            }
            /* Get collection condition ID*/
            const android::sp<::Buffer> buf{new ::Buffer(handlemsg->buffer)};
            uint64_t colId{0U};
            if(buf->data() != nullptr) {
                (void)memcpy(&colId, buf->data(), sizeof(uint64_t));
            } else {
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
            current_time = ParamsDef::getCurrentAcquisiteTime();
            /* Get priority from Center Request */
            uint32_t prio_tmp{0U};
            if (handlemsg->arg1 > 0)
            {
                prio_tmp = static_cast<uint32_t>(handlemsg->arg1);
            }
            /* Get collection condition ID*/
            const android::sp<::Buffer> buf{new ::Buffer(handlemsg->buffer)};
            uint64_t colId{0U};
            if(buf->data() != nullptr) {
                (void)memcpy(&colId, buf->data(), sizeof(uint64_t));
            } else {
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
            LOG_I("Trigger Request have Type: %d Func: %d Prio: %d ID: %d ColID: %llu",
                  pTrigger->getType(), pTrigger->getFunc(), pTrigger->getPriority(), pTrigger->getTriggerId(), pTrigger->getCollectionID());
            PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
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
            RemoteDirectCommand::getInstance()->onTransmissionTimeout();
            break;
        }
        case DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_IG_OFF_TRIGGER:
        {
            RemoteDirectCommand::getInstance()->abortDirectCommand(vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
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
        const android::sp<OBCTransportInfo> mtransportInfo{new OBCTransportInfo()};
        const android::sp<OBCConnectInfo> mConnectInfo{new OBCConnectInfo()};
        const uint8_t mprotocolType{RemoteEcuInformation::getInstance()->convertObcProtocolType(this->mEcuInformation.getCommProtocol(), this->mEcuInformation.getCommType(), this->mEcuInformation.getTargetAddress())};
        OBCCanInfo mcanInfo{};
        std::vector<std::string> ntaArray{};
        ntaArray.clear();
        if ((mprotocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
            (mprotocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
            (mprotocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
        {
            const uint16_t nTa{static_cast<uint16_t>(((this->mEcuInformation.getTargetAddress() >> 8U) & 0xFFU))};
            std::stringstream ss{};
            ss << &std::hex << &std::uppercase << std::setw(2) << std::setfill('0') << nTa;
            const std::string hexString{ss.str()}; // Convert to string
            for (size_t i{0U}; i < hexString.size(); i++)
            {
                const std::string tmp{std::string(1U, hexString[i])};
                ntaArray.push_back(tmp);
            }
        }
        mcanInfo.setData(this->mEcuInformation.getTargetAddress(), ntaArray);
        mtransportInfo->setData(mprotocolType, mcanInfo, false, 0U);
        const std::string applicationName{"remotediag"};
        (void)OnboardclientAdapter::getInstance()->connect(mtransportInfo, applicationName, mConnectInfo);
        if (mConnectInfo->getResponse() == OBCEnum::OBCErrCode::OBC_OK)
        {
            this->mConnectId = mConnectInfo->getConnectId();
            this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_CONNECT;
            OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
            this->sendUds();
        }
        else
        {
            LOG_E("Can't connect to canid = %02X", this->mEcuInformation.getTargetAddress());
            OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
            this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DONE;
            (void)RemoteDirectCommand::getInstance()->mRemoteDirectCommandHandler->obtainMessage(MainHandler::CMD_DIRECTCOMMAND_FINISH_TRANSMISSION)->sendToTarget();
        }
    }

    void RemoteDirectCommand::DirectCommandTransmission::disconnect(void)
    {
        LOG_I("DirectCommandTransmission::disconnect()");
        (void)OnboardclientAdapter::getInstance()->disconnectECU(this->mConnectId);
        OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
        this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_DONE;
    }

    void RemoteDirectCommand::DirectCommandTransmission::sendUds(void)
    {
        LOG_I("DirectCommandTransmission::sendUds()");
        mSentUds = this->udsReqList.front();
        this->udsReqList.pop_front();

        if (mSentUds != nullptr && mSentUds->data() != nullptr && mSentUds->size() > 0)
        {
            uint8_t sid = mSentUds->data()[0];
            if (!SIDFilter::getInstance()->isAllowed(sid))
            {
                LOG_E("[SIDFilter] DirectCommand UDS SID 0x%02X is BLOCKED by SIDFilter! Aborting transmission.", sid);
                this->disconnect();
                return;
            }
            LOG_I("[SIDFilter] DirectCommand UDS SID 0x%02X is ALLOWED.", sid);
        }

        const uint8_t res{OnboardclientAdapter::getInstance()->sendUdsData(this->mConnectId, mSentUds)};
        if (res != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK))
        {
            // TODO Handle exeptional case
            this->disconnect();
        }
        else
        {
            if(mSentUds->data() != nullptr) {
                if ((this->mEcuInformation.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) &&
                    (mSentUds->data()[0] == 0x10U) && (mSentUds->data()[1] == 0x01U))
                {
                    bDefaultSession = true;
                }
                else
                {
                    bDefaultSession = false;
                }
            } else {
                LOG_D("udsRequest->data() is null");
            }

            LOG_I("SendUdsData success, isDefault %d", bDefaultSession);
            this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_UDS;
            RemoteDirectCommand::getInstance()->startTimer(DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_TRANSMISSION_TIMEOUT);
            LOG_I("Start timeout timer for connectId %lu", this->mConnectId);
        }
    }
    void RemoteDirectCommand::DirectCommandTransmission::setUdsReq(const android::sp<Buffer> udsReq)
    {
        this->mSentUds = udsReq;
    }

    android::sp<Buffer> RemoteDirectCommand::DirectCommandTransmission::getUdsReq() const noexcept
    {
        return this->mSentUds;
    }

    void RemoteDirectCommand::onOccurrentRobDetected(const android::sp<OccurrentRobNotification> notification)
    {
        LOG_D("onOccurrentRobDetected, collectionConditionId = %llu", notification->collectionConditionId());
        LOG_D("                        time = %lld", notification->triggerTime());
        LOG_D("                        latitude = 0x%08X", notification->triggerLocation()->getLatitude());
        LOG_D("                        longitude = 0x%08X", notification->triggerLocation()->getLongtitude());
        LOG_D("                        targetAddress = 0x%02x", notification->getTargetCollectionDataRobSsr()->ecu_address_information().target_address());
        LOG_D("                        priority = %d", notification->priority());
        this->mRobNotificationReq = notification;
        const int32_t prio{notification->priority()};
        if (prio >= 0)
        {
            this->triggerDirectCommand(DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER, static_cast<uint32_t>(prio), notification->collectionConditionId(), notification->triggerTime());
        }
        else
        {
            LOG_E("prio < 0");
        }
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

    void RemoteDirectCommand::testingMaxFileSize(const uint16_t fileSize) noexcept
    {
        DIRECTCOMMAND_UPLOAD_DATA_SIZE_MAX = static_cast<uint32_t>(fileSize);
        DIRECTCOMMAND_LAST_UPLOAD_DATA_SIZE_MAX = DIRECTCOMMAND_UPLOAD_DATA_SIZE_MAX / 2U;
        DIRECTCOMMAND_TOTAL_UPLOAD_DATA_SIZE_MAX = DIRECTCOMMAND_UPLOAD_DATA_SIZE_MAX * 2U + DIRECTCOMMAND_LAST_UPLOAD_DATA_SIZE_MAX;
    }
}
