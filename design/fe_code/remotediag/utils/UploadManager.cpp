#include <google/protobuf/util/json_util.h>
#include "UploadManager.h"
#include "services/RegionManagerAdapter.h"
#include "services/ApplicationManagerAdapter.h"

namespace rdgapp {

namespace interface = vccomif::rdg::v1::interfaces;

android::sp<UploadManager> UploadManager::mUploadManager {nullptr};

UploadManager::UploadManager(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper):
        android::RefBase()
        , mApp(app)
        , mHandler(new MainHandler(privateLooper, *this))
        , mMspTaskQueue(new UploadPrioQueue())
        , mTimerHandler(std::make_shared<TimerHandler>(*this))
        , mTimerHandler_Uploading(std::make_shared<TimerHandler_Uploading>(*this))
        , mpTimerHandler_Retry(std::make_shared<TimerHandler_Retry>(*this))
{
    mUploadManager = this;
    mTickTimer = new Timer(mTimerHandler.get(), TimerHandler::ID_IG_OFF_STOP_UPLOADING);
    mTickTimer->setDuration(IG_OFF_STOP_UPLOADING_DURATION, 0U);
    (void)mHandler->obtainMessage(MainHandler::CMD_INIT_UPLOADMANAGER)->sendToTarget();
    m_dtc_rob_max_storage = 0U;
    m_ssr_max_storage = 0U;
    m_directcommand_max_storage = 0U;
    m_robssr_max_storage = 0U;
    m_warning_max_storage = 0U;
    m_ecu_infor_max_storage = 0U;
    m_last_max_storage = 0U;
    m_notification_max_storage = 0U;
    m_error_max_storage = 0U;
    m_ddr_max_storage = 0U;
    m_coll_update_result_noti_max_storage = 0U;
    mCounterValue = 0U;
}

UploadManager::~UploadManager() = default;

android::sp<UploadManager> UploadManager::getInstance()
{
    if (mUploadManager == nullptr)
    {
        LOG_I("mUploadManager is null");
    }
    return mUploadManager;
}

void UploadManager::printData(const std::string data) const
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

void UploadManager::init()
{
    static constexpr char_t UPLOADMGR_DB_STRUCT[]{"(id INTEGER PRIMARY KEY\
                                                , prio INTEGER\
                                                , uploadfiletype INTEGER\
                                                , filesize INTEGER\
                                                , uploadpatch TEXT\
                                                )"};
    static constexpr char_t UPLOADMGR_DB_NAME[]{"upload_manager"};
    LOG_I("Init UploadManager");
    constexpr uint32_t sqlite_rw {static_cast<uint32_t>(SQLITE_OPEN_READWRITE)};
    constexpr uint32_t sqlite_c {static_cast<uint32_t>(SQLITE_OPEN_CREATE)};
    constexpr int32_t timeoutVal{static_cast<int32_t>(sqlite_rw | sqlite_c)};
    mUploadingDB = std::unique_ptr<Database>(new Database(
                                        UPLOADMGR_DB_NAME
                                        , (std::string(DATA_PATH) + std::string(UPLOADMGR_DB_NAME)).c_str()
                                        , UPLOADMGR_DB_STRUCT
                                        , timeoutVal));
    const int32_t ret {mUploadingDB->init()};
    if(ret < 0){
        LOG_E("ret = %d is out of range of uint32_t");
    }
    if(ret != SQLITE_OK)
    {
        LOG_E("UploadMgr DB init failed, reason = %s", Database::errorToString(ret).c_str());
    } else {
        LOG_I("UploadMgr DB init success");
    }
    (void)grpcValues.insert({10U, GRPC_IF_TYPE::DCIF_CAN010});
    (void)grpcValues.insert({11U, GRPC_IF_TYPE::DCIF_CAN012});
    (void)grpcValues.insert({12U, GRPC_IF_TYPE::DCIF_CAN020});
    (void)grpcValues.insert({13U, GRPC_IF_TYPE::DCIF_CAN030});
    (void)grpcValues.insert({14U, GRPC_IF_TYPE::DCIF_CAN040});
    (void)grpcValues.insert({15U, GRPC_IF_TYPE::DCIF_CAN050});
    (void)grpcValues.insert({16U, GRPC_IF_TYPE::DCIF_CAN060});
    (void)grpcValues.insert({17U, GRPC_IF_TYPE::DCIF_CAN090});
    (void)grpcValues.insert({18U, GRPC_IF_TYPE::DCIF_RDG010});
    (void)grpcValues.insert({19U, GRPC_IF_TYPE::DCIF_RDG012});
    (void)grpcValues.insert({20U, GRPC_IF_TYPE::DCIF_RDG030});
    (void)grpcValues.insert({21U, GRPC_IF_TYPE::DCIF_RDG040});
    (void)grpcValues.insert({22U, GRPC_IF_TYPE::DCIF_RDG050});
    (void)grpcValues.insert({23U, GRPC_IF_TYPE::DCIF_RDG060});
    (void)grpcValues.insert({24U, GRPC_IF_TYPE::DCIF_RDG070});
    (void)grpcValues.insert({25U, GRPC_IF_TYPE::DCIF_RDG080});
    (void)grpcValues.insert({26U, GRPC_IF_TYPE::DCIF_RDG100});
    (void)grpcValues.insert({27U, GRPC_IF_TYPE::DCIF_RDG120});
    (void)grpcValues.insert({28U, GRPC_IF_TYPE::DCIF_RDG130});
    (void)grpcValues.insert({29U, GRPC_IF_TYPE::DCIF_RDG160});
    (void)grpcValues.insert({30U, GRPC_IF_TYPE::DCIF_RDG170});
    // std::vector<CommonDefine::UploadFileAttribute> backupUpload;

    // mUploadingDB->getUploadDB(backupUpload);

    // for(std::vector<CommonDefine::UploadFileAttribute>::iterator ptr {backupUpload.begin()}; ptr != backupUpload.end(); ptr++) {
    //     /*Check if file is exist*/
    //     const std::string old_tmp{std::string(DATA_PATH) + ptr->filePath};

    //     if(FileUtil::isPathExist(old_tmp.c_str()) == true) {
    //         LOG_I("%s exist", old_tmp.c_str());
    //         const uint32_t uploadId {genRequestId()};
    //         const android::sp<UploadTask> task {new UploadTask(uploadId)};
    //         task->setUploadFileType(static_cast<GRPC_IF_TYPE>(ptr->fileType));
    //         const std::string file_dir {std::to_string(uploadId) + "_" + ptr->filePath};
    //         const std::string new_tmp{std::string(DATA_PATH) + file_dir};
    //         if (std::rename(old_tmp.c_str(), new_tmp.c_str()) == 0) {
    //             LOG_I("File renamed successfully");
    //             task->setUploadPatch(file_dir);
    //         } else {
    //             LOG_I("Error renaming file");
    //             task->setUploadPatch(ptr->filePath);
    //         }
            
    //         /*Set priority*/
    //         task->setUploadPrio(ptr->priority);
    //         task->setFileSize(ptr->fileSize);
    //         requestUploadTask(task); 
    //     } else {
    //         LOG_I("Delete unexist file");
    //         mUploadingDB->deleteUploadDB(ptr->filePath);
    //     }
 
    // }
}

void UploadManager::sendFile(const android::sp<UploadTask> task) {
    LOG_I("send file");
    (void)mHandler->obtainMessage(MainHandler::MSG_UPLOAD_FILE, task)->sendToTarget();
}

void UploadManager::requestUploadTask(const android::sp<UploadTask> task) {
    LOG_I("send file");
    (void)mHandler->obtainMessage(MainHandler::MSG_ADD_UPLOAD_TASK, task)->sendToTarget();
}

void UploadManager::addTask(const android::sp<UploadTask> &pUploadTask) {
    LOG_I("Upload manager add upload task");
    // android::Mutex::Autolock autolock(gMutexTaskQueue);
    if (pUploadTask != nullptr) {
        (void)mUploadingDB->saveUploadDB(pUploadTask->getUploadPrio()
            , static_cast<uint32_t>(pUploadTask->getUploadFileType())
            , static_cast<uint64_t>(pUploadTask->getFileSize())
            , pUploadTask->getUploadPatch());
        if ((mMspTaskQueue != nullptr) && mMspTaskQueue->insertQueue(pUploadTask)) {
            triggerUpload();
            LOG_I("Add task successfully");
        }
        LOG_I("Add task end");
    }
}

void UploadManager::addTask_delayed(const android::sp<UploadTask> &pUploadTask) {
    if (pUploadTask != nullptr) {
        const uint64_t uploadId {pUploadTask->getUploadId()};
        if(uploadId <= static_cast<uint64_t>(INT64_MAX)){
            if(mRetryCheck[static_cast<int64_t>(uploadId)] == false){
                LOG_I("Do retry Upload Id: %lld", uploadId);
                if ((mMspTaskQueue != nullptr) && mMspTaskQueue->insertQueue(pUploadTask)) {
                    triggerUpload();
                    LOG_I("Add task successfully");
                } else {
                    LOG_I("Add upload task fail");
                }
            } else {
                LOG_I("Dont retry: %lld", uploadId);
                LOG_I("Check Retry map size before: %d", mRetryCheck.size());
                (void)mRetryCheck.erase(static_cast<int64_t>(uploadId));
                LOG_I("Check Retry map size after: %d", mRetryCheck.size());
            }
        }
        else{
            LOG_E("uploadId = %llu is out of range of int64_t", uploadId);
        }
    }
    else{
        LOG_I("pUploadTask is nullptr");
    }
}

void UploadManager::discardAllTask() {
    LOG_I("Start");
    mMspTaskQueue->clearQueue();
    LOG_I("End");
}

void UploadManager::deletePendingTask() {
    LOG_D("Start delete pending task");
    /*Clear mMspTaskQueue*/
    while((mMspTaskQueue != nullptr) && (mMspTaskQueue->isQueueEmpty() != true)) {
        const android::sp<UploadTask> upload_data {mMspTaskQueue->getTop()};
        if(upload_data != nullptr){
            (void)mMspTaskQueue->pop();
            upload_data->deteleUploadFile();
            (void)mUploadingDB->deleteUploadDB(upload_data->getUploadPatch());
        } else {
            LOG_E("upload_data is nullptr");
        }
    }
    /* Delete task is wait response */
    for (std::map<int32_t, android::sp<UploadTask>>::iterator it {mSentTask.begin()}; it != mSentTask.end(); ++it) {
        it->second->deteleUploadFile();
    }
    mSentTask.clear();
    /* Delete task is wait retry */
    for (std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>>::iterator it {mTimers_RetryMap.begin()}; it != mTimers_RetryMap.end(); ++it) {
        const android::sp<Timer> ptimer {it->second.first};
        const android::sp<UploadTask> task {it->second.second};
        ptimer->stop();
        task->deteleUploadFile();
    }
    mTimers_RetryMap.clear();
    /*Clear pending saved file*/
    LOG_D("Delete pending task successful");
}

void UploadManager::triggerUpload() {
    LOG_I("triggerUpload");
    if (mMspTaskQueue->isQueueEmpty() != true) {
        /*Check APP priority by application mgr*/
        const int32_t res1 {ApplicationManagerAdapter::getInstance()->queryActionForFeature("remotediag")};
        LOG_I("Check queryActionForFeature: %d", res1);
        if(res1 == FeatureAction::LAUNCH) {
            LOG_I("No upload prio restirct");
            /*Set active app*/
            const int32_t res2 {ApplicationManagerAdapter::getInstance()->setFeatureStatus("remotediag", "remotediag", true)};
            if(res2 != E_OK) {
                LOG_I("Set active RometeDiag Fail");
            } else {
                LOG_I("Remotediag is Active");
                (void)mHandler->obtainMessage(MainHandler::MSG_ID_DO_UPLOAD)->sendToTarget();
            }
        } else {
            LOG_I("Upload restricted due to prio");
            const android::AutoMutex _l{mLock_OpA};
            /*Pop task from queue*/
            const android::sp<UploadTask> upload_data {mMspTaskQueue->getTop()};
            mqOperationA.push_back(upload_data);
            LOG_D("Check mqOperationA size: %d", mqOperationA.size());
            (void)mMspTaskQueue->pop();
            (void)mHandler->obtainMessage(MainHandler::MSG_ID_TRIGGER_UPLOAD)->sendToTarget();
        }
    }
}

void UploadManager::doUpload() {
    LOG_I("Do upload");
    /*Get upload task from queue*/
    const android::sp<UploadTask> upload_data {mMspTaskQueue->getTop()};
    (void)mMspTaskQueue->pop();
    
    GRPC_IF_TYPE uploadFileType {GRPC_IF_TYPE::DCIF_MAX};
    if(upload_data != nullptr) {
        uploadFileType = upload_data->getUploadFileType();
    }
    
    std::shared_ptr<google::protobuf::Message> messRequest {nullptr};
    switch(uploadFileType) {
        case GRPC_IF_TYPE::DCIF_RDG010:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG010");
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG012:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG012");
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG030:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG030");
            /*Load upload messRequest from file*/
            const uint8_t RDGFlag{DiagManagerAdapter::getInstance()->getRDGFlag()};
            const uint8_t DTCFlag{DiagManagerAdapter::getInstance()->getDTCFlag()};
            if((RDGFlag == 1U) && (DTCFlag == 1U)) {
                    messRequest = DataModel<UploadDtcDataRequest>::get(upload_data->getUploadPatch());
            } else {
                LOG_I("DTC flag off. Don't upload DCIF_RDG030");
                const error_t err{DataModel<UploadDtcDataRequest>::clear(upload_data->getUploadPatch())};
                if(err == E_OK){
                    LOG_D("Clear %s successfull", upload_data->getUploadPatch().c_str());
                    updateStorage(upload_data, false);
                } else {
                    LOG_D("Clear %s FAIL", upload_data->getUploadPatch().c_str());
                }
            }
            (void)DTCFlag;
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG040:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG040");
            const uint8_t RDGFlag{DiagManagerAdapter::getInstance()->getRDGFlag()};
            const uint8_t SSRFlag{DiagManagerAdapter::getInstance()->getSSRFlag()};
            if((RDGFlag == 1U) && (SSRFlag == 1U)) {
                    messRequest = DataModel<UploadSsrDataRequest>::get(upload_data->getUploadPatch());
            } else {
                LOG_I("SSR flag off. Don't upload DCIF_RDG040");
                const error_t err{DataModel<UploadSsrDataRequest>::clear(upload_data->getUploadPatch())};
                if(err == E_OK){
                    LOG_D("Clear %s successfull", upload_data->getUploadPatch().c_str());
                    updateStorage(upload_data, false);
                } else {
                    LOG_D("Clear %s FAIL", upload_data->getUploadPatch().c_str());
                }
            }
            (void)SSRFlag;
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG050:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG050");
            const uint8_t RDGFlag{DiagManagerAdapter::getInstance()->getRDGFlag()};
            const uint8_t RoBflag{DiagManagerAdapter::getInstance()->getRoBflag()};
            if((RDGFlag == 1U) && (RoBflag == 1U)) {
                    messRequest = DataModel<UploadRobDataRequest>::get(upload_data->getUploadPatch());
            } else {
                LOG_I("ROB flag off. Don't upload DCIF_RDG050");
                const error_t err{DataModel<UploadRobDataRequest>::clear(upload_data->getUploadPatch())};
                if(err == E_OK){
                    LOG_D("Clear %s successfull", upload_data->getUploadPatch().c_str());
                    updateStorage(upload_data, false);
                } else {
                    LOG_D("Clear %s FAIL", upload_data->getUploadPatch().c_str());
                }
            }
            (void)RoBflag;
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG060:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG060");
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U)) {
                messRequest = DataModel<UploadRobSsrDataRequest>::get(upload_data->getUploadPatch());
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG060");
                const error_t err{DataModel<UploadRobSsrDataRequest>::clear(upload_data->getUploadPatch())};
                if(err == E_OK){
                    LOG_D("Clear %s successfull", upload_data->getUploadPatch().c_str());
                    updateStorage(upload_data, false);
                } else {
                    LOG_D("Clear %s FAIL", upload_data->getUploadPatch().c_str());
                }
            }
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG070:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG070");

            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U)) {
                messRequest = DataModel<UploadDirectCommandDataRequest>::get(upload_data->getUploadPatch());
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG070");
                const error_t err{DataModel<UploadDirectCommandDataRequest>::clear(upload_data->getUploadPatch())};
                if(err == E_OK){
                    LOG_D("Clear %s successfull", upload_data->getUploadPatch().c_str());
                    updateStorage(upload_data, false);
                } else {
                    LOG_D("Clear %s FAIL", upload_data->getUploadPatch().c_str());
                }
            }
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG080:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG080");
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U)) {
                messRequest = DataModel<UploadWarningInformationRequest>::get(upload_data->getUploadPatch());
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG080");
                const error_t err{DataModel<UploadWarningInformationRequest>::clear(upload_data->getUploadPatch())};
                if(err == E_OK){
                    LOG_D("Clear %s successfull", upload_data->getUploadPatch().c_str());
                    updateStorage(upload_data, false);
                } else {
                    LOG_D("Clear %s FAIL", upload_data->getUploadPatch().c_str());
                }                
            }
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG100:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG100");
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U)) {
                messRequest = DataModel<UploadEcuInformationRequest>::get(upload_data->getUploadPatch());
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG100");
                const error_t err{DataModel<UploadEcuInformationRequest>::clear(upload_data->getUploadPatch())};
                if(err == E_OK){
                    LOG_D("Clear %s successfull", upload_data->getUploadPatch().c_str());
                    updateStorage(upload_data, false);
                } else {
                    LOG_D("Clear %s FAIL", upload_data->getUploadPatch().c_str());
                }                 
            }
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG120:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG120");
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U)) {
                messRequest = DataModel<UploadLastDataRequest>::get(upload_data->getUploadPatch());
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG120");
                const error_t err{DataModel<UploadLastDataRequest>::clear(upload_data->getUploadPatch())};
                if(err == E_OK){
                    LOG_D("Clear %s successfull", upload_data->getUploadPatch().c_str());
                    updateStorage(upload_data, false);
                } else {
                    LOG_D("Clear %s FAIL", upload_data->getUploadPatch().c_str());
                }                  
            }
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG130:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG130");
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U)) {
                messRequest = DataModel<UploadRequestResponseNotificationRequest>::get(upload_data->getUploadPatch());
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG130");
                const error_t err{DataModel<UploadRequestResponseNotificationRequest>::clear(upload_data->getUploadPatch())};
                if(err == E_OK){
                    LOG_D("Clear %s successfull", upload_data->getUploadPatch().c_str());
                    updateStorage(upload_data, false);
                } else {
                    LOG_D("Clear %s FAIL", upload_data->getUploadPatch().c_str());
                }                 
            }
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG160:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG160");
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U)) {
                messRequest = DataModel<UploadErrorDataRequest>::get(upload_data->getUploadPatch());
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG160");
                const error_t err{DataModel<UploadErrorDataRequest>::clear(upload_data->getUploadPatch())};
                if(err == E_OK){
                    LOG_D("Clear %s successfull", upload_data->getUploadPatch().c_str());
                    updateStorage(upload_data, false);                    
                } else {
                    LOG_D("Clear %s FAIL", upload_data->getUploadPatch().c_str());
                }                   
            }
            break;
        }
        case GRPC_IF_TYPE::DCIF_RDG170:{
            LOG_I("GRPC_IF_TYPE::DCIF_RDG170");
            break;
        }
        default:
        {
            LOG_I("File type is undefined");
            messRequest = nullptr;
            break;
        }
    }
    if(messRequest != nullptr) {
        if((DiagManagerAdapter::getInstance()->getAllUploadConsent()) || (upload_data->getSRVC_Flag())) {
        std::string data_str{};
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        (void)google::protobuf::util::MessageToJsonString(*messRequest, &data_str, option);
        printData(data_str);
        /*Call httpmgr to send messRequest*/
        const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
        bool encrypt{false};
        if(region == 2U){
            encrypt = true;
        }
        std::string patch{"/data/rdg/"};
        (void)patch.append(upload_data->getUploadPatch());
        LOG_I("Check path: %s", patch.c_str());
        const android::sp<GrpcReqData> reqData{new GrpcReqData(GRPC_APP_TYPE::RMT_DIAG, uploadFileType, 60, encrypt, nullptr, patch)};
        // LOG_I("GRPC logPrint");
        // reqData->logPrint();
        mRetryCheck[static_cast<int64_t>(upload_data->getUploadId())] = false;
        const int32_t res {HttpManagerAdapter::getInstance()->sendGrpcMessage(reqData)};
        /*Setup timer 120 sec timeout for Call ID: res*/
        if((res >= 0) && (upload_data->getRetryCount() == 4U)) {
            mTickTimer_Uploading = android::sp<Timer>(new Timer(mTimerHandler_Uploading.get(), res));
            mTickTimer_Uploading->setDuration(TIMEOUT_UPLOADING_DURATION, 0U);
            mTimers_Uploading[res] = mTickTimer_Uploading;
            mTickTimer_Uploading->start();
            mTickTimer_Uploading = nullptr;
            LOG_I("Set up uploading timeout success");
        }
        LOG_I("Upload Id: %lld Time: %d CallID: %d", upload_data->getUploadId(), upload_data->getRetryCount(), res);
        /*Check retry count < 4*/
        if(upload_data->canRetry_2() == true) {
            const uint64_t retryInterval_tmp{upload_data->getRetryInterval()};
            // const android::sp<sl::Message> iMessage {mHandler->obtainMessage(MainHandler::MSG_ADD_DELAYED_UPLOAD_TASK_DELAY, upload_data)};
            // mHandler->sendMessageDelayed(iMessage, retryInterval_tmp * 1000U);
            setRetryTimer(upload_data->getUploadId(), retryInterval_tmp, upload_data);
            LOG_I("Setup Retry Upload Id: %lld after %lld sec", upload_data->getUploadId(), upload_data->getRetryInterval());
        } else {
            /*TBD*/
            LOG_D("Can not retry due to retry time exceed");
        }
        /*handle sendGrpcMessage return: if return >= 0 => save CALLID else retry follow rule*/
        if(res >= 0){
            LOG_I("sendGrpcMessage successfull. Call ID: %d", res);
            mSentTask[res] = upload_data;
        } else {
            LOG_I("sendGrpcMessage Fail. Res: %d", res);
            // /*Retry*/
            // if(upload_data->canRetry() == true) {
            //     LOG_I("Upload Id: %lld Retry: %d", upload_data->getUploadId() ,upload_data->getRetryCount());
            //     android::sp<sl::Message> iMessage = mHandler->obtainMessage(MainHandler::MSG_ADD_DELAYED_UPLOAD_TASK, upload_data);
            //     mHandler->sendMessageDelayed(iMessage, upload_data->getRetryInterval() * 1000U);
            // } else {
            //     LOG_I("Can not retry. Clear save upload data");
            //     if(DataModel<UploadDtcDataRequest>::clear(upload_data->getUploadPatch()) == E_OK) {
            //         LOG_I("Clear file: %s success", upload_data->getUploadPatch().c_str());
            //     } else {
            //         LOG_I("Clear file: %s Fail", upload_data->getUploadPatch().c_str());
            //     };
            // };
        }
        } else {
            LOG_I("Upload condition not match");
            LOG_D("Check AllUploadConsent: %s", DiagManagerAdapter::getInstance()->getAllUploadConsent()?"TRUE":"FALSE");
            LOG_D("Check SRVC_Flag: %s", upload_data->getSRVC_Flag()?"ON":"OFF");
            upload_data->deteleUploadFile();
        }
    } else {
        LOG_I("File is empty");
        LOG_I("Upload File is empty");
    }
}

