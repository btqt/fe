#include <google/protobuf/util/json_util.h>
#include "RoBMonitoring.h"
#include <climits>

namespace rdgapp {

RoBMonitoring* RoBMonitoring::mRoBMonitoring{nullptr};

RoBMonitoring::RoBMonitoring (const Remotediag& app, android::sp<sl::SLLooper>& privLooper)
        : android::RefBase()
        , mApp(app)
        , mHandler{new MainHandler(privLooper, *this)}
        , mIsMonitoringRunning{false}
        , mPriority{false}
        , mIGState{false}
        , mTimerHandler{}
        , mUnderRepair{false}
        , mPPIFlag{false}
        , mPriorityId{0}
        , mColId{0U}
        , mCurrentAbortState{Abort::ABORT_INIT}
        , mIsPrioritySuspend{false}
        , mIsSendLastUploadData{true}
        , mMonitoringUploadErrorData{new UploadErrorDataRequest()}
        , mTriggerType(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)
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

void RoBMonitoring::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) {

    //NRC 0x7F
    //Monitoring Stop Request：86 06 02	                    Response : C6 06 00 02
    //Monitoring Setting Request：86 03 02 A0 05 22 A0 06	Response : C6 03 00 02 A0 05 22 A0 06
    //Monitoring Start Request：86 45 02	                Response : C6 45 00 02

    if(mIsMonitoringRunning == true) {
        uint8_t responseType {0U};
        //INIT = 0
        //OKE = 1
        //NEGATIVE(NRC) = 2
        //UNRESPONSIVE = 3
        if(responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_OK) {
            responseType = 1U;
        } 
        else if (responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_NEGATIVE) 
        {
            responseType = 2U;
            mIsSendLastUploadData = false;
        }
        else
        {
            responseType = 3U;
            mIsSendLastUploadData = false;
        }

        //Print Log
        const android::sp<::Buffer> tmpBuf {udsResponse->ToUdsData()};
        if(responseType != 3U) {   //UN response
            std::string log {""};
            for (uint32_t i {0U}; i < tmpBuf->size(); i++)
            {
                if (tmpBuf->data() == nullptr) {
                    LOG_E("UDS payload data is nullptr");
                    break;
                }
                const uint8_t low {tmpBuf->data()[i] & 0x0FU};
                const uint8_t high {tmpBuf->data()[i] >> 4U};
                (void)log.append(1U, uint8ToChar(high));
                (void)log.append(1U, uint8ToChar(low));
                (void)log.append(" ");
            }
            LOG_I("UDS payload: %s", log.c_str());
            log.clear();
        }

        const TransmissionInter it{mMonitoringTransList.find(mCurrentTransmissionId)};
        if (it != mMonitoringTransList.end()) {
            const uint16_t connectId {responseEventInfo->resInfo()->connectId()};
            if (mCurrentConnectId == connectId) {
                LOG_D("Monitoring is running and match connectID. Process receive UDS");
                it->second->stopTimeout();
                if(it->second->getStateData() == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_STOP){   
                    //Don't upload response data 
                    //Update RoB monitoring information list - delete stop ECU - refer sequence
                    EcuAddressInformation ecuRemoveInfo{};
                    convertEcuInformation(it->second->ecuInformation(), ecuRemoveInfo);
                    removeStopECU(ecuRemoveInfo);
                    LOG_D("Don't package response data of stop UDS");   //RDG30-R-1199 
                    if(it->second->getTypeData() == MonitoringTransmission::Type::MONITORING_STOP){
                        it->second->disconnect();
                    } else if(it->second->getTypeData() == MonitoringTransmission::Type::MONITORING_RECONFIGURE) {
                        LOG_V("send Next tranmission");
                        android::sp<::Buffer> udsDataTemp {new ::Buffer()};
                        MonitoringTransmission::State nState {MonitoringTransmission::State::MONITORING_TRANS_INIT};
                        if(it->second->isContinueTransmission(nState, udsDataTemp)){
                            LOG_V("UDS size : %d", udsDataTemp->size());
                            it->second->sendUds(nState, udsDataTemp);
                        } else {
                            LOG_D("undefine continue transmission");
                        }
                    } else {
                        LOG_D("undefine monitoring type");
                    }
                } else {
                    uint32_t centerRxAdd{0U};
                    centerRxAdd = responseEventInfo->getResInfo()->getCanInfo()->getCanId();
                    if(responseType == 2U){
                        LOG_D("Receive NRC response");
                        EcuAddressInformation ecuInfo{};
                        convertEcuInformation(it->second->ecuInformation(), ecuInfo, centerRxAdd);
                        DiagnosticsMessage diagMess{}; 
                        diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL_WITH_NEGATIVE);        //RDG30-R-0552
                        if(udsResponse->ToUdsData()->data() != nullptr) {
                            diagMess.set_user_data(udsResponse->ToUdsData()->data(), udsResponse->ToUdsData()->size());
                        }
                        std::pair<EcuAddressInformation, DiagnosticsMessage> input{};
                        input.first = ecuInfo;
                        input.second = diagMess;
                        mEcuInformationError.push_back(input);  //prepare for upload last error data
                        //Disconnect
                        it->second->setStateData(MonitoringTransmission::State::MONITORING_TRANS_DISCONNECT);
                        it->second->disconnect();
                        LOG_D("END Receive NRC response");
                    } else if(responseType == 3U) {
                        LOG_D("Receive UN response");
                        EcuAddressInformation ecuInfo{};
                        convertEcuInformation(it->second->ecuInformation(), ecuInfo);
                        DiagnosticsMessage diagMess{}; 
                        diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_UNRESPONSIVE);        //RDG30-R-0552
                        if(udsResponse->ToUdsData()->data() != nullptr) {
                            diagMess.set_user_data(udsResponse->ToUdsData()->data(), udsResponse->ToUdsData()->size());
                        }
                        std::pair<EcuAddressInformation, DiagnosticsMessage> input{};
                        input.first = ecuInfo;
                        input.second = diagMess;
                        mEcuInformationError.push_back(input);  //prepare for upload last error data
                        //Disconnect
                        it->second->setStateData(MonitoringTransmission::State::MONITORING_TRANS_DISCONNECT);
                        it->second->disconnect();
                        NOTUSED(centerRxAdd);
                        LOG_D("END Receive UN response"); 
                    } else {
                        /* Save to Response List */
                            //RDG30-R-1193
                        const MonitoringTransmission::State stateData {it->second->getStateData()};
                        if (stateData <= MonitoringTransmission::State::MONITORING_TRANS_MAX)
                        {
                            LOG_D("Receive positive response of UDS - current State (%s)", it->second->enumToString(static_cast<uint32_t>(stateData)).c_str());
                        }
                        else
                        {
                            LOG_E("Invalid state");
                        }
                        
                        EcuAddressInformation ecuInfo{};
                        convertEcuInformation(it->second->ecuInformation(), ecuInfo, centerRxAdd);
                        DiagnosticsMessage diagMess{}; 
                        diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL);     //RDG30-R-1132   
                        // diagMess.set_allocated_ecu_address_information(&ecuInfo);
                        if(udsResponse->ToUdsData()->data() != nullptr) {
                            diagMess.set_user_data(udsResponse->ToUdsData()->data(), udsResponse->ToUdsData()->size());
                        }
                        std::pair<EcuAddressInformation, DiagnosticsMessage> input{};
                        input.first = ecuInfo;
                        input.second = diagMess;
                        mEcuInformationRes.push_back(input);   //RDG30-R-1193 Duplicate Target ECU : 1 for setting and 1 for start

                        LOG_D("send Next tranmission");
                        android::sp<::Buffer> udsDataTemp {new ::Buffer()};
                        MonitoringTransmission::State nState {MonitoringTransmission::State::MONITORING_TRANS_INIT};
                        if(it->second->isContinueTransmission(nState, udsDataTemp)){
                            LOG_D("UDS size : %d", udsDataTemp->size());
                            it->second->sendUds(nState, udsDataTemp);
                        } else {
                            //add to monitoring list after monitoring done
                            EcuAddressInformation ecuAddInfo{};
                            convertEcuInformation(it->second->ecuInformation(), ecuAddInfo);
                            updateRoBInformationList(ecuAddInfo);
                            LOG_D("Disconnect UDS");
                            it->second->setStateData(MonitoringTransmission::State::MONITORING_TRANS_DISCONNECT);
                            it->second->disconnect();
                        }
                    }        
                }
                /* TODO: Delete matched Transmission in mMonitoringTransList*/
            } else {
                LOG_D("Monitoring is NOT running OR NOT match CurrentConnectID. Process receive UDS");
            }
        } else {
            LOG_E("cannot find mCurrentTransmissionId");
        }
    } else {
        LOG_D("Monitoring is not running");
    }
    LOG_I("FINISH onReceiveUDS");
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
            LOG_I("Trigger Request have Type: %d Func: %d Prio: %d ID: %d ColID: %llu", 
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

void RoBMonitoring::handleChangeErasePPIFlag() const {

    LOG_I("handleChangeErasePPIFlag");
}

void RoBMonitoring::handleIGStatus(const bool state) {
    LOG_I({"handleIGStatus, state=%d"}, state);
    mIGState = state;   
    if((mIGState == false) && (mIsMonitoringRunning == true)) {     //RDG30-R-0931
        LOG_I("handleIGStatus : IG ON -> OFF while monitoring process is running");
        mCurrentAbortState = Abort::ABORT_IG_STATE_CHANGE;
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
            if(info == 1){
                mUnderRepair = true;
            } else {
                mUnderRepair = false;
            }
            LOG_I("mUnderRepair (%d)", mUnderRepair);
            if((mUnderRepair == true) && (mIsMonitoringRunning == true)) {  
                mCurrentAbortState = Abort::ABORT_UNDER_REPAIR;
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
                                LOG_D("Don't make monitoring request due to duplication");
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
            
                // Self-Diag
            LOG_E("ECU list is none");
            DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);   
            notifyTriggerDone();
        }

    } else if((!checkPrecondition()) && (mUnderRepair == false)) {                //RDG30-R-0672
        getServiceFlag();
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
    android::sp<DiagTrigger> pTrigger{new DiagTrigger(DiagTrigger::DiagTriggerType::CENTER_TRIGGER,
                                                      prio,
                                                      DiagTrigger::DiagTriggerFunc::ROB_MORNITOR,
                                                      nextTriggerId)};
    /* Get current time */
    TimeManager &mTimeManagerService{TimeManager::getInstance()};
    int64_t current_time {0};
    int64_t tmp_current_time_millis{0};
    tmp_current_time_millis = mTimeManagerService.getCurrentMilliSec();
    if(tmp_current_time_millis > 0) {
        current_time = tmp_current_time_millis/static_cast<int64_t>(1000);
    }    
    pTrigger->setTriggerTime(current_time);
    pTrigger->setCollectionId(colID);
    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
    /* Save request to local*/
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
    LOG_I("Check mSaveReq size: %d", mSaveReq.size());
    if(!ret.second) {
        ret.first->second = pTrigger;
    }
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
    const uint8_t underRepairStatus{DiagManagerAdapter::getInstance()->getUnderRepairStatus()};
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
                    const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
                    const uint64_t colID{it->second->getCollectionID()};
                    mPriorityId = pTriggerId;
                    mColId = colID;
                    mTriggerType = pTriggerType;
                    mPriority = (pTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) ? true : false;
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
    mIsSendLastUploadData = false;
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
    } else {
        LOG_D("Abort init case");
    }
    LOG_I("Finish abort");
    //reset Abort State
    mCurrentAbortState = Abort::ABORT_INIT;
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
	        PriorityControl::getInstance()->notifyTriggerProcessDone(mPriorityId, it->second->getType());
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
        const bool iRoBInfo {((robInfoSize > 0) && (robInfoSize <= 50))  ? true : false};
        if(!iRoBInfo){
            LOG_D("iRoBInfo - robInfoSize (%d)", robInfoSize);
            iResult = false;
            break;
        }
        for(int32_t index {0}; index < robInfoSize; index++){
                //Check RoB Code
            const uint32_t roBCode {mMonitoringRequestData->target_collection_data(idx).rob_information(index).rob()};

                //Check RoB Memory Selection RDG30-R-0790
            const uint32_t memorySelection {mMonitoringRequestData->target_collection_data(idx).rob_information(index).memory_selection()};

                //Check RoB Priority
            const RobInformationPriority robPriority {mMonitoringRequestData->target_collection_data(idx).rob_information(index).rob_priority()};

                //Check RoBFrameNumber
            const int32_t roBFrameNumber {mMonitoringRequestData->target_collection_data(idx).rob_information(index).rob_frame_numbers_size()};

                // check Direct command 
            const int32_t directCommandData {mMonitoringRequestData->target_collection_data(idx).rob_information(index).diagnostics_commands_size()};

            if((roBCode > 0xFFFFFFU) || ((memorySelection != 0x11U) && (memorySelection != 0x12U) && (memorySelection != 0x13U) && (memorySelection != 0x14U))
                || ((robPriority != RobInformationPriority::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_RobPriority_RP_PRIORITY_LOW) 
                && (robPriority != RobInformationPriority::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_RobPriority_RP_PRIORITY_HIGH))
                || (roBFrameNumber > 255)
                || (directCommandData > 512)){
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
    const error_t ret {DataModel<CollectionConditionRobRobSsrDidEvent>::save(OCCURRENCE_ROB_MONITORING_DB_NAME, obj)};
    if(ret == TIGER_ERR::E_OK) {
        LOG_I("SUCCESS");
            // Self-Diag
        const uint8_t operation{getOperation()};
        DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
    } else {
        LOG_I("FAILURE");
    }
    return ret;
}

error_t RoBMonitoring::clearRoBInformationList(void) const {
    const error_t ret {DataModel<CollectionConditionRobRobSsrDidEvent>::clear(OCCURRENCE_ROB_MONITORING_DB_NAME)};
    if(ret == TIGER_ERR::E_OK) {
        LOG_I("SUCCESS");
    } else {
        LOG_I("FAILURE");
    }
    return ret;
}

std::shared_ptr<CollectionConditionRobRobSsrDidEvent> RoBMonitoring::getRoBInformationList(void) const {
    LOG_I("getRoBInformationList"); 
    return DataModel<CollectionConditionRobRobSsrDidEvent>::get(OCCURRENCE_ROB_MONITORING_DB_NAME);
}

bool RoBMonitoring::isValidECUAddress(const EcuAddressInformation ecu){   //RDG30-R-1130
    LOG_I("ECU address: 0x%02x", ecu.target_address());
    bool isResult {false};
    std::list<CommonDefine::EcuInformation>::iterator it{};
    //1. Parameter is abnormal.
        //(Communication protocol is unknown || Communication protocol is out of value) || 
        //  (Communication protocol  is CAN && (Communication type is not set || Communication type is out of value)) || 0 is specified for target address) 
    if(ecu.communication_protocol() == vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN){
        isResult = false;
        LOG_D("ECU communication_protocol: %d", ecu.communication_protocol());
        goto exit;
    }

    if((ecu.communication_protocol() < 0) || (ecu.communication_protocol() > 3)){
        isResult = false;
        LOG_D("ECU communication_protocol: %d", ecu.communication_protocol());
        goto exit;
    }

    if(((ecu.communication_protocol() == vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_CAN) || (ecu.communication_protocol() == vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD))
        && ((ecu.communication_type() == vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN) || (ecu.communication_type() < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN) || (ecu.communication_type() > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))) {
        isResult = false;
        LOG_D("ECU communication_protocol: %d", ecu.communication_protocol());
        LOG_D("ECU communication_type: %d", ecu.communication_type());
        goto exit;
    }
    if(ecu.target_address() == 0U) {
        isResult = false;
        goto exit;
    }
   
    //2. Target ECU does not exist RDG30-R-1164
    for (it = mEcuInformationList.begin(); it != mEcuInformationList.end(); ++it)
    {
        if ((*it).getTargetAddress() == ecu.target_address())
        {
            isResult = true;
            LOG_D(" ECU is exist. ECU diagPhase in DCM: %d", (*it).getDiagPhase());
        } 

        //3. DiagPhase is Unknown + RDG30-R-0090 ==> only check for phase 6
        if(isResult == true) {
            if((*it).getDiagPhase() != CommonDefine::DiagPhase::DP_PHASE_6) {
                isResult = false;
                LOG_D("This ecu is not phase 6, process for acquire monitoring is abort");
            }
            goto exit;
        } else {
            // do nothing
        }
    }

exit: 
    LOG_I("ECU valid: %d", isResult);
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
            (void)mMonitoringUploadData->mutable_target_collection_data()->erase(it);  //delete this ecu
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
            const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocol{ecu.communication_protocol()};
            if ((protocol >= vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN)
                && (protocol <= vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_IP))
            {
                ecuInformation.setCommProtocol(protocol);
            }
            else
            {
                LOG_E("invalid protocol");
            }
            ecuInformation.setCommType(ecu.communication_type());
            const CommonDefine::DiagPhase diagPhase{(*it).getDiagPhase()};
            if((diagPhase >= CommonDefine::DiagPhase::DP_UNKNOW) && (diagPhase <= CommonDefine::DiagPhase::DP_PHASE_6))
            {
                ecuInformation.setDiagPhase(diagPhase);
            } else {
                LOG_E("Unknown diag phase");
            }
            ecuInformation.setSwPartNumber((*it).getSwPartNumber());
            ecuInformation.setHwPartNumber((*it).getHwPartNumber());
        }
        if(isResult == true){
            if((*it).getecuActiveFlag() == true) {
                LOG_I("ECU (0x%02x) is IG-ON", (*it).getTargetAddress()); 
            } else {
                LOG_I("ECU (0x%02x) is IG-OFF", (*it).getTargetAddress()); 
                isResult = false;
            }
            goto exit;
        }
    }
exit: 
    return isResult;
}

