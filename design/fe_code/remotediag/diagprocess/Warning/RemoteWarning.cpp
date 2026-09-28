#include "RemoteWarning.h"
#include <climits>

namespace rdgapp {
android::sp<RemoteWarning> RemoteWarning::mWarning{nullptr};

RemoteWarning::RemoteWarning (const Remotediag& app, android::sp<sl::SLLooper>& privLooper)
        : android::RefBase(), RemoteDelegate()
        , mApp(app)
        , mHandler{new MainHandler(privLooper, *this)}
        , APP_ID{RDG_APPID::WARNING}
        , mIGStatus{false}
        , mWarningPriority{false}
        , mPriority{0U}
        , mWarningChange{}
        , mTimerHandler{}
        , mIGOnTimer{}
        , mEngOnTimer{}
        , mChangedList{}
        , mEngOnStatus{EngineStartStatus::ENGINE_OFF}
        , mRDGFlag{false}
        , mWARFlag{false}
        , mConsentStatus{false}
        , mWarningStart{false}
        , mUnderRepair{0}
        , mIGOnReady{false}
        , mSignalArr{}
        , mTriggerType(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)
{
    mWarning = this;       
    (void)mHandler->obtainMessage(MainHandler::CMD_INIT_WARNING)->sendToTarget();
}

RemoteWarning::~RemoteWarning() noexcept {
    if (mTimerHandler != nullptr) {
        delete mTimerHandler;
    }
}

// RemoteWarning* RemoteWarning::instance {nullptr};
android::sp<RemoteWarning> RemoteWarning::getInstance() {
    if (mWarning == nullptr) {
        LOG_I("RemoteWarning is not created");
    }
    return mWarning;
}

void RemoteWarning::printData(const std::string data) const
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

void RemoteWarning::init() {
    initRemoteWarning();
}

void RemoteWarning::onServiceFlagChange(){  //RDG30-R-0661
    // LOG_I("Check received flag: 0x%x", status);

    // if(RDGFlag_tmp == 0U) {
    //     if(mRDGFlag == true) {

    //     }
    // }
    // if(WARflag_tmp == 0U) {
    //     if(mWARFlag == true){

    //     }
    // }

    if(mWarningPriority) {         //fix 29654468
        if(!mWARFlag){
            getServiceFlag();       
            if(mWARFlag){
                //TODO: RDG30-R-1205 - 1 WARFlag change from OFF-> ON
                isAbortWarning = true;
            } 
        } 
    }

    if (checkPrecondition() && (mUnderRepair == 0)) {  //fix 29647821
        LOG_I({"START WARNING"});
        startWarning(); 
    } else {
        LOG_I({"Condition is not satisfied, DO NOT PROCEED PROCESS"});
        mWarningStart = false;
    }
}

void RemoteWarning::onPPIReceived(){ 
    if (checkPrecondition() && (mUnderRepair == 0)) {  //fix 29647821
        LOG_I({"START WARNING"});
        startWarning(); 
    } else {
        LOG_I({"Condition is not satisfied, DO NOT PROCEED PROCESS"});
        mWarningStart = false;
    }
}

void RemoteWarning::notifyBootComplete() const {
    LOG_I({"boot is completed"});
}

void RemoteWarning::onReceiveIG(const bool status) const {
    (void)mHandler->obtainMessage(MainHandler::CMD_CHANGE_IG_STATUS, static_cast<int32_t>(status))->sendToTarget();
}

void RemoteWarning::onChangedRemoteInfo(const int32_t what, const int32_t info) {
    LOG_I("onChangedRemoteInfo");
    (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_STATUS_FROM_CENTER, what, info)->sendToTarget();
}

void RemoteWarning::onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) {
    const uint32_t prio_data{pCenterReqData->getCenterReq_prio()};
    const uint64_t colID{pCenterReqData->getCenterReq_CollectionID()};
    mPriorityId = prio_data;
    mColID = colID;
    LOG_I("Warning receive Center request ColID: %llu", mColID);
    const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_CENTER)};
    (void)msg->sendToTarget();
}

