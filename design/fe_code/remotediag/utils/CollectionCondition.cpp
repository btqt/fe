#include <utils/Buffer.h>

#include "services/DiagManagerAdapter.h"
#include "CommonUtils.h"
#include "CollectionCondition.h"
#include "DataModel.h"
#include "Logger.h"
#include "utils/RemotediagHandler.h"

ANDROID_SINGLETON_STATIC_INSTANCE(rdgapp::CollectionCondition)

namespace rdgapp {

CollectionCondition::CollectionCondition()
    : android::RefBase()
    , mGetCollectionConditionRequestOpA(false)
    , mNotificationCollectionConditionUpdateResultOpA(false)
    , mCenterRequestAllDtcSsr{nullptr}
    , mCenterRequestAllRob{nullptr}
    , mCenterRequestEcuInformation{nullptr}
    , mGetCollectionConditionRequest{nullptr}
    , mGetCollectionConditionRequestBuff{nullptr}
    , mNotifyCollectionConditionUpdateResultRequest{nullptr}
    , mCollectionConditionDirectCommandList{}
    , mCenterRequestsRobSsr{}
    , mCenterRequestDirectCommand{}
    , mCollectionConditionTimerHandler(*this)
    , mCollectionConditionTimer(&mCollectionConditionTimerHandler, TimerHandler::IG_ON_STATE_MAINTAINED_CHECK_ID)
    , mSendGetCollectionConditionRetryTimer(&mCollectionConditionTimerHandler, TimerHandler::RETRY_TIMER_ID)
    , mNotifyCollectionConditionUpdateResultRequestTimer(&mCollectionConditionTimerHandler, TimerHandler::NOTIFICATION_UPDATE_RESULT_RETRY_TIMER_ID)
    , mTriggerType{DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN}
    , mRetryGetCollectionConditionCounter{0U}
    , mRetryNotifyCollectionConditionUpdateResultRequestCounter{0U}
    , mDeletedCollectionConditionIds()
{
    mCollectionConditionTimer.setDuration(TimerHandler::ID_IG_ON_STATE_MAINTAINED_CHECK_TIMEOUT, 0U);
}

void CollectionCondition::init(android::sp<sl::SLLooper> &privateLooper)
{
    mCollectionConditionHandler = new MainHandler(privateLooper, *this);
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
    case CMD_SEND_GET_COLLECTION_CONDITION_REQUEST:
    {
        LOG_I("CMD_SEND_GET_COLLECTION_CONDITION_REQUEST");
        // 24DCM_RDG_DIS-FR01_281
        if (PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_ON)
        {
            mCollectionCondition.sendGetCollectionConditionRequest();
        } 
        else if (PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_OFF) 
        {
            LOG_D("Abort Center push notification, because IG status is OFF");
        } else {
            //Do noting
        }
        break;
    }
    case CMD_SEND_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT_REQUEST:
    {
        LOG_I("CMD_SEND_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT_REQUEST");

        mCollectionCondition.sendNotifyCollectionConditionUpdateResultRequest();
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
    mRetryGetCollectionConditionCounter = 0U;
    mGetCollectionConditionRequestOpA = false;
    mNotificationCollectionConditionUpdateResultOpA = false;
    mTriggerType = DiagTrigger::DiagTriggerType::CENTER_TRIGGER;
    mSendGetCollectionConditionRetryTimer.stop();
    (void)mCollectionConditionHandler->obtainMessage(MainHandler::CMD_SEND_GET_COLLECTION_CONDITION_REQUEST)->sendToTarget();
}

void CollectionCondition::onReceiveIG(const bool status)
{
    if (status == true)
    {
        // IG ON
        mRetryGetCollectionConditionCounter = 0U;
        mTriggerType = DiagTrigger::DiagTriggerType::IGON_TRIGGER;
        mCollectionConditionTimer.start();
        LOG_D("onReceiveIG ON");
    } else {
        LOG_D("onReceiveIG OFF");
        mRetryGetCollectionConditionCounter = 0U;
        mCollectionConditionTimer.stop();

    }
}
        


void CollectionCondition::onReceivedNotifyCollectionConditionUpdateResultResponse(const android::sp<GrpcResData> pGrpcResData)
{
    LOG_I("Received NotifyCollectionConditionUpdateResultResponse");
    if (pGrpcResData != nullptr)
    {
        const grpc::StatusCode responseCode {pGrpcResData->getGrpcCode()};
        const GRPC_RESULT gRpcResult {pGrpcResData->getGrpcResult()};
        switch (gRpcResult)
        {
            case GRPC_RESULT::SEND_SUCCESS:
            {
                if (responseCode == grpc::StatusCode::OK)
                {
                    mRetryNotifyCollectionConditionUpdateResultRequestCounter = 0U;
                    mNotifyCollectionConditionUpdateResultRequestTimer.stop();
                    (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_COLLECTION_CONDITION_UPDLOAD_END)->sendToTarget();
                }
                break;
            }
            case GRPC_RESULT::SEND_FAIL_NOT_FOUND_RECEIVER:
                (void)HttpManagerAdapter::getInstance()->registerReceiver();
                break;
            case GRPC_RESULT::SEND_FAIL_DATA_DISCONNECTED:
                LOG_I("GRPC response SEND_FAIL_DATA_DISCONNECTED, DoOperationA: retrying when the connection is restored");
                mNotificationCollectionConditionUpdateResultOpA = true;
                mNotifyCollectionConditionUpdateResultRequestTimer.stop();
                mRetryNotifyCollectionConditionUpdateResultRequestCounter = 0U;
                break;
            case GRPC_RESULT::SEND_FAIL:
            case GRPC_RESULT::SEND_FAIL_INVALID_PARAMETER:
            case GRPC_RESULT::SEND_FAIL_GRPC_SETTING:
            case GRPC_RESULT::SEND_FAIL_CONNECT_FAIL:
            case GRPC_RESULT::SEND_FAIL_TIMEOUT:
            {
                LOG_E("Send GetCollectionCondition request failed");
                if ((responseCode == grpc::StatusCode::UNKNOWN)
                        || (responseCode == grpc::StatusCode::DEADLINE_EXCEEDED)
                        || (responseCode == grpc::StatusCode::UNIMPLEMENTED)
                        || (responseCode == grpc::StatusCode::INTERNAL)
                        || (responseCode == grpc::StatusCode::UNAVAILABLE)
                        || (responseCode == grpc::StatusCode::DATA_LOSS))
                {
                    LOG_E("Received Server error, httpResultCode = %d, gRpcStatusCode = %d", gRpcResult, responseCode);
                    if(responseCode == grpc::StatusCode::DEADLINE_EXCEEDED) {
                        LOG_I("Save selfDiagNoCenterResponse");
                        DiagManagerAdapter::getInstance()->selfDiagNoCenterResponse();
                    }
                        if (mRetryNotifyCollectionConditionUpdateResultRequestCounter < (MAX_RETRY+1U))
                        {
                            const int32_t retryTime {pGrpcResData->getRetryTime()};
                            if (retryTime > 0)
                            {
                                mNotifyCollectionConditionUpdateResultRequestTimer.setDuration(static_cast<uint32_t>(retryTime), 0U);
                                mNotifyCollectionConditionUpdateResultRequestTimer.start();
                            } 
                            else if (mRetryNotifyCollectionConditionUpdateResultRequestCounter == 1U) 
                            {
                                    LOG_D("Waiting for first time retry");
                                    mNotifyCollectionConditionUpdateResultRequestTimer.setDuration(TimerHandler::FIRST_RETRY_TIMEOUT, 0U);
                                    mNotifyCollectionConditionUpdateResultRequestTimer.start();
                            }
                            else {
                                // do nothing
                            }
                        }
                } 
                else if ((responseCode == grpc::StatusCode::CANCELLED)
                        || (responseCode == grpc::StatusCode::INVALID_ARGUMENT)
                        || (responseCode == grpc::StatusCode::NOT_FOUND)
                        || (responseCode == grpc::StatusCode::ALREADY_EXISTS)
                        || (responseCode == grpc::StatusCode::PERMISSION_DENIED)
                        || (responseCode == grpc::StatusCode::RESOURCE_EXHAUSTED)
                        || (responseCode == grpc::StatusCode::FAILED_PRECONDITION)
                        || (responseCode == grpc::StatusCode::ABORTED)
                        || (responseCode == grpc::StatusCode::OUT_OF_RANGE)
                        || (responseCode == grpc::StatusCode::UNAUTHENTICATED)) 
                {
                    LOG_E("Received client error, httpResultCode = %d, gRpcStatusCode = %d", gRpcResult, responseCode);
                    LOG_I("DoOperationA: retrying when the connection is restored");
                    mNotificationCollectionConditionUpdateResultOpA = true;
                    mNotifyCollectionConditionUpdateResultRequestTimer.stop();
                    mRetryNotifyCollectionConditionUpdateResultRequestCounter = 0U;
                }
                else
                {
                    //do nothing
                }

                break;
            }
            default:
                LOG_E("Unknown GRPC_RESULT code = %d", static_cast<uint8_t>(gRpcResult));
                break;
        }
        (void)responseCode;
    }
}

void CollectionCondition::onReceivedGetCollectionConditionResponse(const android::sp<GrpcResData> pGrpcResData)
{
    LOG_I("Received get collection condition response");
    bool isNeedNotify {false};
    bool isAbortUpdateCollectionCondition {false};
    (void)mDeletedCollectionConditionIds.clear();
    if (pGrpcResData != nullptr)
    {
        const grpc::StatusCode responseCode {pGrpcResData->getGrpcCode()};
        const GRPC_RESULT gRpcResult {pGrpcResData->getGrpcResult()};

        if (gRpcResult == GRPC_RESULT::SEND_SUCCESS)
        {
            mRetryGetCollectionConditionCounter = 0U;
            mSendGetCollectionConditionRetryTimer.stop();
            if (responseCode == grpc::StatusCode::OK)
            {
                google::protobuf::util::JsonOptions option{};
                option.always_print_primitive_fields = true;
                option.preserve_proto_field_names = true;
                UpdateResultCode finalUrc{URC_SUCCESS};
                mNotifyCollectionConditionUpdateResultRequest = std::make_shared<NotifyCollectionConditionUpdateResultRequest>();
                ErrorInformationList * const errInfor {mNotifyCollectionConditionUpdateResultRequest->mutable_error_information()};
                const std::shared_ptr<google::protobuf::Message> msg {pGrpcResData->getResponseProtoBuf<google::protobuf::Message>()};
                const std::shared_ptr<GetCollectionConditionResponse> res {std::dynamic_pointer_cast<GetCollectionConditionResponse>(msg)};

                LOG_D("Received GetCollectionConditionResponse from the Center");
                std::string getCollectionConditionResponseStr{};
                (void)google::protobuf::util::MessageToJsonString(*res, &getCollectionConditionResponseStr, option);
                printDataDebug(getCollectionConditionResponseStr);
                LOG_D("============================================================================================");

                const bool newRdgFlag{res->rdg_active_flag()};

                DiagManagerAdapter::getInstance()->saveRDGFlag(newRdgFlag);

                const uint32_t freespace {CommonUtils::getFreespace(DATA_PATH)};
                // RDG30-R-1198
                LOG_D("Collection condition size: %d", msg->ByteSizeLong());
                if (msg->ByteSizeLong() > freespace)
                {
                    LOG_E("Total data size of the received data collection condition is larger than the storage capacity");
                    finalUrc = URC_FAILED;
                    errInfor->Add()->mutable_error_messages()->Add(VEHICLE_RELATED_ERROR("not_enough_storage_capacity"));
                    isAbortUpdateCollectionCondition = true;
                }

                // clear center request
                mCenterRequestAllDtcSsr = nullptr;
                mCenterRequestAllRob = nullptr;
                mCenterRequestEcuInformation = nullptr;

                mCenterRequestsRobSsr.Clear();
                mCenterRequestDirectCommand.Clear();

                mGetCollectionConditionRequest = getGetCollectionConditionRequest();
                if (mGetCollectionConditionRequest == nullptr)
                {
                    mGetCollectionConditionRequest
                        = std::make_shared<GetCollectionConditionRequest>();
                }
                UpdateResultCode urc {URC_SUCCESS};

                // check center request
                if (res->has_center_request_all_dtc_ssr() && (res->center_request_all_dtc_ssr().collection_condition_id() != 0U))
                {
                    ErrorInformation errorInfo{};
                    urc = verifyCenterRequestAllDtcSsr(errorInfo, res->center_request_all_dtc_ssr());
                    if ( urc == URC_SUCCESS)
                    {   
                        DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                        mCenterRequestAllDtcSsr 
                            = std::make_shared<CenterRequestAllDtcSsr>();
                        mCenterRequestAllDtcSsr->CopyFrom(res->center_request_all_dtc_ssr());
                        // RDG30-R-1121
                        if ((mCenterRequestAllDtcSsr->schedule_information().schedule_type() != ScheduleType_PERIOD_TRIGGER_ROUTINE) 
                            && (mCenterRequestAllDtcSsr->schedule_information().has_schedule_interval() == true))
                        {
                            mCenterRequestAllDtcSsr->mutable_schedule_information()->clear_schedule_interval();
                        }
                        isNeedNotify = true;
                    } else {
                        finalUrc = URC_FAILED;
                        errInfor->Add()->CopyFrom(errorInfo);
                    }
                } else {
                    LOG_D("No AllDiag Center Request Information");
                }

                if (res->has_center_request_all_rob() && (res->center_request_all_rob().collection_condition_id() != 0U))
                {
                    ErrorInformation errorInfo{};
                    urc = verifyCenterRequestAllRob(errorInfo, res->center_request_all_rob());
                    if ( urc == URC_SUCCESS)
                    {
                        DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                        mCenterRequestAllRob
                            = std::make_shared<CenterRequestAllRob>();
                        mCenterRequestAllRob->CopyFrom(res->center_request_all_rob());
                        LOG_D("Save CenterRequestAllRob data success");
                        // RDG30-R-1121
                        if ((mCenterRequestAllRob->schedule_information().schedule_type() != ScheduleType_PERIOD_TRIGGER_ROUTINE)
                            && (mCenterRequestAllRob->schedule_information().has_schedule_interval() == true))
                        {
                            mCenterRequestAllRob->mutable_schedule_information()->clear_schedule_interval();
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
                if (res->has_center_request_ecu_information() && (res->center_request_ecu_information().collection_condition_id() != 0U))
                {
                    ErrorInformation errorInfo{};
                    urc = verifyCenterRequestEcuInformation(errorInfo, res->center_request_ecu_information());
                    if ( urc == URC_SUCCESS)
                    {
                        DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                        mCenterRequestEcuInformation
                            = std::make_shared<CenterRequestEcuInformation>();
                        mCenterRequestEcuInformation->CopyFrom(res->center_request_ecu_information());
                        // RDG30-R-1121
                        if ((mCenterRequestEcuInformation->schedule_information().schedule_type() != ScheduleType_PERIOD_TRIGGER_ROUTINE) 
                            && (mCenterRequestEcuInformation->schedule_information().has_schedule_interval() == true))
                        {
                            mCenterRequestEcuInformation->mutable_schedule_information()->clear_schedule_interval();
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
                        if (res->has_collection_condition() && (res->collection_condition().ByteSize() > 0))
                        {
                            const CollectionConditionRes &collectionCond {res->collection_condition()};
                            const int32_t directCommandLenght {collectionCond.collection_conditions_direct_command_size()};
                            LOG_D("directCommandLenght %d", directCommandLenght);

                            if (collectionCond.has_collection_condition_diag_common() && (collectionCond.collection_condition_diag_common().collection_condition_id() != 0U))
                            {
                                urc = handleCollectionConditionDiagCommonData(*errInfor, collectionCond.collection_condition_diag_common());
                                if (urc == URC_FAILED)
                                {
                                    finalUrc = urc;
                                } else {
                                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                                    isNeedNotify = true;
                                }
                            }

                            if (collectionCond.has_collection_condition_rob_rob_ssr_did_event() && (collectionCond.collection_condition_rob_rob_ssr_did_event().collection_condition_id() != 0U))
                            {
                                urc = handleCollectionConditionRobRobSsrDidEvent(*errInfor, collectionCond.collection_condition_rob_rob_ssr_did_event());
                                if (urc == URC_FAILED)
                                {
                                    finalUrc = urc;
                                } else {
                                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::ROB_NOTIFICATION_TRIGGER);
                                    isNeedNotify = true;
                                }
                            }

                            if (collectionCond.has_collection_condition_ecu_information() && (collectionCond.collection_condition_ecu_information().collection_condition_id() != 0U))
                            {
                                urc = handleCollectionConditionEcuInformation(*errInfor, collectionCond.collection_condition_ecu_information());
                                if (urc == URC_FAILED)
                                {
                                    finalUrc = urc;
                                } else {
                                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::RD_SCHEDULE_TRIGGER);
                                    isNeedNotify = true;
                                }
                            }

                            if (collectionCond.has_collection_condition_warning_information() && (collectionCond.collection_condition_warning_information().collection_condition_id() != 0U))
                            {
                                urc = handleCollectionConditionWarningInformation(*errInfor, collectionCond.collection_condition_warning_information());
                                if (urc == URC_FAILED)
                                {
                                    finalUrc = urc;
                                } else {
                                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::WARINING_TRIGGER);
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

                            (void)DataModel<GetCollectionConditionRequest>::save(GET_COLLECTION_CONDITION_REQ_DB_NAME, *mGetCollectionConditionRequest);
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
                        finalUrc = URC_FAILED;
                        isNeedNotify = true;
                        ErrorInformation* const errorInfo {errInfor->Add()};
                        if (errorInfo != nullptr)
                        {
                            errorInfo->add_error_messages(INVALID_SETTING_VALUES("rdg_active_flag"));
                            if ((res->has_collection_condition() == false) || (res->collection_condition().ByteSize() <= 0))
                            {
                                errorInfo->add_error_messages(NO_COLLECTION_CONDITION());
                            }
                        }
                    }
                }
                (void)urc;
                
                if (isNeedNotify == true)
                { 
                    (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_NEW_COLLECTION_CONDITION)->sendToTarget();
                }
                
                mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT);
                mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
                mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
                mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(AppCommonHeaderVehicleToCenterGeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
                const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
                mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT, counterValue));
                LOG_I("Check timezoneOffSet: %d", TimeManager::getInstance().getOffset());
                const int32_t tz{TimeManager::getInstance().getOffset()};

                int32_t hour{tz/60};
                int32_t mins{tz%60};

                hour = tz/60;
                mins = tz%60;

                mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(hour);
                mNotifyCollectionConditionUpdateResultRequest->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(mins);
                mNotifyCollectionConditionUpdateResultRequest->set_update_result_code(finalUrc);

                // RDG30-R-1100
                LOG_D("NotifyCollectionConditionUpdateResultRequest Created");
                std::string str{};
                (void)google::protobuf::util::MessageToJsonString(*mNotifyCollectionConditionUpdateResultRequest, &str, option);
                printDataDebug(str);
                LOG_D("============================================================================================");
                mNotifyCollectionConditionUpdateResultRequestTimer.stop();
                mRetryNotifyCollectionConditionUpdateResultRequestCounter = 0U;
                (void)mCollectionConditionHandler->obtainMessage(MainHandler::CMD_SEND_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT_REQUEST)->sendToTarget();
                (void)errInfor;
            }
            else if ((responseCode == grpc::StatusCode::UNKNOWN)
                    || (responseCode == grpc::StatusCode::DEADLINE_EXCEEDED)
                    || (responseCode == grpc::StatusCode::UNIMPLEMENTED)
                    || (responseCode == grpc::StatusCode::INTERNAL)
                    || (responseCode == grpc::StatusCode::UNAVAILABLE)
                    || (responseCode == grpc::StatusCode::DATA_LOSS))
            {
                LOG_E("Send GetCollectionCondition request failed, gRpcResult = %d", gRpcResult);
                LOG_E("                                            responseCode = %d", responseCode);
                if(gRpcResult == GRPC_RESULT::SEND_FAIL_TIMEOUT) {
                    LOG_I("Save selfDiagNoCenterResponse");
                    DiagManagerAdapter::getInstance()->selfDiagNoCenterResponse();
                }
                DoOperationB();
            }
            else
            {
                // Do Operation A: Do not retry communication
                LOG_E("Send GetCollectionCondition request failed, responseCode = %d", responseCode);
            }
            // (void)errInfor;
        }
        else 
        {
            if ((responseCode >= grpc::StatusCode::OK) && (responseCode <= grpc::StatusCode::UNAUTHENTICATED))
            {
                handleGrpcClientErrorEvent(gRpcResult, responseCode);
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

void CollectionCondition::sendGetCollectionConditionRequest(void)
{
    LOG_I("sendGetCollectionConditionRequest with triggertype = %d", static_cast<int32_t>(mTriggerType));
    // Send request to download the data collection conditions to the center
    uint8_t region {0U};
    (void)RegionManager::instance()->getNation(region);

    mGetCollectionConditionRequestBuff = nullptr;
    mGetCollectionConditionRequestBuff = std::make_shared<GetCollectionConditionRequest>();

    uint32_t timeout {0U};
    switch (mRetryGetCollectionConditionCounter)
    {
    case 0U:
        timeout = TimerHandler::REQUEST_TIMEOUT;
        break;
    case 1U:
        timeout = TimerHandler::SECOND_RETRY_TIMEOUT;
        break;
    case 2U:
        timeout = TimerHandler::THIRD_RETRY_TIMEOUT;
        break;
    case 3U:
        timeout = TimerHandler::REQUEST_TIMEOUT;
        break;
    default:
        timeout = 60U;
        break;
    }
    LOG_I("mRetryGetCollectionConditionCounter = %d", mRetryGetCollectionConditionCounter);
    LOG_I("set grpc timeout = %d", timeout);

    mGetCollectionConditionRequest = getGetCollectionConditionRequest();

    mGetCollectionConditionRequestBuff->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_GET_COLLECTION_CONDITION);
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    mGetCollectionConditionRequestBuff->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_GET_COLLECTION_CONDITION, counterValue));
    mGetCollectionConditionRequestBuff->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    mGetCollectionConditionRequestBuff->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    mGetCollectionConditionRequestBuff->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(AppCommonHeaderVehicleToCenterGeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);

    const int32_t tz{TimeManager::getInstance().getOffset()};

    int32_t hour{tz/60};
    int32_t mins{tz%60};

    hour = tz/60;
    mins = tz%60;

    mGetCollectionConditionRequestBuff->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(hour);
    mGetCollectionConditionRequestBuff->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(mins);

    // 24DCM_RDG_DIS-FR01_160
    const uint8_t tmpRDGFlag {DiagManagerAdapter::getInstance()->getRDGFlag()};
    if ((mTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) && (tmpRDGFlag == 0x01U))
    {
        // Do nothing
        LOG_D("Center push while RDG active flag is ON -> collection condition ID stored in the vehicle shall NOT be set");
        /* backup
        mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_common_diag_collection_condition_id(0U);
        mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_rob_rob_ssr_did_event_collection_condition_id(0U);
        mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_ecu_information_collection_condition_id(0U);
        mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_warning_information_collection_condition_id(0U);
        mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->add_direct_command_collection_condition_ids(0U); 
        */
    }
    else if (((mTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) && (tmpRDGFlag == 0x00U)) || (mTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER))
    {
        if ((mGetCollectionConditionRequest != nullptr) && (mGetCollectionConditionRequest->has_collection_condition_id_stored_in_vehicle()))
        {
            LOG_D("Collection condition ID stored in the vehicle is exist -> assinge to the center download request");
            mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->CopyFrom(mGetCollectionConditionRequest->collection_condition_id_stored_in_vehicle());
            if (mGetCollectionConditionRequestBuff->collection_condition_id_stored_in_vehicle().direct_command_collection_condition_ids().size() <= 0)
            {
                mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->add_direct_command_collection_condition_ids(0U);
            }
        }
        else
        {
            // Set zero to collection condition ID if the vehicle has no collection condition
            LOG_D("No Collection condition ID stored in the vehicle -> assinge collection condition ID = 0 to the center download request");
            mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_common_diag_collection_condition_id(0U);
            mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_rob_rob_ssr_did_event_collection_condition_id(0U);
            mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_ecu_information_collection_condition_id(0U);
            mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->set_warning_information_collection_condition_id(0U);
            mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->add_direct_command_collection_condition_ids(0U);
        }
    }
    else
    {
        // Do nothing
    }
    (void)tmpRDGFlag;

    if (mGetCollectionConditionRequestBuff->has_collection_condition_id_stored_in_vehicle())
    {
        LOG_D("mGetCollectionConditionRequestBuff has_collection_condition_id_stored_in_vehicle");
        LOG_D("mGetCollectionConditionRequestBuff common_diag_collection_condition_id = %d", mGetCollectionConditionRequestBuff->mutable_collection_condition_id_stored_in_vehicle()->common_diag_collection_condition_id());
    }

    android::sp<GrpcReqData> reqData{nullptr};
    int32_t res{-1};
    try
    {
        if (timeout <= static_cast<uint32_t>(INT32_MAX))
        {
            reqData = new GrpcReqData(GRPC_APP_TYPE::RMT_DIAG, GRPC_IF_TYPE::DCIF_RDG010, static_cast<int32_t>(timeout), region == 2U ? true : false, static_cast<google::protobuf::Message *>(mGetCollectionConditionRequestBuff.get()), "");
            res = HttpManagerAdapter::getInstance()->sendGrpcMessage(reqData);

            LOG_D("mGetCollectionConditionRequestBuff Created");
            std::string str{};
            google::protobuf::util::JsonOptions option{};
            option.always_print_primitive_fields = true;
            option.preserve_proto_field_names = true;
            (void)google::protobuf::util::MessageToJsonString(*mGetCollectionConditionRequestBuff, &str, option);
            printDataDebug(str);
            LOG_D("============================================================================================");
        }
    }
    catch (std::invalid_argument& e)
    {
        LOG_E("exception: %s", e.what());
    }

    if (res < 0)
    {
        LOG_W("Send get collection condition request failed");
        // DoOperationA();
    }
    else
    {
        LOG_D("Send get collection condition request success");
        switch (mRetryGetCollectionConditionCounter)
        {
        case 0U:
            mSendGetCollectionConditionRetryTimer.setDuration(timeout, 0U);
            mSendGetCollectionConditionRetryTimer.start();
            mRetryGetCollectionConditionCounter++;
            break;
        case 1U:
            
            mSendGetCollectionConditionRetryTimer.setDuration(timeout, 0U);
            mSendGetCollectionConditionRetryTimer.start();
            mRetryGetCollectionConditionCounter++;
            LOG_D("Waiting for second time retry, timeout = %d", timeout);
            break;
        case 2U:
            mSendGetCollectionConditionRetryTimer.setDuration(timeout, 0U);
            mSendGetCollectionConditionRetryTimer.start();
            mRetryGetCollectionConditionCounter++;
            LOG_D("Waiting for third time retry, timeout = %d", timeout);
            break;
        case 3U:
            LOG_D("Waiting for the last timeout = %d", timeout);
            mRetryGetCollectionConditionCounter = 0U;
            mSendGetCollectionConditionRetryTimer.stop();
            break;
        default:
            break;
        }
    }
    LOG_I("Done");
}

void CollectionCondition::sendNotifyCollectionConditionUpdateResultRequest()
{
    LOG_I("sendNotifyCollectionConditionUpdateResultRequest");
    uint8_t region {0U};
    (void)RegionManager::instance()->getNation(region);
    // Send request to download the data collection conditions to the center
    if (mNotifyCollectionConditionUpdateResultRequest != nullptr) {
        uint32_t timeout {0U};
        switch (mRetryNotifyCollectionConditionUpdateResultRequestCounter)
        {
        case 0U:
            timeout = TimerHandler::REQUEST_TIMEOUT;
            break;
        case 1U:
            timeout = TimerHandler::SECOND_RETRY_TIMEOUT;
            break;
        case 2U:
            timeout = TimerHandler::THIRD_RETRY_TIMEOUT;
            break;
        case 3U:
            timeout = TimerHandler::REQUEST_TIMEOUT;
            break;
        default:
            timeout = 60U;
            break;
        }
        (void)timeout;
        
        int32_t res {-1};
        android::sp<GrpcReqData> reqData{nullptr};
        try
        {
            if (timeout <= static_cast<uint32_t>(INT32_MAX))
            {
                reqData = new GrpcReqData(GRPC_APP_TYPE::RMT_DIAG, GRPC_IF_TYPE::DCIF_RDG012, static_cast<int32_t>(timeout), region == 2U ? true : false, static_cast<google::protobuf::Message *>(mNotifyCollectionConditionUpdateResultRequest.get()), "");
                res = HttpManagerAdapter::getInstance()->sendGrpcMessage(reqData);
            }
        }
        catch (std::invalid_argument& e)
        {
            LOG_E("exception: %s", e.what());
        }

        if (res < 0)
        {
            LOG_W("Send NotifyCollectionConditionUpdateResultRequest failed");
            // DoOperationA();
        }
        else
        {
            LOG_D("Send NotifyCollectionConditionUpdateResultRequest success");
            switch (mRetryNotifyCollectionConditionUpdateResultRequestCounter)
            {
            case 0U:
                mNotifyCollectionConditionUpdateResultRequestTimer.setDuration(timeout, 0U);
                mNotifyCollectionConditionUpdateResultRequestTimer.start();
                mRetryNotifyCollectionConditionUpdateResultRequestCounter++;
                break;
            case 1U:
                mNotifyCollectionConditionUpdateResultRequestTimer.setDuration(timeout, 0U);
                mNotifyCollectionConditionUpdateResultRequestTimer.start();
                mRetryNotifyCollectionConditionUpdateResultRequestCounter++;
                LOG_D("Waiting for second time retry, timeout = %d", timeout);
                break;
            case 2U:
                mNotifyCollectionConditionUpdateResultRequestTimer.setDuration(timeout, 0U);
                mNotifyCollectionConditionUpdateResultRequestTimer.start();
                mRetryNotifyCollectionConditionUpdateResultRequestCounter++;
                LOG_D("Waiting for third time retry, timeout = %d", timeout);
                break;
            case 3U:
                LOG_D("Waiting for the last timeout = %d", timeout);
                mRetryNotifyCollectionConditionUpdateResultRequestCounter = 0U;
                mNotifyCollectionConditionUpdateResultRequest = nullptr;
                mNotifyCollectionConditionUpdateResultRequestTimer.stop();
                (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_COLLECTION_CONDITION_UPDLOAD_END)->sendToTarget();
                break;
            default:
                break;
            }
        }
    }
    LOG_I("Done");
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
    if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        LOG_W("verifyCenterRequestAllDtcSsr FAILED, collection_condition_id invalid");
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }

    if (inputData.has_schedule_information() == true)
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCentrerRequest(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        }

        // RDG30-R-1120
        const ScheduleType scheType {inputData.schedule_information().schedule_type()};
        if (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT) 
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(RDG_NOT_SUPPORTED_SCHEDULE_TYPE());
        }
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
    if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        LOG_W("verifyCenterRequestAllRob FAILED, collection_condition_id invalid");
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }

    if (inputData.has_schedule_information())
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCentrerRequest(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        }

        // RDG30-R-1120
        const ScheduleType scheType {inputData.schedule_information().schedule_type()};
        if (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT) 
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(RDG_NOT_SUPPORTED_SCHEDULE_TYPE());
        }
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
    if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }

