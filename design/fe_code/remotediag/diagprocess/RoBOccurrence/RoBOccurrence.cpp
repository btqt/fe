#include "utils/CommonUtils.h"
#include "RoBOccurrence.h"
#include "diagprocess/RoBMonitoring/RoBMonitoring.h"

namespace rdgapp {

constexpr char_t RoBOccurrence::PREVIOUS_TRIP_DB_STRUCT[];
constexpr char_t RoBOccurrence::ROBSSR_CRC_DB_STRUCT[];
constexpr char_t RoBOccurrence::ROBSSR_CRC_DB_NAME[];
constexpr char_t RoBOccurrence::PREVIOUS_TRIP_DB_NAME[];

RoBOccurrence *RoBOccurrence::mRoBOccurrence{nullptr};
RoBOccurrence::RoBOccurrence(const Remotediag &app, android::sp<sl::SLLooper> &privateLooper)
    : android::RefBase(), mApp(app), mTimerHandler(*this), mIgOnCheckTimer(&mTimerHandler, TimerHandler::IG_ON_STATE_MAINTAINED_CHECK_ID), mIsAbortOccurrenceEvent(false)
{
    mTriggerType = DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN;
    mRoBOccurrence = this;
    mRoBOccurrenceHandler = new MainHandler(privateLooper, *this);
    mIgOnCheckTimer.setDuration(TimerHandler::ID_IG_ON_STATE_MAINTAINED_CHECK_TIMEOUT, 0U);

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
        ret = mRobSsrCrcDB->getAllRoBSsrCrcInfo(mCrcList);
        if (ret != SQLITE_OK)
        {
            LOG_D("Restore CRC list failed, reason = %s", Database::errorToString(ret).c_str());
        } else {
            LOG_D("Restore CRC list success, crcList size: %u", mCrcList.size());
        }
    }
}

RoBOccurrence *RoBOccurrence::getInstance(void)
{
    if (mRoBOccurrence == nullptr)
    {
        LOG_E("RoBOccurrence is not created");
    }
    return mRoBOccurrence;
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

void RoBOccurrence::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) noexcept
{
    (void)responseEventInfo;
    (void)udsResponse;
}

void RoBOccurrence::onOccurrenceRoBReceived(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
    // RDG30-R-0094
    if ((mIsAbortOccurrenceEvent == false)
        && (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER))
        && (mApp.getUnderRepair() == 0U) // Table 10-17: not under repair --> running
        && (DiagManagerAdapter::getInstance()->getAllUploadConsent() == true)
        && (DiagManagerAdapter::getInstance()->getRDGFlag() == 1U))
    {
        LOG_D("All conditions are met -> executes Occurrence RoB Detection");
        const uint8_t protocolType {responseEventInfo->resInfo()->protocolType()};
        const uint32_t targetAddress {CommonUtils::calTargetAddressFromCanIdRx(protocolType, responseEventInfo->resInfo()->canInfo()->canId())};

        const android::sp<sl::Message> msg {mRoBOccurrenceHandler->obtainMessage(MainHandler::CMD_ROB_OCCURRENCE_DETECTING_ROB_EVENT, udsResponse)};
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
    case CMD_ROB_OCCURRENCE_DETECTING_ROB_EVENT:
    {
        LOG_I("CMD_ROB_OCCURRENCE_DETECTING_ROB_EVENT");
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
    case CMD_STOP_RDG:
    {
        mRoBOccurrence.handleStopRDG(handlemsg->arg1);
        break;
    }
    default:
        break;
    }
}