uint32_t UploadManager::genRequestId() {
    LOG_I("Call genRequestId");
    const android::AutoMutex _l{mLock};
    if (currentRequestId > MAX_REQUEST_ID) {
        currentRequestId = 0U;
    }
    currentRequestId++;
    return currentRequestId;
}

void UploadManager::onReceivedGrpcRes(const android::sp<GrpcResData> pGrpcResData) {
    /* GRPC_APP_TYPE                                   mAppType;
       GRPC_IF_TYPE                                    mInterfaceType;
       int32_t                                         mCallID;
       GRPC_RESULT                                     mGRPCResult;
       grpc::StatusCode                                mGRPCStatusCode;
       std::string                                     mGRPCErrorMsg;
       std::shared_ptr<google::protobuf::Message>      mResponseProtoBufMsg;*/
    /*Check CallID is valid: check if CALLID is saved*/
    const int32_t callID {pGrpcResData->getCallID()};
    LOG_I("Received CALLID: %d", callID);
    const std::map<int32_t, android::sp<Timer>>::iterator it_timer {mTimers_Uploading.find(callID)};
    if(it_timer != mTimers_Uploading.end()) {
        LOG_I("Stop timer uploading timeout for call id: %d", callID);
        it_timer->second->stop();
        (void)mTimers_Uploading.erase(it_timer);
    }
    const std::map<int32_t, android::sp<UploadTask>>::iterator it {mSentTask.find(callID)};
    if(it != mSentTask.end()) {
        LOG_I("Received CALLID is valid");
        LOG_I("Check GRPC_RESULT: %d", pGrpcResData->getGrpcResult());
        if(pGrpcResData->getGrpcResult() == GRPC_RESULT::SEND_FAIL_TIMEOUT) {
            LOG_I("Save selfDiagNoCenterResponse");
            DiagManagerAdapter::getInstance()->selfDiagNoCenterResponse();
        }
        switch(pGrpcResData->getGrpcResult()){
            case GRPC_RESULT::SEND_FAIL_DATA_DISCONNECTED:
            {
                LOG_I("GRPC response SEND_FAIL_DATA_DISCONNECTED, retrying when the connection is restored");
                const android::AutoMutex _l{mLock_OpA};
                /*Dont retry -> stop retry timer for correct uploadtask*/
                mqOperationA.push_back(it->second);
                const uint64_t tmp_UploadID{it->second->getUploadId()};
                (void)mSentTask.erase(it);
                if(tmp_UploadID > static_cast<uint64_t>(INT64_MAX)){
                    LOG_E("tmp_UploadID = %llu is out of range of int64_t");
                }
                mRetryCheck[static_cast<int64_t>(tmp_UploadID)] = true;
                stopRetryTimer(tmp_UploadID);
                break;
            }
            case GRPC_RESULT::SEND_FAIL_NOT_FOUND_RECEIVER:
            {
                (void)HttpManagerAdapter::getInstance()->registerReceiver();
                break;
            }
            case GRPC_RESULT::SEND_SUCCESS:
            case GRPC_RESULT::SEND_FAIL:
            case GRPC_RESULT::SEND_FAIL_INVALID_PARAMETER:
            case GRPC_RESULT::SEND_FAIL_GRPC_SETTING:
            case GRPC_RESULT::SEND_FAIL_CONNECT_FAIL:
            case GRPC_RESULT::SEND_FAIL_TIMEOUT:
            {
                const grpc::StatusCode resGrpcStatusCode {pGrpcResData->getGrpcCode()};
                LOG_I("Check GRPC status code: %d", resGrpcStatusCode);
                /*24DCM_RDG_DIS-FR01_333: check GRPC status code*/
                
                if(resGrpcStatusCode == grpc::StatusCode::OK) {
                    LOG_I("Receive response with grpc::StatusCode::OK");
                    /*Send successfull -> delete upload file*/
                    // if(DataModel<UploadDtcDataRequest>::clear(it->second->getUploadPatch().c_str()) == E_OK) {
                    //     LOG_I("Clear file: %s success", it->second->getUploadPatch().c_str());
                    // } else {
                    //     LOG_I("Clear file: %s Fail", it->second->getUploadPatch().c_str());
                    // };
                    it->second->deteleUploadFile();
                    (void)mUploadingDB->deleteUploadDB(it->second->getUploadPatch());
                    /*Set retry fail to cancel retry*/
                    const uint64_t tmp_UploadID{it->second->getUploadId()};
                    if(tmp_UploadID > static_cast<uint64_t>(INT64_MAX)){
                        LOG_E("tmp_UploadID = %llu is out of range of int64_t");
                    }
                    mRetryCheck[static_cast<int64_t>(tmp_UploadID)] = true;
                    stopRetryTimer(tmp_UploadID);
                    updateStorage(it->second, false);
                    (void)mSentTask.erase(it);
                } else if((resGrpcStatusCode == grpc::StatusCode::CANCELLED) ||
                    (resGrpcStatusCode == grpc::StatusCode::INVALID_ARGUMENT) ||
                    (resGrpcStatusCode == grpc::StatusCode::NOT_FOUND) ||
                    (resGrpcStatusCode == grpc::StatusCode::ALREADY_EXISTS) ||
                    (resGrpcStatusCode == grpc::StatusCode::PERMISSION_DENIED) ||
                    (resGrpcStatusCode == grpc::StatusCode::RESOURCE_EXHAUSTED) ||
                    (resGrpcStatusCode == grpc::StatusCode::FAILED_PRECONDITION) ||
                    (resGrpcStatusCode == grpc::StatusCode::ABORTED) ||
                    (resGrpcStatusCode == grpc::StatusCode::OUT_OF_RANGE) ||
                    (resGrpcStatusCode == grpc::StatusCode::UNAUTHENTICATED)){
                    LOG_I("No retry follow grpc status code");
                    // mUploadingDB->saveUploadDB(it->second->getUploadPrio()
                    //     , static_cast<uint32_t>(it->second->getUploadFileType())
                    //     , static_cast<uint64_t>(it->second->getFileSize())
                    //     , it->second->getUploadPatch());
                    const uint64_t tmp_UploadID{it->second->getUploadId()};
                    (void)mSentTask.erase(it);
                    if(tmp_UploadID > static_cast<uint64_t>(INT64_MAX)){
                        LOG_E("tmp_UploadID = %llu is out of range of int64_t");
                    }
                    mRetryCheck[static_cast<int64_t>(tmp_UploadID)] = true;
                    stopRetryTimer(tmp_UploadID);
                } else if(
                    (resGrpcStatusCode == grpc::StatusCode::UNKNOWN) ||
                    (resGrpcStatusCode == grpc::StatusCode::DEADLINE_EXCEEDED) ||
                    (resGrpcStatusCode == grpc::StatusCode::UNIMPLEMENTED) ||
                    (resGrpcStatusCode == grpc::StatusCode::INTERNAL) ||
                    (resGrpcStatusCode == grpc::StatusCode::UNAVAILABLE) ||
                    (resGrpcStatusCode == grpc::StatusCode::DATA_LOSS)
                ) {
                    // /*Retry*/
                    // if(it->second->canRetry() == true) {
                    //     // LOG_I("Upload Id: %lld Retry: %d", it->second->getUploadId() ,it->second->getRetryCount());
                    //     android::sp<sl::Message> iMessage = mHandler->obtainMessage(MainHandler::MSG_ADD_DELAYED_UPLOAD_TASK, it->second);
                    //     mHandler->sendMessageDelayed(iMessage, it->second->getRetryInterval() * 1000U);
                    // } else {
                    //     LOG_I("Can not retry. Clear save upload data");
                    //     if(DataModel<UploadDtcDataRequest>::clear(it->second->getUploadPatch().c_str()) == E_OK) {
                    //         LOG_I("Clear file: %s success", it->second->getUploadPatch().c_str());
                    //     } else {
                    //         LOG_I("Clear file: %s Fail", it->second->getUploadPatch().c_str());
                    //     };
                    // };
                    if (it->second->getRetryCount() == 4U){
                        const uint64_t tmp_retryInterval{it->second->getRetryInterval()};
                        const uint64_t tmp_UploadID{it->second->getUploadId()};
                        LOG_I("Setup reUpload upload id: %lld after 30 mins", tmp_UploadID);
                        // const android::sp<sl::Message> iMessage {mHandler->obtainMessage(MainHandler::MSG_ADD_DELAYED_UPLOAD_TASK_DELAY, it->second)};
                        // mHandler->sendMessageDelayed(iMessage, tmp_retryInterval * 1000U);
                        setRetryTimer(tmp_UploadID, tmp_retryInterval, it->second);
                        LOG_I("Reupload Upload ID: %lld after %lld sec", tmp_UploadID, tmp_retryInterval);
                        (void)mSentTask.erase(it);
                    } else if(it->second->getRetryCount() > 3U){
                        const uint64_t tmp_UploadID{it->second->getUploadId()};
                        LOG_I("Stop uploading upload id: %llu", tmp_UploadID);
                        /*Do not retry any more*/
                        // it->second->deteleUploadFile();
                        // updateStorage(it->second, false);
                        // mUploadingDB->deleteUploadDB(it->second->getUploadPatch());
                    } else {
                        LOG_I("Retry in processing");
                        const int32_t retryTime_spec{pGrpcResData->getRetryTime()};
                        if((retryTime_spec >= 0) && (retryTime_spec <= 3600)) {
                            LOG_I("Center specify retry time: %d", retryTime_spec);
                            const uint64_t tmp_UploadID{it->second->getUploadId()};
                            LOG_I("Specify Retry uploadId: %llu after: %d", tmp_UploadID, retryTime_spec);
                            setRetryTimer(tmp_UploadID, static_cast<uint64_t>(retryTime_spec), it->second);
                        } else {
                            LOG_D("Retry time is not specified");
                        }
                    }
                } else {
                    LOG_I("Undefined case");
                }
                break;
            }
            default:
            {
                LOG_D("default");
                break;
            }
        }
    } else {
        LOG_I("Received CALLID is NOT valid");
    }
}

