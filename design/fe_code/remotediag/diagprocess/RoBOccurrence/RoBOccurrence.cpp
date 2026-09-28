#include "utils/CommonUtils.h"
#include "RoBOccurrence.h"
#include "diagprocess/RoBMonitoring/RoBMonitoring.h"

namespace rdgapp {

RoBOccurrence *RoBOccurrence::mRoBOccurrence{nullptr};
RoBOccurrence::RoBOccurrence(const Remotediag &app, android::sp<sl::SLLooper> &privateLooper)
    : android::RefBase(), mApp(app), mTimerHandler(*this), mIgOnCheckTimer(&mTimerHandler, TimerHandler::IG_ON_STATE_MAINTAINED_CHECK_ID), mIsRobSsrProcessing(false), mIsDirectCommandProcessing(false)
{
    mTriggerType = DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN;
    mRoBOccurrence = this;
    mRoBOccurrenceHandler = new MainHandler(privateLooper, *this);
    mIgOnCheckTimer.setDuration(TimerHandler::ID_IG_ON_STATE_MAINTAINED_CHECK_TIMEOUT, 0U);
    (void)mRoBOccurrenceHandler->obtainMessage(MainHandler::CMD_ROB_OCCURRENCE_INIT)->sendToTarget();
}

RoBOccurrence *RoBOccurrence::getInstance(void)
{
    if (mRoBOccurrence == nullptr)
    {
        LOG_E("RoBOccurrence is not created");
    }
    return mRoBOccurrence;
}

void RoBOccurrence::init(void)
{
    static constexpr char_t PREVIOUS_TRIP_DB_STRUCT[]{"(id INTEGER PRIMARY KEY\
                                                    , collection_condition_id INTEGER\
                                                    , location_latitude INTEGER\
                                                    , location_longitude INTEGER\
                                                    , diag_accqui_time INTEGER\
                                                    , priority INTEGER\
                                                    , target_collection_data BLOB\
                                                )"};
    static constexpr char_t ROBSSR_CRC_DB_STRUCT[]{"(id INTEGER PRIMARY KEY\
                                                , target_address INTEGER\
                                                , occurred_rob_ssr_crc INTEGER\
                                                , time_series_rob_ssr_crc INTEGER\
                                                )"};
    static constexpr char_t ROBSSR_CRC_DB_NAME[]{"robssr_crc"};
    static constexpr char_t PREVIOUS_TRIP_DB_NAME[]{"occurrent_rob_notification"};

    constexpr uint32_t sqlite_rw {static_cast<uint32_t>(SQLITE_OPEN_READWRITE)};
    constexpr uint32_t sqlite_c {static_cast<uint32_t>(SQLITE_OPEN_CREATE)};
    constexpr int32_t timeoutVal{static_cast<int32_t>(sqlite_rw | sqlite_c)};

    mOccurrentRobInfoDB = std::unique_ptr<Database>(new Database(
        PREVIOUS_TRIP_DB_NAME, (std::string(DATA_PATH) + std::string(PREVIOUS_TRIP_DB_NAME)).c_str(), PREVIOUS_TRIP_DB_STRUCT, timeoutVal));
    mRobSsrCrcDB = std::unique_ptr<Database>(new Database(
                                    ROBSSR_CRC_DB_NAME
                                    , (std::string(DATA_PATH) + std::string(ROBSSR_CRC_DB_NAME)).c_str()
                                    , ROBSSR_CRC_DB_STRUCT
                                    , timeoutVal
                                ));
    int32_t ret{mOccurrentRobInfoDB->init()};
    if(ret != SQLITE_OK)
    {
        LOG_E("OccurentRobs DB init failed, reason = %s", Database::errorToString(ret).c_str());
    }
    else
    {
        LOG_I("OccurentRob DB init success");
    }

    ret = mRobSsrCrcDB->init();
    if (ret != SQLITE_OK)
    {
        LOG_E("RobSsr Crc DB init failed, reason = %s", Database::errorToString(ret).c_str());
    }
    else
    {
        LOG_I("RobSsr Crc DB init success");
    }
}

void RoBOccurrence::onReceiveIG(const bool status) const
{
    LOG_I("onReceiveIG status = %d", status);
    (void)mRoBOccurrenceHandler->obtainMessage(MainHandler::CMD_ROB_OCCURRENCE_CHANGE_IG_STATUS, static_cast<int32_t>(status))->sendToTarget();

}

void RoBOccurrence::changeIGStatus(const bool status)
{
    if (status == false)
    {
        (void)mRoBOccurrenceHandler->obtainMessage(MainHandler::CMD_ROB_OCCURRENCE_IG_OFF)->sendToTarget();
    }
    else
    {
        mIgOnCheckTimer.start();
    }
}

void RoBOccurrence::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{

    // RDG30-R-0094
    if ((DiagManagerAdapter::getInstance()->getRDGFlag() == 1U) 
        && (DiagManagerAdapter::getInstance()->getAllUploadConsent() == true)
        && (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER)))
    {
        LOG_D("All conditions are met -> executes Occurrence RoB Detection");
        const uint8_t protocolType {responseEventInfo->resInfo()->protocolType()};
        const uint32_t targetAddress {CommonUtils::calTargetAddressFromCanIdRx(protocolType, responseEventInfo->resInfo()->canInfo()->canId())};

        const android::sp<sl::Message> msg {mRoBOccurrenceHandler->obtainMessage(MainHandler::CMD_ROB_OCCURRENCE_START_UP, udsResponse)};
        if(targetAddress <= 2147483647U)
        {
            msg->arg1 = static_cast<int32_t>(targetAddress);
        }else{
            LOG_E("Can not cast value of targetAddress");
        }
        
        (void)msg->sendToTarget();
    }
    else
    {
        // RDG30-R-0710
        LOG_D("Abort executes Occurrence RoB Detection because conditions not met");
    }
}

