#include <utils/Buffer.h>

#include "services/DiagManagerAdapter.h"
#ifdef ENABLE_LGE_LXC
#include "services/RegionManagerAdapter.h"
#endif /* ENABLE_LGE_LXC */
#include "CommonUtils.h"
#include "CollectionCondition.h"
#include "DataModel.h"
#include "Logger.h"
#include "diagprocess/CenterReqDataType.h"
#include "utils/RemotediagHandler.h"

ANDROID_SINGLETON_STATIC_INSTANCE(rdgapp::CollectionCondition)

namespace rdgapp {

CollectionCondition::CollectionCondition()
    : android::RefBase()
    , mGetCollectionConditionRequest{nullptr}
    , mGetCollectionConditionRequestBuff{nullptr}
    , mNotifyCollectionConditionUpdateResultRequest{nullptr}
    , mCollectionConditionDirectCommandList{}
    , mCollectionConditionTimerHandler(*this)
    , mCollectionConditionTimer(&mCollectionConditionTimerHandler, IG_ON_STATE_MAINTAINED_CHECK_ID)
    , mTriggerType{DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN}
    , mDeletedCollectionConditionIds()
    , isIgOnStateMaintainedCheck(false)
{
    mCollectionConditionTimer.setDuration(ID_IG_ON_STATE_MAINTAINED_CHECK_TIMEOUT, 0U);
}

void CollectionCondition::init(android::sp<sl::SLLooper> &privateLooper)
{
    mCollectionConditionHandler = new MainHandler(privateLooper, *this);
    mGetCollectionConditionRequest = getGetCollectionConditionRequest();
}

android::sp<sl::Handler> CollectionCondition::getHandler() const noexcept
{
    return mCollectionConditionHandler;
}

void CollectionCondition::printData(const std::string data) const
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

void CollectionCondition::MainHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
{
    const int32_t whatCmd{handlemsg->what};

    switch (whatCmd)
    {
    case CollectionCondition::CMD_CENTER_PUSH_NOTIFICATION:
    {
        LOG_I("CMD_CENTER_PUSH_NOTIFICATION");
        // 24DCM_RDG_DIS-FR01_281
        if (PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_ON)
        {
            const android::sp<CollectionCondition::CocoTransmission> newTrans {new CollectionCondition::CocoTransmission(mCollectionCondition
                                                                        , DiagTrigger::DiagTriggerType::CENTER_TRIGGER
                                                                        , GRPC_IF_TYPE::DCIF_RDG010)};
            newTrans->setReqPayload(mCollectionCondition.makeGetCollectionConditionRequest(DiagTrigger::DiagTriggerType::CENTER_TRIGGER));
            newTrans->send();
            mCollectionCondition.addCocoTransmission(newTrans);
        } 
        else if (PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_OFF) 
        {
            LOG_D("Abort Center push notification, because IG status is OFF");
        } else {
            //Do noting
        }
        break;
    }
    case CollectionCondition::CMD_SEND_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT_REQUEST:
    {
        LOG_I("CMD_SEND_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT_REQUEST");
        const android::sp<CollectionCondition::CocoTransmission> newTrans {new CollectionCondition::CocoTransmission(mCollectionCondition, DiagTrigger::DiagTriggerType::CUSTOMIZE_TRIGGER, GRPC_IF_TYPE::DCIF_RDG012)};
        newTrans->setReqPayload(mCollectionCondition.mNotifyCollectionConditionUpdateResultRequest);
        newTrans->send();
        mCollectionCondition.addCocoTransmission(newTrans);
        break;
    }
    case CollectionCondition::CMD_FEATURE_STATUS_CHANGE:
    {
        LOG_I("CMD_FEATURE_STATUS_CHANGE");
        mCollectionCondition.onOtherFeatureStatusOff();
        break;
    }
    case CollectionCondition::CMD_HANDLE_IG_STATUS_CHANGE:
    {
        LOG_I("CMD_HANDLE_IG_STATUS_CHANGE");
        bool status{false};
        if (handlemsg->arg1 == 1)
        {
            status = true;
        }
        else if (handlemsg->arg1 == 0)
        {
            status = false;
        }
        else
        {
            // Do nothing
        }
        mCollectionCondition.onReceiveIG(status);
        break;
    }
    case CollectionCondition::CMD_RECEIVE_COLLECTION_CONDITION_REQUEST_RESPONSE:
    {
        LOG_I("CMD_RECEIVE_COLLECTION_CONDITION_REQUEST_RESPONSE");
        android::sp<GrpcResData> pGrpcResData{nullptr};
        handlemsg->getObject(pGrpcResData);
        if (pGrpcResData != nullptr)
        {
            mCollectionCondition.onReceivedGetCollectionConditionResponse(pGrpcResData);
        }
        break;
    }
    case CollectionCondition::CMD_RECEIVE_COLLECTION_CONDITION_UPDATE_RESULT_RESPONSE:
    {
        LOG_I("CMD_RECEIVE_COLLECTION_CONDITION_UPDATE_RESULT_RESPONSE");
        android::sp<GrpcResData> pGrpcResData{nullptr};
        handlemsg->getObject(pGrpcResData);
        if (pGrpcResData != nullptr)
        {
            mCollectionCondition.onReceivedNotifyCollectionConditionUpdateResultResponse(pGrpcResData);
        }
        break;
    }
    case CollectionCondition::CMD_HANDLE_GRPC_COMMUNICATION_RECONNECT:
    {
        LOG_I("CMD_HANDLE_GRPC_COMMUNICATION_RECONNECT");
        mCollectionCondition.onGrpcReconnect();
        break;
    }
    case CollectionCondition::CMD_HANDLE_IG_ON_TIMEOUT:
    {
        LOG_I("CMD_HANDLE_IG_ON_TIMEOUT");
        mCollectionCondition.onIgOnTimeout();
        break;
    }
    case CollectionCondition::CMD_HANDLE_TRANSMISSON_TIMEOUT:
    {
        LOG_I("CMD_HANDLE_TRANSMISSON_TIMEOUT");
        const int32_t callId{handlemsg->arg1};
        CollectionCondition::CocoTransmission::State currState {CollectionCondition::CocoTransmission::State::COCO_TRANS_IDLE};
        if ((handlemsg->arg2 >= static_cast<int32_t>(CollectionCondition::CocoTransmission::State::COCO_TRANS_IDLE)) &&
            (handlemsg->arg2 <= static_cast<int32_t>(CollectionCondition::CocoTransmission::State::COCO_TRANS_FINISHED)))
        {
            currState = static_cast<CollectionCondition::CocoTransmission::State>(handlemsg->arg2);
        }
        GRPC_IF_TYPE grpcIfType {GRPC_IF_TYPE::DCIF_CAN010};
        if ((handlemsg->arg3 >= static_cast<int32_t>(GRPC_IF_TYPE::DCIF_RDG010)) && (handlemsg->arg3 <= static_cast<int32_t>(GRPC_IF_TYPE::DCIF_RDG012)))
        {
            grpcIfType = static_cast<GRPC_IF_TYPE>(handlemsg->arg3);
        }
        const std::vector<android::sp<CollectionCondition::CocoTransmission>>::iterator cocoTransIter{mCollectionCondition.findCocoTransmission(callId, currState, grpcIfType)};
        if (cocoTransIter != mCollectionCondition.mCocoTransmissionList.end())
        {
            (*cocoTransIter)->handleTransmissionTimeout();
        }
        break;
    }
    case CollectionCondition::CMD_STOP_RDG:
    {
        LOG_I("CMD_STOP_RDG");
        break;
    }
    default:
        break;
    }
}

void CollectionCondition::onReceivedCenterRequest(void)
{
    LOG_D("onReceived Center push Request");
    // RDG30-R-0863
    (void)mCollectionConditionHandler->obtainMessage(CollectionCondition::CMD_CENTER_PUSH_NOTIFICATION)->sendToTarget();
}

void CollectionCondition::onReceiveIG(const bool status)
{
    if (status == true)
    {
        // IG ON
        mCollectionConditionTimer.start();
        LOG_D("onReceiveIG ON");
        isIgOnStateMaintainedCheck = true;
    } else {
        LOG_D("onReceiveIG OFF");
        isIgOnStateMaintainedCheck = false;
        mCollectionConditionTimer.stop();
        std::vector<android::sp<CollectionCondition::CocoTransmission>>::iterator it {mCocoTransmissionList.begin()};
        while(it != mCocoTransmissionList.end())
        {
            (*it)->stopTimeout();
            it++;
        }
        (void)mCocoTransmissionList.clear();

        // std::remove_if(mCocoTransmissionList.begin()
        //                 , mCocoTransmissionList.end()
        //                 , [](const android::sp<CollectionCondition::CocoTransmission>& aCocoTransmission) {
        //                   aCocoTransmission->stopTimeout();
        //                   return true;
        //                 });

        const android::AutoMutex _l{mLock};
        std::unordered_map<uint64_t, std::shared_ptr<CenterRequestJob>> tmpJobList{};
        std::unordered_map<uint64_t, std::shared_ptr<CenterRequestJob>>::iterator jobIter {mCenterRequestJobList.begin()};
        while(jobIter != mCenterRequestJobList.end())
        {
            if (jobIter->second->getScheduleType() == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT)
            {
                tmpJobList[jobIter->first] = jobIter->second;
            }
            jobIter++;
        }
        mCenterRequestJobList.clear();

        std::unordered_map<uint64_t, std::shared_ptr<CenterRequestJob>>::iterator tmpJobIter {tmpJobList.begin()};
        while(tmpJobIter != tmpJobList.end())
        {
            mCenterRequestJobList[tmpJobIter->first] = tmpJobIter->second;
            tmpJobIter++;
        }
    }
}

void CollectionCondition::onRdgStop(const bool isStop) const {
    //obtain message to stop rdg
    (void)mCollectionConditionHandler->obtainMessage(CollectionCondition::CMD_STOP_RDG)->sendToTarget();
    (void)isStop;
}

void CollectionCondition::onReceivedNotifyCollectionConditionUpdateResultResponse(const android::sp<GrpcResData> pGrpcResData)
{
    LOG_I("Received NotifyCollectionConditionUpdateResultResponse");
    if (pGrpcResData != nullptr)
    {
        if ((pGrpcResData->getGrpcResult() == GRPC_RESULT::SEND_SUCCESS) && (pGrpcResData->getGrpcCode() == grpc::StatusCode::OK)) {
            handleCenterRespondSuccess(pGrpcResData);
            (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_COLLECTION_CONDITION_UPDLOAD_END)->sendToTarget();
        } else {
            handleGrpcClientErrorEvent( pGrpcResData);
        }
    }
}

void CollectionCondition::onReceivedGetCollectionConditionResponse(const android::sp<GrpcResData> pGrpcResData)
{
    LOG_I("Received get collection condition response");
    bool isNeedNotify {false};
    bool isAbortUpdateCollectionCondition {false};
    (void)mDeletedCollectionConditionIds.clear();
    (void)mUpdatedCollectionConditionIds.clear();
    if (pGrpcResData != nullptr)
    {
        const grpc::StatusCode responseCode {pGrpcResData->getGrpcCode()};
        const GRPC_RESULT gRpcResult {pGrpcResData->getGrpcResult()};

        if (gRpcResult == GRPC_RESULT::SEND_SUCCESS)
        {
            if (responseCode == grpc::StatusCode::OK)
            {
                handleCenterRespondSuccess(pGrpcResData);
                google::protobuf::util::JsonOptions option{};
                option.always_print_primitive_fields = true;
                option.preserve_proto_field_names = true;
                UpdateResultCode finalUrc{URC_SUCCESS};
                mNotifyCollectionConditionUpdateResultRequest = std::make_shared<NotifyCollectionConditionUpdateResultRequest>();
                ErrorInformationList * const errInfor {mNotifyCollectionConditionUpdateResultRequest->mutable_error_information()};

                const std::shared_ptr<GetCollectionConditionResponse> res {pGrpcResData->getResponseProtoBuf<GetCollectionConditionResponse>()};
                if (res != nullptr)
                {
                    LOG_D("Received GetCollectionConditionResponse from the Center");
                    std::string getCollectionConditionResponseStr{};
                    (void)google::protobuf::util::MessageToJsonString(*res, &getCollectionConditionResponseStr, option);
                    printDataDebug(getCollectionConditionResponseStr);
                    LOG_D("============================================================================================");

                    const bool newRdgFlag{res->rdg_active_flag()};

                    DiagManagerAdapter::getInstance()->saveRDGFlag(newRdgFlag);

                    const uint32_t freespace {CommonUtils::getFreespace(DATA_PATH)};
                    // RDG30-R-1198
                    LOG_D("Collection condition size: %u", res->ByteSizeLong());
                    if (res->ByteSizeLong() > freespace)
                    {
                        LOG_E("Total data size of the received data collection condition is larger than the storage capacity");
                        finalUrc = URC_FAILED;
                        errInfor->Add()->mutable_error_messages()->Add(VEHICLE_RELATED_ERROR("not_enough_storage_capacity"));
                        isAbortUpdateCollectionCondition = true;
                    }

                    mNewCenterRequestList.clear();

                    mGetCollectionConditionRequest = getGetCollectionConditionRequest();
                    if (mGetCollectionConditionRequest == nullptr)
                    {
                        mGetCollectionConditionRequest = std::make_shared<GetCollectionConditionRequest>();
                    }
                    UpdateResultCode urc {URC_SUCCESS};

                    // check center request
                    if (res->has_center_request_all_dtc_ssr())
                    {
                        ErrorInformation errorInfo{};
                        urc = verifyCenterRequestAllDtcSsr(errorInfo, res->center_request_all_dtc_ssr());
                        if ( urc == URC_SUCCESS)
                        {   
                            const std::shared_ptr<CenterRequestAllDtcSsr> centerRequestPayload {std::make_shared<CenterRequestAllDtcSsr>()};
                            centerRequestPayload->CopyFrom(res->center_request_all_dtc_ssr());
                            const ScheduleType aType {centerRequestPayload->schedule_information().schedule_type()};
                            // RDG30-R-1121
                            if ((aType != ScheduleType_PERIOD_TRIGGER_ROUTINE)
                                && (centerRequestPayload->schedule_information().has_schedule_interval() == true))
                            {
                                centerRequestPayload->mutable_schedule_information()->clear_schedule_interval();
                            }
                            
                            if (isCenterRequestJobExits(centerRequestPayload->collection_condition_id()) == false)
                            {
                                Rdg_Sched_Type::SchedType tempScheduleType {Rdg_Sched_Type::SchedType::ST_UNKNOWN};

                                if ((aType >= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MIN)
                                    && (aType <= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MAX))
                                {
                                    tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(aType);
                                }
                                const std::shared_ptr<CenterRequestJob> newJob 
                                {std::make_shared<CenterRequestJob>(centerRequestPayload->collection_condition_id()
                                                                                        , MSG_ID_CENTERREQUESTALLDTCSSR
                                                                                        , centerRequestPayload->schedule_information().priority()
                                                                                        , DiagTrigger::DiagTriggerType::CENTER_TRIGGER
                                                                                        , tempScheduleType
                                                                                        , centerRequestPayload)};
                                mNewCenterRequestList.push_back(centerRequestPayload->collection_condition_id());
                                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                                DiagManagerAdapter::getInstance()->storeCollectionConditionId(centerRequestPayload->collection_condition_id());
                                addCenterRequestJob(newJob);
                                LOG_D("Save CenterRequestAllDtcSsr data success");
                            } else {
                                finalUrc = URC_FAILED;
                                errorInfo.set_collection_condition_id(centerRequestPayload->collection_condition_id());
                                errorInfo.mutable_error_messages()->Add(INCORRECT_UPDATE_TYPE());
                                errInfor->Add()->CopyFrom(errorInfo);
                            }

                            isNeedNotify = true;

                        } else {
                            finalUrc = URC_FAILED;
                            errInfor->Add()->CopyFrom(errorInfo);
                        }
                    } else {
                        LOG_D("No AllDiag Center Request Information");
                    }

                    if (res->has_center_request_all_rob())
                    {
                        ErrorInformation errorInfo{};
                        urc = verifyCenterRequestAllRob(errorInfo, res->center_request_all_rob());
                        if ( urc == URC_SUCCESS)
                        {
                            const std::shared_ptr<CenterRequestAllRob> centerRequestPayload {std::make_shared<CenterRequestAllRob>()};
                            centerRequestPayload->CopyFrom(res->center_request_all_rob());
                            const ScheduleType aType {centerRequestPayload->schedule_information().schedule_type()};
                            // RDG30-R-1121
                            if ((aType != ScheduleType_PERIOD_TRIGGER_ROUTINE)
                                && (centerRequestPayload->schedule_information().has_schedule_interval() == true))
                            {
                                centerRequestPayload->mutable_schedule_information()->clear_schedule_interval();
                            }
                            
                            if (isCenterRequestJobExits(centerRequestPayload->collection_condition_id()) == false)
                            {
                                Rdg_Sched_Type::SchedType tempScheduleType {Rdg_Sched_Type::SchedType::ST_UNKNOWN};
                                if ((aType >= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MIN)
                                    && (aType <= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MAX))
                                {
                                    tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(aType);
                                }
                                const std::shared_ptr<CenterRequestJob> newJob 
                                {std::make_shared<CenterRequestJob>(centerRequestPayload->collection_condition_id()
                                                                                        , MSG_ID_CENTERREQUESTALLROB
                                                                                        , centerRequestPayload->schedule_information().priority()
                                                                                        , DiagTrigger::DiagTriggerType::CENTER_TRIGGER
                                                                                        , tempScheduleType
                                                                                        , centerRequestPayload)};
                                mNewCenterRequestList.push_back(centerRequestPayload->collection_condition_id());
                                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                                DiagManagerAdapter::getInstance()->storeCollectionConditionId(centerRequestPayload->collection_condition_id());
                                addCenterRequestJob(newJob);
                                LOG_D("Save CenterRequestAllRob data success");
                            } else {
                                finalUrc = URC_FAILED;
                                errorInfo.set_collection_condition_id(centerRequestPayload->collection_condition_id());
                                errorInfo.mutable_error_messages()->Add(INCORRECT_UPDATE_TYPE());
                                errInfor->Add()->CopyFrom(errorInfo);
                            }

                            isNeedNotify = true;
                        } else {
                            finalUrc = URC_FAILED;
                            errInfor->Add()->CopyFrom(errorInfo);
                        }
                    } else {
                        LOG_D("No AllRoB Center Request Information");
                    }
                    // RDG30-R-1117
                    if (res->has_center_request_ecu_information())
                    {
                        ErrorInformation errorInfo{};
                        urc = verifyCenterRequestEcuInformation(errorInfo, res->center_request_ecu_information());
                        if ( urc == URC_SUCCESS)
                        {
                            const std::shared_ptr<CenterRequestEcuInformation> centerRequestPayload {std::make_shared<CenterRequestEcuInformation>()};
                            centerRequestPayload->CopyFrom(res->center_request_ecu_information());
                            const ScheduleType aType {centerRequestPayload->schedule_information().schedule_type()};
                            // RDG30-R-1121
                            if ((aType != ScheduleType_PERIOD_TRIGGER_ROUTINE)
                                && (centerRequestPayload->schedule_information().has_schedule_interval() == true))
                            {
                                centerRequestPayload->mutable_schedule_information()->clear_schedule_interval();
                            }
                            
                            if (isCenterRequestJobExits(centerRequestPayload->collection_condition_id()) == false)
                            {
                                Rdg_Sched_Type::SchedType tempScheduleType {Rdg_Sched_Type::SchedType::ST_UNKNOWN};
                                if ((aType >= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MIN)
                                    && (aType <= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MAX))
                                {
                                    tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(aType);
                                }
                                const std::shared_ptr<CenterRequestJob> newJob 
                                {std::make_shared<CenterRequestJob>(centerRequestPayload->collection_condition_id()
                                                                                        , MSG_ID_CENTERREQUESTECUINFORMATION
                                                                                        , centerRequestPayload->schedule_information().priority()
                                                                                        , DiagTrigger::DiagTriggerType::CENTER_TRIGGER
                                                                                        , tempScheduleType
                                                                                        , centerRequestPayload)};
                                LOG_D("Save CenterRequestEcuInformation data success");
                                mNewCenterRequestList.push_back(centerRequestPayload->collection_condition_id());
                                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                                DiagManagerAdapter::getInstance()->storeCollectionConditionId(centerRequestPayload->collection_condition_id());
                                addCenterRequestJob(newJob);
                            } else {
                                finalUrc = URC_FAILED;
                                errorInfo.set_collection_condition_id(centerRequestPayload->collection_condition_id());
                                errorInfo.mutable_error_messages()->Add(INCORRECT_UPDATE_TYPE());
                                errInfor->Add()->CopyFrom(errorInfo);
                            }

                            isNeedNotify = true;
                        } else {
                            finalUrc = URC_FAILED;
                            errInfor->Add()->CopyFrom(errorInfo);
                        }
                    } else {
                        LOG_D("Center request information for ECU information acquisition request");
                    }

                    const CenterRequestRobSsrList& newCenterRequestsRobSsr {res->center_requests_rob_ssr()};

                    const CenterRequestDirectCommandList newCenterRequestDirectCommand {res->center_requests_direct_command()};

                    if (newCenterRequestsRobSsr.size() > 0)
                    {
                        urc = handleCenterRequestRobSsrs(*errInfor, newCenterRequestsRobSsr);
                        if ( urc == URC_FAILED)
                        {   
                            finalUrc = urc;
                        } else {
                            isNeedNotify = true;
                        }
                    } else {
                        LOG_D("Center request information for RoBSSR acquisition request");
                    }

                    if (newCenterRequestDirectCommand.size() > 0)
                    {
                        urc = handleCenterRequestDirectCommands(*errInfor, newCenterRequestDirectCommand);
                        if ( urc == URC_FAILED)
                        {  
                            finalUrc = urc;
                        } else {
                            isNeedNotify = true;
                        }
                    } else {
                        LOG_D("Center request information of DirectCommand");
                    }
                    // processing update collection condition
                    if (isAbortUpdateCollectionCondition == false) 
                    {
                        if ((newRdgFlag == true)) 
                        {
                            // RDG30-R-1104
                            if (res->has_collection_condition() && (res->collection_condition().ByteSizeLong() > 0U))
                            {
                                const CollectionConditionRes &collectionCond {res->collection_condition()};
                                const int32_t directCommandLenght {collectionCond.collection_conditions_direct_command_size()};
                                LOG_D("directCommandLenght %d", directCommandLenght);

                                if (collectionCond.has_collection_condition_diag_common())
                                {
                                    urc = handleCollectionConditionDiagCommonData(*errInfor, collectionCond.collection_condition_diag_common());
                                    if (urc == URC_FAILED)
                                    {
                                        finalUrc = urc;
                                    } else {
                                        isNeedNotify = true;
                                    }
                                }

                                if (collectionCond.has_collection_condition_rob_rob_ssr_did_event())
                                {
                                    urc = handleCollectionConditionRobRobSsrDidEvent(*errInfor, collectionCond.collection_condition_rob_rob_ssr_did_event());
                                    if (urc == URC_FAILED)
                                    {
                                        finalUrc = urc;
                                    } else {
                                        isNeedNotify = true;
                                    }
                                }

                                if (collectionCond.has_collection_condition_ecu_information())
                                {
                                    urc = handleCollectionConditionEcuInformation(*errInfor, collectionCond.collection_condition_ecu_information());
                                    if (urc == URC_FAILED)
                                    {
                                        finalUrc = urc;
                                    } else {
                                        isNeedNotify = true;
                                    }
                                }

                                if (collectionCond.has_collection_condition_warning_information())
                                {
                                    urc = handleCollectionConditionWarningInformation(*errInfor, collectionCond.collection_condition_warning_information());
                                    if (urc == URC_FAILED)
                                    {
                                        finalUrc = urc;
                                    } else {
                                        isNeedNotify = true;
                                    }
                                }

                                if (directCommandLenght > 0)
                                {
                                    const CollectionConditionDirectCommandList &newCollectionConditionDirectCommandList {collectionCond.collection_conditions_direct_command()};

                                    urc = handleCollectionConditionDirectCommands(*errInfor, newCollectionConditionDirectCommandList);
                                    if (urc == URC_FAILED)
                                    {
                                        finalUrc = urc;
                                    } else {
                                        isNeedNotify = true;
                                    }
                                }

                                (void)DataModel<GetCollectionConditionRequest>::saveData(GET_COLLECTION_CONDITION_REQ_DB_NAME, *mGetCollectionConditionRequest);
                            }
                            else
                            {
                                if (mTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
                                    finalUrc = URC_FAILED;
                                    errInfor->Add()->mutable_error_messages()->Add(NO_COLLECTION_CONDITION());
                                    LOG_I("No new collection condition data -> update process will not be executed");
                                }
                            }
                        } else {
                            isNeedNotify = true;
                            handleRdgActiveFlagOff();
                        }
                    }
                    (void)urc;
                    
                    if (isNeedNotify == true)
                    { 
                        mNewCenterRequests.push(mNewCenterRequestList);
                        (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_NEW_COLLECTION_CONDITION)->sendToTarget();
                    }
                    
                    mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT);
                    mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
                    mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
                    mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(CommonUtils::getGeodesyInfo());
                    mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT, UploadManager::getInstance()->getCounterMessage()));

                    mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
                    mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());
                    mNotifyCollectionConditionUpdateResultRequest->set_update_result_code(finalUrc);

                    // RDG30-R-1100
                    LOG_D("NotifyCollectionConditionUpdateResultRequest Created");
                    std::string str{};
                    (void)google::protobuf::util::MessageToJsonString(*mNotifyCollectionConditionUpdateResultRequest, &str, option);
                    printDataDebug(str);
                    LOG_D("============================================================================================");
                    (void)mCollectionConditionHandler->obtainMessage(CollectionCondition::CMD_SEND_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT_REQUEST)->sendToTarget();
                    (void)errInfor;
                } else {
                    LOG_E("Received GetCollectionConditionResponse null");
                }
                (void)finalUrc;
                (void)errInfor;
            }
            else {
                handleGrpcClientErrorEvent( pGrpcResData);
            }
        }
        else 
        {
            if ((responseCode >= grpc::StatusCode::OK) && (responseCode <= grpc::StatusCode::UNAUTHENTICATED))
            {
                handleGrpcClientErrorEvent( pGrpcResData);
            }
            else
            {
                LOG_E("Out of range, responseCode = %d", responseCode);
            } 
        }
    } 
    (void)isAbortUpdateCollectionCondition;
    (void)isNeedNotify;
}

