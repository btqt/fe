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
        , mDiagnosticsAcquisitionTime{0U}
        , mWarningTriggerOccurrenceTime{0U}
        , mColId{0U}
        , mPriority{0U}
{
    LOG_D("RemoteLastUpload constructor");
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

//RDG30-R-1162
void RemoteLastUpload::makeUploadData() {
    LOG_I("Make Last Upload data");
    mLastUploadDataReq = std::shared_ptr<UploadLastDataRequest>(
        new UploadLastDataRequest());
    /*RdgCommonRequestHeader*/
    // text_version
    // electronic_pf
    // geodesy_information
    // time_zone_offset
        /*RdgCommonRequestHeader*/
    // text_version
    mLastUploadDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    // electronic_pf
    mLastUploadDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    // geodesy_information
    mLastUploadDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(AppCommonHeaderVehicleToCenterGeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
    const int32_t timezoneOffSet_hour {TimeManager::getInstance().getOffset() / 60};
    const int32_t timezoneOffSet_minute {TimeManager::getInstance().getOffset() % 60};
    LOG_D("Get timezone offset: %d:%d ", timezoneOffSet_hour, timezoneOffSet_minute);
    // time_zone_offset
    mLastUploadDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(timezoneOffSet_hour);
    mLastUploadDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(timezoneOffSet_minute);
    /*interface_type*/
    mLastUploadDataReq->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_LAST_DATA);
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    mLastUploadDataReq->set_counter_value(counterValue);
    /*message_id*/
    mLastUploadDataReq->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_LAST_DATA, counterValue));
    (void)counterValue;
    /*collection_condition_id*/
    mLastUploadDataReq->set_collection_condition_id(mColId);
    /*TriggerType*/
    if (mTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
        mLastUploadDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_WARNING_TRIGGER);
    } else if (mTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
        mLastUploadDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_IG_ON_TRIGGER);
    } else {
        mLastUploadDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_UNKNOWN);
    }
    /*counter_value*/

    /*data_creation_date*/
    /* Get current time */
    //TimeManager &mTimeManagerService{TimeManager::getInstance()};
    int64_t current_time {0};
    current_time = ParamsDef::getCurrentAcquisiteTime();
    if (current_time >= 0)
    {
        mLastUploadDataReq->set_data_creation_date(static_cast<uint64_t>(current_time));
    }
    else {
        //Do nothing
    }
    /*diagnostics_acquisition_time || warning_trigger_occurrence_time*/
    if (mTriggerType != DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        mLastUploadDataReq->set_diagnostics_acquisition_time(mDiagnosticsAcquisitionTime);
    } else {
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
        LOG_I("Last upload data size = %d", sizeOfFileCounter);
        LOG_I("Last upload data less than 4MB");
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
    std::string file_dir{std::to_string(uploadId)};
    (void)file_dir.append("_UploadLastDataRequest.dat");
    (void)DataModel<UploadLastDataRequest>::save(file_dir, *mLastUploadDataReq);
    const android::sp<UploadTask> task{new UploadTask(uploadId)};
    task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG120);
    task->setUploadPatch(file_dir);
    /*Set priority*/
    task->setUploadPrio(mPriority);
    // test_saveUploadData = task;
    LOG_I("requestUploadData");
    const uint64_t fileSize{static_cast<uint64_t>(mLastUploadDataReq->ByteSizeLong())};
    task->setFileSize(fileSize);
    UploadManager::getInstance()->requestUploadTask(task);
    LOG_I("request done");

}
//RDG30-R-1161
void RemoteLastUpload::triggerLastUpload(const DiagTrigger::DiagTriggerType triggerType, const int64_t time, const android::sp<CommonDefine::RDGLocationData> location) {
    LOG_I("Triggered Last upload");
    mTriggerType = triggerType;
    uint8_t time_ptr[sizeof(time)] {0U};
    (void)memcpy(&time_ptr[0], &time, sizeof(time_ptr));
    const android::sp<::Buffer> time_sp {new ::Buffer()};
    time_sp->setTo(&time_ptr[0], sizeof(time));
    if (time >= 0)
    {
        mDiagnosticsAcquisitionTime = static_cast<uint64_t>(time);
    }
    else {
        //Do nothing
    }
    if (triggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        LOG_I("CMD_TRIGGER_FROM_WARNING");
        if (time >= 0)
        {
            mWarningTriggerOccurrenceTime = static_cast<uint64_t>(time);
        } else{
            //Do nothing
        }
        const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_WARNING, location)};
        const uint32_t timeDataSize{time_sp->size()};
        if (timeDataSize <= static_cast<uint32_t>(INT32_MAX))
        {
            msg->buffer.setTo(time_sp->data(), static_cast<int32_t>(timeDataSize));
            (void)msg->sendToTarget();
        }
    } else if (triggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
        LOG_I("CMD_TRIGGER_FROM_IGON");
        const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_IGON, location)};
        const uint32_t timeDataSize{time_sp->size()};
        if (timeDataSize <= static_cast<uint32_t>(INT32_MAX))
        {
            msg->buffer.setTo(time_sp->data(), static_cast<int32_t>(timeDataSize));
            (void)msg->sendToTarget();
        }
    } else {
        LOG_I("CMD_TRIGGER_FROM_CENTER");
        const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_CENTER, location)};
        const uint32_t timeDataSize{time_sp->size()};
        if (timeDataSize <= static_cast<uint32_t>(INT32_MAX))
        {
            msg->buffer.setTo(time_sp->data(), static_cast<int32_t>(timeDataSize));
            (void)msg->sendToTarget();
        }
    }

}