void RemoteWarning::MainHandler::handleMessage(const android::sp<sl::Message>& handlemsg) {
    const int32_t what {handlemsg->what};
    if ((what != CMD_SIGNAL_RECEIVED) && (what != CMD_TIMER_EXPIRED)) {
        LOG_I({"Main hander what: %d"}, what);
    }

    switch (what) {
        case CMD_INIT_WARNING:
        {
            mWarning.init();
            break;
        }
        case CMD_CHANGE_IG_STATUS:
        {
            LOG_I("CMD_CHANGE_IG_STATUS");
            mWarning.handleIGStatus((handlemsg->arg1 != 0) ? true : false);
            break;
        }
        case CMD_END_WARNING:
        {
            mWarning.handleFlagOff();
            break;
        }
        case CMD_SIGNAL_RECEIVED:
        {
            LOG_I("CMD_SIGNAL_RECEIVED");
            mWarning.handleSignalReceived(handlemsg);
            break;
        }
        case CMD_SIGNAL_TIMEOUT_RECEIVED:
        {
            LOG_I("CMD_SIGNAL_TIMEOUT_RECEIVED");
            mWarning.handleSignalTimeOutReceived(handlemsg);
            break;
        }
        case CMD_UPLOAD_DATA:
        {
            LOG_I("CMD_UPLOAD_DATA");
            mWarning.requestDataUpload();
            break;
        }
        case CMD_RECEIVE_STATUS_FROM_CENTER:
        {
            LOG_I("CMD_RECEIVE_STATUS_FROM_CENTER");
            mWarning.changedRemoteStatus(static_cast<int32_t>(handlemsg->arg1), static_cast<int32_t>(handlemsg->arg2));
            break;
        }
        case CMD_TIMER_EXPIRED:
        {
            mWarning.handleTimerExpired(handlemsg->arg1);
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
        case CMD_WARNING_TRIGGER_TO_DTC:
        {
            LOG_I("CMD_WARNING_TRIGGER_TO_DTC");
            const int32_t pTriggerType {static_cast<int32_t> (handlemsg->arg1)};
            mWarning.requestDiagTriggerDTC(pTriggerType);
            break;
        }
        case CMD_TRIGGER_FROM_CENTER:
        {
            LOG_I("CMD_TRIGGER_FROM_CENTER");
            mWarning.handleUpdateCollectionCondition();
            break;    
        }
        default:
        {
            LOG_D("Default case handleMessage");
            break;
        }
    }
}

void RemoteWarning::initRemoteWarning() {
    LOG_I("InitRemoteWarning");

    mIGOnReady = false;    // set flag for ready running
    mEngOnStatus = EngineStartStatus::ENGINE_OFF;  //Init Eng On status
    mTimerHandler = new TimerHandler(*this);   //Init timer (internal)

    mIGOnTimer = new Timer(mTimerHandler, TIMER_IG_ON_READY_ID);   //Init looper for timer
    mEngOnTimer = new Timer(mTimerHandler, TIMER_ENG_ON_ID);

    mChangedList = new FilteringList();   //filter list for warning filter

    makeTableAndParse();
}

void RemoteWarning::handleIGStatus(const bool status) { 
    LOG_I({"handleIGStatus, status=%d"}, status);
    mIGStatus = status;

    if (status) {
        constexpr uint32_t TIME_WAIT_RUNNING_READY{15U};
        mIGOnTimer->setDuration(TIME_WAIT_RUNNING_READY, 0U);  //after 15s timer send a response
        mIGOnTimer->start();
        if(mWarningPriority){
            LOG_I("Receive Twice IGN_ON");
            //Do nothing
        } else {
            if (checkPrecondition() && (mUnderRepair == 0)) {  //RDG30-R-0661
                LOG_I({"START WARNING"});
                startWarning();
            } else {
                LOG_I({"Condition is not satisfied, DO NOT PROCEED PROCESS"});
            }
        }
    } else if(mWarningPriority){
        LOG_I("handleIGStatus : IG ON -> OFF while warning process is running");
        abortUploadWarningInfo(Abort::ABORT_IG_STATE_CHANGE);
        stopWarning();  
        mIGOnReady = false;
        mWarningStart = false; //only reset in case can't monitoring CAN signal
    } else {
        stopWarning(); 
        mIGOnReady = false;
        mWarningStart = false;
    }
}

void RemoteWarning::stopWarning() {
    LOG_I({"stopWarning, mIGStatus:%d, mIGOnReady=%d"}, mIGStatus, mIGOnReady);

    if (!mWarningStart) {
        LOG_I({"already stopped or not started"});
    } else {
        isAbortWarning = false;
        mWarningPriority = false;
        mEngOnTimer->stop();
        mIGOnTimer->stop();

        mChangedList->reset();
    }
}

void RemoteWarning::handleFlagOff() {
    LOG_I("handleFlagOff mWarningStart:%d", mWarningStart);

    if (mWarningStart) {
        stopWarning();
        mWarningStart = false; 
    }
}

void RemoteWarning::changedRemoteStatus(const int32_t what, const int32_t info) {
    switch (what) {
        case WHAT_CHANGED_REPAIR_SATUS:
        {
            LOG_I("WHAT_CHANGED_REPAIR_SATUS: %d", info);
            mUnderRepair = info;
            LOG_I("mUnderRepair (%d)", mUnderRepair);
            if((mUnderRepair == 1) && (mIGOnReady == true)) {    //Only restrict - No Notify error upload to Center
                LOG_I("Restrict due to Under repair");
                abortUploadWarningInfo(Abort::ABORT_UNDER_REPAIR);    
            } else if (mUnderRepair == 0){  //Table 10-17 warning function is restricted when under repair 
                if (checkPrecondition()) {
                    LOG_I({"START WARNING"});
                    startWarning(); 
                } else {
                    LOG_I({"Condition is not satisfied, DO NOT PROCEED PROCESS"});
                    mWarningStart = false;
                }
            } else {
                //Do nothing
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void RemoteWarning::getWarningDataPart(Buffer& buf) const {
    uint8_t tempWarningData[WARNING_BYTE_TOTAL] {};
    
    LOG_I({"WARNING COUNTER VALUE FOR FILE UPLOAD"});
    LOG_I({"=========================="});
    for (uint32_t i {0U}; i < WARNING_BYTE_TOTAL; ++i) {
        uint8_t counterValue {0U};
        std::string log {""};
        for (uint32_t j {0U}; j < 8U; ++j) {
            const android::sp<WarningSignal> pWarning {mSignalArr[(i * 8U) + j]};
            if (pWarning->getFilteringMode() == static_cast<uint8_t>(FilteringMode::FILTERING_MODE_DETECT)) {
                counterValue |= static_cast<uint8_t>((static_cast<uint32_t>(0x1U) << (static_cast<uint8_t>(7U) - static_cast<uint8_t>(j))) & 0xFFU);
                log = log.append("1");
            } else if (pWarning->getFilteringMode() == static_cast<uint8_t>(FilteringMode::FILTERING_MODE_CANCEL)) {
                counterValue |= static_cast<uint8_t>(0x0U << (7U - j));
                log = log.append("0");
            } else {
                const bool counter {pWarning->getWarningCounter()};
                uint8_t tmp_counter{0x0U};
                if(counter)
                {
                    tmp_counter = 0x1U;
                }
                counterValue |= static_cast<uint8_t>((static_cast<uint32_t>(tmp_counter) << (static_cast<uint8_t>(7U) - static_cast<uint8_t>(j))) & 0xFFU);
                log = log.append((counter ? "1" : "0"));
                (void)tmp_counter;
            }
        }
        LOG_I({"WAR%2d (8bit): %s"}, i + 1U, log.c_str());

        tempWarningData[i] = counterValue;
    }
    LOG_I({"=========================="});
    buf.setTo(&tempWarningData[0], static_cast<int8_t>(WARNING_BYTE_TOTAL));
}

void RemoteWarning::requestDataUpload() {
    LOG_I({"requestDataUpload"});
    saveWarningCounterToMemory();
    packageUploadData();
    LOG_D("Check mSaveReq size before: %d", mSaveReq.size());
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mSaveReq.find(mPriorityId)};
    if (it != mSaveReq.end())
    {
        LOG_I("DONE TRIGGER PRIORITY PROCESS ID(%d)", mPriorityId);
        if(mPriorityId<=static_cast<uint32_t>(INT32_MAX))
        {
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mPriorityId), it->second->getType());
        }
        else
        {
            //Nothing
        }
        (void)mSaveReq.erase(it);
    }
    else 
    {
	    LOG_I("Can not find triggerId: %d in mSaveReq", mPriorityId);
    }
    stopWarning();
}

void RemoteWarning::requestDiagTriggerDTC(const int32_t pTriggerType) {
    LOG_I({"triggerWarningToDTC"});      
    uint64_t timeData{0U};
    android::sp<CommonDefine::RDGLocationData> location{new CommonDefine::RDGLocationData()}; 
    uint32_t odoValue {0U};
    uint32_t odoUnit {0U};
    DiagTrigger::DiagTriggerType triggerType{DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN};
    if ((pTriggerType > static_cast<int32_t>(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)) && (pTriggerType < static_cast<int32_t>(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)))
    {
        triggerType = static_cast<DiagTrigger::DiagTriggerType>(pTriggerType);
    }
    mChangedList->getFilteringTimeAndLocation(timeData, location, odoValue, odoUnit);  
    if(timeData <= static_cast<uint64_t>(INT64_MAX))
    {
        mApp.triggerWarningToDTC(triggerType, static_cast<int64_t>(timeData), location, mColID, 20U);
        //RDG30-R-0081   RDG30-R-0082  RDG30-R-0083   RDG30-R-0086   RDG30-R-0088   
    }
    else
    {
        LOG_D({"Error coding"}); 
    }
}

void RemoteWarning::requestPriority(){
    LOG_I("requestPriority");
    const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
    android::sp<DiagTrigger> pTrigger{new DiagTrigger(DiagTrigger::DiagTriggerType::WARNING_TRIGGER,
                            DiagTrigger::PRIO_WARNING_TRIGGER, DiagTrigger::DiagTriggerFunc::WARNING,
                            nextTriggerId)};
    /* set trigger time*/
    uint64_t timestamp{0U};
    android::sp<CommonDefine::RDGLocationData> location{new CommonDefine::RDGLocationData()}; 
    uint32_t odoValue {0U};
    uint32_t odoUnit {0U};
    mChangedList->getFilteringTimeAndLocation(timestamp, location, odoValue, odoUnit);  
    LOG_I("trigger time (%llu)",timestamp);
    if(timestamp <= static_cast<uint64_t>(INT64_MAX))
    {
        pTrigger->setTriggerTime(static_cast<int64_t>(timestamp));
    }
    else
    {
        //nothing
    }
    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
    (void)mHandler->obtainMessage(MainHandler::CMD_WARNING_TRIGGER_TO_DTC, static_cast<int32_t>(DiagTrigger::DiagTriggerFunc::WARNING))->sendToTarget();
    /* Save request to local*/
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
    LOG_I("Check mSaveReq size: %d", mSaveReq.size());
    if(!ret.second) {
        ret.first->second = pTrigger;
    }
    LOG_I("Check saved Warning trigger ID: %d", ret.first->first);
}

void RemoteWarning::saveWarningCounterToMemory() {
    LOG_I({"saveWarningCounterToMemory"});

    // update warning counter in filtering list
    mChangedList->updateWarningCounter();   //Update WC after verification progress

    uint8_t tmpWC[WARNING_BYTE_TOTAL] {};  //a tmpWC is WC value of 8 warning signal 
    for (uint32_t i {0U}; i < WARNING_SIGNAL_TOTAL; i++) {
        uint8_t tmp_cnt{0U};
        if(mSignalArr[i]->getWarningCounter())
        {
            tmp_cnt = 1U;
        }
        else{
            tmp_cnt = 0U;
        }
        (void)tmp_cnt;
        const uint8_t tmp_bit{mSignalArr[i]->getBitNumber()};
        if(tmp_bit <= 7U)
        {
            tmpWC[mSignalArr[i]->getByteNumber()] |= static_cast<uint8_t>((static_cast<uint32_t>(tmp_cnt) << static_cast<uint32_t>(tmp_bit)) & 0xFFU);
        }
        else
        {
            LOG_I("Error coding");
        }
    }

    // Print WC for saving
    LOG_D("============================");
    std::string log {""};
    for (uint32_t i {0U}; i < WARNING_SIGNAL_TOTAL; i++) {
        log = log.append(mSignalArr[i]->getWarningCounter() ? "1" : "0");
        if ((i % 8U) == 7U) {
            LOG_I({"WAR%2d (8bit): %s"}, (i / 8U) + 1U, log.c_str());
            log.clear();
        }
    }
    LOG_D("============================");

    //write data to file
	LOG_I("Write WC to file %s", WARNING_COUNTER_FILE.c_str());

    const FileHandleType fileHdl{FileUtil::openFile(WARNING_COUNTER_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_BIN)};
    if(fileHdl != nullptr) {
        struct WarningPair {
            uint8_t first;
            uint8_t second;
        }__attribute__((packed));
        std::vector<WarningPair> vSavedWC{};
        for(uint8_t i {0U}; i < WARNING_BYTE_TOTAL; i++) {
            const uint8_t low {tmpWC[i] & 0x0FU};
            const uint8_t high {tmpWC[i] >> 4U};
            WarningPair mPair;
            mPair.first = high;
            mPair.second = low;
            vSavedWC.push_back(mPair);
            //LOG_I("Check size of mPair: %d", sizeof(mPair));
        }
        LOG_I("Check vSavedWC size: %d", vSavedWC.size());
        const uint32_t bufferSize{vSavedWC.size() * sizeof(WarningPair)};
        LOG_I("Check bufferSize: %d", bufferSize);
        uint8_t mbuffer[bufferSize];
        (void)std::memcpy(&mbuffer[0], vSavedWC.data(), bufferSize);
        const bool success{FileUtil::writeBinToFile(fileHdl, &mbuffer[0], bufferSize)};
        if (success != true) {
            LOG_I("Failed to write WC to file.");
        } else {
            const uint8_t operation{getOperation()};
            // Self-Diag
            DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        }
        (void)FileUtil::closeFile(fileHdl);
        /*TBD: Save RDG flag into DID */
    } else {
        LOG_I("fileHdl is nullptr");
        //RDG30-R-0073
        mChangedList->revertWarningCounter();
    }
    return; 
    //=======> Write data into file/Store value in nonvolatile memory -> Rewrite
}

void RemoteWarning::saveWarningPropertyTableToMemory() {  //RDG30-R-1182
    LOG_I({"saveWarningPropertyTableToMemory"});

    //write data to file
	LOG_I("Write Warning property to file %s", WARNING_TABLE_FILE.c_str());

    const FileHandleType fileHdl{FileUtil::openFile(WARNING_TABLE_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_BIN)};

    if(fileHdl != nullptr) {
        const bool success{FileUtil::writeBinToFile(fileHdl, WARNING_TABLE_OTHERS, WARNING_SIGNAL_TOTAL)};
        if (success != true) {
            LOG_I("Failed to write Warning property table to file.");
        } else {
            const uint8_t operation{getOperation()};
            // Self-Diag
            DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        }
        (void)FileUtil::closeFile(fileHdl);
        /*TBD: Save RDG flag into DID */
    } else {
        LOG_I("fileHdl is nullptr");
    }
    return; 
    //=======> Write data into file/Store value in nonvolatile memory -> Rewrite
}

void RemoteWarning::initWarningCounter() {
    LOG_I({"initWarningCounter"});
    // update to WarningSignal. Get each bit of warning counter
    //Step 1: Get warning counter from file - RDG30-R-0072
    LOG_I("Read WC from file");
    /* Get file size*/
    uint32_t size{0U};
    std::ifstream file{std::ifstream(WARNING_COUNTER_FILE.c_str(), std::ios::binary | std::ios::ate)};
    const int64_t tmp_tell{file.tellg()};
    if((tmp_tell>=0) && (tmp_tell <= static_cast<int64_t>(UINT32_MAX)))
    {
        size = static_cast<uint32_t>(tmp_tell);
    }
    else
    {
        LOG_I("Error coding");
    }
    file.close();
    LOG_I("CHECK WC size: %d", size);
    /* Read file content */
    const FileHandleType fileHdl{FileUtil::openFile(WARNING_COUNTER_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN)};
    if(fileHdl != nullptr) {
        uint8_t WC_value[size];
        LOG_I({"warning counter file open successfully"});
        const bool success{FileUtil::ReadBinFromFile(fileHdl, &WC_value[0], size)};

        if (success) {
            std::string log {""};
            LOG_I({"WARNING COUNTER VALUE INIT"});
            LOG_I({"=========================="});
            for (uint32_t i {0U}; i < WARNING_SIGNAL_TOTAL; i++) {
                const uint32_t firstPlace {static_cast<uint32_t>(mSignalArr[i]->getByteNumber()) * 2U};
                const uint32_t secondPlace {(static_cast<uint32_t>(mSignalArr[i]->getByteNumber()) * 2U) + 1U};
                uint16_t tmpOpShit{0U}; 
                if(firstPlace < size)
                {
                    tmpOpShit = static_cast<uint16_t>((WC_value[firstPlace])) << 4U;
                }
                bool warningCounter {false};
                uint8_t bitShift{0U};
                const uint32_t tmp_bit{static_cast<uint32_t>(mSignalArr[i]->getBitNumber())};
                if (tmp_bit < 8U) {
                    const uint32_t tmpBitShift{static_cast<uint32_t>(static_cast<uint32_t>(1U) << tmp_bit)};
                    if (tmpBitShift <= static_cast<uint32_t>(UINT8_MAX))
                    {
                        bitShift = static_cast<uint8_t>(tmpBitShift);
                    }
                } else {
                    bitShift = 0U; 
                }
                if((tmpOpShit <= static_cast<uint16_t>(UINT8_MAX)) && (secondPlace < size))
                {
                    warningCounter = (((WC_value[secondPlace]) | static_cast<uint8_t>(tmpOpShit)) & bitShift) != 0U;
                }
                mSignalArr[i]->setWarningCounter(warningCounter);
                log = log.append(warningCounter ? "1" : "0");
                if ((i % 8U) == 7U) {
                    LOG_I({"WAR%2d (8bit): %s"}, (i / 8U) + 1U, log.c_str());
                    log.clear();
                }
                (void)firstPlace;
                (void)secondPlace;
                (void)bitShift;
                (void)warningCounter;
                (void)tmpOpShit;
            }
            LOG_I({"=========================="});
        } else {
            for (uint32_t i {0U}; i < WARNING_SIGNAL_TOTAL; i++) {
                mSignalArr[i]->setWarningCounter(false);
            }
            LOG_I({"warning counter File has no data, use default value"});
        }
    } else {
        for (uint32_t i {0U}; i < WARNING_SIGNAL_TOTAL; i++) {
            mSignalArr[i]->setWarningCounter(false);
        }
        LOG_I({"No warning counter file, use default value"});
    }
    (void)FileUtil::closeFile(fileHdl);
    // ========> need to re-write
}

void RemoteWarning::initWarningPropertyTable() {
    LOG_I({"initWarningPropertyTable"});
    // update to WarningSignal. Get each bit of warning counter
    //Step 1: Get warning counter from file - RDG30-R-0072
    LOG_I("Read Warning Property from file");
    /* Get file size*/
    uint32_t size{0U};
    //ifstream file{std::ifstream(WARNING_TABLE_FILE.c_str(), std::ios::binary | std::ios::ate)};
    std::ifstream file{std::ifstream(WARNING_TABLE_FILE.c_str(), std::ios::binary | std::ios::ate)};
    const int64_t tmp_tell{file.tellg()};
    if((tmp_tell>=0) && (tmp_tell <= static_cast<int64_t>(UINT32_MAX)))
    {
        size = static_cast<uint32_t>(tmp_tell);
    }
    else
    {
        LOG_I("Error coding");
    }
    file.close();
    LOG_I("CHECK Warning property size: %d", size);
    /* Read file content */
    const FileHandleType fileHdl{FileUtil::openFile(WARNING_TABLE_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN)};
    if(fileHdl != nullptr) {
        uint8_t WC_value[size];
        LOG_I({"warning table file open successfully"});
        const bool success{FileUtil::ReadBinFromFile(fileHdl, &WC_value[0], size)};
        if (success) {
            for (uint32_t idx {0U}; idx < WARNING_SIGNAL_TOTAL; idx++) {
                WARNING_TABLE_OTHERS[idx] = WC_value[idx]; 
            }
        } else {
            LOG_I({"warning property File has no data, use default value"});
        }
    } else {
        LOG_I({"No warning property file, use default value"});
    }
    (void)FileUtil::closeFile(fileHdl);

    //Print warning property table
    LOG_I("WARNING PROPERTY TABLE INIT");
    std::string log {""};
    //Print warning property table
    for(uint32_t idx{0U}; idx < WARNING_SIGNAL_TOTAL; ++idx){
        const uint8_t low {WARNING_TABLE_OTHERS[idx] & 0x0FU};
        const uint8_t high {WARNING_TABLE_OTHERS[idx] >> 4U};
        (void)log.append(1U, uint8ToChar(high));
        (void)log.append(1U, uint8ToChar(low));
        (void)log.append(", ");
        if((idx%8U) == 7U) {
            LOG_I("%s", log.c_str());
            log.clear();
        }
    }
    LOG_I("============================");
}

void RemoteWarning::makeTableAndParse() {
    LOG_I("makeTableAndParse");
    initWarningPropertyTable();
    obtainTableInfoFromMemory();    //set value for mSignalArr from hard property table
    initWarningCounter();
} 

void RemoteWarning::obtainTableInfoFromMemory() {
    //Warning Hard table for EU, warning table for others
    // Refer DCM24SPEC-14638 - DCM24SPEC-14830
    LOG_I({"obtainTableInfoFromMemory"});

    for (uint16_t i {0U}; i < WARNING_SIGNAL_TOTAL ; i++) {   //clear signal Arr
        if (mSignalArr[i] != nullptr) {
            mSignalArr[i].clear();
        }
    }

    //* Bit    | 1 bit                           | 1 bit                   | 2 bits    | 4 bits
    //* -      | -                               | -                       | -         | -
    //* Assign | Center notification requirement | Diagnostics requirement | Reserved  | WP determination time
    const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
    LOG_I("Parse Warning Property Hard Table before check nation");
    LOG_I("current Nation: %d", region);
    const uint8_t* refTblInfo {nullptr};
    if (region == LGE_REGION_NONE) {              //TBD-RDG doesn't support EU market               
        refTblInfo = &WARNING_TABLE_EU[0];
    } else {
        refTblInfo = &WARNING_TABLE_OTHERS[0];
    }
    for (uint16_t i {0U}; i < WARNING_SIGNAL_TOTAL ; i++) {
        mSignalArr[i] = new WarningSignal(static_cast<uint8_t>(i));   
        mSignalArr[i]->setTableValue(refTblInfo[i]);      //fill Warning property hard table to signal arr
        mSignalArr[i]->setNotifyCenter((refTblInfo[i] & 0x80U) > 0U);   //Get Notify center data
        mSignalArr[i]->setTriggerDiag((refTblInfo[i] & 0x40U) > 0U);    //Get Trigger Diag
        const uint8_t mwarningProperty{refTblInfo[i] & 0x0FU};   
        mSignalArr[i]->setWarningProperty(static_cast<uint8_t>((mwarningProperty * 4U)));  //Get Warning property(WP determination time) (Min 0s and max 60s)  
        mSignalArr[i]->setFilterCounter(mSignalArr[i]->getWarningProperty() / 2U);  //Get to ensure this warning is need Filter Counter or not (there are WP or not)
        mSignalArr[i]->setConfirmTimer(new Timer(mTimerHandler, static_cast<int32_t>(i))); //new timer for each WAR data
    }
    LOG_I({"obtainTableInfoFromMemory end"});
}

bool RemoteWarning::checkPrecondition() {
    LOG_I({"checkPrecondition "});
    bool iWarningFlag{false};
    getServiceFlag();
    iWarningFlag = mRDGFlag && mWARFlag && mConsentStatus && mIGStatus;
    LOG_I({"check Warning Detection Conditions : Result (%d), mRDG_Flag(%d), mWAR_Flag (%d), mConsentStatus (%d),  mIGStatus (%d)"},iWarningFlag, mRDGFlag, mWARFlag, mConsentStatus, mIGStatus);
    return iWarningFlag;  
}

void RemoteWarning::startWarning() {
    LOG_I({"startWarning"});

    if (mWarningStart) {
        LOG_I({"already started"});
    } else {
        mWarningStart = true;
        isAbortWarning = false; 
    }
}

void RemoteWarning::handleEngOnStatus() {
    if (mEngOnStatus == EngineStartStatus::ENGINE_OFF) {
        LOG_I({"Start ENG on timer"});
        mEngOnStatus = EngineStartStatus::ENGINE_ON_PROGRESS;
        constexpr uint32_t TIME_WAIT_ENG_ON {15U};
        mEngOnTimer->setDuration(TIME_WAIT_ENG_ON, 0U);
        mEngOnTimer->start();
    }
}

void RemoteWarning::handleEngOffStatus() {
    LOG_D("handleEngOffStatus");
    mEngOnTimer->stop();
    mEngOnStatus = EngineStartStatus::ENGINE_OFF;

}

bool RemoteWarning::handleWarningReceived(const android::sp<WarningSignal> warning, const uint8_t data) {  //RDG30-R-0883
    const bool notifyNeeded {warning->getNotifyCenter()};
    //const bool diagNeeded {warning->getTriggerDiag()};
    LOG_D("warningID(%d) - warningCounter(%d) - filterCounter(%d) - filterMode(%d) ",
         warning->getWarningID() , warning->getWarningCounter(),  warning->getFilterCounter(), warning->getFilteringMode());
    LOG_D("-- NeedNotifyCenter(%d) - NeedDoDiag (%d) - wpTime(%d) - Warning bit(%d) ", 
        notifyNeeded, warning->getTriggerDiag(), warning->getWarningProperty(), data);     
    bool ret;
    // Check if needed to notify center. If not then ignore warning.
    if (notifyNeeded && warning->getTriggerDiag()) {   //RDG30-R-0886 and RDG30-R-1192
        const bool warningCounter {warning->getWarningCounter()};
        if (data == 1U) {
            if (!warningCounter) {
                if (mIGOnReady == true) {    //Don't need check ENG state= >refer sequence 7-3  (DCM24SPEC-14989)
                    // Warning bit changed from 0 to 1, run detecting process.
                    if (warning->getFilteringMode() != static_cast<uint8_t>(FilteringMode::FILTERING_MODE_DETECT)) {
                        LOG_V({"WarningID[%d] start detect confirming with %d sec"}, warning->getWarningID(), warning->getWarningProperty());
                        if(mChangedList->getSize() == 0U) {
                            setTimeAndLocation();
                            //notify to get priority
                            // requestPriority();
                        }
                        addToConfirmList(warning, static_cast<uint8_t>(FilteringMode::FILTERING_MODE_DETECT));
                    } else {
                        LOG_V({"WarningID[%d] filter mode was detect"}, warning->getWarningID());
                    }
                    ret = true;
                } else {
                    LOG_I({"WarningID[%d] IG ON is not passed 15s, judge as invalid"}, warning->getWarningID());
                    ret = false;
                }
            } else {
                //if warning in cancellation list, remove it.
                if (warning->getFilteringMode() == static_cast<uint8_t>(FilteringMode::FILTERING_MODE_CANCEL)) {
                    if (warning->getFilterCounter() != 0U) {
                        LOG_I({"WarningID[%d] cancel filter counter not expired so stop and remove from list if existed"}, warning->getWarningID());
                        removeFromConfirmList(warning);
                        uploadIfNeeded();
                    } else {
                        LOG_V({"WarningID[%d] filterCounter has equal is zero"}, warning->getWarningID());
                    }
                } else {
                    LOG_V({"WarningID[%d] filter mode detect is not satisfied (%d)"}, warning->getWarningID(), warning->getFilteringMode());
                }
                ret = false;
            }
        } else {
            if (warningCounter) {
                if ((mIGOnReady == true) && (mEngOnStatus == EngineStartStatus::ENGINE_ON_COMPLETE)) {   //Refer figure 7-4
                    // Warning bit changed from 1 to 0, run cancelling process.
                    if (warning->getFilteringMode() != static_cast<uint8_t>(FilteringMode::FILTERING_MODE_CANCEL)) {
                        LOG_V({"WarningID[%d] start cancel confirming with %d sec"}, warning->getWarningID(), warning->getFilteringMode());
                        if (mChangedList->getSize() == 0U) {
                            setTimeAndLocation();
                            // requestPriority();
                        }
                        addToConfirmList(warning, static_cast<uint8_t>(FilteringMode::FILTERING_MODE_CANCEL));
                    }
                    ret = true;
                } else {
                    LOG_I({"WarningID[%d] ENG ON is not passed 15s, judge as invalid"}, warning->getWarningID());
                    ret = false;
                }  
            }  else {
                // if warning in defining list, remove it.
                if (warning->getFilteringMode() == static_cast<uint8_t>(FilteringMode::FILTERING_MODE_DETECT)) {
                    if (warning->getFilterCounter() != 0U) {
                        LOG_V({"WarningID[%d] detect filter counter not expired so stop and remove from list if existed"}, warning->getWarningID());
                        removeFromConfirmList(warning);
                        uploadIfNeeded();
                    } else {
                        LOG_V({"WarningID[%d] filterCounter has equal is zero"}, warning->getWarningID());
                    }
                } else {
                    LOG_V({"WarningID[%d] filter mode cancel is not satisfied (%d)"}, warning->getWarningID(), warning->getFilteringMode());
                }
                ret = false;
            }
        }

        if (ret) {
            if ((warning->getFilterCounter() == 0U) && (!mChangedList->checkAllFilterCounterExpired())) {
                LOG_V({"WarningID[%d] is waiting for other warning confirm"}, warning->getWarningID());
            }
        }
    } else {
        ret = false;
        LOG_D({"WarningID[%d] No need notify to center"}, warning->getWarningID());
    }

    return ret;
}

android::sp<WarningSignal> RemoteWarning::findWarningSignal(const uint16_t warningID) const {
    android::sp<WarningSignal>ret{nullptr};
    if (warningID != WARNING_SIGNAL_INVALID_ID) {
        for (uint16_t i {0U}; i < WARNING_SIGNAL_TOTAL ; i++) {
            if (mSignalArr[i]->getWarningID() == warningID) {
                ret = mSignalArr[i];
            }
        }
    }
    return ret;
}

void RemoteWarning::addToConfirmList(const android::sp<WarningSignal> warning, const uint8_t filterMode) {
    LOG_I({"Add To Confirm List warningID[%d], filterMode=%d"}, warning->getWarningID(),  filterMode);

    warning->setFilteringMode(filterMode);
    constexpr uint32_t RECEIPTION_INTERVAL {2U};
    warning->getConfirmTimer()->setDuration(RECEIPTION_INTERVAL, RECEIPTION_INTERVAL); // Set duration for Timer --> The handle will be call one time after 2 second
    warning->getConfirmTimer()->start(); //start timer
    mChangedList->addToFilterList(warning);  //Add WAR to filter list
}

void RemoteWarning::removeFromConfirmList(const android::sp<WarningSignal> warning) {
    LOG_I({"Remove From Confirm List warningID[%d]"}, warning->getWarningID());

    warning->setFilteringMode(static_cast<uint8_t>(FilteringMode::FILTERING_MODE_NOTHING));
    warning->getConfirmTimer()->stop();
    warning->setFilterCounter(warning->getWarningProperty() / 2U);
    mChangedList->removeFromFilterList(warning);
}

void RemoteWarning::uploadIfNeeded() {
    LOG_I("uploadIfNeeded");
    // saveWarningCounterToMemory();
    if (mChangedList->getSize() == 0U) {
        LOG_D("Check mSaveReq size before: %d", mSaveReq.size());
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mSaveReq.find(mPriorityId)};
        if (it != mSaveReq.end())
        {
            LOG_I("DONE TRIGGER PRIORITY PROCESS ID(%d)", mPriorityId);
            if(mPriorityId <= static_cast<uint32_t>(INT32_MAX))
            {
                PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mPriorityId), it->second->getType());
            }
	        else
            {
                //Nothing
            }
            (void)mSaveReq.erase(it);
        }
        else 
        {
	        LOG_I("Can not find triggerId: %d in mSaveReq", mPriorityId);
        }
    } else if (mChangedList->checkAllFilterCounterExpired()) {
        if(!mWarningPriority) {
            requestPriority();
        } else {
            LOG_I({"all confirm filter counter expired so upload"});
            // mChangedList->setUploadStatus(true);
            (void)mHandler->obtainMessage(MainHandler::CMD_UPLOAD_DATA)->sendToTarget();
        }
    } else {
        /* Do Nothing */
        LOG_D("Do Nothing - continue wait to expired the last timer");
    }
}