uint8_t RoBOccurrence::getAppId(void) const noexcept
{
    return APP_ID;
}

RoBOccurrence::MainHandler::MainHandler(android::sp<sl::SLLooper> &privateLooper, RoBOccurrence &RoBOccurrence) noexcept : sl::Handler(privateLooper), mRoBOccurrence(RoBOccurrence)
{}

void RoBOccurrence::MainHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
{
    const int32_t what{handlemsg->what};
    switch (what)
    {
    case CMD_ROB_OCCURRENCE_INIT:
    {
        LOG_I("CMD_ROB_OCCURRENCE_INIT");
        {
            mRoBOccurrence.init();
        }
        break;
    }
    case CMD_ROB_OCCURRENCE_START_UP:
    {
        LOG_I("CMD_ROB_OCCURRENCE_START_UP");
        android::sp<UdsMessage> udsResponse{nullptr};
        handlemsg->getObject(udsResponse);
        if(handlemsg->arg1 >= 0)
        {
            mRoBOccurrence.handleOccurrentRobDetection(static_cast<uint32_t>(handlemsg->arg1), udsResponse);
        }else{
            LOG_E("Can not cast msg->arg1 to uint32_t"); 
        }
        
    }
    break;
    case CMD_ROB_OCCURRENCE_IG_OFF:
    {
        LOG_D("CMD_ROB_OCCURRENCE_IG_OFF");
        mRoBOccurrence.handleIgOffEvent();
        break;
    }
    case CMD_ROB_OCCURRENCE_PREVIOUS_TRIP:
    {
        LOG_D("CMD_ROB_OCCURRENCE_PREVIOUS_TRIP");
        mRoBOccurrence.handleOccurrentRobPreviousTrip();
        break;
    }
    case CMD_ROB_OCCURRENCE_CHANGE_IG_STATUS:
    {
        LOG_D("CMD_ROB_OCCURRENCE_CHANGE_IG_STATUS");
        mRoBOccurrence.changeIGStatus((handlemsg->arg1 != 0) ? true : false);
        break;
    }
    default:
        break;
    }
}