UpdateResultCode CollectionCondition::verifyScheduleInformationForCentrerRequest(ErrorInformation &errorInfo, const ScheduleInformation &inputData) const
{
    UpdateResultCode urc{URC_SUCCESS};
    /* Priority
     * Min.           | Max.          | Invalid value           | Note
     * -              | -             | -                       | -
     * 0x00           | 0xFF          | 0xFF FF                 |
     */
    // RDG30-R-1118
    if (inputData.priority() > static_cast<std::uint8_t>(0xFF))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("priority"));
    }

    // RDG30-R-1119
    if ((inputData.schedule_type() <= ScheduleType_MIN) || (inputData.schedule_type() > ScheduleType_MAX))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add((RDG_INVALID_SCHEDULE_TYPE()));
    }

    return urc;
}

UpdateResultCode CollectionCondition::verifyScheduleInformationForCollectionCondition(ErrorInformation &errorInfo, const ScheduleInformation &inputData) const
{
    UpdateResultCode urc{URC_SUCCESS};
    /* Priority
     * Min.           | Max.          | Invalid value           | Note
     * -              | -             | -                       | -
     * 0x00           | 0xFF          | 0xFF FF                 |
     */
    // RDG30-R-1108
    if (inputData.priority() > static_cast<std::uint8_t>(0xFF))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("priority"));
    }

    // RDG30-R-1109
    if ((inputData.schedule_type() <= ScheduleType_MIN) || (inputData.schedule_type() > ScheduleType_MAX))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(RDG_INVALID_SCHEDULE_TYPE());
    }

    if (inputData.has_schedule_interval())
    {
        // RDG30-R-1111
        if (inputData.schedule_interval().schedule_interval_days() > SCHEDULE_INTERVAL_DAYS_MAX)
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("schedule_interval_days"));
        }

        // RDG30-R-1111
        if (inputData.schedule_interval().schedule_interval_hours() > SCHEDULE_INTERVAL_HOURS_MAX)
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("schedule_interval_hours"));
        }

        // RDG30-R-1111
        if (inputData.schedule_interval().schedule_interval_minutes() > SCHEDULE_INTERVAL_MINUTES_MAX)
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("schedule_interval_minutes"));
        }
    }
    else
    {
        // RDG30-R-1112
        if (inputData.schedule_type() == ScheduleType_PERIOD_TRIGGER_ROUTINE)
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(NO_REQUIRED_SETTING_VALUES("schedule_interval"));
        }
    }

    return urc;
}