bool RoBMonitoring::isMakeRequestMonitor(const EcuAddressInformation ecu) {  
    LOG_I("Check ECU (0x%02x)  isMakeRequestMonitor or not", ecu.target_address());
    bool iResult {false};
    std::list<std::pair<CommonDefine::EcuInformation, RoBMonitoring::MonitoringTransmission::Type>>::iterator it{};
    for(it = mTranmissionList.begin(); it != mTranmissionList.end(); ++it){
        const CommonDefine::EcuInformation ecuInfo{it->first};
        if(ecuInfo.getTargetAddress() == ecu.target_address()) {
            iResult = true;
            break;
        }
    }
    LOG_I("isMakeRequestMonitor result : %d", iResult);
    return iResult;
}

RoBMonitoring::MonitoringTransmission::MonitoringTransmission( RoBMonitoring& robMonitoringData, const CommonDefine::EcuInformation ecuInformation, const Type typeData)
: android::RefBase()
, mConnectId(0U)
, mTransmissionId(0U)
, mState(State::MONITORING_TRANS_INIT)
, mType(typeData)
, mMonitoring(robMonitoringData)
, mTimerHandler(robMonitoringData)
, mTimeOut(&mTimerHandler, TimerHandler::ID_TRANSMISSION_TIMEOUT)
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
        mUdsReqStop.getOptionData()->append(&eventWindowTime, 1);
    }

    { //SETTING
        mUdsReqSetting.setSID(sID);         //86 03 02 A0 05 22 A0 06
        mUdsReqSetting.setSFID(sfidSetting);
        mUdsReqSetting.getOptionData()->append(&eventWindowTime, 1);
        mUdsReqSetting.getOptionData()->append(&typeRecord1, 1);
        mUdsReqSetting.getOptionData()->append(&typeRecord2, 1);
        mUdsReqSetting.getOptionData()->append(&serID, 1);
        mUdsReqSetting.getOptionData()->append(&serParam1, 1);
        mUdsReqSetting.getOptionData()->append(&serParam2, 1);
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
    LOG_D("TranmissionID (0x%02llx) - Type: %s", mTransmissionId, enumToString(static_cast<uint32_t>(mType)).c_str());
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
    const OBCResourceEventCode resEventInfo {OnboardclientAdapter::getInstance()->GetObcResource()};
    if (resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK)
    {
        LOG_E("GetObcResource OBC_GET_RESOURCE_WAIT -> wait and check after");
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(MainHandler::CMD_MONITORING_START_TRANMISSION), 1000U);
    }
    else
    {
        LOG_I("Set obc resource OBC_GET_RESOURCE_WAIT");
        OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
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
    LOG_I("finish Current transmission ID: 0x%02llx, mTransmissionIdList size = %d", mCurrentTransmissionId, mTransmissionIdList.size());
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
                    PriorityControl::getInstance()->notifyTriggerProcessDone(mPriorityId, its->second->getType());
                } else {
                    LOG_E("cannot find mPriorityId");
                }
                // (void)mSaveReq.erase(it);   ==> Suspend -> not erase
                //TimeManager &mTimeManagerService{TimeManager::getInstance()};
                const int64_t current_time{ParamsDef::getCurrentAcquisiteTime()};
                LOG_I("Collection ID: %llu - Last complete time: %lld", mColId, current_time);
                mApp.notifyLastOpComplTime(mColId, current_time);
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
            LOG_I("DONE TRIGGER PRIORITY PROCESS ID(%d)", mPriorityId);
            PriorityControl::getInstance()->notifyTriggerProcessDone(mPriorityId, it->second->getType());
            //Notify last complete time
            //TimeManager &mTimeManagerService{TimeManager::getInstance()};
            const int64_t current_time{ParamsDef::getCurrentAcquisiteTime()};
            LOG_I("Collection ID: %llu - Last complete time: %lld", mColId, current_time);
            mApp.notifyLastOpComplTime(mColId, current_time);
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
    LOG_I("Event Transmission Timeout, currentTrans = 0x%02llx", mCurrentTransmissionId); 
    const TransmissionInter itTrans {mMonitoringTransList.find(mCurrentTransmissionId)};
    if (itTrans != mMonitoringTransList.end()) {
        if(itTrans->second->getStateData() == MonitoringTransmission::State::MONITORING_TRANS_SEND_UDS_STOP){    //RDG30-R-1199
            //Don't upload response data 
            LOG_D("Don't package response data of stop UDS");
        } else {
            EcuAddressInformation ecuInfo{};
            convertEcuInformation(itTrans->second->ecuInformation(), ecuInfo);
            DiagnosticsMessage diagMess{}; 
            diagMess.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_UNRESPONSIVE);        //RDG30-R-0552
            pair<EcuAddressInformation, DiagnosticsMessage> input{};
            input.first = ecuInfo;
            input.second = diagMess;
            mEcuInformationError.push_back(input);  //prepare for upload last error data
        }
    } else {
        LOG_E("cannot find mCurrentTransmissionId");
    }
    finishCurrentTransmission();
    OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
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
        const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};

        //Header
        mMonitoringUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);  
        mMonitoringUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
        mMonitoringUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
        const int32_t timezoneOffSet_hour {TimeManager::getInstance().getOffset() / 60};
        const int32_t timezoneOffSet_minute {TimeManager::getInstance().getOffset() % 60};
        LOG_D("Get timezone offset: %d:%d ", timezoneOffSet_hour, timezoneOffSet_minute);
        mMonitoringUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(timezoneOffSet_hour);
        mMonitoringUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(timezoneOffSet_minute); 

        mMonitoringUploadErrorData->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
            //MessageID
        mMonitoringUploadErrorData->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, counterValue));
        mMonitoringUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
        //Content
        mMonitoringUploadErrorData->set_collection_condition_id(mColId);
        mMonitoringUploadErrorData->set_trigger_type(RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER);   //RoB Monitoring operate when center trigger
        mMonitoringUploadErrorData->set_counter_value(counterValue);
        (void)counterValue;
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
            const uint32_t tmpAdditionalSize {static_cast<uint32_t>(it->second.ByteSizeLong())};
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
                if (it->second.ByteSizeLong() > 0U)
                {
                    diagMessage->set_status_code(it->second.status_code());
                    vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{it->first.communication_protocol()};
                    if ((protocolType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN) || (protocolType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_IP))
                    {
                        protocolType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN;
                    }
                    ecuAddressInfo->set_communication_protocol(protocolType);
                    ecuAddressInfo->set_communication_type(it->first.communication_type());
                    ecuAddressInfo->set_target_address(it->first.target_address());
                    if (it->second.user_data().size() > 0U) {
                        const vccomif::rdg::v1::interfaces::StatusCode tmpStatuscode{diagMessage->status_code()};
                        if((tmpStatuscode < vccomif::rdg::v1::interfaces::StatusCode::SC_UNKNOWN) ||
                        (tmpStatuscode > vccomif::rdg::v1::interfaces::StatusCode::SC_ECU_SPECIFIED_ERROR)){
                            LOG_D("Invalid status code");
                        } else{
                            uint32_t sizeOfStatusCode {0U};
                            sizeOfStatusCode = sizeof(::vccomif::rdg::v1::interfaces::StatusCode);
                            const uint32_t sizeOfEcuAddressInformation {diagMessage->ecu_address_information().ByteSizeLong()};
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
                            diagMessage->set_user_data(&it->second.user_data()[0], remainingBytes < it->second.user_data().size() ? remainingBytes : it->second.user_data().size());
                            LOG_D("EXCEEDED_SIZE");
                        }
                    }
                }
                (void) diagMessage;
                (void) ecuAddressInfo;
                (void)sizeOfFileCounter;
                break;
            } else {
                diagMessage->set_status_code(it->second.status_code());
                vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{it->first.communication_protocol()};
                if ((protocolType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN) || (protocolType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_IP))
                {
                    protocolType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN;
                }
                ecuAddressInfo->set_communication_protocol(protocolType);
                ecuAddressInfo->set_communication_type(it->first.communication_type());
                ecuAddressInfo->set_target_address(it->first.target_address());
                diagMessage->set_user_data(it->second.user_data());
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
        const uint32_t uploadId {UploadManager::getInstance()->genRequestId()};
        std::string file_dir {std::to_string(uploadId)};
        (void)file_dir.append("_UploadErrorMonitoringData.dat");
        const error_t bStored{DataModel<UploadErrorDataRequest>::save(file_dir, *mMonitoringUploadErrorData)};
        if (bStored == E_OK)
        {
            const uint8_t operation{getOperation()};
            // Self-Diag
            DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        }
        const android::sp<UploadTask> task {new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
        task->setUploadPatch(file_dir);   //TODO::wait upload module complete
        task->setUploadId(static_cast<uint64_t>(uploadId));
        /*Set priority*/       
        uint32_t uploadPriority{0U};
        if(mPriority){
            uploadPriority = 1U;
        } else {
            uploadPriority = 0U;
        }
        task->setUploadPrio(uploadPriority);
        // test_saveUploadData = task;
        const uint64_t fileSize{static_cast<uint64_t>(mMonitoringUploadErrorData->ByteSizeLong())};
        task->setFileSize(fileSize);
        UploadManager::getInstance()->requestUploadTask(task);  //TODO
        mEcuInformationError.clear();
        mEcuInformationRes.clear();
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
                //Header  
            mMonitoringUploadResponseData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);  
            mMonitoringUploadResponseData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
            mMonitoringUploadResponseData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);

            const int32_t timezoneOffSet_hour {TimeManager::getInstance().getOffset() / 60};
            const int32_t timezoneOffSet_minute {TimeManager::getInstance().getOffset() % 60};
            LOG_D("Get timezone offset: %d:%d ", timezoneOffSet_hour, timezoneOffSet_minute);
            mMonitoringUploadResponseData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(timezoneOffSet_hour);
            mMonitoringUploadResponseData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(timezoneOffSet_minute); 

            mMonitoringUploadResponseData->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_REQUEST_RESPONSE_NOTIFICATION);
            const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
            //MessageID
            mMonitoringUploadResponseData->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_REQUEST_RESPONSE_NOTIFICATION, counterValue));

                //Content
            mMonitoringUploadResponseData->set_collection_condition_id(mColId);
            mMonitoringUploadResponseData->set_counter_value(counterValue);
            (void)counterValue;
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
                RdgProtoInterface::DiagnosticsMessage* const diagMessage {mMonitoringUploadResponseData->add_ecu_response_messages()};
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {diagMessage->mutable_ecu_address_information()};
                const uint32_t tmpAdditionalSize {static_cast<uint32_t>(it->second.ByteSizeLong())};
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
                    if (it->second.ByteSizeLong() > 0U)
                    {
                        diagMessage->set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL_WITH_EXCEEDED_SIZE);
                        vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{it->first.communication_protocol()};
                        if ((protocolType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN) || (protocolType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_IP))
                        {
                            protocolType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN;
                        }
                        ecuAddressInfo->set_communication_protocol(protocolType);
                        ecuAddressInfo->set_communication_type(it->first.communication_type());
                        ecuAddressInfo->set_target_address(it->first.target_address());

                        if (it->second.user_data().size() > 0U) {
                            // const vccomif::rdg::v1::interfaces::StatusCode tmpStatuscode{vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL_WITH_EXCEEDED_SIZE};
                            uint32_t sizeOfStatusCode {0U};
                            sizeOfStatusCode = sizeof(vccomif::rdg::v1::interfaces::StatusCode);
                            const uint32_t sizeOfEcuAddressInformation {diagMessage->ecu_address_information().ByteSizeLong()};
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
                            diagMessage->set_user_data(&it->second.user_data()[0], remainingBytes < it->second.user_data().size() ? remainingBytes : it->second.user_data().size());
                            LOG_D("EXCEEDED_SIZE - SC_SUCCESSFUL_WITH_EXCEEDED_SIZE");
                        }
                    }
                    (void) diagMessage;
                    (void) ecuAddressInfo;
                    (void) sizeOfFileCounter;
                    break;
                } else {
                    diagMessage->set_status_code(it->second.status_code());
                    vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{it->first.communication_protocol()};
                    if ((protocolType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN) || (protocolType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_IP))
                    {
                        protocolType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN;
                    }
                    ecuAddressInfo->set_communication_protocol(protocolType);
                    ecuAddressInfo->set_communication_type(it->first.communication_type());
                    ecuAddressInfo->set_target_address(it->first.target_address());
                    diagMessage->set_user_data(it->second.user_data());
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

            const uint32_t uploadId {UploadManager::getInstance()->genRequestId()};
            std::string file_dir {std::to_string(uploadId)};
            (void)file_dir.append("_LastUploadResponseMonitoringDataRequest.dat");
            const error_t bStored{DataModel<UploadRequestResponseNotificationRequest>::save(file_dir, *mMonitoringUploadResponseData)};
            if (bStored == E_OK)
            {
                const uint8_t operation{getOperation()};
                // Self-Diag
                DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
            }
            const android::sp<UploadTask> task {new UploadTask(uploadId)};
            task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG130);
            task->setUploadPatch(file_dir);   
            task->setUploadId(static_cast<uint64_t>(uploadId));
            /*Set priority*/
            uint32_t uploadPriority{0U};
            if(mPriority){
                uploadPriority = 1U;
            } else {
                uploadPriority = 0U;
            }
            task->setUploadPrio(uploadPriority);
            // test_saveUploadData = task;
            const uint64_t fileSize{static_cast<uint64_t>(mMonitoringUploadResponseData->ByteSizeLong())};
            task->setFileSize(fileSize);
            UploadManager::getInstance()->requestUploadTask(task); 

            mEcuInformationRes.clear();
            LOG_I("Finish is makeUploadResponse");
        }
    } else {
        LOG_I("mMonitoringUploadResponseData is empty");
    }
    LOG_E("End makeUploadResponseData");
}