void RoBOccurrence::handleOccurrentRobDetection(const uint32_t targetAddress, const android::sp<UdsMessage> udsResponse)
{
    LOG_I("handleOccurrentRobDetection, TX CanID = 0x%02X, sid = 0x%02X, uds data size = %d", targetAddress, udsResponse->getSID(), udsResponse->ToUdsData()->size());
    if ((udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER ))
        && (udsResponse->ToUdsData()->size() >= SID22_PAYLOAD_SIZE_AT_LEAST))
    {
        // RDG30-R-1243
        int64_t current_time {ParamsDef::getCurrentAcquisiteTime()};
        if((current_time < static_cast<int64_t>(0x00)) || (current_time > static_cast<int64_t>(UINT32_MAX))) {
            /*RDG30-R-0063*/
            LOG_D("Time information out of range");
            current_time = 0;
        }
        else
        {
            LOG_D("Time information valid");
        }
        LOG_D("Save Diagnostics Acquisition Time: %lld", current_time);
        /* Get location*/
        // RDG30-R-0052
        const android::sp<CommonDefine::RDGLocationData> loc{LocationManagerAdapter::getInstance()->getLocationData()};
        LOG_D("Store location information, 0x%02x 0x%02x", loc->getLatitude(), loc->getLongtitude());
        // Get current Occurrent RoB monitoring information List
        TargetCollectionDataOccurrentRobList currentRobMonitoringList{};
        Uint64 collectionConditionId {0U};
        const android::sp<OccurrentRobNotification> occurrentRobNotificationVal {new OccurrentRobNotification()};
        const error_t error {RoBMonitoring::getInstance()->getRobMonitoringList(currentRobMonitoringList, collectionConditionId)};
        if((error == E_OK) && (udsResponse->ToUdsData()->data() != nullptr))
        {
            const uint16_t did {static_cast<uint16_t>(static_cast<uint32_t>(udsResponse->ToUdsData()->data()[SID22_DID_BYTE_MASK]) << 8U)
                                    | static_cast<uint16_t>(udsResponse->ToUdsData()->data()[SID22_DID_BYTE_MASK + 1U])};
            const uint32_t occurrenceRob {static_cast<uint32_t>(static_cast<uint32_t>(udsResponse->ToUdsData()->data()[SID22_USER_DEFINED_DTC]) << 16U)
                                            | static_cast<uint32_t>(static_cast<uint32_t>(udsResponse->ToUdsData()->data()[SID22_USER_DEFINED_DTC + 1U]) << 8U)
                                            | static_cast<uint32_t>(udsResponse->ToUdsData()->data()[SID22_USER_DEFINED_DTC + 2U])};
            const uint32_t memSelection {static_cast<uint32_t>(udsResponse->ToUdsData()->data()[SID22_MEMORY_SELECTION])};
            LOG_D("Occurrence RoB Detected, ECU target address: 0x%02x", targetAddress);
            LOG_D("                         DID: 0x%02x", did);
            LOG_D("                         OccurrenceRob: 0x%02x", occurrenceRob);
            LOG_D("                         MemorySelection: 0x%02x", memSelection);
            TargetCollectionDataOccurrentRobIter it {currentRobMonitoringList.begin()};
            for(; it != currentRobMonitoringList.end(); it++)
            {
                const uint32_t currTargetAddress {it->ecu_address_information().target_address()};
                if (currTargetAddress == targetAddress)
                {
                    // RDG30-R-0524
                    if ((did == OCCURRENT_ROB_DID) && 
                        (PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_ON))
                    {
                        RobInformationOccurrentRobIter robMonitorIt {it->rob_information().begin()};

                        for (; robMonitorIt != it->rob_information().end(); robMonitorIt++)
                        {
                            if ((memSelection == robMonitorIt->memory_selection()) && (occurrenceRob == robMonitorIt->rob()))
                            {
                                // RDG30-R-0524
                                LOG_D("All conditions are met. Collected RoBSSR of the detected Occurrence RoBs");
                                // Clone base of Occurrence RoB monitoring information from center
                                // RDG30-R-0892
                                // RDG30-R-0893
                                occurrentRobNotificationVal->setCollectionConditionId(collectionConditionId);
                                occurrentRobNotificationVal->setTriggerLocation(loc);
                                occurrentRobNotificationVal->setTriggerTime(current_time);
                                occurrentRobNotificationVal->getTargetCollectionDataRobSsr()->mutable_ecu_address_information()->CopyFrom(it->ecu_address_information());
                                const CollectionCondition::RobInformationOccurrentRobPriority robPrio{robMonitorIt->rob_priority()};
                                if (robPrio == CollectionCondition::RobInformationOccurrentRobPriority::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_RobPriority_RP_PRIORITY_LOW )
                                {
                                    LOG_D("treated as a Occurrence RoB notification (low priority).");
                                    occurrentRobNotificationVal->setPriority(static_cast<int32_t>(robPrio));
                                } 
                                else if (robPrio == CollectionCondition::RobInformationOccurrentRobPriority::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_RobPriority_RP_PRIORITY_HIGH) {
                                    LOG_D("treated as a Occurrence RoB notification (high priority).");
                                    occurrentRobNotificationVal->setPriority(static_cast<int32_t>(robPrio));
                                }
                                else {
                                    // Do nothing
                                }

                                CollectionCondition::RobInformation* const robInfo {occurrentRobNotificationVal->getTargetCollectionDataRobSsr()->mutable_rob_information()};
                                robInfo->set_rob(occurrenceRob);
                                robInfo->set_memory_selection(memSelection);

                                if ((robMonitorIt->rob_frame_numbers().size() == 1) && (robMonitorIt->rob_frame_numbers(0) == 0U))
                                {
                                    // RDG30-R-1259
                                    robMonitorIt->mutable_rob_frame_numbers()->Clear();
                                    LOG_D("Only one RoBFrameNumber with a value of 0 is specified -> treate as not specified");
                                }
                                else
                                {
                                    robInfo->mutable_rob_frame_information()->mutable_rob_frame_numbers()->CopyFrom(robMonitorIt->rob_frame_numbers());
                                }

                                const std::unordered_map<uint32_t, std::shared_ptr<CrcInformation>>::iterator crcInfo {mCrcList.find(targetAddress)};
                                if (crcInfo != mCrcList.end())
                                {
                                    robInfo->mutable_crc_information()->CopyFrom(*(crcInfo->second));
                                }

                                DiagnosticsCommandIter directCmdIt {robMonitorIt->diagnostics_commands().begin()};
                                for(; directCmdIt != robMonitorIt->diagnostics_commands().end(); directCmdIt++)
                                {
                                    if ( targetAddress == directCmdIt->ecu_address_information().target_address())
                                    {
                                        DirectCommand* const directCmd {occurrentRobNotificationVal->directCommands()->Add()};
                                        LOG_D("Add directCommand of targetAddress = 0x%02x", directCmdIt->ecu_address_information().target_address());
                                        
                                        directCmd->mutable_ecu_address_information()->CopyFrom(directCmdIt->ecu_address_information());
                                        directCmd->set_command_messages(directCmdIt->command_message());
                                    }
                                }

                                mOccurrentRobsQueue.push_back(occurrentRobNotificationVal);
                                DiagManagerAdapter::getInstance()->selfDiagEcuUserDefMemoryDTC(occurrentRobNotificationVal);
                                (void)RemotediagHandler::getInstance_2()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR, occurrentRobNotificationVal)->sendToTarget();
                                mIsRobSsrProcessing = true;
                                LOG_D("handleOccurrentRobDetection done");
                            } else {
                                LOG_D("All conditions aren' met. No processing");
                            }
                        }
                    }
                    else
                    {
                        // RDG30-R-0895
                        LOG_D("All conditions aren't met. No processing");
                    }
                }
                else
                {
                    // RDG30-R-0895
                    LOG_D("All conditions aren't met. No processing");
                }
            }
        }
        else
        {
            LOG_E("No Occurrent RoB monitoring list");
        }
    } else {
        LOG_E("Invalid USD data");
    }
}