UpdateResultCode CollectionCondition::verifyEcuAddressInformation(ErrorInformation &errorInfo, const EcuAddressInformation &inputData) const
{
    UpdateResultCode urc{URC_SUCCESS};
    const CommunicationProtocol commProtocol {inputData.communication_protocol()};
    const CommunicationType commType {inputData.communication_type()};
    const Uint32 targetAddress{inputData.target_address()};

    if ((commProtocol < RdgProtoInterface::EcuAddressInformation_CommunicationProtocol_CommunicationProtocol_MIN) || (commProtocol > RdgProtoInterface::EcuAddressInformation_CommunicationProtocol_CommunicationProtocol_MAX))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("communication_protocol"));
    }

    if ((commType < RdgProtoInterface::EcuAddressInformation_CommunicationType_CommunicationType_MIN) || (commType > RdgProtoInterface::EcuAddressInformation_CommunicationType_CommunicationType_MAX))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("communication_type"));
    }
    /*
     * Min. / 最小値  | Max. / 最大値 | Invalid value / 無効値  | Note / 備考
     * -              | -             | -                       | -
     * 0x00 00 00 00  | 0x1F FF FF FF | 0xFF FF FF FF           |
     */
    if (targetAddress > static_cast<Uint32>(0x1FFFFFFF))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("target_address"));
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCenterRequestAllDtcSsr(ErrorInformation &errorInfo, const CenterRequestAllDtcSsr &inputData)
{
    UpdateResultCode urc{URC_SUCCESS};

    if (inputData.collection_condition_id() == 0U)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));
    } 
    else if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        LOG_E("collection_condition_id invalid");
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    } else {
        // do nothing
    }

    if (inputData.has_schedule_information() == true)
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCentrerRequest(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        } else {
            // RDG30-R-1120
            const ScheduleType scheType {inputData.schedule_information().schedule_type()};
            if (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT) 
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(RDG_INVALID_SCHEDULE_TYPE());
            }
        }
    } else {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(NO_REQUIRED_SETTING_VALUES("schedule_information"));
    }

    if (urc == URC_FAILED)
    {
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        DiagManagerAdapter::getInstance()->selfDiagCollectionCondition(inputData.collection_condition_id());
    } else {
        LOG_D("verifyCenterRequestAllDtcSsr PASS");
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCenterRequestAllRob(ErrorInformation &errorInfo, const CenterRequestAllRob &inputData)
{
    UpdateResultCode urc{URC_SUCCESS};
    // RDG30-R-1117

    if (inputData.collection_condition_id() == 0U)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));
    } 
    else if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        LOG_E("collection_condition_id invalid");
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }
    else {
        // do nothing
    }

    if (inputData.has_schedule_information())
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCentrerRequest(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        } else {
            // RDG30-R-1120
            const ScheduleType scheType {inputData.schedule_information().schedule_type()};
            if (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT) 
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(RDG_INVALID_SCHEDULE_TYPE());
            }
        }
    } else {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(NO_REQUIRED_SETTING_VALUES("schedule_information"));
    }

    if (urc == URC_FAILED)
    {
        LOG_D("verifyCenterRequestAllRob FAILED");
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        DiagManagerAdapter::getInstance()->selfDiagCollectionCondition(inputData.collection_condition_id());
    }
    else
    {
        LOG_D("verifyCenterRequestAllRob PASS");
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCenterRequestRobSsr(ErrorInformation &errorInfo, const CenterRequestRobSsr &inputData)
{
    UpdateResultCode urc{URC_SUCCESS};
    // RDG30-R-1117
    if (inputData.collection_condition_id() == 0U)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));
    } 
    else if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }
    else {
        // do nothing
    }

    if (inputData.has_schedule_information() == true)
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCentrerRequest(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        } else {
            // RDG30-R-1120
            const ScheduleType scheType {inputData.schedule_information().schedule_type()};
            if (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT) 
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(RDG_INVALID_SCHEDULE_TYPE());
            }
        }
    } else {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(NO_REQUIRED_SETTING_VALUES("schedule_information"));
    }

    // RDG30-R-1125
    if (inputData.has_target_collection_data() == false)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(NO_REQUIRED_SETTING_VALUES("target_collection_data"));
    }

    if (urc == URC_FAILED)
    {
        LOG_D("verifyCenterRequestRobSsr FAILED");
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        DiagManagerAdapter::getInstance()->selfDiagCollectionCondition(inputData.collection_condition_id());
    }
    else
    {
        LOG_D("verifyCenterRequestRobSsr PASS");
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCenterRequestRobSsrs(ErrorInformationList &errorInfoList, const CenterRequestRobSsrList &inputDataList)
{
    UpdateResultCode urc{URC_SUCCESS};
    // RDG30-R-1124
    if (inputDataList.size() > NUMBER_RoBSSR_ACQUISITION_MAX)
    {
        ErrorInformation errorInfo{};
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(TOO_MANY_COLLECTION_CONDITIOS("center_requests_rob_ssr"));
        errorInfoList.Add()->CopyFrom(errorInfo);
    }
    else
    {
        for (CenterRequestRobSsrIter it {inputDataList.begin()}; it != inputDataList.end(); it++)
        {
            ErrorInformation errorInfo{};
            urc = verifyCenterRequestRobSsr(errorInfo, *it);
            if (urc == URC_FAILED)
            {
                errorInfoList.Add()->CopyFrom(errorInfo);
            }
        }
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCollectionConditionDirectCommand(ErrorInformation &errorInfo, const CollectionConditionDirectCommand &inputData)
{
    UpdateResultCode urc{URC_SUCCESS};
  

    if (inputData.update_type_collection_condition() == UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_NO_CHANGED) {
        // Do nothing
    }
    else if ((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_DELETED)) {
        // Do nothing
    }
    else if ((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_UNKNOWN)) {
        // Do nothing
    }
    else if((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_ADDED))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));
    }
    else if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU) {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }
    else {
        if ((inputData.update_type_collection_condition() <= UpdateTypeMultiple_MIN) || (inputData.update_type_collection_condition() > UpdateTypeMultiple_MAX))
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("update_type_collection_condition"));
        }
        else if (inputData.update_type_collection_condition() == UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_DELETED)
        {
                if (checkExitsCollectionConditionDirectCommand(inputData.collection_condition_id()) == false)
                {
                    urc = URC_FAILED;
                    errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));  
                }
        }
        else {
            if (inputData.has_schedule_information() == true)
            {
                const UpdateResultCode tempUrc {verifyScheduleInformationForCollectionCondition(errorInfo, inputData.schedule_information())};
                if (tempUrc == URC_FAILED)
                {
                    urc = tempUrc;
                } else {
                    // RDG30-R-1120
                    const ScheduleType scheType {inputData.schedule_information().schedule_type()};
                    if ((scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IG_ON_TRIGGER_ROUTINE)
                        && (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IG_OFF_TRIGGER_ROUTINE)
                        && (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_PERIOD_TRIGGER_ROUTINE)) 
                    {
                        urc = URC_FAILED;
                        errorInfo.mutable_error_messages()->Add(RDG_INVALID_SCHEDULE_TYPE());
                    }
                }
            } else {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(NO_REQUIRED_SETTING_VALUES("schedule_information"));
            }


            // RDG30-R-1114
            if (inputData.direct_commands().size() > NUMBER_DIRECT_COMMAND_MAX)
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(TOO_MANY_REPEATED_SETTING_VALUES("direct_commands"));
            }
        }
    }


    if (urc == URC_FAILED)
    {
        LOG_D("verifyCollectionConditionDirectCommand FAILED");
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        DiagManagerAdapter::getInstance()->selfDiagCollectionCondition(inputData.collection_condition_id());
    }
    else
    {
        LOG_D("verifyCollectionConditionDirectCommand PASS");
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCenterRequestEcuInformation(ErrorInformation &errorInfo, const CenterRequestEcuInformation &inputData)
{
    UpdateResultCode urc{URC_SUCCESS};
    if (inputData.collection_condition_id() == 0U)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));
    } 
    else if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }
    else {
        // do nothing
    }

    if (inputData.has_schedule_information() == true)
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCentrerRequest(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        } else {
            // RDG30-R-1120
            const ScheduleType scheType {inputData.schedule_information().schedule_type()};
            if (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT) 
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(RDG_INVALID_SCHEDULE_TYPE());
            }
        }
    } else {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(NO_REQUIRED_SETTING_VALUES("schedule_information"));
    }

    if (urc == URC_FAILED)
    {
        LOG_D("verifyCenterRequestEcuInformation FAILED");
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        DiagManagerAdapter::getInstance()->selfDiagCollectionCondition(inputData.collection_condition_id());
    }
    else
    {
        LOG_D("verifyCenterRequestEcuInformation PASS");
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCenterRequestDirectCommand(ErrorInformation &errorInfo, const CenterRequestDirectCommand &inputData)
{
    UpdateResultCode urc{URC_SUCCESS};
    // RDG30-R-1117
    if (inputData.collection_condition_id() == 0U)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));
    } 
    else if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }
    else {
        // do nothing
    }

    if (inputData.has_schedule_information() == true)
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCentrerRequest(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        } else {
            // RDG30-R-1120
            const ScheduleType scheType {inputData.schedule_information().schedule_type()};
            if ((scheType < ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT) 
                || (scheType > ScheduleType::ScheduleInformation_ScheduleType_ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT)) 
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(RDG_INVALID_SCHEDULE_TYPE());
            }
        }
    } else {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(NO_REQUIRED_SETTING_VALUES("schedule_information"));
    }

    // RDG30-R-1123
    if (inputData.direct_commands().size() > NUMBER_DIRECT_COMMAND_MAX)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(TOO_MANY_REPEATED_SETTING_VALUES("direct_commands"));
    }

    if (urc == URC_FAILED)
    {
        LOG_D("verifyCenterRequestDirectCommand FAILED");
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
    }
    else
    {
        LOG_D("verifyCenterRequestDirectCommand PASS");
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCollectionConditionDiagCommon(ErrorInformation &errorInfo, const CollectionConditionDiagCommon &inputData) const
{
    UpdateResultCode urc{URC_SUCCESS};
 

    if (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_NO_CHANGED) {
        // Do nothing
    }
    else if ((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)) {
        // Do nothing
    }
    else if ((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_UNKNOWN)) {
        // Do nothing
    }
    else if((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));
    } 
    else if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU) {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }
    else {
        // RDG30-R-1107
        if ((inputData.update_type_collection_condition() <= UpdateTypeSingle_MIN) || (inputData.update_type_collection_condition() > UpdateTypeSingle_MAX))
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("update_type_collection_condition"));
        }
        else {
            // Do nothing
        }
    }

    if (urc == URC_FAILED)
    {
        LOG_D("verifyCollectionConditionDiagCommon FAILED");
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        DiagManagerAdapter::getInstance()->selfDiagCollectionCondition(inputData.collection_condition_id());
    }
    else
    {
        LOG_D("verifyCollectionConditionDiagCommon PASS");
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCollectionConditionRobRobSsrDidEvent(ErrorInformation &errorInfo, const CollectionConditionRobRobSsrDidEvent &inputData)
{
    UpdateResultCode urc{URC_SUCCESS};

    if (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_NO_CHANGED) {
        // Do nothing
    }
    else if ((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)) {
        // Do nothing
    }
    else if ((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_UNKNOWN)) {
        // Do nothing
    }
    else if((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));
    } 
    else if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU) {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    } else {
        if ((inputData.update_type_collection_condition() <= UpdateTypeSingle_MIN) || (inputData.update_type_collection_condition() > UpdateTypeSingle_MAX))
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("update_type_collection_condition"));
        }
        else if (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)
        {
            // Do nothing
        }
        else {
            if (inputData.has_schedule_information() == true)
            {
                const UpdateResultCode tempUrc {verifyScheduleInformationForCollectionCondition(errorInfo, inputData.schedule_information())};
                if (tempUrc == URC_FAILED)
                {
                    urc = tempUrc;
                } else {
                    // RDG30-R-1120
                    const ScheduleType scheType {inputData.schedule_information().schedule_type()};
                    if ((scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT)
                        && (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IG_ON_TRIGGER_ROUTINE)
                        && (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_PERIOD_TRIGGER_ROUTINE)) 
                    {
                        urc = URC_FAILED;
                        errorInfo.mutable_error_messages()->Add(RDG_INVALID_SCHEDULE_TYPE());
                    }
                }
            } else {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(NO_REQUIRED_SETTING_VALUES("schedule_information"));
            }

            // RDG30-R-1116
            if (inputData.target_collection_data().size() > NUMBER_OF_TARGET_COLLECTION_DATA_MAX)
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(TOO_MANY_REPEATED_SETTING_VALUES("target_collection_data"));
            }
        }
    }


    if (urc == URC_FAILED)
    {
        LOG_D("verifyCollectionConditionRobRobSsrDidEvent FAILED");
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        DiagManagerAdapter::getInstance()->selfDiagCollectionCondition(inputData.collection_condition_id());
    }
    else
    {
        LOG_D("verifyCollectionConditionRobRobSsrDidEvent PASS");
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCollectionConditionEcuInformation(ErrorInformation &errorInfo, const CollectionConditionEcuInformation &inputData)
{
    UpdateResultCode urc{URC_SUCCESS};
 
    if (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_NO_CHANGED) {
        // Do nothing
    }
    else if ((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)) {
        // Do nothing
    }
    else if ((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_UNKNOWN)) {
        // Do nothing
    }
    else if((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));
    }
    else if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU) {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    } else { 
        // RDG30-R-1107
        if ((inputData.update_type_collection_condition() <= UpdateTypeSingle_MIN) || (inputData.update_type_collection_condition() > UpdateTypeSingle_MAX))
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("update_type_collection_condition"));
        }
        else if (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)
        {
            // Do nothing
        }
        else {
            if (inputData.has_schedule_information())
            {
                const UpdateResultCode tempUrc {verifyScheduleInformationForCollectionCondition(errorInfo, inputData.schedule_information())};
                if (tempUrc == URC_FAILED)
                {
                    urc = tempUrc;
                } else {
                    // RDG30-R-1120
                    const ScheduleType scheType {inputData.schedule_information().schedule_type()};
                    if (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_PERIOD_TRIGGER_ROUTINE) 
                    {
                        urc = URC_FAILED;
                        errorInfo.mutable_error_messages()->Add(RDG_INVALID_SCHEDULE_TYPE());
                    }
                }
            } else {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(NO_REQUIRED_SETTING_VALUES("schedule_information"));
            }
        }
    }

    if (urc == URC_FAILED)
    {
        LOG_D("verifyCollectionConditionEcuInformation FAILED");
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        DiagManagerAdapter::getInstance()->selfDiagCollectionCondition(inputData.collection_condition_id());
    }
    else
    {
        LOG_D("verifyCollectionConditionEcuInformation PASS");
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCollectionConditionWarningInformation(ErrorInformation &errorInfo, const CollectionConditionWarningInformation &inputData) const
{
    UpdateResultCode urc{URC_SUCCESS};

    if (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_NO_CHANGED) {
        // Do nothing
    }
    else if ((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)) {
        // Do nothing
    }
    else if ((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_UNKNOWN)) {
        // Do nothing
    }
    else if((inputData.collection_condition_id() == 0U) && (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));
    }
    else if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU) {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    } else {
        // RDG30-R-1107
        if ((inputData.update_type_collection_condition() <= UpdateTypeSingle_MIN) || (inputData.update_type_collection_condition() > UpdateTypeSingle_MAX))
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("update_type_collection_condition"));
        }
        else if (inputData.update_type_collection_condition() == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)
        {
            // Do nothing
        }
        else {
            // RDG30-R-1280
            if (inputData.warning_property_table().size() != WARNING_PROPERTY_TABLE_SIZE_MAX)
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("warning_property_table"));
            }
        }
    }

    if (urc == URC_FAILED)
    {
        LOG_D("verifyCollectionConditionWarningInformation FAILED");
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        DiagManagerAdapter::getInstance()->selfDiagCollectionCondition(inputData.collection_condition_id());
    }
    else
    {
        LOG_D("verifyCollectionConditionWarningInformation PASS");
    }
    return urc;
}

