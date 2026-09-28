#include <google/protobuf/util/json_util.h>
#include "RemoteLastUpload.h"

namespace rdgapp {

namespace interface = vccomif::rdg::v1::interfaces;

RemoteLastUpload::RemoteLastUpload(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper):
        android::RefBase()
        , mApp(app)
        , mHandler(new MainHandler(privateLooper, *this))
        , mTriggerId{0}
        , mTriggerType{DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN}
        , mPriority{0U}
        , mColId{0U}
        , mLocationData(nullptr)
        , mWarningTriggerOccurrenceTime{0U}
        , mDiagnosticsAcquisitionTime{0U}
{
    LOG_D("RemoteLastUpload constructor");
    mLocationData = new CommonDefine::RDGLocationData;
}

RemoteLastUpload::~RemoteLastUpload() = default;

void RemoteLastUpload::printData(const std::string data) const
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

void RemoteLastUpload::receiveProcessDone(const uint32_t triggerID) 
{
    LOG_I("receiveProcessDone: %u",triggerID);
    uint32_t counterValue{mFunctionCountByID[triggerID]};
    if (counterValue <= UINT32_MAX)
    {
        counterValue++;
    }
    mFunctionCountByID[triggerID] = counterValue;
    if (mFunctionCountByID[triggerID] == 3U)
    {
        const android::AutoMutex _l{mSaveReqLock};
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(triggerID)};
        android::sp<DiagTrigger> tmpTrigger{nullptr};
        if(it != mSaveReq.end())
        {
            tmpTrigger = it->second; 
        } else {
            LOG_D("Can not find triggerId: %d in mSaveReq", triggerID);
        }
        if(tmpTrigger != nullptr) {
            (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_DATA)->sendToTarget();
            (void)mFunctionCountByID.erase(triggerID);
        } else {
            LOG_D("Can not find triggerId: %d in mSaveReq", triggerID);
        }
    } else {
        LOG_D("mFunctionCountByID[%u]: %u", triggerID, mFunctionCountByID[triggerID]);
    }
}