void RoBOccurrence::handleIgOffEvent()
{
    // RDG30-R-0525
    if ((mIsRobSsrProcessing == true) || (mIsDirectCommandProcessing == true))
    {
        LOG_D("IG-OFF occurs during collection of RoBSSR or the InternalDirectCommand function");
        LOG_D("Save Occurrent RoB notification data to EMMC");
    }

    (void)saveOccurrentRob();
}

void RoBOccurrence::handleOccurrentRobPreviousTrip()
{
    // RDG30-R-0525
    const IG_STATUS igstatus {PowerManagerAdapter::getInstance()->getIgnitionStatus()};
    if (igstatus == IG_STATUS::IG_STATUS_ON)
    {
        const int32_t ret {restoreOccurrentRob()};
        if (ret == SQLITE_OK)
        {
            // Get current Occurrent RoB monitoring information List
            TargetCollectionDataOccurrentRobList currentRobMonitoringList{};
            Uint64 collectionConditionId {0U};
            const error_t error {RoBMonitoring::getInstance()->getRobMonitoringList(currentRobMonitoringList, collectionConditionId)};
            if(error == E_OK)
            {
                for (android::sp<OccurrentRobNotification> &occurrentRob : mOccurrentRobsQueue)
                {
                    const uint32_t targetAddress {occurrentRob->getTargetCollectionDataRobSsr()->ecu_address_information().target_address()};
                    const uint32_t rob {occurrentRob->getTargetCollectionDataRobSsr()->rob_information().rob()};
                    const uint32_t memSelection {occurrentRob->getTargetCollectionDataRobSsr()->rob_information().memory_selection()};
                    LOG_D("Occurrence RoB info, ECU target address: 0x%02x", targetAddress);
                    LOG_D("                     OccurrenceRob: 0x%02x", rob);
                    LOG_D("                     MemorySelection: 0x%02x", memSelection);
                    TargetCollectionDataOccurrentRobIter it {currentRobMonitoringList.begin()};
                    for(; it != currentRobMonitoringList.end(); it++)
                    {
                        const uint32_t currTargetAddress {it->ecu_address_information().target_address()};
                        if (currTargetAddress == targetAddress)
                        {
                            RobInformationOccurrentRobIter robMonitorIt{it->rob_information().begin()};

                            for (; robMonitorIt != it->rob_information().end(); robMonitorIt++)
                            {
                                if ((memSelection == robMonitorIt->memory_selection()) && (rob == robMonitorIt->rob()))
                                {
                                    // RDG30-R-0524
                                    LOG_D("All conditions are met. Collected RoBSSR of the detected Occurrence RoBs");

                                    DiagnosticsCommandIter directCmdIt {robMonitorIt->diagnostics_commands().begin()};
                                    for(; directCmdIt != robMonitorIt->diagnostics_commands().end(); directCmdIt++)
                                    {
                                        if ( targetAddress == directCmdIt->ecu_address_information().target_address())
                                        {
                                            DirectCommand* const directCmd {occurrentRob->directCommands()->Add()};
                                            LOG_D("Add directCommand of targetAddress = 0x%02x", directCmdIt->ecu_address_information().target_address());
                                            
                                            directCmd->mutable_ecu_address_information()->CopyFrom(directCmdIt->ecu_address_information());
                                            directCmd->set_command_messages(directCmdIt->command_message());
                                        }
                                    }

                                    (void)RemotediagHandler::getInstance_2()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR, occurrentRob)->sendToTarget();
                                    mIsRobSsrProcessing = true;
                                }
                            }
                        }
                        else
                        {
                            // RDG30-R-0895
                            LOG_D("All conditions aren't met. No processing");
                        }
                    }
                }
            }
        }
        else
        {
            LOG_D("No Occurrent RoB monitoring list");
        }
    }
}