void CollectionCondition::testPrintCollectionConditionData(void)
{
    google::protobuf::util::JsonOptions option{};
    option.always_print_primitive_fields = true;
    option.preserve_proto_field_names = true;
    LOG_I("======= PRINT COLLECTION CONDITION DATA=======");
    LOG_I("rdg_active_flag = %d", DiagManagerAdapter::getInstance()->getRDGFlag());
    LOG_I("==============================================");
    LOG_I("GetCollectionConditionRequest:");
    const std::shared_ptr<GetCollectionConditionRequest> collectionReq {getGetCollectionConditionRequest()};
    if (collectionReq != nullptr)
    {
        std::string collectionReqStr{};
        (void)google::protobuf::util::MessageToJsonString(*collectionReq, &collectionReqStr, option);
        printData(collectionReqStr);
    }
    else
    {
        LOG_I("Empty");
    }
    LOG_I("==============================================");
    LOG_I("collection_condition_diag_common:");
    const std::shared_ptr<CollectionConditionDiagCommon> diagCommonObj {this->getCollectionConditionDiagCommon()};
    if (diagCommonObj != nullptr)
    {
        std::string ccDiagCommon{};
        (void)google::protobuf::util::MessageToJsonString(*diagCommonObj, &ccDiagCommon, option);
        printData(ccDiagCommon);
    }
    else
    {
        LOG_I("Empty");
    }
    LOG_I("==============================================");
    LOG_I("collection_conditions_direct_command:");
    if ((collectionReq != nullptr) && collectionReq->has_collection_condition_id_stored_in_vehicle()) {
        const Uint64List& ccCDirectCommands {collectionReq->collection_condition_id_stored_in_vehicle().direct_command_collection_condition_ids()};
        if (ccCDirectCommands.size() > 0)
        {
            for (ConstUint64Iter it {ccCDirectCommands.cbegin()}; it != ccCDirectCommands.cend(); it++)
            {
                LOG_I("collection_conditions_direct_command id = %ld", *it);
                const std::shared_ptr<CollectionConditionDirectCommand> directCmd {this->getCollectionConditionDirectCommand(*it)};
                if (directCmd != nullptr)
                {
                    std::string directCommandStr{};
                    (void)google::protobuf::util::MessageToJsonString(*directCmd, &directCommandStr, option);
                    printData(directCommandStr);
                }
            }
        }
        else
        {
            LOG_I("Empty");
        }
        LOG_I("==============================================");
        LOG_I("CollectionConditionRobRobSsrDidEvent:");
        const std::shared_ptr<CollectionConditionRobRobSsrDidEvent> robRobSsrDidEvent {getCollectionConditionRobRobSsrDidEvent()};
        if (robRobSsrDidEvent != nullptr)
        {
            std::string robRobSsrDidEventStr{};
            (void)google::protobuf::util::MessageToJsonString(*robRobSsrDidEvent, &robRobSsrDidEventStr, option);
            printData(robRobSsrDidEventStr);
        }
        else
        {
            LOG_I("Empty");
        }
        LOG_I("==============================================");
        LOG_I("CollectionConditionEcuInformation:");
        const std::shared_ptr<CollectionConditionEcuInformation> ecuInformation {getCollectionConditionEcuInformation()};
        if (ecuInformation != nullptr)
        {
            std::string ecuInformationStr{};
            (void)google::protobuf::util::MessageToJsonString(*ecuInformation, &ecuInformationStr, option);
            printData(ecuInformationStr);
        }
        else
        {
            LOG_I("Empty");
        }
        LOG_I("==============================================");
        LOG_I("CollectionConditionWarningInformation:");
        const std::shared_ptr<CollectionConditionWarningInformation> warnInformation {getCollectionConditionWarningInformation()};
        if (warnInformation != nullptr)
        {
            std::string warnInformationStr{};
            (void)google::protobuf::util::MessageToJsonString(*warnInformation, &warnInformationStr, option);
            printData(warnInformationStr);
        }
        else
        {
            LOG_I("Empty");
        }
    }
    LOG_I("==================== Center Request data ==========================");
    if (mCenterRequestJobList.empty())
    {
        LOG_I("Empty");
    } else {
        std::unordered_map<uint64_t, std::shared_ptr<CenterRequestJob>>::iterator jobIter {mCenterRequestJobList.begin()};
        while(jobIter != mCenterRequestJobList.end())
        {
            const std::shared_ptr<google::protobuf::Message> aCenterRequestData {jobIter->second->getPayload()};
            if (aCenterRequestData != nullptr)
            {
                std::string aCenterRequestJsonString{};
                (void)google::protobuf::util::MessageToJsonString(*aCenterRequestData, &aCenterRequestJsonString, option);
                printData(aCenterRequestJsonString);
            }
            jobIter++;
        }
    }

    LOG_I("==================== End ==========================");
}

void CollectionCondition::testReceivedCollectionConditionResponse() const
{
    const std::shared_ptr<GetCollectionConditionResponse> rootRes {std::make_shared<GetCollectionConditionResponse>()};
    /* Get file size*/
    uint32_t size{0U};
    ifstream file{GET_COLLECTION_CONDITION_RES_TEST_PATH.c_str(), (ios::binary | ios::ate)};
    const int64_t tmpFileSize {file.tellg()};
    size = (tmpFileSize > 0) ? static_cast<uint32_t>(tmpFileSize) : 0U;
    file.close();
    LOG_I("CHECK size: %d", size);
    std::vector<uint8_t> raw(size);
    const FileHandleType handle {FileUtil::openFile(GET_COLLECTION_CONDITION_RES_TEST_PATH.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN)};
    if (handle != nullptr)
    {
        (void)FileUtil::ReadBinFromFile(handle, raw.data(), raw.size());
        (void)FileUtil::closeFile(handle);
        const std::string inputStrPiece{reinterpret_cast<char_t*>(raw.data()), raw.size()};
        LOG_I("get json string = %s", inputStrPiece.c_str());

        google::protobuf::util::JsonParseOptions option{};
        option.ignore_unknown_fields = true;
        const google::protobuf::util::Status status {google::protobuf::util::JsonStringToMessage(inputStrPiece, rootRes.get(), option)};

        if (status.ok())
        {
            android::sp<GrpcResData> getCollectionCondRes{nullptr};
            try
            {
                getCollectionCondRes = new GrpcResData(GRPC_APP_TYPE::RMT_DIAG, GRPC_IF_TYPE::DCIF_RDG010, 5000, GRPC_RESULT::SEND_SUCCESS, grpc::StatusCode::OK, std::string("testReceivedCollectionConditionResponse"), rootRes.get());
            }
            catch (std::invalid_argument& e)
            {
                LOG_E("exception: %s", e.what());
            }
            HttpManagerAdapter::getInstance()->testTriggerReceive(getCollectionCondRes);
        }
        else
        {
            LOG_E("Converting to gRPC message failed, Invalid input json data");
        }
    }
    else
    {
        LOG_E("open file %s failed", GET_COLLECTION_CONDITION_RES_TEST_PATH.c_str());
    }
}

void CollectionCondition::testReceivedCollectionConditionResponseBinData() const
{
    /* Get file size*/
    GetCollectionConditionResponse rootRes {};
    if (FileUtil::isPathExist(GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH.c_str()) == true) 
    {
        int32_t fd {-1};
        const FileHandleType handle {FileUtil::openFile(GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN)};
        if(handle != nullptr)
        {
            fd = FileUtil::getFileDescriptor(handle);
        }

        if(fd == -1)
        {
            LOG_E("get data failed because can't open file %s", GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH.c_str());
        }
        else
        {
            LOG_D("Get %s, File Descriptor = %d", GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH.c_str(), fd);
            const bool parseResult {rootRes.ParseFromFileDescriptor(fd)};
            if(parseResult == false)
            {
                LOG_E("fail to parse file");
            }

            const bool closeResult {FileUtil::closeFile(handle)};
            if(closeResult == false)
            {
                LOG_E("fail to close file");
            }

            if (parseResult)
            {
                android::sp<GrpcResData> getCollectionCondRes{nullptr};
                try
                {
                    getCollectionCondRes = new GrpcResData(GRPC_APP_TYPE::RMT_DIAG, GRPC_IF_TYPE::DCIF_RDG010, 5000, GRPC_RESULT::SEND_SUCCESS, grpc::StatusCode::OK, std::string("testReceivedCollectionConditionResponse"), &rootRes);
                }
                catch (std::invalid_argument& e)
                {
                    LOG_E("exception: %s", e.what());
                }
                HttpManagerAdapter::getInstance()->testTriggerReceive(getCollectionCondRes);
            }
            else
            {
                LOG_E("Converting to gRPC message failed, Invalid input data");
            }
        }
    } else {
        LOG_E("file %s not exist", GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH.c_str());
    }
}

error_t CollectionCondition::saveCollectionConditionDiagCommon(const CollectionConditionDiagCommon &obj)
{
    const error_t ret {DataModel<CollectionConditionDiagCommon>::saveData(COLLECTION_CONDITION_DIAG_COMMON_DB_NAME, obj)};
    if (ret == E_OK)
    {
        mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->set_common_diag_collection_condition_id(obj.collection_condition_id());
    }
    return ret;
}

std::shared_ptr<CollectionConditionDiagCommon> CollectionCondition::getCollectionConditionDiagCommon(void) const noexcept
{
    return DataModel<CollectionConditionDiagCommon>::getData(COLLECTION_CONDITION_DIAG_COMMON_DB_NAME);
}

error_t CollectionCondition::saveCollectionConditionRobRobSsrDidEvent(const CollectionConditionRobRobSsrDidEvent &obj)
{
    const error_t ret {DataModel<CollectionConditionRobRobSsrDidEvent>::saveData(COLLECTION_CONDITION_ROB_SSR_DID_DB_NAME, obj)};
    if (ret == E_OK)
    {
        mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->set_rob_rob_ssr_did_event_collection_condition_id(obj.collection_condition_id());
    }
    return ret;
}

std::shared_ptr<CollectionConditionRobRobSsrDidEvent> CollectionCondition::getCollectionConditionRobRobSsrDidEvent(void) const noexcept
{
    return DataModel<CollectionConditionRobRobSsrDidEvent>::getData(COLLECTION_CONDITION_ROB_SSR_DID_DB_NAME);
}

error_t
CollectionCondition::saveCollectionConditionEcuInformation(const CollectionConditionEcuInformation &obj)
{
    const error_t ret {DataModel<CollectionConditionEcuInformation>::saveData(COLLECTION_CONDITION_ECU_INFO_DB_NAME, obj)};
    if (ret == E_OK)
    {
        mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->set_ecu_information_collection_condition_id(obj.collection_condition_id());
    }
    return ret;
}

std::shared_ptr<CollectionConditionEcuInformation> CollectionCondition::getCollectionConditionEcuInformation(void) const noexcept
{
    return DataModel<CollectionConditionEcuInformation>::getData(COLLECTION_CONDITION_ECU_INFO_DB_NAME);
}

error_t
CollectionCondition::saveCollectionConditionWarningInformation(const CollectionConditionWarningInformation &obj)
{
    const error_t ret {DataModel<CollectionConditionWarningInformation>::saveData(COLLECTION_CONDITION_WARN_INFO_DB_NAME, obj)};
    if (ret == E_OK)
    {
        mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->set_warning_information_collection_condition_id(obj.collection_condition_id());
    }
    return ret;
}

std::shared_ptr<CollectionConditionWarningInformation> CollectionCondition::getCollectionConditionWarningInformation(void) const noexcept
{
    return DataModel<CollectionConditionWarningInformation>::getData(COLLECTION_CONDITION_WARN_INFO_DB_NAME);
}

error_t CollectionCondition::saveCollectionConditionDirectCommand(const CollectionConditionDirectCommand &obj) const
{
    const uint64_t collectionConditionId {obj.collection_condition_id()};
    const std::string collectionConditionIdStr {std::to_string(collectionConditionId)};
    std::string path {"CollectionConditionDirectCommand_"};
    (void)path.append(collectionConditionIdStr);
    return DataModel<CollectionConditionDirectCommand>::saveData(path, obj);
}

