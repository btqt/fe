#include "RemoteRoBSSR.h"

namespace rdgapp {

RemoteRoBSSR *RemoteRoBSSR::mRoBSSR_instance{nullptr};
RemoteRoBSSR::RemoteRoBSSR(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper)
    : android::RefBase(), RemoteDelegate()
    , mApp(app)
    , mHandler(new MainHandler(privateLooper, *this))
    , mIsRobSsrRunning(false)
    , mIsNeedUploadErrorData(false)
    , mIsSuspending(false)
    , mTriggerId(0U)
    , mCurrentDiagTriggerType(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)
    , mCurrentTransmissionId(0U)
{
    mRoBSSR_instance = this;
    odo_value = 0U;
    odo_unit = 0U;
    mColId = 0U;
    mPriority = 0U;
    mDiagnosticsAcquisitionTime = 0;
    mTotalSize = 0U;
    ROBSSR_TOTAL_UPLOAD_DATA_SIZE_MAX = 10485760U; // bytes = 10MB
    ROBSSR_UPLOAD_DATA_SIZE_MAX = 4194304U;        // bytes = 4MB
    ROBSSR_LAST_UPLOAD_DATA_SIZE_MAX = 2097152U;   // bytes = 2MB
    mRobSsrUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
    mRobSsrDataReq = std::shared_ptr<UploadRobSsrDataRequest>(new UploadRobSsrDataRequest());
    (void)mHandler->obtainMessage(MainHandler::CMD_INIT_ROBSSR)->sendToTarget();
}

RemoteRoBSSR::~RemoteRoBSSR() = default;

RemoteRoBSSR *RemoteRoBSSR::getInstance()
{
    if (mRoBSSR_instance == nullptr)
    {
        LOG_E("mRoBSSR_instance is nullptr");
    }
    return mRoBSSR_instance;
}

void RemoteRoBSSR::printData(const std::string data) const
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

void RemoteRoBSSR::init(void)
{
    LOG_I("Init success");
    mCRCManager = android::sp<CRCManager>(new CRCManager(*this));
    (void)mHandler->obtainMessage(MainHandler::CMD_READ_SAVED_CRC)->sendToTarget();
}

void RemoteRoBSSR::onReceiveIG(const bool status) const 
{
    (void)mHandler->obtainMessage(MainHandler::CMD_CHANGE_IG_STATUS, static_cast<int32_t>(status))->sendToTarget();
}

void RemoteRoBSSR::changeIGStatus(const bool status) 
{
    LOG_I("onReceiveIG status = %s", status ? "IGN_ON" : "IGN_OFF");
    LOG_I("mCurrentDiagTriggerType = %d", mCurrentDiagTriggerType);
    if ((status == false) && (mIsRobSsrRunning == true))
    {
        const std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>::iterator itTrans{mRobSsrTransList.find(mCurrentTransmissionId)};
        if (itTrans != mRobSsrTransList.end())
        {
            if (mRobSsrTransList[mCurrentTransmissionId]->getState() == RobSsrUdsTransmission::State::ROBSSR_TRANS_REFERENCEDATA_REQUEST)
            {
                LOG_E("IG change to OFF, store undefined value to reference data");
                mResSID22[mRobSsrTransList[mCurrentTransmissionId]->getEcuInfo().getTargetAddress()].setEcuMakerCode(0xFFFFU);
                mResSID22[mRobSsrTransList[mCurrentTransmissionId]->getEcuInfo().getTargetAddress()].setSwPartNumber("");
                (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
            }
            else
            {
                // Abort acquisition
                mIsRobSsrRunning = false;

                while (mTransmissioIdList.empty() != true) {
                    mTransmissioIdList.pop();
                }
                itTrans->second->disconnect();
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
                mRobSsrTransList.clear();
                mRobSsrUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
                if (mRobSsrUploadErrorData != nullptr)
                {
                    mRobSsrUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
                    LOG_I("The RoBSR acquisition sequence is aborted -> push error upload data package");
                    mIsNeedUploadErrorData = true;
                    makeErrorUploadRequest(*mRobSsrUploadErrorData, mCurrentDiagTriggerType, mColId);
                }
            }
        }
    }
}

void RemoteRoBSSR::MainHandler::handleMessage(const android::sp<sl::Message>& handlemsg)
{
    const int32_t what{handlemsg->what};
    LOG_I({"handler is processing with what: %d"}, what);
    switch (what) {
        case CMD_INIT_ROBSSR:
        {
            LOG_I("CMD_INIT_ROBSSR");
            mRoBSSR.init();
            break;
        }
        case CMD_START_ROBSSR:
        {
            LOG_I("CMD_START_ROBSSR");
            android::sp<CommonDefine::RDGLocationData> location {nullptr};
            handlemsg->getObject(location);
            if (location == nullptr)
            {
                LOG_E("location data is null");
                break;
            }
            uint64_t timestamp{0U};
            if (handlemsg->buffer.size() > 0U ) {
                if(handlemsg->buffer.data() != nullptr) {
                    (void)memcpy(&timestamp, handlemsg->buffer.data(), sizeof(timestamp));
                } else {
                    LOG_D("handlemsg->buffer.data() is null");
                }
            } else {
                LOG_E("timestamp data is empty");
            }
            DiagTrigger::DiagTriggerType trigType{DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN};
            if ((handlemsg->arg1 > static_cast<int32_t>(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)) && (handlemsg->arg1 < static_cast<int32_t>(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)))
            {
                trigType = static_cast<DiagTrigger::DiagTriggerType>(handlemsg->arg1);
            }
            mRoBSSR.startUp(trigType, timestamp, location);
            break;
        }
        case CMD_TRIGGER_FROM_CENTER:
        {
            LOG_I("CMD_TRIGGER_FROM_CENTER");
             /* Get current time */
            //TimeManager &mTimeManagerService = TimeManager::getInstance();
            int64_t current_time {0};
            current_time = ParamsDef::getCurrentAcquisiteTime();
            /* Get priority from Center Request */
            const int32_t msg_arg{handlemsg->arg1};
            const uint32_t prio_tmp{(msg_arg >= 0) ? static_cast<uint32_t>(msg_arg) : 0U};
            /* Get collection condition ID*/
            const android::sp<::Buffer> buf {new ::Buffer(handlemsg->buffer)};
            uint64_t colId{0U};
            if(buf->data() != nullptr) {
                (void)memcpy(&colId, buf->data(), sizeof(colId));
            } else {
                LOG_E("buf->data() is null");
            }
            LOG_I("Check Col ID: %llu", colId);
            mRoBSSR.triggerRoBSSR(DiagTrigger::DiagTriggerType::CENTER_TRIGGER, prio_tmp, colId, current_time);
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
        case CMD_READ_SAVED_CRC:
        {
            mRoBSSR.mCRCManager->readCRC32FromFile();
            break;
        }
        case CMD_MAKE_UPLOAD_REQUEST:
        {
            LOG_I("CMD_MAKE_UPLOAD_REQUEST");
            mRoBSSR.makeUploadRequest();
            break;
        }
        case CMD_CHANGE_IG_STATUS:
        {
            LOG_I("CMD_CHANGE_IG_STATUS");
            mRoBSSR.changeIGStatus((handlemsg->arg1 != 0) ? true : false);
            break;
        } 
        case CMD_TRIGGER_FROM_ROBOCCURRENCE:
        {
            LOG_I("CMD_TRIGGER_FROM_ROBOCCURRENCE");
            android::sp<OccurrentRobNotification> notification{nullptr};
            mRoBSSR.mCurrentDiagTriggerType = DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER;
            handlemsg->getObject(notification);
            mRoBSSR.startUp_RoBOccurrence(notification);
            break;
        }
        case CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE:
        {
            mRoBSSR.handleUnderRepairStatusChange(handlemsg->arg1, handlemsg->arg2);
            break;
        }
        case CMD_ROBSSR_FINISH_TRANSMISSION:
        {
            LOG_I("CMD_ROBSSR_FINISH_TRANSMISSION");
            mRoBSSR.finishCurrentTransmission();
            break;
        }
        case CMD_TRANSMISSION_TIMEOUT:
        {
            mRoBSSR.handleTransmissionTimeout();
            break;
        }
        default:
            break;
    }
}

void RemoteRoBSSR::triggerRoBSSR(const DiagTrigger::DiagTriggerType type, const uint32_t prio, const uint64_t colId, const int64_t time)
{
    const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
    android::sp<DiagTrigger> pTrigger{new DiagTrigger(type, prio, DiagTrigger::DiagTriggerFunc::ROBSSR, nextTriggerId)};
    /* set trigger time*/
    pTrigger->setTriggerTime(time);
    LOG_D("Check trigger time: %lld sec", time);
    /* Set collection id*/
    pTrigger->setCollectionId(colId);
    LOG_D("Check Collection ID: %llu ", colId);

    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
    /* Save request to local*/
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
    LOG_D("Check mSaveReq size: %d", mSaveReq.size());
    if(!ret.second) {
        ret.first->second = pTrigger;
    }
}

void RemoteRoBSSR::startUp(const DiagTrigger::DiagTriggerType type, const uint64_t timestamp, const android::sp<CommonDefine::RDGLocationData> location)
{
    //TODO: check diag type: occurrence (repeated rob) / center request (signal rob)

    LOG_I("startup");
    mTotalSize = 0U;
    // check precondtion
    if (checkPrecondition() == true)
    {
        // acquisition time
        // TimeManager &mTimeManagerService{TimeManager::getInstance()};
        const int64_t current_time{ParamsDef::getCurrentAcquisiteTime()};
        mDiagnosticsAcquisitionTime = current_time;
        LOG_I("Get acquisition time: %lld", mDiagnosticsAcquisitionTime);
        // Get Location data
        mLocationData = LocationManagerAdapter::getInstance()->getLocationData();
        LOG_I("Get location data success, 0x%08X 0x%08X", mLocationData->getLatitude(), mLocationData->getLongtitude());
        // Get ODO data
        (void)VehicleManagerAdapter::getInstance()->getOdoInformation(odo_value, odo_unit); // odo value
        this->makeHeaderForUploading(mRobSsrDataReq);

        const uint32_t uploadDataSize{mRobSsrDataReq->ByteSizeLong()};
        if ((uploadDataSize < UINT32_MAX / 3U) && (mTotalSize <= UINT32_MAX - (3U * uploadDataSize)))
        {
            mTotalSize = mTotalSize + (3U * uploadDataSize);
        }
        else
        {
            LOG_E("total size overflow");
        }
        // Get OBC resource
        const OBCResourceEventCode resEventInfo{OnboardclientAdapter::getInstance()->GetObcResource()};
        if ( resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK )
        {
            LOG_E("GetObcResource OBC_GET_RESOURCE_WAIT -> wait and check after");

            const android::sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_START_ROBSSR, location)};
            msg->arg1 = static_cast<int32_t>(type);
            uint8_t timestamp_data[8];
            (void)memcpy(&timestamp_data[0], &timestamp, sizeof(timestamp_data));
            msg->buffer.setTo(&timestamp_data[0], sizeof(timestamp_data));
            (void)mHandler->sendMessageDelayed(msg, 5000U);

        } else {
            LOG_D("Get obc resource success");
            // Lock OBC resource
            OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
            this->mIsRobSsrRunning = true;
            mRobSsrTransList.clear();
            mTransmissioIdList = {};

            //Center request
            CenterRequestRobSsrList robSsrList{CollectionCondition::getInstance().getCenterRequestRobSsr()};
            LOG_I("robSsrList size: %ld, collection condition ID: %llu", robSsrList.size(), mColId);
            CenterRequestRobSsr robSsrReq{};
            for (CenterRequestRobSsrList::iterator it{robSsrList.begin()}; it != robSsrList.end(); ++it)
            {
                if ((*it).collection_condition_id() == mColId)
                {
                    robSsrReq = *it;
                    break;
                }
            }
            //check ECU is active in ECU information list
            const uint32_t ecuAddress{robSsrReq.mutable_target_collection_data()->mutable_ecu_address_information()->target_address()};
            std::list<CommonDefine::EcuInformation> ecuInfoList{};
            RemoteEcuInformation::getInstance()->getEcuInformationList(ecuInfoList);
            std::list<CommonDefine::EcuInformation>::iterator ecu_it{};
            for (ecu_it = ecuInfoList.begin(); ecu_it != ecuInfoList.end(); ++ecu_it)
            {
                if ((ecuAddress == ecu_it->getTargetAddress()) && (ecu_it->getecuActiveFlag() == true))
                {
                    break;
                }
            }
            (void)ecuAddress;

            if (ecu_it == ecuInfoList.end())
            {
                //RDG30-R-1148
                LOG_E("ECU is not active in the list");
                vccomif::rdg::v1::interfaces::DiagnosticsMessage *const errDiagMessage{mRobSsrUploadErrorData->add_diag_messages()};
                errDiagMessage->set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_ECU_SPECIFIED_ERROR);
                makeErrorUploadRequest(*mRobSsrUploadErrorData, mCurrentDiagTriggerType, mColId);
            } else
            {
                //get rob code
                const uint32_t robCode{robSsrReq.mutable_target_collection_data()->mutable_rob_information()->rob()};

                //check specified RobFrameNo from trigger
                bool hasRobFrameNo{robSsrReq.mutable_target_collection_data()->mutable_rob_information()->has_rob_frame_information()};
                const int32_t robFrameSize{robSsrReq.mutable_target_collection_data()->mutable_rob_information()->mutable_rob_frame_information()->rob_frame_numbers_size()};
                if ((hasRobFrameNo == true)
                    && (robFrameSize == 1)
                    && (robSsrReq.mutable_target_collection_data()->mutable_rob_information()->mutable_rob_frame_information()->rob_frame_numbers(0) == 0U))
                {
                    hasRobFrameNo = false; //RDG30-R-1263
                }
                LOG_I("rob code: %d, hasRobFrameNo: %d", robCode, hasRobFrameNo);
                std::vector<uint32_t> robFrameNoList{};
                for (int32_t i{0}; i<robFrameSize; i++)
                {
                    const uint32_t temp{robSsrReq.mutable_target_collection_data()->mutable_rob_information()->mutable_rob_frame_information()->rob_frame_numbers(i)};
                    robFrameNoList.push_back(temp);
                }

                //check CRC of last acquisition
                uint32_t occurredCRC{0U};
                uint32_t timeSeriesCRC{0U};
                const bool hasCrcInfo{robSsrReq.mutable_target_collection_data()->mutable_rob_information()->has_crc_information()};
                if (hasCrcInfo == true)
                {
                    occurredCRC = robSsrReq.mutable_target_collection_data()->mutable_rob_information()->mutable_crc_information()->occurred_rob_ssr_crc();
                    timeSeriesCRC = robSsrReq.mutable_target_collection_data()->mutable_rob_information()->mutable_crc_information()->time_series_rob_ssr_crc();
                }

                // bool hasSubFn{it->mutable_target_collection_data()->mutable_rob_information()->has_sub_function()};
                constexpr bool hasSubFn{true};
                uint32_t subFunction{0U};
                if (hasSubFn == true)
                {
                    subFunction = robSsrReq.mutable_target_collection_data()->mutable_rob_information()->sub_function();
                }

                // const bool hasMemorySelection{it->mutable_target_collection_data()->mutable_rob_information()->has_memory_selection()};
                constexpr bool hasMemorySelection{true};
                uint32_t memorySelection{0U};
                if (hasMemorySelection)
                {
                    memorySelection = robSsrReq.mutable_target_collection_data()->mutable_rob_information()->memory_selection();
                }
                
                LOG_I("hasCrcInfo = %d, hasSubFn = %d, robCode= %d, memorySelection = 0x%x, subFunction = 0x%x", hasCrcInfo, hasSubFn, robCode, memorySelection, subFunction);

                if ( ecu_it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6 )
                {
                    // RDG30-R-0922
                    if (/*((hasCrcInfo == false) && (hasMemorySelection == false))
                        ||*/ (robCode > 0xFFFFFFU) 
                        || ((memorySelection != 0x11U) && (memorySelection != 0x12U) && (memorySelection != 0x13U) && (memorySelection != 0x14U))
                        || ((hasCrcInfo == true) && (hasRobFrameNo == true))
                        || ((hasRobFrameNo == true) && (robSsrReq.mutable_target_collection_data()->mutable_rob_information()->mutable_rob_frame_information()->rob_frame_numbers_size() > 255)))
                    {
                        mRobSsrUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_UNEXPECTED_COMMAND);
                        LOG_I("ECU Phase 6, The RoBSSR acquisition sequence is aborted -> push error upload data package");
                        mIsNeedUploadErrorData = true;
                        // (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                        makeErrorUploadRequest(*mRobSsrUploadErrorData, mCurrentDiagTriggerType, mColId);
                    }
                    else
                    {
                        const android::sp<RobSsrUdsTransmission> transmission {new RobSsrUdsTransmission( *this
                                                                                                    , *ecu_it
                                                                                                    , robCode
                                                                                                    , hasSubFn
                                                                                                    , subFunction
                                                                                                    , hasMemorySelection
                                                                                                    , memorySelection
                                                                                                    , hasRobFrameNo
                                                                                                    , robFrameNoList
                                                                                                    , hasCrcInfo
                                                                                                    , occurredCRC
                                                                                                    , timeSeriesCRC)};
                        transmission->transInit();
                        mRobSsrTransList[transmission->getTransmissionId()] = transmission;
                        mTransmissioIdList.push(transmission->getTransmissionId());
                    }
                } else if ( ecu_it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5 )
                {
                    // RDG30-R-0739
                    if (/*((hasSubFn == false) && (hasRobFrameNo == false))
                        ||*/ (robCode > 0xFFFFU)
                        || (hasSubFn == false)
                        || ((hasSubFn == true) && (subFunction != 0x02U) && (subFunction != 0x12U) && (subFunction != 0x22U) && (subFunction != 0x32U))
                        || ((hasRobFrameNo == true) && (robFrameSize > 255)))
                    {
                        LOG_I("ECU Phase 5, The RoBSSR acquisition sequence is aborted -> push error upload data package");
                        mRobSsrUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_UNEXPECTED_COMMAND);
                        mIsNeedUploadErrorData = true;
                        // (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                        makeErrorUploadRequest(*mRobSsrUploadErrorData, mCurrentDiagTriggerType, mColId);
                    }
                    else
                    {
                        const android::sp<RobSsrUdsTransmission> transmission {new RobSsrUdsTransmission( *this
                                                                                                    , *ecu_it
                                                                                                    , robCode
                                                                                                    , hasSubFn
                                                                                                    , subFunction
                                                                                                    , hasMemorySelection
                                                                                                    , memorySelection
                                                                                                    , hasRobFrameNo
                                                                                                    , robFrameNoList
                                                                                                    , hasCrcInfo
                                                                                                    , occurredCRC
                                                                                                    , timeSeriesCRC)};                            
                        transmission->transInit();
                        mRobSsrTransList[transmission->getTransmissionId()] = transmission;
                        mTransmissioIdList.push(transmission->getTransmissionId());
                    }
                } else {
                    LOG_E("ECU diag phase is invalid");
                }
                (void)subFunction;
                (void)timeSeriesCRC;
                (void)occurredCRC;
            }
            LOG_D("mTransmissioIdList size = %d", mTransmissioIdList.size());
            if ( mTransmissioIdList.size() > 0U )
            {
                mCurrentTransmissionId = mTransmissioIdList.front();
                const std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>::iterator itTrans{mRobSsrTransList.find(mCurrentTransmissionId)};
                if (itTrans != mRobSsrTransList.end())
                {
                    itTrans->second->connect();
                }
            } else {
                // Release OBC resource
                DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
                onFinishAcquisition(mTriggerId);
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
            }
        }
    } else
    {
        LOG_I("Condition not match");
        if (mTriggerId < static_cast<uint32_t>(INT32_MAX))
        {
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mCurrentDiagTriggerType);
        }
        else
        {
            LOG_D("triggerId value error: %d in mTriggerList", mTriggerId);
        }
    }
}

