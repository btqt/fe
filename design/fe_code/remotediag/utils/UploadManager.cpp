#include <google/protobuf/util/json_util.h>
#include "UploadManager.h"
#ifdef ENABLE_LGE_LXC
#include "RemoteFileStore.h"
#endif /* ENABLE_LGE_LXC */
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
        , mListDtcRobFilePath{}
        , mListSsrFilePath{}   
        , mListDirectCommandFilePath{} 
        , mListRoBSSRFilePath{}
        , mListWarningFilePath{}   
        , mListEcuInforFilePath{}  
        , mListLastUploadFilePath{}
        , mListNotificationFilePath{}  
        , mListErrorFilePath{} 
        , mqOperationA{}   
        , mqRestartUploadIG{}
        , mIgStoppedUpload{false}
        , mRdgStop{false}
{
    currentRequestId = 0U;
    mTickTimer_Uploading = nullptr;
    mTickTimer_Retry = nullptr;
    mUploadManager = this;
    mTickTimer = new Timer(mTimerHandler.get(), TimerHandler::ID_IG_OFF_STOP_UPLOADING);
    mTickTimer->setDuration(IG_OFF_STOP_UPLOADING_DURATION, 0U);
        static constexpr char_t UPLOADMGR_DB_STRUCT[]{"(id INTEGER PRIMARY KEY\
                                                , prio INTEGER\
                                                , uploadfiletype INTEGER\
                                                , filesize INTEGER\
                                                , uploadpatch TEXT\
                                                )"};
    static constexpr char_t UPLOADMGR_DB_NAME[]{"upload_manager"};
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
        LOG_E("ret = %d is out of range of uint32_t", ret);
    }
    if(ret != SQLITE_OK)
    {
        LOG_E("UploadMgr DB init failed, reason = %s", Database::errorToString(ret).c_str());
    } else {
        LOG_I("UploadMgr DB init success");
    }
    resetStorageData();

    (void)mHandler->obtainMessage(MainHandler::CMD_INIT_UPLOADMANAGER)->sendToTarget();
}

UploadManager::~UploadManager() = default;

android::sp<UploadManager> UploadManager::getInstance()
{
    if (mUploadManager == nullptr)
    {
        LOG_E("mUploadManager is null");
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
    LOG_I("Init UploadManager");
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

    // /*Load upload data information from database*/
    // mBackupUpload.clear();
    // (void)mUploadingDB->getUploadDB(mBackupUpload);
    // /*Load upload failed data from data base*/
    // loadBackupData();
}

void UploadManager::sendFile(const android::sp<UploadTask> task) {
    LOG_I("send file");
    (void)mHandler->obtainMessage(MainHandler::MSG_UPLOAD_FILE, task)->sendToTarget();
}

void UploadManager::requestUploadTask(const android::sp<UploadTask> task) {
    (void)mHandler->obtainMessage(MainHandler::MSG_ADD_UPLOAD_TASK, task)->sendToTarget();
}

void UploadManager::restartUploadTask(const android::sp<UploadTask> task) {
    (void)mHandler->obtainMessage(MainHandler::MSG_RESTART_UPLOAD_TASK, task)->sendToTarget();
}

void UploadManager::addTask(const android::sp<UploadTask> &pUploadTask, const bool needUpdateDB) {
    LOG_I("Upload manager add upload task");
    // android::Mutex::Autolock autolock(gMutexTaskQueue);
    if (pUploadTask != nullptr) {
        if(needUpdateDB) {
            LOG_D("Update DB: %s", pUploadTask->getUploadPatch().c_str());
            (void)mUploadingDB->saveUploadDB(pUploadTask->getUploadPrio()
                , static_cast<uint32_t>(pUploadTask->getUploadFileType())
                , static_cast<uint64_t>(pUploadTask->getFileSize())
                , pUploadTask->getUploadPatch());
        } else {
            LOG_D("NO Update DB: %s", pUploadTask->getUploadPatch().c_str());
        }

        if ((mMspTaskQueue != nullptr) && mMspTaskQueue->insertQueue(pUploadTask)) {
            triggerUpload();
            LOG_I("Add task successfully");
        }
        LOG_I("Add task end");
    } else {
        LOG_E("pUploadTask is null");
    }
}

void UploadManager::addTask_delayed(const android::sp<UploadTask> &pUploadTask) {
    if (pUploadTask != nullptr) {
        const uint64_t uploadId {pUploadTask->getUploadId()};
        if(uploadId <= static_cast<uint64_t>(INT64_MAX)){
            if(mRetryCheck[static_cast<int64_t>(uploadId)] == false){
                //cppcheck-suppress invalidPrintfArgType_uint
                LOG_I("Do retry UploadId: %llu", uploadId);
                if ((mMspTaskQueue != nullptr) && mMspTaskQueue->insertQueue(pUploadTask)) {
                    triggerUpload();
                    LOG_I("Add task successfully");
                } else {
                    LOG_E("Add upload task fail");
                }
            } else {
                //cppcheck-suppress invalidPrintfArgType_uint
                LOG_I("Dont retry: %llu", uploadId);
                LOG_I("Check Retry map size before: %zu", mRetryCheck.size());
                (void)mRetryCheck.erase(static_cast<int64_t>(uploadId));
                LOG_I("Check Retry map size after: %zu", mRetryCheck.size());
            }
        }
        else{
            LOG_E("uploadId = %llu is out of range of int64_t", uploadId);
        }
    }
    else{
        LOG_E("pUploadTask is nullptr");
    }
}

void UploadManager::discardAllTask() {
    LOG_I("Start");
    mMspTaskQueue->clearQueue();
    LOG_I("End");
}

void UploadManager::deletePendingTask() {
    LOG_W("Start delete pending task");
    /*Clear mMspTaskQueue*/
    while((mMspTaskQueue != nullptr) && (mMspTaskQueue->isQueueEmpty() != true)) {
        const android::sp<UploadTask> upload_data {mMspTaskQueue->getTop()};
        if(upload_data != nullptr){
            (void)mMspTaskQueue->pop();
            if (!upload_data->deteleUploadFile()) {
                LOG_E("Failed to delete upload file: %s", upload_data->getUploadPatch().c_str());
            }
            (void)mUploadingDB->deleteUploadDB(upload_data->getUploadPatch());
        } else {
            LOG_E("upload_data is nullptr");
        }
    }
    /* Delete task is wait response */
    {
        const android::AutoMutex _l{mLock_SentTask};
        for (std::multimap<int32_t, android::sp<UploadTask>>::iterator it {mSentTask.begin()}; it != mSentTask.end(); ++it) {
            android::sp<UploadTask> tmp_UploadTask{nullptr};
            tmp_UploadTask = it->second;
            if(tmp_UploadTask != nullptr) {
                if (!tmp_UploadTask->deteleUploadFile()) {
                    LOG_E("Failed to delete upload file: %s", tmp_UploadTask->getUploadPatch().c_str());
                }
            } else {
                LOG_E("tmp_UploadTask is nullptr");
            }
        }
        mSentTask.clear();
    }
    /*TBD: Stop All Uploading timer*/
    /* Delete task is wait retry */
    // mLock_Retry
    {
        const android::AutoMutex _l{mLock_Retry};
        /*LOG for mTimers_RetryMap size*/
        LOG_D("Check mTimers_RetryMap size: %zu", mTimers_RetryMap.size());
        for (std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>>::iterator it {mTimers_RetryMap.begin()}; it != mTimers_RetryMap.end(); ++it) {
            const android::sp<Timer> ptimer {it->second.first};
            const android::sp<UploadTask> task {it->second.second};
            if(ptimer != nullptr) {
                ptimer->stop();
            } else {
                LOG_D("ptimer is null");
            }
            if(task != nullptr) {
                if (!task->deteleUploadFile()) {
                    LOG_E("Failed to delete upload file: %s", task->getUploadPatch().c_str());
                }
            } else {
                LOG_E("task is null");
            }
        }
        mTimers_RetryMap.clear();
    }

    /*Clear pending saved file*/
    resetStorageData();
    LOG_D("Delete pending task successful");
}

void UploadManager::triggerUpload() {
    LOG_I("triggerUpload");
    if ((mMspTaskQueue != nullptr) &&  (mMspTaskQueue->isQueueEmpty() != true)) {
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
            Remotediag::getInstance()->setOperationA(true);
            /*Pop task from queue*/
            const android::sp<UploadTask> upload_data {mMspTaskQueue->getTop()};
            upload_data->setIsOperationA(true);
            mqOperationA.push_back(upload_data);
            LOG_D("Check mqOperationA size: %zu", mqOperationA.size());
            (void)mMspTaskQueue->pop();
            (void)mHandler->obtainMessage(MainHandler::MSG_ID_TRIGGER_UPLOAD)->sendToTarget();
        }
    } else {
        LOG_W("No task in queue");
    }
}

