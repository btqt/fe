#include "SchedulerManager.h"
#include "SchedulerHdl.h"
#include "SchedulerQueue.h"

namespace rdgapp {

SchedulerManager::SchedulerManager(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper): 
    android::RefBase()
    , mApp(app) 
    , isIgOnRoutineExpired{false}
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
        if(ptr != nullptr) {
            ptr->readyToStart();
        } else {
            LOG_E("SchedulerQueue is nullptr");
        }
        myQueue.pop();
    }
    LOG_I("Finish applyChange");
}

void SchedulerManager::executeSchedIGONRoutine() {
    /*Found ST_IG_ON_TRIGGER_ROUTINE queue and check running status => execute*/
    if(mSchedulerMap.empty() != true) {
        LOG_I("Start executeSchedIGONRoutine");
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
    LOG_I("SchedulerManager receive IG status: %d", status);
    (void)mpSchedulerHdl->obtainMessage(CMD_CHANGE_IG_STATUS, static_cast<int32_t>(status))->sendToTarget();
}

void SchedulerManager::handleReceiveIG(const bool status) {
    if ((mpIGStatus == false) && (status == true)) {
        // LOG_I("mTickTimer Start");
        LOG_I("SchedulerManager receive IG ON");
        // mIgOnTimestamp = MS_TO_SEC(TimeManager::getInstance().getCurrentMilliSec());
        mTimers[0]->start();
        setIgStatus(status);
    } else if((mpIGStatus == true) && (status == false)) {
        // LOG_I("mTickTimer Stop");
        LOG_I("SchedulerManager receive IG OFF");
        setIgOnRoutineExpired(false);
        mTimers[0]->stop();
        setIgStatus(status);
        // mTickTimer->stop();
    } else {
        LOG_I("Duplicated IG notification");
    }
}

void SchedulerManager::onRdgStop(const bool isStop) const {
    //obtain message to stop rdg
    (void)isStop;
    LOG_I("Stop RDG");
}

void SchedulerManager::insertToMap(const android::sp<SchedulerTime> SchedTime, uint64_t schedIndex) {
    LOG_I("insertToMap schedIndex: %llu ScheType: %d", schedIndex, SchedTime->getType());

    if (0U == checkSchedMapSize()) {
        android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
        /*Find schedIndex in mLastCompTime*/
        const std::map<uint64_t, std::pair<int64_t,uint8_t>>::iterator it{mLastCompTime.find(schedIndex)};
        if(it != mLastCompTime.end()) {
            newScheduleQueue->setLastOpComplTime(it->second.first);
            newScheduleQueue->setDiagCompleted(it->second.second);
        } else {
            LOG_E("Do not have schedIndex: %llu", schedIndex);
        }
        (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
    } else {
        android::sp<SchedulerQueue> mFindQueue{};
        const Rdg_Sched_Type::SchedType tmp_SchedType {SchedTime->getType()};
        if (Rdg_Sched_Type::SchedType::ST_IG_ON_TRIGGER_ROUTINE == tmp_SchedType) {
            const android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
            // (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
            mSchedulerMap[schedIndex] = newScheduleQueue;
        } else if (Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_ROUTINE == tmp_SchedType) {
            mFindQueue = getSameSchedType(Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_ROUTINE);
            if (nullptr == mFindQueue.get()){
                const android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
                // (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
                mSchedulerMap[schedIndex] = newScheduleQueue;
            } else {
                (void)mFindQueue->insert(SchedTime);
            }
        } else if (Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT == tmp_SchedType) {
            mFindQueue = getSameSchedType(Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT);
            if (nullptr == mFindQueue.get()){
                const android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
                // (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
                mSchedulerMap[schedIndex] = newScheduleQueue;
            } else {
                (void)mFindQueue->insert(SchedTime);
            }
        } else {
            const android::sp<SchedulerQueue> newScheduleQueue {new SchedulerQueue(mpSchedulerHdl, this, SchedTime, schedIndex)};
            /*Find schedIndex in mLastCompTime*/
            const std::map<uint64_t, std::pair<int64_t,uint8_t>>::iterator it{mLastCompTime.find(schedIndex)};
            if(it != mLastCompTime.end()) {
                newScheduleQueue->setLastOpComplTime(it->second.first);
                newScheduleQueue->setDiagCompleted(it->second.second);
            } else {
                LOG_E("Do not have schedIndex: %llu", schedIndex);
            }
            // (void)mSchedulerMap.insert(std::pair<uint64_t, android::sp<SchedulerQueue> >(schedIndex, newScheduleQueue));
            mSchedulerMap[schedIndex] = newScheduleQueue;

        }
    }
}

void SchedulerManager::notifySchedComplete(const uint64_t schedIndex, const int64_t completeTime, const bool isCompleted)
{
    /*1. Find schedIndex in saved mSchedulerMap
      2. Check schedType is routine or not
      3. Save completeTime as "last operation completion time"
      Format: sched_routine_schedIndex
    */
    const android::sp<SchedCompleteInfo> schedData{new SchedCompleteInfo(schedIndex, completeTime, isCompleted)};
    (void)mpSchedulerHdl->obtainMessage(MSG_NOTIFY_SCHED_COMPLETE, schedData)->sendToTarget();
}

void SchedulerManager::handleSchedComplete(const android::sp<SchedCompleteInfo> schedData) {
    //get const uint64_t schedIndex, const int64_t completeTime, const bool isCompleted from schedData
    const uint64_t schedIndex {schedData->getSchedIndex()};
    const int64_t completeTime {schedData->getCompleteTime()};
    const bool isCompleted {schedData->getIsCompleted()};
    ScheduleMapIt mSchedMapIt{};
    mSchedMapIt = mSchedulerMap.find(schedIndex);
    if(mSchedMapIt != mSchedulerMap.end()) {
        const android::sp<SchedulerQueue> mSchedQueue {mSchedMapIt->second};
        
        if(mSchedQueue != nullptr) {
            if(mSchedQueue->getFirstSchedType() == Rdg_Sched_Type::SchedType::ST_PERIOD_TRIGGER_ROUTINE) {
                LOG_I("SchedIndex: %llu. Save last operation completion time: %lld", schedIndex, completeTime);
                if(isCompleted) {
                    LOG_I("SchedIndex: %llu is Completed", schedIndex);
                } else {
                    LOG_I("SchedIndex: %llu is NOT Completed", schedIndex);
                }
                mSchedQueue->saveLastOpComplTime(completeTime, isCompleted);
            } else if (mSchedQueue->getFirstSchedType() == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT) {
                LOG_I("SchedIndex: %llu is ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT, deleting after completion", schedIndex);
                deleteSchedInMap(schedIndex);
            } else {
                LOG_D("SchedIndex: %llu - Sched type is not ST_PERIOD_TRIGGER_ROUTINE or one-shot", schedIndex);
            }
        } else {
            LOG_E("SchedIndex: %llu - SchedulerQueue is nullptr", schedIndex);
        }
    } else {
        LOG_E("Don't found schedule index: %llu", schedIndex);
    }
    (void)schedIndex;
    (void)completeTime;
    (void)isCompleted;
}

void SchedulerManager::deleteSchedInMap(const uint64_t schedIndex) {
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

    const std::map<uint64_t, std::pair<int64_t,uint8_t>>::iterator it{mLastCompTime.find(schedIndex)};
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
            if(mSchedQueue != nullptr) {
                mSchedQueue->stopTime();
            } else {
                LOG_E("SchedulerQueue is nullptr");
            }
        }
        mSchedulerMap.clear();
    }
}