void RemoteRoBSSR::startUp_RoBOccurrence(const android::sp<OccurrentRobNotification> notification)
{
    LOG_I("startUp_RoBOccurrence");
    mTotalSize = 0U;
    // check precondtion
    if (checkPrecondition() == true)
    {
       mDiagnosticsAcquisitionTime =  notification->triggerTime();
       LOG_I("Get acquisition time: %d", mDiagnosticsAcquisitionTime);
       mLocationData = notification->triggerLocation();
       LOG_I("Get location data success, 0x%08x 0x%08x ", mLocationData->getLatitude(), mLocationData->getLongtitude());
        // Get ODO data
        (void)VehicleManagerAdapter::getInstance()->getOdoInformation(odo_value, odo_unit); // odo value
        this->makeHeaderForUploading(mRobSsrDataReq);

        const uint32_t uploadDataSize{mRobSsrDataReq->ByteSizeLong()};
        if ((uploadDataSize < UINT32_MAX / 3U) && (mTotalSize <= UINT32_MAX - (3U * uploadDataSize)))
        {
            mTotalSize = mTotalSize + (3U * uploadDataSize);
        }
        else
        {
            LOG_E("total size overflow");
        }
        // Get OBC resource
        const OBCResourceEventCode resEventInfo{OnboardclientAdapter::getInstance()->GetObcResource()};
        if ( resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK )
        {
            LOG_E("GetObcResource OBC_GET_RESOURCE_WAIT -> wait and check after");

            const android::sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_ROBOCCURRENCE, notification)};
            (void)mHandler->sendMessageDelayed(msg, 5000U);

        }
        else
        {
            LOG_D("Get obc resource success");
            // Lock OBC resource
            OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
            this->mIsRobSsrRunning = true;
            mRobSsrTransList.clear();
            mTransmissioIdList = {};
            uint32_t ecuAddress {notification->getTargetCollectionDataRobSsr()->ecu_address_information().target_address()};
            const CenterCommunicationType ecuCommType {notification->getTargetCollectionDataRobSsr()->ecu_address_information().communication_type()};
            if (ecuCommType == CenterCommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS)
            {
                ecuAddress = ((ecuAddress << 8U) | 0xFFU);
            }
            std::list<CommonDefine::EcuInformation> ecuInfoList{};
            RemoteEcuInformation::getInstance()->getEcuInformationList(ecuInfoList);
            std::list<CommonDefine::EcuInformation>::iterator ecu_it{};
            for (ecu_it = ecuInfoList.begin(); ecu_it != ecuInfoList.end(); ++ecu_it)
            {
                if ((ecuAddress == ecu_it->getTargetAddress()) && (ecu_it->getecuActiveFlag() == true))
                {
                    break;
                }
            }
            (void)ecuAddress;

            if (ecu_it == ecuInfoList.end())
            {
                LOG_E("ECU is not active in the list");
            } else
            {
                //get rob code
                const uint32_t robCode{notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->rob()};
                //check specified RobFrameNo from trigger
                bool hasRobFrameNo{notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->has_rob_frame_information()};
                const int32_t robFrameSize{notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->mutable_rob_frame_information()->rob_frame_numbers_size()};
                if ((hasRobFrameNo == true)
                    && (robFrameSize == 1)
                    && (notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->mutable_rob_frame_information()->rob_frame_numbers(0) == 0U))
                {
                    hasRobFrameNo = false; //RDG30-R-1263
                }
                LOG_I("rob code: %d, hasRobFrameNo: %d", robCode, hasRobFrameNo);
                std::vector<uint32_t> robFrameNoList{};
                for (int32_t i{0}; i<robFrameSize; i++)
                {
                    const uint32_t temp{notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->mutable_rob_frame_information()->rob_frame_numbers(i)};
                    robFrameNoList.push_back(temp);
                }
                //check CRC of last acquisition
                uint32_t occurredCRC{0U};
                uint32_t timeSeriesCRC{0U};
                const bool hasCrcInfo{notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->has_crc_information()};
                if (hasCrcInfo == true)
                {
                    occurredCRC = notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->mutable_crc_information()->occurred_rob_ssr_crc();
                    timeSeriesCRC = notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->mutable_crc_information()->time_series_rob_ssr_crc();
                }

                // bool hasSubFn{notification->mutable_rob_information()->has_sub_function()};
                constexpr bool hasSubFn{true};
                uint32_t subFunction{0U};
                if (hasSubFn == true)
                {
                    subFunction = notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->sub_function();
                }

                // const bool hasMemorySelection{notification->mutable_rob_information()->has_memory_selection()};
                constexpr bool hasMemorySelection{true};
                uint32_t memorySelection{0U};
                if (hasMemorySelection)
                {
                    memorySelection = notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->memory_selection();
                }
                
                LOG_I("hasCrcInfo = %d, hasSubFn = %d, robCode= %d, memorySelection = %d, subFunction = %d", hasCrcInfo, hasSubFn, robCode, memorySelection, subFunction);
                if ( ecu_it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6 )
                {
                    // RDG30-R-0922
                    if (/*((hasCrcInfo == false) && (hasMemorySelection == false))
                        ||*/ (robCode > 0xFFFFFFU) 
                        || ((memorySelection != 0x11U) && (memorySelection != 0x12U) && (memorySelection != 0x13U) && (memorySelection != 0x14U))
                        || ((hasCrcInfo == true) && (hasRobFrameNo == true))
                        || ((hasRobFrameNo == true) && (notification->getTargetCollectionDataRobSsr()->mutable_rob_information()->mutable_rob_frame_information()->rob_frame_numbers_size() > 255)))
                    {
                        mRobSsrUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
                        LOG_I("ECU Phase 6, The RoBSSR acquisition sequence is aborted -> push error upload data package");
                        mIsNeedUploadErrorData = true;
                        makeErrorUploadRequest(*mRobSsrUploadErrorData, mCurrentDiagTriggerType, mColId);
                    }
                    else
                    {
                        const android::sp<RobSsrUdsTransmission> transmission {new RobSsrUdsTransmission( *this
                                                                                                    , *ecu_it
                                                                                                    , robCode
                                                                                                    , hasSubFn
                                                                                                    , subFunction
                                                                                                    , hasMemorySelection
                                                                                                    , memorySelection
                                                                                                    , hasRobFrameNo
                                                                                                    , robFrameNoList
                                                                                                    , hasCrcInfo
                                                                                                    , occurredCRC
                                                                                                    , timeSeriesCRC)};
                        transmission->transInit();
                        mRobSsrTransList[transmission->getTransmissionId()] = transmission;
                        mTransmissioIdList.push(transmission->getTransmissionId());
                    }
                } else if ( ecu_it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5 )
                {
                    // RDG30-R-0739
                    if (/*((hasSubFn == false) && (hasRobFrameNo == false))
                        ||*/ (robCode > 0xFFFFU)
                        || (hasSubFn == false)
                        || ((hasSubFn == true) && (subFunction != 0x02U) && (subFunction != 0x12U) && (subFunction != 0x22U) && (subFunction != 0x32U))
                        || ((hasRobFrameNo == true) && (robFrameSize > 255)))
                    {
                        LOG_I("ECU Phase 5, The RoBSSR acquisition sequence is aborted -> push error upload data package");
                        mRobSsrUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
                        mIsNeedUploadErrorData = true;
                        // (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                        makeErrorUploadRequest(*mRobSsrUploadErrorData, mCurrentDiagTriggerType, mColId);
                    }
                    else
                    {
                        const android::sp<RobSsrUdsTransmission> transmission {new RobSsrUdsTransmission( *this
                                                                                                    , *ecu_it
                                                                                                    , robCode
                                                                                                    , hasSubFn
                                                                                                    , subFunction
                                                                                                    , hasMemorySelection
                                                                                                    , memorySelection
                                                                                                    , hasRobFrameNo
                                                                                                    , robFrameNoList
                                                                                                    , hasCrcInfo
                                                                                                    , occurredCRC
                                                                                                    , timeSeriesCRC)};                            
                        transmission->transInit();
                        mRobSsrTransList[transmission->getTransmissionId()] = transmission;
                        mTransmissioIdList.push(transmission->getTransmissionId());
                    }
                } else {
                    LOG_E("ECU diag phase is invalid");
                }
                (void)timeSeriesCRC;
                (void)subFunction;
                (void)occurredCRC;
            }
            LOG_D("mTransmissioIdList size = %d", mTransmissioIdList.size());
            if ( mTransmissioIdList.size() > 0U )
            {
                mCurrentTransmissionId = mTransmissioIdList.front();
                const std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>::iterator itTrans{mRobSsrTransList.find(mCurrentTransmissionId)};
                if (itTrans != mRobSsrTransList.end())
                {
                    itTrans->second->connect();
                }
            } else {
                // Release OBC resource
                DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
                RoBOccurrence::getInstance()->onRobSsrAcquisitionCompleted(true, mRobSsrTransList[mCurrentTransmissionId]->getUpload_occurredCRC(), mRobSsrTransList[mCurrentTransmissionId]->getUpload_timeSeriesCRC());
                onFinishAcquisition(mTriggerId);
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
            }
        }
    }
    else
    {
        LOG_I("Condition not match");
        if (mTriggerId < static_cast<uint32_t>(INT32_MAX))
        {
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mCurrentDiagTriggerType);
        }
        else
        {
            LOG_D("triggerId value error: %d in mTriggerList", mTriggerId);
        }

    }
}