void RemoteWarning::handleSignalReceived(const android::sp<sl::Message>& msg) {
    LOG_I("handleSignalReceived");
    android::sp<VehicleData> vehicleData {nullptr};
    msg->getObject(vehicleData);
    const uint32_t sigId {vehicleData->sigID};
    LOG_I("SigID: 0x%04x - bufferSize: %d", sigId, vehicleData->buffer->size());
    const android::sp<::Buffer> mbuffer {vehicleData->buffer};

    // checking Engine state : ASTNRM or RDYIND 
    
    if (sigId == CMD_CAN_SIGNAL_ENG1G90) {    //6-DCF-10-02-24DCM Global CAN Data Specification
        uint8_t engineData{0U};
        if ((mbuffer != nullptr) && (mbuffer->data() != nullptr)) {
            engineData = ((mbuffer->data()[0] >> 7U) & 0x1U);
        } else {
            engineData = 3U;  //undefined value
        }
        LOG_I({"Engine Status: %d"}, engineData);
        if ((mEngOnStatus != EngineStartStatus::ENGINE_ON_COMPLETE) && (engineData == 1U)) {
            handleEngOnStatus();
        } else if ((mEngOnStatus != EngineStartStatus::ENGINE_OFF) && (engineData == 0U)) {
            handleEngOffStatus();
        } else {
            /* Do Nothing */
        }   
    } else {
        if (mUnderRepair != 0) {
            LOG_I({"Is under repair status. Do NOT handle warning signal"});
        } else if (mIGOnReady == true ) {
            //Print CAN FRAME      
            std::string log {""};
            if ((mbuffer != nullptr) && (mbuffer->data() != nullptr)) {
                for (uint32_t i {0U}; i < mbuffer->size(); i++)
                {
                    const uint8_t low {mbuffer->data()[i] & 0x0FU};
                    const uint8_t high {mbuffer->data()[i] >> 4U};
                    (void)log.append(1U, uint8ToChar(high));
                    (void)log.append(1U, uint8ToChar(low));
                    (void)log.append(" ");
                }
            } else {
                LOG_D("mbuffer or mbuffer->data() is nullptr");
            }
    
            LOG_I("CAN FRAME : %s", log.c_str());
            log.clear();
                ////////

            uint16_t index {0U};
            uint16_t count {0U};
            getWarningSetIndex(sigId, index, count);
            if ((index == 0xFFU) && (count == 0xFFU)) {
                LOG_I({"signalid %d is not supported"}, sigId);
            } else {
                LOG_I({"MET Warning sigId= 0x%04x"}, sigId);
                if((index + count) <= static_cast<uint16_t>(UINT16_MAX))
                {
                    updateWarningChange(index, index + count, vehicleData->buffer);
                }
                else
                {
                    LOG_D("Error coding");
                }
                std::map<uint32_t, uint8_t>::iterator it {mWarningChange.begin()};
                if (mWarningChange.size() > 0U) {
                    for (it = mWarningChange.begin(); it != mWarningChange.end(); ++it) {
                        sp<WarningSignal> sig{nullptr};
                        const uint32_t tmp_it{it->first};
                        if(tmp_it <= static_cast<uint32_t>(UINT16_MAX))
                        {
                            sig = findWarningSignal(static_cast<uint16_t>(tmp_it));
                        }
                        (void)handleWarningReceived(sig, it->second);    //second is warning bit
                    }
                }
            }
        } else {
            LOG_I({"IG/ENG is not passed 15s, judge as invalid : Eng state - %d, IG state - %d"},mEngOnStatus, mIGOnReady);
        }
    }
}