void UploadManager::onReceiveIG(const bool status) {
    if(status == false) {
        LOG_I("Stop uploading after 30s");
        mTickTimer->start();
    } else {
        LOG_I("Clear Stop uploading timer");
        mTickTimer->stop();
    }
}

void UploadManager::onServiceFlagChange() {
    LOG_I("RDGFlag off => Stop upload. Delete all upload pending");
    (void)mHandler->obtainMessage(MainHandler::MSG_ID_DETELE_PENDING_UPLOAD)->sendToTarget();
}

void UploadManager::onPPIChangedToFalse() {
    LOG_I("Consent status is not valid => discard all upload task");
    LOG_I("Consent status is not valid => delete un-upload data");
    /*delete un-uploaded data*/
    std::vector<CommonDefine::UploadFileAttribute> backupUpload{};

    (void)mUploadingDB->getUploadDB(backupUpload);

    for(std::vector<CommonDefine::UploadFileAttribute>::iterator ptr {backupUpload.begin()}; ptr != backupUpload.end(); ptr++) {
        /*Check if file is exist*/
        const std::string tmp_oldPath{std::string(DATA_PATH) + ptr->getFilePath()};
        if(FileUtil::isPathExist(tmp_oldPath.c_str()) == true) {
            LOG_I("Delete exist file: %s", tmp_oldPath.c_str());
            if (FileUtil::removeFile(tmp_oldPath.c_str()) != true) {
                LOG_E("delete fail !");
            }
            (void)mUploadingDB->deleteUploadDB(ptr->getFilePath());
        } else {
            LOG_I("Delete unexist file: %s", tmp_oldPath.c_str());
            (void)mUploadingDB->deleteUploadDB(ptr->getFilePath());
        }
    }
    mMspTaskQueue->clearQueue();
    mSentTask.clear();
}

