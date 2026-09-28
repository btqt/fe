#include <vector>
#include "Remotediag.h"

#include "diagprocess/UDS/UdsMessage.h"

namespace rdgapp {
android::sp<Application> gApp{};
Remotediag *Remotediag::mRemotediag{nullptr};
Remotediag::Remotediag()
{
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
    initDLTLog();
}

Remotediag *Remotediag::getInstance()
{
    if (mRemotediag == nullptr)
    {
        mRemotediag = new Remotediag();
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
    LOG_I("Not receive Under Repair Status. Default Under Repair Status: %d", mUnderRepair);
    if (mRemotediagHandler == nullptr)
    {
        mRemotediagHandler = new RemotediagHandler(mLooper, *this);
        mRemotediagHandler->init(mRemotediagHandler.get());
    }
    /*Register ApplicationManagerAdapter */
    initRemoteDiagApp();
    ApplicationManagerAdapter::getInstance()->registerService();
}

void Remotediag::doBootCompleted()
{
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

    (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_CMD_INIT_APP)->sendToTarget();
}

void Remotediag::handleFeatureStatusChange(const std::string feature, const FeatureStatus status) {
    LOG_I("Check feature: %s status: %d", feature.c_str(), status);
    const std::map<std::string, int32_t>::iterator ptr_Feature{mWaitingList.find(feature)};
    if(ptr_Feature != mWaitingList.end()) {
        if (status == FeatureStatus::OFF) {
            (void)mWaitingList.erase(feature);
            UploadManager::getInstance()->operationA();

        }
    } else {
        LOG_E("Do not have feature: %s in saveList", feature.c_str());
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

uint8_t Remotediag::getUnderRepair() const noexcept
{
    return mUnderRepair;
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
    CollectionCondition::getInstance().onReceiveIG(status);
    UploadManager::getInstance()->onReceiveIG(status);
}

void Remotediag::notifyReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo)
{
    const android::sp<::Buffer> udsData{responseEventInfo->resInfo()->udsData()};
    const android::sp<UdsMessage> udsResponse{new UdsMessage()};
    if (udsData->size() > 0U)
    {
        (void)udsResponse->Parser(udsData);
    }
    for (RemoteDelegate::Interator it{mDelegate.begin()}; it != mDelegate.end(); it++)
    {
        it->second->onReceiveUDS(responseEventInfo, udsResponse);
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
        CollectionCondition::getInstance().onReceivedGetCollectionConditionResponse(pGrpcResData);
        break;
    }
    // DCIF-RDG012_notify_collection_condition_update_result_response.proto
    case GRPC_IF_TYPE::DCIF_RDG012:
    {
        LOG_I("GRPC_IF_TYPE::DCIF_RDG012");
        CollectionCondition::getInstance().onReceivedNotifyCollectionConditionUpdateResultResponse(pGrpcResData);
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
        UploadManager::getInstance()->onReceivedGrpcRes(pGrpcResData);
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

void Remotediag::notifyLastOpComplTime(const uint64_t schedIndex, const int64_t completeTime) const
{
    LOG_I("notifyLastOpComplTime");
    mSchedMgr->notifySchedComplete(schedIndex, completeTime);
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
    mUploader = new UploadManager(*this, vehicleTriggerLooper);
    mRoBOccurrence = new RoBOccurrence(*this, vehicleTriggerLooper);

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
    mUnderRepair = DiagManagerAdapter::getInstance()->getUnderRepairStatus();
    LOG_I("Check getUnderRepairStatus: %d", mUnderRepair);
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

void Remotediag::triggerWarningToDTC(const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData, const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority) const
{
    /*if DTC flag off -> last upload*/
    bool dtcFlag{false};
    dtcFlag = DiagManagerAdapter::getInstance()->getDTCFlag()> 0U;
    if (dtcFlag == true)
    {
        LOG_I("Remotediag::triggerWarningToDTC");
        LOG_I("DTC was notified with TriggerType: %d", triggerType);
        LOG_I("DTC was notified with Timedata: %lld", timeData);
        LOG_I("DTC was notified with Location: 0x%08X 0x%08X", location->getLatitude(), location->getLongtitude());
        LOG_I("DTC was notified with Collection ID: %llu", collectionId);
        LOG_I("DTC was notified with priority: %d ", priority);
        mDTC->triggerFromWarning(triggerType, timeData, location, collectionId, priority);
    }
    else
    {
        LOG_I("Trigger LastUpload");
        triggerLastUpload(triggerType, timeData, location, collectionId, priority);
    }
}
void Remotediag::triggerDTCToSSR(const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData,const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority,const std::vector<uint32_t> v_targetEcu) const
{
    LOG_I("Remotediag::triggerDTCToSSR");
    LOG_I("SSR was notified with TriggerType: %d", triggerType);
    LOG_I("SSR was notified with Timedata: %lld", timeData);
    LOG_I("SSR was notified with Location: 0x%08X 0x%08X", location->getLatitude(), location->getLongtitude());
    LOG_I("SSR was notified with Collection ID: %llu", collectionId);
    LOG_I("SSR was notified with priority: %d ", priority);
    mSSR->triggerFromDTC(triggerType, timeData, location, collectionId, priority, v_targetEcu);
}
void Remotediag::triggerSSRToRoB(const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData,const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority) const
{
    /*if RoB flag off -> last upload*/
    bool robFlag{false};
    robFlag = (DiagManagerAdapter::getInstance()->getRoBflag()>0U)?true:false;
    if (robFlag == true)
    {
        LOG_I("Remotediag::triggerSSRToRoB");
        LOG_I("RoB was notified with TriggerType: %d", triggerType);
        LOG_I("RoB was notified with Timedata: %lld", timeData);
        LOG_I("RoB was notified with Location: 0x%08X 0x%08X", location->getLatitude(), location->getLongtitude());
        LOG_I("RoB was notified with Collection ID: %llu", collectionId);
        LOG_I("RoB was notified with priority: %d ", priority);
        mRoB->triggerFromSSR(triggerType, timeData, location, collectionId, priority);
    }
    else
    {
        LOG_I("Trigger LastUpload");
        triggerLastUpload(triggerType, timeData, location, collectionId, priority);
    }
}
void Remotediag::triggerLastUpload(const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData,const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority) const
{
    LOG_I("Start trigger last upload");
    mRemoteLastUpload->triggerLastUpload(triggerType, timeData, location,collectionId, priority);
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
        LOG_I("default");
        break;
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

    /*Call API to get center request*/
    const std::shared_ptr<vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CenterRequestAllDtcSsr>
        ptrCenterRequestAllDtcSsr{CollectionCondition::getInstance().getCenterRequestAllDtcSsr()};

    const std::shared_ptr<vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CenterRequestAllRob>
        ptrCenterRequestAllRob{CollectionCondition::getInstance().getCenterRequestAllRob()};

    const std::shared_ptr<vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CenterRequestEcuInformation>
        ptrCenterRequestEcuInformation{CollectionCondition::getInstance().getCenterRequestEcuInformation()};

    const CenterRequestRobSsrList &robSsrList{CollectionCondition::getInstance().getCenterRequestRobSsr()};
    const CenterRequestDirectCommandList &tempCenterRequestDirectCommandList{CollectionCondition::getInstance().getCenterRequestDirectCommand()};

    const std::shared_ptr<vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_CollectionConditionWarningInformation>
    ptrCenterRequestWarningInformation{CollectionCondition::getInstance().getCollectionConditionWarningInformation()};
    std::map<uint32_t, std::queue<android::sp<CenterReqData>>> centerRequest_map{};
    /*GetCollectionConditionResponse_CenterRequestAllDtcSsr*/
    if (ptrCenterRequestAllDtcSsr != nullptr)
    {
        /*TBD: Check RDG active flag and DTC flag*/
        LOG_I("CenterRequestAllDtcSsr check ID: %llu", ptrCenterRequestAllDtcSsr->collection_condition_id());
        /* make CenterReqData*/
        const uint64_t colID{ptrCenterRequestAllDtcSsr->collection_condition_id()};
        constexpr uint8_t messID{MSG_ID_CENTERREQUESTALLDTCSSR};
        uint32_t prio{0U};
        Rdg_Sched_Type::SchedType tempScheduleType{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
        if (ptrCenterRequestAllDtcSsr->has_schedule_information())
        {
            const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data{ptrCenterRequestAllDtcSsr->schedule_information()};
            prio = schedule_data.priority();
            const vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType sche_type{schedule_data.schedule_type()};
            if ((sche_type >= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MIN)
                && (sche_type <= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MAX))
            {
                tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(sche_type);
            }
            LOG_I("CenterRequestAllDtcSsr check prio: %d", prio);
        }
        constexpr DiagTrigger::DiagTriggerType type{DiagTrigger::DiagTriggerType::CENTER_TRIGGER};
        const android::sp<CenterReqData> tmp_CenterReqData{new CenterReqData(colID, messID, prio, type, tempScheduleType)};
        centerRequest_map[prio].push(tmp_CenterReqData);
        // (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD, tmp_CenterReqData)->sendToTarget();
    }
    else
    {
        LOG_I("Dont have CenterRequestAllDtcSsr");
    }
    /*GetCollectionConditionResponse_CenterRequestAllRob*/
    if (ptrCenterRequestAllRob != nullptr)
    {
        LOG_I("CenterRequestAllRob check ID: %llu", ptrCenterRequestAllRob->collection_condition_id());
        /* make CenterReqData and obtain MSG_RECEIVE_CENTERCOMMNAD*/
        const uint64_t colID{ptrCenterRequestAllRob->collection_condition_id()};
        constexpr uint8_t messID{MSG_ID_CENTERREQUESTALLROB};
        uint32_t prio{0U};
        Rdg_Sched_Type::SchedType tempScheduleType{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
        if (ptrCenterRequestAllRob->has_schedule_information())
        {
            const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data{ptrCenterRequestAllRob->schedule_information()};
            prio = schedule_data.priority();
            const vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType sche_type{schedule_data.schedule_type()};
            if ((sche_type >= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MIN)
                && (sche_type <= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MAX))
            {
                tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(sche_type);
            }
            LOG_I("CenterRequestAllRob check prio: %d", prio);
        }
        constexpr DiagTrigger::DiagTriggerType type{DiagTrigger::DiagTriggerType::CENTER_TRIGGER};
        const android::sp<CenterReqData> tmp_CenterReqData{new CenterReqData(colID, messID, prio, type, tempScheduleType)};
        centerRequest_map[prio].push(tmp_CenterReqData);
        // (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD, tmp_CenterReqData)->sendToTarget();
    }
    else
    {
        LOG_I("Dont have CenterRequestAllRob");
    }
    /*GetCollectionConditionResponse_CenterRequestDirectCommand*/
    if (tempCenterRequestDirectCommandList.size() > 0)
    {
        for (CenterRequestDirectCommandIter it{tempCenterRequestDirectCommandList.begin()}; it != tempCenterRequestDirectCommandList.end(); it++)
        {
            LOG_I("CenterRequestDirectCommand check ID: %llu", it->collection_condition_id());
            /* make CenterReqData and obtain MSG_RECEIVE_CENTERCOMMNAD*/
            const uint64_t colID{it->collection_condition_id()};
            constexpr uint8_t messID{MSG_ID_CENTERREQUESTDIRECTCOMMAND};
            uint32_t prio{0U};
            Rdg_Sched_Type::SchedType tempScheduleType{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
            if (it->has_schedule_information())
            {
                const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data{it->schedule_information()};
                prio = schedule_data.priority();
                tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(schedule_data.schedule_type());
                LOG_I("CenterRequestDirectCommand check prio: %d, schedule type:  %d", prio, tempScheduleType);
            }
            if((tempScheduleType == Rdg_Sched_Type::SchedType::ST_IMMEDIATE_IG_ON_ONE_SHOT) || 
                (tempScheduleType == Rdg_Sched_Type::SchedType::ST_IMMEDIATE_ANY_POWER_STATUS_ONE_SHOT)) {
                constexpr DiagTrigger::DiagTriggerType type{DiagTrigger::DiagTriggerType::CENTER_TRIGGER};
                const android::sp<CenterReqData> tmp_CenterReqData{new CenterReqData(colID, messID, prio, type, tempScheduleType)};
                centerRequest_map[prio].push(tmp_CenterReqData);
                // (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD, tmp_CenterReqData)->sendToTarget();
            } else {
                /*Do nothing*/
            }
            (void) colID;
            (void) prio;
            (void) messID;
        }
    }
    else
    {
        LOG_D("tempCenterRequestDirectCommandList is empty");
    }

    /*GetCollectionConditionResponse_CenterRequestRobSsr*/
    if (robSsrList.size() > 0)
    {
        for (CenterRequestRobSsrIter it{robSsrList.begin()}; it != robSsrList.end(); it++)
        {
            LOG_I("CenterRequestRobSsr check ID: %lld", it->collection_condition_id());
            /* make CenterReqData and obtain MSG_RECEIVE_CENTERCOMMNAD*/
            const uint64_t colID{it->collection_condition_id()};
            constexpr uint8_t messID{MSG_ID_CENTERREQUESTROBSSR};
            uint32_t prio{0U};
            Rdg_Sched_Type::SchedType tempScheduleType{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
            if (it->has_schedule_information())
            {
                const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data{it->schedule_information()};
                prio = schedule_data.priority();
                const vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType sche_type{schedule_data.schedule_type()};
                if ((sche_type >= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MIN)
                    && (sche_type <= vccomif::rdg::v1::interfaces::ScheduleInformation_ScheduleType_ScheduleType_MAX))
                {
                    tempScheduleType = static_cast<Rdg_Sched_Type::SchedType>(sche_type);
                }
                LOG_I("CenterRequestRobSsr check prio: %d", prio);
            }
            constexpr DiagTrigger::DiagTriggerType type{DiagTrigger::DiagTriggerType::CENTER_TRIGGER};
            const android::sp<CenterReqData> tmp_CenterReqData{new CenterReqData(colID, messID, prio, type, tempScheduleType)};
            centerRequest_map[prio].push(tmp_CenterReqData);
            // (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD, tmp_CenterReqData)->sendToTarget();
        }
    }
    else
    {
        LOG_D("Dont have CenterRequestRobSsr");
    }
    /*GetCollectionConditionResponse_CenterRequestEcuInformation*/
    if (ptrCenterRequestEcuInformation != nullptr)
    {
        LOG_I("CenterRequestEcuInformation check ID: %llu", ptrCenterRequestEcuInformation->collection_condition_id());
        /* make CenterReqData and obtain MSG_RECEIVE_CENTERCOMMNAD*/
        const uint64_t colID{ptrCenterRequestEcuInformation->collection_condition_id()};
        constexpr uint8_t messID{MSG_ID_CENTERREQUESTECUINFORMATION};
        uint32_t prio{0U};
        constexpr Rdg_Sched_Type::SchedType tempScheduleType{Rdg_Sched_Type::SchedType::ST_UNKNOWN};
        if (ptrCenterRequestEcuInformation->has_schedule_information())
        {
            const vccomif::rdg::v1::interfaces::ScheduleInformation schedule_data{ptrCenterRequestEcuInformation->schedule_information()};
            prio = schedule_data.priority();
            LOG_I("check prio: %d schedule type: %d", prio, tempScheduleType);
        }
        constexpr DiagTrigger::DiagTriggerType type{DiagTrigger::DiagTriggerType::CENTER_TRIGGER};
        const android::sp<CenterReqData> tmp_CenterReqData{new CenterReqData(colID, messID, prio, type, tempScheduleType)};
        centerRequest_map[prio].push(tmp_CenterReqData);
        // (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD, tmp_CenterReqData)->sendToTarget();
    }
    else
    {
        LOG_I("Dont have CenterRequestEcuInformation");
    }
    /*GetCollectionConditionResponse_CollectionConditionWarningInformation*/
    if (ptrCenterRequestWarningInformation != nullptr)
    {
        const ::vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_UpdateTypeSingle upType{ptrCenterRequestWarningInformation->update_type_collection_condition()};
        LOG_I("CollectionConditionWarningInformation check ID: %llu", ptrCenterRequestWarningInformation->collection_condition_id());
        if(upType == ::vccomif::rdg::v1::interfaces::GetCollectionConditionResponse_UpdateTypeSingle::GetCollectionConditionResponse_UpdateTypeSingle_UTS_CHANGED) {
        /* make CenterReqData and obtain MSG_RECEIVE_CENTERCOMMNAD*/
        const uint64_t colID{ptrCenterRequestWarningInformation->collection_condition_id()};
        constexpr uint8_t messID{MSG_ID_COLLECTIONCONDITIONWARNINGINFORMATION};
        constexpr uint32_t prio{20U};
        constexpr DiagTrigger::DiagTriggerType type{DiagTrigger::DiagTriggerType::CENTER_TRIGGER};
        const android::sp<CenterReqData> tmp_CenterReqData{new CenterReqData(colID, messID, prio, type)};
        centerRequest_map[prio].push(tmp_CenterReqData);
        // (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD, tmp_CenterReqData)->sendToTarget();
        } else {
            LOG_D("CollectionConditionWarningInformation no changed");
        }
    }
    else
    {
        LOG_I("Dont have CollectionConditionWarningInformation");
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
}

void Remotediag::loadNewSchedData()
{
    mSchedMgr->applyNewSchedData(false, false);
}

void Remotediag::onScheduleReceived(const android::sp<CenterReqData> pCenterReqData) const
{
    (void)mRemotediagHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_RECEIVE_CENTERCOMMNAD, pCenterReqData)->sendToTarget();
}

void Remotediag::onUnderRepairStatus(const int32_t status)
{
    if((status>=0) && (status <= UINT8_MAX))
    {
        mUnderRepair = static_cast<uint8_t>(status);
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
        const uint8_t RDGFlag_tmp{(didData->data()[0] >> 7U) & 1U};
        const uint8_t DTCFlag_tmp{(didData->data()[0] >> 6U) & 1U};
        const uint8_t SSRFlag_tmp{(didData->data()[0] >> 5U) & 1U};
        const uint8_t WARflag_tmp{(didData->data()[0] >> 4U) & 1U};
        const uint8_t RoBflag_tmp{(didData->data()[0] >> 3U) & 1U};

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
        mWarning->onPPIReceived();
    }
}

void Remotediag::doRemotediagHandler(const android::sp<sl::Message> &msg)
{
    LOG_D("Remotediag::doRemotediagHandler");
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
        // onPPIReceived(static_cast<uint8_t>(msg->arg1));
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
        LOG_I("MSG_RPC_MESSAGE_RECEIVED");

        android::sp<GrpcResData> resData{nullptr};
        msg->getObject(resData);
        notifyReceiveGrpcRes(resData);
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_ON:
    {
        LOG_I("MSG_POWR_ON_IGN_ON");
        this->bOBDEventStatus = false;
        DiagManagerAdapter::getInstance()->selfDiagIgOnOffTimes(true, DiagManagerAdapter::SELFDIAG_DID_TYPE_10_TIMES);
        DiagManagerAdapter::getInstance()->selfDiagIgOnOffTimes(true, DiagManagerAdapter::SELFDIAG_DID_TYPE_280_TIMES);
        if ((mRemotediagInitComplete == true) && (mIGStatus != true))
        {
            mIGStatus = true;
            notifyReceiveIG(true);
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_POWR_ON_IGN_OFF:
    {
        LOG_V("MSG_POWR_ON_IGN_OFF");
        this->isRestartUploading = true;
        DiagManagerAdapter::getInstance()->selfDiagIgOnOffTimes(false, DiagManagerAdapter::SELFDIAG_DID_TYPE_10_TIMES);
        DiagManagerAdapter::getInstance()->selfDiagIgOnOffTimes(false, DiagManagerAdapter::SELFDIAG_DID_TYPE_280_TIMES);
        OnboardclientAdapter::getInstance()->stopTimer(OnboardclientAdapter::OnboardclientManagerAdapterTimer::OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION);
        this->bOBDEventStatus = false;
        LOG_I("Wire connection is cleared to No Connection due to IG OFF");
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
            loadNewSchedData();
        }
        else
        {
            LOG_D("IG is OFF. Load shedule next IG ON");
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
            mUploader->restartUploading();
            isRestartUploading = false;
        }
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_GRPC_COMMUNICATION_RECONNECT:
    {
        LOG_I("MSG_GRPC_COMMUNICATION_RECONNECT");
        onGRPCCommReconnect();
        CollectionCondition::getInstance().onGrpcReconnect();
        break;
    }
    case HANDLE_MESSAGE_REQUEST::MSG_GRPC_COMMUNICATION_DISCONNECT:
    {
        LOG_I("MSG_GRPC_COMMUNICATION_DISCONNECT");
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
        onUnderRepairStatus(msg->arg1);
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
                    LOG_I("Stop Remotediag uploading");
                    LOG_D("Set Remotediag inactive");
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
        LOG_I("IGNORE. check feature: %s: ", feature.c_str());
        mWaitingList[feature] = 1;
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

error_t Remotediag::onFeatureStatusChanged(const std::string feature, const FeatureStatus status)
{
    LOG_I("Remotediag is onFeatureStatusChanged");
    /*TBD: handle callback active notify for RDG to notify to Uploader*/
    return E_OK;
}

bool Remotediag::getOBDStatus() const noexcept
{
    return this->bOBDEventStatus;
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