void RemoteWarning::handleSignalTimeOutReceived(const android::sp<sl::Message>& msg) {
    LOG_I("handleSignalTimeOutReceived");
    android::sp<VehicleData> vehicleData {nullptr};
    msg->getObject(vehicleData);
    const uint32_t sigId {vehicleData->sigID};
    LOG_I("SigID: 0x%04x - bufferSize: %d", sigId, vehicleData->buffer->size());

    if (mUnderRepair != 0) {
        LOG_I({"Is under repair status. Do NOT handle warning signal timer out"});
    } else  {
        if ((mEngOnStatus == EngineStartStatus::ENGINE_ON_COMPLETE) && (mIGOnReady == true) ) {
            uint16_t index {0U};
            uint16_t count {0U};
            getWarningSetIndex(sigId, index, count);
            if ((index == 0xFFU) && (count == 0xFFU)) {
                LOG_I({"signalid %d is not supported"}, sigId);
            } else {   //RDG30-R-1189
                LOG_I({"Warning signal[0x%04X] disrupted so remove warning in confirming list if existed"}, sigId);
                if (index <= (static_cast<uint16_t>(UINT16_MAX) - count)) {
                    for (uint16_t i {index}; i < (index + count); i++) {
                        const android::sp<WarningSignal> warning {findWarningSignal(i)};
                        removeFromConfirmList(warning);
                        uploadIfNeeded();
                    }
                } else {
                    LOG_E("index overflow");
                }
            }
        } else {
            LOG_I({"IG/ENG is not passed 15s, judge as invalid : Eng state - %d, IG state - %d"},mEngOnStatus, mIGOnReady);
        }
    }
}