    if (inputData.has_schedule_information() == true)
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCentrerRequest(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        }

        // RDG30-R-1120
        const ScheduleType scheType {inputData.schedule_information().schedule_type()};
        if (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT) 
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(RDG_NOT_SUPPORTED_SCHEDULE_TYPE());
        }
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

    for (CenterRequestRobSsrIter it {inputDataList.begin()}; it != inputDataList.end(); it++)
    {
        ErrorInformation errorInfo{};
        urc = verifyCenterRequestRobSsr(errorInfo, *it);
        if (urc == URC_FAILED)
        {
            errorInfoList.Add()->CopyFrom(errorInfo);
        }
    }
    return urc;
}

UpdateResultCode CollectionCondition::verifyCollectionConditionDirectCommand(ErrorInformation &errorInfo, const CollectionConditionDirectCommand &inputData)
{
    UpdateResultCode urc{URC_SUCCESS};
    if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }

    if (inputData.has_schedule_information() == true)
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCollectionCondition(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        }

        // RDG30-R-1120
        const ScheduleType scheType {inputData.schedule_information().schedule_type()};
        if ((scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IG_ON_TRIGGER_ROUTINE)
            && (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IG_OFF_TRIGGER_ROUTINE)
            && (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_PERIOD_TRIGGER_ROUTINE)) 
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(RDG_NOT_SUPPORTED_SCHEDULE_TYPE());
        }
    }

    // RDG30-R-1114
    if (inputData.direct_commands().size() > NUMBER_DIRECT_COMMAND_MAX)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(TOO_MANY_REPEATED_SETTING_VALUES("direct_commands"));
    }

    // RDG30-R-1107
    if ((inputData.update_type_collection_condition() <= UpdateTypeMultiple_MIN) || (inputData.update_type_collection_condition() > UpdateTypeMultiple_MAX))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("update_type_collection_condition"));
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
    if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }

    if (inputData.has_schedule_information() == true)
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCentrerRequest(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        }

        // RDG30-R-1120
        const ScheduleType scheType {inputData.schedule_information().schedule_type()};
        if (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT) 
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(RDG_NOT_SUPPORTED_SCHEDULE_TYPE());
        }
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
    if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }

    if (inputData.has_schedule_information() == true)
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCentrerRequest(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        }

        // RDG30-R-1120
        const ScheduleType scheType {inputData.schedule_information().schedule_type()};
        if ((scheType < ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT) 
            || (scheType > ScheduleType::ScheduleInformation_ScheduleType_ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT)) 
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(RDG_NOT_SUPPORTED_SCHEDULE_TYPE());
        }
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
    if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }

    // RDG30-R-1107
    if ((inputData.update_type_collection_condition() <= UpdateTypeSingle_MIN) || (inputData.update_type_collection_condition() > UpdateTypeSingle_MAX))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INCORRECT_UPDATE_TYPE());
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
    if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }

    // RDG30-R-1107
    if ((inputData.update_type_collection_condition() <= UpdateTypeSingle_MIN) || (inputData.update_type_collection_condition() > UpdateTypeSingle_MAX))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INCORRECT_UPDATE_TYPE());
    }

    if (inputData.has_schedule_information() == true)
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCollectionCondition(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        }

        // RDG30-R-1120
        const ScheduleType scheType {inputData.schedule_information().schedule_type()};
        if ((scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IMMEDIATE_IG_ON_ONE_SHOT)
            && (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_IG_ON_TRIGGER_ROUTINE)
            && (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_PERIOD_TRIGGER_ROUTINE)) 
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(RDG_NOT_SUPPORTED_SCHEDULE_TYPE());
        }
    }

    // RDG30-R-1116
    if (inputData.target_collection_data().size() > NUMBER_OF_TARGET_COLLECTION_DATA_MAX)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(TOO_MANY_REPEATED_SETTING_VALUES("target_collection_data"));
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
    if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }

    // RDG30-R-1107
    if ((inputData.update_type_collection_condition() <= UpdateTypeSingle_MIN) || (inputData.update_type_collection_condition() > UpdateTypeSingle_MAX))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INCORRECT_UPDATE_TYPE());
    }

    if (inputData.has_schedule_information())
    {
        const UpdateResultCode tempUrc {verifyScheduleInformationForCollectionCondition(errorInfo, inputData.schedule_information())};
        if (tempUrc == URC_FAILED)
        {
            urc = tempUrc;
        }

        // RDG30-R-1120
        const ScheduleType scheType {inputData.schedule_information().schedule_type()};
        if (scheType != ScheduleType::ScheduleInformation_ScheduleType_ST_PERIOD_TRIGGER_ROUTINE) 
        {
            urc = URC_FAILED;
            errorInfo.mutable_error_messages()->Add(RDG_NOT_SUPPORTED_SCHEDULE_TYPE());
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
    if (inputData.collection_condition_id() > 0xFFFFFFFFFFFFFFFEU)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(OUT_OF_RANGE("collection_condition_id"));
    }

    // RDG30-R-1107
    if ((inputData.update_type_collection_condition() <= UpdateTypeSingle_MIN) || (inputData.update_type_collection_condition() > UpdateTypeSingle_MAX))
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(INCORRECT_UPDATE_TYPE());
    }

    // RDG30-R-1280
    if (inputData.warning_property_table().size() > WARNING_PROPERTY_TABLE_SIZE_MAX)
    {
        urc = URC_FAILED;
        errorInfo.mutable_error_messages()->Add(TOO_MANY_REPEATED_SETTING_VALUES("warning_property_table"));
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
    LOG_I("rdg_active_flag = %s", DiagManagerAdapter::getInstance()->getRDGFlag() == 1U ? "TRUE" : "FALSE");
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
    if (collectionReq != nullptr) {
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
        LOG_I("==============================================");
        LOG_I("CenterRequestAllDtcSsr:");
        const std::shared_ptr<CenterRequestAllDtcSsr> centerRequestDtcSsr {getCenterRequestAllDtcSsr()};
        if (centerRequestDtcSsr != nullptr)
        {
            std::string centerRequestAllDtcSsrStr{};
            (void)google::protobuf::util::MessageToJsonString(*centerRequestDtcSsr, &centerRequestAllDtcSsrStr, option);
            printData(centerRequestAllDtcSsrStr);
        }
        else
        {
            LOG_I("Empty");
        }
        LOG_I("==============================================");
        LOG_I("CenterRequestAllRob:");
        const std::shared_ptr<CenterRequestAllRob> centerRequestRob {getCenterRequestAllRob()};
        if (centerRequestRob != nullptr)
        {
            std::string centerRequestAllRobStr{};
            (void)google::protobuf::util::MessageToJsonString(*centerRequestRob, &centerRequestAllRobStr, option);
            printData(centerRequestAllRobStr);
        }
        else
        {
            LOG_I("Empty");
        }
        LOG_I("==============================================");
        LOG_I("CenterRequestEcuInformation:");
        const std::shared_ptr<CenterRequestEcuInformation> cenReqEcuInfor {getCenterRequestEcuInformation()};
        if (cenReqEcuInfor != nullptr)
        {
            std::string centerRequestEcuInformationStr{};
            (void)google::protobuf::util::MessageToJsonString(*cenReqEcuInfor, &centerRequestEcuInformationStr, option);
            printData(centerRequestEcuInformationStr);
        }
        else
        {
            LOG_I("Empty");
        }
        LOG_I("==============================================");
        LOG_I("CenterRequestRobSsr:");
        const CenterRequestRobSsrList &cenReqRoBSSRList {getCenterRequestRobSsr()};
        for (CenterRequestRobSsrIter it {cenReqRoBSSRList.begin()}; it != cenReqRoBSSRList.end(); it++)
        {
            std::string str{};
            (void)google::protobuf::util::MessageToJsonString(*it, &str, option);
            printData(str);
        }
        LOG_I("==============================================");
        LOG_I("CenterRequestDirectCommand:");
        const CenterRequestDirectCommandList &cenReqDirectCommandList {getCenterRequestDirectCommand()};
        for (CenterRequestDirectCommandIter it {cenReqDirectCommandList.begin()}; it != cenReqDirectCommandList.end(); it++)
        {
            std::string str{};
            (void)google::protobuf::util::MessageToJsonString(*it, &str, option);
            printData(str);
        }
    }
}