std::shared_ptr<CollectionConditionDirectCommand> CollectionCondition::getCollectionConditionDirectCommand(const uint64_t collectionConditionId) const
{
    const std::string collectionConditionIdStr {std::to_string(collectionConditionId)};
    std::string path {"CollectionConditionDirectCommand_"};
    (void)path.append(collectionConditionIdStr);
    return DataModel<CollectionConditionDirectCommand>::getData(path);
}

error_t CollectionCondition::deleteCollectionConditionDirectCommand(const uint64_t collectionConditionId) const
{
    const std::string collectionConditionIdStr {std::to_string(collectionConditionId)};
    std::string path {DATA_PATH};
    (void)path.append("CollectionConditionDirectCommand_");
    (void)path.append(collectionConditionIdStr);
    const bool ret {FileUtil::removeFile(path)};
    error_t tmpResult {E_OK};
    if (!ret)
    {
        tmpResult = E_ERROR;
    }
    return tmpResult;
}

bool CollectionCondition::checkExitsCollectionConditionDirectCommand(const uint64_t collectionConditionId) const
{
    const std::string collectionConditionIdStr {std::to_string(collectionConditionId)};
    std::string path {"CollectionConditionDirectCommand_"};
    (void)path.append(collectionConditionIdStr);
    return DataModel<CollectionConditionDirectCommand>::checkExist(path);
}

std::shared_ptr<GetCollectionConditionRequest> CollectionCondition::getGetCollectionConditionRequest() const noexcept
{
    return DataModel<GetCollectionConditionRequest>::getData(GET_COLLECTION_CONDITION_REQ_DB_NAME);
}

UpdateResultCode CollectionCondition::handleCenterRequestRobSsrs(ErrorInformationList &errorInfoList, const CenterRequestRobSsrList &inputDataList)
{
    UpdateResultCode urc {URC_SUCCESS};
    // RDG30-R-1124
    if (inputDataList.size() > NUMBER_RoBSSR_ACQUISITION_MAX)
    {
        ErrorInformation errorInfo{};
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(TOO_MANY_COLLECTION_CONDITIOS("center_requests_rob_ssr"));
        errorInfoList.Add()->CopyFrom(errorInfo);
    } else {
        for (CenterRequestRobSsrIter it {inputDataList.begin()}; it != inputDataList.end(); it++)
        {
            ErrorInformation errorInfo{};
            const UpdateResultCode tempUrc {verifyCenterRequestRobSsr(errorInfo, *it)};
            if (tempUrc == URC_FAILED)
            {
                urc = URC_FAILED;
                errorInfoList.Add()->CopyFrom(errorInfo);
                (void)tempUrc;
            }
            else
            {
                const std::shared_ptr<CenterRequestRobSsr> centerRequestPayload { std::make_shared<CenterRequestRobSsr>()};
                centerRequestPayload->CopyFrom(*it);
                const ScheduleType aType {centerRequestPayload->schedule_information().schedule_type()};
                // RDG30-R-1282
                if ((aType != ScheduleType_PERIOD_TRIGGER_ROUTINE) && (centerRequestPayload->schedule_information().has_schedule_interval() == true))
                {
                    LOG_D("schedule_type is not “Periodic Trigger Routine” but the schedule interval (schedule_interval) is specified");
                    centerRequestPayload->mutable_schedule_information()->clear_schedule_interval();
                    LOG_D("Discard schedule_interval success");
                }
                
                if (isCenterRequestJobExits(centerRequestPayload->collection_condition_id()) == false)
                {
                    Rdg_Sched_Type::SchedType tempScheduleType {Rdg_Sched_Type::SchedType::ST_UNKNOWN};
                    if ((aType >= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MIN)
                        && (aType <= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MAX))
                    {
                        tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(aType);
                    }
                    const std::shared_ptr<CenterRequestJob> newJob 
                    {std::make_shared<CenterRequestJob>(centerRequestPayload->collection_condition_id()
                                                                            , MSG_ID_CENTERREQUESTROBSSR
                                                                            , centerRequestPayload->schedule_information().priority()
                                                                            , DiagTrigger::DiagTriggerType::CENTER_TRIGGER
                                                                            , tempScheduleType
                                                                            , centerRequestPayload)};
                    LOG_D("Save CenterRequestRobSsr data success");
                    mNewCenterRequestList.push_back(centerRequestPayload->collection_condition_id());
                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                    DiagManagerAdapter::getInstance()->storeCollectionConditionId(centerRequestPayload->collection_condition_id());
                    addCenterRequestJob(newJob);
                } else {
                    urc = URC_FAILED;
                    errorInfo.set_collection_condition_id(centerRequestPayload->collection_condition_id());
                    errorInfo.mutable_error_messages()->Add(INCORRECT_UPDATE_TYPE());
                    errorInfoList.Add()->CopyFrom(errorInfo);
                }
            }
        }
    }
    return urc;
}
UpdateResultCode CollectionCondition::handleCenterRequestDirectCommands(ErrorInformationList &errorInfoList, const CenterRequestDirectCommandList &inputDataList)
{
    UpdateResultCode urc {URC_SUCCESS};
    // RDG30-R-1122
    if (inputDataList.size() > NUMBER_CENTER_REQUEST_DIRECT_COMMAND_MAX)
    {
        ErrorInformation errorInfo{};
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(TOO_MANY_COLLECTION_CONDITIOS("center_requests_direct_command"));
        errorInfoList.Add()->CopyFrom(errorInfo);
    } else {

        for (CenterRequestDirectCommandIter it {inputDataList.begin()}; it != inputDataList.end(); it++)
        {
            ErrorInformation errorInfo{};
            const UpdateResultCode tempUrc {verifyCenterRequestDirectCommand(errorInfo, *it)};
            if (tempUrc == URC_FAILED)
            {
                urc = URC_FAILED;
                errorInfoList.Add()->CopyFrom(errorInfo);
                (void)tempUrc;
            }
            else
            {
                const std::shared_ptr<CenterRequestDirectCommand> centerRequestPayload { std::make_shared<CenterRequestDirectCommand>()};
                centerRequestPayload->CopyFrom(*it);
                const ScheduleType aType {centerRequestPayload->schedule_information().schedule_type()};
                // RDG30-R-1121
                if ((aType != ScheduleType_PERIOD_TRIGGER_ROUTINE) && (centerRequestPayload->schedule_information().has_schedule_interval() == true))
                {
                    LOG_D("schedule_type is not “Periodic Trigger Routine” but the schedule interval (schedule_interval) is specified");
                    centerRequestPayload->mutable_schedule_information()->clear_schedule_interval();
                    LOG_D("Discard schedule_interval success");
                }
                
                if (isCenterRequestJobExits(centerRequestPayload->collection_condition_id()) == false)
                {
                    Rdg_Sched_Type::SchedType tempScheduleType {Rdg_Sched_Type::SchedType::ST_UNKNOWN};
                    if ((aType >= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MIN)
                        && (aType <= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MAX))
                    {
                        tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(aType);
                    }
                    const std::shared_ptr<CenterRequestJob> newJob 
                    {std::make_shared<CenterRequestJob>(centerRequestPayload->collection_condition_id()
                                                                            , MSG_ID_CENTERREQUESTDIRECTCOMMAND
                                                                            , centerRequestPayload->schedule_information().priority()
                                                                            , DiagTrigger::DiagTriggerType::CENTER_TRIGGER
                                                                            , tempScheduleType
                                                                            , centerRequestPayload)};
                    LOG_D("Save CenterRequestDirectCommand data success");
                    mNewCenterRequestList.push_back(centerRequestPayload->collection_condition_id());
                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                    DiagManagerAdapter::getInstance()->storeCollectionConditionId(centerRequestPayload->collection_condition_id());
                    addCenterRequestJob(newJob);
                } else {
                    urc = URC_FAILED;
                    errorInfo.set_collection_condition_id(centerRequestPayload->collection_condition_id());
                    errorInfo.mutable_error_messages()->Add(INCORRECT_UPDATE_TYPE());
                    errorInfoList.Add()->CopyFrom(errorInfo);
                }
            }
        }
    }
    return urc;
}
UpdateResultCode CollectionCondition::handleCollectionConditionDirectCommands(ErrorInformationList &errorInfoList, const CollectionConditionDirectCommandList &inputDataList)
{
    UpdateResultCode LastUrc{URC_SUCCESS};
    // RDG30-R-1113
    if (inputDataList.size() > NUMBER_COLLECTION_CONDITION_DIRECT_COMMAND_MAX)
    {
        ErrorInformation errorInfo{};
        LastUrc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(TOO_MANY_COLLECTION_CONDITIOS("collection_conditions_direct_command"));
        errorInfoList.Add()->CopyFrom(errorInfo);
    }
    else
    {
        CollectionConditionDirectCommandList newCollectionConditionDirectCommandList{};
        for (CollectionConditionDirectCommandIter it {inputDataList.begin()}; it != inputDataList.end(); it++)
        {
            UpdateResultCode urc{URC_SUCCESS};
            ErrorInformation errorInfo{};
            urc = verifyCollectionConditionDirectCommand(errorInfo, *it);
            if (urc == URC_SUCCESS)
            {
                CollectionConditionDirectCommand* const newData {newCollectionConditionDirectCommandList.Add()};
                newData->CopyFrom(*it);
                // RDG30-R-1282
                if ((newData->schedule_information().schedule_type() != ScheduleType_PERIOD_TRIGGER_ROUTINE) && (newData->schedule_information().has_schedule_interval() == true))
                {
                    LOG_D("schedule_type is not “Periodic Trigger Routine” but the schedule interval (schedule_interval) is specified");
                    newData->mutable_schedule_information()->clear_schedule_interval();
                    LOG_D("Discard schedule_interval success");
                }

                urc = UpdateCollectionConditionDirectCommand(*newData);
                if (urc == URC_FAILED)
                {
                    LastUrc = urc;
                    errorInfo.mutable_error_messages()->Add(VEHICLE_RELATED_ERROR("Memory_read_failure"));
                }
            }
            else
            {
                LastUrc = urc;
            }

            if (LastUrc == URC_FAILED)
            {
                errorInfoList.Add()->CopyFrom(errorInfo);
            }
        }
    }
    return LastUrc;
}

UpdateResultCode CollectionCondition::handleCollectionConditionDiagCommonData(ErrorInformationList &errorInfoList, const CollectionConditionDiagCommon &inputData)
{
    // check update type single
    error_t error{E_OK};
    UpdateResultCode urc{URC_SUCCESS};
    const UpdateTypeSingle updateType {inputData.update_type_collection_condition()};
    const CollectionConditionDiagCommon newData {inputData};

    ErrorInformation errorInfo{};
    errorInfo.set_collection_condition_id(inputData.collection_condition_id());

    urc = verifyCollectionConditionDiagCommon(errorInfo, inputData);

    if (urc == URC_SUCCESS)
    {

        if (DataModel<CollectionConditionDiagCommon>::checkExist(COLLECTION_CONDITION_DIAG_COMMON_DB_NAME))
        {
            if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED)
            {
                error = this->saveCollectionConditionDiagCommon(newData);
                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());              
            }
            else if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)
            {
                error = DataModel<CollectionConditionDiagCommon>::clearData(COLLECTION_CONDITION_DIAG_COMMON_DB_NAME);
                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->clear_common_diag_collection_condition_id();
            }
            else
            {
                //do nothing
            }
        }
        else
        {
            if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED) {
                error = this->saveCollectionConditionDiagCommon(newData);
                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
            } 
            else if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));   
            } else {
                // Do nothing
            }
        }
    }
    (void)updateType;

    if (error != E_OK)
    {
        // RDG30-R-1277
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(VEHICLE_RELATED_ERROR("Memory_read_failure"));
    }

    if (urc == URC_FAILED)
    {
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        errorInfoList.Add()->CopyFrom(errorInfo);
    }
    return urc;
}