void RemoteWarning::getWarningSetIndex(const uint32_t warningSetId, uint16_t& index, uint16_t& count) const noexcept {
    LOG_I("getWarningSetIndex");
    switch (warningSetId) {
        case CMD_CAN_SIGNAL_MET1S34:
        {
            index = 0U;
            count = 64U;
            break;
        }

        case CMD_CAN_SIGNAL_MET1S35:
        {
            index = 64U;
            count = 64U;
            break;
        }

        case CMD_CAN_SIGNAL_MET1S36:
        {
            index = 128U;
            count = 64U;
            break;
        }

        case CMD_CAN_SIGNAL_MET1S37:
        {
            index = 192U;
            count = 64U;
            break;
        }

        default:
        {
            //return maximum numer to check error in signal
            index = 255U;
            count = 255U;
            break;
        }

    }
}

void RemoteWarning::updateWarningChange(const uint16_t start, const uint16_t end, const android::sp<::Buffer> mbuffer) {
    LOG_I("updateWarningChange");
    android::sp<WarningSignal> warning {nullptr};
    
    //clear old data
    mWarningChange.clear();
    const uint32_t tmp_start{static_cast<uint32_t>(start)};
    for(uint32_t warningID {tmp_start}; warningID < end; warningID++) {
        warning = findWarningSignal(static_cast<uint16_t>(warningID));
        if (((warningID - 0U) >= tmp_start)&&((warning != nullptr) && (mbuffer->size() > 0U))) {
            if ((mbuffer != nullptr) && (mbuffer->data() != nullptr)) {
                const uint32_t dataPos {(warningID - tmp_start) / 8U};      //Get position of block
                uint8_t status{0U};
                const uint32_t tmp{(static_cast<uint32_t>(mbuffer->data()[dataPos]) >> (7U - ((warningID - tmp_start) % 8U)))};
                if(tmp <= static_cast<uint32_t>(UINT8_MAX))
                {
                    status = static_cast<uint8_t>(tmp) & 0x1U;
                }
                else{
                    LOG_D("Error coding");
                }
                //uint8_t status {(buffer->data()[dataPos] >> (7U - ((warningID - tmp_start) % 8U))) & 0x1U};  //Get position in block - warning bit
                (void)mWarningChange.insert(std::pair<uint32_t, uint8_t>(warningID, status));
                LOG_D("Warning ID (%u) - status (%u)", warningID, status);
            } else {
                LOG_D("mbuffer or mbuffer->data() is nullptr");
            }
        } else {
            LOG_D("Warning ID value is invalid");
        }
    }
}