// void UploadManager::onCommuRestore(){
//     /**/
// }

void UploadManager::updateStorage(const android::sp<UploadTask> task, const bool isTaskAdded) {
    GRPC_IF_TYPE uploadFileType {GRPC_IF_TYPE::DCIF_MAX};
    if(task != nullptr){
        uploadFileType = task->getUploadFileType();
    }
    switch(uploadFileType) {
    case GRPC_IF_TYPE::DCIF_RDG010:{
        break;
    }
    case GRPC_IF_TYPE::DCIF_RDG012:
    case GRPC_IF_TYPE::DCIF_RDG130:{
        if(isTaskAdded) {
            if(m_coll_update_result_noti_max_storage <= UINT64_MAX - task->getFileSize()){
                m_coll_update_result_noti_max_storage += task->getFileSize();
                mListNotificationFilePath.push_back(task);
            }
            else{
                LOG_E("[updateStorage] m_coll_update_result_noti_max_storage = %llu, mFileSize = %llu is out of range of uint64_t", m_coll_update_result_noti_max_storage, task->getFileSize());
            }
        } else {
            if(m_coll_update_result_noti_max_storage >= task->getFileSize()) {
                m_coll_update_result_noti_max_storage -= task->getFileSize();
            } else {
                m_coll_update_result_noti_max_storage = 0U;
            }
            const std::deque<android::sp<UploadTask>>::iterator it_deleteTask {std::find(mListNotificationFilePath.begin(), mListNotificationFilePath.end(), task)};
            if (it_deleteTask != mListNotificationFilePath.end())
            {
                (void)mListNotificationFilePath.erase(it_deleteTask);
            }
        }
        if(m_coll_update_result_noti_max_storage > NOTIFICATION_MAX_STORAGE) {
            LOG_I("[updateStorage] Excess NOTIFICATION_MAX_STORAGE");
            this->deleteOldestFile(mListNotificationFilePath);

        } else {
            LOG_I("m_coll_update_result_noti_max_storage: %llu", m_coll_update_result_noti_max_storage);
        }
        break;
    }
    case GRPC_IF_TYPE::DCIF_RDG030:
    case GRPC_IF_TYPE::DCIF_RDG050:{
        if(isTaskAdded) {
            if(m_dtc_rob_max_storage <= UINT64_MAX - task->getFileSize()){
                m_dtc_rob_max_storage += task->getFileSize();
                mListDtcRobFilePath.push_back(task);
            }
            else{
                LOG_E("m_dtc_rob_max_storage = %llu, mFileSize = %llu is out of range of uint64_t", m_dtc_rob_max_storage, task->getFileSize());
            }
        } else {
            if(m_dtc_rob_max_storage >= task->getFileSize()) {
                m_dtc_rob_max_storage -= task->getFileSize();
            } else {
                m_dtc_rob_max_storage = 0U;
            }
            const std::deque<android::sp<UploadTask>>::iterator it_deleteTask {std::find(mListDtcRobFilePath.begin(), mListDtcRobFilePath.end(), task)};
            if (it_deleteTask != mListDtcRobFilePath.end())
            {
                (void)mListDtcRobFilePath.erase(it_deleteTask);
            }
        }
        if(m_dtc_rob_max_storage > DTC_ROB_MAX_STORAGE) {
            LOG_I("Excess dtc_rob_max_storage");
            this->deleteOldestFile(mListDtcRobFilePath);
        } else {
            LOG_I("m_dtc_rob_max_storage: %llu", m_dtc_rob_max_storage);
        }
        break;
    }
    case GRPC_IF_TYPE::DCIF_RDG040:{
        if(isTaskAdded) {
            if(m_ssr_max_storage <= UINT64_MAX - task->getFileSize()){
                m_ssr_max_storage += task->getFileSize();
                mListSsrFilePath.push_back(task);
            }
            else{
                LOG_E("m_ssr_max_storage = %llu, mFileSize = %llu is out of range of uint64_t", m_ssr_max_storage, task->getFileSize());
            }
        } else {
            if(m_ssr_max_storage >= task->getFileSize()) {
                m_ssr_max_storage -= task->getFileSize();
            } else {
                m_ssr_max_storage = 0U;
            }
            const std::deque<android::sp<UploadTask>>::iterator it_deleteTask {std::find(mListSsrFilePath.begin(), mListSsrFilePath.end(), task)};
            if (it_deleteTask != mListSsrFilePath.end())
            {
                (void)mListSsrFilePath.erase(it_deleteTask);
            }
        }
        if(m_ssr_max_storage > SSR_MAX_STORAGE) {
            LOG_I("Excess ssr_max_storage");
            this->deleteOldestFile(mListSsrFilePath);
        } else {
            LOG_I("m_ssr_max_storage: %llu", m_ssr_max_storage);
        }
        break;
    }
    case GRPC_IF_TYPE::DCIF_RDG060:{
        if(isTaskAdded) {
            if(m_robssr_max_storage <= UINT64_MAX - task->getFileSize()){
                m_robssr_max_storage += task->getFileSize();
                mListRoBSSRFilePath.push_back(task);
            }
            else{
                LOG_E("m_robssr_max_storage = %llu, mFileSize = %llu is out of range of uint64_t", m_robssr_max_storage, task->getFileSize());
            }
        } else {
            if(m_robssr_max_storage >= task->getFileSize()) {
                m_robssr_max_storage -= task->getFileSize();
            } else {
                m_robssr_max_storage = 0U;
            }
            const std::deque<android::sp<UploadTask>>::iterator it_deleteTask {std::find(mListRoBSSRFilePath.begin(), mListRoBSSRFilePath.end(), task)};
            if (it_deleteTask != mListRoBSSRFilePath.end())
            {
                (void)mListRoBSSRFilePath.erase(it_deleteTask);
            }
        }
        if(m_robssr_max_storage > ROBSSR_MAX_STORAGE) {
            LOG_I("Excess robssr_max_storage");
            this->deleteOldestFile(mListRoBSSRFilePath);
        } else {
            LOG_I("m_robssr_max_storage: %llu", m_robssr_max_storage);
        }
        break;
    }
    case GRPC_IF_TYPE::DCIF_RDG070:{
        if(isTaskAdded) {
            if(m_directcommand_max_storage <= UINT64_MAX - task->getFileSize()){
                m_directcommand_max_storage += task->getFileSize();
                mListDirectCommandFilePath.push_back(task);
            }
            else{
                LOG_E("[updateStorage] m_directcommand_max_storage = %llu, mFileSize = %llu is out of range of uint64_t", m_directcommand_max_storage, task->getFileSize());
            }
        } else {
            if(m_directcommand_max_storage >= task->getFileSize()) {
                m_directcommand_max_storage -= task->getFileSize();
            } else {
                m_directcommand_max_storage = 0U;
            }
            const std::deque<android::sp<UploadTask>>::iterator it_deleteTask {std::find(mListDirectCommandFilePath.begin(), mListDirectCommandFilePath.end(), task)};
            if (it_deleteTask != mListDirectCommandFilePath.end())
            {
                (void)mListDirectCommandFilePath.erase(it_deleteTask);
            }
        }
        if(m_directcommand_max_storage > DIRECTCOMMAND_MAX_STORAGE) {
            LOG_I("[updateStorage] Excess directcommand_max_storage");
            this->deleteOldestFile(mListDirectCommandFilePath);
        } else {
            LOG_I("[updateStorage] m_directcommand_max_storage: %llu", m_directcommand_max_storage);
        }
        break;
    }
    case GRPC_IF_TYPE::DCIF_RDG080:{
        if(isTaskAdded) {
            if(m_warning_max_storage <= UINT64_MAX - task->getFileSize()){
                m_warning_max_storage += task->getFileSize();
                mListWarningFilePath.push_back(task);
            }
            else{
                LOG_E("m_warning_max_storage = %llu, mFileSize = %llu is out of range of uint64_t", m_warning_max_storage, task->getFileSize());
            }
        } else {
            if(m_warning_max_storage >= task->getFileSize()) {
                m_warning_max_storage -= task->getFileSize();
            } else {
                m_warning_max_storage = 0U;
            }
            const std::deque<android::sp<UploadTask>>::iterator it_deleteTask {std::find(mListWarningFilePath.begin(), mListWarningFilePath.end(), task)};
            if (it_deleteTask != mListWarningFilePath.end())
            {
                (void)mListWarningFilePath.erase(it_deleteTask);
            }
        }
        if(m_warning_max_storage > WARNING_MAX_STORAGE) {
            LOG_I("Excess warning_max_storage");
            this->deleteOldestFile(mListWarningFilePath);
        } else {
            LOG_I("m_warning_max_storage: %llu", m_warning_max_storage);
        }
        break;
    }
    case GRPC_IF_TYPE::DCIF_RDG100:{
        if(isTaskAdded) {
            if(m_ecu_infor_max_storage <= UINT64_MAX - task->getFileSize()){
                m_ecu_infor_max_storage += task->getFileSize();
                mListEcuInforFilePath.push_back(task);
            }
            else{
                LOG_E("m_ecu_infor_max_storage = %llu, mFileSize = %llu is out of range of uint64_t", m_ecu_infor_max_storage, task->getFileSize());
            }
        } else {
            if(m_ecu_infor_max_storage >= task->getFileSize()) {
                m_ecu_infor_max_storage -= task->getFileSize();
            } else {
                m_ecu_infor_max_storage = 0U;
            }
            const std::deque<android::sp<UploadTask>>::iterator it_deleteTask {std::find(mListEcuInforFilePath.begin(), mListEcuInforFilePath.end(), task)};
            if (it_deleteTask != mListEcuInforFilePath.end())
            {
                (void)mListEcuInforFilePath.erase(it_deleteTask);
            }
        }
        if(m_ecu_infor_max_storage > ECU_INFOR_MAX_STORAGE) {
            LOG_I("Excess ecu_infor_max_storage");
            this->deleteOldestFile(mListEcuInforFilePath);
        } else {
            LOG_I("m_ecu_infor_max_storage: %llu", m_ecu_infor_max_storage);
        }
        break;
    }
    case GRPC_IF_TYPE::DCIF_RDG120:{
        if(isTaskAdded) {
            if(m_last_max_storage <= UINT64_MAX - task->getFileSize()){
                m_last_max_storage += task->getFileSize();
                mListLastUploadFilePath.push_back(task);
            }
            else{
                LOG_E("m_last_max_storage = %llu, mFileSize = %llu is out of range of uint64_t", m_last_max_storage, task->getFileSize());
            }
        } else {
            if(m_last_max_storage >= task->getFileSize()) {
                m_last_max_storage -= task->getFileSize();
            } else {
                m_last_max_storage = 0U;
            }
            const std::deque<android::sp<UploadTask>>::iterator it_deleteTask {std::find(mListLastUploadFilePath.begin(), mListLastUploadFilePath.end(), task)};
            if (it_deleteTask != mListLastUploadFilePath.end())
            {
                (void)mListLastUploadFilePath.erase(it_deleteTask);
            }
        }
        if(m_last_max_storage > LAST_MAX_STORAGE) {
            LOG_I("Excess last_max_storage");
            this->deleteOldestFile(mListLastUploadFilePath);
        } else {
            LOG_I("m_last_max_storage: %llu", m_last_max_storage);
        }
        break;
    }
    case GRPC_IF_TYPE::DCIF_RDG160:{
        if(isTaskAdded) {
            if(m_error_max_storage <= UINT64_MAX - task->getFileSize()){
                m_error_max_storage += task->getFileSize();
                mListErrorFilePath.push_back(task);
            }
            else{
                LOG_E("m_error_max_storage = %llu, mFileSize = %llu is out of range of uint64_t", m_error_max_storage, task->getFileSize());
            }
        } else {
            if(m_error_max_storage >= task->getFileSize()) {
                m_error_max_storage -= task->getFileSize();
            } else {
                m_error_max_storage = 0U;
            }
            const std::deque<android::sp<UploadTask>>::iterator it_deleteTask {std::find(mListErrorFilePath.begin(), mListErrorFilePath.end(), task)};
            if (it_deleteTask != mListErrorFilePath.end())
            {
                (void)mListErrorFilePath.erase(it_deleteTask);
            }
        }
        if(m_error_max_storage > ERROR_MAX_STORAGE) {
            LOG_I("Excess error_max_storage");
            this->deleteOldestFile(mListErrorFilePath);
        } else {
            LOG_I("m_error_max_storage: %llu", m_error_max_storage);
        }
        break;
    }
    default: {
        LOG_D("Undefined GRPC_IF_TYPE");
        break;
    }
    }
}

