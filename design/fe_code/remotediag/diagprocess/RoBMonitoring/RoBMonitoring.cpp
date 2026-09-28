#include <google/protobuf/util/json_util.h>
#include "RoBMonitoring.h"
#include <climits>

namespace rdgapp {

RoBMonitoring* RoBMonitoring::mRoBMonitoring{nullptr};

RoBMonitoring::RoBMonitoring (const Remotediag& app, android::sp<sl::SLLooper>& privLooper)
        : android::RefBase()
        , mApp(app)
        , mHandler{new MainHandler(privLooper, *this)}
        , mTimerHandler{}
        , mIGState{false}
        , mPriority{0U}
        , mPriorityId{0}
        , mIsMonitoringRunning{false}
        , mCurrentAbortState{Abort::ABORT_INIT}
        , mColId{0U}
        , mIsPrioritySuspend{false}
        , mIsSendLastUploadData{true}
        , mUnderRepair{false}
        , mPPIFlag{false}
        , mTriggerType(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)
        , mMonitoringUploadErrorData{new UploadErrorDataRequest()}
{
    LOG_I("Initialize");
    mRoBMonitoring = this;        
    (void)mHandler->obtainMessage(MainHandler::CMD_INIT_ROB_MONITORING)->sendToTarget();
}

RoBMonitoring::~RoBMonitoring() noexcept{
    if (mTimerHandler != nullptr) {
        delete mTimerHandler;
    }
}

RoBMonitoring* RoBMonitoring::getInstance() {
    if (mRoBMonitoring == nullptr) {
        LOG_I("RoBMonitoring is not created");
    }
    return mRoBMonitoring;
}

void RoBMonitoring::printData(const std::string data) const
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

void RoBMonitoring::notifyBootComplete() const {
    LOG_I({"boot is completed"});
}

void RoBMonitoring::onReceiveIG(const bool status) const {
    (void)mHandler->obtainMessage(MainHandler::CMD_CHANGE_IG_STATUS, static_cast<int32_t>(status))->sendToTarget();
}

void RoBMonitoring::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) 
{
    if (mIsMonitoringRunning == true) {
        (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UDS_RESPONSE, responseEventInfo)->sendToTarget();
    }    
}

void RoBMonitoring::onChangedRemoteInfo(const int32_t what, const int32_t info) {
    LOG_I("onChangedRemoteInfo");
    (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE, what, info)->sendToTarget();
}

void RoBMonitoring::MainHandler::handleMessage(const android::sp<sl::Message>& handlemsg) {
    const int32_t what {handlemsg->what};

    switch (what) {
        case CMD_INIT_ROB_MONITORING:
        {
            LOG_I("CMD_INIT_ROB_MONITORING");
            mMonitoring.initRoBMonitoring();
            break;
        }
        case CMD_CHANGE_IG_STATUS:
        {
            LOG_I("CMD_CHANGE_IG_STATUS");
            mMonitoring.handleIGStatus((handlemsg->arg1 != 0) ? true : false);
            break;
        }
        case CMD_MONITORING_START_TRANMISSION:
        {
            LOG_I("CMD_MONITORING_START_TRANMISSION");
            mMonitoring.startTranmission();
            break;
        }
        case CMD_RECEIVE_STATUS_FROM_CENTER:
        {
            LOG_I("CMD_RECEIVE_STATUS_FROM_CENTER");
            mMonitoring.handleCollectionConditionUpdate();
            break;
        }
        case CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE:
        {
            LOG_I("CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE");
            mMonitoring.changedUnderRepairState(static_cast<int32_t>(handlemsg->arg1), static_cast<int32_t>(handlemsg->arg2));
            break;
        }
        case CMD_TIMER_EXPIRED:
        {
            mMonitoring.handleTimerExpired(handlemsg->arg1);
            break;
        }
        case CMD_REQUEST_TO_PRIORITY_CONTROL:
        {
            LOG_I("CMD_REQUEST_TO_PRIORITY_CONTROL");
            sp<DiagTrigger> pTrigger {nullptr};
            handlemsg->getObject(pTrigger);
            LOG_I("Trigger Request have Type: %d Func: %d Prio: %d ID: %u ColID: %llu", 
                pTrigger->getType(), pTrigger->getFunc(), pTrigger->getPriority(), pTrigger->getTriggerId(), pTrigger->getCollectionID());
            PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
            break;
        }
        case CMD_MONITORING_PRIORITY_TRIGGER:
        {
            LOG_I("CMD_MONITORING_PRIORITY_TRIGGER");
            const int32_t pTriggerType{static_cast<int32_t> (handlemsg->arg1)};
            mMonitoring.handleMonitoringRequest(pTriggerType);
            break;
        }
        case CMD_ROBMONITORING_FINISH_TRANSMISSION:
        {
            LOG_I("CMD_ROBMONITORING_FINISH_TRANSMISSION");
            mMonitoring.finishCurrentTransmission();
            break;
        }
        case CMD_RECEIVE_UDS_RESPONSE:
        {
            LOG_I("CMD_RECEIVE_UDS_RESPONSE");
            android::sp<OBCResponseEventInfo> responseEventInfo{nullptr};
            handlemsg->getObject(responseEventInfo);
            if(responseEventInfo != nullptr) 
            {
                const android::sp<::Buffer> udsData{responseEventInfo->resInfo()->udsData()};
                const android::sp<UdsMessage> udsResponse{new UdsMessage()};
                if (udsData->size() > 0U)
                {
                    (void)udsResponse->Parser(udsData);
                }
                mMonitoring.handleUdsResponse(responseEventInfo, udsResponse);
            } else {
                LOG_E("responseEventInfo is nullptr");
            }
            break;
        }
        case CMD_STOP_RDG:
        {
            const int32_t isStop{handlemsg->arg1};
            if(isStop == 1) {
                LOG_I("CMD_STOP_RDG");
                mMonitoring.handleStopRDG();
            } else {
                LOG_I("Power source enable RDG");
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void RoBMonitoring::initRoBMonitoring() {
    LOG_I("Init Monitoring");
    mMonitoringUploadData = getRoBInformationList();
    mIsPrioritySuspend = false;
}

void RoBMonitoring::onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) {
    LOG_I("Monitoring receive Center request");
    //TODO : CMD_READ_MONITORING_LIST 
    const uint32_t prio_data{pCenterReqData->getCenterReq_prio()};
    const uint64_t colID{pCenterReqData->getCenterReq_CollectionID()};
    uint8_t colId_ptr[sizeof(colID)] {0U};
    (void)memcpy(&colId_ptr[0], &colID, sizeof(colId_ptr));
    const android::sp<::Buffer> colId_sp {new ::Buffer()};
    colId_sp->setTo(&colId_ptr[0], sizeof(colID));
    if(prio_data > static_cast<uint32_t>(INT32_MAX)) {
        LOG_D("invalid prio_data value");
    } else {
        const uint32_t bufferSize{colId_sp->size()}; 
        if(bufferSize > static_cast<uint32_t>(INT32_MAX)){
            LOG_D("invalid bufferSize value");
        } else{
            const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_RECEIVE_STATUS_FROM_CENTER, static_cast<int32_t>(prio_data))};
            msg->buffer.setTo(colId_sp->data(), static_cast<int32_t>(bufferSize));
            (void)msg->sendToTarget();
        }
    }
}

void RoBMonitoring::onRdgStop(const bool isStop) const noexcept {
    //ontain message to stop rdg
    (void)mHandler->obtainMessage(MainHandler::CMD_STOP_RDG, static_cast<int32_t>(isStop))->sendToTarget();
}

void RoBMonitoring::handleChangeErasePPIFlag() const {

    LOG_I("handleChangeErasePPIFlag");
}

void RoBMonitoring::handleIGStatus(const bool state) {
    LOG_I({"handleIGStatus, state=%d"}, state);
    mIGState = state;   
    if((mIGState == false) && (mIsMonitoringRunning == true)) {     //RDG30-R-0931
        LOG_I("handleIGStatus : IG ON -> OFF while monitoring process is running");
        mCurrentAbortState = Abort::ABORT_IG_STATE_CHANGE;
        //abort immediately
        mIsSendLastUploadData = false;
        if(mCurrentTransmissionId != 0U) {
            const TransmissionInter it {mMonitoringTransList.find(mCurrentTransmissionId)};
            if (it != mMonitoringTransList.end())
            {
                it->second->disconnect();
            }
        } else {
            abortRoBMonitoring(mCurrentAbortState);
        }
    }
}

void RoBMonitoring::handleCollectionConditionUpdate() {
    LOG_I("handleCollectionConditionUpdate getCenterRequest"); 
    mMonitoringRequestData = CollectionCondition::getInstance().getCollectionConditionRobRobSsrDidEvent();
    if(mMonitoringRequestData != nullptr){
        /*Get schedule information*/
        if(mMonitoringRequestData->has_schedule_information() == true) {
                /*Get collection ID*/
            const uint64_t colID {static_cast<uint64_t>(mMonitoringRequestData->collection_condition_id())};
            LOG_D("CollectionConditionRobRobSsrDidEvent check id: %llu", colID); 
            const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data{mMonitoringRequestData->schedule_information()};
                /*get priority*/
            const uint32_t prio {static_cast<uint32_t>(schedule_data.priority())}; 
            LOG_I("Check prio: %d ", prio);              
            requestPriority(prio, colID);
        } else{
            LOG_I("Schedule is Empty");
        }
    } else {
        LOG_I("Monitoring section is NULL");
    }
}

void RoBMonitoring::changedUnderRepairState(const int32_t what, const int32_t info){
    switch (what) {
        case WHAT_CHANGED_REPAIR_SATUS:
        {
            LOG_I("WHAT_CHANGED_REPAIR_SATUS: %d", info);
            if(info == 1){   //Under repair states: 1 - true / 0 - false
                mUnderRepair = true;
            } else {
                mUnderRepair = false;
            }
            LOG_I("mUnderRepair (%d)", mUnderRepair);
            if((mUnderRepair == true) && (mIsMonitoringRunning == true)) {  
                mCurrentAbortState = Abort::ABORT_UNDER_REPAIR;
                mIsSendLastUploadData = false;
                //revert abort immediately logic by https://toyota-11f.rickcloud.jp/jira/browse/DCM24SPEC-18283
                // if(mCurrentTransmissionId != 0U) {
                //     const TransmissionInter it {mMonitoringTransList.find(mCurrentTransmissionId)};
                //     if (it != mMonitoringTransList.end())
                //     {
                //         it->second->disconnect();
                //     }
                // } else {
                //     abortRoBMonitoring(mCurrentAbortState);
                // }

            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void RoBMonitoring::handleMonitoringRequest(const int32_t pTriggerType) {
    LOG_I({"handleMonitoringRequest"});
    //Check precondition
    mTranmissionList.clear();
    if (checkPrecondition() && (mUnderRepair == false)) {   //mUnderRepair : refer table 10-17
        LOG_I({"Satisfied Precondition"});
        if(isValidCenterRequest()){
            mIsSendLastUploadData = true;
            //TODO: get Monitoring RoB list from file
            if(DataModel<CollectionConditionRobRobSsrDidEvent>::checkExist(OCCURRENCE_ROB_MONITORING_DB_NAME)){
                LOG_I("RoB Monitoring list is available");
            } else {
                //creat new RoBInformation list 
                mMonitoringUploadData = std::shared_ptr<CollectionConditionRobRobSsrDidEvent>(new CollectionConditionRobRobSsrDidEvent());
                LOG_I("create default RoB Information list");
                (void)saveRoBInformationList(*mMonitoringUploadData);
            }

            mMonitoringUploadData = getRoBInformationList();
            mMonitoringList = std::shared_ptr<CollectionConditionRobRobSsrDidEvent>(new CollectionConditionRobRobSsrDidEvent());  

            if(mMonitoringUploadData != nullptr){   // RDG30-R-0924
                //TODO: Create Occurrence RoB monitoring list 
                int32_t targetSizeEMMC {mMonitoringUploadData->target_collection_data_size()};
                const int32_t targetSizeCenter {mMonitoringRequestData->target_collection_data_size()};

                LOG_I("Target size : 1.EMMC (%d) - 2.Center (%d)", targetSizeEMMC, targetSizeCenter);
                
                if(targetSizeEMMC == 0) {     
                    // RDG30-R-0926
                    LOG_I("Creat new RoB Information list");
                    //TODO: creat RoB Information list

                } else {    //RDG30-R-0925
                    LOG_I("Using old RoB Information list");
                }
                
                //Get ECU Information List
                RemoteEcuInformation::getInstance()->getEcuInformationList(mEcuInformationList);
                LOG_I("Check mEcuInformationList size = %d", mEcuInformationList.size());
                if (mEcuInformationList.size() == 0U)
                {
                    LOG_E("ECU list is none");
                    DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
                }
                
                    //list ECU in Collection condition
                for(int32_t idx {0}; idx < targetSizeCenter; ++idx) {
                    const EcuAddressInformation ecuInfo {mMonitoringRequestData->target_collection_data(idx).ecu_address_information()};
                    LOG_D("Browsing ecu in center request EcuAddress (0x%02x)", ecuInfo.target_address());
                    if(isValidECUAddress(ecuInfo)) { 
                        CommonDefine::EcuInformation ecuInformation{};
                        if(isAliveECU(ecuInfo, ecuInformation)){
                            // monitoring progress
                            LOG_D("ECU (0x%02x) is IG-ON => start monitoring progress", ecuInfo.target_address());
                            if (isMakeRequestMonitor(ecuInfo) == false) {
                                if((isUnderMonitor(ecuInfo) == true)){  //RDG30-R-0744 stop and reconfigured 
                                    std::pair<CommonDefine::EcuInformation, RoBMonitoring::MonitoringTransmission::Type> tranmission{};
                                    tranmission.first = ecuInformation;
                                    tranmission.second = RoBMonitoring::MonitoringTransmission::Type::MONITORING_RECONFIGURE;
                                    mTranmissionList.push_back(tranmission);
                                } else {   //RDG30-R-0743 configure -> start
                                    std::pair<CommonDefine::EcuInformation, RoBMonitoring::MonitoringTransmission::Type> tranmission{};
                                    tranmission.first = ecuInformation;
                                    tranmission.second = MonitoringTransmission::Type::MONITORING_CONFIGURE;
                                    mTranmissionList.push_back(tranmission);
                                }
                                LOG_D("Make request successfully");
                            } else {
                                LOG_D("Don't make monitoring request due to duplication/invalid ECU");
                            }
                        } else {
                            LOG_D("ECU (0x%02x) is IG-OFF => monitoring of occurrence RoB is not performed", ecuInfo.target_address());   //RDG30-R-0093
                        }
                    } else {
                        LOG_D("Push ecu to ECU Information error");                                     //RDG30-R-1130
                        DiagnosticsMessage diagMess{}; 
                        diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_ECU_SPECIFIED_ERROR);
                        std::pair<EcuAddressInformation, DiagnosticsMessage> input{};
                        input.first = ecuInfo;
                        input.second = diagMess;
                        mEcuInformationError.push_back(input);  //prepare for upload last error data
                        mIsSendLastUploadData = false;
                    }
                }

                //check again ECU size in EMMC after reconfig 
                targetSizeEMMC = mMonitoringUploadData->target_collection_data_size();
                LOG_D("Browsing ecu in DCM - size (%d)", targetSizeEMMC);
                    //list ECU in EMMC
                for(int32_t idx {0}; idx < targetSizeEMMC; ++idx) {             //RDG30-R-0556
                    const EcuAddressInformation ecuInfo {mMonitoringUploadData->target_collection_data(idx).ecu_address_information()};
                    //TODO: Check case mMonitoringUploadData was clear
                    LOG_D("Browsing ecu in DCM - ecu target Address (0x%02x)", ecuInfo.target_address());
                    if(isValidECUAddress(ecuInfo)) {                        //RDG30-R-1130
                        CommonDefine::EcuInformation ecuInformation{};
                        if(isAliveECU(ecuInfo, ecuInformation)){
                            // monitoring progress
                            if (isMakeRequestMonitor(ecuInfo) == false) {
                                std::pair<CommonDefine::EcuInformation, RoBMonitoring::MonitoringTransmission::Type> tranmission{};
                                tranmission.first = ecuInformation;
                                tranmission.second = MonitoringTransmission::Type::MONITORING_STOP;
                                mTranmissionList.push_back(tranmission);
                            } else {
                                LOG_D("Don't make stop request due to duplication");
                            }
                        } else {
                            LOG_D("ECU (0x%02x) is IG-OFF => monitoring of occurrence RoB is not performed", ecuInfo.target_address());   //RDG30-R-0093
                        }

                    } else {
                        LOG_D("ECU is not valid: 0x%02x ", ecuInfo.target_address());
                    }
                }
                //start send UDS progress
                makeRequestMonitoring();

            } else{
                LOG_I("Memory reads failure!!!");
                makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_OBE_ERROR_MEMORY_READ_FAILURE);   //RDG30-R-0550
                notifyTriggerDone();
            }

        } else {
            LOG_I("isValidCenterRequest return false");
            makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_UNEXPECTED_COMMAND);      //RDG30-R-0673 
            notifyTriggerDone();
        }

    } else if((!checkPrecondition()) && (mUnderRepair == false)) {                //RDG30-R-0672
        LOG_I("Precondition is not passed : Consent state (%d) - RDG Flag (%d) - IG state (%d) ", mConsentState , mRDGFlag, mIGState);
        if(!mConsentState){
            LOG_I("Consent status is not passed - End process");
            //Do nothing - End monitoring process
        } else if(!mRDGFlag){
            LOG_I("RDG status is false - Upload error data Unprovided vehicle");
            makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_UNPROVIDED_VEHICLE);
        } else if(mIGState == false) {         
            LOG_I("IG ON -> OFF : Upload error data Power condition is not met");
            makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION); // RDG30-R-0932
        } else {
            LOG_E("Do nothing");
        }

        notifyTriggerDone();
    } else {
        LOG_D("Under repair is true: %d", mUnderRepair);
        makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR);  //RDG30-R-1175
        notifyTriggerDone();
    }
    
    LOG_I("handleMonitoringRequest END");
    (void)pTriggerType;
}