void CollectionCondition::testReceivedCollectionConditionResponse() const
{
    const std::shared_ptr<GetCollectionConditionResponse> rootRes {std::make_shared<GetCollectionConditionResponse>()};
    /* Get file size*/
    uint32_t size{0U};
    ifstream file{GET_COLLECTION_CONDITION_RES_TEST_PATH, (ios::binary | ios::ate)};
    const int64_t tmpFileSize {file.tellg()};
    size = (tmpFileSize > 0) ? static_cast<uint32_t>(tmpFileSize) : 0U;
    file.close();
    LOG_I("CHECK size: %d", size);
    char_t raw_ch[size];
    uint8_t raw[size];
    (void)memset(&raw_ch[0], 0, size);
    (void)memset(&raw[0], 0, size);
    const FileHandleType handle {FileUtil::openFile(&GET_COLLECTION_CONDITION_RES_TEST_PATH[0], OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN)};
    if (handle != nullptr)
    {
        (void)FileUtil::ReadBinFromFile(handle, &raw[0], sizeof(raw));
        (void)memcpy(&raw_ch[0], &raw[0], size);
        (void)FileUtil::closeFile(handle);
        const std::string inputStrPiece{raw_ch, size};
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
        LOG_E("open file %s failed", &GET_COLLECTION_CONDITION_RES_TEST_PATH[0]);
    }
}