void UploadManager::stopUploadingIG() {
    LOG_I("Stop uploading");
    /*Stop upload process*/
    /*Clear mMspTaskQueue*/
    mMspTaskQueue->clearQueue();
    /* Delete task is wait response */
    mSentTask.clear();
    /* Delete task is wait retry */
    for (std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>>::iterator it {mTimers_RetryMap.begin()}; it != mTimers_RetryMap.end(); ++it) {
        const android::sp<Timer> ptimer {it->second.first};
        ptimer->stop();
    }
    mTimers_RetryMap.clear();
    /*Clear pending saved file*/
    LOG_D("Delete pending task successful");
    /*Deactive app*/
    /*Set active app*/
    const FeatureStatus status{ApplicationManagerAdapter::getInstance()->getFeatureStatus("remotediag")};
    if (status == FeatureStatus::ON) {
        const int32_t res2 {ApplicationManagerAdapter::getInstance()->setFeatureStatus("remotediag", "remotediag", false)};
        if(res2 != E_OK) {
            LOG_I("Set deactive RometeDiag Fail");
        } else {
            LOG_I("Remotediag is deactive");
        }
    } else {
        LOG_D("Remotediag is not active");
    }
    /*Release power lock*/
    PowerManagerAdapter::getInstance()->releasePowerLock();
}