void RoBMonitoring::createLastFile() const {
    LOG_I("createLastFile");
}

void RoBMonitoring::requestPriority(const uint32_t prio, const uint64_t colID){
    LOG_I("requestPriority");
    const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
    android::sp<DiagTrigger> pTrigger{new DiagTrigger(DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER,
                                                      prio,
                                                      DiagTrigger::DiagTriggerFunc::ROB_MORNITOR,
                                                      nextTriggerId)};
    /* Get current time */
    int64_t current_time {0};
    int64_t tmp_current_time_millis{CommonUtils::getCurrentAcquisiteTime() * static_cast<int64_t>(1000)};
    if(tmp_current_time_millis > 0) {
        current_time = tmp_current_time_millis/static_cast<int64_t>(1000);  //convert millisecond to second
    }    
    pTrigger->setTriggerTime(current_time);
    pTrigger->setCollectionId(colID);
    /* Save request to local*/
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
    LOG_I("Check mSaveReq size: %d", mSaveReq.size());
    if(!ret.second) {
        ret.first->second = pTrigger;
    }
    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
    LOG_I("Check saved priority trigger ID: %d", ret.first->first);
}

bool RoBMonitoring::checkPrecondition() {
    LOG_I("checkPrecondition ");
    bool iMonitoring {false};
    getServiceFlag();
    iMonitoring = mRDGFlag && mConsentState && mIGState;
    LOG_I({"checkPrecondition: Monitoring: %d, mRDG_Flag: %d, mConsentState: %d,  mIGState: %d"}, iMonitoring, mRDGFlag, mConsentState, mIGState);
    return iMonitoring;  //RDG30-R-0089
}

void RoBMonitoring::uploadIfNeeded() const {
    LOG_I("uploadIfNeeded");

}



void RoBMonitoring::handleTimerExpired(const int32_t timerID) {
    switch (timerID) {
        case TimerHandler::ID_TRANSMISSION_TIMEOUT:   
            LOG_I({"Time out of tranmission occurs"});
            onTransmissionTimeout();
            break;
        default:
            break;
    }
}

void RoBMonitoring::getServiceFlag(){

    const uint8_t RDGFlag{DiagManagerAdapter::getInstance()->getRDGFlag()};
    if(RDGFlag > 0U)
    {
        mRDGFlag = true;
    } else {
        mRDGFlag = false;
    }
    mConsentState = DiagManagerAdapter::getInstance()->getAllUploadConsent();
    const uint8_t underRepairStatus{mApp.getUnderRepair()};
    if(underRepairStatus > 0U){
        mUnderRepair = true;
    } else {
        mUnderRepair = false;
    }
    mIGState = PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_ON ? true : false;
    LOG_I("getServiceFlag from DiagManagerAdapter");
}

bool RoBMonitoring::notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff) 
{
    bool isMonitoringTrigger{false};
    /* Check if trigger is in the saved trigger*/
    if(pTriggerId < 0){
        LOG_D("invalid pTriggerId value");
    } else {
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(static_cast<uint32_t>(pTriggerId))};
        if(it != mSaveReq.end()) {
            LOG_I("notify RoB Monitoring state: %d TriggerID: %d TriggerTime: %lld", pState, pTriggerId, it->second->getTriggerTime());
            isMonitoringTrigger = true;
            handleTrigger(pState, pTriggerId, dueToIgOff);
        } else {
            LOG_I("Can not find triggerID in saved list");
            isMonitoringTrigger = false;
            PriorityControl::getInstance()->notifyTriggerNoFound(pTriggerId);
        }
    }
    return isMonitoringTrigger;
}

void RoBMonitoring::handleTrigger(const DiagTrigger::DiagTriggerState& pState,
    const int32_t& pTriggerId, const bool dueToIgOff) 
{
    if(pTriggerId < 0){
        LOG_D("invalid pTriggerId value");
    } else {
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(static_cast<uint32_t>(pTriggerId))};
        if(it != mSaveReq.end()) {
            // uint32_t prio_data{it->second->getPriority()};
            it->second->setState(pState);
            LOG_I("notify Warning Trigger state: %d TriggerID: %d", pState, pTriggerId);
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
                    android::sp<DiagTrigger> const trigger{it->second};
                    const DiagTrigger::DiagTriggerType pTriggerType {trigger->getType()};
                    const uint32_t prio_data{trigger->getPriority()};
                    const uint64_t colID{trigger->getCollectionID()};
                    mPriorityId = pTriggerId;
                    mColId = colID;
                    mTriggerType = pTriggerType;
                    mPriority = prio_data;
                    if(mIsPrioritySuspend){     //RDG30-R-0411 resume after suspend
                        LOG_I("TRIGGER_PROCESSING AFTER SUSPEND");
                        startTranmission();
                        mIsPrioritySuspend = false;
                    }
                    else{
                        LOG_I("START TRIGGER PRIORITY PROCESS ID(%d)", mPriorityId);
                        (void)mHandler->obtainMessage(MainHandler::CMD_MONITORING_PRIORITY_TRIGGER, static_cast<int32_t>(pTriggerType))->sendToTarget();
                    }
                    break;
                }
                case DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED:
                {
                    LOG_I("TRIGGER_SUSPENDED");
                    mIsPrioritySuspend = true;
                    break;
                }
                case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
                {
                    LOG_I("TRIGGER_DISCARDED");
                    mCurrentAbortState = Abort::ABORT_PRIORITY_DISCARDED;
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
                {
                    LOG_I("DEFAULT CASE");
                    break;
                }
            }
        }
    }
    (void)dueToIgOff;
}


void RoBMonitoring::resetUploadData() const {

    LOG_I("resetUploadData");
}