bool RemoteRoBSSR::checkPrecondition()
{
    //get RDG active flag
    const uint8_t RDG_flag{DiagManagerAdapter::getInstance()->getRDGFlag()};
    //get IG status
    const IG_STATUS IG_status{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
    //get data upload consent status
    const bool consent_status{DiagManagerAdapter::getInstance()->getAllUploadConsent()};
    //get Under Repair Status
    const uint8_t under_repair_status{mApp.getUnderRepair()};
    LOG_D("Precondition: RDG_flag = %d, IG_status = %d, consent_status = %d, under_repair_status = %d", RDG_flag, IG_status, consent_status, under_repair_status);
    (void) IG_status;
    (void) consent_status;
    (void) under_repair_status;
    return (RDG_flag == 0x01U) && (IG_status == IG_STATUS_ON) && (consent_status == true) && (under_repair_status != 0x01U);
}

void RemoteRoBSSR::onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) noexcept
{
    LOG_I("receive Center request");
    /*TBD: Process center data*/
    const uint32_t prio_data{pCenterReqData->getCenterReq_prio()};
    /*TBD: get warning trigger occurence*/
    const uint64_t colID{pCenterReqData->getCenterReq_CollectionID()};
    uint8_t colId_ptr[sizeof(colID)];
    (void)memcpy(&colId_ptr[0], &colID, sizeof(colID));
    const android::sp<::Buffer> colId_sp {new ::Buffer()};
    colId_sp->setTo(&colId_ptr[0], sizeof(colID));
    const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_CENTER, (prio_data <= static_cast<uint32_t>(INT32_MAX)) ? static_cast<int32_t>(prio_data) : 0)};
    const uint32_t colIdSize{colId_sp->size()};
    if (colIdSize <= static_cast<uint32_t>(INT32_MAX))
    {
        msg->buffer.setTo(colId_sp->data(), static_cast<int32_t>(colIdSize));
    }
    else
    {
        LOG_E("colIdSize > INT32_MAX");
    }
    (void)msg->sendToTarget();
}

void RemoteRoBSSR::onOccurrentRobDetected(const android::sp<OccurrentRobNotification> notification)
{
    LOG_D("onOccurrentRobDetected, collectionConditionId = %llu", notification->collectionConditionId());
    LOG_D("                        time = %lld", notification->triggerTime());
    LOG_D("                        latitude = 0x%08x", notification->triggerLocation()->getLatitude());
    LOG_D("                        longitude = 0x%08x", notification->triggerLocation()->getLongtitude());
    LOG_D("                        targetAddress = 0x%02x", notification->getTargetCollectionDataRobSsr()->ecu_address_information().target_address());
    this->mRobNotificationRequest = notification;
    const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_ROBOCCURRENCE, notification)};
    (void)msg->sendToTarget();
}

void RemoteRoBSSR::notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const uint32_t& pTriggerId, const bool dueToIgOff)
{
    /* Check if trigger is in the saved trigger*/
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(pTriggerId)};
    if(it != mSaveReq.end()) {
        LOG_I("notify Trigger state: %d TriggerID: %d TriggerTime: %lld", pState, pTriggerId, it->second->getTriggerTime());
        handleTrigger(pState, pTriggerId, dueToIgOff);
    } else {
        LOG_I("Can not find triggerID in saved list");
        if(pTriggerId > static_cast<uint32_t>(INT32_MAX)) {
            LOG_E("out of range INT32_MAX");
        }
        PriorityControl::getInstance()->notifyTriggerNoFound(static_cast<int32_t>(pTriggerId));
    }
}

void RemoteRoBSSR::handleTrigger(const DiagTrigger::DiagTriggerState& pState, const uint32_t& pTriggerId, const bool dueToIgOff) 
{
    (void)dueToIgOff;
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(pTriggerId)};
    if(it != mSaveReq.end()) {
        LOG_I("notify Trigger state: %d TriggerID: %d", pState, pTriggerId);
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
            if (mIsSuspending == true)
            {
                mIsRobSsrRunning = true;
                mIsSuspending = false;

                LOG_D("mTransmissioIdList size = %d", mTransmissioIdList.size());
                if ( mTransmissioIdList.size() > 0U )
                {
                    mCurrentTransmissionId = mTransmissioIdList.front();
                    const std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>::iterator itTrans{mRobSsrTransList.find(mCurrentTransmissionId)};
                    if (itTrans != mRobSsrTransList.end())
                    {
                        itTrans->second->connect();
                    }
                } else {
                    // Release OBC resource
                    DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
                    onFinishAcquisition(mTriggerId);
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                }
            }
            else
            {
                LOG_I("TRIGGER_PROCESSING");
                const android::sp<CommonDefine::RDGLocationData> location{LocationManagerAdapter::getInstance()->getLocationData()};
                const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
                const uint32_t prio_data{it->second->getPriority()};
                const uint64_t colID{it->second->getCollectionID()};
                mTriggerId = pTriggerId;
                mCurrentDiagTriggerType = pTriggerType;
                mColId = colID;
                mPriority = prio_data;
                const android::sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_START_ROBSSR, location)};
                msg->arg1 = static_cast<int32_t>(pTriggerType);
                (void)msg->sendToTarget();
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED:
        {
            LOG_I("TRIGGER_SUSPENDED");
            if (mIsRobSsrRunning == true)
            {
                mIsRobSsrRunning = false;
                mIsSuspending = true;
                const std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>::iterator itTrans{mRobSsrTransList.find(mCurrentTransmissionId)};
                if (itTrans != mRobSsrTransList.end())
                {
                    itTrans->second->disconnect();
                }
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
                if (mTriggerId < static_cast<uint32_t>(INT32_MAX))
                {
                    PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mCurrentDiagTriggerType);
                }
                else
                {
                    LOG_D("triggerId value error: %d in mTriggerList", mTriggerId);
                }
                
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
        {
            LOG_I("TRIGGER_DISCARDED - DueToIGOff = %d", dueToIgOff);
            const std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>::iterator itTrans{mRobSsrTransList.find(mCurrentTransmissionId)};
            if (itTrans != mRobSsrTransList.end())
            {
                if (mRobSsrTransList[mCurrentTransmissionId]->getState() == RobSsrUdsTransmission::State::ROBSSR_TRANS_REFERENCEDATA_REQUEST)
                {
                    LOG_E("Under repair state, store undefined value to reference data");
                    mResSID22[mRobSsrTransList[mCurrentTransmissionId]->getEcuInfo().getTargetAddress()].setEcuMakerCode(0xFFFFU);
                    mResSID22[mRobSsrTransList[mCurrentTransmissionId]->getEcuInfo().getTargetAddress()].setSwPartNumber("");
                    (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                }
                else
                {
                    // Abort aquisition
                    mIsRobSsrRunning = false;
                    while(mTransmissioIdList.empty() != true) {
                        mTransmissioIdList.pop();
                    }

                    itTrans->second->disconnect();
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                    mRobSsrTransList.clear();
                    
                    mRobSsrUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
                    if (mRobSsrUploadErrorData != nullptr)
                    {
                        if (dueToIgOff == true)
                        {
                            LOG_I("RoBSSR acquisition sequence is aborted due to IGN OFF");
                            mRobSsrUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
                        }
                        else
                        {
                            LOG_I("RoBSSR acquisition sequence is aborted by Priority Control");
                            mRobSsrUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED);
                        }
                        mIsNeedUploadErrorData = true;
                        makeErrorUploadRequest(*mRobSsrUploadErrorData, mCurrentDiagTriggerType, mColId);
                    } else {
                        LOG_E("mRobSsrUploadErrorData is null");
                    }

                    if (mTriggerId < static_cast<uint32_t>(INT32_MAX))
                    {
                        PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mCurrentDiagTriggerType);
                    }
                    else
                    {
                        LOG_D("triggerId value error: %d in mTriggerList", mTriggerId);
                    }
                }
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DONE:
        {
            LOG_I("TRIGGER_DONE");
            if (mTriggerId < static_cast<uint32_t>(INT32_MAX))
            {
                PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mCurrentDiagTriggerType);
            }
            else
            {
                LOG_D("triggerId value error: %d in mTriggerList", mTriggerId);
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_STATE_MAX:
        {
            LOG_I("TRIGGER_STATE_MAX");
            break;
        }
        }
    }
}