void UploadManager::onRestartUploading() {
    LOG_I("Start to restart uploading");
    std::vector<CommonDefine::UploadFileAttribute> backupUpload{};

    (void)mUploadingDB->getUploadDB(backupUpload);

    for(std::vector<CommonDefine::UploadFileAttribute>::iterator ptr {backupUpload.begin()}; ptr != backupUpload.end(); ptr++) {
        /*Check if file is exist*/
        const std::string tmp_oldPath{std::string(DATA_PATH) + ptr->getFilePath()};

        if(FileUtil::isPathExist(tmp_oldPath.c_str()) == true) {
            LOG_I("%s exist", tmp_oldPath.c_str());
            const uint32_t uploadId {genRequestId()};
            const android::sp<UploadTask> task {new UploadTask(uploadId)};
            task->setUploadFileType(convertIntToFileType(ptr->getFileType()));
            if(task->getUploadFileType() != GRPC_IF_TYPE::DCIF_RDG012) {
                const std::string file_dir {std::to_string(uploadId) + "_" + ptr->getFilePath()};
                const std::string new_tmp{std::string(DATA_PATH) + file_dir};
                if (std::rename(tmp_oldPath.c_str(), new_tmp.c_str()) == 0) {
                    LOG_I("File renamed successfully");
                    task->setUploadPatch(file_dir);
                } else {
                    LOG_I("Error renaming file");
                    task->setUploadPatch(ptr->getFilePath());
                }
                /*Set priority*/
                task->setUploadPrio(ptr->getPriority());
                task->setFileSize(ptr->getFileSize());
                requestUploadTask(task);
            } else {
                LOG_I("Do not restart to upload DCIF_RDG012");
                (void)mUploadingDB->deleteUploadDB(tmp_oldPath);
                LOG_I("File still exist. Delete file: %s", tmp_oldPath.c_str());
                if(FileUtil::removeFile(tmp_oldPath.c_str())) {
                    LOG_I("remove success");
                } else {
                    LOG_I("remove fail");
                }
            }
        } else {
            LOG_I("Delete unexist file");
            (void)mUploadingDB->deleteUploadDB(ptr->getFilePath());
        }
    }
}