uint32_t SchedulerManager::checkSchedMapSize() const noexcept {
    return mSchedulerMap.size();
}

android::sp<SchedulerQueue> SchedulerManager::getSameSchedType(const Rdg_Sched_Type::SchedType pSchedType) {
    LOG_I("getSameSchedType");
    android::sp<SchedulerQueue> mReturnQueue {nullptr};
    if(mSchedulerMap.empty() != true) {
        LOG_D("mSchedulerMap size: %d", mSchedulerMap.size());

        for (ScheduleMapIt mSchedMapIt {mSchedulerMap.begin()}; mSchedMapIt != mSchedulerMap.end(); mSchedMapIt++) {
            const android::sp<SchedulerQueue> pSchedQueue {mSchedMapIt->second};
            if((pSchedQueue != nullptr) && (pSchedQueue->getFirstSchedType() == pSchedType)) {
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

void SchedulerManager::onIgOnRoutineExpired() {
    //obtain message CMD_IG_ON_ROUTINE_EXPIRED
    LOG_I("onIgOnRoutineExpired");
    (void)mpSchedulerHdl->obtainMessage(CMD_IG_ON_ROUTINE_EXPIRED)->sendToTarget();
}
void SchedulerManager::handleIgOnRoutineExpired() {
    LOG_I("handleIgOnRoutineExpired");
    executeSchedIGONRoutine();
}

void SchedulerManager::saveComplTimeToFile() {
    const string PATH_COMPL_TIME_FILE{DATA_PATH + "compltime.dat"};
    const FileHandleType fileHdl {FileUtil::openFile(PATH_COMPL_TIME_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_BIN)};
    if(fileHdl != nullptr) {
        struct ComplTimePair {
        uint64_t schedIndex;
        int64_t timeStamp;
        uint8_t completeStatus;
        }__attribute__((packed));
        std::vector<ComplTimePair> vLastCompTime{};
        for(std::map<uint64_t, std::pair<int64_t,uint8_t>>::iterator it {mLastCompTime.begin()}; it !=mLastCompTime.end(); it++ ) {
            ComplTimePair p;
            p.schedIndex = it->first;
            p.timeStamp = it->second.first;
            p.completeStatus = it->second.second;
            vLastCompTime.push_back(p);
            LOG_I("Check size of pair: %d", sizeof(p));
        }
        LOG_I("Check vLastCompTime size: %lu", vLastCompTime.size());
        const uint32_t bufferSize {static_cast<uint32_t>(static_cast<uint32_t>(vLastCompTime.size()) * sizeof(ComplTimePair))};
        LOG_I("Check bufferSize: %d", bufferSize);
        if(bufferSize > 0U)
        {
            std::vector<uint8_t> mBuffer(bufferSize);
            (void)std::memcpy(&mBuffer[0], vLastCompTime.data(), bufferSize);
            const bool success {FileUtil::writeBinToFile(fileHdl, &mBuffer[0], bufferSize)};
            if (success != true) {
                LOG_I("Failed to write to file.");
            }
        }
        else
        {
            LOG_E("No ComplTime data");
        }

        (void)FileUtil::closeFile(fileHdl);
        /*TBD: Save RDG flag into DID */
    } else {
        LOG_I("fileHdl is nullptr");
    }
}

void SchedulerManager::saveComplTimeToFile(const uint64_t schedIndex, const int64_t timeData, const bool isCompleted) {
    const string PATH_COMPL_TIME_FILE{DATA_PATH + "compltime.dat"};
    LOG_I("saveComplTimeToFile schedIndex: %llu timestamp: %lld", schedIndex, timeData);
    /*Save time to file*/
    LOG_I("Write completeTime to file %s", PATH_COMPL_TIME_FILE.c_str());
    if(isCompleted) {
        LOG_D("Complete");
        mLastCompTime[schedIndex] = std::make_pair(timeData, 1U);
    } else {
        LOG_D("NOT Complete");
        mLastCompTime[schedIndex].second = 0U;
    }
    saveComplTimeToFile();
}

void SchedulerManager::loadComplTimeFromFile() {
    const string PATH_COMPL_TIME_FILE{DATA_PATH + "compltime.dat"};
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
            /*Calculate size of 
                struct ComplTimePair {
                uint64_t schedIndex;
                int64_t timeStamp;
                uint8_t completeStatus;
                }__attribute__((packed));*/
            constexpr  uint32_t step_size{sizeof(uint64_t) + sizeof(int64_t) + sizeof(uint8_t)};
            for (uint32_t i{0U}; i < size; i += step_size) {
                uint64_t key {0U};
                (void)std::memcpy(&key, &mBuffer[i], sizeof(uint64_t));
                int64_t time_value {0};
                (void)std::memcpy(&time_value, &mBuffer[i + sizeof(uint64_t)], sizeof(int64_t));
                uint8_t status_value {0U};
                (void)std::memcpy(&status_value, &mBuffer[i + sizeof(uint64_t)*2U], sizeof(uint8_t));
                mLastCompTime[key] = std::make_pair(time_value, status_value);
                LOG_I("Check ShedIndex: %llu timeStamp: %lld statusComplete: %d", key, time_value, status_value);
            }
            (void)step_size;
        }
        (void)FileUtil::closeFile(fileHdl);
    } else {
        LOG_I("fileHdl is nullptr");
    }
    LOG_I("End read time complete");
}

void SchedulerManager::executeFunction(const android::sp<SchedulerTime> executeSchedule)
{
    if (executeSchedule != nullptr)
    {
        LOG_I("executeFunction");
        uint64_t colID{0LLU};
        colID = executeSchedule->getRequestID();
        const Rdg_Sched_Type::SchedFuncType funcType{executeSchedule->getFuncType()};
        const Rdg_Sched_Type::SchedType schedType{executeSchedule->getType()};
        const uint8_t messID{static_cast<uint8_t>(funcType)};
        /*TBD: get prio*/
        const uint32_t prio{executeSchedule->getPrio()};
        constexpr DiagTrigger::DiagTriggerType type{DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER};
        const android::sp<CenterReqData> tmp_CenterReqData{new CenterReqData(colID, messID, prio, type, schedType)};
        mApp.onScheduleReceived(tmp_CenterReqData);
    }
    else
    {
        LOG_E("executeSchedule is nullptr");
    }
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
            Rdg_Sched_Type::SchedType schedType_Dat{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
            if(pFindQueue != nullptr) {
                schedType_Dat = pFindQueue->getFirstSchedType();
            }
            if(schedType_Dat == Rdg_Sched_Type::SchedType::ST_PERIOD_TRIGGER_ROUTINE) {
                /*1. get current time
                  2. Compare currentTime vs (LastOpCompleteTime + DurationTime)
                    a) currentTime >= (LastOpCompleteTime + DurationTime)
                    => start after 90s
                    b) currentTime < (LastOpCompleteTime + DurationTime)
                    => startTime((LastOpCompleteTime + DurationTime) - currentTime)
                */
                int64_t tmp_Duration{0};
                const bool tmp_checkDiagCompleted{pFindQueue->getIsDiagCompleted()};
                const int64_t curTime{CommonUtils::getCurrentAcquisiteTime() * Rdg_Sched_Type::MILLIS_PER_SEC};
                LOG_I("curTime: %lld", curTime);
                const int64_t lastOpCompleteTime{pFindQueue->getLastOpComlTime() * Rdg_Sched_Type::MILLIS_PER_SEC};
                LOG_I("lastOpCompleteTime: %lld", lastOpCompleteTime);
                const int64_t intervalDuration{pFindQueue->getIntervalDuration()};
                LOG_I("intervalDuration: %lld", intervalDuration);
                const int64_t tmpTargetTime{lastOpCompleteTime + intervalDuration};
                if((curTime < tmpTargetTime) && (tmp_checkDiagCompleted == true)) {
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
                (void)tmp_checkDiagCompleted;
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
                // pFindQueue->clearAll();
            } else {
                LOG_I("Undefined Sched Type");
            }
        }
    } else {
        /*RDG30-R-1086: set timer for 15s to discard IG OFF DIAG process*/
        mTimers[1]->start();
        for(it = mSchedulerMap.begin(); it != mSchedulerMap.end(); it++) {
            pFindQueue = it->second;
            if(pFindQueue != nullptr) {
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
            } else {
                LOG_E("SchedulerQueue is nullptr");
            }
        }
    }
    LOG_I("Finish setIgStatus");
}

void SchedulerManager::applyNewCenterReqData(const android::sp<CenterReqData> aCenterReqData) {
    //obtain message MSG_RECEIVE_CENTERCOMMNAD
    LOG_I("SchedulerManager receive new CenterRequestDirectCommand");
    (void)mpSchedulerHdl->obtainMessage(MSG_RECEIVE_NEW_CENTERCOMMNAD, aCenterReqData)->sendToTarget();
}

void SchedulerManager::handleNewCenterReqData(const android::sp<CenterReqData> aCenterReqData)
{
    if(aCenterReqData != nullptr) {
        const Rdg_Sched_Type::SchedType aScheduleType{aCenterReqData->getScheduleType()};
        const uint8_t messageId {aCenterReqData->getCenterReq_messageID()};
        if ((aScheduleType == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT) && (messageId == MSG_ID_CENTERREQUESTDIRECTCOMMAND))
        {
            const uint64_t cocoId{aCenterReqData->getCenterReq_CollectionID()};
            const uint32_t prio {aCenterReqData->getCenterReq_prio()};
            LOG_I("CenterRequestDirectCommand check prio: %d, schedule type:  %d", prio, aScheduleType);
            const android::sp<SchedulerTime> scheduleTime {new SchedulerTime(aScheduleType, cocoId, 0U, 0U, 0U)};
            scheduleTime->setFuncType(Rdg_Sched_Type::SchedFuncType::CENTER_RQ_DIRECTCOMMAND);
            scheduleTime->setPrio(prio);
            insertToMap(scheduleTime, static_cast<uint64_t>(cocoId));
            LOG_I("Check prio: %d schedFunc: %d schedule type: %d day: 0 hour: 0 min: 0", 
                prio, scheduleTime->getFuncType(), aScheduleType);
        } else {
            LOG_E("schedule type or message id not suppoted");
        }
        (void)messageId;
    } else {
        LOG_E("aCenterReqData is nullptr");
    }

}

void SchedulerManager::applyNewSchedData(const bool isOnlyLoadSched, const bool isBooting) {
    //obtain message MSG_NEW_SCHED_DATA to handller
    LOG_I("SchedulerManager receive new schedule data");
    (void)mpSchedulerHdl->obtainMessage(MSG_NEW_SCHED_DATA, static_cast<int32_t>(isOnlyLoadSched), static_cast<int32_t>(isBooting))->sendToTarget();
}
void SchedulerManager::handleNewSchedData(const bool isOnlyLoadSched, const bool isBooting) {
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
        deleteSchedInMap(*it);
    }
    saveComplTimeToFile();

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
    // const CenterRequestDirectCommandList &requestList {CollectionCondition::getInstance().getCenterRequestDirectCommand()};
    // /*GetCollectionConditionResponse_CenterRequestDirectCommand*/
    // if (requestList.size() > 0)
    // {
    //     for (CenterRequestDirectCommandIter it {requestList.begin()}; it != requestList.end(); it++)
    //     {
    //         LOG_I("CenterRequestDirectCommand check ID: %llu", it->collection_condition_id());
    //         /* make CenterReqData and obtain MSG_RECEIVE_CENTERCOMMNAD*/
            
    //         if (it->has_schedule_information())
    //         {
    //             Rdg_Sched_Type::SchedType sType{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
    //             const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data{it->schedule_information()};
    //             sType = static_cast<Rdg_Sched_Type::SchedType>(schedule_data.schedule_type());
    //             if(sType == Rdg_Sched_Type::SchedType::ST_IG_OFF_TRIGGER_NO_POWER_STATUS_ONE_SHOT) {
    //                 const uint64_t id{it->collection_condition_id()};
    //                 const uint32_t prio {schedule_data.priority()};
    //                 LOG_I("CenterRequestDirectCommand check prio: %d, schedule type:  %d", prio, sType);
    //                 const android::sp<SchedulerTime> scheduleTime {new SchedulerTime(static_cast<Rdg_Sched_Type::SchedType>(sType), id, 0U, 0U, 0U)};
    //                 scheduleTime->setFuncType(Rdg_Sched_Type::SchedFuncType::CENTER_RQ_DIRECTCOMMAND);
    //                 scheduleTime->setPrio(prio);
    //                 insertToMap(scheduleTime, static_cast<uint64_t>(id));
    //                 LOG_I("Check prio: %d schedFunc: %d schedule type: %d day: 0 hour: 0 min: 0", 
    //                     prio, scheduleTime->getFuncType(), sType);
    //             }
    //         }
    //     }
    // }
    // else
    // {
    //     LOG_D("requestList is empty");
    // }
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
                } 
                else
                {
                    LOG_E("Invalid schedule_type, skip update processing");
                }
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
                }
                else
                {
                    LOG_E("Invalid schedule_type, skip update processing");
                }
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
            mSchedManager.setIgOnRoutineExpired(true);
            mSchedManager.onIgOnRoutineExpired();
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
void SchedulerManager::setIgOnRoutineExpired(const bool data) noexcept {
    isIgOnRoutineExpired.store(data);
}
bool SchedulerManager::getIgOnRoutineExpired() const noexcept {
    return isIgOnRoutineExpired.load();
}
}