void RemoteWarning::handleTimerExpired(const int32_t mtimerID) {
    switch (mtimerID) {
        case RemoteWarning::TIMER_IG_ON_READY_ID: // The first process
            LOG_I({"Timer set IG ON READY occurs"});
            mIGOnTimer->stop();
            mIGOnReady = true;
            break;

        case RemoteWarning::TIMER_ENG_ON_ID:   // The first process 
            LOG_I({"Timer set ENG on occurs"});
            mEngOnTimer->stop();
            mEngOnStatus = EngineStartStatus::ENGINE_ON_COMPLETE; //To register listener for CAN signal => Change to register at vehicle adapter
            break;

        default: //===> default case
            if ((mtimerID < static_cast<int32_t>(WARNING_SIGNAL_TOTAL))&&(mtimerID>=0))
            {
                LOG_D("WP time expired -  TimeID(%d)", mtimerID);
                const uint16_t tmp_timeId{static_cast<uint16_t>(mtimerID)};
                const android::sp<WarningSignal> warning {mSignalArr[tmp_timeId]};
                uint8_t filterCounter {warning->getFilterCounter()};
                if (filterCounter != 0U) {
                    filterCounter -= 1U;
                    warning->setFilterCounter(filterCounter);
                    //const uint8_t filterMode {warning->getFilteringMode()};

                    if (filterCounter == 0U) {
                        LOG_D({"WarningID[%d] is mode[%d](CANCEL 0/DETECT 1) confirmed"}, mtimerID, warning->getFilteringMode());
                        warning->getConfirmTimer()->stop();
                        uploadIfNeeded();
                    } else {
                        LOG_D({"WarningID[%d] is mode[%d](CANCEL 0/DETECT 1) confirming status with %d filterCounter"}, mtimerID, warning->getFilteringMode(), filterCounter);
                    }
                } else {
                    LOG_D("filterCounter is zero for TimerID(%d) => Don't handle it", mtimerID);
                }
            } else {
                LOG_I({"wrong mtimerID :%d"}, mtimerID);
            }
            break;
    }
}

char_t RemoteWarning::uint8ToChar(const uint8_t num) const noexcept {

    uint8_t tmp{0U};
    if(num <= 9U)
    {
        tmp = num + 48U;
    }
    else{
        if(num + 55U <= static_cast<uint8_t>(UINT8_MAX))
        {
            tmp = num + 55U;
        }
    }
    char_t res{'0'};
    if(tmp <= static_cast<uint8_t>(INT8_MAX))
    {
        res = static_cast<char_t>(tmp);
    }
    return res;
}

uint8_t RemoteWarning::charToUint8(const char_t c) const noexcept {
    const uint8_t c_i {static_cast<uint8_t>(c)};
    uint8_t res{0U};
    if(c_i <= 57U)
    {
        if(c_i >= 48U)
        {
            res = c_i - 48U;
        }
    }
    else
    {
        res = c_i - 55U;
    }
    return res;
}

void RemoteWarning::setTimeAndLocation() {
    LOG_D({"setTimeAndLocation"});
    // get Time data
    //TimeManager &mTimeManagerService{TimeManager::getInstance()};
    const int64_t tmp_sec{ParamsDef::getCurrentAcquisiteTime()};
    uint64_t stTimeData{0U};
    if(tmp_sec>=0)
    {
        stTimeData = static_cast<uint64_t>(tmp_sec);
    }
    LOG_I("[time test] (timedata) GMT Time : %lld", stTimeData);  

    // get Location data
    mLocationData = LocationManagerAdapter::getInstance()->getLocationData();
    LOG_I("Get location data success: Latitude 0x%08x - Longitude 0x%08x ", mLocationData->getLatitude(), mLocationData->getLongtitude());
    //get odo
    uint32_t odo_value {0U};
    uint32_t odo_unit {0U};
    (void)VehicleManagerAdapter::getInstance()->getOdoInformation(odo_value, odo_unit);

    LOG_I("[Odo test] odo_value: %lu, odo_unit: %lu", odo_value, odo_unit); 
    //set time and location to changedList
    mChangedList->setFilteringTimeAndLocation(stTimeData, mLocationData, odo_value, odo_unit);
    LOG_D("setTimeAndLocation END");
}

void RemoteWarning::onReceivedCanSignal(const uint32_t channel, const android::sp<VehicleData>& vehicleData){
    NOTUSED(channel);
    LOG_I("onReceivedCanSignal");
    if (vehicleData != nullptr) 
    {
        if(mWarningStart){
            (void)mHandler->obtainMessage(MainHandler::CMD_SIGNAL_RECEIVED, vehicleData)->sendToTarget();
        } else {
            LOG_I("WARNING DETECTION CONDITION IS NOT SATISFIED!!!");
        }
    } 
    else 
    {
        LOG_I("CAN NOT OBTAIN MSG DUE TO VEHICLE DATA IS EMPTY!!!");
    }
}

void RemoteWarning::onReceivedCanSignalTimeout(const uint32_t channel, const android::sp<VehicleData>& vehicleData){
    NOTUSED(channel);
    LOG_I("onReceivedCanSignalTimeout");
    if (vehicleData != nullptr) 
    {
        if(mWarningStart){
            (void)mHandler->obtainMessage(MainHandler::CMD_SIGNAL_TIMEOUT_RECEIVED, vehicleData)->sendToTarget();
        } else {
            LOG_I("WARNING DETECTION CONDITION IS NOT SATISFIED!!!");
        }
    } 
    else 
    {
        LOG_I("CAN NOT OBTAIN MSG DUE TO VEHICLE DATA IS EMPTY!!!");
    }
}


void RemoteWarning::testClearWarningCounter() const{
    LOG_D("SLDD clear Warning Counter file");
    const FileHandleType fileHdl{FileUtil::openFile(WARNING_COUNTER_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_BIN)};
    if(fileHdl != nullptr) {
        struct mMyPair {
            uint8_t first;
            uint8_t second;
        }__attribute__((packed));
        std::vector<mMyPair> vSavedWC{};
        for(uint8_t i {0U}; i < WARNING_BYTE_TOTAL; i++) {
            constexpr uint8_t low {0x00U};
            constexpr uint8_t high {0x00U};
            mMyPair mPair;
            mPair.first = high;
            mPair.second = low;
            vSavedWC.push_back(mPair);
            (void)i;
        }
        const uint32_t bufferSize{vSavedWC.size() * sizeof(mMyPair)};
        uint8_t mbuffer[bufferSize];
        (void)std::memcpy(&mbuffer[0], vSavedWC.data(), bufferSize);
        const bool success{FileUtil::writeBinToFile(fileHdl, &mbuffer[0], bufferSize)};
        if (success != true) {
            LOG_I("Failed to write WC to file.");
        }
        (void)FileUtil::closeFile(fileHdl);
    } else {
        LOG_I("fileHdl is nullptr");
    }
}

void RemoteWarning::getServiceFlag(){
    LOG_I("getServiceFlag from DiagManagerAdapter");
    mRDGFlag = true;
    mWARFlag = true;
    uint8_t tmp{DiagManagerAdapter::getInstance()->getRDGFlag()};
    if(tmp==0U)
    {
        mRDGFlag = false;
    }
    tmp = DiagManagerAdapter::getInstance()->getWARflag();
    if(tmp==0U)
    {
        mWARFlag = false;
    }
    mConsentStatus = DiagManagerAdapter::getInstance()->getAllUploadConsent();
    mIGStatus = PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_ON ? true : false;
}