RemoteRoBSSR::RobSsrUdsTransmission::RobSsrUdsTransmission( RemoteRoBSSR& robssr
                                                            , const CommonDefine::EcuInformation& fEcuInfo
                                                            , const uint32_t fRobCode
                                                            , const bool fHasSubFn
                                                            , const uint32_t fSubFunction
                                                            , const bool fHasMemorySelection
                                                            , const uint32_t fMemorySelection
                                                            , const bool fHasRobFrameNo
                                                            , const std::vector<uint32_t> fRobFrameNoList
                                                            , const bool fHasCrcInfo
                                                            , const uint32_t fOccurredCRC
                                                            , const uint32_t fTimeSeriesCRC)
: android::RefBase()
, mRoBSSR(robssr)
, mTimerHandler(robssr)
, mTimeOut(&mTimerHandler, TimerHandler::ID_TRANSMISSION_TIMEOUT)
, ecuInfo(fEcuInfo)
, connectId(0U)
, transmissionId(0U)
, robCode(fRobCode)
, hasSubFn(fHasSubFn)
, subFunction(fSubFunction)
, hasMemorySelection(fHasMemorySelection)
, memorySelection(fMemorySelection)
, hasRobFrameNo(fHasRobFrameNo)
, robFrameNoList(fRobFrameNoList)
, hasCrcInfo(fHasCrcInfo)
, occurredCRC(fOccurredCRC)
, timeSeriesCRC(fTimeSeriesCRC)
, upload_occurredCRC(0U)
, upload_timeSeriesCRC(0U)
, framesDiscarded(false)
, m_state(State::ROBSSR_TRANS_INIT)
{
    mTimeOut.setDuration(TRANSMISSION_TIME_OUT_DURATION, 0U);
    transmissionId = (static_cast<uint64_t>(ecuInfo.getTargetAddress()) << 32U) | static_cast<uint64_t>(robCode);
    LOG_I("ecuInfo.targetAddress: %ld, robCode: %d, hasSubFn: %d, subFunction: %d, hasMemorySelection: %d, memorySelection: %d, hasRobFrameNo: %d, hasCrcInfo: %d, occurredCRC: %d, timeSeriesCRC: %d", 
        ecuInfo.getTargetAddress(), robCode, hasSubFn, subFunction, hasMemorySelection, memorySelection, hasRobFrameNo, hasCrcInfo, occurredCRC, timeSeriesCRC);
    LOG_I("robFrameNoList: ");
    for (std::vector<uint32_t>::iterator it{robFrameNoList.begin()}; it != robFrameNoList.end(); ++it) {
        LOG_I("robFrameNo: %d", *it);
    }
}

void RemoteRoBSSR::RobSsrUdsTransmission::transInit()
{
    //add SID 10 request
    this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_SESSIONCONTROL_REQUEST;
    uint8_t sfid{0x00U};
    if (this->ecuInfo.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6)
    {
        sfid = 0x03U; //extended session
    } else
    {
        sfid = 0x40U; //remote session
    }
    const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
    pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL));
    pUdsReq->setSFID(sfid);
    udsReqList.push(pUdsReq);
}