void CollectionCondition::testReceivedCollectionConditionResponseBinData() const
{
    /* Get file size*/
    GetCollectionConditionResponse rootRes {};
    if (FileUtil::isPathExist(GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH) == true) 
    {
        int32_t fd {-1};
        const FileHandleType handle {FileUtil::openFile(&GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH[0], OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN)};
        if(handle != nullptr)
        {
            fd = FileUtil::getFileDescriptor(handle);
        }

        if(fd == -1)
        {
            LOG_E("get data failed because can't open file %s", &GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH[0]);
        }
        else
        {
            LOG_D("Get %s, File Descriptor = %d", &GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH[0], fd);
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
        LOG_E("file %s not exist", &GET_COLLECTION_CONDITION_RES_BIN_TEST_PATH[0]);
    }
}

std::shared_ptr<CenterRequestAllDtcSsr> CollectionCondition::getCenterRequestAllDtcSsr(void) noexcept
{
    return mCenterRequestAllDtcSsr;
}

std::shared_ptr<CenterRequestAllRob> CollectionCondition::getCenterRequestAllRob(void) noexcept
{
    return mCenterRequestAllRob;
}

std::shared_ptr<CenterRequestEcuInformation> CollectionCondition::getCenterRequestEcuInformation(void) noexcept
{
    return mCenterRequestEcuInformation;
}

const CenterRequestRobSsrList &CollectionCondition::getCenterRequestRobSsr(void) const noexcept
{
    return mCenterRequestsRobSsr;
}

const CenterRequestDirectCommandList &CollectionCondition::getCenterRequestDirectCommand(void) const noexcept
{
    return mCenterRequestDirectCommand;
}

error_t CollectionCondition::saveCollectionConditionDiagCommon(const CollectionConditionDiagCommon &obj)
{
    const error_t ret {DataModel<CollectionConditionDiagCommon>::save(COLLECTION_CONDITION_DIAG_COMMON_DB_NAME, obj)};
    if (ret == E_OK)
    {
        mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->set_common_diag_collection_condition_id(obj.collection_condition_id());
    }
    return ret;
}

std::shared_ptr<CollectionConditionDiagCommon> CollectionCondition::getCollectionConditionDiagCommon(void) const
{
    return DataModel<CollectionConditionDiagCommon>::get(COLLECTION_CONDITION_DIAG_COMMON_DB_NAME);
}

error_t CollectionCondition::saveCollectionConditionRobRobSsrDidEvent(const CollectionConditionRobRobSsrDidEvent &obj)
{
    const error_t ret {DataModel<CollectionConditionRobRobSsrDidEvent>::save(COLLECTION_CONDITION_ROB_SSR_DID_DB_NAME, obj)};
    if (ret == E_OK)
    {
        mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->set_rob_rob_ssr_did_event_collection_condition_id(obj.collection_condition_id());
    }
    return ret;
}

std::shared_ptr<CollectionConditionRobRobSsrDidEvent> CollectionCondition::getCollectionConditionRobRobSsrDidEvent(void) const
{
    return DataModel<CollectionConditionRobRobSsrDidEvent>::get(COLLECTION_CONDITION_ROB_SSR_DID_DB_NAME);
}

error_t
CollectionCondition::saveCollectionConditionEcuInformation(const CollectionConditionEcuInformation &obj)
{
    const error_t ret {DataModel<CollectionConditionEcuInformation>::save(COLLECTION_CONDITION_ECU_INFO_DB_NAME, obj)};
    if (ret == E_OK)
    {
        mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->set_ecu_information_collection_condition_id(obj.collection_condition_id());
    }
    return ret;
}

std::shared_ptr<CollectionConditionEcuInformation> CollectionCondition::getCollectionConditionEcuInformation(void) const
{
    return DataModel<CollectionConditionEcuInformation>::get(COLLECTION_CONDITION_ECU_INFO_DB_NAME);
}

error_t
CollectionCondition::saveCollectionConditionWarningInformation(const CollectionConditionWarningInformation &obj)
{
    const error_t ret {DataModel<CollectionConditionWarningInformation>::save(COLLECTION_CONDITION_WARN_INFO_DB_NAME, obj)};
    if (ret == E_OK)
    {
        mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->set_warning_information_collection_condition_id(obj.collection_condition_id());
    }
    return ret;
}

std::shared_ptr<CollectionConditionWarningInformation> CollectionCondition::getCollectionConditionWarningInformation(void) const
{
    return DataModel<CollectionConditionWarningInformation>::get(COLLECTION_CONDITION_WARN_INFO_DB_NAME);
}

error_t CollectionCondition::saveCollectionConditionDirectCommand(const CollectionConditionDirectCommand &obj) const
{
    const uint64_t collectionConditionId {obj.collection_condition_id()};
    const std::string collectionConditionIdStr {std::to_string(collectionConditionId)};
    std::string path {"CollectionConditionDirectCommand_"};
    (void)path.append(collectionConditionIdStr);
    return DataModel<CollectionConditionDirectCommand>::save(path, obj);
}

std::shared_ptr<CollectionConditionDirectCommand> CollectionCondition::getCollectionConditionDirectCommand(const uint64_t collectionConditionId) const
{
    const std::string collectionConditionIdStr {std::to_string(collectionConditionId)};
    std::string path {"CollectionConditionDirectCommand_"};
    (void)path.append(collectionConditionIdStr);
    return DataModel<CollectionConditionDirectCommand>::get(path);
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

std::shared_ptr<GetCollectionConditionRequest> CollectionCondition::getGetCollectionConditionRequest() const
{
    return DataModel<GetCollectionConditionRequest>::get(GET_COLLECTION_CONDITION_REQ_DB_NAME);
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
        CenterRequestRobSsrList newCenterRequestsRobSsrList{};
        for (CenterRequestRobSsrIter it {inputDataList.begin()}; it != inputDataList.end(); it++)
        {
            if (it->collection_condition_id() != 0U) {
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
                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                    CenterRequestRobSsr* const newData {newCenterRequestsRobSsrList.Add()};
                    newData->CopyFrom(*it);
                    // RDG30-R-1282
                    if ((newData->schedule_information().schedule_type() != ScheduleType_PERIOD_TRIGGER_ROUTINE) && (newData->schedule_information().has_schedule_interval() == true))
                    {
                        LOG_D("schedule_type is not “Periodic Trigger Routine” but the schedule interval (schedule_interval) is specified");
                        newData->mutable_schedule_information()->clear_schedule_interval();
                        LOG_D("Discard schedule_interval success");
                    }
                }
            }
        }

        if (urc == URC_SUCCESS)
        {
            mCenterRequestsRobSsr.Clear();
            mCenterRequestsRobSsr.CopyFrom(newCenterRequestsRobSsrList);
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
        CenterRequestDirectCommandList newCenterRequestDirectCommandList{};
        for (CenterRequestDirectCommandIter it {inputDataList.begin()}; it != inputDataList.end(); it++)
        {
            if (it->collection_condition_id() != 0U) {
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
                    DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::COLLECTION_CONDITIONS);
                    CenterRequestDirectCommand * const newData {newCenterRequestDirectCommandList.Add()};
                    newData->CopyFrom(*it);
                    // RDG30-R-1121
                    if ((newData->schedule_information().schedule_type() != ScheduleType_PERIOD_TRIGGER_ROUTINE) && (newData->schedule_information().has_schedule_interval() == true))
                    {
                        newData->mutable_schedule_information()->clear_schedule_interval();
                    }
                }
            }
        }

        if (urc == URC_SUCCESS)
        {
            mCenterRequestDirectCommand.Clear();
            mCenterRequestDirectCommand.CopyFrom(newCenterRequestDirectCommandList);
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
            if (it->collection_condition_id() != 0U) {
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
                    else
                    {
                        DiagManagerAdapter::getInstance()->selfDiagSuccessReadNotification(DiagManagerAdapter::RD_SCHEDULE_TRIGGER);
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
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());              
            }
            else if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED)
            {
                error = DataModel<CollectionConditionDiagCommon>::clear(COLLECTION_CONDITION_DIAG_COMMON_DB_NAME);
                if (error == E_OK)
                {
                    const uint64_t collectionConditionId {inputData.collection_condition_id()};
                    uint8_t colId_ptr[sizeof(collectionConditionId)];
                    (void)memcpy(&colId_ptr[0], &collectionConditionId, sizeof(colId_ptr));
                    const android::sp<::Buffer> colId_sp{new ::Buffer()};

                    colId_sp->setTo(&colId_ptr[0], sizeof(colId_ptr));

                    (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_DELETE_COLLECTION_CONDITION, colId_sp)->sendToTarget();
                }
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
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
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
                    Uint64List *const collectionConditionIdList {mGetCollectionConditionRequest->mutable_collection_condition_id_stored_in_vehicle()->mutable_direct_command_collection_condition_ids()};
                    for (ConstUint64Iter it{collectionConditionIdList->cbegin()}; it != collectionConditionIdList->cend(); it++)
                    {
                        if (*it == collectionConditionId)
                        {
                            (void)collectionConditionIdList->erase(it);
                            break;
                        }
                    }

                    uint8_t colId_ptr[sizeof(collectionConditionId)];
                    (void)memcpy(&colId_ptr[0], &collectionConditionId, sizeof(colId_ptr));
                    const android::sp<::Buffer> colId_sp{new ::Buffer()};

                    colId_sp->setTo(&colId_ptr[0], sizeof(colId_ptr));

                    (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_DELETE_COLLECTION_CONDITION, colId_sp)->sendToTarget();
                    
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
                    DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                    LOG_D("Update Data for collection condition ID: %lld to EMMC", inputData.collection_condition_id());
                    LOG_D("Update collection condition ID: %lld for the nex collection condition download request.", inputData.collection_condition_id());
                    if ((error == E_OK) && (oldData != nullptr))
                    {
                        (void)mDeletedCollectionConditionIds.push_back(static_cast<uint64_t>(oldData->collection_condition_id()));
                        (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(oldData->collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_DID_EVENT)));
                    }
                    break;
                }
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED:
                {
                    error = DataModel<CollectionConditionRobRobSsrDidEvent>::clear(COLLECTION_CONDITION_ROB_SSR_DID_DB_NAME);
                    if (error == E_OK)
                    {
                        const uint64_t collectionConditionId {inputData.collection_condition_id()};
                        /* Backup
                        uint8_t colId_ptr[sizeof(collectionConditionId)];
                        (void)memcpy(&colId_ptr[0], &collectionConditionId, sizeof(colId_ptr));
                        const android::sp<::Buffer> colId_sp{new ::Buffer()};

                        colId_sp->setTo(&colId_ptr[0], sizeof(colId_ptr));
                        */

                        // BK (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_DELETE_COLLECTION_CONDITION, colId_sp)->sendToTarget();

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
            LOG_D("Save new collection condition Data to EMMC");
            LOG_D("Save new collection condition ID for the nex collection condition download request.");
            if (updateType == UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED) {
                error = this->saveCollectionConditionRobRobSsrDidEvent(newData);
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(newData.collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_DID_EVENT)));
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
                    DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                    if ((error == E_OK) && (oldData != nullptr))
                    {
                        (void)mDeletedCollectionConditionIds.push_back(static_cast<uint64_t>(oldData->collection_condition_id()));
                        (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(oldData->collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_ECU_INFORMATION)));
                    }
                    break;
                }
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED:
                {
                    error = DataModel<CollectionConditionEcuInformation>::clear(COLLECTION_CONDITION_ECU_INFO_DB_NAME);
                    if (error == E_OK)
                    {
                        
                        const uint64_t collectionConditionId {inputData.collection_condition_id()};
                        /* Backup
                        uint8_t colId_ptr [sizeof(collectionConditionId)];
                        (void)memcpy(&colId_ptr[0], &collectionConditionId, sizeof(colId_ptr));
                        const android::sp<::Buffer> colId_sp{new ::Buffer()};

                        colId_sp->setTo(&colId_ptr[0], sizeof(colId_ptr));
                        */

                        if (oldData != nullptr)
                        {
                            (void)mDeletedCollectionConditionIds.push_back(collectionConditionId);
                        }

                        // BK (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_DELETE_COLLECTION_CONDITION, colId_sp)->sendToTarget();
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
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(newData.collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_ECU_INFORMATION)));
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
                    DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                    if ((error == E_OK) && (oldData != nullptr))
                    {
                        (void)mDeletedCollectionConditionIds.push_back(static_cast<uint64_t>(oldData->collection_condition_id()));
                        (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(oldData->collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_WARNING_INFORMATION)));
                    }
                    break;
                }
                case UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_DELETED:
                {
                    error = DataModel<CollectionConditionWarningInformation>::clear(COLLECTION_CONDITION_ECU_INFO_DB_NAME);
                    if (error == E_OK)
                    {
                        const uint64_t collectionConditionId {inputData.collection_condition_id()};
                        /* backup
                        uint8_t colId_ptr [sizeof(collectionConditionId)];
                        (void)memcpy(&colId_ptr[0], &collectionConditionId, sizeof(colId_ptr));
                        const android::sp<::Buffer> colId_sp{new ::Buffer()};

                        colId_sp->setTo(&colId_ptr[0], sizeof(colId_ptr));
                        */

                        if (oldData != nullptr)
                        {
                            (void)mDeletedCollectionConditionIds.push_back(collectionConditionId);
                        }

                        // BK (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_DELETE_COLLECTION_CONDITION, colId_sp)->sendToTarget();
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
                DiagManagerAdapter::getInstance()->storeCollectionConditionId(newData.collection_condition_id());
                (void)mUpdatedCollectionConditionIds.push_back(std::make_pair(static_cast<uint64_t>(newData.collection_condition_id()), static_cast<uint8_t>(CollectionConditionType::CC_WARNING_INFORMATION)));
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
        errorInfoList.Add()->CopyFrom(errorInfo);
    }
    return urc;
}