void RoBMonitoring::abortRoBMonitoring(const Abort abortReason){
    if(abortReason == Abort::ABORT_PRIORITY_DISCARDED){     //RDG30-R-0931
        LOG_I("Abort Monitoring due to priority discard");
        mIsMonitoringRunning = false;
        makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED);
    } else if(abortReason == Abort::ABORT_IG_STATE_CHANGE){     //RDG30-R-0932
        LOG_I("Abort Monitoring due to IG state change from ON to OFF");
        mIsMonitoringRunning = false;
        makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);  // RDG30-R-0932
    } else if(abortReason == Abort::ABORT_UNDER_REPAIR){     //RDG30-R-1038 + RDG30-R-1172
        LOG_I("Abort Monitoring due to Under repair change from 0 to 1");
        mIsMonitoringRunning = false;
        makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR); 
    } else if(abortReason == Abort::ABORT_BUB){
        LOG_D("Abort due to BUB");
    } else {
        LOG_D("Abort init case");
    }
    LOG_I("Finish abort");
    //reset Abort State
    mCurrentAbortState = Abort::ABORT_INIT;
    mIsSendLastUploadData = true;
}

void RoBMonitoring::modifyRoBInformationList(void) const {

    LOG_I("modifyRoBInformationList");
}

void RoBMonitoring::notifyTriggerDone(){
    LOG_I("notifyTriggerDone");
    LOG_D("Check mSaveReq size before: %d", mSaveReq.size());
    if(mPriorityId < 0){
        LOG_D("invalid mPriorityId value");
    } else {
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mSaveReq.find(static_cast<uint32_t>(mPriorityId))};
        if (it != mSaveReq.end())
        {
            LOG_I("DONE TRIGGER PRIORITY PROCESS ID(%d)", mPriorityId);
            const DiagTrigger::DiagTriggerType tmp_type {it->second->getType()};
            if((tmp_type >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
                    (tmp_type <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)) {
                LOG_I("Trigger type is valid");
            } else {
                LOG_I("Trigger type is out of range");
            }
            PriorityControl::getInstance()->notifyTriggerProcessDone(mPriorityId, tmp_type);
            mPriorityId = 0;   //block condition of upload data
            (void)mSaveReq.erase(it);
        }
        else 
        {
	        LOG_I("Can not find triggerId: %d in mSaveReq", mPriorityId);
        }
    }
}

bool RoBMonitoring::isValidCenterRequest(){ //RDG30-R-0673 - table 8-18
    LOG_I("isValidCenterRequest start");
    bool iResult {true};
    // mMonitoringRequestData = CollectionCondition::getInstance().getCollectionConditionRobRobSsrDidEvent();
    const int32_t targetSize {mMonitoringRequestData->target_collection_data_size()};

    LOG_I("isValidCenterRequest declare value - request size (%d) ", targetSize);

    for(int32_t idx {0}; idx < targetSize; idx++){

        //Check RoB Size
        const int32_t robInfoSize {mMonitoringRequestData->target_collection_data(idx).rob_information_size()};
        const bool iRoBInfo {((robInfoSize > 0) && (robInfoSize <= 50))  ? true : false};   //range of rob size
        if(!iRoBInfo){
            LOG_D("iRoBInfo - robInfoSize (%d)", robInfoSize);
            iResult = false;
            break;
        }
        for(int32_t index {0}; index < robInfoSize; index++){
            const vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation robInformation{mMonitoringRequestData->target_collection_data(idx).rob_information(index)};
                //Check RoB Code
            const uint32_t roBCode {robInformation.rob()};

                //Check RoB Memory Selection RDG30-R-0790
            const uint32_t memorySelection {robInformation.memory_selection()};

                //Check RoB Priority
            const RobInformationPriority robPriority {robInformation.rob_priority()};

                //Check RoBFrameNumber
            const int32_t roBFrameNumber {robInformation.rob_frame_numbers_size()};

                // check Direct command
            const int32_t directCommandData {robInformation.diagnostics_commands_size()};

            if((roBCode > 0xFFFFFFU) || ((memorySelection != 0x11U) && (memorySelection != 0x12U) && (memorySelection != 0x13U) && (memorySelection != 0x14U))
                || ((robPriority != RobInformationPriority::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_RobPriority_RP_PRIORITY_LOW) 
                && (robPriority != RobInformationPriority::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_RobPriority_RP_PRIORITY_HIGH))
                || (roBFrameNumber > 255)       //rob_frame_numbers_size limit
                || (directCommandData > 512)){  //diagnostics_commands_size limit
                LOG_D("roBCode (%d) - index(%d)", roBCode, index);
                LOG_D("memorySelection (0x%02x) - index(%d)", memorySelection, index);
                LOG_D("robPriority (%d) - index(%d)", robPriority, index);
                LOG_D("roBFrameNumber (%d) - index(%d)", roBFrameNumber, index);
                LOG_D("directCommandData (%d) - index(%d)", directCommandData, index);
                iResult = false;
                break;
            }
        }
    }

    LOG_I("isValidCenterRequest END - result: %d", iResult); 
    return iResult;
}

error_t RoBMonitoring::saveRoBInformationList(const CollectionConditionRobRobSsrDidEvent& obj){
    const Mutex::Autolock mLock{mMutexMonitoring};
    const error_t ret {DataModel<CollectionConditionRobRobSsrDidEvent>::saveData(OCCURRENCE_ROB_MONITORING_DB_NAME, obj)};
    if(ret == TIGER_ERR::E_OK) {
        LOG_I("SUCCESS");
            // Self-Diag
        const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
        DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
    } else {
        LOG_I("FAILURE");
    }
    return ret;
}

error_t RoBMonitoring::clearRoBInformationList(void) const {
    const error_t ret {DataModel<CollectionConditionRobRobSsrDidEvent>::clearData(OCCURRENCE_ROB_MONITORING_DB_NAME)};
    if(ret == TIGER_ERR::E_OK) {
        LOG_I("SUCCESS");
    } else {
        LOG_I("FAILURE");
    }
    return ret;
}

std::shared_ptr<CollectionConditionRobRobSsrDidEvent> RoBMonitoring::getRoBInformationList(void) const {
    LOG_I("getRoBInformationList"); 
    return DataModel<CollectionConditionRobRobSsrDidEvent>::getData(OCCURRENCE_ROB_MONITORING_DB_NAME);
}

bool RoBMonitoring::isValidECUAddress(const EcuAddressInformation ecu){   //RDG30-R-1130
    bool isResult {false};
    const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol commProtocol{ecu.communication_protocol()};
    const uint32_t centerTargetAdd{ecu.target_address()};
    const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{ecu.communication_type()};
    if ((centerTargetAdd != 0U) &&
        ((commProtocol == vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD) ||
         ((commProtocol == vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN) &&
          ((commType == vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
           (commType == vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS)))))
    {
        //2. Target ECU does not exist RDG30-R-1164
        for (std::list<CommonDefine::EcuInformation>::iterator it {mEcuInformationList.begin()}; it != mEcuInformationList.end(); ++it)
        {
            if (((*it).getTargetAddress() == centerTargetAdd) &&
                ((*it).getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6))
            {
                isResult = true;
                break;
            } 
        }
    }

    LOG_I("ECU valid: %d, centerTargetAdd 0x%02X, commProtocol: %d, commType: %d", isResult, centerTargetAdd, commProtocol, commType); 
    return isResult;
}

bool RoBMonitoring::isUnderMonitor(const EcuAddressInformation ecu){   //RDG30-R-0928
    LOG_I("Check ECU (0x%02x)  isUnderMonitor or not", ecu.target_address());
    bool iResult {false};
    TargetCollectionDataOccurrentRobList::const_iterator it {mMonitoringUploadData->target_collection_data().cbegin()};
    while(it != mMonitoringUploadData->target_collection_data().cend())
    {
        if(ecu.target_address() == it->ecu_address_information().target_address())
        {
            iResult = true;
            break;
        } else {
            it++;
        }
    }
    LOG_I("isUnderMonitor result : %d", iResult);
    return iResult;
}

bool RoBMonitoring::isAliveECU(const EcuAddressInformation ecu, CommonDefine::EcuInformation& ecuInformation ){ //RDG30-R-0093
    bool isResult {false};
    std::list<CommonDefine::EcuInformation>::iterator it{};
    for (it = mEcuInformationList.begin(); it != mEcuInformationList.end(); ++it)
    {
        if ((*it).getTargetAddress() == ecu.target_address())
        {
            isResult = true;
            ecuInformation.setTargetAddress(ecu.target_address());
            ecuInformation.setCanId((*it).getCanId());
            ecuInformation.setNTa((*it).getNTa());
            const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocol{(*it).getCommProtocol()};
            if ((protocol >= vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN)
                && (protocol <= vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_IP))
            {
                ecuInformation.setCommProtocol(protocol);
            }
            else
            {
                LOG_E("invalid protocol");
            }
            ecuInformation.setCommType((*it).getCommType());
            const CommonDefine::DiagPhase diagPhase{(*it).getDiagPhase()};
            if((diagPhase >= CommonDefine::DiagPhase::DP_UNKNOW) && (diagPhase <= CommonDefine::DiagPhase::DP_PHASE_6))
            {
                ecuInformation.setDiagPhase(diagPhase);
            } else {
                LOG_E("Unknown diag phase");
            }
            break;
        }
    }
    return isResult;
}

bool RoBMonitoring::isMakeRequestMonitor(const EcuAddressInformation ecu) {  
    LOG_I("Check ECU (0x%02x)  was maked request Monitor or not", ecu.target_address());
    bool iResult {false};
    std::list<std::pair<CommonDefine::EcuInformation, RoBMonitoring::MonitoringTransmission::Type>>::iterator it{};
    for(it = mTranmissionList.begin(); it != mTranmissionList.end(); ++it){
        const CommonDefine::EcuInformation ecuInfo{it->first};
        if(ecuInfo.getTargetAddress() == ecu.target_address()) {
            iResult = true;
            LOG_D("Maked monitoring request");
            break;
        }
    }
    //For case the ecu is pushed error list and wait make error upload, don't stop monitoring this ecu
    std::list<std::pair<EcuAddressInformation, DiagnosticsMessage>>::iterator ite{};
    for(ite = mEcuInformationError.begin(); ite != mEcuInformationError.end(); ++ite){
        const EcuAddressInformation ecuInfo{ite->first};
        if(ecuInfo.target_address() == ecu.target_address()) {
            iResult = true;
            LOG_D("Maked ecu error list");
            break;
        }
    }

    LOG_I("isMakeRequestMonitor result : %d", iResult);
    return iResult;
}

RoBMonitoring::MonitoringTransmission::MonitoringTransmission( RoBMonitoring& robMonitoringData, const CommonDefine::EcuInformation ecuInformation, const Type typeData)
: android::RefBase()
, mMonitoring(robMonitoringData)
, mTimerHandler(robMonitoringData)
, mTimeOut(&mTimerHandler, TimerHandler::ID_TRANSMISSION_TIMEOUT)
, mConnectId(0U)
, mTransmissionId(0U)
, mState(State::MONITORING_TRANS_INIT)
, mType(typeData)
, mResSuccessCount(0U)
{
    uint8_t sID;
    sID = 0x86U;
    uint8_t eventWindowTime;
    eventWindowTime = 0x02U;
    uint8_t typeRecord1;
    typeRecord1 = 0xA0U;
    uint8_t typeRecord2;
    typeRecord2 = 0x05U;
    uint8_t serID;
    serID = 0x22U;
    uint8_t serParam1;
    serParam1 = 0xA0U;
    uint8_t serParam2;
    serParam2 = 0x06U;
    uint8_t sfidStop;
    sfidStop = 0x06U;
    uint8_t sfidSetting;
    sfidSetting = 0x03U;
    uint8_t sfidStart;
    sfidStart = 0x45U;
    
    { //STOP
        mUdsReqStop.setSID(sID);            //86 06 02	
        mUdsReqStop.setSFID(sfidStop);
        mUdsReqStop.getOptionData()->append(&eventWindowTime, 1);   //Adding 1 byte of data to the original content
    }

    { //SETTING
        android::sp<::Buffer> const optionData{mUdsReqSetting.getOptionData()};
        mUdsReqSetting.setSID(sID);         //86 03 02 A0 05 22 A0 06
        mUdsReqSetting.setSFID(sfidSetting);
        optionData->append(&eventWindowTime, 1);
        optionData->append(&typeRecord1, 1);
        optionData->append(&typeRecord2, 1);
        optionData->append(&serID, 1);
        optionData->append(&serParam1, 1);
        optionData->append(&serParam2, 1);
    }

    { //START
        mUdsReqStart.setSID(sID);           //86 45 02
        mUdsReqStart.setSFID(sfidStart);
        mUdsReqStart.getOptionData()->append(&eventWindowTime, 1);
    }

    mTimeOut.setDuration(TRANSMISSION_TIME_OUT_DURATION, 0U);
    this->mEcuInformation.setCanId(ecuInformation.getCanId());
    this->mEcuInformation.setCommProtocol(ecuInformation.getCommProtocol());
    this->mEcuInformation.setCommType(ecuInformation.getCommType());
    const CommonDefine::DiagPhase diagPhase{ecuInformation.getDiagPhase()};
    if((diagPhase >= CommonDefine::DiagPhase::DP_UNKNOW) && (diagPhase <= CommonDefine::DiagPhase::DP_PHASE_6))
    {
        this->mEcuInformation.setDiagPhase(diagPhase);
    } else {
        LOG_E("Unknown diag phase");
    }
    this->mEcuInformation.setecuActiveFlag(ecuInformation.getecuActiveFlag() == true ? true : false);
    this->mEcuInformation.setTargetAddress(ecuInformation.getTargetAddress());
    LOG_D("INIT tranmission -  targetAddress: 0x%02x", ecuInformation.getTargetAddress());
 
    if (this->mEcuInformation.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6)
    {
        mTransmissionId = this->mEcuInformation.getTargetAddress();
    } else {
        LOG_D("This ecu is not phase 6, Don't make transactionID");
    }
    LOG_I("TranmissionID (0x%llx) - Type: %s", mTransmissionId, enumToString(static_cast<uint32_t>(mType)).c_str());
}

void RoBMonitoring::makeRequestMonitoring(void){
    LOG_I("--------------------Start makeRequestMonitoring------------------"); 
    mMonitoringTransList.clear();
    std::queue<uint64_t> empty_queue{};
    std::swap(mTransmissionIdList, empty_queue);

    std::list<std::pair<CommonDefine::EcuInformation, RoBMonitoring::MonitoringTransmission::Type>>::iterator it{};
    for(it = mTranmissionList.begin(); it != mTranmissionList.end(); ++it){
        const RoBMonitoring::MonitoringTransmission::Type requestType{it->second};
        const android::sp<MonitoringTransmission> transmission{new MonitoringTransmission(*this, it->first, requestType)};
        mMonitoringTransList[transmission->transmissionId()] = transmission;
        mTransmissionIdList.push(transmission->transmissionId());
    }

    if(mTransmissionIdList.size() > 0U){
        //TODO: send UDS
        // mIsSendLastUploadData = true;
        startTranmission();
    }
    else
    {
        LOG_I("mMonitoringTransList is empty");
        finishMonitoringTranmission();
    } 
    LOG_I("--------------------Finish make request Monitoring------------------"); 
}

void RoBMonitoring::startTranmission(){
    LOG_I("startTranmission Monitoring");
    mIsMonitoringRunning = true;
    android::sp<OnboardclientAdapter> const OBCAdapter{OnboardclientAdapter::getInstance()};
    const OBCResourceEventCode resEventInfo {OBCAdapter->GetObcResource()};
    if (resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK)
    {
        LOG_E("RoBMon wait ObcResource");
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(MainHandler::CMD_MONITORING_START_TRANMISSION), 1000U);
    }
    else
    {
        LOG_I("Set obc resource OBC_GET_RESOURCE_WAIT");
        OBCAdapter->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
        LOG_I("mTransmissionIdList size = %d", mTransmissionIdList.size());
        mCurrentTransmissionId = mTransmissionIdList.front();
        const TransmissionInter itTrans {mMonitoringTransList.find(mCurrentTransmissionId)};
        if (itTrans != mMonitoringTransList.end())
        {
            itTrans->second->connect();
        }
    }
}