bool RemoteWarning::notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff) 
{
    bool isWarningTrigger{false};
    /* Check if trigger is in the saved trigger*/
    if(pTriggerId>=0)
    {
        const uint32_t tmp{static_cast<uint32_t>(pTriggerId)};
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(tmp)};
        if(it != mSaveReq.end()) {
            LOG_I("notify Warning Trigger state: %d TriggerID: %d TriggerTime: %lld", pState, tmp, it->second->getTriggerTime());
            isWarningTrigger = true;
            handleTrigger(pState, pTriggerId, dueToIgOff);
        } else {
            LOG_I("Can not find triggerID in saved list");
            isWarningTrigger = false;
            PriorityControl::getInstance()->notifyTriggerNoFound(pTriggerId);
        }
    }
    else
    {
        // Do nothing
    }
    return isWarningTrigger;
}

void RemoteWarning::handleTrigger(const DiagTrigger::DiagTriggerState& pState,
    const int32_t& pTriggerId, const bool dueToIgOff) 
{
    if(pTriggerId>=0)
    {
        const uint32_t tmp{static_cast<uint32_t>(pTriggerId)};
        const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(tmp)};
        //const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
        if(it != mSaveReq.end()) {
            //const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
            LOG_I("notify Warning Trigger state: %d TriggerID: %d", pState, pTriggerId);
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
                const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
                const uint32_t prio_data{it->second->getPriority()};
                LOG_I("START TRIGGER PRIORITY PROCESSING - ID(%d)", pTriggerId);
                mPriorityId = static_cast<uint32_t>(pTriggerId);
                mTriggerType = pTriggerType;
                mPriority = prio_data;
                mWarningPriority = (pTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) ? true : false;
                uploadIfNeeded();   //For case check done all warning signal but is not received priority trigger
                break;
            }
            case DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED:
            {
                LOG_I("TRIGGER_SUSPENDED");
                break;
            }
            case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
            {
                const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
                mWarningPriority = (pTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) ? false : true;
                abortUploadWarningInfo(Abort::ABORT_PRIORITY_DISCARDED);
                LOG_I("TRIGGER_DISCARDED");
                break;
            }
            case DiagTrigger::DiagTriggerState::TRIGGER_DONE:
            {
                LOG_I("TRIGGER_DONE");
                break;
            }
            case DiagTrigger::DiagTriggerState::TRIGGER_STATE_MAX:
            {
                LOG_I("TRIGGER_STATE_MAX");
                break;
            }
            }
        }
        else
        {
            //nothing
        }
    }
    else
    {
        //nothing
    }
    (void)dueToIgOff;
}

void RemoteWarning::handleUpdateCollectionCondition(){
    LOG_I("handleUpdateCollectionCondition");
    const std::shared_ptr<CollectionConditionWarningInformation> mCenterWarning{CollectionCondition::getInstance().getCollectionConditionWarningInformation()};
    const std::string warningPropertyTable{mCenterWarning->warning_property_table()}; 
    LOG_I("warning table size: %d", warningPropertyTable.size());

    if(warningPropertyTable.size() == WARNING_SIGNAL_TOTAL) {
        (void)memcpy( WARNING_TABLE_OTHERS, warningPropertyTable.data(), WARNING_SIGNAL_TOTAL );
    } else {
        LOG_I("invalid warning table size");
    }
    
    //* Bit    | 1 bit                           | 1 bit                   | 2 bits    | 4 bits
    //* -      | -                               | -                       | -         | -
    //* Assign | Center notification requirement | Diagnostics requirement | Reserved  | WP determination time

    LOG_I("WARNING PROPERTY TABLE:");
    std::string log {""};
    //Print warning property table
    for(uint32_t idx{0U}; idx < WARNING_SIGNAL_TOTAL; ++idx){
        const uint8_t low {WARNING_TABLE_OTHERS[idx] & 0x0FU};
        const uint8_t high {WARNING_TABLE_OTHERS[idx] >> 4U};
        (void)log.append(1U, uint8ToChar(high));
        (void)log.append(1U, uint8ToChar(low));
        (void)log.append(", ");
        if((idx%8U) == 7U) {
            LOG_I("%s", log.c_str());
            log.clear();
        }
    }
    LOG_I("=======================");

    saveWarningPropertyTableToMemory();
    LOG_I("Refresh warning table");
    initWarningPropertyTable();      //Fix 29653133  
    obtainTableInfoFromMemory();    

    //RDG30-R-1205
    if(mWarningPriority) {  //fix 29654468
        //RDG30-R-1205 - 2 Property update while warning is processing
        abortUploadWarningInfo(Abort::ABORT_WARNING_TABLE_UPDATE);
    } else {
        //RDG30-R-1205 - 2  + RDG30-R-0708 -> Reset filter list and don't make error upload data
        LOG_D("Abort while WP timer is running");   
        mChangedList->reset();  //reset all timer
    }

    //RDG30-R-1207 Upload Data handle it    

}

uint64_t RemoteWarning::getCurrentColID() const noexcept
{
    // https://toyota-11f.rickcloud.jp/jira/browse/DCM24MON-3751
    // => Use conditionID of Diag_common
    uint64_t currentColID{0U};
    const std::shared_ptr<CollectionConditionDiagCommon> diagCommon{CollectionCondition::getInstance().getCollectionConditionDiagCommon()};
    if(diagCommon != nullptr) {
        currentColID = diagCommon->collection_condition_id();
    } else {
        LOG_V("centerWarning is null"); 
        currentColID = 0U;
    }

    LOG_I("CocoID: %llu", currentColID); 
    return currentColID;
}

void RemoteWarning::abortUploadWarningInfo(const Abort abortReason){    //RDG30-R-0708 - 2
    if(abortReason == Abort::ABORT_PRIORITY_DISCARDED){         // RDG30-R-0708
        LOG_I("Abort Warning due to priority discard");
        isAbortWarning = true;  //=> don't upload warning info to center after done table 10-17
    } else if((abortReason == Abort::ABORT_WARNING_FLAG_CHANGE) || (abortReason == Abort::ABORT_WARNING_TABLE_UPDATE)){  //RDG30-R-1205    
        //the warning file shall not be created and there shall be no warning cancellation judgment.
        LOG_I("Abort Warning due to WAR Flag OFF -> ON / Warning table update");
        isAbortWarning = true;
        makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED); //RDG30-R-0407
    } else if(abortReason == Abort::ABORT_RDG_FLAG_CHANGE){  //RDG30-R-1207
        isAbortWarning = true;
        makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED); //RDG30-R-0407
        //TODO: When the RDG active flag or warning flag is turned off, the warning upload data shall be deleted from Uploader.
    } else if(abortReason == Abort::ABORT_IG_STATE_CHANGE){  //RDG30-R-0708 - 1
        LOG_I("Abort Warning due to IG state change from ON to OFF");
        makeUploadErrorData(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
    } else if(abortReason == Abort::ABORT_UNDER_REPAIR){ // RDG30-R-1175 --> Don't notify Error - Table 10 - 17
        LOG_I("Abort Warning due to Under repair");
        isAbortWarning = true;
        //Only restrict - No Notify error upload to Center - RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR
    } else {
        LOG_D("Abort init case");
    }

    LOG_D("Check mSaveReq size before: %d", mSaveReq.size());
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it{mSaveReq.find(mPriorityId)};
    if (it != mSaveReq.end())
    {
        LOG_I("DONE TRIGGER PRIORITY PROCESS ID(%d)", mPriorityId);
        if(mPriorityId <= static_cast<uint32_t>(INT32_MAX))
        {
            PriorityControl::getInstance()->notifyTriggerProcessDone( static_cast<int32_t>(mPriorityId), it->second->getType());
            (void)mSaveReq.erase(it);
        }
	    else
        {
            //Nothing
        }
    }
    else 
    {
	    LOG_I("Can not find triggerId: %d in mSaveReq", mPriorityId);
    }
    stopWarning();
}