void UploadManager::onDoOperationA() {
    LOG_D("Do operation A");
    /*ReUpload task saved in operationAQueue*/
    const android::AutoMutex _l{mLock_OpA};
    while(mqOperationA.empty() != true) {
        const android::sp<UploadTask> tmp_task{mqOperationA.front()};
        tmp_task->resetRetryCount();
        requestUploadTask(tmp_task);
        mqOperationA.pop_front();
    }
    LOG_D("Done operation A");
}

void UploadManager::restartUploading() {
    (void)mHandler->obtainMessage(MainHandler::MSG_RESTART_UPLOADING)->sendToTarget();
}

void UploadManager::operationA() {
    (void)mHandler->obtainMessage(MainHandler::MSG_DO_OPERATION_A)->sendToTarget();
}

void UploadManager::onCallIdTimeout(const int32_t callid) {
    LOG_I("Check call id: %d", callid);
    /*1. Trigger no response from center => retry*/
    const std::map<int32_t, android::sp<UploadTask>>::iterator it {mSentTask.find(callid)};
    if(it != mSentTask.end()) {
        const uint64_t tmp_UploadID{it->second->getUploadId()};
        // LOG_I("Setup reUpload upload id: %lld after 30 mins", tmp_UploadID);
        // const android::sp<sl::Message> iMessage {mHandler->obtainMessage(MainHandler::MSG_ADD_DELAYED_UPLOAD_TASK_DELAY, it->second)};
        // mHandler->sendMessageDelayed(iMessage, it->second->getRetryInterval() * 1000U);
        setRetryTimer(tmp_UploadID, it->second->getRetryInterval(), it->second);
        LOG_I("Reupload Upload ID: %lld after %lld sec", tmp_UploadID, it->second->getRetryInterval());
        (void)mSentTask.erase(it);
    } else {
        LOG_D("Can not find call id in mSentTask: %d", callid);
    }

    const std::map<int32_t, android::sp<Timer>>::iterator it_timer {mTimers_Uploading.find(callid)};
    if(it_timer != mTimers_Uploading.end()) {
        it_timer->second->stop();
        (void)mTimers_Uploading.erase(it_timer);
    } else {
        LOG_D("Can not find call id in mTimers_Uploading: %d", callid);
    }
}

void UploadManager::setRetryTimer(const uint64_t uploadId, const uint64_t duration, const android::sp<UploadTask> task){
    LOG_D("setRetryTimer uploadId: %llu duration: %u", uploadId, duration);
    if(uploadId > static_cast<uint64_t>(INT32_MAX)) {
        LOG_E("uploadId is greater than INT32_MAX");
    }
    /*Find timer id: uploadId and stop if it is in progress*/
    const std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>>::iterator it{mTimers_RetryMap.find(static_cast<int32_t>(uploadId))};

    if (it != mTimers_RetryMap.end()) {
        const android::sp<Timer> ptimer {it->second.first};
        // const android::sp<UploadTask> uploadTask = it->second.second;
        ptimer->stop();
    } else {
        /*TBD*/
    }
    mTickTimer_Retry = android::sp<Timer>(new Timer(mpTimerHandler_Retry.get(), static_cast<int32_t>(uploadId)));
    if(duration > static_cast<uint64_t>(UINT32_MAX)) {
        LOG_E("out of range");
    }
    mTickTimer_Retry->setDuration(static_cast<uint32_t>(duration), 0U);
    const std::pair<android::sp<Timer>, android::sp<UploadTask>> tmp_pair{std::make_pair(mTickTimer_Retry, task)};
    mTimers_RetryMap[static_cast<int32_t>(uploadId)] = tmp_pair;
    mTickTimer_Retry->start();
    mTickTimer_Retry = nullptr;
}

void UploadManager::stopRetryTimer(const uint64_t uploadId) {
    LOG_D("stopRetryTimer uploadId: %llu", uploadId);
    if(uploadId > static_cast<uint64_t>(INT32_MAX)) {
        LOG_E("uploadId is greater than INT32_MAX");
    }
    /*Find timer id: uploadId and stop if it is in progress*/
    const std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>>::iterator it{mTimers_RetryMap.find(static_cast<int32_t>(uploadId))};

    if (it != mTimers_RetryMap.end()) {
        const android::sp<Timer> ptimer {it->second.first};
        // const android::sp<UploadTask> uploadTask = it->second.second;
        ptimer->stop();
        (void)mTimers_RetryMap.erase(it);
    } else {
        /*TBD*/
    }
}