void RoBMonitoring::finishCurrentTransmission(){
    LOG_I("finish Current transmission ID: 0x%llx, mTransmissionIdList size = %d", mCurrentTransmissionId, mTransmissionIdList.size());
    /* TODO: Delete finished DTCUdsTransmission in mapl_DirectCommandTrans */
    if(mCurrentAbortState != Abort::ABORT_INIT){
        LOG_D("Abort Monitoring at current transmission");
        mMonitoringTransList.clear();
        std::queue<uint64_t> empty_queue{};
        std::swap(mTransmissionIdList, empty_queue);
    }

    if (mTransmissionIdList.size() > 1U)
    {         
        mTransmissionIdList.pop();
        mCurrentTransmissionId = mTransmissionIdList.front();
        if(!mIsPrioritySuspend){            //RDG30-R-0410
            const TransmissionInter it {mMonitoringTransList.find(mCurrentTransmissionId)};
            if (it != mMonitoringTransList.end())
            {
                it->second->connect();
            }
        } else {
            LOG_I("Stop setup monitoring due to priority suspend");  //RDG30-R-0411 + RDG30-R-0410
            if(mPriorityId < 0){
                LOG_D("invalid mPriorityId value");
            } else {
                const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator its{mSaveReq.find(static_cast<uint32_t>(mPriorityId))};
                if (its != mSaveReq.end()) {
                    LOG_I("DONE TRIGGER PRIORITY PROCESS ID(%d)", mPriorityId);
                    const DiagTrigger::DiagTriggerType tmp_type {its->second->getType()};
                    if ((tmp_type >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
                            (tmp_type <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)) {
                        LOG_I("Trigger type is valid");
                    } else {
                        LOG_I("Trigger type is out of range");
                    }
                    // Release OBC resource
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                    PriorityControl::getInstance()->notifyTriggerProcessDone(mPriorityId, tmp_type);
                } else {
                    LOG_E("cannot find mPriorityId");
                }
            }
        }
    }
    else
    {
        if (mTransmissionIdList.empty() != true)
        {
            mTransmissionIdList.pop();
        }
        // Release OBC resource
        OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
        LOG_I("mTransmissionIdList size = %d ->  finished the last ECU -> upload package", mTransmissionIdList.size());
        finishMonitoringTranmission();
    }

    LOG_I("FINISH Current transmission");
}

void RoBMonitoring::finishMonitoringTranmission(){
    //TODO:
    if(mPriorityId < 0){
        LOG_D("invalid mPriorityId value");
    } else {
        LOG_D("Check mSaveReq size before: %d", mSaveReq.size());
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mSaveReq.find(static_cast<uint32_t>(mPriorityId))};
        if (it != mSaveReq.end())
        {
            const std::list<std::pair<EcuAddressInformation, DiagnosticsMessage>>::iterator it_2{mEcuInformationError.end()};
            (void)mEcuInformationError.insert(it_2, mEcuInformationRes.begin(), mEcuInformationRes.end());  //RDG30-R-0552 + RDG30-R-0933
            if(mIsSendLastUploadData){
                makeUploadResponseData();   //RDG30-R-0551    
            } else if(mCurrentAbortState == Abort::ABORT_INIT){
                makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_ROB_MONITORING_FAILURE);  //RDG30-R-1130
            } else {
                abortRoBMonitoring(mCurrentAbortState);                
            }

            // mMonitoringUploadData = CollectionCondition::getInstance().getCollectionConditionRobRobSsrDidEvent();
            generateRoBInformationList();
            getServiceFlag();
            if(mIGState == true) {
                (void)saveRoBInformationList(*mMonitoringUploadData);  //RDG30-R-0522   +  RDG30-R-0927
            } else {
                LOG_I("IG ON -> OFF : Don't store Occurrence RoB Monitoring list");
            }

            std::string str_RoBInformationList{""};
            google::protobuf::util::JsonOptions option{};
            option.always_print_primitive_fields = true;
            option.preserve_proto_field_names = true;
            (void)google::protobuf::util::MessageToJsonString(*mMonitoringUploadData, &str_RoBInformationList, option);
            //for Testing
            LOG_D("RoB Information List: ");
            printData(str_RoBInformationList);
            mIsMonitoringRunning = false;
            mCurrentTransmissionId = 0U;
            LOG_I("DONE TRIGGER PRIORITY PROCESS ID(%d)", mPriorityId);
            const DiagTrigger::DiagTriggerType tmp_type {it->second->getType()};
            if ((tmp_type >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
                    (tmp_type <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)) {
                LOG_I("Trigger type is valid");
            } else {
                LOG_I("Trigger type is out of range");
            }
            PriorityControl::getInstance()->notifyTriggerProcessDone(mPriorityId, tmp_type);
            mPriorityId = 0;
            (void)mSaveReq.erase(it);
            //Reset data
            // mMonitoringRequestData = std::shared_ptr<CollectionConditionRobRobSsrDidEvent>(new CollectionConditionRobRobSsrDidEvent());
            // mMonitoringUploadData = std::shared_ptr<CollectionConditionRobRobSsrDidEvent>(new CollectionConditionRobRobSsrDidEvent());
            mEcuInformationList.clear();
        }
        else
        {
            LOG_I("Can not find triggerId: %d in mSaveReq", mPriorityId);
        }
    }
    LOG_I("FINISH ALL MONITORING TRANMISSION");
}

void RoBMonitoring::onTransmissionTimeout(){
    LOG_I("Event Transmission Timeout, currentTrans = 0x%llx", mCurrentTransmissionId);
    const TransmissionInter itTrans {mMonitoringTransList.find(mCurrentTransmissionId)};
    if (itTrans != mMonitoringTransList.end()) {
        mIsSendLastUploadData = false;
        android::sp<MonitoringTransmission> const curTransmission{itTrans->second};
        if(curTransmission->getStateData() == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_STOP){    //RDG30-R-1199
            //Don't upload response data
            LOG_D("Don't package response data of stop UDS");
            curTransmission->setStateData(MonitoringTransmission::State::MONITORING_TRANS_DISCONNECT);
            curTransmission->disconnect();
        } else {
            EcuAddressInformation ecuInfo{};
            DiagnosticsMessage diagMess{};
            convertEcuInformation(curTransmission->ecuInformation(), ecuInfo);
            diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_UNRESPONSIVE); // RDG30-R-0552
            const MonitoringTransmission::State currentState {curTransmission->getStateData()};
            if (currentState == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE) {
                packageUdsData(curTransmission->getUdsMessageReqData(MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE), diagMess);
            } else if (currentState == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START) {
                packageUdsData(curTransmission->getUdsMessageReqData(MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START), diagMess);
            } else {
                LOG_E("UN matching State");  //Note: Error code
            }
            mEcuInformationError.emplace_back(ecuInfo, diagMess);
            sendNextTransmission(itTrans);
        }
    } else {
        LOG_E("cannot find mCurrentTransmissionId");
    }
}

