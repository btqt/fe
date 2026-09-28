#include <vector>
#include "Remotediag.h"

#include "diagprocess/UDS/UdsMessage.h"
#ifdef ENABLE_LGE_LXC
#include "utils/ProxyIpcServer.h"
#endif /* ENABLE_LGE_LXC */

namespace rdgapp {
android::sp<Application> gApp{};
android::sp<Remotediag> Remotediag::mRemotediag{nullptr};
Remotediag::Remotediag()
{
    mRdgCanRun = true;
    mRemotediag = this;
    mLooper = nullptr;
    mRemotediagHandler = nullptr;
    mIsBootCompleted = false;
    mIGStatus = false;
    mRDGActiveFlag = 0U;
    mDTCFlag = 0U;
    mSSRFlag = 0U;
    mWARflag = 0U;
    mRoBflag = 0U;
    mDDRflag = 0U;
    mUnderRepair = 0U;
    isDoOperationA.store(false, std::memory_order_seq_cst);
    initDLTLog();
}

android::sp<Remotediag> Remotediag::getInstance()
{
    if (mRemotediag == nullptr)
    {
        LOG_I("Remotediag is not created");
    }
    return mRemotediag;
}

void Remotediag::onCreate()
{
    LOG_I("Remotediag is onCreate");
    /* Init Remotediag handler*/
    if (mLooper == nullptr)
    {
        LOG_I("Initialize message queue and looper");
        mLooper = sl::SLLooper::myLooper();
        mLooper->setName({"RemoteDiag"});
    }
    LOG_I("Not receive Under Repair Status. Default Under Repair Status: %u", mUnderRepair);
    if (mRemotediagHandler == nullptr)
    {
        mRemotediagHandler = new RemotediagHandler(mLooper, *this);
    }
#ifdef ENABLE_LGE_LXC

    if (!ProxyIpcServer::getInstance().start())
    {
        LOG_E("Failed to start ProxyIpcServer at boot");
    }

    // Proxy callbacks are delivered over the IPC response/callback channel.
    
#endif /* ENABLE_LGE_LXC */
    makeFolder();
    /*Register ApplicationManagerAdapter */
    initRemoteDiagApp();
    ApplicationManagerAdapter::getInstance()->registerService();
}

void Remotediag::doBootCompleted()
{
#ifdef ENABLE_LGE_LXC
    if (mIsBootCompleted)
    {
        LOG_I("doBootCompleted already handled");
        return;
    }

#endif /* ENABLE_LGE_LXC */
    LOG_I("doBootCompleted");
    mIsBootCompleted = true;
    /* Initialize and Register Tiger services */
    /*Register DiagManagerAdapter */
    DiagManagerAdapter::getInstance()->registerService();
    /*Register CalibManagerAdapter */
    CalibManagerAdapter::getInstance()->registerService();
    /*Register HttpManagerAdapter */
    HttpManagerAdapter::getInstance()->registerService();
    /*Register LocationManagerAdapter */
    LocationManagerAdapter::getInstance()->registerService();

    MqttManagerAdapter::getInstance()->registerService();
    OnboardclientAdapter::getInstance()->registerService();
    /*Register PowerManagerAdapter */
    PowerManagerAdapter::getInstance()->registerService();
    /*Register PPIManagerAdapter */
    PPIManagerAdapter::getInstance()->registerService();
    VehicleManagerAdapter::getInstance()->registerService();
    /*Register RegionManagerAdapter */
    RegionManagerAdapter::getInstance()->registerService();
    SomeipManagerAdapter::getInstance()->registerService();
    RemoteOTA::getInstance().startFaServer();
    (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_CMD_INIT_APP)->sendToTarget();
}

void Remotediag::makeFolder()
{
    int32_t flagCheck{0};
    /*...........................S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH */
    flagCheck = mkdir(DATA_PATH.c_str(), 0x1C0U  | 0x20U   | 0x08U   | 0x04U   | 0x01U);
    if (flagCheck != 0)
    {
        LOG_D("Create folder %s is fail", DATA_PATH.c_str());
    }
    else
    {
        LOG_D("Create folder %s is success", DATA_PATH.c_str());
    }
}

void Remotediag::handleFeatureStatusChange(const std::string feature, const FeatureStatus status) {
    LOG_I("Check feature: %s status: %d", feature.c_str(), status);
    const std::map<std::string, int32_t>::iterator ptr_Feature{mWaitingList.find(feature)};
    if(ptr_Feature != mWaitingList.end()) {
        if (status == FeatureStatus::OFF) {
            (void)mWaitingList.erase(feature);
            // UploadManager::getInstance()->operationA();
            const android::sp<sl::Handler> mCollHandler{CollectionCondition::getInstance().getHandler()};
            if (mCollHandler != nullptr)
            {
                (void)mCollHandler->obtainMessage(CollectionCondition::CMD_FEATURE_STATUS_CHANGE)->sendToTarget();
            }
            
        }
    } else {
        LOG_I("Do not have feature: %s in saveList", feature.c_str());
    }
}

void Remotediag::onDestroy()
{
    LOG_I("Remotediag is onDestroy");
}

uint8_t Remotediag::getIGStatus() const
{
    /* IG status */
    return (PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_ON) ? 1U : 0U;
    // return mIGStatus;
}

bool Remotediag::getInternalIGStatus() const noexcept
{
    return mIGStatus;
}

uint8_t Remotediag::getUnderRepair() const noexcept
{
    const Mutex::Autolock lock{Mutex::Autolock(mMutexUnderRepair)};
    return mUnderRepair;
}

void Remotediag::setUnderRepair(const uint8_t status) noexcept
{
    const Mutex::Autolock lock{Mutex::Autolock(mMutexUnderRepair)};
    mUnderRepair = status;
}

void Remotediag::notifyReceiveIG(const bool status)
{
    LOG_I("IG value is changed to %s", status ? "ON" : "OFF");
    mIGStatus = status;
    for (RemoteDelegate::Interator it{mDelegate.begin()}; it != mDelegate.end(); it++)
    {
        it->second->onReceiveIG(status);
    }
    mSchedMgr->onReceiveIG(status);
    const android::sp<sl::Handler> mCollHandler{CollectionCondition::getInstance().getHandler()};
    if (mCollHandler != nullptr)
    {
        (void)mCollHandler->obtainMessage(CollectionCondition::CMD_HANDLE_IG_STATUS_CHANGE, (status ? static_cast<int32_t>(1) : static_cast<int32_t>(0)))->sendToTarget();
    }
    UploadManager::getInstance()->onReceiveIG(status);
}

void Remotediag::notifyReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo)
{
    const uint8_t eventType{responseEventInfo->resInfo()->responseType()};
    OBCEnum::OBCUdsResponseType resType{OBCEnum::OBCUdsResponseType::UNKOWN};
    if (eventType <= static_cast<uint8_t>(OBCEnum::OBCUdsResponseType::OBC_RES_MAX))
    {
        resType = static_cast<OBCEnum::OBCUdsResponseType>(eventType);
    }
    (void)eventType;
    const android::sp<::Buffer> udsData{responseEventInfo->resInfo()->udsData()};
    const android::sp<UdsMessage> udsResponse{new UdsMessage()};
    if (udsData->size() > 0U)
    {
        (void)udsResponse->Parser(udsData);
    }
    if (resType == OBCEnum::OBCUdsResponseType::EVENT)
    {
        RoBOccurrence::getInstance()->onOccurrenceRoBReceived(responseEventInfo, udsResponse);
    }
    else
    {
        for (RemoteDelegate::Interator it{mDelegate.begin()}; it != mDelegate.end(); it++)
        {
            it->second->onReceiveUDS(responseEventInfo, udsResponse);
        }
    }
}