UpdateResultCode CollectionCondition::UpdateCollectionConditionDirectCommand(const CollectionConditionDirectCommand &inputData)
{
    error_t error{E_OK};
    UpdateResultCode urc{URC_SUCCESS};
    const uint64_t collectionConditionId {inputData.collection_condition_id()};
    LOG_I("Handle id = %llu", collectionConditionId);
    const UpdateTypeMultiple updateType {inputData.update_type_collection_condition()};

    if (checkExitsCollectionConditionDirectCommand(collectionConditionId))
    {
        const std::shared_ptr<CollectionConditionDirectCommand> oldData {this->getCollectionConditionDirectCommand(collectionConditionId)};
        switch(updateType)
        {
            case UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_ADDED:
            {
                // DirectCommandList* newDirectCommandList = obj.mutable_direct_commands();
                // std::shared_ptr<CollectionConditionDirectCommand> currentCollectionConditionDirectCommand
                //     = getCollectionConditionDirectCommand(collectionConditionId);
                // const DirectCommandList& currentDirectCommandList
                //                         = currentCollectionConditionDirectCommand->direct_commands();
                // newDirectCommandList->MergeFrom(currentDirectCommandList);
                error = this->saveCollectionConditionDirectCommand(inputData);
                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::RD_SCHEDULE_TRIGGER);
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(inputData.collection_condition_id());
                LOG_D("Update Data for collection condition ID: %llu to EMMC", collectionConditionId);
                LOG_D("Update collection condition ID: %llu for the nex collection condition download request.", collectionConditionId);
                const Uint64List& collectionConditionIdList {mGetCollectionConditionRequest->collection_condition_id_stored_in_vehicle().direct_command_collection_condition_ids()};
                bool needAdd {true};
                for (ConstUint64Iter it{collectionConditionIdList.cbegin()}; it != collectionConditionIdList.cend(); it++)
                {
                    if (*it == collectionConditionId)
                    {
                        needAdd = false;
                        break;
                    }
                }

                if (needAdd == true)
                {
                    mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->add_direct_command_collection_condition_ids(collectionConditionId);
                }
                if ((error == E_OK) && (oldData != nullptr))
                {
                    (void)mDeletedCollectionConditionIds.push_back(static_cast<uint64_t>(oldData->collection_condition_id()));
                    (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(oldData->collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_DIRECT_COMMAND)));
                }
                break;
            }
            case UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_DELETED:
            {
                LOG_D("delete Collection Condition DirectCommand, Id = %llu", collectionConditionId);
                error = this->deleteCollectionConditionDirectCommand(collectionConditionId);
                if (error == E_OK)
                {
                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::RD_SCHEDULE_TRIGGER);
                    Uint64List *const collectionConditionIdList {mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->mutable_direct_command_collection_condition_ids()};
                    for (ConstUint64Iter it{collectionConditionIdList->cbegin()}; it != collectionConditionIdList->cend(); it++)
                    {
                        if (*it == collectionConditionId)
                        {
                            (void)collectionConditionIdList->erase(it);
                            break;
                        }
                    }
                    if (oldData != nullptr)
                    {
                        (void)mDeletedCollectionConditionIds.push_back(collectionConditionId);
                    }
                }
                else
                {
                    urc = URC_FAILED;
                }
                break;
            }
            case UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_NO_CHANGED:
            {
                const Uint64List& collectionConditionIdList {mGetCollectionConditionRequest->collection_condition_id_stored_in_vehicle().direct_command_collection_condition_ids()};
                bool needAdd {true};
                for (ConstUint64Iter it {collectionConditionIdList.cbegin()}; it != collectionConditionIdList.end(); it++)
                {
                    if (*it == collectionConditionId)
                    {
                        needAdd = false;
                        break;
                    }
                }

                if (needAdd == true)
                {
                    mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->add_direct_command_collection_condition_ids(collectionConditionId);
                }

                if (oldData != nullptr)
                {
                    oldData->set_update_type_collection_condition(UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_NO_CHANGED);
                    error = this->saveCollectionConditionDirectCommand(*oldData);
                }
                break;
            }
            default:
                break; 
        }
    }
    else 
    {
        if (updateType == UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_ADDED) 
        {
            LOG_D("Save new collection condition Data to EMMC");
            LOG_D("Save new collection condition ID for the nex collection condition download request.");
            error = this->saveCollectionConditionDirectCommand(inputData);
            if (error == E_OK)
            {
                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::RD_SCHEDULE_TRIGGER);
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(inputData.collection_condition_id());
                mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->add_direct_command_collection_condition_ids(collectionConditionId);
                (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(inputData.collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_DIRECT_COMMAND)));

            }
            else
            {
                urc = URC_FAILED;
            }
        }
    }

    (void)error;

    return urc;
}

UpdateResultCode CollectionCondition::handleCollectionConditionRobRobSsrDidEvent(ErrorInformationList &errorInfoList, const CollectionConditionRobRobSsrDidEvent &inputData)
{
    error_t error{E_OK};
    UpdateResultCode urc{URC_SUCCESS};
    const UpdateTypeSingle updateType {inputData.update_type_collection_condition()};
    CollectionConditionRobRobSsrDidEvent newData{};
    newData = inputData;
    ErrorInformation errorInfo{};
    urc = verifyCollectionConditionRobRobSsrDidEvent(errorInfo, inputData);
    if (urc == URC_SUCCESS)
    {
        // RDG30-R-1282
        if ((newData.schedule_information().schedule_type() != ScheduleType_PERIOD_TRIGGER_ROUTINE) && (newData.schedule_information().has_schedule_interval() == true))
        {
            LOG_D("schedule_type is not “Periodic Trigger Routine” but the schedule interval (schedule_interval) is specified");
            newData.mutable_schedule_information()->clear_schedule_interval();
            LOG_D("Discard schedule_interval success");
        }
        bool isExit {false};
        isExit = DataModel<CollectionConditionRobRobSsrDidEvent>::checkExist(COLLECTION_CONDITION_ROB_SSR_DID_DB_NAME);
        if (isExit == true)
        {
            const std::shared_ptr<CollectionConditionRobRobSsrDidEvent> oldData {this->getCollectionConditionRobRobSsrDidEvent()};
            switch (updateType) 
            {
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED:
                {
                    error = this->saveCollectionConditionRobRobSsrDidEvent(newData);
                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::ROB_NOTIFICATION_TRIGGER);
                    DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                    LOG_D("Update Data for collection condition ID: %lld to EMMC", inputData.collection_condition_id());
                    LOG_D("Update collection condition ID: %lld for the nex collection condition download request.", inputData.collection_condition_id());
                    if ((error == E_OK) && (oldData != nullptr))
                    {
                        (void)mDeletedCollectionConditionIds.push_back(static_cast<uint64_t>(oldData->collection_condition_id()));
                        if (newData.collection_condition_id() == oldData->collection_condition_id()) {
                            (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(oldData->collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_DID_EVENT)));
                        } else {
                            (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(newData.collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_DID_EVENT)));
                        }
                    }
                    break;
                }
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED:
                {
                    error = DataModel<CollectionConditionRobRobSsrDidEvent>::clearData(COLLECTION_CONDITION_ROB_SSR_DID_DB_NAME);
                    if (error == E_OK)
                    {
                        const uint64_t collectionConditionId {inputData.collection_condition_id()};
                        DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::ROB_NOTIFICATION_TRIGGER);

                        if (oldData != nullptr)
                        {
                            (void)mDeletedCollectionConditionIds.push_back(collectionConditionId);
                        }
                    }
                    mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->clear_rob_rob_ssr_did_event_collection_condition_id();
                    break;
                }
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_NO_CHANGED:
                {
                    if (oldData != nullptr)
                    {
                        oldData->set_update_type_collection_condition(UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_NO_CHANGED);
                        error = this->saveCollectionConditionRobRobSsrDidEvent(*oldData);
                    }
                    break;
                }
                default:
                    //do nothing
                    break;
            }
        }
        else
        {
            if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED) {
                LOG_D("Save new collection condition Data to EMMC");
                LOG_D("Save new collection condition ID for the nex collection condition download request.");
                error = this->saveCollectionConditionRobRobSsrDidEvent(newData);
                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::ROB_NOTIFICATION_TRIGGER);
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(newData.collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_DID_EVENT)));
            }        
            else if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));   
            } else {
                // Do nothing
            }
        }
    }
    (void)updateType;

    if (error != E_OK)
    {
        // RDG30-R-1277
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(VEHICLE_RELATED_ERROR("Memory_read_failure"));
    }

    if (urc == URC_FAILED)
    {
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        errorInfoList.Add()->CopyFrom(errorInfo);
    }
    return urc;
}

UpdateResultCode CollectionCondition::handleCollectionConditionEcuInformation(ErrorInformationList &errorInfoList, const CollectionConditionEcuInformation &inputData)
{
    error_t error{E_OK};
    UpdateResultCode urc{URC_SUCCESS};
    const UpdateTypeSingle updateType {inputData.update_type_collection_condition()};
    CollectionConditionEcuInformation newData {};
    newData = inputData;
    ErrorInformation errorInfo{};
    urc = verifyCollectionConditionEcuInformation(errorInfo, inputData);
    if (urc == URC_SUCCESS)
    {
        // RDG30-R-1282
        if ((newData.schedule_information().schedule_type() != ScheduleType_PERIOD_TRIGGER_ROUTINE) && (newData.schedule_information().has_schedule_interval() == true))
        {
            newData.mutable_schedule_information()->clear_schedule_interval();
        }

        bool isExit {false};
        isExit = DataModel<CollectionConditionEcuInformation>::checkExist(COLLECTION_CONDITION_ECU_INFO_DB_NAME);
        if (isExit == true)
        {
            const std::shared_ptr<CollectionConditionEcuInformation> oldData {this->getCollectionConditionEcuInformation()};
            switch(updateType)
            {
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED:
                {
                    error = this->saveCollectionConditionEcuInformation(newData);
                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::RD_SCHEDULE_TRIGGER);
                    DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                    if ((error == E_OK) && (oldData != nullptr))
                    {
                        (void)mDeletedCollectionConditionIds.push_back(static_cast<uint64_t>(oldData->collection_condition_id()));
                        if (newData.collection_condition_id() == oldData->collection_condition_id()) {
                            (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(oldData->collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_ECU_INFORMATION)));
                        } else {
                            (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(newData.collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_ECU_INFORMATION)));
                        }
                    }
                    break;
                }
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED:
                {
                    error = DataModel<CollectionConditionEcuInformation>::clearData(COLLECTION_CONDITION_ECU_INFO_DB_NAME);
                    if (error == E_OK)
                    {
                        const uint64_t collectionConditionId {inputData.collection_condition_id()};
                        DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::RD_SCHEDULE_TRIGGER);

                        if (oldData != nullptr)
                        {
                            (void)mDeletedCollectionConditionIds.push_back(collectionConditionId);
                        }
                    }
                    mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->clear_ecu_information_collection_condition_id();
                    break;
                }
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_NO_CHANGED:
                {
                    if (oldData != nullptr)
                    {
                        oldData->set_update_type_collection_condition(UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_NO_CHANGED);
                        error = this->saveCollectionConditionEcuInformation(*oldData);
                    }
                    break;
                }
                default:
                    break;

            }
        }
        else
        {
            if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED) 
            {
                error = this->saveCollectionConditionEcuInformation(newData);
                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::RD_SCHEDULE_TRIGGER);
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(newData.collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_ECU_INFORMATION)));
            }
            else if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));   
            } else {
                // Do nothing
            }
        }
    }

    (void)updateType;

    if (error != E_OK)
    {
        // RDG30-R-1277
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(VEHICLE_RELATED_ERROR("Memory_read_failure"));
    }

    if (urc == URC_FAILED)
    {
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        errorInfoList.Add()->CopyFrom(errorInfo);
    }
    return urc;
}

UpdateResultCode CollectionCondition::handleCollectionConditionWarningInformation(ErrorInformationList &errorInfoList, const CollectionConditionWarningInformation &inputData)
{
    error_t error{E_OK};
    UpdateResultCode urc{URC_SUCCESS};
    const UpdateTypeSingle updateType {inputData.update_type_collection_condition()};
    const CollectionConditionWarningInformation newData {inputData};
    ErrorInformation errorInfo{};
    urc = verifyCollectionConditionWarningInformation(errorInfo, inputData);
    if (urc == URC_SUCCESS)
    {
        bool isExit {false};
        isExit = DataModel<CollectionConditionWarningInformation>::checkExist(COLLECTION_CONDITION_WARN_INFO_DB_NAME);
        if (isExit == true)
        {
            const std::shared_ptr<CollectionConditionWarningInformation> oldData {this->getCollectionConditionWarningInformation()};
            switch(updateType)
            {
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED:
                {
                    error = this->saveCollectionConditionWarningInformation(newData);
                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::WARINING_TRIGGER);
                    DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                    if ((error == E_OK) && (oldData != nullptr))
                    {
                        (void)mDeletedCollectionConditionIds.push_back(static_cast<uint64_t>(oldData->collection_condition_id()));
                        if (newData.collection_condition_id() == oldData->collection_condition_id()) {
                            (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(oldData->collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_WARNING_INFORMATION)));
                        } else {
                            (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(newData.collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_WARNING_INFORMATION)));
                        }
                    }
                    break;
                }
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED:
                {
                    error = DataModel<CollectionConditionWarningInformation>::clearData(COLLECTION_CONDITION_WARN_INFO_DB_NAME);
                    if (error == E_OK)
                    {
                        DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::WARINING_TRIGGER);
                        const uint64_t collectionConditionId {inputData.collection_condition_id()};

                        if (oldData != nullptr)
                        {
                            (void)mDeletedCollectionConditionIds.push_back(collectionConditionId);
                        }
                    }
                    mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->clear_warning_information_collection_condition_id();
                    break;
                }
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_NO_CHANGED:
                {
                    if (oldData != nullptr)
                    {
                        oldData->set_update_type_collection_condition(UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_NO_CHANGED);
                        error = this->saveCollectionConditionWarningInformation(*oldData);
                    }
                    break;
                }
                default:
                    break;

            }
        }
        else
        {
            if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED) 
            {
                error = this->saveCollectionConditionWarningInformation(newData);
                DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::WARINING_TRIGGER);
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(newData.collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_WARNING_INFORMATION)));
            }            
            else if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)
            {
                urc = URC_FAILED;
                errorInfo.mutable_error_messages()->Add(INVALID_SETTING_VALUES("collection_condition_id"));   
            } else {
                // Do nothing
            }
            
        }
    }

    (void)updateType;

    if (error != E_OK)
    {
        // RDG30-R-1277
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(VEHICLE_RELATED_ERROR("Memory_read_failure"));
    }

    if (urc == URC_FAILED)
    {
        errorInfo.set_collection_condition_id(inputData.collection_condition_id());
        errorInfoList.Add()->CopyFrom(errorInfo);
    }
    return urc;
}

const CollectionConditionDirectCommandList &CollectionCondition::getCollectionConditionDirectCommandRequest(void)
{

    // get DB --> get ID list ->
    mCollectionConditionDirectCommandList.Clear();
    const std::shared_ptr<GetCollectionConditionRequest> collectionReq {getGetCollectionConditionRequest()};
    if ((collectionReq != nullptr) && collectionReq->has_collection_condition_id_stored_in_vehicle())
    {
        const Uint64List *const ccCDirectCommands {collectionReq->mutable_collection_condition_id_stored_in_vehicle()->mutable_direct_command_collection_condition_ids()};
        if (ccCDirectCommands->size() > 0)
        {
            for (ConstUint64Iter it{ccCDirectCommands->cbegin()}; it != ccCDirectCommands->cend(); it++)
            {
                LOG_I("collection_conditions_direct_command id = %llu", *it);
                const std::shared_ptr<CollectionConditionDirectCommand> directCmd {this->getCollectionConditionDirectCommand(*it)};
                if (directCmd != nullptr)
                {
                    mCollectionConditionDirectCommandList.Add()->CopyFrom(*directCmd);
                }
            }
        }
        else
        {
            LOG_I("No direct Command");
        }
    }
    return mCollectionConditionDirectCommandList;
}