void RoBMonitoring::makeUploadErrorData(const vccomif::rdg::v1::interfaces::ResponseCode resCode){      //RDG30-R-1130
    LOG_I("makeUploadErrorData - EcuInformation size: %d", mEcuInformationError.size());
    if(mPriorityId < 0){
        LOG_D("invalid mPriorityId value");
    } else {
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator itr{mSaveReq.find(static_cast<uint32_t>(mPriorityId))};
        int64_t triggerOccurrenceTime{0};
        if (itr != mSaveReq.end())
        {
            triggerOccurrenceTime = itr->second->getTriggerTime();
        }
        else 
        {
            LOG_D("Can not find triggerId: %d in mSaveReq", mPriorityId);
        }
        mMonitoringUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
        //initialize
        vccomif::rdg::v1::interfaces::RdgCommonRequestHeader *const commonHeader{mMonitoringUploadErrorData->mutable_rdg_common_request_header()};
        vccomif::common::v1::AppCommonHeaderVehicleToCenter *const appCommonHeader{commonHeader->mutable_app_common_header()};
        vccomif::common::v1::AppCommonHeaderVehicleToCenter_TimeZoneOffset *const timeZoneOffset{appCommonHeader->mutable_time_zone_offset()};

        std::shared_ptr<HttpManagerAdapter> const httpAdapter{HttpManagerAdapter::getInstance()};
        android::sp<UploadManager> const uploadData{UploadManager::getInstance()};
        //Header
        appCommonHeader->set_text_version(httpAdapter->getProtoTextVersion());  
        appCommonHeader->set_electronic_pf(EPF_19EPF);
        appCommonHeader->set_geodesy_information(CommonUtils::getGeodesyInfo());
        timeZoneOffset->set_hours(CommonUtils::getTimeZoneOffsetHour());
        timeZoneOffset->set_minutes(CommonUtils::getTimeZoneOffsetMinutes()); 

        commonHeader->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
            //MessageID
        commonHeader->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, uploadData->getCounterMessage()));
        appCommonHeader->set_text_version(httpAdapter->getProtoTextVersion());
        //Content
        mMonitoringUploadErrorData->set_collection_condition_id(mColId);
        mMonitoringUploadErrorData->set_trigger_type(RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER);   //RoB Monitoring operate when center trigger
        mMonitoringUploadErrorData->set_counter_value(uploadData->getCounterValue());
        if (triggerOccurrenceTime > 0)
        {
            mMonitoringUploadErrorData->set_data_creation_date(static_cast<uint64_t>(triggerOccurrenceTime));
        }
        mMonitoringUploadErrorData->set_obd2_installed_flag(mApp.getOBDStatus());
            /*under_repair_flag*/
        mMonitoringUploadErrorData->set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
        mMonitoringUploadErrorData->set_response_code(resCode);
        mMonitoringUploadErrorData->set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_OCCURRENCE_ROB_MONITORING);  
            /*diag_mesages*/
        uint32_t sizeOfFileCounter {0U};
        sizeOfFileCounter = mMonitoringUploadErrorData->ByteSizeLong();
        std::list<std::pair<EcuAddressInformation, DiagnosticsMessage>>::iterator it{};
        for(it = mEcuInformationError.begin(); it != mEcuInformationError.end(); ++it){
            const EcuAddressInformation currentECU{it->first};      
            const DiagnosticsMessage currentMessage{it->second};
            const uint32_t tmpAdditionalSize {static_cast<uint32_t>(currentMessage.ByteSizeLong())};
            uint32_t potentialSize {sizeOfFileCounter};
            if (potentialSize <= (UINT32_MAX - tmpAdditionalSize)) {
                potentialSize += tmpAdditionalSize;
            } else {
                LOG_D("over size !!");
                (void)tmpAdditionalSize;
            }
            RdgProtoInterface::DiagnosticsMessage* const diagMessage {mMonitoringUploadErrorData->add_diag_messages()};
            RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {diagMessage->mutable_ecu_address_information()};
            if (potentialSize > ROB_UPLOAD_DATA_SIZE_MAX)   //4MB  RDG30-R-0929
            {
                LOG_D("Error upload data exceeds 4MB -> discard exceeds data");
                if (currentMessage.ByteSizeLong() > 0U)
                {
                    diagMessage->set_status_code(currentMessage.status_code());
                    const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{currentECU.communication_protocol()};
                    if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                    {
                        vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{currentECU.communication_type()};
                        if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                            (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                        {
                            commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                        }
                        ecuAddressInfo->set_communication_type(commType);
                    }
                    ecuAddressInfo->set_communication_protocol(protocolType);
                    ecuAddressInfo->set_target_address(currentECU.target_address());
                    if (currentMessage.user_data().size() > 0U) {
                        const vccomif::rdg::v1::interfaces::StatusCode tmpStatuscode{diagMessage->status_code()};
                        if((tmpStatuscode < vccomif::rdg::v1::interfaces::StatusCode::SC_UNKNOWN) ||
                        (tmpStatuscode > vccomif::rdg::v1::interfaces::StatusCode::SC_ECU_SPECIFIED_ERROR)){
                            LOG_D("Invalid status code");
                        } else{
                            uint32_t sizeOfStatusCode {0U};
                            sizeOfStatusCode = sizeof(::vccomif::rdg::v1::interfaces::StatusCode);
                            const uint32_t sizeOfEcuAddressInformation {static_cast<uint32_t>(diagMessage->ecu_address_information().ByteSizeLong())};
                            const uint64_t tmpSubtractiveByte {static_cast<uint64_t>(sizeOfFileCounter) + static_cast<uint64_t>(sizeOfStatusCode) + static_cast<uint64_t>(sizeOfEcuAddressInformation)};
                            uint32_t subtractiveByte{0U};
                            if(tmpSubtractiveByte > UINT32_MAX) {
                                LOG_D("Over size of subtractiveByte");
                            } else {
                                subtractiveByte = static_cast<uint32_t>(tmpSubtractiveByte);
                            }
                            if(subtractiveByte <= ROB_UPLOAD_DATA_SIZE_MAX){
                                subtractiveByte = ROB_UPLOAD_DATA_SIZE_MAX - subtractiveByte;
                            } else {
                                LOG_D("Over ROB_UPLOAD_DATA_SIZE_MAX value");
                                subtractiveByte = 0U;
                            }
                            const uint32_t remainingBytes {subtractiveByte};
                            diagMessage->set_user_data(&currentMessage.user_data()[0], remainingBytes < currentMessage.user_data().size() ? remainingBytes : currentMessage.user_data().size());
                            LOG_D("EXCEEDED_SIZE");
                        }
                    }
                }
                (void) diagMessage;
                (void) ecuAddressInfo;
                (void)sizeOfFileCounter;
                break;
            } else {
                diagMessage->set_status_code(currentMessage.status_code());
                const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{currentECU.communication_protocol()};
                if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                {
                    vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{currentECU.communication_type()};
                    if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                        (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                    {
                        commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                    }
                    ecuAddressInfo->set_communication_type(commType);
                }
                ecuAddressInfo->set_communication_protocol(protocolType);
                ecuAddressInfo->set_target_address(currentECU.target_address());
                diagMessage->set_user_data(currentMessage.user_data());
                const uint32_t additionalSize {static_cast<uint32_t>(diagMessage->ByteSizeLong())};
                if (sizeOfFileCounter <= (UINT32_MAX - additionalSize)) {
                    sizeOfFileCounter += additionalSize;
                } else {
                    LOG_D("over size!!!");
                    (void)additionalSize;
                }
                LOG_I("Monitoring upload size (%d)", sizeOfFileCounter);
            }
        }
        LOG_D("sizeOfFileCounter value (%d)", sizeOfFileCounter);
        //For Testing
        std::string str_Monitoring_Error_upload{""};
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        (void)google::protobuf::util::MessageToJsonString(*mMonitoringUploadErrorData, &str_Monitoring_Error_upload, option);
        LOG_D("Error upload data: ");
        printData(str_Monitoring_Error_upload);
        const uint32_t uploadId {uploadData->genRequestId()};
        const uint64_t uploadCount {uploadData->genCountUpload()};
        std::string file_dir {std::to_string(uploadCount)};
        (void)file_dir.append("_UploadErrorMonitoringData.dat");
        uint32_t fileSize{0U};
        error_t bStored{E_OK};
        const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
        if (region == LGE_REGION::LGE_REGION_CN)
        {
            bStored = DataModel<UploadErrorDataRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG160, file_dir, *mMonitoringUploadErrorData, fileSize);
        }
        else
        {
            fileSize = mMonitoringUploadErrorData->ByteSizeLong();
            bStored = DataModel<UploadErrorDataRequest>::saveUpload(file_dir, *mMonitoringUploadErrorData);
        }
        if (bStored == E_OK)
        {
            const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
            // Self-Diag
            DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
            const android::sp<UploadTask> task {new UploadTask(uploadId)};
            task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
            task->setUploadPatch(file_dir);   //TODO::wait upload module complete
            task->setUploadId(static_cast<uint64_t>(uploadId));
            /*Set priority*/       
            task->setUploadPrio(mPriority);
            // test_saveUploadData = task;
            task->setFileSize(static_cast<uint64_t>(fileSize));
            uploadData->requestUploadTask(task);  //TODO
        }
        else
        {
            LOG_E("Failed to store");
        }
            //Notify finish to schedule 
        if(mEcuInformationError.size() > 0U){    //RDG30-R-0207  
            const int64_t current_time{CommonUtils::getCurrentAcquisiteTime()};
            LOG_I("Collection ID: %llu - Last complete time: %lld - trigger type: %d", mColId, current_time, mTriggerType);
            if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
            {
                mApp.notifyLastOpComplTime(mColId, current_time, false);
            }
        } else{
            const int64_t current_time{CommonUtils::getCurrentAcquisiteTime()};
            LOG_I("Collection ID: %llu - Last complete time: %lld - trigger type: %d", mColId, current_time, mTriggerType);
            if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
            {
                mApp.notifyLastOpComplTime(mColId, current_time, true);
            }
        }

        mEcuInformationError.clear(); //Clear list error ecu
        mEcuInformationRes.clear();   //Clear list response ecu
        (void) uploadId;
        (void) fileSize;
        LOG_I("End upload Error data");
    }
}

