#ifndef PRIORITY_CONTROL_H
#define PRIORITY_CONTROL_H

#include <deque>
#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>

#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "RemoteDelegate.h"
#include "DiagTrigger.h"
#include "DiagQueue.h"
#include "Remotediag.h"

namespace rdgapp {

class Remotediag;

class PriorityControl : public sl::Handler, public RemoteDelegate {
protected:
    bool checkPrecondition() noexcept override {return false;};
public:
    static constexpr uint8_t APP_ID {RDG_APPID::PRIORITYCONTROLS};
    PriorityControl(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper);
    ~PriorityControl() override;
    PriorityControl(const PriorityControl&) = default;
    PriorityControl(PriorityControl&&) = default;
    PriorityControl& operator=(const PriorityControl&) = default;
    PriorityControl& operator=(PriorityControl&&) = default;

    void notifyBootComplete() const noexcept override {};
    void onReceiveIG(const bool status) const override;
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) noexcept override {}
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0) noexcept override {};
    void onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) noexcept override {};
    void onRdgStop(const bool isStop) const noexcept override;
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final {return std::map<uint64_t, android::sp<UdsMessage>>();};
    uint8_t getAppId() const noexcept override {return APP_ID;};

    void requestTriggerProcess(const android::sp<DiagTrigger> pDiagTrigger);
    static android::sp<PriorityControl> getInstance();
    virtual void handleMessage(const android::sp<sl::Message>& handlemsg);
    void notifyTriggerProcessDone(const int32_t pTriggerId, const DiagTrigger::DiagTriggerType type);
    void notifyTriggerNoFound(const int32_t pTriggerId);
private:
    uint32_t getNumTaskQueue();
    // bool queueTaskEmpty() noexcept;
    uint32_t getDiagTriggerRange(const uint32_t priority) const noexcept;
    void addDiagTrigger(const DiagTrigger& trigger);
    void disCardDiagTrigger(const DiagTrigger& trigger) noexcept;
    void discardDueToIgOff();
    DiagTrigger getHighestPriorityTask();
    void discardLowestPriorityTask();
    void notifyStatus(const DiagTrigger& pDiagTrigger, const bool dueToIgOff = false) const;
    void resolvePriorityConflict(DiagTrigger& pTrigger);
    void popHighestPriorityTask();
    void popLowestPriorityTask();
    void handleStopRDG();
    void clearAllTask();

    std::deque<DiagTrigger>::iterator matchDoneTrigger(const int32_t pTriggerId, const DiagTrigger::DiagTriggerType triggerType);
    void dumpQueueTask();
    void setSelfDiagStopOpeartion(const DiagTrigger::DiagTriggerType type) const;

    static constexpr int32_t CMD_TRIGGER_DIAG_REQUEST {101};
    static constexpr int32_t CMD_TRIGGER_DIAG_DONE {102};
    static constexpr int32_t CMD_TRIGGER_DIAG_NOT_FOUND {103};
    static constexpr int32_t CMD_REQUEST_RESUME {105};
    static constexpr int32_t CMD_RECEIVE_IG_OFF_DISCARD {109};
    static constexpr int32_t CMD_TRIGGER_DIAG_REQUEST_DELAY {110};
    static constexpr int32_t CMD_STOP_RDG {111};

    static constexpr uint32_t MAX_TRIGGER_QUEUE {300U};
    /* 7 prio-queue for 7 type of diag func*/
    /*PRIO 0-9*/
    DiagQueue<DiagTrigger> queue1;
    /*PRIO 10 - OTA HIGH*/
    DiagQueue<DiagTrigger> queue_OTA_H;
    /*PRIO 11-19*/
    DiagQueue<DiagTrigger> queue3;
    /*PRIO 20 - WARNING*/
    DiagQueue<DiagTrigger> queue_Warning;
    /*PRIO 21-79*/
    DiagQueue<DiagTrigger> queue5;
    /*PRIO 80 - OTA LOW*/
    DiagQueue<DiagTrigger> queue_OTA_L;
    /*PRIO 81-255*/    
    DiagQueue<DiagTrigger> queue7;
    const Remotediag& mApp;
    static android::sp<PriorityControl> mPriorityControl;
    std::deque<DiagTrigger> processQueue;
    bool m_ota_non_interuptible;
    bool igOffDisscardOnProcessing;
    bool isRDGStop;
};
}
#endif // PRIORITY_CONTROL_H