void Remotediag::notifyReceiveGrpcRes(const android::sp<GrpcResData> &pGrpcResData) const
{
    const GRPC_IF_TYPE msgType{pGrpcResData->getInterfaceType()};
    LOG_I("notifyReceiveGrpcRes, msgType=%d", static_cast<uint8_t>(msgType));
    switch (msgType)
    {
    // DCIF-RDG010_get_collection_condition_response.proto
    case GRPC_IF_TYPE::DCIF_RDG010:
    {
        LOG_I("GRPC_IF_TYPE::DCIF_RDG010");
        const android::sp<sl::Handler> mCollHandler{CollectionCondition::getInstance().getHandler()};
        if (mCollHandler != nullptr)
        {
            (void)mCollHandler->obtainMessage(CollectionCondition::CMD_RECEIVE_COLLECTION_CONDITION_REQUEST_RESPONSE, pGrpcResData)->sendToTarget();
        }
        break;
    }
    // DCIF-RDG012_notify_collection_condition_update_result_response.proto
    case GRPC_IF_TYPE::DCIF_RDG012:
    {
        LOG_I("GRPC_IF_TYPE::DCIF_RDG012");
        const android::sp<sl::Handler> mCollHandler{CollectionCondition::getInstance().getHandler()};
        if (mCollHandler != nullptr)
        {
            (void)mCollHandler->obtainMessage(CollectionCondition::CMD_RECEIVE_COLLECTION_CONDITION_UPDATE_RESULT_RESPONSE, pGrpcResData)->sendToTarget();
        }
        break;
    }
    case GRPC_IF_TYPE::DCIF_RDG030:
    case GRPC_IF_TYPE::DCIF_RDG040:
    case GRPC_IF_TYPE::DCIF_RDG050:
    case GRPC_IF_TYPE::DCIF_RDG060:
    case GRPC_IF_TYPE::DCIF_RDG070:
    case GRPC_IF_TYPE::DCIF_RDG080:
    case GRPC_IF_TYPE::DCIF_RDG100:
    case GRPC_IF_TYPE::DCIF_RDG120:
    case GRPC_IF_TYPE::DCIF_RDG130:
    case GRPC_IF_TYPE::DCIF_RDG160:
    {
        UploadManager::getInstance()->receivedGrpcRes(pGrpcResData);
        break;
    }
    default:
        LOG_I("GRPC_IF_TYPE::OTHER");
        break;
    }
}

void Remotediag::notifyChangedRemoteStatus(const int32_t what, const int32_t info)
{
    for (RemoteDelegate::Interator it{mDelegate.begin()}; it != mDelegate.end(); it++)
    {
        it->second->onChangedRemoteInfo(what, info);
    }
}

void Remotediag::notifyLastOpComplTime(const uint64_t schedIndex, const int64_t completeTime, const bool isCompleted) const
{
    LOG_I("notifyLastOpComplTime");
    mSchedMgr->notifySchedComplete(schedIndex, completeTime, isCompleted);
}

void Remotediag::onNotifyStatus(const DiagTrigger &pDiagTrigger, const bool dueToIgOff) const
{
    LOG_I("onNotifyStatus");
    LOG_I("Trigger notified Type: %d | Func: %d | Prio: %d | ID: %d due to IG off: %d",
          pDiagTrigger.getType(), pDiagTrigger.getFunc(), pDiagTrigger.getPriority(), pDiagTrigger.getTriggerId(), dueToIgOff);
    /* Obtain MSG_NOTIFY_DIAG_TRIGGER*/
    if(pDiagTrigger.getTriggerId() <= static_cast<uint32_t>(INT32_MAX))
    {
        const DiagTrigger::DiagTriggerState state{pDiagTrigger.getState()};
        if ((state > DiagTrigger::DiagTriggerState::TRIGGER_STATE_MIN) && (state < DiagTrigger::DiagTriggerState::TRIGGER_STATE_MAX))
        {
            if (dueToIgOff == true)
            {
                (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_DIAG_TRIGGER_DISCARD_IGOFF,
                                            static_cast<int32_t>(pDiagTrigger.getFunc()),
                                            static_cast<int32_t>(state),
                                            static_cast<int32_t>(pDiagTrigger.getTriggerId()))
                ->sendToTarget();
            }
            else
            {
                (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_DIAG_TRIGGER,
                                            static_cast<int32_t>(pDiagTrigger.getFunc()),
                                            static_cast<int32_t>(state),
                                            static_cast<int32_t>(pDiagTrigger.getTriggerId()))
                ->sendToTarget();
            }

        }
        else
        {
            LOG_E("invalid state");
        }
    }
    else{
        // do nothing
    }
    (void) dueToIgOff;
}

void Remotediag::notifyTriggerStatus(const DiagTrigger::DiagTriggerFunc &pFunc, const DiagTrigger::DiagTriggerState &pState,
                                     const int32_t &pTriggerId, const bool dueToIgOff)
{
    LOG_I("notifyTriggerStatus");
    LOG_I("Trigger notified Func: %d | State: %d | ID: %d | dueToIgOff: %d", pFunc, pState, pTriggerId, dueToIgOff);
    switch (pFunc)
    {
    case DiagTrigger::DiagTriggerFunc::DIAG_FUNC_MIN:
    {
        LOG_I("DiagTriggerFunc::DIAG_FUNC_MIN");
        break;
    }
    case DiagTrigger::DiagTriggerFunc::WARNING:
    {
        LOG_I("DiagTriggerFunc::WARNING");
        (void)mWarning->notifyTrigger(pState, pTriggerId, dueToIgOff);
        break;
    }
    case DiagTrigger::DiagTriggerFunc::DTC:
    {
        LOG_I("DiagTriggerFunc::DTC");
        (void)mDTC->notifyTrigger(pState, pTriggerId, dueToIgOff);
        break;
    }
    case DiagTrigger::DiagTriggerFunc::SSR:
    {
        LOG_I("DiagTriggerFunc::SSR");
        (void)mSSR->notifyTrigger(pState, pTriggerId, dueToIgOff);
        break;
    }
    case DiagTrigger::DiagTriggerFunc::ROB:
    {
        LOG_I("DiagTriggerFunc::ROB");
        (void)mRoB->notifyTrigger(pState, pTriggerId, dueToIgOff);
        break;
    }
    // case DiagTrigger::DiagTriggerFunc::ALLDIAG:
    // {
    //     LOG_I("DiagTriggerFunc::ALLDIAG");
    //         (void)mDTC->notifyTrigger(pState, pTriggerId, dueToIgOff);
    //         (void)mSSR->notifyTrigger(pState, pTriggerId, dueToIgOff);
    //         (void)mRoB->notifyTrigger(pState, pTriggerId, dueToIgOff);
    //     break;
    // }
    case DiagTrigger::DiagTriggerFunc::ALLROB:
    {
        LOG_I("DiagTriggerFunc::ALLROB");
        (void)mRoB->notifyTrigger(pState, pTriggerId, dueToIgOff);
        break;
    }
    case DiagTrigger::DiagTriggerFunc::DIRECT_COMMAND:
    {
        LOG_I("DiagTriggerFunc::DIRECT_COMMAND");
        (void)mRemoteDirectCommand->notifyTrigger(pState, pTriggerId, dueToIgOff);
        break;
    }
    case DiagTrigger::DiagTriggerFunc::ROBSSR:
    {
        LOG_I("DiagTriggerFunc::ROBSSR");
        if(pTriggerId>=0)
        {
            mRoBSSR->notifyTrigger(pState, static_cast<uint32_t>(pTriggerId), dueToIgOff);
        }
        else{
            // print error log
        }
        break;
    }
    case DiagTrigger::DiagTriggerFunc::ROB_MORNITOR:
    {
        LOG_I("DiagTriggerFunc::ROB_MORNITOR");
        (void)mMonitoring->notifyTrigger(pState, pTriggerId, dueToIgOff);
        break;
    }
    case DiagTrigger::DiagTriggerFunc::ECU_UPDATE_INFO:
    {
        LOG_I("DiagTriggerFunc::ECU_UPDATE_INFO");
        if(pTriggerId>=0)
        {
            mEcuInformation->notifyTrigger(pState, static_cast<uint32_t>(pTriggerId), dueToIgOff);
        }
        else{
            // print error log
        }
        break;
    }
    case DiagTrigger::DiagTriggerFunc::OTA:
    {
        LOG_I("DiagTriggerFunc::OTA");
        (void)mOTA->notifyTrigger(pState, pTriggerId, dueToIgOff);
        break;
    }
    case DiagTrigger::DiagTriggerFunc::DIAG_FUNC_MAX:
    {
        LOG_I("DiagTriggerFunc::DIAG_FUNC_MA");
        break;
    }
    default:
    {
        LOG_I("default");
        break;
    }
    }
}

