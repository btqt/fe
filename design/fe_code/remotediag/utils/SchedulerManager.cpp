#include "SchedulerManager.h"
#include "SchedulerHdl.h"
#include "SchedulerQueue.h"

namespace rdgapp {

SchedulerManager::SchedulerManager(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper): 
    android::RefBase()
    , mApp(app) 
    , isIgOnRoutineExpired{0U}
    // , mIgOnTimestamp{0}
    {
    LOG_I("Constructor");
    mpSchedulerHdl = new SchedulerHdl(privateLooper, this);
    (void)mpSchedulerHdl->obtainMessage(CMD_INIT_SCHEDULERHDL)->sendToTarget();
    loadComplTimeFromFile();
    applyNewSchedData(true, true);
    /*Check IG status and set timer 70 sec for IG-ON Trigger Routine*/
    mTimerHandler = std::make_shared<TimerHandler>(*this);
    mTickTimer = new Timer(mTimerHandler.get(), TimerHandler::ID_IG_ON_TRIGGER_ROUTINE);
    mTickTimer->setDuration(IG_ON_ROUTINE_TIMER, 0U);
    mTimers.push_back(mTickTimer);
    mTickTimer = new Timer(mTimerHandler.get(), TimerHandler::ID_IG_OFF_PROCESS_EXPIRED);
    mTickTimer->setDuration(IG_OFF_PROCESS_EXPIRED, 0U);
    mTimers.push_back(mTickTimer);
    mpIGStatus = false;
    if(mApp.getIGStatus() == 1U) {
        mpIGStatus = true;
        // mIgOnTimestamp = MS_TO_SEC(TimeManager::getInstance().getCurrentMilliSec());
        LOG_I("IG is ON. Count down for 70 sec");
        mTimers[0]->start();
        setIgStatus(true);
    } else {
        mpIGStatus = false;
        LOG_I("IG is OFF");
    }
}

void SchedulerManager::applyChange() {
    LOG_I("Start applyChange");
    struct ComparePriority {
    bool operator()(const android::sp<SchedulerQueue> a, const android::sp<SchedulerQueue> b) {
        return a->getPrioSchedQue() > b->getPrioSchedQue();
    }
    };
    std::priority_queue<android::sp<SchedulerQueue>, std::vector<android::sp<SchedulerQueue>>, ComparePriority> myQueue{};
    for(ScheduleMapIt mSchedMapIt {mSchedulerMap.begin()}; mSchedMapIt != mSchedulerMap.end(); mSchedMapIt++) {
        myQueue.push(mSchedMapIt->second);
        // const android::sp<SchedulerQueue> mSchedQueue {mSchedMapIt->second};
        // mSchedQueue->readyToStart();
    }
    while(!myQueue.empty()) {
        const android::sp<SchedulerQueue> ptr{myQueue.top()};
        ptr->readyToStart();
        myQueue.pop();
    }
    LOG_I("Finish applyChange");
}

void SchedulerManager::executeSchedIGONRoutine() {
    LOG_I("Start executeSchedIGONRoutine");
    /*Found ST_IG_ON_TRIGGER_ROUTINE queue and check running status => execute*/
    if(mSchedulerMap.empty() != true) {
        LOG_D("mSchedulerMap size: %d", mSchedulerMap.size());
        for (ScheduleMapIt mSchedMapIt {mSchedulerMap.begin()}; mSchedMapIt != mSchedulerMap.end(); mSchedMapIt++) {
            const android::sp<SchedulerQueue> pSchedQueue {mSchedMapIt->second};
            if(pSchedQueue->getFirstSchedType() == Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE) {
                if (nullptr != pSchedQueue.get()){
                    if(pSchedQueue->getIsExecuted() == 0U) {
                        pSchedQueue->executeQueue(false);
                    } else {
                        LOG_I("ST_IG_ON_TRIGGER_ROUTINE was executed");
                    }
                } else {
                    LOG_I("Sched is not ST_IG_ON_TRIGGER_ROUTINE");
                }
            }
        }
    } else {
        LOG_D("mSchedulerMap is empty");
    }
}

void SchedulerManager::onReceiveIG(const bool status) {
    if ((mpIGStatus == false) && (status == true)) {
        // LOG_I("mTickTimer Start");
        LOG_I("SchedulerManager receive IG ON");
        // mIgOnTimestamp = MS_TO_SEC(TimeManager::getInstance().getCurrentMilliSec());
        mTimers[0]->start();
        setIgStatus(status);
    } else if((mpIGStatus == true) && (status == false)) {
        // LOG_I("mTickTimer Stop");
        LOG_I("SchedulerManager receive IG OFF");
        setIgStatus(status);
        // mTickTimer->stop();
    } else {
        LOG_I("Duplicated IG notification");
    }
}

void SchedulerManager::insertToMap(const android::sp<SchedulerTime> SchedTime, uint64_t schedIndex) {
    LOG_I("insertToMap schedIndex: %llu ScheType: %d", schedIndex, SchedTime->getType());

    if (0U == checkSchedMapSize()) {
        android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
        /*Find schedIndex in mLastCompTime*/
        const std::map<uint64_t, int64_t>::iterator it{mLastCompTime.find(schedIndex)};
        if(it != mLastCompTime.end()) {
            newScheduleQueue->setLastOpComplTime(it->second);
        } else {
            LOG_E("Do not have schedIndex: %llu", schedIndex);
        }
        (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
    } else {
        android::sp<SchedulerQueue> mFindQueue{};
        const Rdg_Sched_Type::SchedType tmp_SchedType {SchedTime->getType()};
        if (Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE == tmp_SchedType) {
            // mFindQueue = getSameTypeIGON();
            // if (nullptr == mFindQueue.get()){
            //     android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
            //     (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
            // } else {
            //     (void)mFindQueue->insert(SchedTime);
            // }
            android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
            (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
        } else if (Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_ROUTINE == tmp_SchedType) {
            mFindQueue = getSameSchedType(Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_ROUTINE);
            if (nullptr == mFindQueue.get()){
                android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
                (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
            } else {
                (void)mFindQueue->insert(SchedTime);
            }
        } else if (Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT == tmp_SchedType) {
            mFindQueue = getSameSchedType(Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT);
            if (nullptr == mFindQueue.get()){
                android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
                (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
            } else {
                (void)mFindQueue->insert(SchedTime);
            }
        } else {
            android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
            /*Find schedIndex in mLastCompTime*/
            const std::map<uint64_t, int64_t>::iterator it{mLastCompTime.find(schedIndex)};
            if(it != mLastCompTime.end()) {
                newScheduleQueue->setLastOpComplTime(it->second);
            } else {
                LOG_E("Do not have schedIndex: %llu", schedIndex);
            }
            (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
        }
    }
}

void SchedulerManager::notifySchedComplete(const uint64_t schedIndex, const int64_t completeTime)
{
    /*1. Find schedIndex in saved mSchedulerMap
      2. Check schedType is routine or not
      3. Save completeTime as "last operation completion time"
      Format: sched_routine_schedIndex
    */
    (void)mpSchedulerHdl->obtainMessage(MSG_NOTIFY_SCHED_COMPLETE)->sendToTarget();
    ScheduleMapIt mSchedMapIt{};
    mSchedMapIt = mSchedulerMap.find(schedIndex);
    if(mSchedMapIt != mSchedulerMap.end()) {
        if(mSchedMapIt->second->getFirstSchedType() == Rdg_Sched_Type::SchedType::ST_PERIOD_TRIGGER_ROUTINE) {
            LOG_I("SchedIndex: %llu. Save last operation completion time: %lld", schedIndex, completeTime);
            mSchedMapIt->second->saveLastOpComplTime(completeTime);
        } else {
            /*TBD: detele one shot sched*/
            LOG_I("Sched type is not ST_PERIOD_TRIGGER_ROUTINE");
        }
    } else {
        LOG_I("Dont found sched index: %llu", schedIndex);
    }
}
void SchedulerManager::deteleSchedInMap(const uint64_t schedIndex) {
    ScheduleMapIt mSchedMapIt{};
    mSchedMapIt = mSchedulerMap.find(schedIndex);
    if(mSchedMapIt != mSchedulerMap.end()) {
        LOG_D("Delete schdeIndex: %llu", schedIndex);
        const android::sp<SchedulerQueue> tmp_SchedQueue {mSchedMapIt->second};
        tmp_SchedQueue->stopTime();
        (void)mSchedulerMap.erase(mSchedMapIt);
    } else {
        LOG_D("Don't have schedIndex: %llu in schedManager", schedIndex);
    }

    const std::map<uint64_t, int64_t>::iterator it{mLastCompTime.find(schedIndex)};
    if(it != mLastCompTime.end()) {
        (void)mLastCompTime.erase(it);
        LOG_D("Delete mLastCompTime schedIndex: %llu", schedIndex);
    } else {
        LOG_E("Do not have schedIndex: %llu", schedIndex);
    }
}

void SchedulerManager::clearMap() {
    if(mSchedulerMap.empty() != true) {
        for (ScheduleMapIt mSchedMapIt {mSchedulerMap.begin()}; mSchedMapIt != mSchedulerMap.end(); mSchedMapIt++) {
            const android::sp<SchedulerQueue> mSchedQueue {mSchedMapIt->second};
            mSchedQueue->stopTime();
        }
        mSchedulerMap.clear();
    }
}

uint32_t SchedulerManager::checkSchedMapSize() const noexcept {
    return mSchedulerMap.size();
}

android::sp<SchedulerQueue> SchedulerManager::getSameTypeIGON() {
    LOG_I("getSameTypeIGON");
    android::sp<SchedulerQueue> mReturnQueue {nullptr};
    if(mSchedulerMap.empty() != true) {
        for (ScheduleMapIt mSchedMapIt {mSchedulerMap.begin()}; mSchedMapIt != mSchedulerMap.end(); mSchedMapIt++) {
            bool result {false};
            const android::sp<SchedulerQueue> pSchedQueue {mSchedMapIt->second};
            result = pSchedQueue->isSameIGON();
            if (true == result) {
                mReturnQueue = mSchedMapIt->second;
                break;
            }
        }
    } else {
        mReturnQueue = nullptr;
    }
    return mReturnQueue;
}

android::sp<SchedulerQueue> SchedulerManager::getSameSchedType(const Rdg_Sched_Type::SchedType pSchedType) {
    LOG_I("getSameSchedType");
    android::sp<SchedulerQueue> mReturnQueue {nullptr};
    if(mSchedulerMap.empty() != true) {
        LOG_D("mSchedulerMap size: %d", mSchedulerMap.size());

        for (ScheduleMapIt mSchedMapIt {mSchedulerMap.begin()}; mSchedMapIt != mSchedulerMap.end(); mSchedMapIt++) {
            const android::sp<SchedulerQueue> pSchedQueue {mSchedMapIt->second};
            if(pSchedQueue->getFirstSchedType() == pSchedType) {
                mReturnQueue = mSchedMapIt->second;
                break;
            }
        }
    } else {
        LOG_D("mSchedulerMap is empty");
        mReturnQueue = nullptr;
    }
    return mReturnQueue;
}

void SchedulerManager::saveComplTimeToFile(const uint64_t schedIndex, const int64_t timeData) {
    const string PATH_COMPL_TIME_FILE{"/data/rdg/compltime.dat"};
    LOG_I("saveComplTimeToFile schedIndex: %llu timestamp: %lld", schedIndex, timeData);
    /*Save time to file*/
    LOG_I("Write completeTime to file %s", PATH_COMPL_TIME_FILE.c_str());
    mLastCompTime[schedIndex] = timeData;
    const FileHandleType fileHdl {FileUtil::openFile(PATH_COMPL_TIME_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_BIN)};
    if(fileHdl != nullptr) {
        struct ComplTimePair {
        uint64_t schedIndex;
        int64_t timeStamp;
        }__attribute__((packed));
        std::vector<ComplTimePair> vLastCompTime{};
        for(std::map<uint64_t, int64_t>::iterator it {mLastCompTime.begin()}; it !=mLastCompTime.end(); it++ ) {
            ComplTimePair p;
            p.schedIndex = it->first;
            p.timeStamp = it->second;
            vLastCompTime.push_back(p);
            LOG_I("Check size of pair: %d", sizeof(p));
        }
        LOG_I("Check vLastCompTime size: %d", vLastCompTime.size());
        const uint32_t bufferSize {vLastCompTime.size() * sizeof(ComplTimePair)};
        LOG_I("Check bufferSize: %d", bufferSize);
        uint8_t mBuffer[bufferSize];
        (void)std::memcpy(&mBuffer[0], vLastCompTime.data(), bufferSize);

        const bool success {FileUtil::writeBinToFile(fileHdl, &mBuffer[0], bufferSize)};
        if (success != true) {
            LOG_I("Failed to write to file.");
        }
        (void)FileUtil::closeFile(fileHdl);
        /*TBD: Save RDG flag into DID */
    } else {
        LOG_I("fileHdl is nullptr");
    }
}

void SchedulerManager::loadComplTimeFromFile() {
    const string PATH_COMPL_TIME_FILE{"/data/rdg/compltime.dat"};
    LOG_I("Read Complete Time from file: %s", PATH_COMPL_TIME_FILE.c_str());
    /* Get file size*/
    uint32_t size{0U};
    std::ifstream file{std::ifstream(PATH_COMPL_TIME_FILE.c_str(), std::ios::binary | std::ios::ate)};
    const int64_t tmp_tell{file.tellg()};
    if((tmp_tell>=0) && (tmp_tell <= static_cast<int64_t>(UINT32_MAX)))
    {
        size = static_cast<uint32_t>(tmp_tell);
    }
    else
    {
        // do nothing
    }
    file.close();
    LOG_I("CHECK compltime.dat size: %d", size);
    /* Read file content */
    const FileHandleType fileHdl {FileUtil::openFile(PATH_COMPL_TIME_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN)};
    if(fileHdl != nullptr) {
        uint8_t mBuffer[size];
        const bool success{FileUtil::ReadBinFromFile(fileHdl, &mBuffer[0], size)};
        if(success == true) {
            LOG_I("Read file success");
            for (uint32_t i{0U}; i < size; i += sizeof(uint64_t) + sizeof(int64_t)) {
                uint64_t key {0U};
                (void)std::memcpy(&key, &mBuffer[i], sizeof(uint64_t));
                int64_t value {0};
                (void)std::memcpy(&value, &mBuffer[i + sizeof(uint64_t)], sizeof(int64_t));
                mLastCompTime[key] = value;
                LOG_I("Check ShedIndex: %llu timeStamp: %lld", key, value);
            }
        }
        (void)FileUtil::closeFile(fileHdl);
    } else {
        LOG_I("fileHdl is nullptr");
    }
    LOG_I("End read time complete");
}

void SchedulerManager::executeFunction(const android::sp<SchedulerTime> executeSchedule) {
    LOG_I("executeFunction");
    uint64_t colID{0LLU};
    colID = executeSchedule->getRequestID();
    const Rdg_Sched_Type::SchedFuncType funcType {executeSchedule->getFuncType()};
    const uint8_t messID{static_cast<uint8_t>(funcType)};
    /*TBD: get prio*/
    const uint32_t prio{executeSchedule->getPrio()};
    constexpr DiagTrigger::DiagTriggerType type{DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER};
    const android::sp<CenterReqData> tmp_CenterReqData{new CenterReqData(colID, messID, prio, type)};
    mApp.onScheduleReceived(tmp_CenterReqData);
}

void SchedulerManager::setIgStatus(const bool IGStatus) {
    LOG_I("Start setIgStatus");
    mpIGStatus = IGStatus;
    LOG_I("Set IG Status: %s", mpIGStatus? "TRUE" : "FALSE");
    ScheduleMapIt it{};
    android::sp<SchedulerQueue> pFindQueue {nullptr};

    if(mpIGStatus == true) {
        mTimers[1]->stop();
        for(it = mSchedulerMap.begin(); it != mSchedulerMap.end(); it++) {
            pFindQueue = it->second;
            const Rdg_Sched_Type::SchedType schedType_Dat{pFindQueue->getFirstSchedType()};
            if(schedType_Dat == Rdg_Sched_Type::SchedType::ST_PERIOD_TRIGGER_ROUTINE) {
                /*1. get current time
                  2. Compare currentTime vs (LastOpCompleteTime + DurationTime)
                    a) currentTime >= (LastOpCompleteTime + DurationTime)
                    => start after 90s
                    b) currentTime < (LastOpCompleteTime + DurationTime)
                    => startTime((LastOpCompleteTime + DurationTime) - currentTime)
                */
                int64_t tmp_Duration{0};
                const int64_t curTime{ParamsDef::getCurrentAcquisiteTime() * Rdg_Sched_Type::MILLIS_PER_SEC};
                LOG_I("curTime: %lld", curTime);
                const int64_t lastOpCompleteTime{pFindQueue->getLastOpComlTime() * Rdg_Sched_Type::MILLIS_PER_SEC};
                LOG_I("lastOpCompleteTime: %lld", lastOpCompleteTime);
                const int64_t intervalDuration{pFindQueue->getIntervalDuration()};
                LOG_I("intervalDuration: %lld", intervalDuration);
                const int64_t tmpTargetTime{lastOpCompleteTime + intervalDuration};
                if(curTime < tmpTargetTime) {
                    tmp_Duration = tmpTargetTime - curTime;
                } else {
                    tmp_Duration = Rdg_Sched_Type::IG_ON_PERIOD_TRIGGER_DURATION*Rdg_Sched_Type::MILLIS_PER_SEC;
                }
                LOG_I("tmp_Duration: %lld", tmp_Duration);
                /* RDG30-R-1080: Trigger routine after 90s */
                LOG_I("Start ST_PERIOD_TRIGGER_ROUTINE index: %llu after: %lld", pFindQueue->getSchedIndex(), tmp_Duration);
                pFindQueue->stopTime();
                pFindQueue->setIsExecuted(0U);
                // pFindQueue->startTime(Rdg_Sched_Type::IG_ON_PERIOD_TRIGGER_DURATION*Rdg_Sched_Type::MILLIS_PER_SEC);
                pFindQueue->startTime(tmp_Duration);
            } else if(schedType_Dat == Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE) {
                LOG_I("Start ST_IG_ON_TRIGGER_ROUTINE index: %llu after 70s", pFindQueue->getSchedIndex());
                pFindQueue->stopTime();
                pFindQueue->setIsExecuted(0U);
                pFindQueue->startTime();
            } else if(schedType_Dat == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_ROUTINE) {
                LOG_I("Stop ST_IG_OFF_TRIGGER_ROUTINE index: %llu", pFindQueue->getSchedIndex());
                pFindQueue->stopTime();
            } else if(schedType_Dat == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT) {
                LOG_I("Stop ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT index: %llu", pFindQueue->getSchedIndex());
                pFindQueue->stopTime();
            } else {
                LOG_I("Undefined Sched Type");
            }
        }
    } else {
        /*RDG30-R-1086: set timer for 15s to discard IG OFF DIAG process*/
        mTimers[1]->start();
        for(it = mSchedulerMap.begin(); it != mSchedulerMap.end(); it++) {
            pFindQueue = it->second;
            if(pFindQueue->getFirstSchedType() == Rdg_Sched_Type::SchedType::ST_PERIOD_TRIGGER_ROUTINE) {
                LOG_I("Stop ST_PERIOD_TRIGGER_ROUTINE index: %llu", pFindQueue->getSchedIndex());
                pFindQueue->stopTime();
            } else if(pFindQueue->getFirstSchedType() == Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE) {
                LOG_I("Stop ST_IG_ON_TRIGGER_ROUTINE index: %llu", pFindQueue->getSchedIndex());
                pFindQueue->stopTime();
            } else if(pFindQueue->getFirstSchedType() == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_ROUTINE) {
                LOG_I("Start ST_IG_OFF_TRIGGER_ROUTINE index: %llu", pFindQueue->getSchedIndex());
                pFindQueue->setIsExecuted(0U);
                pFindQueue->startTime();
            } else if(pFindQueue->getFirstSchedType() == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT) {
                LOG_I("Start ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT index: %llu", pFindQueue->getSchedIndex());
                pFindQueue->startTime();
            } else {
                LOG_I("Undefined Sched Type");
            }
        }
    }
    LOG_I("Finish setIgStatus");
}

void SchedulerManager::applyNewSchedData(const bool isOnlyLoadSched, const bool isBooting) {
    /* Check ScheduleInformation for below data
      * collection condition                   | 1 | 2 | 3 | 4 | 5 | 6
      * CenterRequestDirectCommand             | x | x | x | - | - | -
      * CollectionConditionDirectCommand       | - | - | - | x | x | x
      * CollectionConditionEcuInformation      | - | - | - | - | x | -
      * CollectionConditionRobRobSsrDidEvent   | x | - | - | x | x | -
*/
    const std::shared_ptr<vccomif::rdg::v1::interfaces::GetCollectionConditionRequest> collectionReq 
        {CollectionCondition::getInstance().getGetCollectionConditionRequest()};
    // clearMap();
    /*Delete The deletion collection Conditions*/
    const std::vector<uint64_t> deleteCollID{CollectionCondition::getInstance().getDeletedCollectionConditionIds()};
    for (std::vector<uint64_t>::const_iterator it {deleteCollID.begin()}; it != deleteCollID.end(); ++it) {
        /*Find Schedindex *it*/
        /*Delete Schedindex*/
        LOG_D("Delete schedIndex: %llu", *it);
        deteleSchedInMap(*it);
    }

    std::unordered_map<uint64_t, bool> uMap_DID_EVENT{};
    std::unordered_map<uint64_t, bool> uMap_ECU_INFOMATION{};
    std::unordered_map<uint64_t, bool> uMap_WARNING_INFORMATION{};
    std::unordered_map<uint64_t, bool> uMap_DIRECT_COMMAND{};
    const std::vector<std::pair<uint64_t, uint8_t>> collData{CollectionCondition::getInstance().getUpdatedCollectionConditionIds()};
    uint64_t collID{0U};
    uint8_t func{0U};
    for(std::vector<std::pair<uint64_t, uint8_t>>::const_iterator it{collData.begin()}; it != collData.end(); it++) {
        collID = it->first;
        func = it->second;
        LOG_D("Check collID: %llu func: %d", collID, func);
        if(func == 0U) {
            uMap_DID_EVENT[collID] = true;
        } else if(func == 1U) {
            uMap_ECU_INFOMATION[collID] = true;
        } else if(func == 2U) {
            uMap_WARNING_INFORMATION[collID] = true;
        } else if(func == 3U) {
            uMap_DIRECT_COMMAND[collID] = true;
        } else {
            LOG_D("Undefined func");
        }
    }
    const CenterRequestDirectCommandList &requestList {CollectionCondition::getInstance().getCenterRequestDirectCommand()};
    /*GetCollectionConditionResponse_CenterRequestDirectCommand*/
    if (requestList.size() > 0)
    {
        for (CenterRequestDirectCommandIter it {requestList.begin()}; it != requestList.end(); it++)
        {
            LOG_I("CenterRequestDirectCommand check ID: %llu", it->collection_condition_id());
            /* make CenterReqData and obtain MSG_RECEIVE_CENTERCOMMNAD*/
            
            if (it->has_schedule_information())
            {
                Rdg_Sched_Type::SchedType sType{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
                const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data{it->schedule_information()};
                sType = static_cast<Rdg_Sched_Type::SchedType>(schedule_data.schedule_type());
                if(sType == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT) {
                    const uint64_t id{it->collection_condition_id()};
                    const uint32_t prio {schedule_data.priority()};
                    LOG_I("CenterRequestDirectCommand check prio: %d, schedule type:  %d", prio, sType);
                    const android::sp<SchedulerTime> scheduleTime {new SchedulerTime(static_cast<Rdg_Sched_Type::SchedType>(sType), id, 0U, 0U, 0U)};
                    scheduleTime->setFuncType(Rdg_Sched_Type::SchedFuncType::CENTER_RQ_DIRECTCOMMAND);
                    scheduleTime->setPrio(prio);
                    insertToMap(scheduleTime, static_cast<uint64_t>(id));
                    LOG_I("Check prio: %d schedFunc: %d schedule type: %d day: 0 hour: 0 min: 0", 
                        prio, scheduleTime->getFuncType(), sType);
                }
            }
        }
    }
    else
    {
        LOG_D("requestList is empty");
    }
    /*CollectionConditionDirectCommand*/
    if(collectionReq != nullptr){
        const vccomif::rdg::v1::interfaces::GetCollectionConditionRequest_CollectionConditionIdStoredInVehicle& 
            ColIdStored {collectionReq->collection_condition_id_stored_in_vehicle()};
        google::protobuf::RepeatedField<google::protobuf::uint64> DirectCommandIdList{};
        DirectCommandIdList = ColIdStored.direct_command_collection_condition_ids();
        LOG_I("Check Collection Condition DirectCommandIdList size: %d", DirectCommandIdList.size());
        for(google::protobuf::RepeatedField<google::protobuf::uint64>::iterator it {DirectCommandIdList.begin()}; it != DirectCommandIdList.end(); it++) {
            LOG_I("Check collection condition Directcomand id: %llu", *it);
            const std::shared_ptr<vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CollectionConditionDirectCommand> command
                {CollectionCondition::getInstance().getCollectionConditionDirectCommand(*it)};
            if(command != nullptr) {
                const bool resFind{uMap_DIRECT_COMMAND.find(*it) != uMap_DIRECT_COMMAND.end()};
                const ::vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_UpdateTypeMultiple upType{command->update_type_collection_condition()};
                if((resFind && (upType == ::vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_UpdateTypeMultiple::GetCollectionConditionResponse_UpdateTypeMultiple_UTM_ADDED))
                || (isBooting)) {
                if(command->has_schedule_information() == true) {
                    const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data{command->schedule_information()};
                    /*get ScheduleInterval:
                     - schedule_interval_days
                     - schedule_interval_hours
                     - schedule_interval_minutes*/
                    uint32_t day{0U};
                    uint32_t hour{0U};
                    uint32_t min{0U};

                    if(schedule_data.has_schedule_interval() == true) {
                        const ::vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleInterval& interval {schedule_data.schedule_interval()};
                        day = interval.schedule_interval_days();
                        hour = interval.schedule_interval_hours();
                        min = interval.schedule_interval_minutes();
                    }
                    /*SchedulerTime(SchedType pSchedType, int64_t request_id, 
                        uint8_t interval_d, uint8_t interval_h, uint8_t interval_m);*/
                    const uint64_t id{*it};
                    // if(id > static_cast<uint64_t>(INT64_MAX)){
                    //     // print error log
                    // }
                    /*get priority*/
                    const uint32_t prio {static_cast<uint32_t>(schedule_data.priority())};
                    /*get ScheduleType*/
                    const vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType sType {schedule_data.schedule_type()};
                    if(day > static_cast<uint32_t>(UINT8_MAX)){
                        // print error log
                        day = 0U;
                    }
                    if(hour > static_cast<uint32_t>(UINT8_MAX)){
                        // print error log
                        hour = 0U;
                    }
                    if(min > 60U){
                        // print error log
                        min = 0U;
                    }
                    const android::sp<SchedulerTime> scheduleTime {new SchedulerTime(static_cast<Rdg_Sched_Type::SchedType>(sType), id, static_cast<uint8_t>(day), static_cast<uint8_t>(hour), static_cast<uint8_t>(min))};
                    scheduleTime->setFuncType(Rdg_Sched_Type::SchedFuncType::COLLECTION_COND_DIRECTCOMMAND);
                    scheduleTime->setPrio(prio);
                    insertToMap(scheduleTime, static_cast<uint64_t>(*it));
                    LOG_I("Check prio: %d schedFunc: %d schedule type: %d day: %d hour: %d min: %d", 
                        prio, scheduleTime->getFuncType(), sType, day, hour, min);
                    
                } else {
                    LOG_I("has_schedule_information return false");
                }
                } else {
                    LOG_D("Schedindex: %llu is not ADDED", *it);
                }
                (void)upType;
            } else {
                LOG_I("command is nullptr");
            }
        }

        /*CollectionConditionEcuInformation*/
        const std::shared_ptr<CollectionConditionEcuInformation> ptrEcuInformation 
            {CollectionCondition::getInstance().getCollectionConditionEcuInformation()};
        if(ptrEcuInformation != nullptr) {
            const bool resFind{uMap_ECU_INFOMATION.find(ptrEcuInformation->collection_condition_id()) != uMap_ECU_INFOMATION.end()};
            const ::vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_UpdateTypeSingle uptype{ptrEcuInformation->update_type_collection_condition()};
            if((resFind && (uptype == ::vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED))
                || (isBooting)) {
            /*Get schedule information*/
            if(ptrEcuInformation->has_schedule_information() == true) {
                Rdg_Sched_Type::SchedType tempScheduleType{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
                const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data {ptrEcuInformation->schedule_information()};
                /*get ScheduleType*/
                const vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType sType {schedule_data.schedule_type()};
                if ((sType >= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MIN)
                    && (sType <= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MAX))
                {
                    tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(sType);
                }
                /*get ScheduleInterval:
                    - schedule_interval_days
                    - schedule_interval_hours
                    - schedule_interval_minutes*/
                uint32_t day{0U};
                uint32_t hour{0U};
                uint32_t min{0U};

                if(schedule_data.has_schedule_interval() == true) {
                    const ::vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleInterval& interval {schedule_data.schedule_interval()};
                    day = interval.schedule_interval_days();
                    hour = interval.schedule_interval_hours();
                    min = interval.schedule_interval_minutes();
                }
                /*SchedulerTime(SchedType pSchedType, int64_t request_id, 
                    uint8_t interval_d, uint8_t interval_h, uint8_t interval_m);*/
                /*Get collection ID*/
                const uint64_t id {static_cast<uint64_t>(ptrEcuInformation->collection_condition_id())};
                /*get priority*/
                const uint32_t prio {static_cast<uint32_t>(schedule_data.priority())};
                LOG_D("CollectionConditionEcuInformation check id: %llu", id);
                if(day > static_cast<uint32_t>(UINT8_MAX)){
                    // print error log
                    day = 0U;
                }
                if(hour > static_cast<uint32_t>(UINT8_MAX)){
                    // print error log
                    hour = 0U;
                }
                if(min > 60U){
                    // print error log
                    min = 0U;
                }
                const android::sp<SchedulerTime> scheduleTime {new SchedulerTime(tempScheduleType, id, static_cast<uint8_t>(day), static_cast<uint8_t>(hour), static_cast<uint8_t>(min))};
                scheduleTime->setFuncType(Rdg_Sched_Type::SchedFuncType::COLLECTION_COND_ECUINFORMATION);
                scheduleTime->setPrio(prio);                
                insertToMap(scheduleTime, static_cast<uint64_t>(id));
                LOG_I("Check prio: %d schedFunc: %d schedType: %d day: %d hour: %d min: %d", 
                    prio, scheduleTime->getFuncType(), tempScheduleType, day, hour, min);
            } else {
                LOG_D("has_schedule_information return false");
            }
            } else {
                LOG_D("SchedIndex: %llu is not UTS_CHANGED", ptrEcuInformation->collection_condition_id());
            }
            (void)resFind;
            (void)uptype;
            /*Insert to scheduler*/
        } else {
            LOG_I("ptrEcuInformation is null");
        }
        /*CollectionConditionRobRobSsrDidEvent*/
        const std::shared_ptr<CollectionConditionRobRobSsrDidEvent> ptrRobSSR 
            {CollectionCondition::getInstance().getCollectionConditionRobRobSsrDidEvent()};
        if(ptrRobSSR != nullptr) {
            const bool resFind{uMap_DID_EVENT.find(ptrRobSSR->collection_condition_id()) != uMap_DID_EVENT.end()};
            const ::vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_UpdateTypeSingle uptype{ptrRobSSR->update_type_collection_condition()};
            if((resFind && (uptype == ::vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED))
                || (isBooting)) {
            /*Get schedule information*/
            if(ptrRobSSR->has_schedule_information() == true) {
                Rdg_Sched_Type::SchedType tempScheduleType{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
                const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data {ptrRobSSR->schedule_information()};        
                /*get ScheduleType*/
                const vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType sType{schedule_data.schedule_type()};
                if ((sType >= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MIN)
                    && (sType <= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MAX))
                {
                    tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(sType);
                }

                /*get ScheduleInterval:
                    - schedule_interval_days
                    - schedule_interval_hours
                    - schedule_interval_minutes*/
                uint32_t day{0U};
                uint32_t hour{0U};
                uint32_t min{0U};

                if(schedule_data.has_schedule_interval() == true) {
                    const ::vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleInterval& interval {schedule_data.schedule_interval()};
                    day = interval.schedule_interval_days();
                    hour = interval.schedule_interval_hours();
                    min = interval.schedule_interval_minutes();
                }
                /*SchedulerTime(SchedType pSchedType, int64_t request_id, 
                    uint8_t interval_d, uint8_t interval_h, uint8_t interval_m);*/
                /*Get collection ID*/
                const uint64_t id {static_cast<uint64_t>(ptrRobSSR->collection_condition_id())};
                LOG_D("CollectionConditionRobRobSsrDidEvent check id: %llu", id);
                if(day > static_cast<uint32_t>(UINT8_MAX)){
                    // print error log
                    day = 0U;
                }
                if(hour > static_cast<uint32_t>(UINT8_MAX)){
                    // print error log
                    hour = 0U;
                }
                if(min > 60U){
                    // print error log
                    min = 0U;
                }
                /*get priority*/
                const uint32_t prio {static_cast<uint32_t>(schedule_data.priority())};  
                const android::sp<SchedulerTime> scheduleTime {new SchedulerTime(tempScheduleType, id, static_cast<uint8_t>(day), static_cast<uint8_t>(hour), static_cast<uint8_t>(min))};
                scheduleTime->setFuncType(Rdg_Sched_Type::SchedFuncType::COLLECTION_COND_ROBROBSSRDIDEVENT);
                scheduleTime->setPrio(prio);
                insertToMap(scheduleTime, static_cast<uint64_t>(id));
                LOG_I("Check prio: %d schedFunc: %d schedType: %d day: %d hour: %d min: %d", 
                    prio, scheduleTime->getFuncType(), tempScheduleType, day, hour, min);
            } else {
                LOG_D("has_schedule_information return false");
            }
            } else {
                LOG_D("SchedIndex: %llu is not UTS_CHANGED", ptrRobSSR->collection_condition_id());
            }
            (void)uptype;
        } else {
            LOG_I("ptrRobSSR is null");
        }
        if(isOnlyLoadSched != true) {
            applyChange();
        }
    } else {
        LOG_I("collectionReq is nullptr");
    }
    (void)collID;
    (void)func;
}

// void SchedulerManager::discardIgOffProcess() {
//     LOG_I("Start discard IG OFF process");
// }
void SchedulerManager::TimerHandler::handlerFunction(const int32_t timerId){
    switch (timerId)
    {
        case ID_IG_ON_TRIGGER_ROUTINE:
        {
            LOG_I("ID_IG_ON_TRIGGER_ROUTINE");
            LOG_I("70 sec expired from IG_ON");
            mSchedManager.setIgOnRoutineExpired(1U);
            mSchedManager.executeSchedIGONRoutine();
            break;
        }
        case ID_IG_OFF_PROCESS_EXPIRED:
        {
            LOG_I("ID_IG_OFF_PROCESS_EXPIRED");
            // SchedulerManager::discardIgOffProcess();
            break;
        }
        default:
            break;
    }
}
void SchedulerManager::setIgOnRoutineExpired(const uint8_t data) noexcept {
    isIgOnRoutineExpired = data;
}
uint8_t SchedulerManager::getIgOnRoutineExpired() const noexcept {
    return isIgOnRoutineExpired;
}
}
