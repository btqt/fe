#include "PriorityControl.h"

namespace rdgapp {

android::sp<PriorityControl> PriorityControl::mPriorityControl{nullptr};
PriorityControl::PriorityControl(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper)
    : sl::Handler(privateLooper)
    , mApp(app)
    , m_ota_non_interuptible(false)
    , igOffDisscardOnProcessing(false)
{
    mPriorityControl = this;
}

PriorityControl::~PriorityControl() = default;

android::sp<PriorityControl> PriorityControl::getInstance()
{
    if (mPriorityControl == nullptr)
    {
        LOG_I("mPriorityControl is null");
        // mPriorityControl = new PriorityControl();
    }
    return mPriorityControl;
}

void PriorityControl::onReceiveIG(const bool status) const {
    if(status) {
        /* Do nothing */
    } else {
        /* TBD: ACtion in case IG_OFF*/
        (void)mPriorityControl->obtainMessage(CMD_RECEIVE_IG_OFF_DISCARD)->sendToTarget();
    }
}

void PriorityControl::disCardDiagTrigger(const DiagTrigger& trigger) noexcept {
    const uint32_t mPriority{static_cast<uint32_t>(trigger.getPriority())};
    const DiagTrigger::DiagTriggerFunc triggerFunc{trigger.getFunc()};
    /* If OTA low on procesing and DiagTrigger have prio < PRIO_OTA_LOW => delete OTA LOW in process queue and waiting queue */
    if(processQueue.empty() != true) {
        if((processQueue.front().getPriority() == DiagTrigger::PRIO_OTA_LOW) && 
            (mPriority < DiagTrigger::PRIO_OTA_LOW)) {
            /* Clear queue and waiting task */
            
            while(!queue_OTA_L.empty()) {
                queue_OTA_L.pop();
            }
            // /* If processing task is OTA LOW => discard task and notify discard*/
            // processQueue.front().changeState(DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED);
            // notifyStatus(processQueue.front());
        }
    }    
    /* If trigger is warning => delete previous waiting warning task */
    if((mPriority == DiagTrigger::PRIO_WARNING_TRIGGER) && (triggerFunc == DiagTrigger::DiagTriggerFunc::WARNING)) {
        /* Clear queue and notify */
        
        while(!queue_Warning.empty()) {
            queue_Warning.pop();
        }
        // if((processQueue.empty() != true) && (processQueue.front().getPriority() == DiagTrigger::PRIO_WARNING_TRIGGER)) {
        //     processQueue.front().changeState(DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED);
        //     notifyStatus(processQueue.front());
        // }
    }
    
}

void PriorityControl::discardDueToIgOff() {
    if(processQueue.empty() != true) {
        LOG_I("Discard processing unit due to IG OFF");
        /*Note: Processing trigger is discarded by specific function and priority do not 
        notify state change for processing task*/
        // processQueue.front().changeState(DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED);
        // notifyStatus(processQueue.front(), true);
        processQueue.clear();
    }

    LOG_I("Start discard Diag Trigger due to IG OFF");
    while(!queue1.empty()) {
        queue1.pop();
    }

    while(!queue_OTA_H.empty()) {
        queue_OTA_H.pop();
    }

    while(!queue3.empty()) {
        queue3.pop();
    }

    while(!queue_Warning.empty()) {
        queue_Warning.pop();
    }

    while(!queue5.empty()) {
        queue5.pop();
    }

    while(!queue_OTA_L.empty()) {
        queue_OTA_L.pop();
    }

    while(!queue7.empty()) {
        queue7.pop();
    }

    LOG_I("Check prio queue size after discard: %d", getNumTaskQueue());

}

uint32_t PriorityControl::getDiagTriggerRange(const uint32_t priority) const {
    uint32_t res{0U};
    if (priority < DiagTrigger::PRIO_OTA_HIGH) {
        res = 1U;
    } else if (priority == DiagTrigger::PRIO_OTA_HIGH) {
        res = 2U;
    } else if (priority < DiagTrigger::PRIO_WARNING_TRIGGER) {
        res = 3U;
    } else if (priority == DiagTrigger::PRIO_WARNING_TRIGGER) {
        res = 4U;
    } else if (priority < DiagTrigger::PRIO_OTA_LOW) {
        res = 5U;
    } else if (priority == DiagTrigger::PRIO_OTA_LOW) {
        res = 6U;
    } else if (priority <= DiagTrigger::PRIO_MAX) {
        res = 7U;
    } else {
        LOG_I("Priority is out of range");
    }
    return res;
}

void PriorityControl::addDiagTrigger(const DiagTrigger& trigger) {
    LOG_I("Add Trigger Type: %d | Func: %d | Prio: %d | ID: %d", 
        trigger.getType(), trigger.getFunc(), trigger.getPriority(), trigger.getTriggerId());
    const uint32_t mPriority{trigger.getPriority()};
    disCardDiagTrigger(trigger);
    if (mPriority < DiagTrigger::PRIO_OTA_HIGH) {
        queue1.push(trigger);
    } else if (mPriority == DiagTrigger::PRIO_OTA_HIGH) {
        queue_OTA_H.push(trigger);
    } else if (mPriority < DiagTrigger::PRIO_WARNING_TRIGGER) {
        queue3.push(trigger);
    } else if (mPriority == DiagTrigger::PRIO_WARNING_TRIGGER) {
        queue_Warning.push(trigger);
    } else if (mPriority < DiagTrigger::PRIO_OTA_LOW) {
        queue5.push(trigger);
    } else if (mPriority == DiagTrigger::PRIO_OTA_LOW) {
        queue_OTA_L.push(trigger);
    } else if (mPriority <= DiagTrigger::PRIO_MAX) {
        queue7.push(trigger);
    } else {
        LOG_I("DiagTrigger is not valid");
    }
    LOG_I("Size of DiagTrigger: %d", sizeof(trigger));
}

DiagTrigger PriorityControl::getHighestPriorityTask() {
    DiagTrigger result{DiagTrigger(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN, 
                        DiagTrigger::PRIO_MIN, 
                        DiagTrigger::DiagTriggerFunc::DIAG_FUNC_MIN, 
                        DiagTrigger::DEFAULT_DIAG_TRIGGER_ID)};
    if((m_ota_non_interuptible == true) && (queue_OTA_H.empty() != true)) {
        result = queue_OTA_H.top();
    }
    else if (!queue1.empty()) {
        result = queue1.top();
    } else if (!queue_OTA_H.empty()) {
        result = queue_OTA_H.top();
    } else if (!queue3.empty()) {
        result = queue3.top();
    } else if (!queue_Warning.empty()) {
        result = queue_Warning.top();
    } else if (!queue5.empty()) {
        result = queue5.top();
    } else if (!queue_OTA_L.empty()) {
        result = queue_OTA_L.top();
    } else if (!queue7.empty()) {
        result = queue7.top();
    } else {
        // Return a default task if all queues are empty
        LOG_I("Queue task is empty. Return default DiagTrigger");
    }
    return result;
}

void PriorityControl::discardLowestPriorityTask() {
    LOG_I("discardLowestPriorityTask");
    DiagTrigger result{DiagTrigger(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN, 
                    DiagTrigger::PRIO_MIN, 
                    DiagTrigger::DiagTriggerFunc::DIAG_FUNC_MIN, 
                    DiagTrigger::DEFAULT_DIAG_TRIGGER_ID)};
    bool check{false};
    if(!queue7.empty()) {
        result = queue7.back();
        check = true;
    } else if(!queue_OTA_L.empty()) {
        result = queue_OTA_L.back();
        check = true;
    } else if(!queue5.empty()) {
        result = queue5.back();
        check = true;
    } else if(!queue_Warning.empty()) {
        result = queue_Warning.back();
        check = true;
    } else if(!queue3.empty()) {
        result = queue3.back();
        check = true;
    } else if(!queue_OTA_H.empty()) {
        result = queue_OTA_H.back();
        check = true;
    } else if(!queue1.empty()) {
        result = queue1.back();
        check = true;
    } else {
        //do nothing
    }
    if(check == true) {
        result.changeState(DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED);
        notifyStatus(result);
        /*Delete lowest task in queue*/
        popLowestPriorityTask();
    } else {
        LOG_I("Priority queue is empty");
    }
}

void PriorityControl::popHighestPriorityTask() {
    LOG_I("popHighestPriorityTask");
    if((m_ota_non_interuptible == true) && (queue_OTA_H.empty() != true)) {
        queue_OTA_H.pop();
    }
    else if (!queue1.empty()) {
        queue1.pop();
    } else if (!queue_OTA_H.empty()) {
        queue_OTA_H.pop();
    } else if (!queue3.empty()) {
        queue3.pop();
    } else if (!queue_Warning.empty()) {
        queue_Warning.pop();
    } else if (!queue5.empty()) {
        queue5.pop();
    } else if (!queue_OTA_L.empty()) {
        queue_OTA_L.pop();
    } else if (!queue7.empty()) {
        queue7.pop();
    } else {
        LOG_I("Priority queue is empty");
    }
}

void PriorityControl::popLowestPriorityTask() {
    LOG_I("popLowestPriorityTask");
    if(!queue7.empty()) {
        queue7.pop_back();
    } else if(!queue_OTA_L.empty()) {
        queue_OTA_L.pop_back();
    } else if(!queue5.empty()) {
        queue5.pop_back();
    } else if(!queue_Warning.empty()) {
        queue_Warning.pop_back();
    } else if(!queue3.empty()) {
        queue3.pop_back();
    } else if(!queue_OTA_H.empty()) {
        queue_OTA_H.pop_back();
    } else if(!queue1.empty()) {
        queue1.pop_back();
    } else {
        LOG_I("Priority queue is empty");
    }
}

std::deque<DiagTrigger>::iterator PriorityControl::matchDoneTrigger(const int32_t pTriggerId, const DiagTrigger::DiagTriggerType triggerType) {
    // uint32_t pos {0U};
    std::deque<DiagTrigger>::iterator result{processQueue.end()};
    for (std::deque<DiagTrigger>::iterator it{processQueue.begin()}; it != processQueue.end(); ++it) {
        const uint32_t u_pTriggerId{(pTriggerId >= 0) ? static_cast<uint32_t>(pTriggerId) : 0U};
        if ((it->getTriggerId() == u_pTriggerId) && (it->getType() == triggerType)) {
            LOGI({"match done trigger: id: %d, type: %d"}, pTriggerId, triggerType);
            result = it;
            break;
        }
        // if (pos > 1U) {
        //     LOGE({"Invalid matching position. Trigger should be included within 2nd place"});
        //     break;
        // }
        // ++pos;
    }
    return result;
}

void PriorityControl::resolvePriorityConflict(DiagTrigger& pTrigger) {
    LOG_I("Trigger have Type: %d Func: %d Prio: %d ID: %d TriggerTime: %lld", 
        pTrigger.getType(), pTrigger.getFunc(), pTrigger.getPriority(), pTrigger.getTriggerId(), pTrigger.getTriggerTime());
    /* Check Diag trigger Queue boundary */
    const uint32_t numTask{getNumTaskQueue()};
    if(numTask > PriorityControl::MAX_TRIGGER_QUEUE) {
        LOG_I("Number of DiagTrigger exceed: %d", numTask);
        LOG_I("Delete lowest priority task");
        LOG_I("Discard lowest priority task");
        discardLowestPriorityTask();
    } else {
        LOG_I("Number of DiagTrigger: %d", numTask);
    }
    /* If queue size is 1*/
    LOG_I("Current highest prio task ID: %d Prio: %d", getHighestPriorityTask().getTriggerId(), getHighestPriorityTask().getPriority());

    if((processQueue.empty() == true)) {
        /* POP highest priority task into processing queue */
        LOG_I("Move highest prio task to process queue");
        const DiagTrigger diagTriggerValue{getHighestPriorityTask()};
        processQueue.push_back(diagTriggerValue);
        popHighestPriorityTask();
        
        /* Change diag trigger state and notify to diag*/
        if((processQueue.front().getState() != DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING)
            && (processQueue.front().getState() != DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED)
            && (processQueue.front().getState() != DiagTrigger::DiagTriggerState::TRIGGER_DONE)) 
        {
            if((processQueue.front().getPriority() == DiagTrigger::PRIO_OTA_HIGH)
                || (processQueue.front().getType() == DiagTrigger::DiagTriggerType::OTA_TRIGGER)) {
                m_ota_non_interuptible = true;
            } else {
                m_ota_non_interuptible = false;
            }
            processQueue.front().changeState(DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING);
            notifyStatus(processQueue.front());
        }
    } else {
        /* Compare processing vs new taskID */
        const uint32_t new_TaskID{pTrigger.getPriority()};
        const uint32_t procesing_TaskID{processQueue.front().getTriggerId()};
        if(new_TaskID == procesing_TaskID){
            /* new is not same processing */
            LOG_D("DiagTrigger ID: %d is on processing");
        } else {
        /* Compare processing task vs new task => suspend/discard/NOT */
        if((processQueue.front().getPriority() == DiagTrigger::PRIO_OTA_LOW) && 
            (pTrigger.getPriority() < DiagTrigger::PRIO_OTA_LOW)) {
            LOG_I("Discard OTA processing");
            /* If processing task is OTA LOW => discard task and notify discard*/
            processQueue.front().changeState(DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED);
            notifyStatus(processQueue.front());
        } else if((processQueue.front().getPriority() == DiagTrigger::PRIO_WARNING_TRIGGER) && 
            (pTrigger.getFunc() == DiagTrigger::DiagTriggerFunc::WARNING) &&
           (pTrigger.getPriority() == DiagTrigger::PRIO_WARNING_TRIGGER)) {
            LOG_I("Discard Warning processing");
            processQueue.front().changeState(DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED);
            notifyStatus(processQueue.front());
        } else {
            const uint32_t processingTask_rank{getDiagTriggerRange(processQueue.front().getPriority())};
            const uint32_t newTask_rank{getDiagTriggerRange(pTrigger.getPriority())};
            if(newTask_rank < processingTask_rank ) {
                /* suspend processing task if NOT m_ota_non_interuptible == false*/
                if(m_ota_non_interuptible == false) {
                    LOG_I("Suspending processing");
                    processQueue.front().changeState(DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED);
                    notifyStatus(processQueue.front());
                } else {
                    LOG_I("OTA on processing. Wait until OTA done");
                }
            } else {
                /* Notify TRIGGER_PENDING*/
                pTrigger.changeState(DiagTrigger::DiagTriggerState::TRIGGER_PENDING);
                notifyStatus(pTrigger);
            }
        }
    }
    dumpQueueTask();
    }
    /*TBD: Request Active to ApplicationMgr: mApp.notifyStart(APP_ID);*/
}

void PriorityControl::requestTriggerProcess(const android::sp<DiagTrigger> pDiagTrigger) {
    LOG_I("Receive request TriggerID: %d Prio: %d", pDiagTrigger->getTriggerId(),
        pDiagTrigger->getPriority());
    if(igOffDisscardOnProcessing != true) {
        /* Priority resolve and add task to queue*/
        addDiagTrigger(*pDiagTrigger);
        /* Obtain CMD_TRIGGER_DIAG_REQUEST*/
        (void)obtainMessage(CMD_TRIGGER_DIAG_REQUEST, pDiagTrigger)->sendToTarget();
    } else {
        LOG_I("Wait to complete IG OFF discard processing");
        (void)sendMessageDelayed(obtainMessage(CMD_TRIGGER_DIAG_REQUEST_DELAY, pDiagTrigger), 3000U);
    }
}

void PriorityControl::handleMessage(const android::sp<sl::Message>& handlemsg) {
    const int32_t what {handlemsg->what};
    LOG_I({"PriorityControl handle is processing with what: %d"}, what);
    switch (what) {
    case CMD_TRIGGER_DIAG_REQUEST: {
        LOG_I("CMD_TRIGGER_DIAG_REQUEST");
        sp<DiagTrigger> pTrigger {nullptr};
        handlemsg->getObject(pTrigger);
        if(pTrigger != nullptr) {
            resolvePriorityConflict(*pTrigger);            
        } else {
            LOG_I("pTrigger is nullptr");
        }
        break;
    }
    case CMD_TRIGGER_DIAG_REQUEST_DELAY: {
        LOG_I("CMD_TRIGGER_DIAG_REQUEST_DELAY");
        sp<DiagTrigger> pTrigger {nullptr};
        handlemsg->getObject(pTrigger);
        if(pTrigger != nullptr) {
            requestTriggerProcess(pTrigger);            
        } else {
            LOG_I("pTrigger is nullptr");
        }
        break;
    }
    case CMD_TRIGGER_DIAG_DONE: {
        /* Check matching triggerID */
        DiagTrigger::DiagTriggerType trigType{DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN};
        if ((handlemsg->arg2 > static_cast<int32_t>(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)) && (handlemsg->arg2 < static_cast<int32_t>(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)))
        {
            trigType = static_cast<DiagTrigger::DiagTriggerType>(handlemsg->arg2);
        }
        const std::deque<DiagTrigger>::iterator dTrigger{matchDoneTrigger(handlemsg->arg1, trigType)};

        if(dTrigger != processQueue.cend()) {
            LOG_I("Found Done Trigger Type: %d | Func: %d | Prio: %d | ID: %d | State: %d", 
                dTrigger->getType(), dTrigger->getFunc(), dTrigger->getPriority(), dTrigger->getTriggerId(), dTrigger->getState());
            if(dTrigger->getState() == DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED){
                LOG_I("TRIGGER_SUSPENDED");
                /*Move processing task back to queue*/
                const android::sp<DiagTrigger> tmp_Trigger{new DiagTrigger(dTrigger->getType(), dTrigger->getPriority(), dTrigger->getFunc(), dTrigger->getTriggerId())};
                addDiagTrigger(*tmp_Trigger);
                // if(processQueue.empty() != true) {
                //     LOG_D("ProcessQueue size before remove done trigger: %d", processQueue.size());
                //     (void)processQueue.erase(dTrigger);
                //     LOG_D("ProcessQueue size after remove done trigger: %d", processQueue.size());
                // }
                /*Don't break here*/
            }
            if((dTrigger->getState()<=DiagTrigger::DiagTriggerState::TRIGGER_STATE_MIN)
            || (dTrigger->getState()>=DiagTrigger::DiagTriggerState::TRIGGER_STATE_MAX)) {
                LOG_E("It is abnormal status");
            } else {
                if(processQueue.empty() != true) {
                    LOG_D("ProcessQueue size before remove done trigger: %d", processQueue.size());
                    (void)processQueue.erase(dTrigger);
                    LOG_D("ProcessQueue size after remove done trigger: %d", processQueue.size());
                } else {
                        
                }
                const DiagTrigger tmp_DiagTrigger_highest{getHighestPriorityTask()};
                LOG_I("Current highest prio task ID: %d Prio: %d", tmp_DiagTrigger_highest.getTriggerId()
                    , tmp_DiagTrigger_highest.getPriority());
                if(getNumTaskQueue() != 0U) {
                    /* Process next DIAG task*/
                    LOG_I({"DONE processing trigger. Process new one"});
                    const DiagTrigger tmp_DiagTrigger{getHighestPriorityTask()};
                    processQueue.push_back(tmp_DiagTrigger);
                    popHighestPriorityTask();
                    /* Change diag trigger state and notify to diag*/
                    if((processQueue.front().getState() == DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED)
                        || (processQueue.front().getState() == DiagTrigger::DiagTriggerState::TRIGGER_PENDING)) 
                    {
                        LOG_I("Check process queue item: %d", processQueue.size());
                        if((processQueue.front().getPriority() == DiagTrigger::PRIO_OTA_HIGH) 
                        || (processQueue.front().getType() == DiagTrigger::DiagTriggerType::OTA_TRIGGER)) {
                            m_ota_non_interuptible = true;
                        } else {
                            m_ota_non_interuptible = false;
                        }
                        processQueue.front().changeState(DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING);
                        notifyStatus(processQueue.front());
                    }
                }
            }
        } else {
            LOG_I("cannot find dTrigger in processQueue");
        }
        dumpQueueTask();
        break;
    }
    case CMD_TRIGGER_DIAG_NOT_FOUND: {
        LOG_I("CMD_TRIGGER_DIAG_NOT_FOUND");
        int32_t triggerId{0};
        triggerId = handlemsg->arg1;
        LOG_D("Process not found triggerID: %ld", triggerId);
        std::deque<DiagTrigger>::const_iterator result{processQueue.cend()};
        for (std::deque<DiagTrigger>::const_iterator it{processQueue.cbegin()}; it != processQueue.cend(); ++it) {
            const uint32_t u_pTriggerId{(triggerId >= 0) ? static_cast<uint32_t>(triggerId) : 0U};
            if (it->getTriggerId() == u_pTriggerId) {
                LOGI({"match done trigger: id: %d"}, triggerId);
                result = it;
                break;
            }
        }
        if(result != processQueue.cend())
        {
            LOG_D("ProcessQueue size before remove done trigger: %d", processQueue.size());
            (void)processQueue.erase(result);
            LOG_D("ProcessQueue size after remove done trigger: %d", processQueue.size());
            const DiagTrigger tmp_DiagTrigger_highest{getHighestPriorityTask()};
            LOG_I("Current highest prio task ID: %d Prio: %d", tmp_DiagTrigger_highest.getTriggerId()
                , tmp_DiagTrigger_highest.getPriority());
            if(getNumTaskQueue() != 0U) {
                /* Process next DIAG task*/
                LOG_I({"DONE processing trigger. Process new one"});
                const DiagTrigger tmp_DiagTrigger{getHighestPriorityTask()};
                processQueue.push_back(tmp_DiagTrigger);
                popHighestPriorityTask();
                /* Change diag trigger state and notify to diag*/
                if((processQueue.front().getState() == DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED)
                    || (processQueue.front().getState() == DiagTrigger::DiagTriggerState::TRIGGER_PENDING)) 
                {
                    LOG_I("Check process queue item: %d", processQueue.size());
                    if((processQueue.front().getPriority() == DiagTrigger::PRIO_OTA_HIGH) 
                    || (processQueue.front().getType() == DiagTrigger::DiagTriggerType::OTA_TRIGGER)) {
                        m_ota_non_interuptible = true;
                    } else {
                        m_ota_non_interuptible = false;
                    }
                    processQueue.front().changeState(DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING);
                    notifyStatus(processQueue.front());
                }
            }
        }
        else
        {
            LOG_D("Not found trigger is already deleted in queue");
        }
        break;
    }
    case CMD_RECEIVE_IG_OFF_DISCARD: {
        LOG_I("CMD_RECEIVE_IG_OFF_DISCARD");
        igOffDisscardOnProcessing = true;
        discardDueToIgOff();
        igOffDisscardOnProcessing = false;
        break;
    }
    case CMD_REQUEST_RESUME:
    {
        break;
    }
    default: {
        break;
    }
    }
}

uint32_t PriorityControl::getNumTaskQueue() {
    uint32_t result{0U};

    result += queue1.size();

    if (result <= UINT32_MAX - queue_OTA_H.size()) {
        result += queue_OTA_H.size();
    } else {
        LOG_E("queue over flow");
    }

    if (result <= UINT32_MAX - queue3.size()) {
        result += queue3.size();
    } else {
        LOG_E("queue over flow");
    }

    if (result <= UINT32_MAX - queue_Warning.size()) {
        result += queue_Warning.size();
    } else {
        LOG_E("queue over flow");
    }

    if (result <= UINT32_MAX - queue5.size()) {
        result += queue5.size();
    } else {
        LOG_E("queue over flow");
    }

    if (result <= UINT32_MAX - queue_OTA_L.size()) {
        result += queue_OTA_L.size();
    } else {
        LOG_E("queue over flow");
    }

    if (result <= UINT32_MAX - queue7.size()) {
        result += queue7.size();
    } else {
        LOG_E("queue over flow");
    }

    return result;
}

bool PriorityControl::queueTaskEmpty() noexcept {
    bool result{true};
    result = queue1.empty()
    && queue_OTA_H.empty()
    && queue3.empty()
    && queue_Warning.empty()
    && queue5.empty()
    && queue_OTA_L.empty()
    && queue7.empty();
    return result;
}

void PriorityControl::dumpQueueTask() {
    LOG_I("queue1 size: %d",queue1.size());
    LOG_I("queue_OTA_H size: %d",queue_OTA_H.size());
    LOG_I("queue3 size: %d",queue3.size());
    LOG_I("queue_Warning size: %d",queue_Warning.size());
    LOG_I("queue5 size: %d",queue5.size());
    LOG_I("queue_OTA_L size: %d",queue_OTA_L.size());
    LOG_I("queue7 size: %d",queue7.size());
    LOG_I("=======================================");
    LOG_I("processQueue size: %d", processQueue.size());
    if(processQueue.empty() != true) {
        LOG_I("Check processing task: ID: %d Prio: %d TriggerType: %d Func: %d"
            , processQueue.front().getTriggerId()
            , processQueue.front().getPriority()
            , processQueue.front().getType()
            , processQueue.front().getFunc());
    }
}
void PriorityControl::notifyTriggerProcessDone(const int32_t pTriggerId, const DiagTrigger::DiagTriggerType type) {
    /* Notify diag process is done to do next task*/
    (void)obtainMessage(CMD_TRIGGER_DIAG_DONE, pTriggerId, static_cast<int32_t>(type))->sendToTarget();
}

void PriorityControl::notifyTriggerNoFound(const int32_t pTriggerId) {
    (void)obtainMessage(CMD_TRIGGER_DIAG_NOT_FOUND, pTriggerId)->sendToTarget();
}

void PriorityControl::notifyStatus(const DiagTrigger& pDiagTrigger, const bool dueToIgOff) const {
    /* Notify diag trigger status appdate*/
    LOG_D("TriggerID: %lu TriggerType: %d TriggerFunc: %d TriggerState: %d", 
        pDiagTrigger.getTriggerId(), 
        static_cast<int32_t>(pDiagTrigger.getType()), 
        static_cast<int32_t>(pDiagTrigger.getFunc()), 
        static_cast<int32_t>(pDiagTrigger.getFunc()), 
        static_cast<int32_t>(pDiagTrigger.getState()));
    mApp.onNotifyStatus(pDiagTrigger, dueToIgOff);

}
}