void CollectionCondition::handleGrpcClientErrorEvent(const android::sp<GrpcResData> resData)
{
    if ((resData != nullptr) && (mCocoTransmissionList.size() > 0U)) 
    {
        const std::vector<android::sp<CollectionCondition::CocoTransmission>>::iterator cocoTransIter {findCocoTransmission(resData->getCallID()
                                                                                                                     , CollectionCondition::CocoTransmission::State::COCO_TRANS_SEND_REQUEST
                                                                                                                     , resData->getInterfaceType())};
        if (cocoTransIter != mCocoTransmissionList.end())
        {
            switch (resData->getGrpcResult())
            {
                case GRPC_RESULT::SEND_FAIL_NOT_FOUND_RECEIVER:
#ifdef ENABLE_LGE_LXC
                    (void)HttpManagerAdapter::getInstance()->registerService();
#else
                    (void)HttpManagerAdapter::getInstance()->registerReceiver();
#endif /* ENABLE_LGE_LXC */
                    (*cocoTransIter)->doOperationA();
                    break;
                case GRPC_RESULT::SEND_FAIL_DATA_DISCONNECTED:
                    LOG_I("GRPC response SEND_FAIL_DATA_DISCONNECTED, DoOperationA: retrying when the connection is restored");
                    (*cocoTransIter)->doOperationA();
                    break;
                case GRPC_RESULT::SEND_FAIL:
                case GRPC_RESULT::SEND_FAIL_INVALID_PARAMETER:
                case GRPC_RESULT::SEND_FAIL_GRPC_SETTING:
                case GRPC_RESULT::SEND_FAIL_CONNECT_FAIL:
                case GRPC_RESULT::SEND_FAIL_TIMEOUT:
                case GRPC_RESULT::SEND_FAIL_RESPONSE_PARSE_FAIL:
                case GRPC_RESULT::SEND_FAIL_QUEUE_FULL:
                {
                    const grpc::StatusCode grpcCode {resData->getGrpcCode()};
                    LOG_E("Send GetCollectionCondition request failed");
                    if ((grpcCode == grpc::StatusCode::UNKNOWN)
                            || (grpcCode == grpc::StatusCode::DEADLINE_EXCEEDED)
                            || (grpcCode == grpc::StatusCode::UNIMPLEMENTED)
                            || (grpcCode == grpc::StatusCode::INTERNAL)
                            || (grpcCode == grpc::StatusCode::UNAVAILABLE)
                            || (grpcCode == grpc::StatusCode::DATA_LOSS))
                    {
                        LOG_E("Received Server error, GrpcResult = %d, GrpcCode = %d", static_cast<int32_t>(resData->getGrpcResult()), static_cast<int32_t>(grpcCode));
                        if(grpcCode == grpc::StatusCode::DEADLINE_EXCEEDED) {
                            LOG_I("Save selfDiagNoCenterResponse");
                            DiagManagerAdapter::getInstance()->selfDiagNoCenterResponse();
                        }
                    } 
                    else if ((grpcCode == grpc::StatusCode::CANCELLED)
                            || (grpcCode == grpc::StatusCode::INVALID_ARGUMENT)
                            || (grpcCode == grpc::StatusCode::NOT_FOUND)
                            || (grpcCode == grpc::StatusCode::ALREADY_EXISTS)
                            || (grpcCode == grpc::StatusCode::PERMISSION_DENIED)
                            || (grpcCode == grpc::StatusCode::RESOURCE_EXHAUSTED)
                            || (grpcCode == grpc::StatusCode::FAILED_PRECONDITION)
                            || (grpcCode == grpc::StatusCode::ABORTED)
                            || (grpcCode == grpc::StatusCode::OUT_OF_RANGE)
                            || (grpcCode == grpc::StatusCode::UNAUTHENTICATED)) 
                    {
                        LOG_E("Received client error, GrpcResult = %d, GrpcCode = %d", static_cast<int32_t>(resData->getGrpcResult()), static_cast<int32_t>(grpcCode));
                        (*cocoTransIter)->doOperationA();
                    }
                    else
                    {
                        //do nothing
                    }

                    break;
                }
                default:
                    LOG_E("Unknown GrpcResult code = %d", static_cast<int32_t>(resData->getGrpcResult()));
                    break;
            }
        }
    }
}

void CollectionCondition::removeErrorDirectCommand(const uint64_t collId, vector<uint32_t> vRemoveIdx)
{
    LOG_I("removeErrorDirectCommand %llu", collId);
    // get DB --> get ID list ->
    CollectionConditionDirectCommandList currCollectionConditionDirectCommandList{};
    currCollectionConditionDirectCommandList.CopyFrom(getCollectionConditionDirectCommandRequest());
    if (vRemoveIdx.size() > 0U)
    {
        for (CollectionConditionDirectCommandIter it{currCollectionConditionDirectCommandList.begin()}; it != currCollectionConditionDirectCommandList.end(); it++)
        {
            if ((*it).collection_condition_id() == collId)
            {
                DirectCommandList *const l_DirectcommandRequest{it->mutable_direct_commands()};
                const uint32_t removeSize {vRemoveIdx[vRemoveIdx.size() - 1U]};
                if ((l_DirectcommandRequest->size() >= 0) && (static_cast<uint32_t>(l_DirectcommandRequest->size()) > removeSize))
                {
                    for (uint32_t idx{0U}; idx < vRemoveIdx.size(); idx++)
                    {
                        LOG_I("[removeErrorDirectCommand] error idx %lu", vRemoveIdx[vRemoveIdx.size() - idx - 1U]);
                        const uint32_t tmpRev {vRemoveIdx[vRemoveIdx.size() - idx - 1U]};
                        if (tmpRev <= static_cast<uint32_t>(INT32_MAX))
                        {
                            (void)l_DirectcommandRequest->erase(l_DirectcommandRequest->cbegin() + static_cast<int32_t>(tmpRev));
                        }
                        else
                        {
                            LOG_E("vRemoveIdx is out of range.");
                        }
                    }
                    (void)this->saveCollectionConditionDirectCommand(*it);
                }
                (void)removeSize;
                break;
            }
        }
    }
    else
    {
        LOG_E("[removeErrorDirectCommand] Remove vector is null");
    }
}

void CollectionCondition::onGrpcReconnect(void)
{
    LOG_D("Handle gRPC reconnect event");
    if (mCocoTransmissionList.empty() && (isIgOnStateMaintainedCheck == false))
    {
        (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_COLLECTION_CONDITION_UPDLOAD_END)->sendToTarget();
    } else {        
        std::vector<android::sp<CollectionCondition::CocoTransmission>>::iterator it {mCocoTransmissionList.begin()};
        while(it != mCocoTransmissionList.end())
        {
            (*it)->onNetworkOnline();
            it++;
        }
    }
}

void CollectionCondition::TimerHandler::handlerFunction(const int32_t timerId)
{
    switch (timerId)
    {
        case IG_ON_STATE_MAINTAINED_CHECK_ID:
        {
            LOG_I("IG_ON_STATE_MAINTAINED_CHECK_ID");
            (void)mCollectionCondition.getHandler()->obtainMessage(CollectionCondition::CMD_HANDLE_IG_ON_TIMEOUT)->sendToTarget();
            break;
        }
        default:
            break;
    }
}

void CollectionCondition::onIgOnTimeout(void)
{
    isIgOnStateMaintainedCheck = false;
    const android::sp<CollectionCondition::CocoTransmission> newTrans {new CollectionCondition::CocoTransmission(*this, DiagTrigger::DiagTriggerType::IGON_TRIGGER, GRPC_IF_TYPE::DCIF_RDG010)};
    newTrans->setReqPayload(makeGetCollectionConditionRequest(DiagTrigger::DiagTriggerType::IGON_TRIGGER));
    newTrans->send();
    addCocoTransmission(newTrans);
}

void CollectionCondition::handleDeleteCollectionConditionDiagCommon(const Uint64 id)
{
    const bool isExist {DataModel<CollectionConditionDiagCommon>::checkExist(COLLECTION_CONDITION_DIAG_COMMON_DB_NAME)};
    if (isExist == true)
    {
        const Uint64 cocoId {mGetCollectionConditionRequest->collection_condition_id_stored_in_vehicle().common_diag_collection_condition_id()};
        if ( cocoId != 0U) {
            if ((id != 0U) && (id != cocoId))
            {
                LOG_E("cocoid not match");
            }
            (void)mDeletedCollectionConditionIds.push_back(cocoId);
        }
        const error_t error {DataModel<CollectionConditionDiagCommon>::clearData(COLLECTION_CONDITION_DIAG_COMMON_DB_NAME)};
        if (error != E_OK)
        {
            LOG_E("Delete CollectionConditionDiagCommon failed");
        } else {
            LOG_D("Delete CollectionConditionDiagCommon success");
        }
    }
}
void CollectionCondition::handleDeleteCollectionConditionRobRobSsrDidEvent(const Uint64 id)
{
    if ((mGetCollectionConditionRequest != nullptr) && mGetCollectionConditionRequest->has_collection_condition_id_stored_in_vehicle())
    {
        const bool isExist {DataModel<CollectionConditionRobRobSsrDidEvent>::checkExist(COLLECTION_CONDITION_ROB_SSR_DID_DB_NAME)};
        if (isExist == true)
        {
            const Uint64 cocoId {mGetCollectionConditionRequest->collection_condition_id_stored_in_vehicle().rob_rob_ssr_did_event_collection_condition_id()};
            if ( cocoId != 0U) {
                if ((id != 0U) && (id != cocoId))
                {
                    LOG_E("cocoid not match");
                }
                (void)mDeletedCollectionConditionIds.push_back(cocoId);
            }
            const error_t error {DataModel<CollectionConditionRobRobSsrDidEvent>::clearData(COLLECTION_CONDITION_ROB_SSR_DID_DB_NAME)};
            if (error != E_OK)
            {
                LOG_E("Delete CollectionConditionRobRobSsrDidEvent failed");
            } else {
                LOG_D("Delete CollectionConditionRobRobSsrDidEvent success");
            }
        }
    }
}

void CollectionCondition::handleDeleteCollectionConditionEcuInformation(const Uint64 id)
{
    if ((mGetCollectionConditionRequest != nullptr) && mGetCollectionConditionRequest->has_collection_condition_id_stored_in_vehicle())
    {
        const bool isExist {DataModel<CollectionConditionEcuInformation>::checkExist(COLLECTION_CONDITION_ECU_INFO_DB_NAME)};
        if (isExist == true)
        {
            const Uint64 cocoId {mGetCollectionConditionRequest->collection_condition_id_stored_in_vehicle().ecu_information_collection_condition_id()};
            if ( cocoId != 0U) {
                if ((id != 0U) && (id != cocoId))
                {
                    LOG_E("cocoid not match");
                }
                (void)mDeletedCollectionConditionIds.push_back(cocoId);
            }
            const error_t error {DataModel<CollectionConditionEcuInformation>::clearData(COLLECTION_CONDITION_ECU_INFO_DB_NAME)};
            if (error != E_OK)
            {
                LOG_E("Delete CollectionConditionEcuInformation failed");
            } else {
                LOG_D("Delete CollectionConditionEcuInformation success");
            }
        }
    }
}

void CollectionCondition::handleDeleteCollectionConditionWarningInformation(const Uint64 id)
{
    if ((mGetCollectionConditionRequest != nullptr) && mGetCollectionConditionRequest->has_collection_condition_id_stored_in_vehicle())
    {
        const bool isExist {DataModel<CollectionConditionWarningInformation>::checkExist(COLLECTION_CONDITION_WARN_INFO_DB_NAME)};
        if (isExist == true)
        {
            const Uint64 cocoId {mGetCollectionConditionRequest->collection_condition_id_stored_in_vehicle().warning_information_collection_condition_id()};
            if ( cocoId != 0U) {
                if ((id != 0U) && (id != cocoId))
                {
                    LOG_E("cocoid not match");
                }
                (void)mDeletedCollectionConditionIds.push_back(cocoId);
            }
            const error_t error {DataModel<CollectionConditionWarningInformation>::clearData(COLLECTION_CONDITION_WARN_INFO_DB_NAME)};
            if (error != E_OK)
            {
                LOG_E("Delete CollectionConditionWarningInformation failed");
            } else {
                LOG_D("Delete CollectionConditionWarningInformation success");
            }
        }
    }
}

void CollectionCondition::handleDeleteCollectionConditionDirectCommands(void)
{
    if ((mGetCollectionConditionRequest != nullptr) && mGetCollectionConditionRequest->has_collection_condition_id_stored_in_vehicle())
    {
        const int32_t lenght {mGetCollectionConditionRequest->collection_condition_id_stored_in_vehicle().direct_command_collection_condition_ids_size()};
        if (lenght > 0)
        {
            const Uint64List& ccCDirectCommands {mGetCollectionConditionRequest->collection_condition_id_stored_in_vehicle().direct_command_collection_condition_ids()};
            ConstUint64Iter it {ccCDirectCommands.cbegin()};

            while ((it != ccCDirectCommands.cend()) && (*it != 0U))
            {
                const error_t error {deleteCollectionConditionDirectCommand(*it)};
                if (error == E_OK)
                {
                    LOG_D("Delete Collection Condition DirectCommand, Id = %llu success", *it);
                    (void)mDeletedCollectionConditionIds.push_back(*it);
                } else {
                    LOG_D("Delete Collection Condition DirectCommand, Id = %llu failed", *it);
                }
                it++;
            }
        }
    }
}

void CollectionCondition::handleRdgActiveFlagOff(void)
{
    handleDeleteCollectionConditionDiagCommon();
    handleDeleteCollectionConditionRobRobSsrDidEvent();
    handleDeleteCollectionConditionEcuInformation();
    handleDeleteCollectionConditionWarningInformation();
    handleDeleteCollectionConditionDirectCommands();

    (void)DataModel<GetCollectionConditionRequest>::clearData(GET_COLLECTION_CONDITION_REQ_DB_NAME);
    mGetCollectionConditionRequest = nullptr;
}