void RoBOccurrence::handleOccurrentRobDetection(const uint32_t targetAddress, const android::sp<UdsMessage> udsResponse)
{
    LOG_I("handleOccurrentRobDetection, TX CanID = 0x%X, sid = 0x%X, uds data size = %u", targetAddress, udsResponse->getSID(), udsResponse->ToUdsData()->size());
    if ((mIsAbortOccurrenceEvent == false)
        && (udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER ))
        && (udsResponse->ToUdsData()->size() >= SID22_PAYLOAD_SIZE_AT_LEAST))
    {
        // Get current Occurrent RoB monitoring information List
        TargetCollectionDataOccurrentRobList currentRobMonitoringList{};
        Uint64 collectionConditionId {0U};
        const error_t error {RoBMonitoring::getInstance()->getRobMonitoringList(currentRobMonitoringList, collectionConditionId)};
        const std::shared_ptr<CollectionConditionRobRobSsrDidEvent> aCoCoRobRobSsrDidEvent {CollectionCondition::getInstance().getCollectionConditionRobRobSsrDidEvent()};
        if((error == E_OK) && (udsResponse->ToUdsData()->data() != nullptr) && (aCoCoRobRobSsrDidEvent != nullptr))
        {
            const uint16_t did {static_cast<uint16_t>(static_cast<uint16_t>(static_cast<uint32_t>(udsResponse->ToUdsData()->data()[SID22_DID_BYTE_MASK]) << 8U)
                                    | static_cast<uint16_t>(udsResponse->ToUdsData()->data()[SID22_DID_BYTE_MASK + 1U]))};
            TargetCollectionDataOccurrentRobList aTargetCollectionList {};
            aTargetCollectionList.CopyFrom(aCoCoRobRobSsrDidEvent->target_collection_data());
            if ((did == OCCURRENT_ROB_DID) && (aTargetCollectionList.size() > 0))
            {
                // RDG30-R-1243
                int64_t current_time {CommonUtils::getCurrentAcquisiteTime()};
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
                LOG_D("Store location information, 0x%lX 0x%lX", loc->getLatitude(), loc->getLongtitude());

                const uint32_t occurrenceRob {static_cast<uint32_t>(static_cast<uint32_t>(udsResponse->ToUdsData()->data()[SID22_USER_DEFINED_DTC]) << 16U)
                                                | static_cast<uint32_t>(static_cast<uint32_t>(udsResponse->ToUdsData()->data()[SID22_USER_DEFINED_DTC + 1U]) << 8U)
                                                | static_cast<uint32_t>(udsResponse->ToUdsData()->data()[SID22_USER_DEFINED_DTC + 2U])};
                const uint32_t memSelection {static_cast<uint32_t>(udsResponse->ToUdsData()->data()[SID22_MEMORY_SELECTION])};
                LOG_D("Occurrence RoB Detected, ECU target address: 0x%X", targetAddress);
                LOG_D("                         DID: 0x%X", did);
                LOG_D("                         OccurrenceRob: 0x%X", occurrenceRob);
                LOG_D("                         MemorySelection: 0x%X", memSelection);
                TargetCollectionDataOccurrentRobIter it {currentRobMonitoringList.begin()};
                for(; it != currentRobMonitoringList.end(); it++)
                {
                    const uint32_t currTargetAddress {it->ecu_address_information().target_address()};
                    if (currTargetAddress == targetAddress)
                    {
                        // RDG30-R-0524
                        if (PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_ON)
                        {
                            TargetCollectionDataOccurrentRobIter aTargetCollectionIt {aTargetCollectionList.begin()};
                            for(; aTargetCollectionIt != aTargetCollectionList.end(); aTargetCollectionIt++)
                            {
                                if (aTargetCollectionIt->has_ecu_address_information())
                                {
                                    const EcuAddressInformation& aEcuAddressInfo {aTargetCollectionIt->ecu_address_information()};
                                    const RobInformationOccurrentRobList& aRobInformationOccurrentRobList {aTargetCollectionIt->rob_information()};
                                    const uint32_t cocoTargetAddress {aEcuAddressInfo.target_address()};

                                    if (cocoTargetAddress == targetAddress)
                                    {
                                        RobInformationOccurrentRobIter robMonitorIt {aRobInformationOccurrentRobList.begin()};

                                        for (; robMonitorIt != aRobInformationOccurrentRobList.end(); robMonitorIt++)
                                        {
                                            if ((memSelection == robMonitorIt->memory_selection()) && (occurrenceRob == robMonitorIt->rob()))
                                            {
                                                // RDG30-R-0524
                                                LOG_D("All conditions are met. Collected RoBSSR of the detected Occurrence RoBs");
                                                // Clone base of Occurrence RoB monitoring information from center
                                                // RDG30-R-0892
                                                // RDG30-R-0893
                                                const android::sp<OccurrentRobNotification> occurrentRobNotificationVal {new OccurrentRobNotification()};
                                                occurrentRobNotificationVal->setId(CommonUtils::generateRandomUint64());
                                                occurrentRobNotificationVal->setCollectionConditionId(collectionConditionId);
                                                occurrentRobNotificationVal->setTriggerLocation(loc);
                                                occurrentRobNotificationVal->setTriggerTime(current_time);
                                                occurrentRobNotificationVal->mutableTargetCollectionDataRobSsr()->mutable_ecu_address_information()->CopyFrom(it->ecu_address_information());
                                                const CollectionCondition::RobInformationOccurrentRobPriority robPrio{robMonitorIt->rob_priority()};
                                                if (robPrio == CollectionCondition::RobInformationOccurrentRobPriority::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_RobPriority_RP_PRIORITY_LOW )
                                                {
                                                    LOG_D("treated as a Occurrence RoB notification (low priority).");
                                                    occurrentRobNotificationVal->setPriority(ParamsDef::OCCURRENT_ROB_NOTIFICATION_LOW_PRIORITY);
                                                } 
                                                else if (robPrio == CollectionCondition::RobInformationOccurrentRobPriority::GetCollectionConditionResponse_CollectionConditionRobRobSsrDidEvent_TargetCollectionData_RobInformation_RobPriority_RP_PRIORITY_HIGH) {
                                                    LOG_D("treated as a Occurrence RoB notification (high priority).");
                                                    occurrentRobNotificationVal->setPriority(ParamsDef::OCCURRENT_ROB_NOTIFICATION_HIGH_PRIORITY);
                                                }
                                                else {
                                                    // Do nothing
                                                }

                                                CollectionCondition::RobInformation* const robInfo {occurrentRobNotificationVal->mutableTargetCollectionDataRobSsr()->mutable_rob_information()};
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
                                                    const google::protobuf::RepeatedField<google::protobuf::uint32>& robFrameNumbersInput {robMonitorIt->rob_frame_numbers()};
                                                    google::protobuf::RepeatedField<google::protobuf::uint32>::const_iterator robFrameNumberIt {robFrameNumbersInput.cbegin()};
                                                    while(robFrameNumberIt != robFrameNumbersInput.cend())
                                                    {
                                                        LOG_D("Add robFrameNumber = %u", *robFrameNumberIt);
                                                        occurrentRobNotificationVal->getRobFrameNumbers()->push_back(*robFrameNumberIt);
                                                        robFrameNumberIt++;
                                                    }
                                                    
                                                }

                                                DiagnosticsCommandIter directCmdIt {robMonitorIt->diagnostics_commands().begin()};
                                                for(; directCmdIt != robMonitorIt->diagnostics_commands().end(); directCmdIt++)
                                                {
                                                    DirectCommand* const directCmd {occurrentRobNotificationVal->directCommands()->Add()};
                                                    LOG_D("Add directCommand of targetAddress = 0x%X", directCmdIt->ecu_address_information().target_address());
                                                    
                                                    directCmd->mutable_ecu_address_information()->CopyFrom(directCmdIt->ecu_address_information());
                                                    directCmd->set_command_messages(directCmdIt->command_message());
                                                }

                                                addNotification(occurrentRobNotificationVal);
                                                const android::sp<sl::Message> msg { RemotediagHandler::getInstance_2()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR, occurrentRobNotificationVal)};
                                                (void)RemotediagHandler::getInstance_2()->sendMessageDelayed(msg, 100U);
                                                DiagManagerAdapter::getInstance()->selfDiagEcuUserDefMemoryDTC(occurrentRobNotificationVal);
                                                LOG_D("handleOccurrentRobDetection done");
                                            } else {
                                                LOG_D("All conditions aren' met. No processing");
                                            }
                                        }
                                    }
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
                        LOG_E("target not found in monitoring list");
                    }
                }
            } else {
                LOG_E("No TargetCollectionData");
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
    LOG_D("IG-OFF occurs during collection of RoBSSR or the InternalDirectCommand function");
    LOG_D("Save Occurrent RoB notification data to EMMC");

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
            const std::shared_ptr<CollectionConditionRobRobSsrDidEvent> aCoCoRobRobSsrDidEvent {CollectionCondition::getInstance().getCollectionConditionRobRobSsrDidEvent()};
            if((error == E_OK) && (aCoCoRobRobSsrDidEvent != nullptr))
            {
                TargetCollectionDataOccurrentRobList aTargetCollectionList {};
                aTargetCollectionList.CopyFrom(aCoCoRobRobSsrDidEvent->target_collection_data());

                if (aTargetCollectionList.size() > 0) {
                    for (OccurrentRobNotificationIter notificationIt {mNotificationList.begin()}; (notificationIt != mNotificationList.cend()); notificationIt++)
                    {
                        if (notificationIt->second->getIsRecoveryRoB() == true)
                        {
                            notificationIt->second->setIsRecoveryRoB(false);
                            const uint32_t targetAddress {notificationIt->second->getTargetCollectionDataRobSsr().ecu_address_information().target_address()};
                            const uint32_t rob {notificationIt->second->getTargetCollectionDataRobSsr().rob_information().rob()};
                            const uint32_t memSelection {notificationIt->second->getTargetCollectionDataRobSsr().rob_information().memory_selection()};
                            LOG_I("Occurrence RoB info, ECU target address: 0x%X", targetAddress);
                            LOG_I("                     OccurrenceRob: 0x%X", rob);
                            LOG_I("                     MemorySelection: 0x%X", memSelection);
                            TargetCollectionDataOccurrentRobIter it {currentRobMonitoringList.begin()};
                            for(; it != currentRobMonitoringList.end(); it++)
                            {
                                const uint32_t currTargetAddress {it->ecu_address_information().target_address()};
                                if (currTargetAddress == targetAddress)
                                {

                                    TargetCollectionDataOccurrentRobIter aTargetCollectionIt {aTargetCollectionList.begin()};
                                    for(; aTargetCollectionIt != aTargetCollectionList.end(); aTargetCollectionIt++)
                                    {
                                        if (aTargetCollectionIt->has_ecu_address_information())
                                        {
                                            const EcuAddressInformation& aEcuAddressInfo {aTargetCollectionIt->ecu_address_information()};
                                            const RobInformationOccurrentRobList& aRobInformationOccurrentRobList {aTargetCollectionIt->rob_information()};
                                            const uint32_t cocoTargetAddress {aEcuAddressInfo.target_address()};

                                            if (cocoTargetAddress == targetAddress)
                                            {
                                                RobInformationOccurrentRobIter robMonitorIt {aRobInformationOccurrentRobList.begin()};

                                                for (; robMonitorIt != aRobInformationOccurrentRobList.end(); robMonitorIt++)
                                                {
                                                    if ((memSelection == robMonitorIt->memory_selection()) && (rob == robMonitorIt->rob()))
                                                    {
                                                        // RDG30-R-0524
                                                        LOG_D("All conditions are met. Collected RoBSSR of the detected Occurrence RoBs");

                                                        if ((robMonitorIt->rob_frame_numbers().size() == 1) && (robMonitorIt->rob_frame_numbers(0) == 0U))
                                                        {
                                                            // RDG30-R-1259
                                                            robMonitorIt->mutable_rob_frame_numbers()->Clear();
                                                            LOG_D("Only one RoBFrameNumber with a value of 0 is specified -> treate as not specified");
                                                        }
                                                        else
                                                        {
                                                            const google::protobuf::RepeatedField<google::protobuf::uint32>& robFrameNumbersInput {robMonitorIt->rob_frame_numbers()};
                                                            google::protobuf::RepeatedField<google::protobuf::uint32>::const_iterator robFrameNumberIt {robFrameNumbersInput.cbegin()};
                                                            while(robFrameNumberIt != robFrameNumbersInput.cend())
                                                            {
                                                                LOG_D("Add robFrameNumber = %u", *robFrameNumberIt);
                                                                notificationIt->second->getRobFrameNumbers()->push_back(*robFrameNumberIt);
                                                                robFrameNumberIt++;
                                                            }
                                                        }

                                                        DiagnosticsCommandIter directCmdIt {robMonitorIt->diagnostics_commands().begin()};
                                                        for(; directCmdIt != robMonitorIt->diagnostics_commands().end(); directCmdIt++)
                                                        {
                                                            DirectCommand* const directCmd {notificationIt->second->directCommands()->Add()};
                                                            LOG_D("Add directCommand of targetAddress = 0x%X", directCmdIt->ecu_address_information().target_address());
                                                            
                                                            directCmd->mutable_ecu_address_information()->CopyFrom(directCmdIt->ecu_address_information());
                                                            directCmd->set_command_messages(directCmdIt->command_message());
                                                        }
                                                        const android::sp<sl::Message> msg {RemotediagHandler::getInstance_2()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR, notificationIt->second)};
                                                        (void)RemotediagHandler::getInstance_2()->sendMessageDelayed(msg, 100U);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    // RDG30-R-0895
                                    LOG_E("target not found in monitoring list");
                                }
                            }
                        }
                        else
                        {
                            LOG_I("Not from previous trip data");
                        }
                    }
                } else {
                    LOG_E("No TargetCollectionData");
                    (void)mOccurrentRobInfoDB->clear(); // if no TargetCollectionData => previous trip data is not needed anymore
                    (void)mNotificationList.clear(); // clear previous trip data
                }
            } else {
                LOG_E("No collection condition DidEvent data");
                (void)mOccurrentRobInfoDB->clear(); // if COCO data is deleted => previous trip data is not needed anymore
                (void)mNotificationList.clear(); // clear previous trip data
            }
        }
        else
        {
            LOG_E("Get previous trip data from EMMC failed");
        }
    }
}

OccurrentRobNotification::OccurrentRobNotification() 
    : android::RefBase()
    , mTriggerType(DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER)
    , mCollectionConditionId(0U)
    , mNotificationId(0U)
    , mPriority(0)
    , mTriggerTimestamp(0)                                              
{}


int32_t RoBOccurrence::saveOccurrentRob(void)
{
    LOG_D("saveOccurrentRob, queue size = %u, mCrcList = %u", mNotificationList.size(), mCrcList.size());
    (void)mOccurrentRobInfoDB->clear();
    (void)mRobSsrCrcDB->clear();
    int32_t ret {SQLITE_ERROR};
    
    for (OccurrentRobNotificationIter notificationIt {mNotificationList.begin()}; notificationIt != mNotificationList.cend(); notificationIt++)
    {
        ret = mOccurrentRobInfoDB->saveOccurrentRobNotification(notificationIt->second);
    }
    // RDG30-R-0714
    std::unordered_map<uint64_t, std::shared_ptr<CrcInformation>>::iterator crcIt {mCrcList.begin()};
    for (; crcIt != mCrcList.end(); crcIt++)
    {
        ret = mRobSsrCrcDB->saveRoBSsrCrcInfo(crcIt->first
                                            , crcIt->second->occurred_rob_ssr_crc()
                                            , crcIt->second->time_series_rob_ssr_crc());
    }
    mNotificationList.clear();
    return ret;
}



int32_t RoBOccurrence::saveCRC(void)
{
    int32_t ret {SQLITE_ERROR};
    LOG_D("saveCRC, mCrcList = %u", mCrcList.size());
    (void)mRobSsrCrcDB->clear();
    std::unordered_map<uint64_t, std::shared_ptr<CrcInformation>>::iterator crcIt {mCrcList.begin()};
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
    // mNotificationList.clear();
    const int32_t ret {mOccurrentRobInfoDB->getAllOccurrentRobNotification(mNotificationList)};
    if (ret != SQLITE_OK)
    {
        LOG_D("restore OccurrentRob failed, reason = %s", Database::errorToString(ret).c_str());
        goto exit;
    }

    LOG_I("restoreOccurrentRob success, previous Occurrent Robs size: %u", mNotificationList.size());
exit:
    return ret;
}

void RoBOccurrence::onRobSsrAcquisitionCompleted(const bool result, const uint64_t notificationId, const uint64_t transId, const uint32_t occurredrRobSsrCRC, const uint32_t timeSeriesRobSsrCRC)
{
    const android::sp<OccurrentRobNotification> notification {getNotification(notificationId)};
    if (notification != nullptr) 
    {
        if (result == true)
        {
            // RDG30-R-0896
            LOG_D("triggerDirectCommand, notificationId = %llu", notification->getId());
            (void)RemotediagHandler::getInstance_2()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_OCCURRENT_ROB_DETECTION_DIRECT_COMMAND, notification)->sendToTarget();
            const std::shared_ptr<CrcInformation> crcInfo {std::shared_ptr<CrcInformation>(new CrcInformation())};
            crcInfo->set_occurred_rob_ssr_crc(occurredrRobSsrCRC);
            crcInfo->set_time_series_rob_ssr_crc(timeSeriesRobSsrCRC);
            mCrcList[transId] = crcInfo;
            LOG_D("RobSsr Acquisition successfully Completed, notificationId = %llu", notificationId);
            if (saveCRC() == SQLITE_OK)
            {
                LOG_D("Saved CRC values success, transId = 0x%llX, occurredrRobSsrCRC = %u, timeSeriesRobSsrCRC = %u", transId, occurredrRobSsrCRC, timeSeriesRobSsrCRC);
            } else {
                LOG_D("Saved CRC values failed, transId = 0x%llX, occurredrRobSsrCRC = %u, timeSeriesRobSsrCRC = %u", transId, occurredrRobSsrCRC, timeSeriesRobSsrCRC);
            }
        }
        else
        {
            LOG_D("RobSsr Acquisition unsuccessfully completed");
            (void)removeNotification(notificationId);
        }
    }
}

void RoBOccurrence::onRdgStop(const bool isStop) const noexcept {
    //obtain message to stop rdg
    (void)mRoBOccurrenceHandler->obtainMessage(MainHandler::CMD_STOP_RDG, static_cast<int32_t>(isStop))->sendToTarget();
}

void RoBOccurrence::onDirectCommandCompleted(const bool result, const uint64_t notificationId)
{
    NOTUSED(result);
    LOG_D("onDirectCommandCompleted, remove notificationId %llu", notificationId);
    (void)removeNotification(notificationId);
}

void RoBOccurrence::onServiceFlagChange()
{
    // RDG30-R-1126
    LOG_D("RDG service flag is OFF. clear CRC values");
    (void)mRobSsrCrcDB->clear();
    mCrcList.clear();
}

const uint32_t RoBOccurrence::getOccurredCRC(const uint64_t transId)
{
    uint32_t ret {0U};
    const std::unordered_map<uint64_t, std::shared_ptr<CrcInformation>>::iterator crcInfor {mCrcList.find(transId)};
    if (crcInfor != mCrcList.cend())
    {
        ret = crcInfor->second->occurred_rob_ssr_crc();
    }
    LOG_D("getOccurredCRC = %u", ret);
    return ret;
}
const uint32_t RoBOccurrence::getTimeSeriesCRC(const uint64_t transId)
{
    uint32_t ret {0U};
    const std::unordered_map<uint64_t, std::shared_ptr<CrcInformation>>::iterator crcInfor {mCrcList.find(transId)};
    if (crcInfor != mCrcList.cend())
    {
        ret = crcInfor->second->time_series_rob_ssr_crc();
    }
    LOG_D("getTimeSeriesCRC = %u", ret);
    return ret;
}

void RoBOccurrence::addNotification(const android::sp<OccurrentRobNotification>& aNotification)
{
    const android::AutoMutex _l{mLock};
    LOG_D("add notificationid = %llu", aNotification->getId());
    mNotificationList[aNotification->getId()] = aNotification;
}

void RoBOccurrence::handleStopRDG(const int32_t isStop) 
{
    mIsAbortOccurrenceEvent = isStop == 1 ? true : false;
    if(mIsAbortOccurrenceEvent) {
        LOG_I("Stop RoBOccurrence");
    } else {
        LOG_W("Power source enable RDG");
    }
}

const android::sp<OccurrentRobNotification> RoBOccurrence::getNotification(const uint64_t notificationId)
{
    const android::AutoMutex _l{mLock};
    LOG_D("getNotification = %llu", notificationId);
    android::sp<OccurrentRobNotification> notification {nullptr};
    const OccurrentRobNotificationIter it {mNotificationList.find(notificationId)};
    if (it != mNotificationList.end())
    {
        notification = it->second;
        LOG_D("getNotification = %llu success", notification->getId());
    }
    return notification;
}

error_t RoBOccurrence::removeNotification(const uint64_t notificationId)
{
    const android::AutoMutex _l{mLock};
    error_t err {E_ERROR};
    const OccurrentRobNotificationIter it {mNotificationList.find(notificationId)};
    if (it != mNotificationList.end())
    {
        (void)mNotificationList.erase(it);
        LOG_D("remove notification success, notificationId = %llu", notificationId);
        err = E_OK;
    } else {
        LOG_D("remove notification failed, notificationId = %llu not found", notificationId);
    }
    return err;
}

}