void RoBMonitoring::makeUploadResponseData(){       //RDG30-R-0551
    LOG_I("makeUploadResponseData");
    if(mEcuInformationRes.size() > 0U){
        if(mPriorityId < 0){
            LOG_D("invalid mPriorityId value");
        } else {
            const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator itr{mSaveReq.find(static_cast<uint32_t>(mPriorityId))};
            int64_t triggerOccurrenceTime{0};
            if (itr != mSaveReq.end())
            {
                triggerOccurrenceTime = itr->second->getTriggerTime();
            }
            else 
            {
	            LOG_D("Can not find triggerId: %d in mSaveReq", mPriorityId);
            }

            mMonitoringUploadResponseData = std::shared_ptr<UploadRequestResponseNotificationRequest>(new UploadRequestResponseNotificationRequest());  
                    //initialize
            vccomif::rdg::v1::interfaces::RdgCommonRequestHeader *const commonHeader{mMonitoringUploadResponseData->mutable_rdg_common_request_header()};
            vccomif::common::v1::AppCommonHeaderVehicleToCenter *const appCommonHeader{commonHeader->mutable_app_common_header()};
            vccomif::common::v1::AppCommonHeaderVehicleToCenter_TimeZoneOffset *const timeZoneOffset{appCommonHeader->mutable_time_zone_offset()};

            android::sp<UploadManager> const uploadData{UploadManager::getInstance()};
                //Header
            appCommonHeader->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
            appCommonHeader->set_electronic_pf(EPF_19EPF);
            appCommonHeader->set_geodesy_information(CommonUtils::getGeodesyInfo());
            timeZoneOffset->set_hours(CommonUtils::getTimeZoneOffsetHour());
            timeZoneOffset->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());

            commonHeader->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_REQUEST_RESPONSE_NOTIFICATION);
            //MessageID
            commonHeader->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_REQUEST_RESPONSE_NOTIFICATION, uploadData->getCounterMessage()));

                //Content
            mMonitoringUploadResponseData->set_collection_condition_id(mColId);
            mMonitoringUploadResponseData->set_counter_value(uploadData->getCounterValue());
            if (triggerOccurrenceTime > 0)
            {
                mMonitoringUploadResponseData->set_data_creation_date(static_cast<uint64_t>(triggerOccurrenceTime));
            }
            mMonitoringUploadResponseData->set_obd2_installed_flag(mApp.getOBDStatus());
                /*under_repair_flag*/
            mMonitoringUploadResponseData->set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);

            //check 4 Mb  RDG30-R-1042 + RDG30-R-1171 => TODO: need to valid in case exceed 4Mb
            uint32_t sizeOfFileCounter {0U};
            sizeOfFileCounter = mMonitoringUploadResponseData->ByteSizeLong();
            std::list<std::pair<EcuAddressInformation, DiagnosticsMessage>>::iterator it{};
            for(it = mEcuInformationRes.begin(); it != mEcuInformationRes.end(); ++it){
                const EcuAddressInformation currentECU{it->first};
                const DiagnosticsMessage currentMessage{it->second};
                RdgProtoInterface::DiagnosticsMessage* const diagMessage {mMonitoringUploadResponseData->add_ecu_response_messages()};
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {diagMessage->mutable_ecu_address_information()};
                const uint32_t tmpAdditionalSize {static_cast<uint32_t>(currentMessage.ByteSizeLong())};
                uint32_t potentialSize {sizeOfFileCounter};
                if (potentialSize <= (UINT32_MAX - tmpAdditionalSize)) {
                    potentialSize += tmpAdditionalSize;
                } else {
                    LOG_D("over size !");
                    (void)tmpAdditionalSize;
                }

                if (potentialSize > ROB_UPLOAD_DATA_SIZE_MAX)   //4MB  RDG30-R-0929
                {
                    LOG_D("Last Response upload data exceeds 4MB -> discard exceeds data");
                    if (currentMessage.ByteSizeLong() > 0U)
                    {
                        diagMessage->set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL_WITH_EXCEEDED_SIZE);
                        const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{currentECU.communication_protocol()};
                        if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                        {
                            vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{currentECU.communication_type()};
                            if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                                (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                            {
                                commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                            }
                            ecuAddressInfo->set_communication_type(commType);
                        }
                        ecuAddressInfo->set_communication_protocol(protocolType);
                        ecuAddressInfo->set_target_address(currentECU.target_address());

                        if (currentMessage.user_data().size() > 0U) {
                            // const vccomif::rdg::v1::interfaces::StatusCode tmpStatuscode{vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL_WITH_EXCEEDED_SIZE};
                            uint32_t sizeOfStatusCode {0U};
                            sizeOfStatusCode = sizeof(vccomif::rdg::v1::interfaces::StatusCode);
                            const uint32_t sizeOfEcuAddressInformation {static_cast<uint32_t>(diagMessage->ecu_address_information().ByteSizeLong())};
                            const uint64_t tmpSubtractiveByte {static_cast<uint64_t>(sizeOfFileCounter) + static_cast<uint64_t>(sizeOfStatusCode) + static_cast<uint64_t>(sizeOfEcuAddressInformation)};
                            uint32_t subtractiveByte{0U};
                            if(tmpSubtractiveByte > UINT32_MAX) {
                                LOG_D("Over size of subtractiveByte");
                            } else {
                                subtractiveByte = static_cast<uint32_t>(tmpSubtractiveByte);
                            }
                            if(subtractiveByte <= ROB_UPLOAD_DATA_SIZE_MAX){
                                subtractiveByte = ROB_UPLOAD_DATA_SIZE_MAX - subtractiveByte;
                            } else {
                                LOG_D("Over ROB_UPLOAD_DATA_SIZE_MAX value");
                                subtractiveByte = 0U;
                            }
                            const uint32_t remainingBytes {subtractiveByte};
                            diagMessage->set_user_data(&currentMessage.user_data()[0], remainingBytes < currentMessage.user_data().size() ? remainingBytes : currentMessage.user_data().size());
                            LOG_D("EXCEEDED_SIZE - SC_SUCCESSFUL_WITH_EXCEEDED_SIZE");
                        }
                    }
                    (void) diagMessage;
                    (void) ecuAddressInfo;
                    (void) sizeOfFileCounter;
                    break;
                } else {
                    diagMessage->set_status_code(currentMessage.status_code());
                    const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{currentECU.communication_protocol()};
                    if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                    {
                        vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{currentECU.communication_type()};
                        if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                            (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                        {
                            commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                        }
                        ecuAddressInfo->set_communication_type(commType);
                    }
                    ecuAddressInfo->set_communication_protocol(protocolType);
                    ecuAddressInfo->set_target_address(currentECU.target_address());
                    diagMessage->set_user_data(currentMessage.user_data());
                    const uint32_t additionalSize {static_cast<uint32_t>(diagMessage->ByteSizeLong())};
                    if (sizeOfFileCounter <= (UINT32_MAX - additionalSize)) {
                        sizeOfFileCounter += additionalSize;
                    } else {
                        LOG_D("over size!!!");
                        (void)additionalSize;
                    }
                    LOG_V("Monitoring upload size (%d)", sizeOfFileCounter);
                }
            }
            LOG_I("sizeOfFileCounter value (%d)", sizeOfFileCounter);
            //For Testing
            std::string str_Monitoring_upload{""};
            google::protobuf::util::JsonOptions option{};
            option.always_print_primitive_fields = true;
            option.preserve_proto_field_names = true;
            (void)google::protobuf::util::MessageToJsonString(*mMonitoringUploadResponseData, &str_Monitoring_upload, option);
            LOG_D("Monitoring upload data: ");
            printData(str_Monitoring_upload);

            const uint32_t uploadId {uploadData->genRequestId()};
            const uint64_t uploadCount {uploadData->genCountUpload()};
            std::string file_dir {std::to_string(uploadCount)};
            (void)file_dir.append("_LastUploadResponseMonitoringDataRequest.dat");
            uint32_t fileSize{0U};
            error_t bStored{E_OK};
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if (region == LGE_REGION::LGE_REGION_CN)
            {
                bStored = DataModel<UploadRequestResponseNotificationRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG130, file_dir, *mMonitoringUploadResponseData, fileSize);
            }
            else
            {
                fileSize = mMonitoringUploadResponseData->ByteSizeLong();
                bStored = DataModel<UploadRequestResponseNotificationRequest>::saveUpload(file_dir, *mMonitoringUploadResponseData);
            }
            if (bStored == E_OK)
            {
                const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
                // Self-Diag
                DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
                const android::sp<UploadTask> task {new UploadTask(uploadId)};
                task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG130);
                task->setUploadPatch(file_dir);   
                task->setUploadId(static_cast<uint64_t>(uploadId));
                /*Set priority*/
                task->setUploadPrio(mPriority);
                // test_saveUploadData = task;
                task->setFileSize(static_cast<uint64_t>(fileSize));
                uploadData->requestUploadTask(task);
            }
            else
            {
                LOG_E("Failed to store");
            }

            mEcuInformationError.clear();   //Clear list error ecu
            mEcuInformationRes.clear();    //Clear list response ecu
            LOG_I("Finish is makeUploadResponse");

                //Notify finish to schedule
            const int64_t current_time{CommonUtils::getCurrentAcquisiteTime()};
            LOG_I("Collection ID: %llu - Last complete time: %lld - trigger type: %d", mColId, current_time, mTriggerType);
            if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
            {
                mApp.notifyLastOpComplTime(mColId, current_time, true);
            }
            (void) fileSize;
            (void) uploadId;
        }
    } else {
        LOG_I("mMonitoringUploadResponseData is empty");
    }
    LOG_E("End makeUploadResponseData");
}

void RoBMonitoring::convertEcuInformation(const CommonDefine::EcuInformation ecuInformation, EcuAddressInformation &ecu, const uint32_t rxAdd) const {
    if(rxAdd == 0U) {
        ecu.set_target_address(ecuInformation.getTargetAddress());
    } else {
        ecu.set_target_address(rxAdd);
    }
    ecu.set_communication_protocol(ecuInformation.getCommProtocol());
    ecu.set_communication_type(ecuInformation.getCommType());
}