void RemoteLastUpload::triggerLastUpload(const DiagTrigger::DiagTriggerType triggerType
, const int64_t timeData
, const android::sp<CommonDefine::RDGLocationData> location
, const uint64_t collectionId
, const uint32_t priority) {
    mColId = collectionId;
    mPriority = priority;
    mLocationData = location;
    triggerLastUpload(triggerType, timeData, location);
}

void RemoteLastUpload::trigger_LU(const DiagTrigger::DiagTriggerType type, const uint32_t prio, const uint64_t colId, const int64_t time) {
    LOG_I("Last Upload trigger");
    /* TBD: check ppi flag*/
    /* Get trigger ID*/
    LOG_I("Trigger type: %d", type);
    const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
    /* Create NewDiag Trigger*/
    /* DiagTrigger(const DiagTrigger::DiagTriggerType type, const uint32_t priority, const DiagTrigger::DiagTriggerFunc func, const int32_t triggerId)*/
    android::sp<DiagTrigger> pTrigger{new DiagTrigger(type, prio, DiagTrigger::DiagTriggerFunc::ALLDIAG, nextTriggerId)};
    /* set trigger time*/
    pTrigger->setTriggerTime(time);
    LOG_I("Check Last Upload trigger time: %lld sec", time);
    /* Set collection id*/
    pTrigger->setCollectionId(colId);
    LOG_I("Check Last Upload Collection ID: %lld ", colId);
    /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
    (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_DATA, pTrigger)->sendToTarget();
    /* Save request to local*/
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
    LOG_I("Check mSaveReq size: %d", mSaveReq.size());
    if(!ret.second) {
        ret.first->second = pTrigger;
    }
    LOG_I("Check saved Last Upload trigger ID: %d", ret.first->first);
}