void Remotediag::initRemoteDiagApp()
{
    LOG_I("initRemoteDiagApp");
    /* Create looper for diagnostic*/
    android::sp<sl::SLLooper> diagnosticsLooper{new sl::SLLooper()};
    diagnosticsLooper->setName({"Diagnostics"});
    (void)diagnosticsLooper->prepare();
    (void)diagnosticsLooper->start(false);

    CollectionCondition::getInstance().init(diagnosticsLooper);
    mCollectionCondition = (&CollectionCondition::getInstance());

    mSSR = new RemoteSSR(*this, diagnosticsLooper);
    mDTC = new RemoteDTC(*this, diagnosticsLooper);

    RemoteRoB::getInstance().init(this, diagnosticsLooper);
    mRoB = &(RemoteRoB::getInstance());

    RemoteOTA::getInstance().init(diagnosticsLooper);
    mOTA = &(RemoteOTA::getInstance());

    mRemoteLastUpload = new RemoteLastUpload(*this, diagnosticsLooper);
    mRemoteDirectCommand = new RemoteDirectCommand(*this, diagnosticsLooper);
    mPriorityControl = new PriorityControl(*this, diagnosticsLooper);
    mEcuInformation = new RemoteEcuInformation(*this, diagnosticsLooper);
    mRoBSSR = new RemoteRoBSSR(*this, diagnosticsLooper);
    mSchedMgr = new SchedulerManager(*this, diagnosticsLooper);
    mMonitoring = new RoBMonitoring(*this, diagnosticsLooper);
    /* Create looper for vehicle trigger*/
    android::sp<sl::SLLooper> vehicleTriggerLooper{new sl::SLLooper()};
    vehicleTriggerLooper->setName({"VehicleTrigger"});
    (void)vehicleTriggerLooper->prepare();
    (void)vehicleTriggerLooper->start(false);

    mWarning = new RemoteWarning(*this, vehicleTriggerLooper);
    mRoBOccurrence = new RoBOccurrence(*this, vehicleTriggerLooper);

    /* Create looper for uploader*/
    android::sp<sl::SLLooper> UploaderLooper{new sl::SLLooper()};
    UploaderLooper->setName({"UploaderLooper"});
    (void)UploaderLooper->prepare();
    (void)UploaderLooper->start(false);
    mUploader = new UploadManager(*this, UploaderLooper);

    TriggerIDGenerator::getInstance().setInit();
    mDelegate[mSSR->getAppId()] = mSSR.get();
    mDelegate[mDTC->getAppId()] = mDTC.get();
    mDelegate[mRoB->getAppId()] = mRoB.get();
    mDelegate[mOTA->getAppId()] = mOTA.get();
    mDelegate[mWarning->getAppId()] = mWarning.get();
    mDelegate[mRemoteDirectCommand->getAppId()] = mRemoteDirectCommand.get();
    mDelegate[mPriorityControl->getAppId()] = mPriorityControl.get();
    mDelegate[mEcuInformation->getAppId()] = mEcuInformation.get();
    mDelegate[mRoBSSR->getAppId()] = mRoBSSR.get();
    mDelegate[mRoBOccurrence->getAppId()] = mRoBOccurrence.get();
    mDelegate[mMonitoring->getAppId()] = mMonitoring.get();

    mRemotediagInitComplete = true;
}

void Remotediag::initApp() {
    LOG_D("initApp");
    /*Init APP complete -> do next task*/
    /*Check IG status and notify IG*/
    if (PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_ON)
    {
        (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_ON)->sendToTarget();
    }
    /*Get PPI flag 0x1022*/
    mSRVC_AC = DiagManagerAdapter::getInstance()->getSRVC_AC();
    mSRVC_VC = DiagManagerAdapter::getInstance()->getSRVC_VC();
    mSRVC_PC = DiagManagerAdapter::getInstance()->getSRVC_PC();
    mSRVC_STT = DiagManagerAdapter::getInstance()->getSRVC_STT();
    LOG_I("Check mSRVC_AC: %d mSRVC_VC: %d mSRVC_PC: %d mSRVC_STT: %d", mSRVC_AC, mSRVC_VC, mSRVC_PC, mSRVC_STT);
    LOG_I("Check RDGFlag: %d DTCFlag: %d SSRFlag: %d WARflag: %d RoBflag: %d DDRflag: %d",
          DiagManagerAdapter::getInstance()->getRDGFlag(),
          DiagManagerAdapter::getInstance()->getDTCFlag(),
          DiagManagerAdapter::getInstance()->getSSRFlag(),
          DiagManagerAdapter::getInstance()->getWARflag(),
          DiagManagerAdapter::getInstance()->getRoBflag(),
          DiagManagerAdapter::getInstance()->getDDRflag());
}

void Remotediag::onPostReceived(const android::sp<::Post> &systemPost)
{
    const int32_t what{systemPost->what};
    LOG_I("Remotediag::onPostReceived %d", what);
    constexpr static int32_t MSG_SLDD_TEST_INIT{10000};
    constexpr static int32_t MSG_SLDD_TEST_END{10100};
    if ((what >= MSG_SLDD_TEST_INIT) && (what <= MSG_SLDD_TEST_END))
    {
        LOG_I("Receive System Post to test RemoteDiag");
        RemoteDiagSLDD::getInstance()->runRemoteDiagSlddTesting(what, systemPost->arg1, systemPost->arg2);
    }
    (void) MSG_SLDD_TEST_END;
}

void Remotediag::triggerWarningToDTC(const uint32_t triggerID, const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData, const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority) const
{
    /*if DTC flag off -> last upload*/
    bool dtcFlag{false};
    dtcFlag = DiagManagerAdapter::getInstance()->getDTCFlag()> 0U;
    if (dtcFlag == true)
    {
        LOG_I("DTC was notified with TriggerType: %d", triggerType);
        LOG_I("DTC was notified with Timedata: %lld", timeData);
        LOG_I("DTC was notified with Location: 0x%08X 0x%08X", location->getLatitude(), location->getLongtitude());
        LOG_I("DTC was notified with Collection ID: %llu", collectionId);
        LOG_I("DTC was notified with priority: %d ", priority);
        mDTC->triggerFromWarning(triggerID, triggerType, timeData, location, collectionId, priority);
    }
    else
    {
        LOG_I("Trigger LastUpload");
        triggerLastUpload(triggerID, triggerType, timeData, location, collectionId, priority);
        mRemoteLastUpload->makeUploadData();
        if(triggerID > static_cast<uint32_t>(INT32_MAX))
        {
            LOG_E("Invalid Trigger ID");
        }
        PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(triggerID), triggerType);
    }
}
void Remotediag::triggerDTCToSSR(const uint32_t triggerID, const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData,const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority,const std::vector<pair<uint32_t, uint32_t>> v_targetEcu) const
{
    LOG_I("SSR was notified with TriggerID: %u", triggerID);
    LOG_I("SSR was notified with TriggerType: %d", triggerType);
    LOG_I("SSR was notified with Timedata: %lld", timeData);
    LOG_I("SSR was notified with Location: 0x%08X 0x%08X", location->getLatitude(), location->getLongtitude());
    LOG_I("SSR was notified with Collection ID: %llu", collectionId);
    LOG_I("SSR was notified with priority: %d ", priority);
    mSSR->triggerFromDTC(triggerID, triggerType, timeData, location, collectionId, priority, v_targetEcu);
}
void Remotediag::forwardWarningToDtc(const DiagTrigger::DiagTriggerState pState, const int32_t pTriggerId, const bool dueToIgOff) const
{
    LOG_I("DTC was notified with TriggerID: %u", pTriggerId);
    LOG_I("DTC was notified with TriggerState: %d", pState);
    LOG_I("DTC was notified with dueToIgOff: %d", dueToIgOff);
    (void)mDTC->notifyTrigger(pState, pTriggerId, dueToIgOff);
}
void Remotediag::forwardDtcToSsr(const DiagTrigger::DiagTriggerState pState, const int32_t pTriggerId, const bool dueToIgOff) const
{
    LOG_I("Remotediag::forwardDtcToSsr");
    LOG_I("SSR was notified with TriggerID: %u", pTriggerId);
    LOG_I("SSR was notified with TriggerState: %d", pState);
    LOG_I("SSR was notified with dueToIgOff: %d", dueToIgOff);
    (void)mSSR->notifyTrigger(pState, pTriggerId, dueToIgOff);
}

void Remotediag::forwardSsrToRob(const DiagTrigger::DiagTriggerState pState, const int32_t pTriggerId, const bool dueToIgOff) const
{
    LOG_I("Remotediag::forwardSsrToRob");
    LOG_I("RoB was notified with TriggerID: %u", pTriggerId);
    LOG_I("RoB was notified with TriggerState: %d", pState);
    LOG_I("RoB was notified with dueToIgOff: %d", dueToIgOff);
    (void)mRoB->notifyTrigger(pState, pTriggerId, dueToIgOff);
}