OccurrentRobNotification::OccurrentRobNotification() 
    : android::RefBase()
    , mTriggerType(DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
    , mCollectionConditionId(0U)
    , mPriority(0)
    , mTriggerTimestamp(0)
{
}


int32_t RoBOccurrence::saveOccurrentRob(void)
{
    LOG_D("saveOccurrentRob, queue size = %d, mCrcList = %d", mOccurrentRobsQueue.size(), mCrcList.size());
    (void)mOccurrentRobInfoDB->clear();
    (void)mRobSsrCrcDB->clear();
    int32_t ret {SQLITE_ERROR};
    for (android::sp<OccurrentRobNotification>& occurrentRob : mOccurrentRobsQueue)
    {
        ret = mOccurrentRobInfoDB->saveOccurrentRobNotification(occurrentRob);
    }
    // RDG30-R-0714
    std::unordered_map<uint32_t, std::shared_ptr<CrcInformation>>::iterator crcIt {mCrcList.begin()};
    for (; crcIt != mCrcList.end(); crcIt++)
    {
        ret = mRobSsrCrcDB->saveRoBSsrCrcInfo(crcIt->first
                                            , crcIt->second->occurred_rob_ssr_crc()
                                            , crcIt->second->time_series_rob_ssr_crc());
    }
    return ret;
}

int32_t RoBOccurrence::restoreOccurrentRob(void)
{
    mOccurrentRobsQueue.clear();
    int32_t ret {mOccurrentRobInfoDB->getAllOccurrentRobNotification(mOccurrentRobsQueue)};
    if (ret != SQLITE_OK)
    {
        LOG_D("restore OccurrentRob failed, reason = %s", Database::errorToString(ret).c_str());
        goto exit;
    }

    ret = mRobSsrCrcDB->getAllRoBSsrCrcInfo(mCrcList);
    if (ret != SQLITE_OK)
    {
        LOG_D("restore crc list failed, reason = %s", Database::errorToString(ret).c_str());
        goto exit;
    }

    LOG_D("restoreOccurrentRob success, previous Occurrent Robs size: %d, crcList size: ", mOccurrentRobsQueue.size(), mCrcList.size());

exit:
    return ret;
}

void RoBOccurrence::onRobSsrAcquisitionCompleted(const bool result, const uint32_t occurredrRobSsrCRC, const uint32_t timeSeriesRobSsrCRC)
{
    mIsRobSsrProcessing = false;
    if (result == true)
    {
        mIsDirectCommandProcessing = true;
        // RDG30-R-0896
        (void)RemotediagHandler::getInstance_2()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_OCCURRENT_ROB_DETECTION_DIRECT_COMMAND, mOccurrentRobsQueue.front())->sendToTarget();
        const uint32_t currentTargetAddress {mOccurrentRobsQueue.front()->getTargetCollectionDataRobSsr()->ecu_address_information().target_address()};
        const std::shared_ptr<CrcInformation> crcInfo {std::shared_ptr<CrcInformation>(new CrcInformation())};
        crcInfo->set_occurred_rob_ssr_crc(occurredrRobSsrCRC);
        crcInfo->set_time_series_rob_ssr_crc(timeSeriesRobSsrCRC);
        mCrcList[currentTargetAddress] = crcInfo;
        LOG_D("RobSsr Acquisition Completed. Saved CRC values");
    }
    else
    {
        mOccurrentRobsQueue.pop_front();
        mIsRobSsrProcessing = true;
        (void)RemotediagHandler::getInstance_2()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR, mOccurrentRobsQueue.front())->sendToTarget();
    }
}