void UploadManager::onRetryTimeOut(const int32_t timerId) {
    LOG_I("onRetryTimeOut: %d", timerId);
    /*Do upload have id is timerId*/
    const android::sp<UploadTask> task_tmp{mTimers_RetryMap[timerId].second};
    if(task_tmp != nullptr) {
        (void)mHandler->obtainMessage(MainHandler::MSG_ADD_DELAYED_UPLOAD_TASK_DELAY, task_tmp)->sendToTarget();
    } else {
        LOG_D("Upload task is nullptr");
    }
    (void)mTimers_RetryMap.erase(timerId);
}

void UploadManager::MainHandler::handleMessage (const android::sp<sl::Message>& handlemsg)
{
    const int32_t what {handlemsg->what};
    switch (what) {
        case CMD_INIT_UPLOADMANAGER:
        {
            LOG_I("CMD_INIT_UPLOADMANAGER");
            mParent.init();
            break;
        }
        case MSG_UPLOAD_FILE:
        {
            LOG_I("MSG_UPLOAD_FILE");
            /*Send over gRPC*/
            android::sp<UploadTask> upTask{nullptr};
            handlemsg->getObject(upTask);
            if(upTask != nullptr) {
                LOG_I("send message over Grpc");
            } else {
                LOG_I("centerCommand is nullptr");
            }
            break;
            /*Handle return response*/
        }
        case MSG_ADD_UPLOAD_TASK:
        {
            LOG_I("MSG_ADD_UPLOAD_TASK");
            android::sp<UploadTask> upTask{nullptr};
            handlemsg->getObject(upTask);
            if(upTask != nullptr) {
                LOG_I("Check received upload task");
                LOG_D("Check SRVC_Flag: %s", upTask->getSRVC_Flag()?"ON":"OFF");
                LOG_I("File dir: %s FileType: %d Prio: %d FileSize: %lld",
                    upTask->getUploadPatch().c_str(), upTask->getUploadFileType(), upTask->getUploadPrio(), upTask->getFileSize());
                /*Add task to queue*/
                /*If upload consent is not valid => dont add uploadTask*/
                if((DiagManagerAdapter::getInstance()->getAllUploadConsent() == true) ||
                    (upTask->getSRVC_Flag()) ||
                    (upTask->getUploadFileType() == GRPC_IF_TYPE::DCIF_RDG012))
                {
                    mParent.addTask(upTask);
                    mParent.updateStorage(upTask, true);
                } else {
                    LOG_I("Upload consent is not valid => dont add uploadTask");
                    LOG_I("Do not upload due to permission. Delete upload file");
                    LOG_D("Check AllUploadConsent: %s", DiagManagerAdapter::getInstance()->getAllUploadConsent()?"TRUE":"FALSE");
                    LOG_D("Check SRVC_Flag: %s", upTask->getSRVC_Flag()?"ON":"OFF");
                    /*Delete file*/
                    upTask->deteleUploadFile();
                }
            } else {
                LOG_I("upTask is nullptr");
            }
            break;
        }
        case MSG_ADD_DELAYED_UPLOAD_TASK_DELAY:
        {
            LOG_I("MSG_ADD_DELAYED_UPLOAD_TASK_DELAY");
            /*Check upload status and check retry*/
            /*Use uploadId to find status of uploading*/
            android::sp<UploadTask> upTask{nullptr};
            handlemsg->getObject(upTask);
            if(upTask != nullptr) {
                LOG_I("Check received retry upload task");
                /*Check file upload existed*/
                const std::string mPath {std::string(DATA_PATH) + upTask->getUploadPatch()};

                if(FileUtil::isPathExist(mPath.c_str()) == true) {
                    LOG_I("AddTask_tmp Id: %lld Retry: %d", upTask->getUploadId() ,upTask->getRetryCount());
                    LOG_I("File dir: %s FileType: %d Prio: %d ",
                        upTask->getUploadPatch().c_str(), upTask->getUploadFileType(), upTask->getUploadPrio());
                    /*Add task to queue*/
                    if((DiagManagerAdapter::getInstance()->getAllUploadConsent() == true) || (upTask->getSRVC_Flag())) {
                        mParent.addTask_delayed(upTask);
                    } else {
                        LOG_I("Upload consent is not valid => dont add uploadTask");
                        LOG_I("Do not upload due to permission. Delete upload file");
                        LOG_D("Check AllUploadConsent: %s", DiagManagerAdapter::getInstance()->getAllUploadConsent()?"TRUE":"FALSE");
                        LOG_D("Check SRVC_Flag: %d", upTask->getSRVC_Flag()?"ON":"OFF");
                        /*Delete file*/
                        upTask->deteleUploadFile();
                    }
                } else {
                    LOG_I("File do not exist: %s", mPath.c_str());
                }
            } else {
                LOG_I("centerCommand is nullptr");
            }
            break;
        }
        case MSG_ID_DO_UPLOAD:
        {
            LOG_I("MSG_ID_DO_UPLOAD");
            mParent.doUpload();
            break;
        }
        case MSG_ID_DELAYED_INSERT_MSG:
        {
            LOG_I("MSG_ID_DELAYED_INSERT_MSG");
            break;
        }
        case MSG_RESTART_UPLOADING:
        {
            LOG_I("MSG_RESTART_UPLOADING");
            mParent.onRestartUploading();
            break;
        }
        case MSG_DO_OPERATION_A:
        {
            LOG_I("MSG_DO_OPERATION_A");
            mParent.onDoOperationA();
            break;
        } case MSG_ID_TRIGGER_UPLOAD:
        {
            LOG_I("MSG_ID_TRIGGER_UPLOAD");
            mParent.triggerUpload();
            break;
        }
        case MSG_ID_DETELE_PENDING_UPLOAD:
        {
            LOG_I("MSG_ID_DETELE_PENDING_UPLOAD");
            mParent.deletePendingTask();
            break;
        }
        default:
        {
            LOG_I("default");
            break;
        }
    }
}

GRPC_IF_TYPE UploadManager::convertIntToFileType(const uint32_t fileType)
{
    GRPC_IF_TYPE res{GRPC_IF_TYPE::DCIF_MAX};
    const std::map<uint32_t, GRPC_IF_TYPE>::iterator found {grpcValues.find(fileType)};
    if(found != grpcValues.end()){
        res = found->second;
    }
    else{
        LOG_E("Invalid file type");
    }
    return res;
}

void UploadManager::TimerHandler::handlerFunction (const int32_t timerId)
{
    switch (timerId) {
        case ID_IG_OFF_STOP_UPLOADING:
        {
            LOG_I("ID_IG_OFF_STOP_UPLOADING");
            mUploadManager.stopUploadingIG();
            break;
        }
        default:
            break;
    }
}

void UploadManager::deleteOldestFile(std::deque<android::sp<UploadTask>> &oldTask)
{
    const android::sp<UploadTask> taskDeleted{oldTask.front()};
    oldTask.pop_front();
    (void)mUploadingDB->deleteUploadDB(taskDeleted->getUploadPatch());
    const std::string deletePath{std::string(DATA_PATH) + taskDeleted->getUploadPatch()};
    (void)taskDeleted;
    if(FileUtil::isPathExist(deletePath.c_str()) == true)
    {
        LOG_I("[deleteOldestFile] Delete exist file: %s", deletePath.c_str());
        if (FileUtil::removeFile(deletePath.c_str()) != true)
        {
            LOG_E("[deleteOldestFile] delete fail !");
        }
    }
    else {
        LOG_I("[deleteOldestFile] Delete unexist file: %s", deletePath.c_str());
    }
}

void UploadManager::TimerHandler_Uploading::handlerFunction(const int32_t timerId)
{
    LOG_I("call id upload timeout: %d", timerId);
    mUploadManager.onCallIdTimeout(timerId);
}

void UploadManager::TimerHandler_Retry::handlerFunction(const int32_t timerId)
{
    LOG_I("call id upload timeout: %d", timerId);
    mUploadManager.onRetryTimeOut(timerId);
}

uint32_t UploadManager::getCounterValue() noexcept
{
    if (mCounterValue >= NUMBER_UPLOAD_DATA_IS_MADE_AFTER_IG_ON_MAX)
    {
        mCounterValue = 0U;
    }
    else
    {
        mCounterValue++;
    }
    return mCounterValue;
}

} //end namespace