void Remotediag::triggerSSRToRoB(const uint32_t triggerID, const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData,const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority) const
{
    /*if RoB flag off -> last upload*/
    bool robFlag{false};
    robFlag = (DiagManagerAdapter::getInstance()->getRoBflag()>0U)?true:false;
    if (robFlag == true)
    {
        LOG_I("Remotediag::triggerSSRToRoB");
        LOG_I("RoB was notified with TriggerID: %u", triggerID);
        LOG_I("RoB was notified with TriggerType: %d", triggerType);
        LOG_I("RoB was notified with Timedata: %lld", timeData);
        LOG_I("RoB was notified with Location: 0x%08X 0x%08X", location->getLatitude(), location->getLongtitude());
        LOG_I("RoB was notified with Collection ID: %llu", collectionId);
        LOG_I("RoB was notified with priority: %d ", priority);
        mRoB->triggerFromSSR(triggerID, triggerType, timeData, location, collectionId, priority);
    }
    else
    {
        LOG_I("Trigger LastUpload");
        LOG_I("RoB Done");
        triggerLastUpload(triggerID, triggerType, timeData, location, collectionId, priority);
        notifyDiagDoneToLastUpload(triggerID);
        /*Notify trigger process done*/
        if(triggerID > static_cast<uint32_t>(INT32_MAX))
        {
            LOG_E("Invalid Trigger ID");
        }
        PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(triggerID), triggerType);
    }
}

void Remotediag::notifyDiagDoneToLastUpload(const uint32_t triggerID) const
{
    mRemoteLastUpload->receiveProcessDone(triggerID);
}

void Remotediag::triggerLastUpload(const uint32_t triggerID, const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData,const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority) const
{
    LOG_I("Start trigger last upload");
    mRemoteLastUpload->triggerLastUpload(triggerID, triggerType, timeData, location,collectionId, priority);
}

void Remotediag::receiveCenterCommnad(const android::sp<CenterReqData> pCenterReqData)
{
    LOG_I("Receive Center request data ID: %llu messageID: %d Prio: %d TriggerType: %d", pCenterReqData->getCenterReq_CollectionID(), pCenterReqData->getCenterReq_messageID(), pCenterReqData->getCenterReq_prio(), pCenterReqData->getTrigger_type());
    /*Check messageID to forward to proper AppID*/
    const uint8_t messageID{pCenterReqData->getCenterReq_messageID()};
    uint8_t sendingAppId{0U};
    switch (messageID)
    {
    case MSG_ID_CENTERREQUESTALLDTCSSR:
    {
        LOG_I("MSG_ID_CENTERREQUESTALLDTCSSR->trigger to All Diag");
        sendingAppId = mDTC->getAppId();
        break;
    }
    case MSG_ID_CENTERREQUESTALLROB:
    {
        LOG_I("MSG_ID_CENTERREQUESTALLROB");
        sendingAppId = mRoB->getAppId();
        break;
    }
    case MSG_ID_CENTERREQUESTROBSSR:
    {
        LOG_I("MSG_ID_CENTERREQUESTROBSSR");
        sendingAppId = mRoBSSR->getAppId();
        break;
    }
    case MSG_ID_COLLECTIONCONDITIONROBROBSSRDIDEVENT:
    {
        LOG_I("MSG_ID_COLLECTIONCONDITIONROBROBSSRDIDEVENT");
        sendingAppId = mMonitoring->getAppId();
        break;
    }
    case MSG_ID_CENTERREQUESTECUINFORMATION:
    case MSG_ID_COLLECTIONCONDITIONECUINFORMATION:
    {
        LOG_I("MSG_ID_CENTERREQUESTECUINFORMATION");
        sendingAppId = mEcuInformation->getAppId();
        break;
    }
    case MSG_ID_CENTERREQUESTDIRECTCOMMAND:
    case MSG_ID_COLLECTIONCONDITIONDIRECTCOMMAND:
    {
        LOG_I("MSG_ID_CENTERREQUESTDIRECTCOMMAND");
        sendingAppId = mRemoteDirectCommand->getAppId();
        break;
    }
    case MSG_ID_COLLECTIONCONDITIONWARNINGINFORMATION:
    {
        LOG_I("MSG_ID_COLLECTIONCONDITIONWARNINGINFORMATION");
        sendingAppId = mWarning->getAppId();
        break;
    }
    default:
    {
        LOG_I("default: unknown messageID %u", messageID);
        return;
    }
    }
    /*TBD: Check precondition*/
    mDelegate[sendingAppId]->onCenterCommandForward(pCenterReqData);
}

void Remotediag::loadNewCenterRequest()
{
    LOG_I("loadNewCenterRequest");
    /* Check RDG active flag */
    mRDGActiveFlag = DiagManagerAdapter::getInstance()->getRDGFlag();
    /* Get DTC/SSR/ROB/WAR/DDR flag */
    const std::shared_ptr<vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CollectionConditionDiagCommon>
        ptrCollectionConditionDiagCommon{CollectionCondition::getInstance().getCollectionConditionDiagCommon()};
    uint8_t RDGFlag_data{0U};
    RDGFlag_data = RDGFlag_data | static_cast<uint8_t>(mRDGActiveFlag << 7U);
    if (ptrCollectionConditionDiagCommon != nullptr)
    {
        mDTCFlag = (ptrCollectionConditionDiagCommon->dtc_flag() == true) ? 1U : 0U;
        mSSRFlag = (ptrCollectionConditionDiagCommon->ssr_flag() == true) ? 1U : 0U;
        mWARflag = (ptrCollectionConditionDiagCommon->war_flag() == true) ? 1U : 0U;
        mRoBflag = (ptrCollectionConditionDiagCommon->rob_flag() == true) ? 1U : 0U;
        mDDRflag = (ptrCollectionConditionDiagCommon->ddr_flag() == true) ? 1U : 0U;
    }
    else
    {
        mDTCFlag = 0U;
        mSSRFlag = 0U;
        mWARflag = 0U;
        mRoBflag = 0U;
        mDDRflag = 0U;
    }
    RDGFlag_data = RDGFlag_data | static_cast<uint8_t>(mDTCFlag << 6U);
    RDGFlag_data = RDGFlag_data | static_cast<uint8_t>(mSSRFlag << 5U);
    RDGFlag_data = RDGFlag_data | static_cast<uint8_t>(mWARflag << 4U);
    RDGFlag_data = RDGFlag_data | static_cast<uint8_t>(mRoBflag << 3U);
    RDGFlag_data = RDGFlag_data | static_cast<uint8_t>(mDDRflag << 2U);
    RDGFlag_data = RDGFlag_data | static_cast<uint8_t>((DiagManagerAdapter::getInstance()->getSRVC_AC() == true ? 0x01U : 0U) << 1U);
    RDGFlag_data = RDGFlag_data | static_cast<uint8_t>(DiagManagerAdapter::getInstance()->getSRVC_STT() == true ? 0x01U : 0U);
    const uint8_t mArray[2]{RDGFlag_data, 0x00U} ;
    const android::sp<::Buffer> spBuf{new ::Buffer()};
    spBuf->setTo(&mArray[0], sizeof(mArray));
    DiagManagerAdapter::getInstance()->writeDidData(DiagManagerAdapter::PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);
    LOG_I("Check RDGFlag: %d DTCFlag: %d SSRFlag: %d WARflag: %d RoBflag: %d DDRflag: %d",
          DiagManagerAdapter::getInstance()->getRDGFlag(),
          DiagManagerAdapter::getInstance()->getDTCFlag(),
          DiagManagerAdapter::getInstance()->getSSRFlag(),
          DiagManagerAdapter::getInstance()->getWARflag(),
          DiagManagerAdapter::getInstance()->getRoBflag(),
          DiagManagerAdapter::getInstance()->getDDRflag());
          
    std::map<uint32_t, std::queue<android::sp<CenterReqData>>> centerRequest_map{};

const std::vector<uint64_t> newCenterRequestList {CollectionCondition::getInstance().getNewCenterRequestList()};
std::vector<uint64_t>::const_iterator idIter {newCenterRequestList.cbegin()};
while(idIter != newCenterRequestList.cend()) {
    const std::shared_ptr<CenterRequestJob> job {CollectionCondition::getInstance().getCenterRequestJob(*idIter)};
    if (job != nullptr)
    {
        const android::sp<CenterReqData> centerRequestHeader { job->getCenterRequestHeader() };
        if (centerRequestHeader != nullptr)
        {
            const Rdg_Sched_Type::SchedType tempScheduleType {centerRequestHeader->getScheduleType()};
            if ( (tempScheduleType == Rdg_Sched_Type::SchedType::ST_IMMEDIATE_IG_ON_ONE_SHOT)
                || (tempScheduleType == Rdg_Sched_Type::SchedType::ST_IMMEDIATE_ANY_POWER_STATUS_ONE_SHOT) )
            {
                centerRequest_map[centerRequestHeader->getCenterReq_prio()].push(centerRequestHeader);
            } else {
                mSchedMgr->applyNewCenterReqData(centerRequestHeader);
            }
        }
    }
    ++idIter;
}

    /*Obtain MSG_RECEIVE_CENTERCOMMNAD in order of priority*/
    for(std::map<uint32_t, std::queue<android::sp<CenterReqData>>>::iterator ptr_center{centerRequest_map.begin()}; 
        ptr_center != centerRequest_map.end(); ptr_center++) {
        std::queue<android::sp<CenterReqData>>& centerQueue{ptr_center->second};
        while(centerQueue.empty() != true) {
            const android::sp<CenterReqData> tmp_CenterReqData{centerQueue.front()};
            (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD, tmp_CenterReqData)->sendToTarget();
            centerQueue.pop();
        }
    }

    const std::vector<std::pair<uint64_t, uint8_t>> updatedList {CollectionCondition::getInstance().getUpdatedCollectionConditionIds()};

    for(std::vector<std::pair<uint64_t, uint8_t>>::const_iterator it {updatedList.begin()}; it != updatedList.end(); it++) {
        const uint64_t cocoId {it->first};
        const uint8_t cocoType {it->second};
        LOG_D("Check cocoId: %llu, coco data type: %d", cocoId, cocoType);
        if(cocoType == static_cast<uint8_t>(CollectionCondition::CollectionConditionType::CC_WARNING_INFORMATION)) {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CollectionConditionWarningInformation>
            ptrCollectionConditionWarningInformation(CollectionCondition::getInstance().getCollectionConditionWarningInformation());

                /*GetCollectionConditionResponse_CollectionConditionWarningInformation*/
            if ((mUnderRepair == 0U) && (ptrCollectionConditionWarningInformation != nullptr))
            {
                /* make CenterReqData and obtain MSG_RECEIVE_CENTERCOMMNAD*/
                const uint64_t colID{ptrCollectionConditionWarningInformation->collection_condition_id()};
                const uint8_t messID{MSG_ID_COLLECTIONCONDITIONWARNINGINFORMATION};
                const uint32_t prio{0U};
                const DiagTrigger::DiagTriggerType type{DiagTrigger::DiagTriggerType::CENTER_TRIGGER};
                const android::sp<CenterReqData> tmp_CenterReqData{new CenterReqData(colID, messID, prio, type)};
                (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD, tmp_CenterReqData)->sendToTarget();
            }
            else
            {
                LOG_I("Dont have CollectionConditionWarningInformation");
            }
        }
    }
}