void RemoteRoBSSR::RobSsrUdsTransmission::connect()
{
    LOG_I("Start connect, transmissionID = 0x%02llx", this->transmissionId);
    this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_CONNECT;
    const android::sp<OBCTransportInfo> mtransportInfo{new OBCTransportInfo()};
    const android::sp<OBCConnectInfo> mConnectInfo{new OBCConnectInfo()};
    const uint8_t mprotocolType{RemoteEcuInformation::getInstance()->convertObcProtocolType(this->ecuInfo.getCommProtocol(), this->ecuInfo.getCommType(), this->ecuInfo.getTargetAddress())};
    // const android::sp<OBCTransportInfo> obcTransportInfo = new OBCTransportInfo();
    OBCCanInfo mcanInfo{};
    std::vector<std::string> ntaArray{};
    ntaArray.clear();
    if ((mprotocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
        (mprotocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
        (mprotocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
    {
        const uint16_t nTa{static_cast<uint16_t>(((this->ecuInfo.getTargetAddress() >> 8U) & 0xFFU))};
        std::stringstream ss{};
        ss << &std::hex << &std::uppercase << std::setw(2) << std::setfill('0') << nTa;
        const std::string hexString{ss.str()}; // Convert to string
        for (size_t i{0U}; i < hexString.size(); i++)
        {
            const std::string tmp{std::string(1U, hexString[i])};
            ntaArray.push_back(tmp);
        }
    }
    mcanInfo.setData(this->ecuInfo.getTargetAddress(), ntaArray);
    mtransportInfo->setData(mprotocolType, mcanInfo, false, 0U);
    (void)OnboardclientAdapter::getInstance()->connect(mtransportInfo, APP_NAME, mConnectInfo);
    
    if (mConnectInfo->getResponse() != OBCEnum::OBCErrCode::OBC_OK)
    {
        LOG_E("can't connect to canid = %d", this->ecuInfo.getTargetAddress());
        (void)mRoBSSR.mHandler->obtainMessage(MainHandler::CMD_ROBSSR_FINISH_TRANSMISSION)->sendToTarget();
    } else {
        LOG_D("connect success transmissionID = 0x%02llx", this->transmissionId);
        this->connectId = mConnectInfo->getConnectId();
        
        OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
        this->sendNextUdsRequest();
    }
}

void RemoteRoBSSR::RobSsrUdsTransmission::stopTimeout()
{
    this->mTimeOut.stop();
}

void RemoteRoBSSR::RobSsrUdsTransmission::disconnect()
{
    (void)OnboardclientAdapter::getInstance()->disconnectECU(this->connectId);
    OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
    this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_DONE;
    mRoBSSR.finishCurrentTransmission();
}

void RemoteRoBSSR::RobSsrUdsTransmission::sendNextUdsRequest()
{
    if (udsReqList.empty() != true)
    {
        const android::sp<::Buffer> udsData{this->udsReqList.front()->ToUdsData()};

        const uint8_t err{OnboardclientAdapter::getInstance()->sendUdsData(this->connectId, udsData)};
        if (err != 0x00U) //E_OK
        {
            LOG_E("Send SID 0x%02x for transmissionId 0x%02llx error = %d -> Disconnect", this->udsReqList.front()->getSID(), this->transmissionId, err);
            this->udsReqList.pop();
            if (udsReqList.empty() == true)
            {
                this->disconnect();
            } else
            {
                this->sendNextUdsRequest();
            }
        }
        else {
            LOG_D("Send SID 0x%02x for transmissionId 0x%02llx success", this->udsReqList.front()->getSID(), this->transmissionId);
            this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_SEND_UDS;
            mTimeOut.start();
        }
    }
    else
    {
        this->disconnect();
        this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_DISCONNECT;
        mRoBSSR.finishCurrentTransmission();
    }
}

void RemoteRoBSSR::RobSsrUdsTransmission::handleUdsResponse(const android::sp<UdsMessage> udsResponse, const android::sp<OBCResponseEventInfo> responseEventInfo)
{
    const uint8_t sid {udsResponse->getSID()};
    const uint8_t sfid {udsResponse->getSFID()};
    
    LOG_I("sid: 0x%02x, sfid: 0x%02x", sid, sfid);

    switch (sid)
    {
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_SESSION_CONTROL):
        {
            this->stopTimeout();
            //TODO: handle session control response
            if ((sfid == static_cast<uint8_t>(0x03)) && (this->ecuInfo.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6))
            {
                //add SID 31 req to obtain RoBFrameNo
                const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_31_ROUTINE_CONTROL));
                pUdsReq->setSFID(0x01U); //start routine
                uint8_t payload[6]{0U,};
                //RID 0xD000 (Retrving UserDefDTCSnapShotRecord)
                payload[0] = 0xD0U;
                payload[1] = 0x00U;
                //RoB code
                payload[2] = static_cast<uint8_t>((this->robCode >> 16U) & 0xFFU);
                payload[3] = static_cast<uint8_t>((this->robCode >> 8U) & 0xFFU);
                payload[4] = static_cast<uint8_t>(this->robCode & 0xFFU);
                //memory selection
                payload[5] = static_cast<uint8_t>(this->memorySelection & 0xFFU);
                pUdsReq->GetUdsPayload()->setTo(&payload[0], sizeof(payload));
                udsReqList.push(pUdsReq);
            } else
            {
                //add SID AB req to obtain RoBFrameNo
                const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_AB_READ_ROB_INFORMATION));
                pUdsReq->setSFID(static_cast<uint8_t>(this->subFunction & 0xFFU));
                uint8_t robData[2]{0U, };
                robData[0] = static_cast<uint8_t>((this->robCode >> 8U) & 0xFFU);
                robData[1] = static_cast<uint8_t>(this->robCode & 0xFFU);
                pUdsReq->GetUdsPayload()->setTo(&robData[0], sizeof(robData));
                udsReqList.push(pUdsReq);
            }
            //remove the request in front and send next request
            this->udsReqList.pop();
            this->sendNextUdsRequest();
            break;
        }
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_ROUTINE_CONTROL):
        {
            this->stopTimeout();
            //TODO: handle SID 31 response
            const android::sp<::Buffer> udsData{udsResponse->ToUdsData()};
            if(udsData->data() != nullptr) {
            const uint16_t rid{static_cast<uint16_t>(static_cast<uint16_t>(udsData->data()[2]) << 8U) | static_cast<uint16_t>(udsData->data()[3])};
            const uint32_t resRobCode{static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[5]) << 16U) | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[6]) << 8U) | static_cast<uint32_t>(udsData->data()[7])};
            const uint8_t resMemmorySelection{udsData->data()[8]};
            if ((resRobCode != this->robCode) || (resMemmorySelection != (this->memorySelection & 0xFFU)))
            {
                LOG_E("robCode/memorySelection in the response is different from request");
                (void)resMemmorySelection;
                (void)rid;
                break;
            }

            if (rid == 0xD000U)
            {
                //response of robFrameNo request
                const uint8_t typeOfFrameNo{udsData->data()[9]};
                if (typeOfFrameNo == 0x00U) // Standard type
                {
                    // get RoBFrameNoList
                    std::vector<uint8_t> occurenceRoBFrameNoList{};
                    std::vector<uint8_t> timeseriesRoBFrameNoList{};
                    const uint32_t numRobFrameNo{udsData->size() - 12U};
                    if (numRobFrameNo <= static_cast<uint32_t>(INT32_MAX))
                    {
                        for (int32_t idx{0}; idx < static_cast<int32_t>(numRobFrameNo); idx++)
                        {
                            const uint8_t robFrameNo{udsData->data()[12+idx]};
                            if (robFrameNo > 0x1FU)
                            {
                                if (this->hasCrcInfo == true)
                                {
                                    timeseriesRoBFrameNoList.push_back(robFrameNo);                               
                                }
                            }
                            else
                            {
                                occurenceRoBFrameNoList.push_back(robFrameNo);
                            }
                        }
                    }
                    else
                    {
                        LOG_E("numRobFrameNo > INT32_MAX");
                    }
                    //RDG30-R-0992
                    sort(occurenceRoBFrameNoList.begin(), occurenceRoBFrameNoList.end(), greater<uint8_t>());
                    sort(timeseriesRoBFrameNoList.begin(), timeseriesRoBFrameNoList.end(), greater<uint8_t>());
                    // send UDS request to get RoBSSR data                  
                    for (size_t id{0U}; id < occurenceRoBFrameNoList.size(); id++) 
                    {
                        const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                        pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION));
                        pUdsReq->setSFID(0x18U); //SubFunction 0x18
                        //RoBCode
                        pUdsReq->GetUdsPayload()->setTo(&udsData->data()[5], 1);
                        pUdsReq->GetUdsPayload()->append(&udsData->data()[6], 1);
                        pUdsReq->GetUdsPayload()->append(&udsData->data()[7], 1);
                        //UserDefDTCSnapshotRecordNumber
                        pUdsReq->GetUdsPayload()->append(&occurenceRoBFrameNoList[id], 1);
                        //MemorySelection
                        pUdsReq->GetUdsPayload()->append(&udsData->data()[8], 1);
                        udsReqList.push(pUdsReq);
                    }
                                        
                    for (size_t id{0U}; id < timeseriesRoBFrameNoList.size(); id++) 
                    {
                        const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                        pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION));
                        pUdsReq->setSFID(0x18U); //SubFunction 0x18
                        //RoBCode
                        pUdsReq->GetUdsPayload()->setTo(&udsData->data()[5], 1);
                        pUdsReq->GetUdsPayload()->append(&udsData->data()[6], 1);
                        pUdsReq->GetUdsPayload()->append(&udsData->data()[7], 1);
                        //UserDefDTCSnapshotRecordNumber
                        pUdsReq->GetUdsPayload()->append(&timeseriesRoBFrameNoList[id], 1);
                        //MemorySelection
                        pUdsReq->GetUdsPayload()->append(&udsData->data()[8], 1);
                        udsReqList.push(pUdsReq);
                    }
                    //remove the request in front and send next request
                    this->udsReqList.pop();
                    this->sendNextUdsRequest();
                }
                else if (typeOfFrameNo == 0x01U) // Widened type
                {
                    // get RoBFrameNoList
                    std::vector<uint32_t> occurenceRoBFrameNoList{};
                    std::vector<uint32_t> timeseriesRoBFrameNoList{};
                    uint32_t numRobFrameNo{0U};
                    const uint32_t udsDataSize{udsData->size()};
                    if (udsDataSize >= 12U)
                    {
                        numRobFrameNo = (udsDataSize - 12U) / 4U;
                    }
                    for (uint32_t idx{0U}; idx < numRobFrameNo; idx++)
                    {
                        // uint32_t robFrameNo = *(static_cast<uint32_t*>(&udsData->data()[12+idx*4]));
                        const uint32_t robFrameNo{static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[12U+idx*4U]) << 24U) | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[12U+idx*4U+1U]) << 16U)
                                                | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[12U+idx*4U+2U]) << 8U) | static_cast<uint32_t>(udsData->data()[12U+idx*4U+3U])};
                        if (((robFrameNo >> 16U) & 0xFFFFU) == 0x0000U)
                        {
                            occurenceRoBFrameNoList.push_back(robFrameNo);
                        }
                        else
                        {
                            timeseriesRoBFrameNoList.push_back(robFrameNo);
                        }
                    }
                    //RDG30-R-0992
                    sort(occurenceRoBFrameNoList.begin(), occurenceRoBFrameNoList.end(), greater<uint32_t>());
                    sort(timeseriesRoBFrameNoList.begin(), timeseriesRoBFrameNoList.end(), greater<uint32_t>());
                    // send UDS request to get RoBSSR data                  
                    for (size_t id{0U}; id < occurenceRoBFrameNoList.size(); id++) 
                    {
                        const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                        pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_31_ROUTINE_CONTROL));
                        pUdsReq->setSFID(0x01U); //start routine
                        //RIDOfRetrieveWidenedUserDefDTCSnapshotRecord
                        pUdsReq->GetUdsPayload()->setTo(&udsData->data()[10], 1);
                        pUdsReq->GetUdsPayload()->append(&udsData->data()[11], 1);
                        //WidenedUserDefDTCSnapshotRecordNumber
                        uint8_t occurenceRoBFrameNo_data[4];
                        (void)memcpy(&occurenceRoBFrameNo_data[0], &occurenceRoBFrameNoList[id], sizeof(occurenceRoBFrameNo_data));
                        pUdsReq->GetUdsPayload()->append(&occurenceRoBFrameNo_data[0], 4);
                        udsReqList.push(pUdsReq);
                    }
                                        
                    for (size_t id{0U}; id < timeseriesRoBFrameNoList.size(); id++) 
                    {
                        const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                        pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_31_ROUTINE_CONTROL));
                        pUdsReq->setSFID(0x01U); //start routine
                        //RIDOfRetrieveWidenedUserDefDTCSnapshotRecord
                        pUdsReq->GetUdsPayload()->setTo(&udsData->data()[10], 1);
                        pUdsReq->GetUdsPayload()->append(&udsData->data()[11], 1);
                        //WidenedUserDefDTCSnapshotRecordNumber
                        uint8_t timeseriesRoBFrameNo_data[4];
                        (void)memcpy(&timeseriesRoBFrameNo_data[0], &timeseriesRoBFrameNoList[id], sizeof(timeseriesRoBFrameNo_data));
                        pUdsReq->GetUdsPayload()->append(&timeseriesRoBFrameNo_data[0], 4);
                        udsReqList.push(pUdsReq);
                    }
                    //remove the request in front and send next request
                    this->udsReqList.pop();
                    this->sendNextUdsRequest();
                }
                else
                {
                    LOG_E("wrong type");
                }
            } else
            {
                //response of RoBSSR request - Widened type
                //Phase 6
                // this->mDiagResp[transmissionId] = udsResponse;
                const uint32_t tempCRCVal {mRoBSSR.mCRCManager->calculateCRC32ForCommon(udsResponse)};
                const uint32_t robFrameNo{static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[12]) << 24U) | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[13]) << 16U)
                                        | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[14]) << 8U) | static_cast<uint32_t>(udsData->data()[15])};


                if (((robFrameNo >> 16U) & 0xFFFFU) == 0x0000U)
                {
                    if (tempCRCVal == this->occurredCRC)
                    {
                        const android::sp<UdsMessage> tmp_req{this->udsReqList.front()};
                        while (this->udsReqList.empty() != true)
                        {
                            const android::sp<::Buffer> tmp_UdsData {this->udsReqList.front()->ToUdsData()};
                            uint32_t tmp_robFrameNo{0U};
                            if(tmp_UdsData->data() != nullptr) {
                                tmp_robFrameNo = static_cast<uint32_t>(static_cast<uint32_t>(tmp_UdsData->data()[12]) << 24U) | static_cast<uint32_t>(static_cast<uint32_t>(tmp_UdsData->data()[13]) << 16U)
                                | static_cast<uint32_t>(static_cast<uint32_t>(tmp_UdsData->data()[14]) << 8U) | static_cast<uint32_t>(tmp_UdsData->data()[15]);
                            } else {
                                LOG_E("tmp_UdsData->data() is null");
                            }
                            if (((tmp_robFrameNo >> 16U) & 0xFFFFU) == 0x0000U) 
                            {
                                this->udsReqList.pop();
                            }
                            else
                            {
                                break;
                            }
                        }
                        udsReqList.push(tmp_req);
                    }
                    else
                    {
                        vccomif::rdg::v1::interfaces::DiagnosticsMessage diagMessage{};
                        this->createDiagnosticsData(diagMessage, responseEventInfo);
                        const uint32_t diagMessageSize{diagMessage.ByteSizeLong()};
                        if (mRoBSSR.mTotalSize <= UINT32_MAX - diagMessageSize)
                        {
                            mRoBSSR.mTotalSize += diagMessageSize;
                        }
                        else
                        {
                            LOG_E("total size overflow");
                        }
                        LOG_V("total size: %d", mRoBSSR.mTotalSize);
                        if (mRoBSSR.mTotalSize > mRoBSSR.ROBSSR_TOTAL_UPLOAD_DATA_SIZE_MAX)
                        {
                            this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_DISCONNECT;
                            this->disconnect();
                            LOG_I("Abort robssr due to total size exceeded");
                            this->udsReqList = {};
                            this->framesDiscarded = true;
                            mRoBSSR.finishCurrentTransmission();
                        }
                        else
                        {
                            this->upload_occurredCRC = tempCRCVal;
                            this->mDiagResponse.push_back(udsResponse);
                            this->mDiagRoBSSRResponse.push_back(diagMessage);
                        }
                    }
                }
                else
                {
                    if (tempCRCVal == this->timeSeriesCRC)
                    {
                        const android::sp<UdsMessage> tmp_req{this->udsReqList.front()};
                        while (this->udsReqList.empty() != true)
                        {
                            const android::sp<::Buffer> tmp_UdsData {this->udsReqList.front()->ToUdsData()};
                            uint32_t tmp_robFrameNo {0U};
                            if(tmp_UdsData->data() != nullptr) {
                                tmp_robFrameNo = static_cast<uint32_t>(static_cast<uint32_t>(tmp_UdsData->data()[12]) << 24U) | static_cast<uint32_t>(static_cast<uint32_t>(tmp_UdsData->data()[13]) << 16U)
                                | static_cast<uint32_t>(static_cast<uint32_t>(tmp_UdsData->data()[14]) << 8U) | static_cast<uint32_t>(tmp_UdsData->data()[15]);
                            } else {
                                LOG_E("tmp_UdsData->data() is null");
                            }
                            if (((tmp_robFrameNo >> 16U) & 0xFFFFU) != 0x0000U) 
                            {
                                this->udsReqList.pop();
                            }
                        }
                        udsReqList.push(tmp_req);
                    }
                    else
                    {
                        vccomif::rdg::v1::interfaces::DiagnosticsMessage diagMessage{};
                        this->createDiagnosticsData(diagMessage, responseEventInfo);
                        const uint32_t diagMessageSize{diagMessage.ByteSizeLong()};
                        if (mRoBSSR.mTotalSize <= UINT32_MAX - diagMessageSize)
                        {
                            mRoBSSR.mTotalSize += diagMessageSize;
                        }
                        else
                        {
                            LOG_E("total size overflow");
                        }
                        LOG_V("total size: %d", mRoBSSR.mTotalSize);
                        if (mRoBSSR.mTotalSize > mRoBSSR.ROBSSR_TOTAL_UPLOAD_DATA_SIZE_MAX)
                        {
                            this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_DISCONNECT;
                            this->disconnect();
                            LOG_I("Abort robssr due to total size exceeded");
                            this->udsReqList = {};
                            this->framesDiscarded = true;
                            mRoBSSR.finishCurrentTransmission();
                        }
                        else
                        {
                            this->upload_timeSeriesCRC = tempCRCVal;
                            this->mDiagResponse.push_back(udsResponse);
                            this->mDiagRoBSSRResponse.push_back(diagMessage);
                        }
                    }
                }

                //get reference data
                if ((udsReqList.size() == 1U) && (mRoBSSR.mResSID22.find(this->ecuInfo.getTargetAddress()) == mRoBSSR.mResSID22.end()))
                {
                    this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_REFERENCEDATA_REQUEST;
                    // send SID22 DID$F18A
                    const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                    pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
                    pUdsReq->setSFID(0xF1U); //start routine
                    constexpr uint8_t tmp_did{0x8AU};
                    pUdsReq->GetUdsPayload()->setTo(&tmp_did, sizeof(uint8_t));
                    udsReqList.push(pUdsReq);

                    //send SID22 DID$F188
                    const android::sp<UdsMessage> pUdsReq1 {new UdsMessage()};
                    pUdsReq1->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
                    pUdsReq1->setSFID(0xF1U); //start routine
                    constexpr uint8_t tmp_did1{0x88U};
                    pUdsReq1->GetUdsPayload()->setTo(&tmp_did1, sizeof(uint8_t));
                    udsReqList.push(pUdsReq1);
                }
                this->udsReqList.pop();
                this->sendNextUdsRequest();
            }
            break;
            } else {
                LOG_E("udsData->data() is nullptr");
            }
            break;
        }
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION):
        {
            this->stopTimeout();
            //TODO: handle SID 19 response
            //response of RoBSSR request - Standard type
            //Phase 6
            const android::sp<::Buffer> udsData{udsResponse->ToUdsData()};
            const uint32_t tempCRCVal {mRoBSSR.mCRCManager->calculateCRC32ForCommon(udsResponse)};
            if(udsData->data() != nullptr) {
            const uint8_t robFrameNo {udsData->data()[7]};


            if (robFrameNo <= 0x1FU)
            {
                if (tempCRCVal == this->occurredCRC)
                {
                    const android::sp<UdsMessage> tmp_req{this->udsReqList.front()};
                    while (this->udsReqList.empty() != true)
                    {
                        const android::sp<::Buffer> tmp_UdsData {this->udsReqList.front()->ToUdsData()};
                        const uint8_t tmp_robFrameNo {udsData->data()[7]};
                        if (tmp_robFrameNo <= 0x1FU) 
                        {
                            this->udsReqList.pop();
                        }
                        else
                        {
                            break;
                        }
                    }
                    udsReqList.push(tmp_req);
                }
                else
                {
                    vccomif::rdg::v1::interfaces::DiagnosticsMessage diagMessage{};
                    this->createDiagnosticsData(diagMessage, responseEventInfo);
                    const uint32_t diagMessageSize{diagMessage.ByteSizeLong()};
                    if (mRoBSSR.mTotalSize <= UINT32_MAX - diagMessageSize)
                    {
                        mRoBSSR.mTotalSize += diagMessageSize;
                    }
                    else
                    {
                        LOG_E("total size overflow");
                    }
                    LOG_V("total size: %d", mRoBSSR.mTotalSize);
                    if (mRoBSSR.mTotalSize > mRoBSSR.ROBSSR_TOTAL_UPLOAD_DATA_SIZE_MAX)
                    {
                        this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_DISCONNECT;
                        this->disconnect();
                        LOG_I("Abort robssr due to total size exceeded");
                        this->udsReqList = {};
                        this->framesDiscarded = true;
                        mRoBSSR.finishCurrentTransmission();
                    }
                    else
                    {
                        this->upload_occurredCRC = tempCRCVal;
                        this->mDiagResponse.push_back(udsResponse);
                        this->mDiagRoBSSRResponse.push_back(diagMessage);
                    }
                }
            }
            else
            {
                if (tempCRCVal == this->timeSeriesCRC)
                {
                    const android::sp<UdsMessage> tmp_req{this->udsReqList.front()};
                    while (this->udsReqList.empty() != true)
                    {
                        const android::sp<::Buffer> tmp_UdsData {this->udsReqList.front()->ToUdsData()};
                        const uint32_t tmp_robFrameNo {udsData->data()[7]};
                        if (tmp_robFrameNo > 0x1FU)
                        {
                            this->udsReqList.pop();
                        }
                    }
                    udsReqList.push(tmp_req);
                }
                else
                {
                    vccomif::rdg::v1::interfaces::DiagnosticsMessage diagMessage{};
                    this->createDiagnosticsData(diagMessage, responseEventInfo);
                    const uint32_t diagMessageSize{diagMessage.ByteSizeLong()};
                    if (mRoBSSR.mTotalSize <= UINT32_MAX - diagMessageSize)
                    {
                        mRoBSSR.mTotalSize += diagMessageSize;
                    }
                    else
                    {
                        LOG_E("total size overflow");
                    }
                    LOG_V("total size: %d", mRoBSSR.mTotalSize);
                    if (mRoBSSR.mTotalSize > mRoBSSR.ROBSSR_TOTAL_UPLOAD_DATA_SIZE_MAX)
                    {
                        this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_DISCONNECT;
                        this->disconnect();
                        LOG_I("Abort robssr due to total size exceeded");
                        this->udsReqList = {};
                        this->framesDiscarded = true;
                        mRoBSSR.finishCurrentTransmission();
                    }
                    else
                    {
                        this->upload_timeSeriesCRC = tempCRCVal;
                        this->mDiagResponse.push_back(udsResponse);
                        this->mDiagRoBSSRResponse.push_back(diagMessage);
                    }
                }
            }
            // this->mDiagResp[transmissionId] = udsResponse;
            //get reference data
            if ((udsReqList.size() == 1U) && (mRoBSSR.mResSID22.find(this->ecuInfo.getTargetAddress()) == mRoBSSR.mResSID22.end()))
            {
                this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_REFERENCEDATA_REQUEST;
                // send SID22 DID$F18A
                const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
                pUdsReq->setSFID(0xF1U); //start routine
                constexpr uint8_t tmp_did{0x8AU};
                pUdsReq->GetUdsPayload()->setTo(&tmp_did, sizeof(uint8_t));
                udsReqList.push(pUdsReq);

                //send SID22 DID$F188
                const android::sp<UdsMessage> pUdsReq1 {new UdsMessage()};
                pUdsReq1->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
                pUdsReq1->setSFID(0xF1U); //start routine
                constexpr uint8_t tmp_did1{0x88U};
                pUdsReq1->GetUdsPayload()->setTo(&tmp_did1, sizeof(uint8_t));
                udsReqList.push(pUdsReq1);
            }
            this->udsReqList.pop();
            this->sendNextUdsRequest();
            } else {
                LOG_E("udsData->data() is null");
            }
            (void)tempCRCVal;
            break;
        }
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER):
        {
            this->stopTimeout();
            //TODO: handle SID 22 response
            const ReferenceData refData{};
            const android::sp<::Buffer> udsData{udsResponse->ToUdsData()};
            if(udsData->data() != nullptr) {
            const uint16_t resDid{static_cast<uint16_t>(static_cast<uint16_t>(udsData->data()[1]) << 8U) | static_cast<uint16_t>(udsData->data()[2])};
            if (resDid == 0xF18AU)
            {
                mRoBSSR.mResSID22[this->ecuInfo.getTargetAddress()].setEcuMakerCode(static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[3]) << 8U) | static_cast<uint32_t>(udsData->data()[4]));
            }
            else if ((resDid == 0xF188U) || (resDid == 0xF181U))
            {
                const uint32_t udsDataSize{udsData->size()};
                if (udsDataSize > 3U)
                {
                    std::string stSwPtNum{};
                    stSwPtNum.resize(udsDataSize - 3U);
                    if (stSwPtNum.size() > 0U)
                    {
                        (void)memcpy(&stSwPtNum[0], &udsData->data()[3], stSwPtNum.size());
                    }
                    mRoBSSR.mResSID22[this->ecuInfo.getTargetAddress()].setSwPartNumber(stSwPtNum);
                }
            }
            else
            {
                mRoBSSR.mResSID22[this->ecuInfo.getTargetAddress()].setEcuMakerCode(0xFFFFU);
            }
            this->udsReqList.pop();
            this->sendNextUdsRequest();
            } else {
                LOG_E("udsData->data() is null");
            }
            break;
        }
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_RESPONSE_READ_ROB_INFORMATION):
        {
            this->stopTimeout();
            //TODO: handle response of SID$AB
            const android::sp<::Buffer> udsData {udsResponse->ToUdsData()};
            if(udsData->data() != nullptr) {
            const uint32_t resRobCode {static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[2]) << 8U) | static_cast<uint32_t>(udsData->data()[3])};
            if (resRobCode != this->robCode)
            {
                LOG_E("robCode in the response is different from request");
                break;
            }

            switch (sfid)
            {
                case static_cast<uint8_t>(0x02):
                case static_cast<uint8_t>(0x12):
                {
                    // get RoBFrameNoList
                    std::vector<uint32_t> roBFrameNoList_standard{};
                    if (this->getHasRobFrameNo() == true) 
                    {
                        roBFrameNoList_standard = this->getRobFrameNoList();
                    }
                    else
                    {
                        const uint32_t numRobFrameNo{udsData->size()-4U};
                        for (uint32_t idx{0U}; idx < numRobFrameNo; idx++)
                        {
                            const uint32_t robFrameNo {static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[4U+idx*2U]) << 8U) | static_cast<uint32_t>(udsData->data()[4U+idx*2U+1U])};
                            roBFrameNoList_standard.push_back(robFrameNo);
                        }
                    }
                    sort(roBFrameNoList_standard.begin(), roBFrameNoList_standard.end(), greater<uint32_t>());
                    //push request message SID$AB
                    for (size_t id{0U}; id < roBFrameNoList_standard.size(); id++)
                    {
                        const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                        pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_AB_READ_ROB_INFORMATION));
                        pUdsReq->setSFID(static_cast<uint8_t>(sfid + 1U)); //SubFunction 03/13
                        //RoBCode
                        pUdsReq->GetUdsPayload()->setTo(&udsData->data()[2], 1);
                        pUdsReq->GetUdsPayload()->append(&udsData->data()[3], 1);
                        //RoBFrameNumber
                        uint8_t robFrameNo_data[2];
                        (void)memcpy(&robFrameNo_data[0], &roBFrameNoList_standard[id], sizeof(robFrameNo_data));
                        pUdsReq->GetUdsPayload()->append(&robFrameNo_data[0], 2);
                        udsReqList.push(pUdsReq);
                    }
                    this->udsReqList.pop();
                    this->sendNextUdsRequest();
                    break;
                }
                case static_cast<uint8_t>(0x22):
                case static_cast<uint8_t>(0x32):
                {
                     // get RoBFrameNoList
                    std::vector<uint32_t> roBFrameNoList_widened{};
                    if (this->getHasRobFrameNo() == true) 
                    {
                        roBFrameNoList_widened = this->getRobFrameNoList();
                    }
                    else
                    {
                        const uint32_t numRobFrameNo{udsData->size()-4U};
                        for (uint32_t idx{0U}; idx < numRobFrameNo; idx++)
                        {
                            const uint32_t robFrameNo{static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[4U+idx*4U]) << 24U) | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[4U+idx*4U+1U]) << 16U)
                                                    | static_cast<uint32_t>(static_cast<uint32_t>(udsData->data()[4U+idx*4U+2U]) << 8U) | static_cast<uint32_t>(udsData->data()[4U+idx*4U+3U])};
                            roBFrameNoList_widened.push_back(robFrameNo);
                        }
                    }

                    sort(roBFrameNoList_widened.begin(), roBFrameNoList_widened.end(), greater<uint32_t>());
                    //push request message SID$AB
                    for (size_t id{0U}; id < roBFrameNoList_widened.size(); id++)
                    {
                        const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
                        pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_AB_READ_ROB_INFORMATION));
                        pUdsReq->setSFID(static_cast<uint8_t>(sfid + 1U)); //SubFunction 23/33
                        //RoBCode
                        pUdsReq->GetUdsPayload()->setTo(&udsData->data()[2], 1);
                        pUdsReq->GetUdsPayload()->append(&udsData->data()[3], 1);
                        //RoBFrameNumber
                        uint8_t robFrameNo_data[4];
                        (void)memcpy(&robFrameNo_data[0], &roBFrameNoList_widened[id], sizeof(robFrameNo_data));
                        pUdsReq->GetUdsPayload()->append(&robFrameNo_data[0], 4);
                        udsReqList.push(pUdsReq);
                    }
                    this->udsReqList.pop();
                    this->sendNextUdsRequest();
                    break;
                }
                case static_cast<uint8_t>(0x03):
                case static_cast<uint8_t>(0x13):
                case static_cast<uint8_t>(0x23):
                case static_cast<uint8_t>(0x33):
                {
                    //response of RoBSSR request
                    // this->mDiagResp[transmissionId] = udsResponse;
                    vccomif::rdg::v1::interfaces::DiagnosticsMessage diagMessage{};
                    this->createDiagnosticsData(diagMessage, responseEventInfo);
                    const uint32_t diagMessageSize{diagMessage.ByteSizeLong()};
                    if (mRoBSSR.mTotalSize <= UINT32_MAX - diagMessageSize)
                    {
                        mRoBSSR.mTotalSize += diagMessageSize;
                    }
                    else
                    {
                        LOG_E("total size overflow");
                    }
                    LOG_V("total size: %d", mRoBSSR.mTotalSize);
                    if (mRoBSSR.mTotalSize > mRoBSSR.ROBSSR_TOTAL_UPLOAD_DATA_SIZE_MAX)
                    {
                        this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_DISCONNECT;
                        this->disconnect();
                        LOG_I("Abort robssr due to total size exceeded");
                        this->udsReqList = {};
                        this->framesDiscarded = true;
                        mRoBSSR.finishCurrentTransmission();
                    }
                    else
                    {
                        this->mDiagResponse.push_back(udsResponse);
                        this->mDiagRoBSSRResponse.push_back(diagMessage);
                    }
                    //get reference data
                    if ((udsReqList.size() == 1U) && (mRoBSSR.mResSID22.find(this->ecuInfo.getTargetAddress()) == mRoBSSR.mResSID22.end()))
                    {
                        this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_REFERENCEDATA_REQUEST;

                        //send SID22 DID$F181
                        const android::sp<UdsMessage> pUdsReq1 {new UdsMessage()};
                        pUdsReq1->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
                        pUdsReq1->setSFID(0xF1U); //start routine
                        constexpr uint8_t tmp_did1{0x81U};
                        pUdsReq1->GetUdsPayload()->setTo(&tmp_did1, sizeof(uint8_t));
                        udsReqList.push(pUdsReq1);
                    }
                    this->udsReqList.pop();
                    this->sendNextUdsRequest();
                    break;
                }
                default:
                {
                    break;
                }
            }
            } else {
                LOG_E("udsData->data() is null");
            }
            break;
        }       
        case static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE):
        {
            this->stopTimeout();
            //TODO: handle negative response
            if (udsReqList.size() > 0U) 
            {
                this->udsReqList.pop();
                this->sendNextUdsRequest();
            }
            else
            {
                this->disconnect();
                this->m_state = RobSsrUdsTransmission::State::ROBSSR_TRANS_DISCONNECT;
                mRoBSSR.finishCurrentTransmission();
            }
            break;
        }
        default:
        {
            LOG_E("unsupported SID 0x%02x", sid);
            break;
        }
    }
}