void RemoteWarning::packageUploadData(){
    LOG_I("packageUploadData - current isAbortWarning(%d)", isAbortWarning);
    if(!isAbortWarning) {
        mColID = getCurrentColID();
        uint64_t timestamp{0U};
        android::sp<CommonDefine::RDGLocationData> location{new CommonDefine::RDGLocationData()}; 
        uint32_t odoValue {0U};
        uint32_t odoUnit {0U};
        // DiagTrigger::DiagTriggerType triggerType = DiagTrigger::DiagTriggerType::WARNING_TRIGGER;
        mChangedList->getFilteringTimeAndLocation(timestamp, location, odoValue, odoUnit);  
        LOG_I("Warning data : time(%llu) - odoValue(%lu) - odoUnit(%lu)",timestamp, odoValue, odoUnit);
        mWarningInfoReq = std::shared_ptr<vccomif::rdg::v1::interfaces::UploadWarningInformationRequest>(new vccomif::rdg::v1::interfaces::UploadWarningInformationRequest());

        LOG_I("Time data: %lld ", timestamp);
        LOG_I("location data: Latitude 0x%08X - longitude 0x%08X ", location->getLatitude(), location->getLongtitude());

        //Make RdgCommonRequestHeader 
        //vccomif.common.AppCommonHeaderVehicleToCenter app_common_header
        //uint32 text_version  ==> PROTOBUF_VERSION
        mWarningInfoReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
        //ElectronicPf electronic_pf   ===> ElectronicPf::EPF_EPF3_0
        mWarningInfoReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
        //GeodesyInformation geodesy_information  ===> GeodesyInformation::GI_UNKNOWN  (Need to confirm)
        mWarningInfoReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);

        //TimeZoneOffset time_zone_offset 
        //sint32 hours  ==> UTC time
        //sint32 minutes ==> UTC time
        const int32_t tz{TimeManager::getInstance().getOffset()};
        mWarningInfoReq->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(tz / 60);
        mWarningInfoReq->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(tz % 60);


        //InterfaceType interface_type ==> RdgCommonRequestHeader_InterfaceType::IT_UPLOAD_WARNING_INFORMATION
        mWarningInfoReq->mutable_rdg_common_request_header()->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_WARNING_INFORMATION);
        //string message_id
        const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
        mWarningInfoReq->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_WARNING_INFORMATION, counterValue));
            //Content
        mWarningInfoReq->set_collection_condition_id(mColID);  //uint32 value
        //Make uint32 counter_value
        mWarningInfoReq->set_counter_value(counterValue);
        (void)counterValue;
        //Make uint64 warning_trigger_occurrence_time
        mWarningInfoReq->set_warning_trigger_occurrence_time(timestamp); //uint64 value
        //Make vccomif.rdg.interfaces.Location 
        vccomif::rdg::v1::interfaces::Location* const ilocation{mWarningInfoReq->mutable_location()};
        //sint32 latitude
        ilocation->set_latitude(location->getLatitude());
        ilocation->set_longitude(location->getLongtitude());
        //Make bool obd2_installed_flag
        mWarningInfoReq->set_obd2_installed_flag(mApp.getOBDStatus()); //bool value
        //Make odo_information
        if (odoUnit == 2U)
        {
            // uint32 odo_information_mile
            mWarningInfoReq->set_odo_information_mile(odoValue); // odo_unit = 10b ~ Mile
        }
        else if (odoUnit == 1U)
        {
            // uint32 odo_information_km
            mWarningInfoReq->set_odo_information_km(odoValue); // odo_unit = 01b ~ km
        }
        else
        {
            LOG_D("Terminating else statement");
        }

        //bool under_repair_flag
        if(mUnderRepair == 0)
        {
            mWarningInfoReq->set_under_repair_flag(false);
        }
        else
        {
            mWarningInfoReq->set_under_repair_flag(true);
        }
        //mWarningInfoReq->set_under_repair_flag(mUnderRepair);         //bool value    
        //bytes warning_data
        Buffer warningData{};
        getWarningDataPart(warningData);
        if (warningData.data() != nullptr) {
            mWarningInfoReq->set_warning_data(warningData.data(), warningData.size());              //std::string
        } else {
            LOG_D("mWarningData or mWarningData->data() is nullptr");
        }
    
        //RDG30-R-0929
        uint32_t sizeOfFileCounter {0U};
        sizeOfFileCounter += mWarningInfoReq->ByteSizeLong();
        LOG_D("Warning upload size (%d)", sizeOfFileCounter);

        //For Testing
        std::string str_Warning_Info_Upload{""};
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        (void)google::protobuf::util::MessageToJsonString(*mWarningInfoReq, &str_Warning_Info_Upload, option);
        LOG_D("Warning Info upload data: ");
        printData(str_Warning_Info_Upload);

        //Task Upload
        const uint32_t uploadId{UploadManager::getInstance()->genRequestId()};
        std::string file_dir{std::to_string(uploadId)};
        (void)file_dir.append("_UploadWarningTriggerData.dat");
        const error_t bStored{DataModel<UploadWarningInformationRequest>::save(file_dir, *mWarningInfoReq)};
        if(bStored == E_OK){
            const uint8_t operation{getOperation()};
            // Self-Diag
            DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        }
        const android::sp<UploadTask> task{new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG080);
        task->setUploadPatch(file_dir);   
        task->setUploadId(static_cast<uint64_t>(uploadId));
        /*Set priority*/
        task->setUploadPrio(DiagTrigger::PRIO_WARNING_TRIGGER);
        // test_saveUploadData = task;
        const uint64_t fileSize{static_cast<uint64_t>(mWarningInfoReq->ByteSizeLong())};
        task->setFileSize(fileSize);
        UploadManager::getInstance()->requestUploadTask(task);  //TODO

    } else {
        LOG_I("No upload warning information due to abort");  //RDG30-R-0708 - 2  + RDG30-R-1205
    }
    LOG_I("packageUploadData End");
}

void RemoteWarning::makeUploadErrorData(const vccomif::rdg::v1::interfaces::ResponseCode resCode){
    LOG_I("makeUploadErrorData");
    mWarningUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
    mColID = getCurrentColID();
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator itr{mSaveReq.find(mPriorityId)};
    //DiagTrigger::DiagTriggerType triggerType{};
    int64_t triggerOccurrenceTime{0};
    if (itr != mSaveReq.end())
    {
        //triggerType = itr->second->getType();
        triggerOccurrenceTime = itr->second->getTriggerTime();
    }
    else 
    {
	    LOG_D("Can not find triggerId: %d in mSaveReq", mPriorityId);
    }
    //Header
    mWarningUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    mWarningUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    mWarningUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
    const int32_t timezoneOffSet_hour {TimeManager::getInstance().getOffset() / 60};
    const int32_t timezoneOffSet_minute {TimeManager::getInstance().getOffset() % 60};
    LOG_D("Get timezone offset: %d:%d ", timezoneOffSet_hour, timezoneOffSet_minute);
    mWarningUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(timezoneOffSet_hour);
    mWarningUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(timezoneOffSet_minute); 
    mWarningUploadErrorData->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
    //MessageID
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    mWarningUploadErrorData->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, counterValue));
        //Content
    mWarningUploadErrorData->set_collection_condition_id(mColID);
    mWarningUploadErrorData->set_trigger_type(RdgProtoInterface::TriggerType::TT_WARNING_TRIGGER);  
    mWarningUploadErrorData->set_counter_value(counterValue);
    (void)counterValue;
    if (triggerOccurrenceTime > 0)
    {
        mWarningUploadErrorData->set_data_creation_date(static_cast<uint64_t>(triggerOccurrenceTime));
    }
    mWarningUploadErrorData->set_obd2_installed_flag(mApp.getOBDStatus());
        /*under_repair_flag*/
    mWarningUploadErrorData->set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
    mWarningUploadErrorData->set_response_code(resCode);
    mWarningUploadErrorData->set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_WARNING_TRIGGER);

    //RDG30-R-1042 + RDG30-R-1171
    //uint32_t sizeOfFileCounter {0U};
    //sizeOfFileCounter += mWarningUploadErrorData->ByteSizeLong();
    LOG_I("Warning upload size (%d)", mWarningUploadErrorData->ByteSizeLong());

    //For Testing
    std::string str_Warning_Error_upload{""};
    google::protobuf::util::JsonOptions option{};
    option.always_print_primitive_fields = true;
    option.preserve_proto_field_names = true;
    (void)google::protobuf::util::MessageToJsonString(*mWarningUploadErrorData, &str_Warning_Error_upload, option);
    LOG_D("Warning Error upload data: ");
    printData(str_Warning_Error_upload);

    const uint32_t uploadId {UploadManager::getInstance()->genRequestId()};
    std::string file_dir{std::to_string(uploadId)};
    (void)file_dir.append("_UploadErrorWarningTrigger.dat");
    const error_t bStored{DataModel<UploadErrorDataRequest>::save(file_dir, *mWarningUploadErrorData)};
    if(bStored == E_OK){
        const uint8_t operation{getOperation()};
        // Self-Diag
        DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
    }
    const android::sp<UploadTask> task{new UploadTask(uploadId)};
    task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
    task->setUploadPatch(file_dir);   //
    task->setUploadId(static_cast<uint64_t>(uploadId));
    /*Set priority*/
    task->setUploadPrio(DiagTrigger::PRIO_WARNING_TRIGGER);
    // test_saveUploadData = task;
    const uint64_t fileSize{static_cast<uint64_t>(mWarningUploadErrorData->ByteSizeLong())};
    task->setFileSize(fileSize);
    UploadManager::getInstance()->requestUploadTask(task);  //TODO
}

uint8_t RemoteWarning::getOperation() const noexcept
{
    uint8_t operation{DiagManagerAdapter::COLLECTION_CONDITIONS};
    if (mTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        operation = DiagManagerAdapter::WARINING_TRIGGER;
    }
    else if (mTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
    {
        operation = DiagManagerAdapter::COLLECTION_CONDITIONS;
    }
    else
    {
        operation = DiagManagerAdapter::COLLECTION_CONDITIONS;
    }
    return operation;
}
}