void RoBOccurrence::onRobSsrAcquisitionCompleted_CenterRequest(const uint32_t ecuTargetAddress, const uint32_t occurredrRobSsrCRC, const uint32_t timeSeriesRobSsrCRC)
{
    const std::shared_ptr<CrcInformation> crcInfo {std::shared_ptr<CrcInformation>(new CrcInformation())};
    crcInfo->set_occurred_rob_ssr_crc(occurredrRobSsrCRC);
    crcInfo->set_time_series_rob_ssr_crc(timeSeriesRobSsrCRC);
    mCrcList[ecuTargetAddress] = crcInfo;
}

void RoBOccurrence::onDirectCommandCompleted(const bool result)
{
    NOTUSED(result);
    LOG_D("onDirectCommandCompleted, remove notification Data");
    mOccurrentRobsQueue.pop_front();
    mIsDirectCommandProcessing = false;
    if (mOccurrentRobsQueue.size() > 0U)
    {
        mIsRobSsrProcessing = true;
        (void)RemotediagHandler::getInstance_2()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR, mOccurrentRobsQueue.front())->sendToTarget();
    }
}

void RoBOccurrence::onServiceFlagChange()
{
    // RDG30-R-1126
    LOG_D("RDG service flag is OFF. clear CRC values");
    (void)mRobSsrCrcDB->clear();
    mCrcList.clear();
}
}