void RemoteRoBSSR::RobSsrUdsTransmission::printData(const std::string data) const
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

void RemoteRoBSSR::RobSsrUdsTransmission::createDiagnosticsData(vccomif::rdg::v1::interfaces::DiagnosticsMessage &diagMessage, const android::sp<OBCResponseEventInfo> responseEventInfo)
{
    if (responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_OK)
    {
        diagMessage.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL);
    }
    else if (responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_NEGATIVE)
    {
        diagMessage.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_SUCCESSFUL_WITH_NEGATIVE);
    }
    else
    {
        diagMessage.set_status_code(vccomif::rdg::v1::interfaces::StatusCode::SC_UNRESPONSIVE);
    }
    vccomif::rdg::v1::interfaces::EcuAddressInformation* const ecuAddressInfo{diagMessage.mutable_ecu_address_information()};
    ecuAddressInfo->set_communication_protocol(this->ecuInfo.getCommProtocol());
    const CenterCommunicationType diagResType{this->ecuInfo.getCommType()};
    ecuAddressInfo->set_communication_type(diagResType);
    uint32_t centerTargetAdd{this->ecuInfo.getTargetAddress()};
    if (diagResType ==  CenterCommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS)
    {
        centerTargetAdd = (centerTargetAdd >> 8U);
    }
    ecuAddressInfo->set_target_address(centerTargetAdd);
    if(responseEventInfo->resInfo()->udsData()->data() != nullptr) {
        diagMessage.set_user_data(responseEventInfo->resInfo()->udsData()->data(), responseEventInfo->resInfo()->udsData()->size());
    } else {
        LOG_E("udsData()->data() is nullptr");
    }
    // print data
    std::string std_diagMessage{""};
    google::protobuf::util::JsonOptions option{};
    option.always_print_primitive_fields = true;
    option.preserve_proto_field_names = true;
    (void)google::protobuf::util::MessageToJsonString(diagMessage, &std_diagMessage, option);
    printData(std_diagMessage);
}