void RemoteLastUpload::onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) {
    LOG_I("Last Upload receive Center request");
    /*TBD: Process center data*/
    const uint32_t prio_data{pCenterReqData->getCenterReq_prio()};
    /*TBD: get warning trigger occurence*/
    const uint64_t colID{pCenterReqData->getCenterReq_CollectionID()};
    uint8_t colId_ptr[sizeof(colID)] {0U};
    (void)memcpy(&colId_ptr[0], &colID, sizeof(colId_ptr));
    const android::sp<::Buffer> colId_sp {new ::Buffer()};
    colId_sp->setTo(&colId_ptr[0], sizeof(colID));
    if((prio_data <= static_cast<uint32_t>(INT32_MAX)))
    {
        const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_CENTER, static_cast<int32_t>(prio_data))};
        const uint32_t collSize{colId_sp->size()};
        if((collSize <= static_cast<uint32_t>(INT32_MAX)))
        {
            msg->buffer.setTo(colId_sp->data(), static_cast<int32_t>(collSize));
            (void)msg->sendToTarget();
        }
    }

}

void RemoteLastUpload::MainHandler::handleMessage (const android::sp<sl::Message>& handlemsg) 
{
    const int32_t what {handlemsg->what};
    LOG_I({"handler is processing with what: %d"}, what);
    switch (what) {
        case CMD_TRIGGER_FROM_IGON:
        {
            LOG_I("CMD_TRIGGER_FROM_IGON");
            /*TBD: check data upload consent state && IG is ON*/
                /* Get current time */
            //TimeManager &mTimeManagerService {TimeManager::getInstance()};
            int64_t current_time {0};
            current_time = ParamsDef::getCurrentAcquisiteTime();
            mLU.trigger_LU(DiagTrigger::DiagTriggerType::IGON_TRIGGER, DiagTrigger::PRIO_IG_ON_TRIGGER, mLU.mColId, current_time);
            break;
        }
        case CMD_TRIGGER_FROM_WARNING:
        {
            LOG_I("CMD_TRIGGER_FROM_WARNING");
            /*TBD: check RDG flag && DTC FLAG && data upload consent state && IG is ON*/
            /* Get time from buffer*/
            const android::sp<::Buffer> buf {new ::Buffer(handlemsg->buffer)};
            int64_t timeData{0};
            if (buf->data() != nullptr)
            {
                (void)std::memcpy(&timeData, buf->data(), sizeof(int64_t));
            }else {
                //Do Nothing
            }
            LOG_I("Check timeData: %lld", timeData);
            /*Get location data*/
            android::sp<CommonDefine::RDGLocationData> loc{nullptr};
            handlemsg->getObject(loc);
            LOG_I("Check Location data 0x%08X 0x%08X", loc->getLatitude(), loc->getLongtitude());
            mLU.trigger_LU(DiagTrigger::DiagTriggerType::WARNING_TRIGGER, DiagTrigger::PRIO_WARNING_TRIGGER, mLU.mColId, timeData);
            break;
        }
        case CMD_TRIGGER_FROM_CENTER:
        {
            LOG_I("CMD_TRIGGER_FROM_CENTER");
            /* Get current time */
            //TimeManager &mTimeManagerService {TimeManager::getInstance()};
            int64_t current_time {0};
            current_time = ParamsDef::getCurrentAcquisiteTime();
            /* Get priority from Center Request */
            const int32_t argData{handlemsg->arg1};
            uint32_t prio_tmp{0U};
            if (argData >= 0)
            {
                prio_tmp = static_cast<uint32_t>(argData);
            }
            /* Get collection condition ID*/
            const android::sp<::Buffer> buf {new ::Buffer(handlemsg->buffer)};
            uint64_t colId{0U};
            if(buf->data() != nullptr)
            {
                (void)std::memcpy(&colId, buf->data(), sizeof(uint64_t));
            } else {
                //Do Nothing
            }
            LOG_I("Check Col ID: %lld", colId);
            mLU.trigger_LU(DiagTrigger::DiagTriggerType::CENTER_TRIGGER, prio_tmp, colId, current_time);
            break;
        }
        case CMD_MAKE_DATA:
        {
            LOG_I("CMD_MAKE_LUD");
            mLU.makeUploadData();
            break;
        }
        default:
            break;
    }
}
}