//FUNCTION FOR MONITORINGTRANMISSION
void RoBMonitoring::MonitoringTransmission::connect()
{
    LOG_I("Start connect, transmissionID = 0x%llx", this->transmissionId());
    const android::sp<OBCTransportInfo> mtransportInfo{new OBCTransportInfo()};
    const android::sp<OBCConnectInfo> mConnectInfo{new OBCConnectInfo()};
    OBCCanInfo mcanInfo{};
    std::vector<std::string> ntaArray{};
    constexpr const bool mperiodicRes{false};
    constexpr const uint16_t mudsResTimeout{0U};
    const uint8_t protocolType{RemoteEcuInformation::getInstance()->convertObcProtocolType(this->ecuInformation().getCommProtocol(), this->ecuInformation().getCommType(), this->ecuInformation().getTargetAddress())};
    ntaArray.clear();
    if ((protocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
        (protocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
        (protocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
    {
        const uint16_t nTa{static_cast<uint16_t>(((this->ecuInformation().getTargetAddress() >> 8U) & 0xFFU))};
        std::stringstream ss{};
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << nTa;
        const std::string hexString{ss.str()}; // Convert to string
        for (size_t i{0U}; i < hexString.size(); i++)
        {
            const std::string tmp{std::string(1U, hexString[i])};
            ntaArray.push_back(tmp);
        }
    }
    mcanInfo.setData(this->ecuInformation().getTargetAddress(), ntaArray);
    mtransportInfo->setData(protocolType
                                , mcanInfo
                                , mperiodicRes
                                , mudsResTimeout);
    const std::string applicationName{"remotediag"};
    android::sp<OnboardclientAdapter> const OBCAdapter{OnboardclientAdapter::getInstance()};
    (void)OBCAdapter->connect(mtransportInfo, applicationName, mConnectInfo);
    if (mConnectInfo->getResponse() == OBCEnum::OBCErrCode::OBC_OK)
    {
        this->connectId() = mConnectInfo->getConnectId();
        mMonitoring.mCurrentConnectId = mConnectInfo->getConnectId();
        this->setStateData(MonitoringTransmission::State::MONITORING_TRANS_CONNECT);
        OBCAdapter->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
        android::sp<::Buffer> udsData {new ::Buffer()};
        MonitoringTransmission::State nState;
        if(this->isContinueTransmission(nState, udsData)) {
            this->sendUds(nState, udsData);
        } else {
            LOG_D("INVALID");
        }
    }
    else
    {
        LOG_E("Can't connect to target adress = %02X", this->ecuInformation().getTargetAddress());
        // TMCDCMTF-35635
        // OBCAdapter->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
        this->setStateData(MonitoringTransmission::State::MONITORING_TRANS_DONE);
        (void)mMonitoring.mHandler->obtainMessage(MainHandler::CMD_ROBMONITORING_FINISH_TRANSMISSION)->sendToTarget();
    }
}

void RoBMonitoring::MonitoringTransmission::stopTimeout()
{
    this->mTimeOut.stop();
}

void RoBMonitoring::MonitoringTransmission::disconnect()
{
    const uint16_t connectId {this->connectId()};
    android::sp<OnboardclientAdapter> const OBCAdapter{OnboardclientAdapter::getInstance()};
    LOG_I("MonitoringTransmission::disconnect for connectID (%u) ", connectId);
    (void)OBCAdapter->disconnectECU(this->connectId());
    // TMCDCMTF-35635
    // OBCAdapter->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
    this->setStateData(MonitoringTransmission::State::MONITORING_TRANS_DONE);
    mMonitoring.finishCurrentTransmission();
}

void RoBMonitoring::MonitoringTransmission::sendUds(const MonitoringTransmission::State nState, const android::sp<::Buffer> uds)
{
    LOG_I("MonitoringTransmission::sendUds() - state(%s)", enumToString(static_cast<uint32_t>(nState)).c_str());
    //Print send UDS data
    this->mUdsReq = uds;
    const uint8_t res{OnboardclientAdapter::getInstance()->sendUdsData(this->connectId(), uds)};
    if (res != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK))
    {
        // TODO Handle exeptional case
        mMonitoring.handleUnableSendUDS(nState);
        this->disconnect();
    }
    else
    {
        this->setStateData(nState);
        LOG_I("SendUdsData success - current state: %s", enumToString(static_cast<uint32_t>(nState)).c_str());
        // std::stringstream ss{};
        // ss << &std::hex << std::setfill('0');
        // if (uds->data() != nullptr)
        // {
        //     for (uint32_t i {0U}; i < uds->size(); i++)
        //     {
        //         ss << " " << std::setw(2) << uds->data()[i];
        //     }
        // }
        // LOG_I("[sendUDSData] uds data = %s", ss.str().c_str());
        mTimeOut.start();
        LOG_I("Start timeout timer for mTransmissionId 0x%llx", this->transmissionId());
    }
}

bool RoBMonitoring::MonitoringTransmission::isContinueTransmission(MonitoringTransmission::State &nState, android::sp<::Buffer> &uds){
    bool isContinue {false};
    if(this->getTypeData() == MonitoringTransmission::Type::MONITORING_STOP){
        if(this->getStateData() == MonitoringTransmission::State::MONITORING_TRANS_CONNECT){
            uds = this->mUdsReqStop.ToUdsData();
            nState = MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_STOP;
            isContinue = true;
        }
    }
    if(this->getTypeData() == MonitoringTransmission::Type::MONITORING_CONFIGURE){
        if(this->getStateData() == MonitoringTransmission::State::MONITORING_TRANS_CONNECT){
            uds = this->mUdsReqSetting.ToUdsData();
            nState = MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE;
            isContinue = true;
        }
        if(this->getStateData() == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE){
            uds = this->mUdsReqStart.ToUdsData();
            nState = MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START;
            isContinue = true;
        }
    }
    if(this->getTypeData() == MonitoringTransmission::Type::MONITORING_RECONFIGURE){
        if(this->getStateData() == MonitoringTransmission::State::MONITORING_TRANS_CONNECT){
            uds = this->mUdsReqStop.ToUdsData();
            nState = MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_STOP;
            isContinue = true;
        }
        if(this->getStateData() == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_STOP){
            uds = this->mUdsReqSetting.ToUdsData();
            nState = MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE;
            isContinue = true;
        }
        if(this->getStateData() == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE){
            uds = this->mUdsReqStart.ToUdsData();
            nState = MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START;
            isContinue = true;
        }
    }

    if(isContinue == false){
        nState = MonitoringTransmission::State::MONITORING_TRANS_DISCONNECT;
    }
    LOG_I("isContinueTransmission (%d)", isContinue);
    return isContinue;
}

std::string RoBMonitoring::MonitoringTransmission::enumToString(const uint32_t input) const noexcept {
    std::string output{};
    switch(input) {
        case static_cast<uint32_t>(State::MONITORING_TRANS_CONNECT):
            {
                output = std::string("MONITORING_TRANS_CONNECT");
                break;
            } 
        case static_cast<uint32_t>(State::MONITORING_TRANS_SEND_UDS_START):
            {
                output = std::string("MONITORING_TRANS_SEND_UDS_START");
                break;
            } 
        case static_cast<uint32_t>(State::MONITORING_TRANS_SEND_UDS_CONFIGURE):
            {
                output = std::string("MONITORING_TRANS_SEND_UDS_CONFIGURE");
                break;
            }
        case static_cast<uint32_t>(State::MONITORING_TRANS_SEND_UDS_STOP):
            {
                output = std::string("MONITORING_TRANS_SEND_UDS_STOP");
                break;
            } 
        case static_cast<uint32_t>(State::MONITORING_TRANS_DISCONNECT):
            {
                output = std::string("MONITORING_TRANS_DISCONNECT");
                break;
            } 
        case static_cast<uint32_t>(State::MONITORING_TRANS_DONE):
            {
                output = std::string("MONITORING_TRANS_DONE");
                break;
            } 
        case static_cast<uint32_t>(State::MONITORING_TRANS_INIT):
            {
                output = std::string("MONITORING_TRANS_INIT");
                break;
            } 
        case static_cast<uint32_t>(Type::MONITORING_INIT):
            {
                output = std::string("MONITORING_INIT");
                break;
            } 
        case static_cast<uint32_t>(Type::MONITORING_STOP):
            {
                output = std::string("MONITORING_STOP");
                break;
            } 
        case static_cast<uint32_t>(Type::MONITORING_CONFIGURE):
            {
                output = std::string("MONITORING_CONFIGURE");
                break;
            } 
        case static_cast<uint32_t>(Type::MONITORING_RECONFIGURE):
            {
                output = std::string("MONITORING_RECONFIGURE");
                break;
            } 
        default:
                output = std::string("INVALID VALUE");
                break;

    }
    return output;
}

android::sp<Buffer> RoBMonitoring::MonitoringTransmission::getUdsMessageReqData(const MonitoringTransmission::State nState) 
{
    android::sp<Buffer> udsReqMessageBuffer {};
    switch(nState){
        case MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START:
        {
            if(this->mUdsReqStart.ToUdsData() != nullptr){
                udsReqMessageBuffer = this->mUdsReqStart.ToUdsData();
            } else {
                LOG_E("ReqStart Uds Data is nullprt");
            }
            break;
        }
        case MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE:
        {
            if(this->mUdsReqSetting.ToUdsData() != nullptr){
                udsReqMessageBuffer = this->mUdsReqSetting.ToUdsData();
            } else {
                LOG_E("ReqStart Uds Data is nullprt");
            }
            break;
        }
        case MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_STOP:
        {
            if(this->mUdsReqStop.ToUdsData() != nullptr){
                udsReqMessageBuffer = this->mUdsReqStop.ToUdsData();
            } else {
                LOG_E("ReqStop Uds Data is nullprt");
            }
            break;
        }
        default: 
        {
            LOG_I("Invalid Input");
            break;
        }
    }
    return udsReqMessageBuffer;
}

error_t RoBMonitoring::getRobMonitoringList(TargetCollectionDataOccurrentRobList& aList, Uint64& collectionConditionId)
{
    const Mutex::Autolock mLock{mMutexMonitoring};
    error_t result{E_OK};
    const std::shared_ptr<CollectionConditionRobRobSsrDidEvent>  orgRoBMonitoringList{getRoBInformationList()};
    if ((orgRoBMonitoringList == nullptr) || (orgRoBMonitoringList->target_collection_data().size() <= 0)) 
    {
        result = E_BUFFER_EMPTY;
    }
    else {
        collectionConditionId = orgRoBMonitoringList->collection_condition_id();
        aList.CopyFrom(orgRoBMonitoringList->target_collection_data());
        LOG_D("current collection_condition_id: %llu", collectionConditionId); 
    }
    return result;
}

void RoBMonitoring::updateRoBInformationList(const EcuAddressInformation& ecu)
{
    if(!mMonitoringRequestData){
        LOG_I("mMonitoringRequestData is NULL");
    } else {
        LOG_I("updateRoBInformationList ");
        int32_t targetSize;
        targetSize = mMonitoringRequestData->target_collection_data_size();
        if(mMonitoringList != nullptr) {
            for(int32_t idx {0}; idx < targetSize; idx ++){
                if(ecu.target_address() == mMonitoringRequestData->target_collection_data(idx).ecu_address_information().target_address()){
                    const TargetCollectionDataOccurrentRob tmpTargetCollectionData {mMonitoringRequestData->target_collection_data(idx)};
                    mMonitoringList->mutable_target_collection_data()->Add()->CopyFrom(tmpTargetCollectionData);
                }
            }
        } else {
            LOG_E("mMonitoringList is NULL");
        }
    }
}

void RoBMonitoring::removeStopECU(const EcuAddressInformation ecu)
{
    LOG_I("Removing ECU (0x%02x) from target collection data", ecu.target_address());
    LOG_D("Monitoring list size: %d", mMonitoringUploadData->target_collection_data_size());
    const std::shared_ptr<CollectionConditionRobRobSsrDidEvent> tempMonitoringUploadData {std::make_shared<CollectionConditionRobRobSsrDidEvent>()};

    TargetCollectionDataOccurrentRobList::const_iterator it {mMonitoringUploadData->target_collection_data().cbegin()};
    while(it != mMonitoringUploadData->target_collection_data().cend())
    {
        if(it->ecu_address_information().target_address() == ecu.target_address())
        { 
            LOG_I("Found the stop ECU");
        } else {
            tempMonitoringUploadData->mutable_target_collection_data()->Add()->CopyFrom(*it);
        }
        ++it;
    }
    mMonitoringUploadData.reset(); 
    mMonitoringUploadData = std::shared_ptr<CollectionConditionRobRobSsrDidEvent>();
    mMonitoringUploadData = tempMonitoringUploadData;

    LOG_D("Monitoring list size after remove StopECU: %u", mMonitoringUploadData->target_collection_data().size());
    LOG_I("ECU (0x%02x) removed from target collection data", ecu.target_address());
}

void RoBMonitoring::generateRoBInformationList()
{
    LOG_I("generateRoBInformationList");
    mMonitoringUploadData->set_collection_condition_id(mMonitoringRequestData->collection_condition_id());
    mMonitoringUploadData->mutable_target_collection_data()->MergeFrom(mMonitoringList->target_collection_data());
}

char_t RoBMonitoring::uint8ToChar(const uint8_t num) const noexcept {

    uint8_t tmp{0U};
    if(num <= 9U)
    {
        tmp = num + 48U;
    }
    else{
        if(num + 55U <= static_cast<uint8_t>(UINT8_MAX))
        {
            tmp = num + 55U;
        }
    }
    char_t res{'0'};
    if(tmp <= static_cast<uint8_t>(INT8_MAX))
    {
        res = static_cast<char_t>(tmp);
    }
    return res;
}

void RoBMonitoring::handleUdsResponse(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) 
{
    //Un-response 0x7F, SID #86, 78
    //Negative 0x7F, SID # 86, other than 78
    //Negative 0x7F, SID other than 86 , ... => send unresponse
    //Positive but SID #C6 => send unresponse
    //Monitoring Stop Request：86 06 02	                    Response : C6 06 00 02
    //Monitoring Setting Request：86 03 02 A0 05 22 A0 06	Response : C6 03 00 02 A0 05 22 A0 06
    //Monitoring Start Request：86 45 02	                Response : C6 45 00 02

    //INIT = 0
    //OKE = 1
    //NEGATIVE(NRC) = 2
    //UNRESPONSIVE = 3
    const uint8_t responseType {determineResponseType(responseEventInfo, udsResponse)};
    LOG_W("responseType %u", responseType);
    // Print Log
    const android::sp<::Buffer> tmpBuf {udsResponse->ToUdsData()};

    logUdsPayload(tmpBuf);

    const TransmissionInter it {mMonitoringTransList.find(mCurrentTransmissionId)};
    if (it == mMonitoringTransList.end()) {
        LOG_E("cannot find mCurrentTransmissionId");
    } else {
        android::sp<MonitoringTransmission> const receiveTransmission{it->second};
        const uint16_t connectId {responseEventInfo->resInfo()->connectId()};
        if (mCurrentConnectId != connectId) {
            LOG_D("Monitoring is NOT running OR NOT match CurrentConnectID. Process receive UDS");
        } else {
            LOG_D("Monitoring is running and match connectID. Process receive UDS");
            receiveTransmission->stopTimeout();
            if (receiveTransmission->getStateData() == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_STOP) {
                handleStopUdsResponse(it, responseType);
            } else {
                handleOtherUdsResponse(it, responseType, responseEventInfo, udsResponse);
            }
        }
    }
} 

uint8_t RoBMonitoring::determineResponseType(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) const noexcept {
    //DCM24SPEC-17084
    Type responseType {Type::INIT};
    if (responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_OK) {
        if(udsResponse->getSID() == 0xC6U) {    //compare SID
            responseType = Type::POSITIVE_RESPONSE; //positive 
        } else {
            responseType = Type::UN_RESPONSE; // Un-response
        }
    } else if (responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_NEGATIVE) {  //receiving response 7F
        if(udsResponse->getSFID() == 0x86U) {     //compare SID
            if (udsResponse->getNRC() != 0x78U) {   
                responseType = Type::NEGATIVE_RESPONSE; // Negative response
            } else {
                responseType = Type::UN_RESPONSE; // Un-response
            }
        } else {
            responseType = Type::UN_RESPONSE; // Un-response
        }
    } else {
        LOG_E("CAN NOT detect response type");
        responseType = Type::UN_RESPONSE;  //Note: Handle Error case
    }
    return static_cast<uint8_t>(responseType);
}

void RoBMonitoring::logUdsPayload(const android::sp<Buffer> tmpBuf) {
    if(tmpBuf == nullptr){
        LOG_E("UDS payload is nullptr");
    } else {
        if (tmpBuf->data() == nullptr) {
            LOG_E("UDS payload data is nullptr");
        } else {
            constexpr size_t BUFFER_SIZE {4096U}; // Limit buffer size
            std::string log {""};
            for (uint32_t i {0U}; i < tmpBuf->size(); i++)
            {
                if (log.size() >= (BUFFER_SIZE - 4U)) { // Note: avoid potential buffer overflow
                    LOG_I("Log size limit reached, truncating further appends.");
                    break;
                }

                const uint8_t* const dataPtr {tmpBuf->data()}; // Store pointer to data
                if(dataPtr != nullptr){
                    const uint8_t value {dataPtr[i]};
                    const uint8_t low {static_cast<uint8_t>(value & 0x0FU)};
                    const uint8_t high {static_cast<uint8_t>(value >> 4U)};
                    (void)log.append(1U, uint8ToChar(high));
                    (void)log.append(1U, uint8ToChar(low));
                    (void)log.append(" ");
                } else {
                    LOG_E("UDS payload data is nullptr");
                }
            }
            LOG_I("UDS payload = %s", log.c_str());
            log.clear();
            (void) BUFFER_SIZE;
        }
    }
}

void RoBMonitoring::handleStopUdsResponse(const TransmissionInter& it, const uint8_t responseType) {
    //Don't upload response data 
    LOG_I("Don't package response data of stop UDS"); // RDG30-R-1199
    if (responseType == static_cast<uint8_t>(Type::POSITIVE_RESPONSE)) {
        EcuAddressInformation ecuRemoveInfo{};
        android::sp<MonitoringTransmission> const curTransmission{it->second};
        convertEcuInformation(curTransmission->ecuInformation(), ecuRemoveInfo);
        removeStopECU(ecuRemoveInfo);
        const MonitoringTransmission::Type curTransmissionType{curTransmission->getTypeData()};
        if (curTransmissionType == MonitoringTransmission::Type::MONITORING_STOP) {
            curTransmission->disconnect();
        } else if (curTransmissionType == MonitoringTransmission::Type::MONITORING_RECONFIGURE) {
            sendNextTransmission(it);
        } else {
            LOG_E("undefine monitoring type");  //Note: Handle error code
        }
    } else { // Negative + Unresponse 
        handleNegativeUNResponse(it);
    } 
}

void RoBMonitoring::handleNegativeUNResponse(const TransmissionInter& it) {
    //RDG30-R-0558 
    //Don't remove Stop ECU if receiving negative response, disconnect and connect to next ecu    
    //RDG30-R-0552 
    //Response Code: "ECU response failure when setting RoB monitoring"
    //Status code: Negative response + User_data: Negative response message from the ECU.
    android::sp<MonitoringTransmission> const curTransmission{it->second};
    const MonitoringTransmission::Type curTransmissionType{curTransmission->getTypeData()};
    LOG_I("Response result for stop UDS is NEGATIVE/ UNRESPONSE");
    if (curTransmissionType == MonitoringTransmission::Type::MONITORING_STOP) {
        mIsSendLastUploadData = false;
        curTransmission->setStateData(MonitoringTransmission::State::MONITORING_TRANS_DISCONNECT);
        curTransmission->disconnect();
    } else if (curTransmissionType == MonitoringTransmission::Type::MONITORING_RECONFIGURE) {
        mIsSendLastUploadData = false;
        //https://toyota-11f.rickcloud.jp/jira/browse/DCM24MON-7249
        //Don't package setting and start UDS when stop usd is failed
        // EcuAddressInformation ecuInfo{};
        // convertEcuInformation(it->second->ecuInformation(), ecuInfo);
        // DiagnosticsMessage diagMess{};
        // diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_UNRESPONSIVE); // RDG30-R-0552

        // packageUdsData(it->second->getUdsMessageReqData(MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE), diagMess);
        // mEcuInformationError.emplace_back(ecuInfo, diagMess);

        // packageUdsData(it->second->getUdsMessageReqData(MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START), diagMess);
        // mEcuInformationError.emplace_back(ecuInfo, diagMess);

        curTransmission->setStateData(MonitoringTransmission::State::MONITORING_TRANS_DISCONNECT);
        curTransmission->disconnect();
    } else {
        LOG_E("INVALID tranmission Type");  //Note: Error code
    }
}

void RoBMonitoring::packageUdsData(const android::sp<Buffer> iUdsMessage, DiagnosticsMessage& diagMess) const noexcept {
    const android::sp<Buffer> udsData {iUdsMessage};
    if (udsData->data() != nullptr) {
        diagMess.set_user_data(udsData->data(), udsData->size());
    } else {
        LOG_E("UDS data is empty");  //Note: Error code
    }
}

void RoBMonitoring::handleOtherUdsResponse(const TransmissionInter& it, const uint8_t responseType, const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) {
    const uint32_t centerRxAdd {responseEventInfo->getResInfo()->getCanInfo()->getCanId()};
    EcuAddressInformation ecuInfo{};
    DiagnosticsMessage diagMess{};
    if (responseType == static_cast<uint8_t>(Type::NEGATIVE_RESPONSE)) {
        handleNrcResponse(it, udsResponse, centerRxAdd, ecuInfo, diagMess);
    } else if (responseType == static_cast<uint8_t>(Type::UN_RESPONSE)) {
        handleUnresponsiveResponse(it, ecuInfo, diagMess);
    } else {
        handlePositiveResponse(it, udsResponse, centerRxAdd, ecuInfo, diagMess);
    } //Note: Don't need handle Error code
    (void) centerRxAdd;
}

void RoBMonitoring::handleNrcResponse(const TransmissionInter& it, const android::sp<UdsMessage> udsResponse, const uint32_t centerRxAdd, EcuAddressInformation& ecuInfo, DiagnosticsMessage& diagMess) {
    //RDG30-R-0552 
    //Response Code: "ECU response failure when setting RoB monitoring"
    //Status code: Negative response + User_data: Negative response message from the ECU.
    LOG_D("Receive NRC response");
    mIsSendLastUploadData = false;
    convertEcuInformation(it->second->ecuInformation(), ecuInfo, centerRxAdd);
    diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL_WITH_NEGATIVE); // RDG30-R-0552
    if (udsResponse->ToUdsData()->data() != nullptr) {
        diagMess.set_user_data(udsResponse->ToUdsData()->data(), udsResponse->ToUdsData()->size());
    } else {
        LOG_E("Response data is NULL");  //Note: Error code
    }
    mEcuInformationError.emplace_back(ecuInfo, diagMess);
    sendNextTransmission(it);
}

void RoBMonitoring::handleUnresponsiveResponse(const TransmissionInter& it, EcuAddressInformation& ecuInfo, DiagnosticsMessage& diagMess) {
    //RDG30-R-0552 
    //Response Code: "ECU response failure when setting RoB monitoring"
    //Status code: Unresponsive + User_data: request message
    LOG_D("Receive UN response");
    mIsSendLastUploadData = false;
    android::sp<MonitoringTransmission> const curTransmission{it->second};
    convertEcuInformation(curTransmission->ecuInformation(), ecuInfo);
    diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_UNRESPONSIVE); // RDG30-R-0552
    const MonitoringTransmission::State currentState {curTransmission->getStateData()};
    if (currentState == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE) {
        packageUdsData(curTransmission->getUdsMessageReqData(MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE), diagMess);
    } else if (currentState == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START) {
        packageUdsData(curTransmission->getUdsMessageReqData(MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START), diagMess);
    } else {
        LOG_E("UN matching State");  //Note: Error code
    }
    mEcuInformationError.emplace_back(ecuInfo, diagMess);
    sendNextTransmission(it);
}

void RoBMonitoring::handlePositiveResponse(const TransmissionInter& it, const android::sp<UdsMessage> udsResponse, const uint32_t centerRxAdd, EcuAddressInformation& ecuInfo, DiagnosticsMessage& diagMess) {
    /* Save to Response List */
    //RDG30-R-1193
    LOG_D("Receive positive response of UDS");
    android::sp<MonitoringTransmission> const curTransmission{it->second};
    const MonitoringTransmission::State stateData {curTransmission->getStateData()};
    if (stateData <= MonitoringTransmission::State::MONITORING_TRANS_MAX) {
        LOG_D("Current State (%s)", curTransmission->enumToString(static_cast<uint32_t>(stateData)).c_str());
    } else {
        LOG_E("Invalid state");  //Note: Error code
    }
    convertEcuInformation(curTransmission->ecuInformation(), ecuInfo, centerRxAdd);
    diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL); // RDG30-R-1132
    if (udsResponse->ToUdsData()->data() != nullptr) {
        diagMess.set_user_data(udsResponse->ToUdsData()->data(), udsResponse->ToUdsData()->size());
    }
    mEcuInformationRes.emplace_back(ecuInfo, diagMess);     //RDG30-R-1193 Duplicate Target ECU : 1 for setting and 1 for start
    const uint8_t resCount {static_cast<uint8_t>(curTransmission->getResSuccessCount())};
    if (resCount < (static_cast<uint8_t>(UINT8_MAX) - 1U)) {
        curTransmission->setResSuccessCount(static_cast<uint8_t>(resCount + 1U));
    } else {
        LOG_E("ResSuccessCount invalid");
    }
    sendNextTransmission(it);
}