bool RemoteRoBSSR::calculateCRC() 
{
    bool isDifferent{false};
    if(mCRCManager != nullptr) {
        /*TBD: caculate CRC*/
        mCRCManager->calculateCRC32Data();
        isDifferent = mCRCManager->compareCrc32Value();
    } else {
        isDifferent = true;
    }
    return isDifferent;
}

void RemoteRoBSSR::finishCurrentTransmission(void)
{
    if (this->mIsRobSsrRunning == true) {
        LOG_I("finish Current transmission ID: 0x%02llx, mTransmissioIdList size = %d", mCurrentTransmissionId, mTransmissioIdList.size());
        mRobSsrTransList[mCurrentTransmissionId]->setState(RobSsrUdsTransmission::State::ROBSSR_TRANS_DONE);
        if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
        {
            RoBOccurrence::getInstance()->onRobSsrAcquisitionCompleted(true, mRobSsrTransList[mCurrentTransmissionId]->getUpload_occurredCRC(), mRobSsrTransList[mCurrentTransmissionId]->getUpload_timeSeriesCRC());
        }
        else if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
        {
            RoBOccurrence::getInstance()->onRobSsrAcquisitionCompleted_CenterRequest(mRobSsrTransList[mCurrentTransmissionId]->getEcuInfo().getTargetAddress(),
                mRobSsrTransList[mCurrentTransmissionId]->getUpload_occurredCRC(), mRobSsrTransList[mCurrentTransmissionId]->getUpload_timeSeriesCRC());
        }
        else
        {
            LOG_E("Trigger Type invalid");
        }
        (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
        if ( mTransmissioIdList.size() > 1U ) 
        {
            mTransmissioIdList.pop();
            mCurrentTransmissionId = mTransmissioIdList.front();
            const std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>::iterator it{mRobSsrTransList.find(mCurrentTransmissionId)};
            if (it != mRobSsrTransList.end())
            {
                it->second->connect();
            }
        } else  {
            if(mTransmissioIdList.empty() != true) {
                mTransmissioIdList.pop();
            }
            // Release OBC resource

            OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
            mIsRobSsrRunning = false;
        }
    }
}

void RemoteRoBSSR::onTransmissionTimeout(void)
{
    (void)mHandler->obtainMessage(MainHandler::CMD_TRANSMISSION_TIMEOUT)->sendToTarget();
}

void RemoteRoBSSR::handleTransmissionTimeout()
{
    LOG_I("Event Transmission Timeout, currentTrans = 0x%02llx", mCurrentTransmissionId);
    const std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>::iterator it{mRobSsrTransList.find(mCurrentTransmissionId)};
    if ((it != mRobSsrTransList.end()) && (it->second->getState() == RobSsrUdsTransmission::State::ROBSSR_TRANS_SEND_UDS))
    {
        it->second->disconnect();
    } 
}

void RemoteRoBSSR::onChangedRemoteInfo(const int32_t what, const int32_t info) noexcept
{
    LOG_I("onChangedRemoteInfo");
    (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE, what, info)->sendToTarget();
}

void RemoteRoBSSR::handleUnderRepairStatusChange(const int32_t& what, const int32_t& status)
{
    LOG_D("handleUnderRepairStatusChange");
    switch (what)
    {
    case WHAT_CHANGED_REPAIR_SATUS:
    {
        if (mIsRobSsrRunning == true)
        {
            const std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>::iterator itTrans{mRobSsrTransList.find(mCurrentTransmissionId)};
            if (itTrans != mRobSsrTransList.end())
            {
                if (mRobSsrTransList[mCurrentTransmissionId]->getState() == RobSsrUdsTransmission::State::ROBSSR_TRANS_REFERENCEDATA_REQUEST)
                {
                    LOG_E("Under repair state, store undefined value to reference data");
                    mResSID22[mRobSsrTransList[mCurrentTransmissionId]->getEcuInfo().getTargetAddress()].setEcuMakerCode(0xFFFFU);
                    mResSID22[mRobSsrTransList[mCurrentTransmissionId]->getEcuInfo().getTargetAddress()].setSwPartNumber("");
                    (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                }
                else
                {
                    // Abort acquisition
                    mIsRobSsrRunning = false;

                    while (mTransmissioIdList.empty() != true) {
                        mTransmissioIdList.pop();
                    }
                    itTrans->second->disconnect();
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                    mRobSsrTransList.clear();
                    mRobSsrUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
                    if (mRobSsrUploadErrorData != nullptr)
                    {
                        mRobSsrUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR);
                        LOG_I("The RoBSR acquisition sequence is aborted -> push error upload data package");
                        mIsNeedUploadErrorData = true;
                        makeErrorUploadRequest(*mRobSsrUploadErrorData, mCurrentDiagTriggerType, mColId);
                    }
                }
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }
    (void)status;
}

void RemoteRoBSSR::makeUploadRequest(void)
{
    LOG_I("Make RoBSSR data to upload");
    uint32_t sizeOfFile{0U};
    // split file
    // const std::unordered_map<uint64_t, android::sp<RobSsrUdsTransmission>>::iterator it{mRobSsrTransList.find(mCurrentTransmissionId)};
    std::vector<vccomif::rdg::v1::interfaces::DiagnosticsMessage> tmpRoBSSRRes {mRobSsrTransList[mCurrentTransmissionId]->getDiagRoBSSRResponse()};
    LOG_I("Size of response: %d", tmpRoBSSRRes.size());
    std::vector<vccomif::rdg::v1::interfaces::DiagnosticsMessage>::iterator it_ResData{tmpRoBSSRRes.begin()};
    for (uint32_t i{0U}; i < MAX_SPLIT_FILE; i++)
    {
        LOG_V("Current file: %d", i);
        UploadRobSsrDataRequest pUploadRobSsrDataRequest{};
        pUploadRobSsrDataRequest.CopyFrom(*mRobSsrDataReq);
        sizeOfFile = pUploadRobSsrDataRequest.ByteSizeLong();
        while (it_ResData != tmpRoBSSRRes.end())
        {
            // -> sizeOfFile + getSize() > 4194304 --> ko push
            if (sizeOfFile <= UINT32_MAX - it_ResData->ByteSizeLong())
            {
                sizeOfFile += it_ResData->ByteSizeLong();
            }
            else
            {
                LOG_E("file size overflow");
            }
            LOG_V("Size of user data: %d, sizeOfFile: %d", it_ResData->ByteSizeLong(), sizeOfFile);
            if ((sizeOfFile <= ROBSSR_UPLOAD_DATA_SIZE_MAX) /* test: 55 + X */ && (i < MAX_SPLIT_FILE - 1U)) // 4M - 1st and 2nd file //
            {
                vccomif::rdg::v1::interfaces::DiagnosticsMessage* const diagMessage{pUploadRobSsrDataRequest.add_rob_ssr_messages()};
                diagMessage->CopyFrom(*it_ResData);
                it_ResData++;
            }
            else if ((sizeOfFile <= ROBSSR_LAST_UPLOAD_DATA_SIZE_MAX) /* test: 55 + X */ && (i == MAX_SPLIT_FILE - 1U)) // 2M - last file
            {
                vccomif::rdg::v1::interfaces::DiagnosticsMessage* const diagMessage{pUploadRobSsrDataRequest.add_rob_ssr_messages()};
                diagMessage->CopyFrom(*it_ResData);
                it_ResData++;
            }
            else
            {
                break;
            }
        }
        //set Reference data to upload
        const std::unordered_map<uint32_t, ReferenceData>::iterator itRef{mResSID22.find(mRobSsrTransList[mCurrentTransmissionId]->getEcuInfo().getTargetAddress())};
        if (itRef != mResSID22.end())
        {
            pUploadRobSsrDataRequest.set_software_part_number(itRef->second.getSwPartNumber());
            pUploadRobSsrDataRequest.set_ecu_maker_code(itRef->second.getEcuMakerCode());
        }
        else
        {
            //RDG30-R-0777
            const uint32_t undefinedEcuMakerCode {0xFFFFU};
            const std::string undefinedSwPartNumber {""};
            pUploadRobSsrDataRequest.set_software_part_number(undefinedSwPartNumber);
            pUploadRobSsrDataRequest.set_ecu_maker_code(undefinedEcuMakerCode);
        }
        //set Memory_Selection
        pUploadRobSsrDataRequest.set_memory_selection(mRobSsrTransList[mCurrentTransmissionId]->getMemory_selection());
        //set RoB Code
        pUploadRobSsrDataRequest.set_rob(mRobSsrTransList[mCurrentTransmissionId]->getRob_code());
        //set frames_are_discarded
        pUploadRobSsrDataRequest.set_frames_are_discarded(mRobSsrTransList[mCurrentTransmissionId]->getFramesAreDiscarded());
        //set CRC Information
        if (mRobSsrTransList[mCurrentTransmissionId]->getHasCrcInfor() == true)
        {
            pUploadRobSsrDataRequest.mutable_crc_information()->set_occurred_rob_ssr_crc(mRobSsrTransList[mCurrentTransmissionId]->getUpload_occurredCRC());
            pUploadRobSsrDataRequest.mutable_crc_information()->set_time_series_rob_ssr_crc(mRobSsrTransList[mCurrentTransmissionId]->getUpload_timeSeriesCRC());
        }
        l_UploadRoBSSRDataRequest.push_back(pUploadRobSsrDataRequest);
        if (it_ResData == tmpRoBSSRRes.end())
        {
            break;
        }
    }
    for (size_t i{0U}; i < l_UploadRoBSSRDataRequest.size(); i++)
    {
        l_UploadRoBSSRDataRequest[i].set_split_counter(i + 1U);
        if (i == l_UploadRoBSSRDataRequest.size() - 1U)
        {
            l_UploadRoBSSRDataRequest[i].set_last_split_data_flag(true);
        }
        else
        {
            l_UploadRoBSSRDataRequest[i].set_last_split_data_flag(false);
        }
    }
    for (const auto it : l_UploadRoBSSRDataRequest)
    {
        std::string str_robssr_upload{""};
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        (void)google::protobuf::util::MessageToJsonString(it, &str_robssr_upload, option);
        printData(str_robssr_upload);
    }

    /*Save UploadRoBSSRDataRequest to file*/
    for (const auto it : l_UploadRoBSSRDataRequest)
    {
        const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
        std::string file_dir{std::to_string(uploadId)};
        (void)file_dir.append("_UploadRoBSSRDataRequest.dat");
        const error_t bStored{DataModel<UploadRobSsrDataRequest>::save(file_dir, it)};
        if (bStored == E_OK)
        {
            const uint8_t operation{getOperation()};
            //Self-diag
            DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        }
        const android::sp<UploadTask> task{new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG060);
        task->setUploadPatch(file_dir);
        task->setUploadId(static_cast<uint64_t>(uploadId));
        /*Set priority*/
        task->setUploadPrio(mPriority);
        const uint64_t fileSize{static_cast<uint64_t>(it.ByteSizeLong())};
        task->setFileSize(fileSize);
        UploadManager::getInstance()->requestUploadTask(task);
        // test_saveUploadData = task;
    }
    mRobSsrDataReq->Clear();
    l_UploadRoBSSRDataRequest.clear();
    mIsRobSsrRunning = false;
    onFinishAcquisition(mTriggerId);
    (void)sizeOfFile;
    (void)it_ResData;
}

void RemoteRoBSSR::makeErrorUploadRequest(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colId)
{
    LOG_I("Make RoBSSR Error data");
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);

    vccomif::rdg::v1::interfaces::RdgCommonRequestHeader *const mRdgCommonRequestHeader{errorData.mutable_rdg_common_request_header()};
    vccomif::common::v1::AppCommonHeaderVehicleToCenter *const mAppCommonHeaderVehicleToCenter{mRdgCommonRequestHeader->mutable_app_common_header()};

    mAppCommonHeaderVehicleToCenter->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    mAppCommonHeaderVehicleToCenter->set_electronic_pf(EPF_19EPF);
    mAppCommonHeaderVehicleToCenter->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);

    mRdgCommonRequestHeader->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    mRdgCommonRequestHeader->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, counterValue));

    const sp<CommonDefine::RDGLocationData> mLocation{LocationManagerAdapter::getInstance()->getLocationData()};
    errorData.set_collection_condition_id(colId); /*TBD*/
    errorData.set_counter_value(counterValue);
    (void)counterValue;
    vccomif::rdg::v1::interfaces::TriggerType errorTriggerType{vccomif::rdg::v1::interfaces::TriggerType::TT_UNKNOWN};
    if (type == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
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
    const bool bOBDFlag{mApp.getOBDStatus()};
    errorData.set_obd2_installed_flag(bOBDFlag);
    errorData.set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
    errorData.set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ROB_ACQUISITION_REQUEST);
    std::string str_err_robssr{""};
    google::protobuf::util::JsonOptions option{};
    option.always_print_primitive_fields = true;
    option.preserve_proto_field_names = true;
    (void)google::protobuf::util::MessageToJsonString(errorData, &str_err_robssr, option);
    printData(str_err_robssr);

    const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
    std::string file_dir{std::to_string(uploadId)};
    (void)file_dir.append("_UploadErrorRoBSSRDataRequest.dat");
    (void)DataModel<UploadErrorDataRequest>::save(file_dir, errorData);
    const android::sp<UploadTask> task{new UploadTask(uploadId)};

    task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
    task->setUploadPatch(file_dir);
    task->setUploadId(static_cast<uint64_t>(uploadId));
    /*Set priority*/
    task->setUploadPrio(mPriority);
    const uint64_t fileSize{static_cast<uint64_t>(errorData.ByteSizeLong())};
    task->setFileSize(fileSize);
    UploadManager::getInstance()->requestUploadTask(task);
    mRobSsrUploadErrorData->Clear();
}