void Remotediag::loadNewSchedData(const bool isOnlyLoadSched, const bool isBooting)
{
    mSchedMgr->applyNewSchedData(isOnlyLoadSched, isBooting);
}

void Remotediag::onScheduleReceived(const android::sp<CenterReqData> pCenterReqData) const
{
    
    /*check if RDG can run => notify routine is not complete*/
    
    if(mRdgCanRun) {
        (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD, pCenterReqData)->sendToTarget();
    } else {
        if(pCenterReqData != nullptr)
        {
            const uint64_t schedIndex{pCenterReqData->getCenterReq_CollectionID()};
            constexpr int64_t completeTime{0};
            constexpr bool isCompleted{false};
            notifyLastOpComplTime(schedIndex, completeTime, isCompleted);
        } else {
            LOG_E("ERROR: pCenterReqData is nullptr");
        }
    }
}

void Remotediag::onUnderRepairStatus(const int32_t status)
{
    if((status>=0) && (status <= UINT8_MAX))
    {
        setUnderRepair(static_cast<uint8_t>(status));
        // mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_RDG_UPDATED_STATUS,
        //                                   Remotediag::WHAT_CHANGED_REPAIR_SATUS, static_cast<int32_t>(status))
        //     ->sendToTarget();
        for (RemoteDelegate::Interator it{mDelegate.begin()}; it != mDelegate.end(); it++)
        {
            it->second->onChangedRemoteInfo(WHAT_CHANGED_REPAIR_SATUS, status);
        }
    }
    else{
        LOG_E("ERROR: status is out of range UINT8");
    }
}

void Remotediag::onServiceFlagChange(const android::sp<Buffer> didData)
{
    LOG_I("Notify service flag change");
    if (didData->data() != nullptr)
    {
        const uint8_t RDGFlag_tmp{static_cast<uint8_t>((didData->data()[0] >> 7U) & 1U)};
        const uint8_t DTCFlag_tmp{static_cast<uint8_t>((didData->data()[0] >> 6U) & 1U)};
        const uint8_t SSRFlag_tmp{static_cast<uint8_t>((didData->data()[0] >> 5U) & 1U)};
        const uint8_t WARflag_tmp{static_cast<uint8_t>((didData->data()[0] >> 4U) & 1U)};
        const uint8_t RoBflag_tmp{static_cast<uint8_t>((didData->data()[0] >> 3U) & 1U)};

        if (RDGFlag_tmp == 0U)
        {
            LOG_I("RDGFlag off => notify to function");
            mRoBOccurrence->onServiceFlagChange();
            UploadManager::getInstance()->onServiceFlagChange();
        }
        if (DTCFlag_tmp == 0U)
        {
            LOG_I("DTCFlag off => notify to function");
            if (mDTC != nullptr)
            {
                mDTC->onDTCFlagChangeOFF();
            }
        }
        if (SSRFlag_tmp == 0U)
        {
            LOG_I("SSRFlag off => notify to function");
            if (mSSR != nullptr)
            {
                mSSR->onSSRFlagChangeOFF();
            }
        }
        if (WARflag_tmp == 0U)
        {
            LOG_I("WARflag off => notify to function");
        }
        if (RoBflag_tmp == 0U)
        {
            LOG_I("RoBflag off => notify to function");
            if (mRoB != nullptr)
            {
                mRoB->onRobFlagChangeOFF();
            }
        }
        mWarning->onServiceFlagChange();
        LOG_I("Check RDGFlag: %d DTCFlag: %d SSRFlag: %d WARflag: %d RoBflag: %d",
            RDGFlag_tmp, DTCFlag_tmp, SSRFlag_tmp, WARflag_tmp, RoBflag_tmp);
    }
}

void Remotediag::onPPIReceived(const android::sp<Buffer> didData)
{
    LOG_I("Notify PPI flag change");
    /*Check if have any flag change from 10b to other => notify uploader*/
    if ( didData->data() != nullptr)
    {
        const bool mNewSRVC_AC{(((didData->data()[0] >> 6U) & 0b11U) == 0b10U) ? true : false};
        const bool mNewSRVC_VC{(((didData->data()[0] >> 4U) & 0b11U) == 0b10U) ? true : false};
        const bool mNewSRVC_PC{(((didData->data()[0] >> 2U) & 0b11U) == 0b10U) ? true : false};
        const bool mNewSRVC_STT{((didData->data()[0] & 0b11U) == 0b10U) ? true : false};
        if (((mSRVC_AC == true) && (mNewSRVC_AC == false)) ||
            ((mSRVC_VC == true) && (mNewSRVC_VC == false)) ||
            ((mSRVC_PC == true) && (mNewSRVC_PC == false)) ||
            ((mSRVC_STT == true) && (mNewSRVC_STT == false)))
        {
            UploadManager::getInstance()->onPPIChangedToFalse();
        }
        else{
            // do nothing
        }
        if (mNewSRVC_AC != mSRVC_AC)
        {
            LOG_I("SRVC_AC changed");
            DiagManagerAdapter::getInstance()->setSRVC(true, mNewSRVC_AC);
            mWarning->onPPIReceived();
        }
        if (mNewSRVC_STT != mSRVC_STT)
        {
            LOG_I("SRVC_STT changed");
            DiagManagerAdapter::getInstance()->setSRVC(false, mNewSRVC_STT);
        }
        mSRVC_AC = mNewSRVC_AC;
        mSRVC_VC = mNewSRVC_VC;
        mSRVC_PC = mNewSRVC_PC;
        mSRVC_STT = mNewSRVC_STT;
    }
}