void UploadManager::doUpload() {
    LOG_I("Do upload");
    /*Get upload task from queue*/
    android::sp<UploadTask> upload_data{nullptr};
    if(mMspTaskQueue != nullptr) {
        upload_data = mMspTaskQueue->getTop();
        (void)mMspTaskQueue->pop();
    } else {
        LOG_E("mMspTaskQueue is nullptr");
    }
    

    if(upload_data != nullptr) {
        GRPC_IF_TYPE uploadFileType {GRPC_IF_TYPE::DCIF_MAX};
        uploadFileType = upload_data->getUploadFileType();
    
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
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if((RDGFlag == 1U) && (DTCFlag == 1U)) {
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    messRequest = DataModel<vccomif::rdg::v1::interfaces::UploadDtcDataRequestEncryption>::getUpload(upload_data->getUploadPatch());
                }
                else
                {
                    messRequest = DataModel<UploadDtcDataRequest>::getUpload(upload_data->getUploadPatch());
                }
            } else {
                LOG_I("DTC flag off. Don't upload DCIF_RDG030");
                error_t err{E_ERROR};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    err = DataModel<vccomif::rdg::v1::interfaces::UploadDtcDataRequestEncryption>::clearUpload(upload_data->getUploadPatch());
                }
                else
                {
                    err = DataModel<UploadDtcDataRequest>::clearUpload(upload_data->getUploadPatch());
                }
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
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if((RDGFlag == 1U) && (SSRFlag == 1U))
            {
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    messRequest = DataModel<vccomif::rdg::v1::interfaces::UploadSsrDataRequestEncryption>::getUpload(upload_data->getUploadPatch());
                }
                else
                {
                    messRequest = DataModel<UploadSsrDataRequest>::getUpload(upload_data->getUploadPatch());
                }
            } else {
                LOG_I("SSR flag off. Don't upload DCIF_RDG040");
                error_t err{E_ERROR};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    err = DataModel<vccomif::rdg::v1::interfaces::UploadSsrDataRequestEncryption>::clearUpload(upload_data->getUploadPatch());
                }
                else
                {
                    err = DataModel<UploadSsrDataRequest>::clearUpload(upload_data->getUploadPatch());
                }
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
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if((RDGFlag == 1U) && (RoBflag == 1U))
            {
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    messRequest = DataModel<vccomif::rdg::v1::interfaces::UploadRobDataRequestEncryption>::getUpload(upload_data->getUploadPatch());
                }
                else
                {
                    messRequest = DataModel<UploadRobDataRequest>::getUpload(upload_data->getUploadPatch());
                }
            } else {
                LOG_I("ROB flag off. Don't upload DCIF_RDG050");
                error_t err{E_ERROR};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    err = DataModel<vccomif::rdg::v1::interfaces::UploadRobDataRequestEncryption>::clearUpload(upload_data->getUploadPatch());
                }
                else
                {
                    err = DataModel<UploadRobDataRequest>::clearUpload(upload_data->getUploadPatch());
                }
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
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U))
            {
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    messRequest = DataModel<vccomif::rdg::v1::interfaces::UploadRobSsrDataRequestEncryption>::getUpload(upload_data->getUploadPatch());
                }
                else
                {
                    messRequest = DataModel<UploadRobSsrDataRequest>::getUpload(upload_data->getUploadPatch());
                }
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG060");
                error_t err{E_ERROR};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    err = DataModel<vccomif::rdg::v1::interfaces::UploadRobSsrDataRequestEncryption>::clearUpload(upload_data->getUploadPatch());
                }
                else
                {
                    err = DataModel<UploadRobSsrDataRequest>::clearUpload(upload_data->getUploadPatch());
                }
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
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U))
            {
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    messRequest = DataModel<vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequestEncryption>::getUpload(upload_data->getUploadPatch());
                }
                else
                {
                    messRequest = DataModel<UploadDirectCommandDataRequest>::getUpload(upload_data->getUploadPatch());
                }
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG070");
                error_t err{E_ERROR};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    err = DataModel<vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequestEncryption>::clearUpload(upload_data->getUploadPatch());
                }
                else
                {
                    err = DataModel<UploadDirectCommandDataRequest>::clearUpload(upload_data->getUploadPatch());
                }
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
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U))
            {
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    messRequest = DataModel<vccomif::rdg::v1::interfaces::UploadWarningInformationRequestEncryption>::getUpload(upload_data->getUploadPatch());
                }
                else
                {
                    messRequest = DataModel<UploadWarningInformationRequest>::getUpload(upload_data->getUploadPatch());
                }
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG080");
                error_t err{E_ERROR};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    err = DataModel<vccomif::rdg::v1::interfaces::UploadWarningInformationRequestEncryption>::clearUpload(upload_data->getUploadPatch());
                }
                else
                {
                    err = DataModel<UploadWarningInformationRequest>::clearUpload(upload_data->getUploadPatch());
                }
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
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U))
            {
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    messRequest = DataModel<vccomif::rdg::v1::interfaces::UploadEcuInformationRequestEncryption>::getUpload(upload_data->getUploadPatch());
                }
                else
                {
                    messRequest = DataModel<UploadEcuInformationRequest>::getUpload(upload_data->getUploadPatch());
                }
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG100");
                error_t err{E_ERROR};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    err = DataModel<vccomif::rdg::v1::interfaces::UploadEcuInformationRequestEncryption>::clearUpload(upload_data->getUploadPatch());
                }
                else
                {
                    err = DataModel<UploadEcuInformationRequest>::clearUpload(upload_data->getUploadPatch());
                }
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
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U))
            {
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    messRequest = DataModel<vccomif::rdg::v1::interfaces::UploadLastDataRequestEncryption>::getUpload(upload_data->getUploadPatch());
                }
                else
                {
                    messRequest = DataModel<UploadLastDataRequest>::getUpload(upload_data->getUploadPatch());
                }
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG120");
                error_t err{E_ERROR};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    err = DataModel<vccomif::rdg::v1::interfaces::UploadLastDataRequestEncryption>::clearUpload(upload_data->getUploadPatch());
                }
                else
                {
                    err = DataModel<UploadLastDataRequest>::clearUpload(upload_data->getUploadPatch());
                }
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
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U))
            {
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    messRequest = DataModel<vccomif::rdg::v1::interfaces::UploadRequestResponseNotificationRequestEncryption>::getUpload(upload_data->getUploadPatch());
                }
                else
                {
                    messRequest = DataModel<UploadRequestResponseNotificationRequest>::getUpload(upload_data->getUploadPatch());
                }
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG130");
                error_t err{E_ERROR};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    err = DataModel<vccomif::rdg::v1::interfaces::UploadRequestResponseNotificationRequestEncryption>::clearUpload(upload_data->getUploadPatch());
                }
                else
                {
                    err = DataModel<UploadRequestResponseNotificationRequest>::clearUpload(upload_data->getUploadPatch());
                }
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
            const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
            if((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U)) {
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    messRequest = DataModel<vccomif::rdg::v1::interfaces::UploadErrorDataRequestEncryption>::getUpload(upload_data->getUploadPatch());
                }
                else
                {
                    messRequest = DataModel<UploadErrorDataRequest>::getUpload(upload_data->getUploadPatch());
                }
            } else {
                LOG_I("RDG active flag off. Don't upload DCIF_RDG160");
                error_t err{E_ERROR};
                if (region == LGE_REGION::LGE_REGION_CN)
                {
                    err = DataModel<vccomif::rdg::v1::interfaces::UploadErrorDataRequestEncryption>::clearUpload(upload_data->getUploadPatch());
                }
                else
                {
                    err = DataModel<UploadErrorDataRequest>::clearUpload(upload_data->getUploadPatch());
                }
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
        bool isEncrypted{false};
        if(region == LGE_REGION::LGE_REGION_CN){
            encrypt = true;
            isEncrypted = true;
        }
        std::string patch{UPLOAD_PATH};
        (void)patch.append(upload_data->getUploadPatch());
        LOG_I("Check path: %s", patch.c_str());
        const uint64_t resTimeout{upload_data->getResTimeout()};
        LOG_D("Check response timeout: %llu", resTimeout);

        const android::sp<GrpcReqData> reqData{new GrpcReqData(GRPC_APP_TYPE::RMT_DIAG, uploadFileType, static_cast<int32_t>(resTimeout), encrypt, nullptr, patch, isEncrypted)};
        // LOG_I("GRPC logPrint");
        // reqData->logPrint();
        mRetryCheck[static_cast<int64_t>(upload_data->getUploadId())] = false;
        upload_data->setGRPCResCode(grpc::StatusCode::DEADLINE_EXCEEDED);
        const int32_t res {HttpManagerAdapter::getInstance()->sendGrpcMessage(reqData)};
        /*Setup timer 120 sec timeout for Call ID: res*/
        if((res >= 0) && (upload_data->getRetryCount() == 3U)) {
            
            mTickTimer_Uploading = android::sp<Timer>(new Timer(mTimerHandler_Uploading.get(), res));
            mTickTimer_Uploading->setDuration(INTERNAL_TIMEOUT_UPLOADING_DURATION, 0U);
            mTickTimer_Uploading->start();
            const android::AutoMutex _lock{mLock_ReceivedRes};
            (void)mTimers_Uploading.insert({res, mTickTimer_Uploading});
            mTickTimer_Uploading = nullptr;
            LOG_I("Set up uploading timeout success");
        }
        LOG_I("UploadId: %llu Time: %u CallID: %d File: %s", 
            upload_data->getUploadId(), upload_data->getRetryCount(), res, upload_data->getUploadPatch().c_str());
        /*Check retry count < 4*/
        if(upload_data->canRetry_2() == true) {
            const uint64_t retryInterval_tmp{upload_data->getRetryInterval()};
            // const android::sp<sl::Message> iMessage {mHandler->obtainMessage(MainHandler::MSG_ADD_DELAYED_UPLOAD_TASK_DELAY, upload_data)};
            // mHandler->sendMessageDelayed(iMessage, retryInterval_tmp * 1000U);
            setRetryTimer(upload_data->getUploadId(), retryInterval_tmp, upload_data);
            LOG_I("Setup Retry UploadId: %llu after %llu sec", upload_data->getUploadId(), upload_data->getRetryInterval());
        } else {
            /*TBD*/
            LOG_W("Can not retry due to retry time exceed");
        }
        /*handle sendGrpcMessage return: if return >= 0 => save CALLID else retry follow rule*/
        if(res >= 0){
            LOG_I("sendGrpcMessage successfull. Call ID: %d", res);
            const android::AutoMutex _l{mLock_SentTask};
            (void)mSentTask.insert({res, upload_data});
        } else {
            LOG_I("sendGrpcMessage Fail. Res: %d", res);
            /*Add data to restart upload next IG ON*/
            // mqRestartUploadIG.push_back(upload_data);
        }
        } else {
            LOG_W("Upload condition not match");
            LOG_D("Check AllUploadConsent: %s", DiagManagerAdapter::getInstance()->getAllUploadConsent()?"TRUE":"FALSE");
            LOG_D("Check SRVC_Flag: %s", upload_data->getSRVC_Flag()?"ON":"OFF");
            if (!upload_data->deteleUploadFile()) {
                LOG_E("Failed to delete upload file: %s", upload_data->getUploadPatch().c_str());
            }
        }
    } else {
        LOG_W("File is empty");
        const IG_STATUS igStatus{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
        if((upload_data->getIsRestartUpload() == false) && (upload_data->getIsOperationA() == false)) {
            LOG_I("Upload File is empty. Trigger next upload");
            (void)mHandler->obtainMessage(MainHandler::MSG_ID_TRIGGER_UPLOAD)->sendToTarget();
        }
        
        if (igStatus == IG_STATUS::IG_STATUS_ON)
        {
            if (upload_data->getIsRestartUpload() == true)
            {
                LOG_I("Upload File is empty. Trigger restart upload");
                (void)mHandler->obtainMessage(MainHandler::MSG_RESTART_UPLOADING_IG)->sendToTarget();
            }
            if (upload_data->getIsOperationA() == true)
            {
                LOG_I("Upload File is empty. Trigger operation A");
                (void)mHandler->obtainMessage(MainHandler::MSG_DO_OPERATION_A)->sendToTarget();
            }
        }
    }
    (void)uploadFileType;
    } else {
        LOG_E("Upload data is nullptr");
    }
}