//RDG30-R-1162
void RemoteLastUpload::makeUploadData() {
    LOG_I("Make Last Upload data");
    const android::AutoMutex _l{mSaveReqLock};
    android::sp<DiagTrigger> tmpTrigger{nullptr};
    if (mTriggerId >= 0)
    {
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(static_cast<uint32_t>(mTriggerId))};
        if(it != mSaveReq.end())
        {
            tmpTrigger = it->second; 
        } else {
            LOG_D("Can not find triggerId: %d in mSaveReq", mTriggerId);
        }
    }
    if(tmpTrigger != nullptr) {
        mTriggerType = tmpTrigger->getType();
        mPriority = tmpTrigger->getPriority();
        mColId = tmpTrigger->getCollectionID();
        mLocationData->setLatitude(tmpTrigger->getLatitude());
        mLocationData->setLongitude(tmpTrigger->getLongtitude());
        if (mTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
        {
            const int64_t triggerTime{tmpTrigger->getWarningTriggerTime()};
            if(triggerTime >= 0)
            {
                mWarningTriggerOccurrenceTime = static_cast<uint64_t>(triggerTime);
                LOG_I("mWarningTriggerOccurrenceTime: %llu", mWarningTriggerOccurrenceTime);
            }
        } else {
            const int64_t triggerTime{tmpTrigger->getTriggerTime()};
            if(triggerTime >= 0)
            {
                mDiagnosticsAcquisitionTime = static_cast<uint64_t>(triggerTime);
                LOG_I("Diagnostics Acquisition Time: %llu", mDiagnosticsAcquisitionTime);
            }
        }
        if (mTriggerId >= 0)
        {
            (void)mFunctionCountByID.erase(static_cast<uint32_t>(mTriggerId));
        }
    } else {
        LOG_D("Can not find triggerId: %d in mSaveReq", mTriggerId);
    }
    mLastUploadDataReq = std::shared_ptr<UploadLastDataRequest>(
        new UploadLastDataRequest());
    /*RdgCommonRequestHeader*/
    // text_version
    // electronic_pf
    // geodesy_information
    // time_zone_offset
        /*RdgCommonRequestHeader*/
    // text_version
    mLastUploadDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
    // electronic_pf
    mLastUploadDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    // geodesy_information
    mLastUploadDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(CommonUtils::getGeodesyInfo());
    // time_zone_offset
    mLastUploadDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
    mLastUploadDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());
    /*interface_type*/
    mLastUploadDataReq->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_LAST_DATA);
    mLastUploadDataReq->set_counter_value(UploadManager::getInstance()->getCounterValue());
    /*message_id*/
    mLastUploadDataReq->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_LAST_DATA, UploadManager::getInstance()->getCounterMessage()));
    /*collection_condition_id*/
    mLastUploadDataReq->set_collection_condition_id(mColId);
    /*TriggerType*/
    if (mTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
        mLastUploadDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_WARNING_TRIGGER);
    } else if (mTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
        mLastUploadDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_IG_ON_TRIGGER);
    } else if (mTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) {
        mLastUploadDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER);
    } else {
        mLastUploadDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_UNKNOWN);
    }

    /*data_creation_date*/
    /*diagnostics_acquisition_time || warning_trigger_occurrence_time*/
    if (mTriggerType != DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        mLastUploadDataReq->set_data_creation_date(mDiagnosticsAcquisitionTime);
        mLastUploadDataReq->set_diagnostics_acquisition_time(mDiagnosticsAcquisitionTime);
    } else {
        mLastUploadDataReq->set_data_creation_date(mWarningTriggerOccurrenceTime);
        mLastUploadDataReq->set_warning_trigger_occurrence_time(mWarningTriggerOccurrenceTime);
    }
    /*obd2_installed_flag*/
    const bool bOBDFlag{mApp.getOBDStatus()};
    mLastUploadDataReq->set_obd2_installed_flag(bOBDFlag);
    /*under_repair_flag*/
    mLastUploadDataReq->set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true:false);

    //RDG30-R-1037
    uint32_t sizeOfFileCounter {0U};
    sizeOfFileCounter = mLastUploadDataReq->ByteSizeLong();
    static constexpr uint32_t LAST_UPLOAD_DATA_SIZE_MAX{4194304U}; // bytes = 4MB
    if ((sizeOfFileCounter) > LAST_UPLOAD_DATA_SIZE_MAX)
    {
        LOG_I("Last upload data exceeds 4MB -> discard exceeds data");
    }
    else
    {
        LOG_I("Last upload data size = %u", sizeOfFileCounter);
    }
    std::string LastDataReq_Str{};
    google::protobuf::util::JsonOptions option{};
    option.always_print_primitive_fields = true;
    option.preserve_proto_field_names = true;
    (void)google::protobuf::util::MessageToJsonString(*mLastUploadDataReq, &LastDataReq_Str, option);
    LOG_I("Print Last Upload data");
    printData(LastDataReq_Str);

    /*Save UploadLastDataRequest to file*/ 
    //RDG30-R-1161
    const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
    const uint64_t uploadCount {UploadManager::getInstance()->genCountUpload()};
    std::string file_dir {std::to_string(uploadCount)};
    (void)file_dir.append("_UploadLastDataRequest.dat");
    uint32_t fileSize{0U};
    error_t bSaved{E_ERROR};
    const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
    if (region == LGE_REGION::LGE_REGION_CN)
    {
        bSaved = DataModel<UploadLastDataRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG120, file_dir, *mLastUploadDataReq, fileSize);
    }
    else
    {
        fileSize = mLastUploadDataReq->ByteSizeLong();
        bSaved = DataModel<UploadLastDataRequest>::saveUpload(file_dir, *mLastUploadDataReq);
    }
    if (bSaved == E_OK)
    {
        const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
        DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        const android::sp<UploadTask> task{new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG120);
        task->setUploadPatch(file_dir);
        /*Set priority*/
        task->setUploadPrio(mPriority);
        // test_saveUploadData = task;
        task->setFileSize(static_cast<uint64_t>(fileSize));
        UploadManager::getInstance()->requestUploadTask(task);
    }
    else
    {
        LOG_E("Failed to store UploadLastDataRequest to file");
    }
    
    LOG_I("request done");

}
//RDG30-R-1161
void RemoteLastUpload::triggerLastUpload(
    const uint32_t triggerID
, const DiagTrigger::DiagTriggerType triggerType
, const int64_t timeData
, const android::sp<CommonDefine::RDGLocationData> location
, const uint64_t collectionId
, const uint32_t priority) {
    LOG_I("Triggered Last upload");
    if (triggerID < static_cast<uint32_t>(INT32_MAX))
    {
        mTriggerId = static_cast<int32_t>(triggerID);
    } else {
        LOG_E("triggerID out of range");
    }
    const uint32_t nextTriggerId{triggerID};
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
    const android::AutoMutex _l{mSaveReqLock};
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
    LOG_D("Check mSaveReq size: %d", mSaveReq.size());
    if(!ret.second) {
        ret.first->second = pTrigger;
    }
    LOG_D("Check saved Last Upload trigger ID: %u", ret.first->first);
}
// void RemoteLastUpload::onRdgStop(const bool isStop) const noexcept {
//     //obtain message to stop rdg
//     const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_STOP_RDG)};
//     (void)msg->sendToTarget();
// }
void RemoteLastUpload::MainHandler::handleMessage (const android::sp<sl::Message>& handlemsg) 
{
    const int32_t what {handlemsg->what};
    LOG_I({"handler is processing with what: %d"}, what);
    switch (what) {
        case CMD_MAKE_DATA:
        {
            LOG_I("CMD_MAKE_LUD");
            mLU.makeUploadData();
            break;
        }
        // case CMD_STOP_RDG:
        // {
        //     LOG_I("CMD_STOP_RDG");
        //     break;
        // }
        default:
            break;
    }
}
}