std::map<uint64_t, android::sp<UdsMessage>> RemoteRoBSSR::getDiagResponseList() const noexcept
{
    std::map<uint64_t, android::sp<UdsMessage>> diagResponse{};
    return diagResponse;
}

void RemoteRoBSSR::onFinishAcquisition(const uint32_t& triggerId)
{
    /* Check if pTriggerId is in savedID then notify done to priorityControl*/
    LOG_I("Finish RoBSSR Acquisition triggerID: %d", triggerId);
    LOG_D("Check mSaveReq size before: %d", mSaveReq.size());
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(triggerId)};
    if(it != mSaveReq.end()) {
        if (triggerId < static_cast<uint32_t>(INT32_MAX))
        {
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(triggerId), it->second->getType());
        }
        else
        {
            LOG_D("triggerId value error: %d in mTriggerList", triggerId);
        }
        (void)mSaveReq.erase(it);
    } else {
        LOG_D("Can not find triggerId: %d in mSaveReq", triggerId);
    }
    LOG_D("Check mSaveReq size after: %d", mSaveReq.size());
}

void RemoteRoBSSR::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) noexcept
{
    const android::sp<OBCCanInfo> canInfo {responseEventInfo->resInfo()->canInfo()};
    const uint16_t connectId {responseEventInfo->resInfo()->connectId()};
    
    LOG_D("onReceiveUDS canId = 0x%02x", canInfo->canId());
    LOG_D("onReceiveUDS connectId = %d", connectId);

    if (this->mIsRobSsrRunning == true) {
        if ((mTransmissioIdList.empty() != true) && (mRobSsrTransList[mCurrentTransmissionId]->getConnectId() == connectId))
        {
            mRobSsrTransList[mCurrentTransmissionId]->handleUdsResponse(udsResponse, responseEventInfo);
        }
    }
    return;
}

void RemoteRoBSSR::makeHeaderForUploading(const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadRobSsrDataRequest> &robssrDataUpload)
{
    LOG_I("Make RoBSSR Header upload data");
    // RdgCommonRequestHeader

    vccomif::rdg::v1::interfaces::RdgCommonRequestHeader* const mRdgCommonRequestHeader{robssrDataUpload->mutable_rdg_common_request_header()};
    vccomif::common::v1::AppCommonHeaderVehicleToCenter* const mAppCommonHeaderVehicleToCenter{mRdgCommonRequestHeader->mutable_app_common_header()};

    mAppCommonHeaderVehicleToCenter->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    mAppCommonHeaderVehicleToCenter->set_electronic_pf(EPF_19EPF);
    mAppCommonHeaderVehicleToCenter->set_geodesy_information(::vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
    // Interface type
    mRdgCommonRequestHeader->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ROB_SSR_DATA);

    // Set collection condition ID of occurrence RoB (fixed value)
    robssrDataUpload->set_collection_condition_id(mColId);
    // counter_value
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    // messageId
    mRdgCommonRequestHeader->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ROB_SSR_DATA, counterValue));

    robssrDataUpload->set_counter_value(counterValue);
    (void)counterValue;

    // Set Time information
    // Set Location information

    vccomif::rdg::v1::interfaces::Location* const mLocation{robssrDataUpload->mutable_location()};
    
    if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
    {
        robssrDataUpload->set_trigger_type(vccomif::rdg::v1::interfaces::TriggerType::TT_OTHER_TRIGGER);
        mDiagnosticsAcquisitionTime = (mDiagnosticsAcquisitionTime >= 0) ? mDiagnosticsAcquisitionTime : 0;
        robssrDataUpload->set_diagnostics_acquisition_time(static_cast<uint64_t>(mDiagnosticsAcquisitionTime));        
        try
        {
            mLocation->set_latitude(mLocationData->getLatitude());
            mLocation->set_longitude(mLocationData->getLongtitude());
        }
        catch (const std::exception& e)
        { 
            LOG_D("Exception caught: %s", e.what());
        }
        LOG_D("Get Current Time success: %lld", mDiagnosticsAcquisitionTime);
        LOG_D("Get location data success, 0x%08x 0x%08x ", mLocationData->getLatitude(), mLocationData->getLongtitude());
    }
    else if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
    {
        robssrDataUpload->set_trigger_type(vccomif::rdg::v1::interfaces::TriggerType::TT_OCCURRENCE_NOTIFICATION_TRIGGER);
        int64_t acquisitionTime{this->mRobNotificationRequest->triggerTime()};
        acquisitionTime = (acquisitionTime >= 0) ? acquisitionTime : 0;
        robssrDataUpload->set_spontaneous_notification_trigger_occurrence_time(static_cast<uint64_t>(acquisitionTime));
        try
        {
            mLocation->set_latitude(this->mRobNotificationRequest->triggerLocation()->getLatitude());
            mLocation->set_longitude(this->mRobNotificationRequest->triggerLocation()->getLongtitude());
        }
        catch (const std::exception &e)
        {
            LOG_D("Exception caught: %s", e.what());
        }
        LOG_D("Get Current Time success: %lld", this->mRobNotificationRequest->triggerTime());
        LOG_D("Get location data success, 0x%08x 0x%08x ", this->mRobNotificationRequest->triggerLocation()->getLatitude(), this->mRobNotificationRequest->triggerLocation()->getLongtitude());
    }
    else
    {
        robssrDataUpload->set_trigger_type(vccomif::rdg::v1::interfaces::TriggerType::TT_UNKNOWN);
        LOG_E("Invalid trigger type");
    }
    (void)mLocation;

    // Set OBD2 flag
    const bool bOBDFlag{mApp.getOBDStatus()};
    robssrDataUpload->set_obd2_installed_flag(bOBDFlag);
    // Set ODO data
    if (odo_unit == 0x2U)
    {
        robssrDataUpload->set_odo_information_mile(odo_value); // odo_unit = 10b ~ Mile
    }
    else if (odo_unit == 0x1U)
    {
        robssrDataUpload->set_odo_information_km(odo_value); // odo_unit = 01b ~ km
    }
    else
    {
        robssrDataUpload->set_odo_information_km(0xFFFFFFFF); // RDG30-R-1050 undefined value
    }
    robssrDataUpload->set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
    robssrDataUpload->set_split_counter(1U);
    robssrDataUpload->set_last_split_data_flag(true);
}

uint8_t RemoteRoBSSR::getOperation() const noexcept
{
    uint8_t operation{DiagManagerAdapter::COLLECTION_CONDITIONS};
    if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
    {
        operation = DiagManagerAdapter::ROB_NOTIFICATION_TRIGGER;
    }
    else if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
    {
        operation = DiagManagerAdapter::RD_SCHEDULE_TRIGGER;
    }
    else if (mCurrentDiagTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
    {
        operation = DiagManagerAdapter::COLLECTION_CONDITIONS;
    }
    else
    {
        operation = DiagManagerAdapter::COLLECTION_CONDITIONS;
    }
    return operation;
}

void RemoteRoBSSR::TimerHandler::handlerFunction (const int32_t timerId)
{
    switch(timerId) 
    {
        case ID_TRANSMISSION_TIMEOUT:
            mRoBSSR.onTransmissionTimeout();
            break;
        default:
            break;
    }
}

void RemoteRoBSSR::testingMaxFileSize(const uint16_t fileSize) noexcept
{
    ROBSSR_UPLOAD_DATA_SIZE_MAX = static_cast<uint32_t>(fileSize);
    ROBSSR_LAST_UPLOAD_DATA_SIZE_MAX = ROBSSR_UPLOAD_DATA_SIZE_MAX / 2U; 
    ROBSSR_TOTAL_UPLOAD_DATA_SIZE_MAX = ROBSSR_UPLOAD_DATA_SIZE_MAX * 2U + ROBSSR_LAST_UPLOAD_DATA_SIZE_MAX;
}
}