uint32_t UploadManager::genRequestId() {
    LOG_I("Call genRequestId");
    const android::AutoMutex _l{mLock};
    if (currentRequestId >= static_cast<uint32_t>(INT32_MAX)) {
        currentRequestId = 0U;
    }
    currentRequestId++;
    return currentRequestId;
}

uint64_t UploadManager::genCountUpload() {
    LOG_I("genCountUpload");
    const android::AutoMutex _l{mLock};
    const std::string RDG_UPLOAD_COUNT_PROP{"remotediag.prop.uploadcount"};
    const std::string propUploadCount{RDG_UPLOAD_COUNT_PROP};
    const char_t *const currUploadCount{Remotediag::getInstance()->getProperty(propUploadCount.c_str())};
    uint64_t count{0LLU};
    if (currUploadCount != nullptr)
    {
        count = std::stoul(currUploadCount, nullptr, 0);
        LOG_D("Current upload count: %llu", count);
        if(count >= static_cast<uint64_t>(INT32_MAX)){/*need changed to UINT64_MAX*/
            count = 0U;
            LOG_W("Reset count to 0");
        }
        count = count + 1U;
    }
    else
    {
        LOG_D("Current upload count: %llu", count);
        count = count + 1U;
    }
    /*Save to prop*/
    LOG_D("New upload count: %llu", count);
    Remotediag::getInstance()->setProperty(propUploadCount.c_str(), static_cast<int32_t>(count), true);
    return count;
}
void UploadManager::receivedGrpcRes(const android::sp<GrpcResData> pGrpcResData) {
    (void)mHandler->obtainMessage(MainHandler::MSG_RECEIVE_GRPC_RES, pGrpcResData)->sendToTarget();
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

    {
        const android::AutoMutex _lock{mLock_ReceivedRes};
        android::sp<Timer> tmp_Timer{nullptr};
        const std::multimap<int32_t, android::sp<Timer>>::iterator it_timer {mTimers_Uploading.find(callID)};
        if(it_timer != mTimers_Uploading.end()) {
            tmp_Timer = it_timer->second;
            (void)mTimers_Uploading.erase(it_timer);
        } else {
            LOG_D("Error: Timer with callID %d not found in mTimers_Uploading", callID);
        }
        if(tmp_Timer != nullptr) {
            LOG_I("Stop timer uploading timeout for call id: %d", callID);
            tmp_Timer->stop();
        } else {
            LOG_D("Error: tmp_Timer is nullptr for callID %d", callID);
        }
    }

    android::sp<UploadTask> tmp_UploadTask{nullptr};
    {
        const android::AutoMutex _l1{mLock_SentTask};
        const std::multimap<int32_t, android::sp<UploadTask>>::iterator it {mSentTask.find(callID)};
        if(it != mSentTask.end()) {
            tmp_UploadTask = it->second;
            (void)mSentTask.erase(it);
        } else {
            LOG_E("Error: UploadTask with callID %d not found in mSentTask", callID);
        }
    }

    if(tmp_UploadTask != nullptr) {
        const IG_STATUS igStatus{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
        LOG_I("Received CALLID is valid");
        LOG_I("Received CALLID: %d, Check UploadID: %llu, Check GRPC_RESULT: %d"
            , callID, tmp_UploadTask->getUploadId(), pGrpcResData->getGrpcResult());
        if(pGrpcResData->getGrpcResult() == GRPC_RESULT::SEND_FAIL_TIMEOUT) {
            LOG_I("Save selfDiagNoCenterResponse");
            DiagManagerAdapter::getInstance()->selfDiagNoCenterResponse();
        }
        switch(pGrpcResData->getGrpcResult()){
            case GRPC_RESULT::SEND_FAIL_DATA_DISCONNECTED:
            {
                LOG_I("GRPC response SEND_FAIL_DATA_DISCONNECTED, retrying when the connection is restored");
                {
                    const android::AutoMutex _l{mLock_OpA};
                    /*Dont retry -> stop retry timer for correct uploadtask*/
                    tmp_UploadTask->setIsOperationA(true);
                    mqOperationA.push_back(tmp_UploadTask);
                }
                const uint64_t tmp_UploadID{tmp_UploadTask->getUploadId()};
                if(tmp_UploadID > static_cast<uint64_t>(INT64_MAX)){
                    //cppcheck-suppress invalidPrintfArgType_uint
                    LOG_E("tmp_UploadID = %llu is out of range of int64_t", tmp_UploadID);
                }
                if (tmp_UploadID <= static_cast<uint64_t>(INT64_MAX)) {
                    mRetryCheck[static_cast<int64_t>(tmp_UploadID)] = true;
                } else {
                    LOG_E("tmp_UploadID = %llu is out of range of int64_t", tmp_UploadID);
                }
                // stopRetryTimer(tmp_UploadID);
                break;
            }
            case GRPC_RESULT::SEND_FAIL_NOT_FOUND_RECEIVER:
            {
                /*Sent fail due to HTTP -> register -> retry when timer expired*/
#ifdef ENABLE_LGE_LXC
                (void)HttpManagerAdapter::getInstance()->registerService();
#else
                (void)HttpManagerAdapter::getInstance()->registerReceiver();
#endif /* ENABLE_LGE_LXC */
                break;
            }
            default:
            {
                const grpc::StatusCode resGrpcStatusCode {pGrpcResData->getGrpcCode()};
                LOG_I("Check GRPC status code: %d", resGrpcStatusCode);
                /*24DCM_RDG_DIS-FR01_333: check GRPC status code*/
                tmp_UploadTask->setGRPCResCode(resGrpcStatusCode);
                if(resGrpcStatusCode == grpc::StatusCode::OK) {
                    LOG_I("Receive response with grpc::StatusCode::OK");
                    /*Send successfull -> delete upload file*/
                    if (!tmp_UploadTask->deteleUploadFile())
                    {
                        LOG_E("Failed to delete upload file: %s", tmp_UploadTask->getUploadPatch().c_str());
                        /*Only update storage if deteleUploadFile (tmp_UploadTask is existed)*/
                    } else {
                        updateStorage(tmp_UploadTask, false);
                    }
                    (void)mUploadingDB->deleteUploadDB(tmp_UploadTask->getUploadPatch());
                    /*Set retry fail to cancel retry*/
                    const uint64_t tmp_UploadID{tmp_UploadTask->getUploadId()};
                    if(tmp_UploadID > static_cast<uint64_t>(INT64_MAX)){
                        LOG_E("tmp_UploadID = %llu is out of range of int64_t", tmp_UploadID);
                    }
                    mRetryCheck[static_cast<int64_t>(tmp_UploadID)] = true;
                    // stopRetryTimer(tmp_UploadID);
                    /*Trigger restart upload next failed data*/
                    /*Check if getIsRestartUpload is true*/
                    if (igStatus == IG_STATUS::IG_STATUS_ON)
                    {
                        if (tmp_UploadTask->getIsRestartUpload() == true)
                        {
                            (void)mHandler->obtainMessage(MainHandler::MSG_RESTART_UPLOADING_IG)->sendToTarget();
                        }
                        if (tmp_UploadTask->getIsOperationA() == true)
                        {
                            (void)mHandler->obtainMessage(MainHandler::MSG_DO_OPERATION_A)->sendToTarget();
                        }
                    } else {
                        LOG_I("IG OFF => do not retry");
                    }
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
                    // mUploadingDB->saveUploadDB(tmp_UploadTask->getUploadPrio()
                    //     , static_cast<uint32_t>(tmp_UploadTask->getUploadFileType())
                    //     , static_cast<uint64_t>(tmp_UploadTask->getFileSize())
                    //     , tmp_UploadTask->getUploadPatch());
                    const uint64_t tmp_UploadID{tmp_UploadTask->getUploadId()};
                    if(tmp_UploadID > static_cast<uint64_t>(INT64_MAX)){
                        LOG_E("tmp_UploadID = %llu is out of range of int64_t", tmp_UploadID);
                    }
                    mRetryCheck[static_cast<int64_t>(tmp_UploadID)] = true;
                    // /*Received http response after IG OFF 30s => Ignore*/
                    // if (!mIgStoppedUpload)
                    // {
                    //     mqRestartUploadIG.push_back(tmp_UploadTask);
                    // }
                    // stopRetryTimer(tmp_UploadID);
                } else if(
                    (resGrpcStatusCode == grpc::StatusCode::UNKNOWN) ||
                    (resGrpcStatusCode == grpc::StatusCode::UNIMPLEMENTED) ||
                    (resGrpcStatusCode == grpc::StatusCode::INTERNAL) ||
                    (resGrpcStatusCode == grpc::StatusCode::UNAVAILABLE) ||
                    (resGrpcStatusCode == grpc::StatusCode::DATA_LOSS) ||
                    (resGrpcStatusCode == grpc::StatusCode::DEADLINE_EXCEEDED)
                ) {
                    /*Retry*/
                    const uint32_t retryCount {tmp_UploadTask->getRetryCount()};
                    const uint64_t tmp_UploadID {tmp_UploadTask->getUploadId()};
                    /*If receiving HTTP response after IG OFF 30s => do not retry*/
                    if(igStatus != IG_STATUS::IG_STATUS_OFF) {
                        if (retryCount > 3U) {
                            LOG_W("Stop retry to upload the UploadId: %llu", tmp_UploadID);
                            /*Do not set timer=>no retry more*/
                            // stopRetryTimer(tmp_UploadID);
                        } else {
                            LOG_I("Retry in processing");
                            const int32_t retryTime_spec{pGrpcResData->getRetryTime()};
                            if((retryTime_spec >= 0) && (retryTime_spec <= 3600)) {
                                LOG_I("Center specify retry time: %d", retryTime_spec);
                                LOG_I("Specify Retry uploadId: %llu after: %d", tmp_UploadID, retryTime_spec);
                                setRetryTimer(tmp_UploadID, static_cast<uint64_t>(retryTime_spec), tmp_UploadTask);
                            } else {
                                LOG_D("Retry time is not specified");
                            }
                        }
                        (void)tmp_UploadID;
                    } else {
                        LOG_I("IG OFF do not retry");
                        if (tmp_UploadID <= static_cast<uint64_t>(INT64_MAX)) {
                            mRetryCheck[static_cast<int64_t>(tmp_UploadID)] = true;
                        } else {
                            LOG_E("tmp_UploadID = %llu is out of range of int64_t", tmp_UploadID);
                        }
                    }
                    (void)retryCount;
                    (void)tmp_UploadID;

                }
                else {
                    LOG_E("Undefined case");
                }
                break;
            }
        }
        if (igStatus == IG_STATUS::IG_STATUS_ON)
        {
            /*If the upload fails even after retrying, try again after 30 minutes.*/
            const grpc::StatusCode resGrpcStatusCode{pGrpcResData->getGrpcCode()};
            if ((tmp_UploadTask->getRetryCount() == 4U) && (resGrpcStatusCode != grpc::StatusCode::OK))
            {
                const uint64_t tmp_retryInterval{tmp_UploadTask->getRetryInterval()};
                const uint64_t tmp_UploadID{tmp_UploadTask->getUploadId()};
                // cppcheck-suppress invalidPrintfArgType_uint
                LOG_I("Setup ReUpload Fail data UploadId: %llu after 30 mins", tmp_UploadID);
                setRetryTimer(tmp_UploadID, tmp_retryInterval, tmp_UploadTask);
                // cppcheck-suppress invalidPrintfArgType_uint
                LOG_I("Reupload UploadId: %llu after %llu sec", tmp_UploadID, tmp_retryInterval);
            }
            (void)resGrpcStatusCode;
        } else {
            LOG_I("IG OFF => do not retry");
        }
    } else {
        LOG_E("Received CALLID is NOT valid");
    }
}

void UploadManager::onReceiveIG(const bool status) {
    int32_t igStatus{0};
    if (status) {
        igStatus = 1;
    } else {
        igStatus = 0;
    }
    (void)mHandler->obtainMessage(MainHandler::MSG_ON_RECEIVE_IG, igStatus)->sendToTarget();
}

void UploadManager::handleOnReceiveIG(const bool status) {
    if (status == false)
    {
        LOG_I("IG OFF");
        LOG_I("Stop uploading after 30s");
        mTickTimer->start();
        LOG_D("Clear opA upload queue");
        const android::AutoMutex _l{mLock_OpA};
        mqOperationA.clear();
        /*Set each member of mRetryCheck to true*/
        for (std::map<int64_t, bool>::iterator it{mRetryCheck.begin()}; it != mRetryCheck.end(); ++it)
        {
            it->second = true;
        }
    }
    else
    {
        LOG_I("IG ON");
        mTickTimer->stop();
        /*Load upload fail data from database=>reupload*/
        /*Load upload data information from database*/
        mBackupUpload.clear();
        mqRestartUploadIG.clear();
        /*Reset store data*/
        resetStorageData();

        (void)mUploadingDB->getUploadDB(mBackupUpload);
        LOG_I("mBackupUpload size: %zu", mBackupUpload.size());
        /*Load upload failed data from data base*/
        loadBackupData();
    }
}

void UploadManager::setRDGStop(const bool isStop) noexcept {
    mRdgStop = isStop;
}

bool UploadManager::getRDGStop() const noexcept {
    return mRdgStop;
}

void UploadManager::onRdgStop(const bool isStop) {
    int32_t isStopStatus{0};
    if (isStop) {
        isStopStatus = 1;
        setRDGStop(true);
    } else {
        setRDGStop(false);
        isStopStatus = 0;
    }

    (void)mHandler->obtainMessage(MainHandler::CMD_STOP_RDG, isStopStatus)->sendToTarget();

    (void)isStop;
    //obtain message to stop RDG
}

void UploadManager::onServiceFlagChange() {
    LOG_I("RDGFlag off => Stop upload. Delete all upload pending");
    (void)mHandler->obtainMessage(MainHandler::MSG_ID_DETELE_PENDING_UPLOAD)->sendToTarget();
}

void UploadManager::onPPIChangedToFalse() {
    LOG_W("Consent status is not valid => discard all upload task");
    LOG_I("Consent status is not valid => delete un-upload data");
    /*delete un-uploaded data*/
    std::vector<CommonDefine::UploadFileAttribute> backupUpload{};

    (void)mUploadingDB->getUploadDB(backupUpload);

    for(std::vector<CommonDefine::UploadFileAttribute>::iterator ptr {backupUpload.begin()}; ptr != backupUpload.end(); ptr++) {
        /*Check if file is exist*/
        const std::string tmp_oldPath{std::string(UPLOAD_PATH) + ptr->getFilePath()};
#ifdef ENABLE_LGE_LXC
        if(RemoteFileStore::exists(ptr->getFilePath()) == true) {
            LOG_I("Delete exist file: %s", tmp_oldPath.c_str());
            if (!RemoteFileStore::removeFile(ptr->getFilePath())) {
#else
        if(FileUtil::isPathExist(tmp_oldPath.c_str()) == true) {
            LOG_I("Delete exist file: %s", tmp_oldPath.c_str());
            if (!FileUtil::removeFile(tmp_oldPath.c_str())) {
#endif /* ENABLE_LGE_LXC */
                LOG_E("Failed to delete file: %s", tmp_oldPath.c_str());
            }
            (void)mUploadingDB->deleteUploadDB(ptr->getFilePath());
        } else {
            LOG_I("Delete unexist file: %s", tmp_oldPath.c_str());
            (void)mUploadingDB->deleteUploadDB(ptr->getFilePath());
        }
    }
    /*Clear store data*/
    resetStorageData();

    if (mMspTaskQueue != nullptr)
    {
        mMspTaskQueue->clearQueue();
    }
    const android::AutoMutex _l{mLock_SentTask};
    mSentTask.clear();
}

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
            } else {
                LOG_E("Error: task not found in mListNotificationFilePath");
            }
        }
        if(m_coll_update_result_noti_max_storage > NOTIFICATION_MAX_STORAGE) {
            LOG_W("Excess m_coll_update_result_noti_max_storage");
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
            } else {
                LOG_E("Error: task not found in mListDtcRobFilePath");
            }
        }
        if(m_dtc_rob_max_storage > DTC_ROB_MAX_STORAGE) {
            LOG_W("Excess dtc_rob_max_storage");
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
            } else {
                LOG_E("Error: task not found in mListSsrFilePath");
            }
        }
        if(m_ssr_max_storage > SSR_MAX_STORAGE) {
            LOG_W("Excess ssr_max_storage");
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
            } else {
                LOG_E("Error: task not found in mListRoBSSRFilePath");
            }
        }
        if(m_robssr_max_storage > ROBSSR_MAX_STORAGE) {
            LOG_W("Excess robssr_max_storage");
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
            } else {
                LOG_E("Error: task not found in mListDirectCommandFilePath");
            }
        }
        if(m_directcommand_max_storage > DIRECTCOMMAND_MAX_STORAGE) {
            LOG_W("Excess directcommand_max_storage");
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
            } else {
                LOG_E("Error: task not found in mListWarningFilePath");
            }
        }
        if(m_warning_max_storage > WARNING_MAX_STORAGE) {
            LOG_W("Excess warning_max_storage");
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
            } else {
                LOG_E("Error: task not found in mListEcuInforFilePath");
            }
        }
        if(m_ecu_infor_max_storage > ECU_INFOR_MAX_STORAGE) {
            LOG_W("Excess ecu_infor_max_storage");
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
            } else {
                LOG_E("Error: task not found in mListLastUploadFilePath");
            }
        }
        if(m_last_max_storage > LAST_MAX_STORAGE) {
            LOG_W("Excess last_max_storage");
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
            } else {
                LOG_E("Error: task not found in mListErrorFilePath");
            }
        }
        if(m_error_max_storage > ERROR_MAX_STORAGE) {
            LOG_W("Excess error_max_storage");
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
    if (mMspTaskQueue != nullptr)
    {
        LOG_D("Clear mMspTaskQueue");
        mMspTaskQueue->clearQueue();
    }
    else
    {
        LOG_W("mMspTaskQueue is null");
    }
    /* Delete task is wait response */
    {
        const android::AutoMutex _l{mLock_SentTask};
        /*Get uploadTask from waiting task of mSentTask to mqRestartUploadIG*/
        for (std::multimap<int32_t, android::sp<UploadTask>>::iterator it {mSentTask.begin()}; it != mSentTask.end(); ++it) {
            const android::sp<UploadTask> tmp_task{it->second};
            if (tmp_task != nullptr)
            {
                /*Get UploadID of UploadTask and set mRetryCheck to false*/
                const uint64_t tmp_UploadID{tmp_task->getUploadId()};
                if (tmp_UploadID > static_cast<uint64_t>(INT64_MAX))
                {
                    //cppcheck-suppress invalidPrintfArgType_uint
                    LOG_E("tmp_UploadID = %llu is out of range of int64_t", tmp_UploadID);
                }
                /*Waiting upload data will not be retry from here*/
                mRetryCheck[static_cast<int64_t>(tmp_UploadID)] = true;
            }
            else
            {
                LOG_D("tmp_task is null");
            }
        }
        // mSentTask.clear();//comment due to still process http response
    }
    {
        const android::AutoMutex _l{mLock_Retry};
        /* Delete task is wait retry */
        /*Print mTimers_RetryMap size*/
        LOG_D("mTimers_RetryMap size: %d", mTimers_RetryMap.size());
        for (std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>>::iterator it {mTimers_RetryMap.begin()}; it != mTimers_RetryMap.end(); ++it) {
            const android::sp<Timer> ptimer {it->second.first};
            if(ptimer != nullptr) {
                ptimer->stop();
            } else {
                LOG_D("ptimer is null");
            }
        }
    }
    {
        const android::AutoMutex _l{mLock_Retry};
        /*Iterate through mTimers_RetryMap and get android::sp<UploadTask> then push to mqRestartUploadIG*/
        for (std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>>::iterator it {mTimers_RetryMap.begin()}; it != mTimers_RetryMap.end(); ++it) {
            const android::sp<UploadTask> tmp_task{it->second.second};
            if (tmp_task != nullptr)
            {
                /*Get UploadID of UploadTask and set mRetryCheck to false*/
                const uint64_t tmp_UploadID{tmp_task->getUploadId()};
                if (tmp_UploadID > static_cast<uint64_t>(INT64_MAX))
                {
                    //cppcheck-suppress invalidPrintfArgType_uint
                    LOG_E("tmp_UploadID = %llu is out of range of int64_t", tmp_UploadID);
                }
                mRetryCheck[static_cast<int64_t>(tmp_UploadID)] = true;
            }
            else
            {
                LOG_D("tmp_task is null");
            }
        }
        mTimers_RetryMap.clear();
    }
    LOG_D("mTimers_RetryMap size: %d", mTimers_RetryMap.size());
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
void UploadManager::loadBackupData()
{
    /*Iterate mBackupUpload and check if file is existed 
    * if file is existed => make new android::sp<UploadTask>. then add task to mqRestartUploadIG. Then break loop
    * Check if mBackupUpload have data*/

    while (!mBackupUpload.empty())
    {
        /*Check if file is exist*/
        const std::string tmp_oldPath{std::string(UPLOAD_PATH) + mBackupUpload.front().getFilePath()};

#ifdef ENABLE_LGE_LXC
        if (RemoteFileStore::exists(mBackupUpload.front().getFilePath()) == true)
#else
        if (FileUtil::isPathExist(tmp_oldPath.c_str()) == true)
#endif /* ENABLE_LGE_LXC */
        {
            LOG_I("%s exist", tmp_oldPath.c_str());
            const uint32_t uploadId {genRequestId()};
            const android::sp<UploadTask> task{new UploadTask(uploadId)};
            task->setUploadFileType(convertIntToFileType(mBackupUpload.front().getFileType()));

            const std::string file_dir{mBackupUpload.front().getFilePath()};
            task->setUploadPatch(file_dir);
            LOG_I("File: %s", file_dir.c_str());
            /*Set priority*/
            task->setUploadPrio(mBackupUpload.front().getPriority());
            task->setFileSize(mBackupUpload.front().getFileSize());
            updateStorage(task, true);
            mqRestartUploadIG.push_back(task);
            (void)mBackupUpload.erase(mBackupUpload.cbegin());
            /*Check internal IG status*/
            if (mApp.getInternalIGStatus() == true)
            {
                (void)mHandler->obtainMessage(MainHandler::MSG_LOAD_BACKUP_DATA)->sendToTarget();
            } else {
                LOG_I("IG OFF => Stop load backup data");
            }
            
            break;
        }
        else
        {
            LOG_I("Delete unexist file: %s", tmp_oldPath.c_str());
            (void)mUploadingDB->deleteUploadDB(mBackupUpload.front().getFilePath());
            (void)mBackupUpload.erase(mBackupUpload.cbegin());
        }
    }
}
void UploadManager::handleRestartUploadIG() {
    LOG_I("Start to restart uploading");
    loadBackupData();
    while(mqRestartUploadIG.empty() != true) {
        const android::sp<UploadTask> tmp_task{mqRestartUploadIG.front()};
        mqRestartUploadIG.pop_front();
        LOG_I("Do restart UploadId: %llu File: %s", 
            tmp_task->getUploadId(), tmp_task->getUploadPatch().c_str());
        tmp_task->resetRetryCount();
        tmp_task->setIsRestartUpload(true);
        restartUploadTask(tmp_task);
        break;
    }
}

void UploadManager::handleOperationA() {
    /*ReUpload task saved in operationAQueue*/
    const android::AutoMutex _l{mLock_OpA};
    while(mqOperationA.empty() != true) {
        const android::sp<UploadTask> tmp_task{mqOperationA.front()};
        mqOperationA.pop_front();
        LOG_I("Do operation A UploadId: %llu File: %s", 
            tmp_task->getUploadId(), tmp_task->getUploadPatch().c_str());
        tmp_task->resetRetryCount();
        tmp_task->setIsOperationA(true);
        restartUploadTask(tmp_task);
        LOG_D("Done operation A");
        break;
    }
}

void UploadManager::onRestartUploading() {
    // loadBackupData();
    (void)mHandler->obtainMessage(MainHandler::MSG_RESTART_UPLOADING_IG)->sendToTarget();
    /*Restart Upload for Operation A*/
    const android::AutoMutex _l{mLock_OpA};
    if (mqOperationA.empty() == false)
    {
        LOG_D("Restart upload for Operation A");
        (void)mHandler->obtainMessage(MainHandler::MSG_DO_OPERATION_A)->sendToTarget();
    } else {
        LOG_W("No data to restart upload for Operation A");
    }
}

void UploadManager::operationA() {
    const IG_STATUS igStatus{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
    if(igStatus == IG_STATUS::IG_STATUS_ON) {
        LOG_I("IG ON => Execute operation A");
        (void)mHandler->obtainMessage(MainHandler::MSG_DO_OPERATION_A)->sendToTarget();
    } else {
        LOG_I("IG OFF => Stop operation A");
    } 
}

void UploadManager::doCallIdTimeout(const int32_t callid) {
    (void)mHandler->obtainMessage(MainHandler::MSG_CALLID_TIMEOUT, callid, 0)->sendToTarget();
}

void UploadManager::onCallIdTimeout(const int32_t callid) {
    LOG_I("Check call id: %d", callid);
    /*1. Trigger no response from center => retry*/
    android::sp<UploadTask> tmp_UploadTask{nullptr};
    {
        const android::AutoMutex _l{mLock_SentTask};
        const std::multimap<int32_t, android::sp<UploadTask>>::iterator it {mSentTask.find(callid)};
        if(it != mSentTask.end()) {
            tmp_UploadTask = it->second;
            (void)mSentTask.erase(it);
        } else {
            LOG_D("Can not find call id in mSentTask: %d", callid);
        }
    }

    if(tmp_UploadTask != nullptr) {
        const uint64_t tmp_UploadID{tmp_UploadTask->getUploadId()};
        setRetryTimer(tmp_UploadID, tmp_UploadTask->getRetryInterval(), tmp_UploadTask);
        //cppcheck-suppress invalidPrintfArgType_uint
        LOG_I("Reupload UploadId: %llu after %llu sec", tmp_UploadID, tmp_UploadTask->getRetryInterval());
    } else {
        LOG_D("tmp_UploadTask is null");
    }
    // android::sp<Timer> tmp_Timer{nullptr};
    // {
    //     const android::AutoMutex _lock{mLock_ReceivedRes};
    //     const std::multimap<int32_t, android::sp<Timer>>::iterator it_timer {mTimers_Uploading.find(callid)};
    //     if(it_timer != mTimers_Uploading.end()) {
    //         tmp_Timer = it_timer->second;
    //         (void)mTimers_Uploading.erase(it_timer);
    //     } else {
    //         LOG_D("Can not find call id in mTimers_Uploading: %d", callid);
    //     }
    // }
    // if(tmp_Timer != nullptr) {
    //     LOG_I("Stop timer uploading timeout for call id: %d", callid);
    //     tmp_Timer->stop();
    // }
}

void UploadManager::setRetryTimer(const uint64_t uploadId, const uint64_t duration, const android::sp<UploadTask> task){
    LOG_D("setRetryTimer uploadId: %llu duration: %llu", uploadId, duration);
    if(uploadId > static_cast<uint64_t>(INT32_MAX)) {
        LOG_E("uploadId is greater than INT32_MAX");
    }
    android::sp<Timer> ptimer{nullptr};
    
    const android::AutoMutex _l{mLock_Retry};
    /*Find timer id: uploadId and stop if it is in progress*/
    LOG_D("mTimers_RetryMap size: %d", mTimers_RetryMap.size());
    const std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>>::iterator it{mTimers_RetryMap.find(static_cast<int32_t>(uploadId))};
    if (it != mTimers_RetryMap.end()) {
        ptimer = it->second.first;
        // const android::sp<UploadTask> uploadTask = it->second.second;
        
    } else {
        LOG_D("Do not found retry timer id: %llu", uploadId);
    }
    
    if(ptimer != nullptr) {
        ptimer->stop();
    }

    mTickTimer_Retry = android::sp<Timer>(new Timer(mpTimerHandler_Retry.get(), static_cast<int32_t>(uploadId)));
    if(duration > static_cast<uint64_t>(UINT32_MAX)) {
        LOG_E("out of range");
    }
    mTickTimer_Retry->setDuration(static_cast<uint32_t>(duration), 0U);
    const std::pair<android::sp<Timer>, android::sp<UploadTask>> tmp_pair{std::make_pair(mTickTimer_Retry, task)};
    mTimers_RetryMap[static_cast<int32_t>(uploadId)] = tmp_pair;
    LOG_D("mTimers_RetryMap size: %d", mTimers_RetryMap.size());
    mTickTimer_Retry->start();
    mTickTimer_Retry = nullptr;
}

void UploadManager::stopRetryTimer(const uint64_t uploadId) {
    LOG_D("stopRetryTimer uploadId: %llu", uploadId);
    if(uploadId > static_cast<uint64_t>(INT32_MAX)) {
        LOG_E("uploadId is greater than INT32_MAX");
    }
    android::sp<Timer> ptimer{nullptr};
    {
        const android::AutoMutex _l{mLock_Retry};
        /*Find timer id: uploadId and stop if it is in progress*/
        const std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>>::iterator it{mTimers_RetryMap.find(static_cast<int32_t>(uploadId))};

        if (it != mTimers_RetryMap.end()) {
            ptimer = it->second.first;
            // const android::sp<UploadTask> uploadTask = it->second.second;
            (void)mTimers_RetryMap.erase(it);
        } else {
            /*TBD*/
            LOG_E("Do not found %llu in Timers_RetryMap", uploadId);
        }
    }
    if(ptimer != nullptr) {
            ptimer->stop();
    } else {
        LOG_D("ptimer is null");
    }

}

void UploadManager::doRetryTimeOut(const int32_t timerId) {
    LOG_I("UploadId doRetryTimeOut: %d", timerId);
    (void)mHandler->obtainMessage(MainHandler::MSG_RETRY_TIMEOUT, timerId)->sendToTarget();
}

void UploadManager::onRetryTimeOut(const int32_t timerId) {
    LOG_I("UploadId onRetryTimeOut: %d", timerId);
    /*Do upload have id is timerId*/
    /*Get UploadTask need to check to retry*/
    android::sp<UploadTask> task_tmp{nullptr};
    {
        const android::AutoMutex _l{mLock_Retry};
        const std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>>::iterator it{mTimers_RetryMap.find(timerId)};
        if (it != mTimers_RetryMap.end()) {
            task_tmp = it->second.second;
            (void)mTimers_RetryMap.erase(it);
        } else {
            LOG_W("Do not found retry timer id: %d", timerId);
        }
    }
    bool mCanretry{true};
    if(mRetryCheck[static_cast<int64_t>(timerId)] == false){
        mCanretry = true;
    } else {
        mCanretry = false;
    }

    if((task_tmp != nullptr) && (mCanretry == true)) {
        LOG_I("Do retry upload");
        LOG_I("Check received retry upload task");
        /*Check file upload existed*/
        const std::string mPath {std::string(UPLOAD_PATH) + task_tmp->getUploadPatch()};

#ifdef ENABLE_LGE_LXC
        if(RemoteFileStore::exists(task_tmp->getUploadPatch()) == true) {
#else
        if(FileUtil::isPathExist(mPath.c_str()) == true) {
#endif /* ENABLE_LGE_LXC */
            LOG_I("UploadId: %llu Retry: %u", task_tmp->getUploadId() ,task_tmp->getRetryCount());
            LOG_I("File dir: %s FileType: %d Prio: %u ",
                task_tmp->getUploadPatch().c_str(), task_tmp->getUploadFileType(), task_tmp->getUploadPrio());
            /*Add task to queue*/
            if((DiagManagerAdapter::getInstance()->getAllUploadConsent() == true) || (task_tmp->getSRVC_Flag())) {
                addTask_delayed(task_tmp);
            } else {
                LOG_I("Upload consent is not valid => dont add uploadTask");
                LOG_I("Do not upload due to permission. Delete upload file");
                LOG_D("Check AllUploadConsent: %s", DiagManagerAdapter::getInstance()->getAllUploadConsent()?"TRUE":"FALSE");
                LOG_D("Check SRVC_Flag: %s", task_tmp->getSRVC_Flag()?"ON":"OFF");
                /*Delete file*/
                if (!task_tmp->deteleUploadFile()) {
                    LOG_E("Failed to delete upload file: %s", task_tmp->getUploadPatch().c_str());
                }
            }
        } else {
            LOG_W("File do not exist: %s", mPath.c_str());
        }
    }  else {
        //Show log do not upload file when timeout
        LOG_D("Do not upload file when timeout");
        /*delete mRetryCheck member*/
        if(mRetryCheck.find(static_cast<int64_t>(timerId)) != mRetryCheck.end()) {
            (void)mRetryCheck.erase(static_cast<int64_t>(timerId));
        }
    }
    (void)mCanretry;
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
                LOG_I("File dir: %s FileType: %d Prio: %d FileSize: %llu",
                    upTask->getUploadPatch().c_str(), upTask->getUploadFileType(), upTask->getUploadPrio(), upTask->getFileSize());
                /*Add task to queue*/
                /*If upload consent is not valid => dont add uploadTask*/
                if((DiagManagerAdapter::getInstance()->getAllUploadConsent() == true) ||
                    (upTask->getSRVC_Flag()) ||
                    (upTask->getUploadFileType() == GRPC_IF_TYPE::DCIF_RDG012))
                {
                    mParent.addTask(upTask, true);
                    mParent.updateStorage(upTask, true);
                } else {
                    LOG_I("Upload consent is not valid => dont add uploadTask");
                    LOG_I("Do not upload due to permission. Delete upload file");
                    LOG_D("Check AllUploadConsent: %s", DiagManagerAdapter::getInstance()->getAllUploadConsent()?"TRUE":"FALSE");
                    LOG_D("Check SRVC_Flag: %s", upTask->getSRVC_Flag()?"ON":"OFF");
                    /*Delete file*/
                    if (!upTask->deteleUploadFile()) {
                        LOG_E("Failed to delete upload file: %s", upTask->getUploadPatch().c_str());
                    }
                }
            } else {
                LOG_E("upTask is nullptr");
            }
            break;
        }
        case MSG_RESTART_UPLOAD_TASK:
        {
            LOG_I("MSG_RESTART_UPLOAD_TASK");
            android::sp<UploadTask> upTask{nullptr};
            handlemsg->getObject(upTask);
            if(upTask != nullptr) {
                LOG_I("Check received upload task");
                LOG_D("Check SRVC_Flag: %s", upTask->getSRVC_Flag()?"ON":"OFF");
                LOG_I("File dir: %s FileType: %d Prio: %d FileSize: %llu",
                    upTask->getUploadPatch().c_str(), upTask->getUploadFileType(), upTask->getUploadPrio(), upTask->getFileSize());
                /*Add task to queue*/
                /*If upload consent is not valid => dont add uploadTask*/
                if((DiagManagerAdapter::getInstance()->getAllUploadConsent() == true) ||
                    (upTask->getSRVC_Flag()) ||
                    (upTask->getUploadFileType() == GRPC_IF_TYPE::DCIF_RDG012))
                {
                    mParent.addTask(upTask, false);
                    // mParent.updateStorage(upTask, true);
                } else {
                    LOG_I("Upload consent is not valid => dont add uploadTask");
                    LOG_I("Do not upload due to permission. Delete upload file");
                    LOG_D("Check AllUploadConsent: %s", DiagManagerAdapter::getInstance()->getAllUploadConsent()?"TRUE":"FALSE");
                    LOG_D("Check SRVC_Flag: %s", upTask->getSRVC_Flag()?"ON":"OFF");
                    /*Delete file*/
                    if (!upTask->deteleUploadFile()) {
                        LOG_E("Failed to delete upload file: %s", upTask->getUploadPatch().c_str());
                    }
                }
            } else {
                LOG_I("upTask is nullptr");
            }
            break;
        }
        case MSG_ID_DO_UPLOAD:
        {
            const bool isUploadStop{mParent.getRDGStop()};
            if(isUploadStop == false) {
                LOG_I("MSG_ID_DO_UPLOAD");
                mParent.doUpload();
            } else {
                LOG_I("RDG is stop => Do not upload");
            }
            break;
        }
        case MSG_ID_DELAYED_INSERT_MSG:
        {
            LOG_I("MSG_ID_DELAYED_INSERT_MSG");
            break;
        }
        case MSG_RESTART_UPLOADING_IG:
        {
            LOG_I("MSG_RESTART_UPLOADING_IG");
            mParent.handleRestartUploadIG();
            break;
        }
        case MSG_DO_OPERATION_A:
        {
            LOG_I("MSG_DO_OPERATION_A");
            mParent.handleOperationA();
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
        case MSG_RECEIVE_GRPC_RES:
        {
            LOG_I("MSG_RECEIVE_GRPC_RES");
            android::sp<GrpcResData> resData{nullptr};
            handlemsg->getObject(resData);
            mParent.onReceivedGrpcRes(resData);
            break;
        }
        case MSG_RETRY_TIMEOUT:
        {
            LOG_I("MSG_RETRY_TIMEOUT");
            const int32_t tmp_TimerId{handlemsg->arg1};
            mParent.onRetryTimeOut(tmp_TimerId);
            break;
        }
        case MSG_CALLID_TIMEOUT:
        {
            LOG_I("MSG_CALLID_TIMEOUT");
            const int32_t tmp_CallId{handlemsg->arg1};
            mParent.onCallIdTimeout(tmp_CallId);
            break;
        }
        case MSG_ON_RECEIVE_IG: {
            bool status{true};
            if (handlemsg->arg1 == 1) {
                status = true;
            } else {
                status = false;
            }
            mParent.handleOnReceiveIG(status);
            break;
        }
        case MSG_LOAD_BACKUP_DATA:
        {
            LOG_I("MSG_LOAD_BACKUP_DATA");
            mParent.loadBackupData();
            break;
        }
        case CMD_STOP_RDG:
        {
            if (handlemsg->arg1 == 1) {
                LOG_I("CMD_STOP_RDG");
                mParent.setRDGStop(true);
            } else {
                mParent.setRDGStop(false);
            }
            break;
        }
        default:
        {
            LOG_E("default");
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
            LOG_W("ID_IG_OFF_STOP_UPLOADING");
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
    const std::string deletePath{std::string(UPLOAD_PATH) + taskDeleted->getUploadPatch()};
    (void)taskDeleted;
#ifdef ENABLE_LGE_LXC
    if(RemoteFileStore::exists(taskDeleted->getUploadPatch()) == true)
    {
        LOG_I("[deleteOldestFile] Delete exist file: %s", deletePath.c_str());
        if (RemoteFileStore::removeFile(taskDeleted->getUploadPatch()) != true)
#else
    if(FileUtil::isPathExist(deletePath.c_str()) == true)
    {
        LOG_I("[deleteOldestFile] Delete exist file: %s", deletePath.c_str());
        if (FileUtil::removeFile(deletePath.c_str()) != true)
#endif /* ENABLE_LGE_LXC */
        {
            LOG_E("[deleteOldestFile] delete fail !");
        }
    }
    else {
        LOG_W("[deleteOldestFile] Delete unexist file: %s", deletePath.c_str());
    }
    updateStorage(taskDeleted, false);
}

void UploadManager::TimerHandler_Uploading::handlerFunction(const int32_t timerId)
{
    LOG_I("TimerHandler_Uploading CallID: %d", timerId);
    mUploadManager.doCallIdTimeout(timerId);
}

void UploadManager::TimerHandler_Retry::handlerFunction(const int32_t timerId)
{
    LOG_I("TimerHandler_Retry UploadId: %d", timerId);
    mUploadManager.doRetryTimeOut(timerId);
}

uint32_t UploadManager::getCounterValue() noexcept
{
    const std::string propCounter{"remotediag.prop.counterValue"};
    const char_t *const strCounter{Remotediag::getInstance()->getProperty(propCounter.c_str())};
    std::string counterVal{""};

    if (strCounter != nullptr)
    {
        counterVal = strCounter;
    }
    else
    {
        LOG_I("No set property, set to default");
        counterVal = "00000000";
    }
    uint32_t mCounterValue{CommonUtils::hexStringToUint32(counterVal)};
    if (mCounterValue >= UINT32_MAX)
    {
        mCounterValue = 0U;
    }
    else
    {
        mCounterValue++;
    }
    counterVal = CommonUtils::uint32ToHexString(mCounterValue);
    Remotediag::getInstance()->setProperty(propCounter.c_str(), counterVal.c_str(), true);
    return mCounterValue;
}

uint32_t UploadManager::getCounterMessage() noexcept
{
    if (mCounterMess >= NUMBER_UPLOAD_DATA_IS_MADE_AFTER_IG_ON_MAX)
    {
        mCounterMess = 0U;
    }
    else
    {
        mCounterMess++;
    }
    return mCounterMess;
}

void UploadManager::resetCounterByIgON() noexcept
{
    mCounterMess = 0U;
}
void UploadManager::clearSentQueue() {
    {
        LOG_D("Clear sent queue");
        const android::AutoMutex _l{mLock_SentTask};
        /*Clear sent queue task*/
        mSentTask.clear();
    }
    {
        const android::AutoMutex _lock{mLock_ReceivedRes};
        for (std::multimap<int32_t, android::sp<Timer>>::iterator it {mTimers_Uploading.begin()}; it != mTimers_Uploading.end(); ++it) {
            // it->second is of type android::sp<Timer>
            if(it->second != nullptr) {
                it->second->stop();
            } else {
                LOG_D("Timer pointer is null");
            }
        }
        mTimers_Uploading.clear();
    }

}

void UploadManager::resetStorageData() {
    LOG_D("Reset storage data");
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
    mCounterMess = 0U;

    mListDtcRobFilePath.clear();
    mListSsrFilePath.clear();
    mListDirectCommandFilePath.clear();
    mListRoBSSRFilePath.clear();
    mListWarningFilePath.clear();
    mListEcuInforFilePath.clear();
    mListLastUploadFilePath.clear();
    mListNotificationFilePath.clear();
    mListErrorFilePath.clear();
}
void UploadManager::testSetUploadStorage(const int32_t type, const uint64_t value) noexcept
{
    switch(type)
    {
        case 1:
        {
            m_dtc_rob_max_storage = value;
            break;
        }
        case 2:
        {
            m_ssr_max_storage = value;
            break;
        }
        case 3:
        {
            m_directcommand_max_storage = value;
            break;
        }
        case 4:
        {
            m_robssr_max_storage = value;
            break;
        }
        case 5:
        {
            m_warning_max_storage = value;
            break;
        }
        case 6:
        {
            m_ecu_infor_max_storage = value;
            break;
        }
        case 7:
        {
            m_last_max_storage = value;
            break;
        }
        case 8:
        {
            m_error_max_storage = value;
            break;
        }
        case 9:
        {
            m_coll_update_result_noti_max_storage = value;
            break;
        }
        default:
        {
            break;
        }
    }
}

void UploadManager::testGetUploadStorage() noexcept
{
    LOG_I("1. m_dtc_rob_max_storage: %llu", m_dtc_rob_max_storage);
    LOG_I("2. m_ssr_max_storage: %llu", m_ssr_max_storage);
    LOG_I("3. m_directcommand_max_storage: %llu", m_directcommand_max_storage);
    LOG_I("4. m_robssr_max_storage: %llu", m_robssr_max_storage);
    LOG_I("5. m_warning_max_storage: %llu", m_warning_max_storage);
    LOG_I("6. m_ecu_infor_max_storage: %llu", m_ecu_infor_max_storage);
    LOG_I("7. m_last_max_storage: %llu", m_last_max_storage);
    LOG_I("8. m_error_max_storage: %llu", m_error_max_storage);
    LOG_I("9. m_coll_update_result_noti_max_storage: %llu", m_coll_update_result_noti_max_storage);
}

void UploadManager::testCounterValue(const uint32_t valTest) noexcept
{
    const std::string propCounter{"remotediag.prop.counterValue"};
    std::string strCounter{""};
    const uint32_t counterValue{UINT32_MAX - valTest};
    strCounter = CommonUtils::uint32ToHexString(counterValue);
    Remotediag::getInstance()->setProperty(propCounter.c_str(), strCounter.c_str(), true);
}

} //end namespace