void RoBMonitoring::convertEcuInformation(const CommonDefine::EcuInformation ecuInformation, EcuAddressInformation &ecu, const uint32_t rxAdd) const {
    LOG_V("convertEcuInformation");
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
    LOG_I("Start connect, transmissionID = 0x%02llx", this->transmissionId());
    const android::sp<OBCTransportInfo> mtransportInfo{new OBCTransportInfo()};
    const android::sp<OBCConnectInfo> mConnectInfo{new OBCConnectInfo()};
    OBCCanInfo mcanInfo{};
    const std::vector<std::string> ntaArray{};
    mcanInfo.setData(this->ecuInformation().getTargetAddress(), ntaArray);
    constexpr const bool mperiodicRes{false};
    constexpr const uint16_t mudsResTimeout{0U};
    mtransportInfo->setData(static_cast<uint8_t>(RemoteEcuInformation::getInstance()->convertObcProtocolType(this->ecuInformation().getCommProtocol(), this->ecuInformation().getCommType(), this->ecuInformation().getTargetAddress()))
                                , mcanInfo
                                , mperiodicRes
                                , mudsResTimeout);
    const std::string applicationName{"remotediag"};
    (void)OnboardclientAdapter::getInstance()->connect(mtransportInfo, applicationName, mConnectInfo);
    if (mConnectInfo->getResponse() == OBCEnum::OBCErrCode::OBC_OK)
    {
        this->connectId() = mConnectInfo->getConnectId();
        mMonitoring.mCurrentConnectId = mConnectInfo->getConnectId();
        this->setStateData(MonitoringTransmission::State::MONITORING_TRANS_CONNECT);
        OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
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
        OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
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
    LOG_I("MonitoringTransmission::disconnect()");
    (void)OnboardclientAdapter::getInstance()->disconnectECU(this->connectId());
    OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
    this->setStateData(MonitoringTransmission::State::MONITORING_TRANS_DONE);
    mMonitoring.finishCurrentTransmission();
    LOG_I("DONE disconnect");
}

void RoBMonitoring::MonitoringTransmission::sendUds(const MonitoringTransmission::State nState, const android::sp<::Buffer> uds)
{
    LOG_I("MonitoringTransmission::sendUds() - state(%s)", enumToString(static_cast<uint32_t>(nState)).c_str());
    //Print send UDS data
    const uint8_t res{OnboardclientAdapter::getInstance()->sendUdsData(this->connectId(), uds)};
    if (res != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK))
    {
        // TODO Handle exeptional case
        this->disconnect();
    }
    else
    {
        this->setStateData(nState);
        LOG_I("SendUdsData success - current state: %s", enumToString(static_cast<uint32_t>(nState)).c_str());
        //print UDS data
        std::string log {""};
        for (uint32_t i {0U}; i < uds->size(); i++)
        {
            if (uds->data() == nullptr) {
                LOG_E("[sendUDSData] uds data is nullptr");
                break; 
            }
            const uint8_t low {uds->data()[i] & 0x0FU};
            const uint8_t high {uds->data()[i] >> 4U};
            (void)log.append(1U, mMonitoring.uint8ToChar(high));
            (void)log.append(1U, mMonitoring.uint8ToChar(low));
            (void)log.append(" ");
        }
        LOG_I("[sendUDSData] uds data = %s", log.c_str());
        log.clear();

        mTimeOut.start();
        LOG_I("Start timeout timer for mTransmissionId 0x%02llx", this->transmissionId());
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

error_t RoBMonitoring::getRobMonitoringList(TargetCollectionDataOccurrentRobList& aList, Uint64& collectionConditionId)
{
    error_t result{E_OK};
    if ((mMonitoringUploadData == nullptr) || (mMonitoringUploadData->target_collection_data().size() <= 0)) 
    {
        result = E_BUFFER_EMPTY;
    }
    else {
        collectionConditionId = mMonitoringUploadData->collection_condition_id();
        aList.CopyFrom(mMonitoringUploadData->target_collection_data());
    }
    return result;
}

uint8_t RoBMonitoring::getOperation() const noexcept
{
    uint8_t operation{DiagManagerAdapter::COLLECTION_CONDITIONS};
    if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
    {
        operation = DiagManagerAdapter::RD_SCHEDULE_TRIGGER;
    }
    else if (mTriggerType == DiagTrigger::DiagTriggerType::ONE_SHOT_TRIGGER)
    {
        operation = DiagManagerAdapter::RD_SCHEDULE_TRIGGER;
    }
    else if (mTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
    {
        operation = DiagManagerAdapter::COLLECTION_CONDITIONS;
    }
    else
    {
        operation = DiagManagerAdapter::COLLECTION_CONDITIONS;
    }
    return operation;
}

void RoBMonitoring::updateRoBInformationList(const EcuAddressInformation& ecu)
{
    LOG_I("updateRoBInformationList ");
    int32_t targetSize;
    targetSize = mMonitoringRequestData->target_collection_data_size();
    for(int32_t idx {0}; idx < targetSize; idx ++){
        if(ecu.target_address() == mMonitoringRequestData->target_collection_data(idx).ecu_address_information().target_address()){
            const TargetCollectionDataOccurrentRob tmpTargetCollectionData {mMonitoringRequestData->target_collection_data(idx)};
            mMonitoringList->mutable_target_collection_data()->Add()->CopyFrom(tmpTargetCollectionData);
        }
    }
}

void RoBMonitoring::removeStopECU(const EcuAddressInformation ecu)
{
    LOG_I("removeStopECU ");
    TargetCollectionDataOccurrentRobList::const_iterator it {mMonitoringUploadData->target_collection_data().cbegin()};
    while(it != mMonitoringUploadData->target_collection_data().cend())
    {
        if(it->ecu_address_information().target_address() == ecu.target_address())
        {
            (void)mMonitoringUploadData->mutable_target_collection_data()->erase(it);  //delete this ecu
        } else {
            ++it;
        }
    }
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
}