const CollectionConditionDirectCommandList &CollectionCondition::getCollectionConditionDirectCommandRequest(void)
{

    // get DB --> get ID list ->
    mCollectionConditionDirectCommandList.Clear();
    const std::shared_ptr<GetCollectionConditionRequest> collectionReq {getGetCollectionConditionRequest()};
    if (collectionReq != nullptr)
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

void CollectionCondition::DoOperationB(void)
{
    // Do Operation B: Restart the collection condition update sequence from DCIF-RDG010 request in accordance with retry rules.
    LOG_I("Do Operation B");
    if (mRetryGetCollectionConditionCounter < (MAX_RETRY+1U))
    {
        if (mRetryGetCollectionConditionCounter == 1U) {
            LOG_D("Waiting for first time retry");
            mSendGetCollectionConditionRetryTimer.setDuration(TimerHandler::FIRST_RETRY_TIMEOUT, 0U);
            mSendGetCollectionConditionRetryTimer.start();
        }
    }
}

void CollectionCondition::DoOperationA(void) const
{
    LOG_I("Do Operation A");
}

void CollectionCondition::handleGrpcClientErrorEvent(const GRPC_RESULT httpResultCode, const grpc::StatusCode grpcCode)
{
    switch (httpResultCode)
    {
        case GRPC_RESULT::SEND_FAIL_NOT_FOUND_RECEIVER:
            (void)HttpManagerAdapter::getInstance()->registerReceiver();
            break;
        case GRPC_RESULT::SEND_FAIL_DATA_DISCONNECTED:
            LOG_I("GRPC response SEND_FAIL_DATA_DISCONNECTED, DoOperationA: retrying when the connection is restored");
            mGetCollectionConditionRequestOpA = true;
            mSendGetCollectionConditionRetryTimer.stop();
            mRetryGetCollectionConditionCounter = 0U;
            break;
        case GRPC_RESULT::SEND_FAIL:
        case GRPC_RESULT::SEND_FAIL_INVALID_PARAMETER:
        case GRPC_RESULT::SEND_FAIL_GRPC_SETTING:
        case GRPC_RESULT::SEND_FAIL_CONNECT_FAIL:
        case GRPC_RESULT::SEND_FAIL_TIMEOUT:
        {
            LOG_E("Send GetCollectionCondition request failed");
            if ((grpcCode == grpc::StatusCode::UNKNOWN)
                    || (grpcCode == grpc::StatusCode::DEADLINE_EXCEEDED)
                    || (grpcCode == grpc::StatusCode::UNIMPLEMENTED)
                    || (grpcCode == grpc::StatusCode::INTERNAL)
                    || (grpcCode == grpc::StatusCode::UNAVAILABLE)
                    || (grpcCode == grpc::StatusCode::DATA_LOSS))
            {
                LOG_E("Received Server error, httpResultCode = %d, gRpcStatusCode = %d", httpResultCode, grpcCode);
                if(grpcCode == grpc::StatusCode::DEADLINE_EXCEEDED) {
                    LOG_I("Save selfDiagNoCenterResponse");
                    DiagManagerAdapter::getInstance()->selfDiagNoCenterResponse();
                }
                DoOperationB();
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
                LOG_E("Received client error, httpResultCode = %d, gRpcStatusCode = %d", httpResultCode, grpcCode);
                mGetCollectionConditionRequestOpA = true;
                DoOperationA();
            }
            else
            {
                //do nothing
            }

            break;
        }
        default:
            LOG_E("Unknown GRPC_RESULT code = %d", static_cast<uint8_t>(httpResultCode));
            break;
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
    if (mGetCollectionConditionRequestOpA)
    {
        mRetryGetCollectionConditionCounter = 0U;
        // mTriggerType = DiagTrigger::DiagTriggerType::CENTER_TRIGGER;
        (void)mCollectionConditionHandler->obtainMessage(MainHandler::CMD_SEND_GET_COLLECTION_CONDITION_REQUEST)->sendToTarget();
        mGetCollectionConditionRequestOpA = false;
    }

    if (mNotificationCollectionConditionUpdateResultOpA)
    {
        mNotificationCollectionConditionUpdateResultOpA = false;
    }

}

void CollectionCondition::TimerHandler::handlerFunction(const int32_t timerId)
{
    switch (timerId)
    {
    case IG_ON_STATE_MAINTAINED_CHECK_ID:
        LOG_I("IG_ON_STATE_MAINTAINED_CHECK_ID");
        (void)mCollectionCondition.mCollectionConditionHandler->obtainMessage(MainHandler::CMD_SEND_GET_COLLECTION_CONDITION_REQUEST)->sendToTarget();
        break;
    case RETRY_TIMER_ID:
    {
        LOG_I("RETRY_TIMER_ID");
        // Do not send if network out of range
        if (mCollectionCondition.mGetCollectionConditionRequestOpA == false)
        {
            (void)mCollectionCondition.mCollectionConditionHandler->obtainMessage(MainHandler::CMD_SEND_GET_COLLECTION_CONDITION_REQUEST)->sendToTarget();
        }
        break;
    }
    case NOTIFICATION_UPDATE_RESULT_RETRY_TIMER_ID:
    {
        LOG_I("NOTIFICATION_UPDATE_RESULT_RETRY_TIMER_ID");
        if (mCollectionCondition.mNotificationCollectionConditionUpdateResultOpA == false)
        {
            (void)mCollectionCondition.mCollectionConditionHandler->obtainMessage(MainHandler::CMD_SEND_NOTIFY_COLLECTION_CONDITION_UPDATE_RESULT_REQUEST)->sendToTarget();
        }
        break;
    }
    default:
        break;
    }
}

std::vector<uint64_t> CollectionCondition::getDeletedCollectionConditionIds(void) const noexcept
{
    return mDeletedCollectionConditionIds;
}

std::vector<std::pair<uint64_t, uint8_t>> CollectionCondition::getUpdatedCollectionConditionIds(void) const noexcept
{
    return mUpdatedCollectionConditionIds;
}

}