std::shared_ptr<GetCollectionConditionRequest> CollectionCondition::makeGetCollectionConditionRequest(const DiagTrigger::DiagTriggerType type)
{
    const std::shared_ptr<GetCollectionConditionRequest> collectionConditionRequestBuff {std::make_shared<GetCollectionConditionRequest>()};

    mGetCollectionConditionRequest = getGetCollectionConditionRequest();

    collectionConditionRequestBuff->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_GET_COLLECTION_CONDITION);
    collectionConditionRequestBuff->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_GET_COLLECTION_CONDITION, UploadManager::getInstance()->getCounterMessage()));
    collectionConditionRequestBuff->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
    collectionConditionRequestBuff->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    collectionConditionRequestBuff->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(CommonUtils::getGeodesyInfo());

    collectionConditionRequestBuff->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
    collectionConditionRequestBuff->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());

    // 24DCM_RDG_DIS-FR01_160
    const uint8_t tmpRDGFlag {DiagManagerAdapter::getInstance()->getRDGFlag()};
    if ((type == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) && (tmpRDGFlag == 0x01U))
    {
        // Do nothing
        LOG_D("Center push while RDG active flag is ON -> collection condition ID stored in the vehicle shall NOT be set");
    }
    else if (((type == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) && (tmpRDGFlag == 0x00U)) || (type == DiagTrigger::DiagTriggerType::IGON_TRIGGER))
    {
        if ((mGetCollectionConditionRequest != nullptr) && (mGetCollectionConditionRequest->has_collection_condition_id_stored_in_vehicle()))
        {
            LOG_D("Collection condition ID stored in the vehicle is exist -> assinge to the center download request");
            collectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->CopyFrom(mGetCollectionConditionRequest->collection_condition_id_stored_in_vehicle());
            if (collectionConditionRequestBuff->collection_condition_id_stored_in_vehicle().direct_command_collection_condition_ids().size() <= 0)
            {
                collectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->add_direct_command_collection_condition_ids(0U);
            }
        }
        else
        {
            // Set zero to collection condition ID if the vehicle has no collection condition
            LOG_D("No Collection condition ID stored in the vehicle -> assinge collection condition ID = 0 to the center download request");
            collectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_common_diag_collection_condition_id(0U);
            collectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_rob_rob_ssr_did_event_collection_condition_id(0U);
            collectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_ecu_information_collection_condition_id(0U);
            collectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_warning_information_collection_condition_id(0U);
            collectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->add_direct_command_collection_condition_ids(0U);
        }
    }
    else
    {
        // Do nothing
    }
    (void)tmpRDGFlag;

    if (collectionConditionRequestBuff->has_collection_condition_id_stored_in_vehicle())
    {
        LOG_D("collectionConditionRequestBuff has_collection_condition_id_stored_in_vehicle");
        LOG_D("collectionConditionRequestBuff common_diag_collection_condition_id = %lu", collectionConditionRequestBuff->collection_condition_id_stored_in_vehicle().common_diag_collection_condition_id());
    }
    return collectionConditionRequestBuff;
}

CollectionCondition::CocoTransmission::CocoTransmission(CollectionCondition &inst
                                                        , const DiagTrigger::DiagTriggerType triggerType
                                                        , const GRPC_IF_TYPE type)                              
    : mCollectionCondition(inst)
    , mNetworkOutOfRange(false)
    , mState(State::COCO_TRANS_IDLE)
    , mRetryCounter(0U)
    , mCallId(0)
    , mTriggerType(triggerType)
    , mRequestType(type)
    , mTransmissionTimerHandler(*this)
    , mTransmissionTimer(&mTransmissionTimerHandler, RETRY_TIMER_ID)
{
    mDuration = updateRetryDuration();
}

void CollectionCondition::CocoTransmission::send()
{
    if (mRetryCounter <= MAX_RETRY_COUNT)
    {
        if(mCollectionCondition.takeFeatureStatus())
        {
            // Send request to download the data collection conditions to the center
            this->setState(State::COCO_TRANS_SEND_REQUEST);
            if (mReqPayload != nullptr)
            {
#ifdef ENABLE_LGE_LXC
                uint8_t region {RegionManagerAdapter::getInstance()->getNation()};
#else
                uint8_t region {0U};
                (void)RegionManager::instance()->getNation(region);
#endif /* ENABLE_LGE_LXC */
                if (mDuration > 1U)
                {
                    const uint32_t httpTimeout {mDuration - 1U};
                    if (httpTimeout <= static_cast<uint32_t>(INT32_MAX))
                    {
                        const android::sp<GrpcReqData> requestData {new GrpcReqData(GRPC_APP_TYPE::RMT_DIAG, mRequestType, static_cast<int32_t>(httpTimeout), region == LGE_REGION::LGE_REGION_CN ? true : false, mReqPayload.get(), "")};
                        mCallId = HttpManagerAdapter::getInstance()->sendGrpcMessage(requestData);
                    }
                    LOG_D("CocoTransmission send request: trigger type = %d, callId = %d, timeout = %u", static_cast<int32_t>(mTriggerType), mCallId, mDuration);
                }
            } else {
                LOG_E("mReqPayload is null");
            }

            mTransmissionTimer.setDuration(mDuration, 0U);
            mTransmissionTimer.start();
        } else {
            doOperationA();
        }
    } else {
        this->setState(State::COCO_TRANS_FINISHED);
    }
}

void CollectionCondition::CocoTransmission::startTimeout(const uint32_t duration)
{
    mTransmissionTimer.setDuration(duration, 0U);
    mTransmissionTimer.start();
}

void CollectionCondition::CocoTransmission::stopTimeout()
{
    LOG_I("CocoTransmission id = %d timeout stop success", this->getCallId());
    this->mTransmissionTimer.stop();
}

uint32_t CollectionCondition::CocoTransmission::updateRetryDuration()
{
    uint32_t timeout {0U};
    switch (mRetryCounter)
    {
    case 0U:
        timeout = FIRST_RETRY_TIMEOUT;
        break;
    case 1U:
        timeout = SECOND_RETRY_TIMEOUT;
        break;
    case 2U:
        timeout = THIRD_RETRY_TIMEOUT;
        break;
    case 3U:
        timeout = REQUEST_TIMEOUT;
        break;
    default:
        LOG_D("retry counter > 3");
        break;
    }
    LOG_D("Update retry, mRetryCounter = %d, duration = %u", mRetryCounter, timeout);
    if ((mRetryCounter + 1U) <= static_cast<uint8_t>(UINT8_MAX))
    {
        mRetryCounter++;
    }
    return timeout;
}

void CollectionCondition::handleCenterRespondSuccess(const android::sp<GrpcResData> pGrpcResData)
{
    removeCocoTransmission(pGrpcResData->getCallID(), CocoTransmission::State::COCO_TRANS_SEND_REQUEST, pGrpcResData->getInterfaceType());
}

void CollectionCondition::CocoTransmission::doOperationB(const int32_t httpRetryTime = 0)
{
    // Do Operation B: Restart the collection condition update sequence from DCIF-RDG010 request in accordance with retry rules.
    LOG_I("Do Operation B");
    this->mState = State::COCO_TRANS_OPERATION_B;
    this->stopTimeout();
    this->mDuration = updateRetryDuration();
    if (httpRetryTime > 0)
    {
        this->mDuration = static_cast<uint32_t>(httpRetryTime);
    }
    this->send();
}

void CollectionCondition::CocoTransmission::doOperationA()
{
    LOG_I("Do Operation A");
    this->setNetworkOutOfRange(true);
    this->setState(State::COCO_TRANS_OPERATION_A);
    this->setRetryCounter(0U);
    this->stopTimeout();
}

void CollectionCondition::CocoTransmission::onNetworkOnline()
{
    setNetworkOutOfRange(false);
    send();
}

void CollectionCondition::CocoTransmission::onFeatureStatusOn()
{
    if (mState == State::COCO_TRANS_OPERATION_A)
    {
        setNetworkOutOfRange(false);
        send();
    }
}

void CollectionCondition::CocoTransmission::TransmissionTimerHandler::handlerFunction(const int32_t timerId)
{
    switch (timerId)
    {
        case RETRY_TIMER_ID:
        {
            (void)mTransmission.mCollectionCondition.getHandler()->obtainMessage(CollectionCondition::CMD_HANDLE_TRANSMISSON_TIMEOUT, mTransmission.getCallId(), static_cast<int32_t>(mTransmission.getState()), static_cast<int32_t>(mTransmission.getRequestType()))->sendToTarget();
            break;
        }
    default:
        break;
    }
}

void CollectionCondition::CocoTransmission::handleTransmissionTimeout()
{
    LOG_I("RETRY_TIMER_ID, RetryCounter = %d", getRetryCounter());
    if (getRetryCounter() <= MAX_RETRY_COUNT)
    {
        if ((mNetworkOutOfRange == false) && (getState() == State::COCO_TRANS_SEND_REQUEST))
        {
            setDuration(updateRetryDuration());
            send();
        }
    }
    else
    {
        mCollectionCondition.removeCocoTransmission(getCallId(), getState(), getRequestType());
    }
}

std::vector<android::sp<CollectionCondition::CocoTransmission>>::iterator CollectionCondition::findCocoTransmission(const int32_t callId
                                                                                                                    , const CocoTransmission::State aState
                                                                                                                    , const GRPC_IF_TYPE aType)
{
    std::vector<android::sp<CocoTransmission>>::iterator it {mCocoTransmissionList.begin()};
    while(it != mCocoTransmissionList.end())
    {
        if ((callId == (*it)->getCallId()) 
            && (aState == (*it)->getState())
            && (aType == (*it)->getRequestType()))
        {
            LOG_D("Found CocoTransmission callId = %d", callId);
            goto exit;
        } else {
            it++;
        }
    }
exit:
    return it;
}

std::vector<android::sp<CollectionCondition::CocoTransmission>>::iterator CollectionCondition::findCocoTransmission(const android::sp<CollectionCondition::CocoTransmission>& trans)
{
    std::vector<android::sp<CocoTransmission>>::iterator it {mCocoTransmissionList.begin()};
    while(it != mCocoTransmissionList.end())
    {
        if ((trans->getCallId() == (*it)->getCallId()) 
            && (trans->getTriggerType() == (*it)->getTriggerType())
            && (trans->getRequestType() == (*it)->getRequestType()))
        {
            LOG_D("Found CocoTransmission callId = %d", trans->getCallId());
            goto exit;
        } else {
            it++;
        }
    }
exit:
    return it;
}

void CollectionCondition::removeCocoTransmission(const int32_t callId, const CocoTransmission::State aState, const GRPC_IF_TYPE aType)
{
    const std::vector<android::sp<CocoTransmission>>::iterator it {findCocoTransmission(callId, aState, aType)};
    if (it != mCocoTransmissionList.end()) {
        (*it)->stopTimeout();
        (void)mCocoTransmissionList.erase(it);
        LOG_D("Removed CocoTransmission callId = %d, size remaining %u", callId, mCocoTransmissionList.size());
        
    } else {
        LOG_D("Not found CocoTransmission callId = %d", callId);
    }
}

void CollectionCondition::addCocoTransmission(const android::sp<CocoTransmission>& trans)
{
    const std::vector<android::sp<CocoTransmission>>::iterator currentTrans {findCocoTransmission(trans)};
    if (currentTrans != mCocoTransmissionList.end())
    {
        (*currentTrans)->stopTimeout();
        trans->stopTimeout();

        (*currentTrans)->setRetryCounter(trans->getRetryCounter());
        (*currentTrans)->startTimeout(trans->getDuration());
    } else {
        mCocoTransmissionList.push_back(trans);
    }
}

bool CollectionCondition::isCenterRequestJobExits(const uint64_t jobId)
{
    bool result {false};
    const std::unordered_map<uint64_t, std::shared_ptr<CenterRequestJob>>::iterator it {mCenterRequestJobList.find(jobId)};
    result = it != mCenterRequestJobList.end() ? true : false;
    return result;
}

void CollectionCondition::addCenterRequestJob(const std::shared_ptr<CenterRequestJob>& aJob)
{
    const android::AutoMutex _l{mLock};
    mCenterRequestJobList[aJob->getId()] = aJob;
}

const std::shared_ptr<CenterRequestJob> CollectionCondition::getCenterRequestJob(const uint64_t jobId)
{
    std::shared_ptr<CenterRequestJob> job {nullptr};
    const std::unordered_map<uint64_t, std::shared_ptr<CenterRequestJob>>::iterator it {mCenterRequestJobList.find(jobId)};
    if (it != mCenterRequestJobList.end())
    {
        job = it->second;
    }
    return job;
}

error_t CollectionCondition::removeCenterRequestJob(const uint64_t jobId)
{
    const android::AutoMutex _l{mLock};
    error_t err {E_ERROR};
    const std::unordered_map<uint64_t, std::shared_ptr<CenterRequestJob>>::iterator it {mCenterRequestJobList.find(jobId)};
    if (it != mCenterRequestJobList.end())
    {
        (void)mCenterRequestJobList.erase(it);
        LOG_D("remove CenterRequestJob success, job id = %llu", jobId);
        err = E_OK;
    } else {
        LOG_D("remove CenterRequestJob failed, job id = %llu not found", jobId);
    }
    return err;
}

void CollectionCondition::onFinishCenterRequestJob(const uint64_t aJob)
{
    LOG_D("on finish center request job, id = %llu", aJob);
    (void)removeCenterRequestJob(aJob);
    LOG_D("CenterRequestJobList size = %d", mCenterRequestJobList.size());
}

std::vector<uint64_t> CollectionCondition::getNewCenterRequestList(void) 
{
    std::vector<uint64_t> mNewCenterReqList {};
    if (mNewCenterRequests.empty() == false)
    {
        mNewCenterReqList = mNewCenterRequests.front();
        mNewCenterRequests.pop();
    }
    return mNewCenterReqList;
}

void CollectionCondition::onOtherFeatureStatusOff()
{
    if (mCocoTransmissionList.empty() && (isIgOnStateMaintainedCheck == false))
    {
        (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_COLLECTION_CONDITION_UPDLOAD_END)->sendToTarget();
    } else {
        std::vector<android::sp<CocoTransmission>>::iterator it {mCocoTransmissionList.begin()};
        while(it != mCocoTransmissionList.end())
        {
            (*it)->onFeatureStatusOn();
            it++;
        }
    }
}

bool CollectionCondition::takeFeatureStatus()
{
    bool ret {false};
    const int32_t featureAction {ApplicationManagerAdapter::getInstance()->queryActionForFeature("remotediag")};
    if (featureAction == FeatureAction::LAUNCH)
    {
        const int32_t error {ApplicationManagerAdapter::getInstance()->setFeatureStatus("remotediag", "remotediag", true)};
        if(error != E_OK) {
            LOG_I("set featureStatus Fail");
            ret = false;
        } else {
            LOG_I("set featureStatus success");
            ret = true;
        }
    }
    return ret;
}
}