void RoBMonitoring::sendNextTransmission(const TransmissionInter& it) {
    LOG_D("send Next transmission");
    android::sp<MonitoringTransmission> const nextTransmission{it->second};
    android::sp<::Buffer> udsDataTemp {};
    MonitoringTransmission::State nState {MonitoringTransmission::State::MONITORING_TRANS_INIT};
    if (nextTransmission->isContinueTransmission(nState, udsDataTemp)) {
        LOG_D("UDS size : %u", udsDataTemp->size());
        nextTransmission->sendUds(nState, udsDataTemp);
    } else {
        LOG_D("Disconnect UDS");
        const uint8_t resCount {nextTransmission->getResSuccessCount()};
        if(resCount == 2U) {  //Number of positive responses receive - 2 is receiving full positive responses
            EcuAddressInformation ecuAddInfo{};
            convertEcuInformation(nextTransmission->ecuInformation(), ecuAddInfo);
            updateRoBInformationList(ecuAddInfo);
        } else {
            LOG_D("Response count: %u", resCount);
        }
        nextTransmission->setStateData(MonitoringTransmission::State::MONITORING_TRANS_DISCONNECT);
        nextTransmission->disconnect();
    }  //Note: Don't need handle error code
}

void RoBMonitoring::handleStopRDG() {
    if(mIsMonitoringRunning == true) {     //RDG30-R-0931
        LOG_I("Stop RoBMonitoring");
        mIsSendLastUploadData = false;
        //abort immediately
        mCurrentAbortState = Abort::ABORT_BUB;
        if(mCurrentTransmissionId != 0U) {
            const TransmissionInter it {mMonitoringTransList.find(mCurrentTransmissionId)};
            if (it != mMonitoringTransList.end())
            {
                it->second->disconnect();
            }
        } else {
            LOG_E("mCurrentTransmissionId is NULL");
        }
    } else {
        LOG_D("RoBMonitoring is not running");
    }
}

void RoBMonitoring::handleUnableSendUDS(const MonitoringTransmission::State nState)
{
    //Un-response
    const TransmissionInter it {mMonitoringTransList.find(mCurrentTransmissionId)};
    if (it == mMonitoringTransList.end()) {
        LOG_E("cannot find mCurrentTransmissionId");
    } else {
        LOG_D("handleUnableSendUDS");
        if (nState == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_STOP) {
            mIsSendLastUploadData = false;
            //Don't package setting and start UDS when stop usd is failed ==> refer DCM24MON-7249
        } else {
            mIsSendLastUploadData = false;
            android::sp<MonitoringTransmission> const curTransmission{it->second};
            EcuAddressInformation ecuInfo{};
            convertEcuInformation(curTransmission->ecuInformation(), ecuInfo);
            DiagnosticsMessage diagMess{};
            diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_UNRESPONSIVE);
            if (nState == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE) {
                packageUdsData(curTransmission->getUdsMessageReqData(MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_CONFIGURE), diagMess);
                mEcuInformationError.emplace_back(ecuInfo, diagMess);
                packageUdsData(curTransmission->getUdsMessageReqData(MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START), diagMess);
                mEcuInformationError.emplace_back(ecuInfo, diagMess);
            } else if(nState == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START){
                packageUdsData(curTransmission->getUdsMessageReqData(MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_START), diagMess);
                mEcuInformationError.emplace_back(ecuInfo, diagMess);
            } else {
                LOG_E("INVALID tranmission state");  //Note: Error code
            }
        }
    }
}

}