void Remotediag::doRemotediagHandler(const android::sp<sl::Message> &msg)
{
    const int32_t what{msg->what};
    switch (what)
    {
    case HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_BOOT_COMPLETED:
    {
        LOG_I("MSG_APPL_ON_BOOT_COMPLETED");
        doBootCompleted();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_CMD_INIT_APP:
    {
        LOG_I("MSG_CMD_INIT_APP");
        initApp();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_APPLICATION_MGR:
    {
        LOG_I("MSG_REGISTER_APPLICATION_MGR");
        ApplicationManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_LOCATION_MGR:
    {
        LOG_I("MSG_REGISTER_LOCATION_MGR");
        LocationManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_POWER_MODE_MGR:
    {
        LOG_I("MSG_REGISTER_POWER_MODE_MGR");
        PowerManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_VEHICLE_MGR:
    {
        LOG_I("MSG_REGISTER_VEHICLE_MGR");
        VehicleManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CONFIG_MGR:
    {
        LOG_I("MSG_REGISTER_CONFIG_MGR");       
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_MQTT_MGR:
    {
        LOG_I("MSG_REGISTER_MQTT_MGR");
        MqttManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_SOMEIP_MGR:
    {
        LOG_I("MSG_REGISTER_SOMEIP_MGR");
        SomeipManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CALIB_MGR:
    {
        LOG_I("MSG_REGISTER_CALIB_MGR");
        CalibManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_DIAG_MGR:
    {
        LOG_I("MSG_REGISTER_DIAG_MGR");
        DiagManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_HTTP_MGR:
    {
        LOG_I("MSG_REGISTER_HTTP_MGR");
        /*Clear sentTask/clear old call id store*/
        UploadManager::getInstance()->clearSentQueue();
        HttpManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_OBC_MGR:
    {
        LOG_I("MSG_REGISTER_OBC_MGR");
        OnboardclientAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_REGION_MGR:
    {
        LOG_I("MSG_REGISTER_REGION_MGR");
        RegionManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_PPI_MGR:
    {
        LOG_I("MSG_REGISTER_PPI_MGR");
        PPIManagerAdapter::getInstance()->registerService();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_ALAM_ON_EXPIRED:
    {
        LOG_I("MSG_ALAM_ON_EXPIRED");
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_PPI_INFO_RECEIVED:
    {
        LOG_I("MSG_PPI_INFO_RECEIVED");
        std::vector<char_t> temp {};
        const uint32_t sizeOfBuf{msg->buffer.size()};
        if ((msg->buffer.data() != nullptr) && (sizeOfBuf > 0U) && (sizeOfBuf <= (temp.max_size() - 1U)))
        {
            temp.resize(sizeOfBuf + 1U, '\0');
            (void)std::memcpy(temp.data(), msg->buffer.data(), sizeOfBuf);
        }
        (void)sizeOfBuf;
        const uint32_t PPIflag {PPIManagerAdapter::getInstance()->receivePPIErase(temp.data())};
        setPPIFlag(PPIflag);
        handlePPIErase(PPIflag);
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_PPI_FLAG_CHANGE:
    {
        LOG_I("MSG_NOTIFY_PPI_FLAG_CHANGE");
        android::sp<Buffer> spBuf{nullptr};
        msg->getObject(spBuf);
        if ((spBuf != nullptr) && (spBuf->size() == 1U))
        {
            onPPIReceived(spBuf);
        }
        else
        {
            LOG_D("spBuf is nullptr");
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_VIN_CHANGE:
    {
        android::sp<Buffer> spBuf{nullptr};
        msg->getObject(spBuf);
        if ((spBuf != nullptr) && (spBuf->data() != nullptr) && (spBuf->size() == 17U))
        {
            std::stringstream ss{};
            for(size_t i {0U}; i < spBuf->size(); i++)
            {
                if (spBuf->data()[i]<= 127U)
                {
                    ss << spBuf->data()[i];
                } else {
                    LOG_E("Invalid ASCII");
                }
            }
            const std::string vinData{ss.str()};
            LOG_I("MSG_NOTIFY_VIN_CHANGE VIN data: %s", vinData.c_str());
            MqttManagerAdapter::getInstance()->subscribeTopic(vinData);
        }
        else
        {
            LOG_D("spBuf is nullptr");
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_CENTER_PUSH_RECEIVED:
    {
        LOG_I("MSG_CENTER_PUSH_RECEIVED");
        if (PowerManagerAdapter::getInstance()->getIgnitionStatus() == IG_STATUS::IG_STATUS_ON)
        {
            CollectionCondition::getInstance().onReceivedCenterRequest();
        }
        else
        {
            // RDG30-R-1234
            LOG_D("Reveived center push but IG is OFF -> reject");
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_OBC_OBD_EVENT_RECEIVED:
    {
        LOG_V("MSG_OBC_OBD_EVENT_RECEIVED, arg1 %d", msg->arg1);
        const IG_STATUS mIgnitionStatus{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
        if ((msg->arg1 == 1) && (mIgnitionStatus == IG_STATUS_ON))
        {
            LOG_I("Wire connection detected");
            OnboardclientAdapter::getInstance()->startTimer(OnboardclientAdapter::OnboardclientManagerAdapterTimer::OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION);
            this->bOBDEventStatus = true;
        }
        else
        {
            LOG_I("No Wire connection");
            this->bOBDEventStatus = false;
        }
        (void) mIgnitionStatus;
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_OBC_UDS_RESPONSE_RECEIVED:
    {
        LOG_I("MSG_OBC_UDS_RESPONSE_RECEIVED");
        android::sp<OBCResponseEventInfo> udsRes{nullptr};
        msg->getObject(udsRes);
        notifyReceiveUDS(udsRes);
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_RPC_MESSAGE_RECEIVED:
    {
        if(mRdgCanRun)
        {
            LOG_I("MSG_RPC_MESSAGE_RECEIVED");
            android::sp<GrpcResData> resData{nullptr};
            msg->getObject(resData);
            notifyReceiveGrpcRes(resData);
        }
        else
        {
            LOG_I("MSG_RPC_MESSAGE_RECEIVED but RDG is not ready");
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_ON:
    {
        LOG_I("MSG_POWR_ON_IGN_ON");
        this->bOBDEventStatus = false;       
        if(mIGStatus == false)   //to fix write DID 3001 twice when IG ON
        {
            const android::sp<::Buffer> timeData{new ::Buffer()};
            CommonUtils::convertCurrentTimeToBuffer(timeData);
            (void)mRemotediagHandler->sendMessageDelayed(mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_POWRER_WRITE_DID_ON, timeData),
                        static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
        UploadManager::getInstance()->resetCounterByIgON();
        if ((mRemotediagInitComplete == true) && (mIGStatus != true))
        {
            mIGStatus = true;
            notifyReceiveIG(true);
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_OFF:
    {
        LOG_I("MSG_POWR_ON_IGN_OFF");
        this->isRestartUploading = true;
        if(mIGStatus != false)
        {
            const android::sp<::Buffer> timeData{new ::Buffer()};
            CommonUtils::convertCurrentTimeToBuffer(timeData);
            (void)mRemotediagHandler->sendMessageDelayed(mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_POWRER_WRITE_DID_OFF, timeData),
                                           static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
        OnboardclientAdapter::getInstance()->stopTimer(OnboardclientAdapter::OnboardclientManagerAdapterTimer::OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION);
        this->bOBDEventStatus = false;
        LOG_V("IG OFF, Wire connection is cleared to No Connection due to IG OFF");
        if ((mRemotediagInitComplete == true) && (mIGStatus != false))
        {
            mIGStatus = false;
            PowerManagerAdapter::getInstance()->acquirePowerLock();
            notifyReceiveIG(false);
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_DIAG_TRIGGER_DISCARD_IGOFF:
    {
        LOG_I("MSG_NOTIFY_DIAG_TRIGGER_DISCARD_IGOFF");
        if((msg->arg1 >= static_cast<int32_t>(DiagTrigger::DiagTriggerFunc::DIAG_FUNC_MIN)) && (msg->arg1 <= static_cast<int32_t>(DiagTrigger::DiagTriggerFunc::DIAG_FUNC_MAX)) 
            && (msg->arg2 >= static_cast<int32_t>(DiagTrigger::DiagTriggerState::TRIGGER_STATE_MIN)) && (msg->arg2 <= static_cast<int32_t>(DiagTrigger::DiagTriggerState::TRIGGER_STATE_MAX)))
        {
            notifyTriggerStatus(static_cast<DiagTrigger::DiagTriggerFunc>(msg->arg1),
                        static_cast<DiagTrigger::DiagTriggerState>(msg->arg2), msg->arg3, true);
        }

        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_DIAG_TRIGGER:
    {
        LOG_I("MSG_NOTIFY_DIAG_TRIGGER");
        if((msg->arg1 >= static_cast<int32_t>(DiagTrigger::DiagTriggerFunc::DIAG_FUNC_MIN)) && (msg->arg1 <= static_cast<int32_t>(DiagTrigger::DiagTriggerFunc::DIAG_FUNC_MAX)) 
            && (msg->arg2 >= static_cast<int32_t>(DiagTrigger::DiagTriggerState::TRIGGER_STATE_MIN)) && (msg->arg2 <= static_cast<int32_t>(DiagTrigger::DiagTriggerState::TRIGGER_STATE_MAX)))
        {
            notifyTriggerStatus(static_cast<DiagTrigger::DiagTriggerFunc>(msg->arg1),
                        static_cast<DiagTrigger::DiagTriggerState>(msg->arg2), msg->arg3);
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_NEW_COLLECTION_CONDITION:
    {
        LOG_I("MSG_RECEIVE_NEW_COLLECTION_CONDITION");
        loadNewCenterRequest();
        /*RDG30-R-1219*/
        if (mIGStatus == true)
        {
            loadNewSchedData(false, false);
        }
        else
        {
            LOG_D("IG is OFF while scheduling data is received");
            /*RDG30-R-1080*/
            loadNewSchedData(true, false);
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_COLLECTION_CONDITION_UPDLOAD_END:
    {
        LOG_I("MSG_RECEIVE_COLLECTION_CONDITION_UPDLOAD_END");
        /*Restart uploader*/
        /*Only restart uploading 1 time after next IG ON*/
        const IG_STATUS ig_status_data{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
        if((ig_status_data == IG_STATUS::IG_STATUS_ON) && (isRestartUploading)) {
            mUploader->onRestartUploading();
            isRestartUploading = false;
        }
        if(isDoOperationA.load(std::memory_order_seq_cst))
        {
            onGRPCCommReconnect();
            isDoOperationA.store(false, std::memory_order_seq_cst);

        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_GRPC_COMMUNICATION_RECONNECT:
    {
        LOG_I("MSG_GRPC_COMMUNICATION_RECONNECT");
        // onGRPCCommReconnect();
        const android::sp<sl::Handler> mCollHandler{CollectionCondition::getInstance().getHandler()};
        if (mCollHandler != nullptr)
        {
            (void)mCollHandler->obtainMessage(CollectionCondition::CMD_HANDLE_GRPC_COMMUNICATION_RECONNECT)->sendToTarget();
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_GRPC_COMMUNICATION_DISCONNECT:
    {
        LOG_I("MSG_GRPC_COMMUNICATION_DISCONNECT");
        isDoOperationA.store(true, std::memory_order_seq_cst);
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD:
    {
        LOG_I("MSG_RECEIVE_CENTERCOMMNAD");
        android::sp<CenterReqData> centerCommand{nullptr};
        msg->getObject(centerCommand);
        if (centerCommand != nullptr)
        {
            receiveCenterCommnad(centerCommand);
        }
        else
        {
            LOG_D("centerCommand is nullptr");
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_RDG_UPDATED_STATUS:
    {
        LOG_I("MSG_NOTIFY_RDG_UPDATED_STATUS");
        notifyChangedRemoteStatus(msg->arg1, msg->arg2);
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_SERVICE_MODE_STATUS:
    {
        LOG_I("MSG_NOTIFY_SERVICE_MODE_STATUS");
        android::sp<Buffer> spBuf{nullptr};
        msg->getObject(spBuf);
        if ((spBuf != nullptr) && (spBuf->data() != nullptr))
        {
            DiagManagerAdapter::getInstance()->setUnderRepairStatus(spBuf);
            onUnderRepairStatus(static_cast<int32_t>(spBuf->data()[0]));
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_SERVICE_FLAG_CHANGE:
    {
        LOG_I("MSG_NOTIFY_SERVICE_FLAG_CHANGE");
        android::sp<Buffer> spBuf{nullptr};
        msg->getObject(spBuf);
        if ((spBuf != nullptr) && (spBuf->size() == 2U))
        {
            onServiceFlagChange(spBuf);
        }
        else
        {
            LOG_D("spBuf is nullptr");
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_FIREWALL_ACTION_DIAG_DISABLE:
    {
        LOG_I("MSG_FIREWALL_ACTION_DIAG_DISABLE");
        if (mOTA != nullptr)
        {
            mOTA->onFirewallActionDiagDisable();
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_FIREWALL_ACTION_DIAG_ENABLE:
    {
        LOG_I("MSG_FIREWALL_ACTION_DIAG_ENABLE");
        if (mOTA != nullptr)
        {
            mOTA->onFirewallActionDiagEnable();
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_DELETE_COLLECTION_CONDITION:
    {
        LOG_I("MSG_NOTIFY_DELETE_COLLECTION_CONDITION");
        android::sp<::Buffer> buf{nullptr};
        msg->getObject(buf);
        if ((buf != nullptr) && (buf->data() != nullptr)) {
            uint64_t collectionConditionId{0U};
            if (buf->data() != nullptr)
            {
                (void)std::memcpy(&collectionConditionId, buf->data(), sizeof(uint64_t));
            } else {
                LOG_E("buf->data() is nullptr");
            }
            // mSchedMgr->deteleSchedInMap(msg->arg1);
            LOG_I("MSG_NOTIFY_DELETE_COLLECTION_CONDITION, collectionConditionId = %llu", collectionConditionId);
        } else {
            LOG_E("buf is NULL");
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_STATUS_CHANGED:
    {
        LOG_I("MSG_APPL_ON_FEATURE_STATUS_CHANGED");
        sp<::Post> post_ptr {nullptr};
        try {
            msg->getObject(post_ptr);
        } catch (...) {
            LOG_E("Get post_ptr object failed");
            post_ptr = nullptr;
        }

        if(post_ptr != nullptr) {
            if(post_ptr->buffer.data() != nullptr) {
                char_t temp[post_ptr->buffer.size() + 1U] {0,};
                (void)std::memcpy(&temp[0], post_ptr->buffer.data(), post_ptr->buffer.size());
                const std::string feature {temp};
                const FeatureStatus status {static_cast<FeatureStatus>(post_ptr->arg1)};
                LOG_D("Check feature: %s status: %d", feature.c_str(), status);
                (void)handleFeatureStatusChange(feature, status);
            } else {
                // Do nothing
            }
        } else {
            LOG_E("Post object is null");
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_ACTION_DELIVERED:
    {
        LOG_I("MSG_APPL_ON_FEATURE_ACTION_DELIVERED");
        sp<::Post> post_ptr{nullptr};
        try
        {
            msg->getObject(post_ptr);
        }
        catch (...)
        {
            LOG_E("Get post_ptr object failed");
            post_ptr = nullptr;
        }

        if (post_ptr != nullptr)
        {
            if (post_ptr->buffer.data() != nullptr)
            {
                char_t temp[post_ptr->buffer.size() + 1U] {0,};
                (void)std::memcpy(&temp[0], post_ptr->buffer.data(), post_ptr->buffer.size());
                const std::string feature{temp};
                uint8_t action{0U};
                if((post_ptr->arg1 <= UINT8_MAX) && (post_ptr->arg1>=0))
                {
                    action = static_cast<uint8_t>(post_ptr->arg1);
                }
                const FeatureStatus status{ApplicationManagerAdapter::getInstance()->getFeatureStatus("remotediag")};
                LOG_D("action: %d feature name: %s", action, feature.c_str());
                if ((action == static_cast<uint8_t>(FeatureAction::UPDATE)) && (status == FeatureStatus::ON))
                {
                    LOG_I("Stop Remotediag uploading, Set Remotediag inactive");
                    const int32_t res2 {ApplicationManagerAdapter::getInstance()->setFeatureStatus("remotediag", "remotediag", FeatureStatus::OFF)};
                    if(res2 != E_OK) {
                        LOG_D("Set inactive RometeDiag Fail");
                    } else {
                        LOG_D("Remotediag is In_active");
                    }
                }
                else
                {
                    // Do nothing
                }
                (void) status;
            }
            else
            {
                // Do nothing
            }
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR:
    {
        LOG_I("MSG_NOTIFY_OCCURRENT_ROB_DETECTION_ROBSSR");
        android::sp<OccurrentRobNotification> notification{nullptr};
        msg->getObject(notification);
        mRoBSSR->onOccurrentRobDetected(notification);
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_NOTIFY_OCCURRENT_ROB_DETECTION_DIRECT_COMMAND:
    {
        LOG_I("MSG_NOTIFY_OCCURRENT_ROB_DETECTION_DIRECT_COMMAND");
        android::sp<OccurrentRobNotification> notification{nullptr};
        msg->getObject(notification);
        mRemoteDirectCommand->onOccurrentRobDetected(notification);
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_ACTION_PERFORMED:
    {
        LOG_I("MSG_APPL_ON_FEATURE_ACTION_PERFORMED");
        android::sp<Buffer> spBuf{nullptr};
        msg->getObject(spBuf);
        if((spBuf != nullptr) &&  (spBuf->data() != nullptr) && (spBuf->size() > 0U))
        {
            char_t temp[spBuf->size() + 1U] {0,};
            (void)std::memcpy(&temp[0], spBuf->data(), spBuf->size());
            const std::string featureName {temp};
            LOG_I("MSG_APPL_ON_FEATURE_ACTION_PERFORMED, feature: %s", featureName.c_str());
            handleFeatureActionPerformed(featureName);
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::HANDLE_MSG_BUB_ON:
    {
        LOG_I("HANDLE_MSG_BUB_ON");
        stopRDG(true);
        PowerManagerAdapter::getInstance()->releasePowerLock();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::HANDLE_MSG_BUB_OFF:
    {
        LOG_I("HANDLE_MSG_BUB_OFF");
        stopRDG(false);
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_PREPARE_TO_SHUTDOWN:
    {
        LOG_I("MSG_PREPARE_TO_SHUTDOWN");
        stopRDG(true);
        break;
    }    
    case HANDLE_MESSAGE_REQUEST::MSG_POWRER_WRITE_DID_ON:
    {
        LOG_I("MSG_POWRER_WRITE_DID_ON");
        android::sp<::Buffer> timeData{new ::Buffer()};
        msg->getObject(timeData);
        if((timeData != nullptr) &&  (timeData->data() != nullptr) && (timeData->size() > 0U))
        {
            DiagManagerAdapter::getInstance()->selfDiagIgOnOffTimes(true, DiagManagerAdapter::SELFDIAG_DID_TYPE_10_TIMES, timeData);
            DiagManagerAdapter::getInstance()->selfDiagIgOnOffTimes(true, DiagManagerAdapter::SELFDIAG_DID_TYPE_280_TIMES, timeData);
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_POWRER_WRITE_DID_OFF:
    {
        LOG_I("MSG_POWRER_WRITE_DID_OFF");
        android::sp<::Buffer> timeData{new ::Buffer()};
        msg->getObject(timeData);
        if((timeData != nullptr) &&  (timeData->data() != nullptr) && (timeData->size() > 0U))
        {
            DiagManagerAdapter::getInstance()->selfDiagIgOnOffTimes(false, DiagManagerAdapter::SELFDIAG_DID_TYPE_10_TIMES, timeData);
            DiagManagerAdapter::getInstance()->selfDiagIgOnOffTimes(false, DiagManagerAdapter::SELFDIAG_DID_TYPE_280_TIMES, timeData);
        }
        break;
    }
    default:
    {
        LOG_I("End of doRemotediagHandler, what = %d", what);
        break;
    }
    }
}

error_t Remotediag::onFeatureActionPerformed(const FeatureAction action, const std::string feature)
{
    LOG_I("onFeatureActionPerformed: %d : %s", action, feature.c_str());
    switch (action)
    {
    case FeatureAction::LAUNCH:
    {
        LOG_I("LAUNCH. check feature: %s: ", feature.c_str());
        break;
    }
    case FeatureAction::IGNORE:
    {
        LOG_I("IGNORE. check feature: %s", feature.c_str());
        const android::sp<Buffer> spBuf{new Buffer()};
        const uint32_t sizeData {static_cast<uint32_t>(feature.size())};
        if (sizeData <= static_cast<uint32_t>(INT32_MAX))
        {
            spBuf->setTo(feature.c_str(), static_cast<int32_t>(sizeData));
        }
        (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_ACTION_PERFORMED, spBuf)->sendToTarget();
        break;
    }
    case FeatureAction::UPDATE:
    {
        LOG_I("UPDATE. check feature: %s: ", feature.c_str());
        LOG_I("Wait for %s until termination", feature.c_str());
        break;
    }
    case FeatureAction::POSTPONE:
    {
        LOG_I("POSTPONE. check feature: %s: ", feature.c_str());
        LOG_I("wait for %s until current behavier is finished", feature.c_str());
        break;
    }
    case FeatureAction::TRIGGER:
    {
        LOG_I("TRIGGER. check feature: %s: ", feature.c_str());
        break;
    }
    default:
    {
        LOG_I("default");
        break;
    }
    }
    return E_OK;
}

void Remotediag::handleFeatureActionPerformed(const std::string featureName)
{
    mWaitingList[featureName] = 1;
}

error_t Remotediag::onFeatureStatusChanged(const std::string feature, const FeatureStatus status)
{
    LOG_I("Remotediag is onFeatureStatusChanged");
    /*TBD: handle callback active notify for RDG to notify to Uploader*/
    return E_OK;
}

void Remotediag::handlePPIErase(const uint32_t ppiFlag) const
{
    LOG_I("ppiFlag = %u", ppiFlag);
    switch(ppiFlag)
    {
        case IPPIManagerServiceType::PPI_OPERATING:
        case IPPIManagerServiceType::PPI_RETRY_1ST:
        case IPPIManagerServiceType::PPI_RETRY_2ND:
        {
            LOG_I("PPI_APP_TYPE_REMOTE_DIAG : PPI_APP_STATUS_CLEANUP");
            (void)PPIManagerAdapter::getInstance()->responsePPIErase(PPI_APP_TYPE::PPI_APP_TYPE_REMOTE_DIAG, IPPIManagerServiceType::PPI_APP_STATUS_CLEANUP);
            break;
        }

        case IPPIManagerServiceType::PPI_NO_RESPONSE:
        case IPPIManagerServiceType::PPI_FORMATTING:
        {
            break;
        }
        
        case IPPIManagerServiceType::PPI_FORMAT_COMPLETE:
        case IPPIManagerServiceType::PPI_FORMAT_UNCOMPLETE:
        case IPPIManagerServiceType::PPI_END:
        {
            // When data deletion is finished, be sure to set "PPI_APP_STATUS_RE_RUNNING" and call responseDeletePPInformation().
            // Post-action in PPI Mgr and change to "PPI_APP_STATUS_INIT".
            LOG_I("PPI_APP_TYPE_REMOTE_DIAG : PPI_APP_STATUS_RE_RUNNING");
            (void)PPIManagerAdapter::getInstance()->responsePPIErase(PPI_APP_TYPE::PPI_APP_TYPE_REMOTE_DIAG, IPPIManagerServiceType::PPI_APP_STATUS_RE_RUNNING);
            break;
        }

        default:
            break;
    }
}

bool Remotediag::getOBDStatus() const noexcept
{
    return this->bOBDEventStatus;
}

uint32_t Remotediag::getPPIFlag() const noexcept
{
    const Mutex::Autolock lock{Mutex::Autolock(mMutexPPIFlag)};
    return this->mPPIflag;
}

void Remotediag::setPPIFlag(const uint32_t PPIFlag) noexcept
{
    const Mutex::Autolock lock{Mutex::Autolock(mMutexPPIFlag)};
    mPPIflag = PPIFlag;
}

void Remotediag::setOperationA(const bool status) noexcept {
    isDoOperationA.store(status, std::memory_order_seq_cst);
}
// void Remotediag::notifyCommunicationRestore()
// {
//     LOG_I("notify communication restore");
//     mUploader->onCommuRestore();
// }

void Remotediag::onGRPCCommReconnect() {
    LOG_I("onGRPCCommReconnect");
    LOG_I("Check wait list: %d", mWaitingList.size());
    if(mWaitingList.empty()) {
        UploadManager::getInstance()->operationA();
    } else {
        LOG_D("Wait for priority valid");
    }
}

void Remotediag::onGRPCCommDisconnect() const {
    LOG_I("onGRPCCommDisconnect");
}

void Remotediag::stopRDG(const bool isStop)
{
    // - Do not handle Comming HTTP data
    if(isStop) {
        mRdgCanRun = false;
    } else {
        mRdgCanRun = true;
    }
    LOG_I("Stop RDG");
    /* 
    * - Stop all timer
    * - Stop all service
    * - Stop all connection
    * - Stop all thread
    * - Stop all task
    * - Stop all handler
    * - Stop all delegate
    * - Stop all manager
    * - Stop all adapter
    */
//    - Stop all operation
   for (RemoteDelegate::Interator it{mDelegate.begin()}; it != mDelegate.end(); it++)
   {
       it->second->onRdgStop(isStop);
   }
   
   mSchedMgr->onRdgStop(isStop);
   CollectionCondition::getInstance().onRdgStop(isStop);
//    - Stop uploading 
   UploadManager::getInstance()->onRdgStop(isStop);
}

#ifdef __cplusplus
extern "C" class Application *createApplication()
{
    (void)printf("create Remotediag");
    gApp = new Remotediag;
    return gApp.get();
}

extern "C" void destroyApplication(class Application *const application)
{
    delete (Remotediag *)application;
}
#endif
}
