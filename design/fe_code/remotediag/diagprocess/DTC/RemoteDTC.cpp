#include <google/protobuf/util/json_util.h>
#include "RemoteDTC.h"

namespace rdgapp {

namespace interface = vccomif::rdg::v1::interfaces;

RemoteDTC* RemoteDTC::mDTC_instance {nullptr};

RemoteDTC::RemoteDTC(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper):
        android::RefBase(), RemoteDelegate()
        , mApp(app)
        , mHandler(new MainHandler(privateLooper, *this))
        , mTimerHandler{}
        , mDTCFlag{0x0000U}
        , mTriggerId{0x000000}
        , mTriggerType{DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN}
        , mDiagnosticsAcquisitionTime{0U}
        , mWarningTriggerOccurrenceTime{0U}
        , mDTCUploadErrorData(std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest()))
        , mCurrentTransmissionId{0U}
        , mIsDTCRunning{RemoteDTC::DTC_STOP}
        , mColId{0ULL}
        , mCRCManager{nullptr}
        , mPriority{0U}
        , bUploadErrorData(false)
        , isDtcAcquireAborted(false)
        , isDtcAcquireSuspend(false)
        , mStopImmediately(false)
        , mMaxUploadFileSize(DTC_UPLOAD_DATA_SIZE_MAX)
{
    mDTC_instance = this;
    (void)mHandler->obtainMessage(MainHandler::CMD_INIT_DTC)->sendToTarget();
}

RemoteDTC* RemoteDTC::getInstance(void)
{
    if (mDTC_instance == nullptr)
    {
        LOG_E("getInstance error, mDTC_instance is null");
    }
    return mDTC_instance;
}

RemoteDTC::~RemoteDTC() = default;

void RemoteDTC::printData(const std::string data) const
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

void RemoteDTC::init() 
{
    LOG_I("Init DTC");
    mTimerHandler = std::make_shared<TimerHandler>(*this);
    mTickTimer = new Timer(mTimerHandler.get(), TimerHandler::ID_IG_ON_TRIGGER);
    mTickTimer->setDuration(RemoteDTC::IGON_TRIGGER_DURATION, 0U);
    /* Get DTC flag */
    /*IG ON and not under repair => IG ON trigger after 5 min*/
    if(mApp.getIGStatus() == 1U) {
        LOG_I("IG is ON and NOT under repair. DTC start after %d sec", RemoteDTC::IGON_TRIGGER_DURATION);
        mTickTimer->start();
    } else {
        LOG_I("IG OFF when init DTC");
    }
    mCRCManager = new CRCManager(*this);
    (void)mHandler->obtainMessage(MainHandler::CMD_READ_SAVED_CRC)->sendToTarget();
}

void RemoteDTC::onReceiveIG(const bool status) const
{
    (void)mHandler->obtainMessage(MainHandler::CMD_CHANGE_IG_STATUS,
        static_cast<int32_t>(status))->sendToTarget();
}
/*Receive uds at func onReceiveUDS and obtain message CMD_RECEIVE_UDS_RESPONSE*/
void RemoteDTC::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
    /*Check If DTC is running*/
    if (mIsDTCRunning == RemoteDTC::DTC_RUNNING)
    {
        (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UDS_RESPONSE, responseEventInfo)->sendToTarget();
        (void)udsResponse;
    }
}

void RemoteDTC::handleUDSResponse(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) {
    if(mIsDTCRunning == RemoteDTC::DTC_RUNNING) {
        /*Case normal*/
        /*Case suspend*/
        uint8_t tmp_code{responseEventInfo->errCode()};
        if (tmp_code > static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_MAX))
        {
           LOG_E("errCode is out of range OBC_ERR_MAX");
           tmp_code = static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_MAX);
        }
        const OBCEnum::OBCErrCode responseCode {static_cast<OBCEnum::OBCErrCode>(tmp_code)};
        const android::sp<OBCCanInfo> canInfo {responseEventInfo->resInfo()->canInfo()};
        const uint16_t connectId {responseEventInfo->resInfo()->connectId()};
        const uint8_t protocolType {responseEventInfo->resInfo()->protocolType()};
        const uint8_t sid {udsResponse->getSID()};
        const uint8_t subFunc {udsResponse->getSFID()};
        /*Get NRC*/
        /*Check NRC 0x78*/
        const uint8_t nrc{udsResponse->getNRC()};
        const uint32_t targetAddress {CommonUtils::calTargetAddressFromCanIdRx(protocolType, canInfo->canId())};
        LOG_D("Check targetAddress: 0x%x SID: 0x%x subFunc: 0x%x NRC: 0x%x", targetAddress, sid, subFunc, nrc);
        uint32_t centerRxAdd{0U};
        const uint32_t tmpCenterRxAdd{responseEventInfo->getResInfo()->getCanInfo()->getCanId()};
        centerRxAdd = tmpCenterRxAdd;
        if((isDtcAcquireAborted == false) || (mStopImmediately == false)) {
        if ((responseCode == OBCEnum::OBCErrCode::OBC_NEGATIVE) || (responseCode == OBCEnum::OBCErrCode::OBC_OK)) {
            if (responseCode == OBCEnum::OBCErrCode::OBC_NEGATIVE) {
                LOG_D("Received negative response from OBC, targetAddress = 0x%02x", targetAddress);
                LOG_D("Processing UDS data and Adding to Error upload data");
            }
            if(sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_SESSION_CONTROL)) {
                /*Process remote session change response*/
                LOG_I("UDS_PR_SESSION_CONTROL ECU PHASE 5: 0x%02x", targetAddress);
                LOG_D("Find transmissionId: 0x%llx", mCurrentTransmissionId);
                const uint8_t sfid{udsResponse->getSFID()};
                LOG_D("Check SFID: 0x%x", sfid);
                android::sp<DTCUdsTransmission> pCurrentTransmission {nullptr};
                const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
                if(it != mDTCTransList.end()) {
                    pCurrentTransmission = it->second;
                } else {
                    LOG_E("Can not find transmissionId: 0x%llx", mCurrentTransmissionId);
                }
                if ((pCurrentTransmission != nullptr) 
                    && (sfid == 0x40U)
                    && (pCurrentTransmission->getState() == DTCUdsTransmission::State::DTC_TRANS_REMOTE_SS))
                {
                    if (pCurrentTransmission->getConnectId() == connectId) {
                        pCurrentTransmission->stopTimeoutTimer();
                        pCurrentTransmission->send();
                    } else {
                        LOG_D("Connect ID not match");
                    }
                } else if((pCurrentTransmission != nullptr)
                    && (sfid == 0x01U)
                    && (pCurrentTransmission->getState() == DTCUdsTransmission::State::DTC_TRANS_DEFAULT_SS)) {
                    if (pCurrentTransmission->getConnectId() == connectId) {
                        pCurrentTransmission->stopTimeoutTimer();
                        pCurrentTransmission->disconnect();
                        pCurrentTransmission->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                        finishCurrentTransmission();
                        
                    } else {
                        LOG_D("Connect ID not match");
                    }
                } else {
                    LOG_E("NOT match UDS processing condition");
                }
                (void)sfid;
            }
            else if(sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION))
            {
                /*Process ECU phase 5 and 6 positive response*/
                uint64_t transmissionId {0U};
                transmissionId |= (0xFFFFFFFFFFFFFFFF & targetAddress) << 8U;
                transmissionId |= (0xFFFFFFFFFFFFFFFF & udsResponse->getSFID());
                LOG_I("Check response SID: 0x%x SubFunc: 0x%x", udsResponse->getSID(), udsResponse->getSFID());
                LOG_I("transmissionId: 0x%02llx", transmissionId);
                android::sp<DTCUdsTransmission> pTransmission {nullptr};
                const TransmissionInter it {mDTCTransList.find(transmissionId)};
                if(it != mDTCTransList.end()) {
                    pTransmission = it->second;
                } else {
                    LOG_E("Can not find transmissionId: 0x%llx", transmissionId);
                }
                if ((pTransmission != nullptr) && (pTransmission->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS)) {
                    LOG_I("DTC is running and match transmissionId. Process receive UDS");
                    /*Save RX target address to mDTCTransList*/
                    pTransmission->setRxAdd(centerRxAdd);
                    /* Save to Response List */
                    /*RDG30-R-0230*/
                    /*udsResponse_crc is smart pointer. It is safe to manage new memory allocation*/
                    const android::sp<UdsMessage> udsResponse_crc{new UdsMessage(udsResponse)};
                    if(udsResponse_crc != nullptr) {
                        if(udsResponse_crc->ToUdsData()->data() != nullptr) {
                        uint8_t* const dtcUserData{&(udsResponse_crc->ToUdsData()->data()[0])};
                        uint32_t noDTCNumber{0U};
                        const uint32_t udsDataSize{udsResponse_crc->ToUdsData()->size()};
                        if (udsDataSize > 3U)
                        {
                            /*
                            Data byte   Parameter Name
                            0           ReadDTCInformation Response SID
                            1           reportDTCByStatusMask
                            2           DTCStatusAvailabilityMask
                            3           DTCHighByte
                            4           DTCMiddleByte
                            5           DTCLowByte
                            6           statusOfDTC
                            */
                            noDTCNumber = (udsDataSize - 3U) / 4U;
                        }
                        if((dtcUserData != nullptr) && ((udsDataSize - 3U)%4U == 0U)) {
                            for (uint32_t i {0U} ; i < noDTCNumber; i++)
                            {
                                LOG_V("Add to targetEcuList");
                                const uint32_t DTCNo {static_cast<uint32_t>(static_cast<uint32_t>(dtcUserData[3U + i*4U]) << 16U) | static_cast<uint32_t>(static_cast<uint32_t>(dtcUserData[3U + i*4U+1U]) << 8U)
                                                    | static_cast<uint32_t>(static_cast<uint32_t>(dtcUserData[3U + i*4U+2U]))};
                                LOG_D("DTC Numbber: 0x%02X", DTCNo);
                                //LOG_D("StatusOfDTC before mask status bits: 0x%02X", i, dtcUserData[3U + i*4U + 3U]);
                                dtcUserData[3U + i*4U + 3U] &=  0x0DU; // masking StatusOfDTC  with 0x0D (keep pendingDTC,confirmedDTC and testFailed bits, other set to 0)
                                v_targetEcuList.push_back(make_pair(targetAddress, DTCNo));
                                //LOG_D("StatusOfDTC after mask status bits: 0x%02X", i, dtcUserData[3U + i*4U + 3U]);
                            }
                        } else {
                            LOG_D("dtcUserData is invalid");
                        }
                        (void)dtcUserData;
                        }
                        mDiagResp_CRC[transmissionId] = udsResponse_crc;
                    } else {
                        LOG_I("udsResponse_crc is nullptr");
                    }
                    mDiagResp_2.push_back({transmissionId, udsResponse});

                    if(transmissionId == mCurrentTransmissionId) {
                        /*If ECU phase 5 => change to default session*/
                        if(pTransmission->getEcuInformation().getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) {
                            pTransmission->stopTimeoutTimer();
                            pTransmission->changeToDefaultSS();
                        } else {
                            pTransmission->stopTimeoutTimer();
                            pTransmission->disconnect();
                            pTransmission->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                            finishCurrentTransmission();
                        }

                    } else {
                        LOG_I("Receive expired response");
                    }
                } else {
                    LOG_I("NOT match transmissionId. Process receive UDS");
                }
            } else if((sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE)) && (nrc != 0x78U)) {
                LOG_D("Find transmissionId: 0x%llx", mCurrentTransmissionId);
                const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
                android::sp<DTCUdsTransmission> tmp_DtcTrans{nullptr};
                if(it != mDTCTransList.end()) {
                    tmp_DtcTrans = it->second;
                } else {
                    LOG_E("NOT match transmissionId");
                }
                
                if (tmp_DtcTrans != nullptr)
                {
                    if((tmp_DtcTrans->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS)) {
                        /*Set negative response:
                         - Phase 4: res subfunction must be 0x13
                         - Phase 5 and 6: res sub function must be 0x19*/
                        if(((tmp_DtcTrans->getEcuInformation().getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_4) && (subFunc == 0x13U)) || 
                            ((tmp_DtcTrans->getEcuInformation().getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) && (subFunc == 0x19U))|| 
                            ((tmp_DtcTrans->getEcuInformation().getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6) && (subFunc == 0x19U))) {
                                mDiagResp_CRC[mCurrentTransmissionId] = udsResponse;
                                tmp_DtcTrans->setRxAdd(centerRxAdd);
                                if (tmp_DtcTrans->getConnectId() == connectId) {
                                    tmp_DtcTrans->stopTimeoutTimer();
                                    mDiagResp_2.push_back({mCurrentTransmissionId, udsResponse});
                                    /*If ECU phase 5 => change to default session*/
                                    if(tmp_DtcTrans->getEcuInformation().getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) {
                                        tmp_DtcTrans->changeToDefaultSS();
                                    } else {
                                        tmp_DtcTrans->disconnect();
                                        tmp_DtcTrans->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                                        finishCurrentTransmission();
                                    }
                                }
                        } else {
                            /*unresponsive case*/
                            handleUnresponsiveEcu(responseEventInfo, udsResponse);
                        }
                    } else if((tmp_DtcTrans->getState() == DTCUdsTransmission::State::DTC_TRANS_REMOTE_SS)) {
                        /**/
                        if(subFunc == 0x10U) {
                            // mDiagResp_CRC[mCurrentTransmissionId] = udsResponse;
                            tmp_DtcTrans->setRxAdd(centerRxAdd);
                            tmp_DtcTrans->stopTimeoutTimer();
                            mDiagResp_2.push_back({mCurrentTransmissionId, udsResponse});
                            tmp_DtcTrans->disconnect();
                            tmp_DtcTrans->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                            finishCurrentTransmission();
                        } else {
                            /*Unresponsive*/
                            handleUnresponsiveEcu(responseEventInfo, udsResponse);

                        }


                    } else if(tmp_DtcTrans->getState() == DTCUdsTransmission::State::DTC_TRANS_DEFAULT_SS) {
                        tmp_DtcTrans->stopTimeoutTimer();
                        tmp_DtcTrans->disconnect();
                        tmp_DtcTrans->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                        finishCurrentTransmission();
                    } else {
                        LOG_D("Other state");
                    }
                } else {
                    LOG_E("tmp_DtcTrans is nullptr");
                }
            } else if(sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION_PHASE4)) {
                /*Process ECU phase 4 positive response*/
                /*Compare received ECU targetAddress vs (mCurrentTransmissionId >> 8)*/
                LOG_D("TransmissionId: %llx", mCurrentTransmissionId);
                LOG_D("Income targetAddress: %llx", static_cast<uint64_t>(targetAddress));
                LOG_D("Current targetAddress: %llx", ((mCurrentTransmissionId >> 8U) & 0x00000000FFFFFFFFU));
                if(static_cast<uint64_t>(targetAddress) == ((mCurrentTransmissionId >> 8U) & 0x00000000FFFFFFFFU)) {
                    LOG_I("matched");
                    const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
                    if ((it != mDTCTransList.end()) && (it->second != nullptr) && (it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS))
                    {
                        it->second->setRxAdd(centerRxAdd);
                        if (it->second->getConnectId() == connectId) {
                            it->second->stopTimeoutTimer();
                            mDiagResp_CRC[mCurrentTransmissionId] = udsResponse;
                            mDiagResp_2.push_back({mCurrentTransmissionId, udsResponse});
                            it->second->disconnect();
                            it->second->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                            finishCurrentTransmission();
                        }
                    } else {
                        LOG_I("NOT match transmissionId");
                    }
                } else {
                    LOG_I("NOT matched");
                }
            } else {
                /*Unresponsive case*/
                handleUnresponsiveEcu(responseEventInfo, udsResponse);
            } 
        } else {
            LOG_D("Unresponsive of targetAddress = 0x%02x", targetAddress);
            handleUnresponsiveEcu(responseEventInfo, udsResponse);
        }
    } else {
        LOG_I("DTC is suspending");
        /*Stop timeout timer*/
        mIsDTCRunning = RemoteDTC::DTC_STOP;
        const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
        if ((it != mDTCTransList.end()) && (it->second != nullptr) && (it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS))
        {
            if (it->second->getConnectId() == connectId) {
                it->second->stopTimeoutTimer();
                it->second->disconnect();
                it->second->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                finishCurrentTransmission();
            }
        } else {
            LOG_I("NOT match transmissionId");
        }
        isDtcAcquireAborted = false;
        mStopImmediately = false;
        /*Release OBC resource*/
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        /*Notify done to priority control*/
        PriorityControl::getInstance()->notifyTriggerProcessDone(mTriggerId, mTriggerType);
    }
    (void)centerRxAdd;
    (void)responseCode;
    (void)connectId;
    (void)sid;
    (void)targetAddress;
    (void)canInfo;
    (void)protocolType;
    }
}

void RemoteDTC::onChangedRemoteInfo(const int32_t what, const int32_t info) {
    // CMD_RECEIVE_STATUS_FROM_CENTER
    (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_STATUS_FROM_CENTER, 
        what, info)->sendToTarget();
}

void RemoteDTC::onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) {
    LOG_I("DTC receive Center request");
    const android::sp<sl::Message> msg{mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_CENTER, pCenterReqData)};
    (void)msg->sendToTarget();
}

void RemoteDTC::onRdgStop(const bool isStop) const noexcept{
    //ontain message to stop rdg
    (void)mHandler->obtainMessage(MainHandler::CMD_STOP_RDG, static_cast<int32_t>(isStop))->sendToTarget();
}

void RemoteDTC::onDTCFlagChangeOFF() {
    LOG_D("Clear CRC");
    mCRCManager->requestRemoveCRCFile();
}

uint8_t RemoteDTC::getAppId() const noexcept{
    return RDG_APPID::DTC;
}

void RemoteDTC::changeIGStatus(const bool status) 
{
    if (status) {
        /*Check under repair status to start IG ON trigger*/
        if(mApp.getUnderRepair() == 0U) {
            LOG_I("IG ON TRIGGER after 5 mins");
            mTickTimer->start();
        } else {
            LOG_I("Discard IG ON TRIGGER due to Under repair");
        }
    } else {
        LOG_I("IG OFF. Stop IG ON TRIGGER 5min Timer.");
        mTickTimer->stop();
        if(mIsDTCRunning == RemoteDTC::DTC_RUNNING) {
            abortDtcProcessing(vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION, true);
        } else {
            LOG_I("IG OFF while DTC is not running");
        }
    }
}

void RemoteDTC::trigger_DTC(const DiagTrigger::DiagTriggerType type, const uint32_t prio
    , const uint64_t colId, const int64_t time, const android::sp<CommonDefine::RDGLocationData> loc) {
    LOG_I("DTC trigger");
    /* TBD: check ppi flag*/
    /* Get trigger ID*/
    const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
    LOG_I("Trigger DTC start with TriggerId: %d prio: %d", nextTriggerId, prio);
    /* Create NewDiag Trigger*/
    /* DiagTrigger(const DiagTrigger::DiagTriggerType type, const uint32_t priority, const DiagTrigger::DiagTriggerFunc func, const int32_t triggerId)*/
    android::sp<DiagTrigger> pTrigger{new DiagTrigger(type, prio, DiagTrigger::DiagTriggerFunc::DTC, nextTriggerId)};
    /* set trigger time*/
    pTrigger->setTriggerTime(time);
    LOG_I("Retain time data: %lld sec", time);
    /* Set collection id*/
    pTrigger->setCollectionId(colId);
    LOG_I("Check DTC Collection ID: %llu", colId);

    pTrigger->setLatitude(loc->getLatitude());
    pTrigger->setLongitude(loc->getLongtitude());

    LOG_I("Retain Location data 0x%08x 0x%08x", static_cast<int32_t>(loc->getLatitude()), static_cast<int32_t>(loc->getLongtitude()));
    /* Save request to local*/
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
    /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
    LOG_I("Check mSaveReq size: %d", mSaveReq.size());
    if(!ret.second) {
        ret.first->second = pTrigger;
    }
    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
    LOG_I("Check saved DTC trigger ID: %d", ret.first->first);
}

void RemoteDTC::trigger_DTC(const android::sp<DiagTrigger> pDiag) {
    /*pDiag is check null before calling*/
    /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
    /* Get trigger ID*/
    const uint32_t tmpTriggerId{pDiag->getTriggerId()};
    LOG_I("Trigger DTC start with TriggerId: %d prio: %d", tmpTriggerId, pDiag->getPriority());
    /* Save request to local*/
    (void)mSaveReq.emplace(tmpTriggerId, pDiag);
    // (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pDiag)->sendToTarget();
    /*processing DTC(warning)*/
    if(tmpTriggerId > static_cast<uint32_t>(INT32_MAX)) {
        LOG_E("tmpTriggerId out of range INT32_MAX");
    }
    (void)notifyTrigger(DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING, static_cast<int32_t>(tmpTriggerId), false);
}

void RemoteDTC::sendDTC() 
{
    /* Start DTC */
    LOG_I("Start DTC");
    // isDtcAcquireAborted = false;
    // isDtcAcquireSuspend = false;
    // mStopImmediately = false;
    bool dtcCanRun{true};
    if((isDtcAcquireAborted == true) || (isDtcAcquireSuspend == true) || (mStopImmediately == true)) {
        LOG_I("Stop DTC");
        /*Stop timeout timer*/
        dtcCanRun = false;
        mIsDTCRunning = RemoteDTC::DTC_STOP;
        isDtcAcquireAborted = false;
        isDtcAcquireSuspend = false;
        mStopImmediately = false; 
        PriorityControl::getInstance()->notifyTriggerProcessDone(mTriggerId, mTriggerType);      
    }
    // Get OBC resource
    OBCResourceEventCode resEventInfo{OBCResourceEventCode::OBC_GET_RESOURCE_WAIT};
    /*If DTC can run -> get OBC resource*/
    if(dtcCanRun == true) {
        resEventInfo = OnboardclientAdapter::getInstance()->GetObcResource();
    } else {
        LOG_I("DTC can not run due to abort or suspend");
    }
    
    if ((dtcCanRun == true) && (resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK))
    {
        LOG_E("DTC wait ObcResource");
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(MainHandler::CMD_SEND_DTC), static_cast<uint64_t>(1000U));

    } else if(dtcCanRun == true){
        LOG_I("Get obc resource success");
        OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
        mIsDTCRunning = RemoteDTC::DTC_RUNNING;
    // Using EcuInfomation API to get ECU Candidate informations and remove bellow mockup code in production
    std::list<CommonDefine::EcuInformation> ecuInformationList{};
    RemoteEcuInformation::getInstance()->getEcuInformationList(ecuInformationList);
    LOG_I("Check ecuInformationList size = %d", ecuInformationList.size());
    if (ecuInformationList.size() == 0U)
    {
        LOG_E("ECU list is none");
        DiagManagerAdapter::getInstance()->selfDiagStopOpeartion(DiagManagerAdapter::FAILURE_ACQUIRE_ECU_LIST);
    }

    std::list<CommonDefine::EcuInformation>::iterator it {ecuInformationList.begin()};
    mDTCTransList.clear();
    mDiagResp_CRC.clear();
    mDiagResp_2.clear();
    v_targetEcuList.clear();
    std::queue<uint64_t> empty_queue{};
    std::swap(mTransmissioIdList, empty_queue);
    LOG_I("Start generate DTC request message");
    while(it != ecuInformationList.end()) {
        LOG_I("Check ecuInformationList target_address  = 0x%02lx", it->getTargetAddress());
            /*RDG30-R-0956: if diagphase is 5 and 6*/
            uint8_t sid{static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION)};
            uint8_t sfid{static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_02_REPORT_DTC_BY_STATUS_MASK)};
            uint8_t dtcStatusMask{0x00U};
            if((it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) || 
            (it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6)) {
                LOG_I("ECU is DP_PHASE_5_6");
                sid  = static_cast<uint8_t>(UDS_SID::SID_19_READ_DTC_INFORMATION);
                sfid  = static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_02_REPORT_DTC_BY_STATUS_MASK);
                dtcStatusMask  = 0x0DU;
                const android::sp<DTCUdsTransmission> transmission {new DTCUdsTransmission(*this, *it, sid, sfid, dtcStatusMask)};
                mDTCTransList[transmission->getTransmissionId()] = transmission;
                mTransmissioIdList.push(transmission->getTransmissionId());
                ++it;
            } else if (it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_4) {
                LOG_I("ECU is DP_PHASE_4");
                /*RDG30-R-0957*/
                sid = static_cast<uint8_t>(UDS_SID::SID_13_READ_DTC_PH4);
                sfid = static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_80);
                const android::sp<DTCUdsTransmission> transmission1 {new DTCUdsTransmission(*this, *it, sid, sfid)};
                mDTCTransList[transmission1->getTransmissionId()] = transmission1;
                mTransmissioIdList.push(transmission1->getTransmissionId());

                sid = static_cast<uint8_t>(UDS_SID::SID_13_READ_DTC_PH4);
                sfid = static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_81);
                const android::sp<DTCUdsTransmission> transmission2 {new DTCUdsTransmission(*this, *it, sid, sfid)};
                mDTCTransList[transmission2->getTransmissionId()] = transmission2;
                mTransmissioIdList.push(transmission2->getTransmissionId());

                sid = static_cast<uint8_t>(UDS_SID::SID_13_READ_DTC_PH4);
                sfid = static_cast<uint8_t>(UDS_READ_DTC_INFO_SFID::SFID_B0_READ_DTC);
                const android::sp<DTCUdsTransmission> transmission3 {new DTCUdsTransmission(*this, *it, sid, sfid)};
                mDTCTransList[transmission3->getTransmissionId()] = transmission3;
                mTransmissioIdList.push(transmission3->getTransmissionId());

                ++it;
            } else {
                /*ECU is unknown phase*/
                LOG_E("Unknown phase");
                ++it;
                // continue;
            }
            (void)sid;
            (void)sfid;
            (void)dtcStatusMask;
    }
    LOG_I("Start DTC request sequence");
    if(mTransmissioIdList.size() > 0U) {
        LOG_I("mDTCTransList size = %d", mTransmissioIdList.size());
        mCurrentTransmissionId = mTransmissioIdList.front();
        const TransmissionInter itTrans {mDTCTransList.find(mCurrentTransmissionId)};
        if ((itTrans != mDTCTransList.end()) && (itTrans->second != nullptr)) 
        {
            itTrans->second->connect();
        } else {
            LOG_E("DTC transmission not found or in invalid state");
        }
    } else {
        LOG_I("mTransmissioIdList is empty");
        // Release OBC resource
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        mIsDTCRunning = RemoteDTC::DTC_STOP;
        finishDTC();
    }
    } else {
        LOG_I("DTC can not run due to abort or suspend");
    }
    (void)resEventInfo;
}

void RemoteDTC::finishDTC() {
    LOG_I("DTC is finished, triggerType: %d TriggerID: %d", mTriggerType, mTriggerId);
    /*Generate upload data*/
    const bool isVehicleTrigger{(mTriggerType== DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
        || (mTriggerType== DiagTrigger::DiagTriggerType::IGON_TRIGGER)};

    /*If DTC was NOT aborted => check DTC response error => Generate Error upload data*/
    if((isDtcAcquireAborted == false) && (bUploadErrorData == true)) {
        makeErrorUploadData();
    } else {
        LOG_I("Dont make error upload data");
    }

    if(isVehicleTrigger == true) {
        LOG_I("Trigger is warning or IG_ON => caculate CRC");
        /*If DTC is Aborted -> Dont generate UploadData*/
        const uint8_t crcResult{calculateCRC()};

        if((isDtcAcquireAborted == false) && (crcResult == 0x01U)) {
            /* generte upload data */
            LOG_I("CRC = 1, Have change in response list");
            /* Call to Uploader to make upload data*/
            LOG_I("Generate upload data for DTC");
            triggerToNextService();
            makeUploadData();
            if(mCRCManager != nullptr) {
                /*Caculate CRC*/
                mCRCManager->saveCRC16();
            } else {
                LOG_I("mCRCManager is null");
            }           
        } else {
            /*trigger to next service*/
            LOG_I("CRC = 0, NO change in response list");
            triggerToNextService();
        }
        (void)crcResult;
    } else {
        LOG_I("TriggerType is %d. NOT warning or IG_ON", mTriggerType);
        if(isDtcAcquireAborted == false) {
            LOG_I("Generate upload data for DTC");
            triggerToNextService();
            makeUploadData();
        } else {
            LOG_I("DTC is aborted => do not generate upload data");
        }
    }
    onFinishDTCAcquisition(mTriggerId);
    mIsDTCRunning = RemoteDTC::DTC_STOP;
    finalize();
}

void RemoteDTC::finalize() {
    if(mIsDTCRunning == RemoteDTC::DTC_STOP) {
        LOG_I("DTC is finalized");
        LOG_I("DTC Done");
        if(mTriggerId < 0) {
            LOG_D("mTriggerId is out of range");
            mTriggerId = 0;
        } else {
            /*Do nothing*/
        }
        mApp.notifyDiagDoneToLastUpload(static_cast<uint32_t>(mTriggerId));
        mDiagResp_CRC.clear();
        mDiagResp_2.clear();
        v_targetEcuList.clear();
    }
}

void RemoteDTC::abortDtcProcessing(const vccomif::rdg::v1::interfaces::ResponseCode resCode, const bool stopImmediately) {
    if(isDtcAcquireAborted == false) {
        LOG_I("Start abort DTC processing");
        /*Do abort process*/
        LOG_D("Check isDtcAcquireAborted before: %d", isDtcAcquireAborted);
        isDtcAcquireAborted = true;
        LOG_D("Check isDtcAcquireAborted after: %d", isDtcAcquireAborted);
        mStopImmediately = stopImmediately;
        if(stopImmediately) {
            mIsDTCRunning = RemoteDTC::DTC_STOP;
            /*Disconnect current ECU*/
            LOG_I("Disconnect current TransmitionID: 0x%llx", mCurrentTransmissionId);
            const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
            if ((it != mDTCTransList.end()) && (it->second != nullptr)) {
                it->second->stopTimeoutTimer();
                it->second->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                it->second->disconnect();
                /* Save to Response List */

                /* TBD: Delete matched DTCUdsTransmission in mDTCTransList*/
            } else {
                LOG_I("NOT match transmissionId or ECU was disconnected");
            }
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
        } else {
            LOG_D("Do not stop. Continue process DTC");
        }

        /*RDG30-R-0687 and RDG30-R-0749: Check Trigger name*/
        if((mTriggerType != DiagTrigger::DiagTriggerType::IGON_TRIGGER) &&
            (mTriggerType != DiagTrigger::DiagTriggerType::WARNING_TRIGGER)) {
            /*Notify make error upload data*/
            // (void)mHandler->obtainMessage(MainHandler::CMD_UPLOAD_ERROR)->sendToTarget();
            vccomif::rdg::v1::interfaces::UploadErrorDataRequest mUploadErrorData{};
            mUploadErrorData.set_response_code(((resCode>=vccomif::rdg::v1::interfaces::ResponseCode_MIN) && (resCode<=vccomif::rdg::v1::interfaces::ResponseCode_MAX))
                                                ? static_cast<vccomif::rdg::v1::interfaces::ResponseCode>(resCode)
                                                : vccomif::rdg::v1::interfaces::ResponseCode::RC_OTHER_ERROR);
            makeErrorUploadData(mUploadErrorData, mTriggerType, mColId);
        } else {
            if((mTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) && 
            (resCode == vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION)) {
                /*RDG30-R-0687: IG change to OFF while IG ON trigger running*/
                LOG_D("Do not generate error upload data due to trigger name is IG ON");
            } else if((mTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
            && (resCode == vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION)){
                vccomif::rdg::v1::interfaces::UploadErrorDataRequest mUploadErrorData{};
                mUploadErrorData.set_response_code(static_cast<vccomif::rdg::v1::interfaces::ResponseCode>(resCode));
                makeErrorUploadData(mUploadErrorData, mTriggerType, mColId);
            } else {
                LOG_D("Do not generate error upload data");
            }
            
        }
        if(stopImmediately) {
            // onFinishDTCAcquisition(mTriggerId);
            LOG_I("Check mSaveReq size before: %d", mSaveReq.size());
            if(mTriggerId < 0) {
                LOG_D("mTriggerId is out of range");
                mTriggerId = 0;
            } else {
                /*Do nothing*/
            }
            const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it_diag {mSaveReq.find(static_cast<uint32_t>(mTriggerId))};
            if(it_diag != mSaveReq.end()) {
                (void)mSaveReq.erase(it_diag);
                // mTriggerId = 0; // fix notify done with trigger Id 0 - TMCDCMTF-27298
            } else {
                LOG_I("Can not find triggerId: %d in mSaveReq", mTriggerId);
            }
            LOG_I("Check mSaveReq size after: %d", mSaveReq.size());
            finalize();
            isDtcAcquireAborted = false;
            mStopImmediately = false;
            PriorityControl::getInstance()->notifyTriggerProcessDone(mTriggerId, mTriggerType);
        }
    } else {
        LOG_D("DTC was aborted");
    }
}

void RemoteDTC::discardTrigger(const android::sp<DiagTrigger> pTrigger) {
    LOG_I("Start discard DTC triggerID: %d", pTrigger->getTriggerId());
    vccomif::rdg::v1::interfaces::UploadErrorDataRequest mUploadErrorData{};
    mUploadErrorData.set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED);
    LOG_I("RC_REQUEST_ERROR_INTERRUPT_PROHIBITED");
    const DiagTrigger::DiagTriggerType tmp_type{pTrigger->getType()};
    if((tmp_type >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
            (tmp_type <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)) {
        LOG_I("Trigger type is valid");
    } else {
        LOG_I("Trigger type is out of range");
    }
    makeErrorUploadData(mUploadErrorData, tmp_type, pTrigger->getCollectionID());
}

void RemoteDTC::suspendDtcTrigger(const int32_t triggerId) {
    LOG_I("Start suspend DTC processing");
    isDtcAcquireSuspend = true;
    const TransmissionInter it_ptr {mDTCTransList.find(mCurrentTransmissionId)};
    if ((it_ptr != mDTCTransList.end()) && (it_ptr->second != nullptr) && (it_ptr->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS)) {
        LOG_D("Wait response to suspend process");
    } else {
        LOG_I("NOT match transmissionId or ECU was disconnected");
    }
    (void)triggerId;
}

uint8_t RemoteDTC::calculateCRC() {
    uint8_t isDifferent{0x00U};
    if(mCRCManager != nullptr) {
        /*Caculate CRC*/
        mCRCManager->calculateCRC16Data();
        isDifferent = (mCRCManager->compareCrc16Value() == true)? 0x01U : 0x00U;
    } else {
        isDifferent = 0x00U;
    }
    return isDifferent;
}

void RemoteDTC::triggerToNextService() {
    if(mTriggerId < 0) {
        LOG_I("mTriggerId is out of range");
        mTriggerId = 0;
    } else {
        /*Do nothing*/
    }
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(static_cast<uint32_t>(mTriggerId))};
    if(it != mSaveReq.end() && (it->second != nullptr)) {
        const DiagTrigger::DiagTriggerType pTriggerType{it->second->getType()};
        bool isValidTriggerType {true};
        if ((pTriggerType >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
            (pTriggerType <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX))
        {
            LOG_I("Notify DTC Trigger done: %d", mTriggerId);
        }
        else
        {
            LOG_E("Trigger type is out of range: %d. Must be between %d and %d",
                  pTriggerType,
                  DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN,
                  DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX);
            isValidTriggerType = false;
        }
        // Check pTriggerType is warning, triggertime is set to getWarningTriggerTime
        // else triggertime is set to getTriggerTime
        int64_t triggerTime{0};
        if (isValidTriggerType)
        {
            if (pTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
            {
                triggerTime = it->second->getWarningTriggerTime();
            }
            else
            {
                triggerTime = it->second->getTriggerTime();
            }
            if (triggerTime < 0)
            {
                LOG_I("triggerTime is out of range");
                triggerTime = 0;
            }
            mApp.triggerDTCToSSR(static_cast<uint32_t>(mTriggerId), pTriggerType, triggerTime, mLocationData, mColId, mPriority, v_targetEcuList);
            for (uint32_t i{0U}; i < v_targetEcuList.size(); i++)
            {
                LOG_V("RemoteDTC - Target ECU %u: 0x%02x - DTC %x", i, v_targetEcuList[i].first, v_targetEcuList[i].second);
            }
        } else {
            LOG_E("Invalid trigger type or trigger time");
        }
        (void)triggerTime;
    } else {
        LOG_I("Can not find triggerID in saved list");
    }
}

void RemoteDTC::readCRC() {
    /*TBD: read dtc flag*/
    if(mCRCManager == nullptr) {
        mCRCManager = new CRCManager(*this);
    }
    mCRCManager->readCRC16FromFile();
}

bool RemoteDTC::notifyTrigger(const DiagTrigger::DiagTriggerState& pState,
    const int32_t& pTriggerId, const bool dueToIgOff) 
{
    bool isDTCTrigger{false};
    int32_t pTriggerId_tmp{pTriggerId};
    /* Check if trigger is in the saved trigger*/
    if(pTriggerId_tmp < 0) {
        LOG_I("mTriggerId is out of range");
        pTriggerId_tmp = 0;
    } else {
        /*Do nothing*/
    }
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(static_cast<uint32_t>(pTriggerId_tmp))};
    if((it != mSaveReq.end()) && (it->second != nullptr)) {
        LOG_I("notify DTC Trigger state: %d TriggerID: %d TriggerTime: %lld", pState, pTriggerId_tmp, it->second->getTriggerTime());
        isDTCTrigger = true;
        handleTrigger(pState, pTriggerId_tmp, dueToIgOff);
    } else {
        LOG_I("Can not find triggerID in saved list");
        isDTCTrigger = false;
        // PriorityControl::getInstance()->notifyTriggerNoFound(pTriggerId);
        /*Trigger next service*/
        mApp.forwardDtcToSsr(pState, pTriggerId, dueToIgOff);
    }
    return isDTCTrigger;
}

void RemoteDTC::onFinishDTCAcquisition(const int32_t& triggerId) {
    /* Check if pTriggerId is in savedID then notify done to priorityControl*/
    LOG_I("Finish DTC Acquisition triggerID: %d", triggerId);
    LOG_I("Check mSaveReq size before: %d", mSaveReq.size());
    int32_t triggerId_tmp{triggerId};
    if(triggerId_tmp < 0) {
        LOG_E("mTriggerId is out of range uint32_t");
        triggerId_tmp = 0;
    }
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(static_cast<uint32_t>(triggerId_tmp))};
    // android::sp<DiagTrigger> pTrigger{nullptr};
    if(it != mSaveReq.end()) {
        // pTrigger = it->second;
        (void)mSaveReq.erase(it);
    } else {
        LOG_I("Can not find triggerId: %d in mSaveReq", triggerId_tmp);
    }
    // if(pTrigger != nullptr) {
    //     const DiagTrigger::DiagTriggerType pTriggerType {pTrigger->getType()};
    //     if((pTriggerType >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
    //         (pTriggerType <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)) {
    //         LOG_I("Notify DTC Trigger done: %d", triggerId);
    //     } else {
    //         LOG_I("Trigger type is out of range");
    //     }
    //     // PriorityControl::getInstance()->notifyTriggerProcessDone(triggerId, pTriggerType);
        
    // } else {
    //     LOG_I("Can not find triggerId: %d in mSaveReq", triggerId);
    // }
    LOG_I("Check mSaveReq size after: %d", mSaveReq.size());
}

void RemoteDTC::handleTrigger(const DiagTrigger::DiagTriggerState& pState,
    const int32_t& pTriggerId, const bool dueToIgOff) 
{
    if(pTriggerId < 0) {
        LOG_I("mTriggerId is out of range");
    } else {
        /*Do nothing*/
    }
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(static_cast<uint32_t>(pTriggerId))};
    android::sp<DiagTrigger> pTrigger{nullptr};
    if(it != mSaveReq.end()) {
        pTrigger = it->second;
    } else {
        LOG_I("Can not find triggerId: %d in mSaveReq", pTriggerId);
    }
    if(pTrigger != nullptr) {
        const DiagTrigger::DiagTriggerType pTriggerType {pTrigger->getType()};
        const uint64_t colID{pTrigger->getCollectionID()};
        const uint32_t prio_data{pTrigger->getPriority()};
        if ((pTriggerType >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
            (pTriggerType <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX))
        {
            LOG_I("Notify DTC Trigger done: %d", pTriggerId);
        }
        else
        {
            LOG_I("Trigger type is out of range");
        }
        LOG_I("notify DTC Trigger state: %d TriggerID: %d", pState, pTriggerId);
        switch (pState) {
        case DiagTrigger::DiagTriggerState::TRIGGER_STATE_MIN:
        {
            LOG_I("TRIGGER_STATE_MIN");
            PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_PENDING:
        {
            LOG_I("TRIGGER_PENDING");
            pTrigger->changeState(pState);
            PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING:
        {
            LOG_I("TRIGGER_PROCESSING");
            pTrigger->changeState(pState);
            /* DTC can run */
            const uint8_t rdgFlag{DiagManagerAdapter::getInstance()->getRDGFlag()};
            const IG_STATUS igStatus{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
            const uint8_t dtcFlag{DiagManagerAdapter::getInstance()->getDTCFlag()};
            const bool allUploadConsent{DiagManagerAdapter::getInstance()->getAllUploadConsent()};
            const uint8_t underRepairStatus{mApp.getUnderRepair()};
            mTriggerId = pTriggerId;
            mTriggerType = pTriggerType;
            mColId = colID;
            mPriority = prio_data;
            isDtcAcquireAborted = false;
            isDtcAcquireSuspend = false;
            mStopImmediately = false;
            /*Get time and location*/
            const android::sp<CommonDefine::RDGLocationData> loc{new CommonDefine::RDGLocationData()};
            loc->setLatitude(pTrigger->getLatitude());
            loc->setLongitude(pTrigger->getLongtitude());
            mLocationData = loc;
            if((pTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)) {
                const int64_t triggerTime {pTrigger->getTriggerTime()};
                if (triggerTime < 0)
                {
                    LOG_E("Invalid trigger time: %lld", triggerTime);
                    // Handle the error appropriately, e.g., set a default value or return
                    mDiagnosticsAcquisitionTime = 0LLU;
                }
                else
                {
                    mDiagnosticsAcquisitionTime = static_cast<uint64_t>(triggerTime);
                    LOG_I("Diagnostics Acquisition Time set to: %llu", mDiagnosticsAcquisitionTime);
                }

            LOG_I("DTC trigger Time: %lld", mDiagnosticsAcquisitionTime);

            if ((rdgFlag == 1U) && (igStatus == IG_STATUS_ON) && (dtcFlag == 1U) && (allUploadConsent == true))
            {
                LOG_I("AllDiag executes Conditions is PASS");
                
                mIsDTCRunning = RemoteDTC::DTC_RUNNING;
                if(underRepairStatus == 0x00U) {
                    LOG_D("Under repair status is false");
                    (void)mHandler->obtainMessage(MainHandler::CMD_SEND_DTC)->sendToTarget();
                } else {
                    LOG_D("Under repair status is true");
                    LOG_D("Alldiag is restricted due to under repair status");
                    LOG_D("RC_OBE_ERROR_UNDER_REPAIR");
                    constexpr vccomif::rdg::v1::interfaces::ResponseCode resCode{vccomif::rdg::v1::interfaces::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR};
                    abortDtcProcessing(resCode, true);
                    PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
                }
            } else {
                LOG_I("AllDiag executes Conditions is FAIL");
                vccomif::rdg::v1::interfaces::ResponseCode resCode{vccomif::rdg::v1::interfaces::ResponseCode::RC_OTHER_ERROR};
                if ((rdgFlag != 1U) || (dtcFlag != 1U))
                {
                    LOG_I("RC_REQUEST_ERROR_UNPROVIDED_VEHICLE");
                    resCode = vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_UNPROVIDED_VEHICLE;
                }

                if (igStatus != IG_STATUS_ON)
                {
                    LOG_I("RC_VEHICLE_ERROR_POWER_CONDITION");
                    if(resCode > vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION) {
                        resCode = vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION;
                    }
                }

                if(allUploadConsent != true) {
                    LOG_I("All upload consent is not 10b");
                }
                if(resCode != vccomif::rdg::v1::interfaces::ResponseCode::RC_OTHER_ERROR) {
                    abortDtcProcessing(resCode, true);
                } else {
                    LOG_I("RC_OTHER_ERROR. Do not notify error upload");
                }
                PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            }
            } else if (pTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
                LOG_I("DTC running by IG ON trigger");
                const int64_t triggerTime {pTrigger->getTriggerTime()};
                if (triggerTime < 0)
                {
                    LOG_E("Invalid trigger time: %lld", triggerTime);
                    // Handle the error appropriately, e.g., set a default value or return
                    mDiagnosticsAcquisitionTime = 0U;
                }
                else
                {
                    mDiagnosticsAcquisitionTime = static_cast<uint64_t>(triggerTime);
                    LOG_I("Diagnostics Acquisition Time set to: %llu", mDiagnosticsAcquisitionTime);
                }

                mIsDTCRunning = RemoteDTC::DTC_RUNNING;
                if(underRepairStatus == 0x00U) {
                    LOG_D("Under repair status is false");
                    (void)mHandler->obtainMessage(MainHandler::CMD_SEND_DTC)->sendToTarget();
                } else {
                    LOG_D("Under repair status is true");
                    LOG_D("DTC IG ON Trigger is restricted due to under repair status");
                    PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
                }
            } else if (pTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
                /* Handle warning trigger*/
                LOG_I("DTC running by warning trigger");
                mIsDTCRunning = RemoteDTC::DTC_RUNNING;
                const int64_t warningTriggerTime {pTrigger->getWarningTriggerTime()};
                if (warningTriggerTime < 0)
                {
                    LOG_E("Invalid warning trigger time: %lld", warningTriggerTime);
                    // Handle the error appropriately, e.g., set a default value or return
                    mWarningTriggerOccurrenceTime = 0U;
                }
                else
                {
                    mWarningTriggerOccurrenceTime = static_cast<uint64_t>(warningTriggerTime);
                    LOG_I("Warning Trigger Occurrence Time set to: %llu", mWarningTriggerOccurrenceTime);
                }
                if(underRepairStatus == 0x00U) {
                    LOG_D("Under repair status is false");
                    (void)mHandler->obtainMessage(MainHandler::CMD_SEND_DTC)->sendToTarget();
                } else {
                    LOG_D("Under repair status is true");
                    LOG_D("DTC Warning trigger is restricted due to under repair status");
                    LOG_D("Do not notify error upload");
                    mIsDTCRunning = RemoteDTC::DTC_STOP;
                    PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
                }
            } else {
                LOG_D("Undefined DiagTriggerType");
            }
            (void)rdgFlag;
            (void)igStatus;
            (void)dtcFlag;
            (void)allUploadConsent;
            (void)underRepairStatus;
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED:
        {
            LOG_I("TRIGGER_SUSPENDED");
            /* DTC suspend */
            mTriggerId = pTriggerId;
            mTriggerType = pTriggerType;
            mColId = colID;
            mPriority = prio_data;
            pTrigger->changeState(pState);
            suspendDtcTrigger(mTriggerId);
            // PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
        {
            LOG_I("TRIGGER_DISCARDED");
            pTrigger->changeState(pState);
            if((mIsDTCRunning == 1U) && (mTriggerId == pTriggerId)) {
                if(dueToIgOff == true) {
                    abortDtcProcessing(vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION, true);
                } else {
                    abortDtcProcessing(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED, false);
                }
            } else {
                /*discard diag trigger*/
                discardTrigger(pTrigger);
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
                PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
                (void)mSaveReq.erase(it);
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DONE:
        {
            LOG_I("TRIGGER_DONE");
            pTrigger->changeState(pState);
            PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_STATE_MAX:
        {
            LOG_I("TRIGGER_STATE_MAX");
            pTrigger->changeState(pState);
            PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        }
        (void)prio_data;
        (void)pTriggerType;
        (void)colID;
        (void)pTriggerType;
    } else {
        LOG_I("Can not find triggerID in saved list");
    }
}

// android::sp<Timer> RemoteDTC::getTickTimer() const {
//     return mTickTimer;
// };

void RemoteDTC::MainHandler::handleMessage (const android::sp<sl::Message>& handlemsg) 
{
    const int32_t what {handlemsg->what};
    LOG_I({"handler is processing with what: %d"}, what);
    switch (what) {
        case CMD_INIT_DTC:
        {
            LOG_I("CMD_INIT_DTC");
            mDTC.init();
            break;
        }
        case CMD_CHANGE_IG_STATUS:
        {
            LOG_I("CMD_CHANGE_IG_STATUS");
            mDTC.changeIGStatus((handlemsg->arg1 != 0) ? true : false);
            break;
        }
        case CMD_READ_SAVED_CRC:
        {
            LOG_I("CMD_READ_SAVED_CRC");
            mDTC.readCRC();
            break;
        }
        case CMD_TRIGGER_FROM_IGON:
        {
            LOG_I("CMD_TRIGGER_FROM_IGON");
            LOG_I("Check IG ON trigger conditions");
            const uint8_t rdgFlag_tmp{DiagManagerAdapter::getInstance()->getRDGFlag()};
            const uint8_t dtcFlag_tmp{DiagManagerAdapter::getInstance()->getDTCFlag()};
            const IG_STATUS igStatus_tmp{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
            const bool allUploadConsentStatus{DiagManagerAdapter::getInstance()->getAllUploadConsent()};
            if( (rdgFlag_tmp == 1U) &&
                (dtcFlag_tmp == 1U) &&
                (igStatus_tmp == IG_STATUS_ON) &&
                (allUploadConsentStatus == true)
            ) {
                LOG_I("Check IG ON trigger conditions PASS");
                /*RDG30-R-1224: Get current time */
                int64_t current_time_millis {CommonUtils::getCurrentAcquisiteTime() * static_cast<int64_t>(1000)};
                if(current_time_millis < static_cast<int64_t>(0x00)) {
                    LOG_D("Get time fail. Set default time value");
                    current_time_millis = 0;
                    LOG_D("Check time: %lld", current_time_millis);
                }
                int64_t current_time {CommonUtils::getCurrentAcquisiteTime()};
                if(current_time > static_cast<int64_t>(0x00000000FFFFFFFF)) {
                    /*RDG30-R-0063*/
                    LOG_I("Time information out of range");
                    current_time = 0;
                } else {
                    LOG_I("Time information valid");
                }
                LOG_I("Start Time to acquire Diag: %lld", current_time);
                LOG_I("Save IG ON trigger start time acquire");
                /*RDG30-R-1225: get location */
                const android::sp<CommonDefine::RDGLocationData> loc{LocationManagerAdapter::getInstance()->getLocationData()};
                LOG_I("Save location IG ON trigger, 0x%08x 0x%08x ", static_cast<int32_t>(loc->getLatitude()), static_cast<int32_t>(loc->getLongtitude()));
                uint64_t colId{static_cast<uint64_t>(0U)};
                // colId = static_cast<uint64_t>(TriggerIDGenerator::getInstance().getNextId());
                /*get collID from CollectionConditionDiagCommon*/
                const std::shared_ptr<CollectionConditionDiagCommon> diagCommon{CollectionCondition::getInstance().getCollectionConditionDiagCommon()};
                if (diagCommon != nullptr)
                {
                    colId = static_cast<uint64_t>(diagCommon->collection_condition_id());
                }
                mDTC.trigger_DTC(DiagTrigger::DiagTriggerType::IGON_TRIGGER, DiagTrigger::PRIO_IG_ON_TRIGGER, colId, current_time, loc);
            } else {
                LOG_I("Check IG ON trigger conditions FAIL");
                LOG_E("rdgFlag: %d dtcFlag: %d", rdgFlag_tmp, dtcFlag_tmp);
                if(igStatus_tmp != IG_STATUS_ON) {
                    LOG_E("igStatus is OFF");
                }
                if(allUploadConsentStatus != true) {
                    LOG_E("allUploadConsentStatus is not determined");
                }
            }
            (void)rdgFlag_tmp;
            (void)dtcFlag_tmp;
            (void)igStatus_tmp;
            (void)allUploadConsentStatus;
            break;
        }
        case CMD_TRIGGER_FROM_WARNING:
        {
            LOG_I("CMD_TRIGGER_FROM_WARNING");

            sp<DiagTrigger> pTrigger {nullptr};
            handlemsg->getObject(pTrigger);
            if (pTrigger != nullptr)
            {
                LOG_I("Trigger Request have Type: %d Func: %d Prio: %d ID: %d ColID: %llu",
                      pTrigger->getType(), pTrigger->getFunc(), pTrigger->getPriority(), pTrigger->getTriggerId(), pTrigger->getCollectionID());

                mDTC.trigger_DTC(pTrigger);
            }
            else
            {
                LOG_E("pTrigger is null");
            }
            break;
        }
        case CMD_TRIGGER_FROM_CENTER:
        {
            LOG_I("CMD_TRIGGER_FROM_CENTER");
            int64_t current_time{0};
            current_time = CommonUtils::getCurrentAcquisiteTime();
            if(current_time < 0) {
                LOG_D("Time data is negative");
            }
            sp<CenterReqData> pCenterReq {nullptr};
            handlemsg->getObject(pCenterReq);
            /* Get priority from Center Request */
            const uint32_t prioCenterReq{pCenterReq->getCenterReq_prio()};
            /* Get collection condition ID*/
            const uint64_t idCenterReq {pCenterReq->getCenterReq_CollectionID()};
            const android::sp<CommonDefine::RDGLocationData> loc{LocationManagerAdapter::getInstance()->getLocationData()};
            LOG_I("Check Col ID: %llu", idCenterReq);
            mDTC.trigger_DTC(DiagTrigger::DiagTriggerType::CENTER_TRIGGER, prioCenterReq, idCenterReq, current_time, loc);
            CollectionCondition::getInstance().onFinishCenterRequestJob(idCenterReq);
            break;
        }
        case CMD_REQUEST_TO_PRIORITY_CONTROL:
        {
            LOG_I("CMD_REQUEST_TO_PRIORITY_CONTROL");
            sp<DiagTrigger> pTrigger {nullptr};
            handlemsg->getObject(pTrigger);
            if (pTrigger != nullptr)
            {
                LOG_I("Trigger Request have Type: %d Func: %d Prio: %d ID: %d ColID: %llu",
                      pTrigger->getType(), pTrigger->getFunc(), pTrigger->getPriority(), pTrigger->getTriggerId(), pTrigger->getCollectionID());
                PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
            }
            else
            {
                LOG_E("pTrigger is null");
            }
            break;
        }
        case CMD_SEND_DTC:
        {
            LOG_I("CMD_SEND_DTC");
            mDTC.sendDTC();
            break;
        }
        case CMD_RECEIVE_STATUS_FROM_CENTER:
        {
            LOG_I("CMD_RECEIVE_STATUS_FROM_CENTER");
            int32_t tmp{handlemsg->arg2};
            if(tmp < 0){
                LOG_E("out of range uint32_t");
                tmp = 0;
            }
            mDTC.changedRemoteStatus(static_cast<int32_t>(handlemsg->arg1), static_cast<uint32_t>(tmp));
            break;
        }
        case CMD_UPLOAD_ERROR:
        {
            LOG_I("CMD_UPLOAD_ERROR");
            mDTC.makeErrorUploadData();
            break;
        }
        case CMD_DTC_FINISH_TRANSMISSION:
        {
            LOG_I("CMD_DTC_FINISH_TRANSMISSION");
            mDTC.finishCurrentTransmission();
            break;
        }
        /*Handle message uds receive*/
        case CMD_RECEIVE_UDS_RESPONSE:
        {
            LOG_I("CMD_RECEIVE_UDS_RESPONSE");
            android::sp<OBCResponseEventInfo> responseEventInfo{nullptr};
            handlemsg->getObject(responseEventInfo);
            if (responseEventInfo != nullptr)
            {
                const android::sp<OBCUDSResInfo> resInfo{responseEventInfo->getResInfo()};
                if (resInfo != nullptr)
                {
                    const android::sp<::Buffer> udsData{resInfo->udsData()};
                    const android::sp<UdsMessage> udsResponse{new UdsMessage()};
                    if (udsData->size() > 0U)
                    {
                        (void)udsResponse->Parser(udsData);
                    }
                    /* responseEventInfo and resInfo andudsResponse make sure is not nullptr from here*/
                    mDTC.handleUDSResponse(responseEventInfo, udsResponse);
                }
                else
                {
                    LOG_E("resInfo is null");
                }
            }
            else
            {
                LOG_E("responseEventInfo is null");
            }
            break;
        }
        case CMD_STOP_RDG:
        {
            const int32_t isStop{handlemsg->arg1};
            if(isStop == 1) {
                LOG_I("CMD_STOP_RDG");
                mDTC.handleStopRDG();
            } else {
                LOG_I("Power source enable RDG");
            }
            break;
        }
        case CMD_TRANSMISSION_TIMEOUT:
        {
            LOG_I("CMD_TRANSMISSION_TIMEOUT");
            mDTC.handleTransmissionTimeout();
            break;
        }
        default:
        {
            break;
        }
    }
}
void RemoteDTC::finishCurrentTransmission(void) {
    LOG_I("finish Current transmission ID: 0x%02llx, mTransmissioIdList size = %d", mCurrentTransmissionId, mTransmissioIdList.size());
    if ( mTransmissioIdList.size() > 1U ) {
        const uint64_t pre_TransmissionId{mCurrentTransmissionId};
        mTransmissioIdList.pop();
        /*If DTC was not aborted && is running=> connect next ECU*/
        LOG_D("Check isDtcAcquireAborted: %d", isDtcAcquireAborted);
        LOG_D("Check isDtcAcquireSuspended: %d", isDtcAcquireSuspend);
        LOG_D("Check mIsDTCRunning: %d", mIsDTCRunning);
        /*Check condition for SUSPEND and DISCARD on each ECU*/
        /*If next TransmissionId and current TransmissionId have same ECU addr 
        => Continue to communicating, do not stop due to SUSPEND|DISCARD*/
        if (!mTransmissioIdList.empty()) {
            mCurrentTransmissionId = mTransmissioIdList.front();
        } else {
            LOG_I("mTransmissioIdList is empty, no next TransmissionId");
            mCurrentTransmissionId = 0x0LLU; // Reset to default value
        }
        
        const uint64_t tmp_Id_Pre{pre_TransmissionId >> 8U};
        const uint64_t tmp_Id_Cur{mCurrentTransmissionId >> 8U};
        LOG_D("tmp_Id_Pre: %llx", tmp_Id_Pre);
        LOG_D("tmp_Id_Cur: %llx", tmp_Id_Cur);
        if((isDtcAcquireSuspend == true) && (tmp_Id_Pre == tmp_Id_Cur)) {
            LOG_D("Do not stop. Continue process on ECU: %llx", mCurrentTransmissionId);
        }
        if((isDtcAcquireAborted == true) && (mStopImmediately == false) && (tmp_Id_Pre == tmp_Id_Cur)) {
            LOG_D("Do not Abort. Continue process on ECU: %llx", mCurrentTransmissionId);
        }
        /*Check condition if should continue to connect ECU*/
        if(((isDtcAcquireAborted == false) && (isDtcAcquireSuspend == false) && (mIsDTCRunning == RemoteDTC::DTC_RUNNING))
            || ((isDtcAcquireSuspend == true) && (tmp_Id_Pre == tmp_Id_Cur))
            || ((isDtcAcquireAborted == true) && (mStopImmediately == false) && (tmp_Id_Pre == tmp_Id_Cur))) {
            const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
            if ((it != mDTCTransList.end()) && (it->second != nullptr))
            {
                it->second->connect();
            } else {
                LOG_E("Transmission ID: 0x%02llx not found in mDTCTransList", mCurrentTransmissionId);
            }
        } else {
            LOG_I("Stop DTC => dont process next ECU");
            // OnboardclientAdapter::getInstance()->ReleaseObcResource();
            /*Stop timeout timer*/
            mIsDTCRunning = RemoteDTC::DTC_STOP;
            isDtcAcquireAborted = false;
            isDtcAcquireSuspend = false;
            mStopImmediately = false;
            /*Release OBC resource*/
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            /*Notify done to priority control*/
            PriorityControl::getInstance()->notifyTriggerProcessDone(mTriggerId, mTriggerType);
        }
    } else {
        /*The last transmisionId*/
        if(mTransmissioIdList.empty() != true) {
            mTransmissioIdList.pop();
        }
        /*The mTransmissioIdList is empty here*/
        
        if(mStopImmediately == true) {
            /*Just prevent still receive UDS response when DTC stop immediately*/
            LOG_D("Stop DTC due to stop immediately");
            mIsDTCRunning = RemoteDTC::DTC_STOP;
        }
        else if (((isDtcAcquireAborted == true) && (mStopImmediately == false)) ||
            (isDtcAcquireSuspend == true))
        {
            /*If DTC is aborted or Suspended but not is stop immediately => Stop DTC but not do finish + trigger SSR*/
            LOG_I("Stop DTC due to suspend or abort");
            mIsDTCRunning = RemoteDTC::DTC_STOP;
            /*Release OBC resource*/
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            /*Notify done to priority control*/
            PriorityControl::getInstance()->notifyTriggerProcessDone(mTriggerId, mTriggerType);
        }
        else
        {
            /*Normal case, DTC complete, Make upload data*/
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            mIsDTCRunning = RemoteDTC::DTC_STOP;
            LOG_I("mTransmissioIdList size = %d ->  finished the last ECU -> upload DTC package", mTransmissioIdList.size());
            finishDTC();
            mDTCTransList.clear();
        }
    }
}

void RemoteDTC::onTransmissionTimeout(void) {
    LOG_I("Transmission Timeout, currentTrans = 0x%02llx", mCurrentTransmissionId);
    if(mIsDTCRunning == RemoteDTC::DTC_RUNNING) {
        (void)mHandler->obtainMessage(MainHandler::CMD_TRANSMISSION_TIMEOUT)->sendToTarget();
    } else {
        LOG_D("Timeout while DTC is not running");
    }
}

void RemoteDTC::handleTransmissionTimeout(void) {
    LOG_I("Event Transmission Timeout, currentTrans = 0x%02llx", mCurrentTransmissionId);

    if(mIsDTCRunning == RemoteDTC::DTC_RUNNING) {
        mDiagResp_2.push_back({mCurrentTransmissionId, nullptr});
        const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
        if ((it != mDTCTransList.end()) && (it->second != nullptr))
        {
            if(it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_REMOTE_SS) {
                LOG_D("Time out when DTC_TRANS_REMOTE_SS");
                /*Disconnect*/
                it->second->disconnect();
                finishCurrentTransmission();
            } else if(it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS) {
                LOG_D("Time out when DTC_TRANS_SEND_UDS");
                /*If phase 5 => Change back to default
                else disconnect*/
                if(it->second->getEcuInformation().getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) {
                    it->second->changeToDefaultSS();
                } else {
                    finishCurrentTransmission();
                }
            } else if(it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_DEFAULT_SS) {
                LOG_D("Time out when DTC_TRANS_DEFAULT_SS");
                /*Disconnect*/
                it->second->disconnect();
                finishCurrentTransmission();
            } else {
                LOG_D("Undefined state");
            }
        } else {
            LOG_E("Transmission ID: 0x%02llx not found in mDTCTransList", mCurrentTransmissionId);
        }
        // finishCurrentTransmission();
        if(isDtcAcquireAborted == true) {
            LOG_D("Timeout while suspending");
            mIsDTCRunning = RemoteDTC::DTC_STOP;
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            PriorityControl::getInstance()->notifyTriggerProcessDone(mTriggerId, mTriggerType);
        }
    } else {
        LOG_D("Timeout while DTC is not running");
    }

}

void RemoteDTC::triggerFromWarning(
    const uint32_t triggerId
    , const DiagTrigger::DiagTriggerType triggerType
    , const int64_t timeData
    , const android::sp<CommonDefine::RDGLocationData> location
    , const uint64_t collectionId
    , const uint32_t priority) {
    if(triggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
        LOG_I("Trigger ID: %u", triggerId);
        LOG_I("Trigger Type: %d", triggerType);
        LOG_I("Time Data: %lld", timeData);
        LOG_I("Location Latitude: %d", static_cast<int32_t>(location->getLatitude()));
        LOG_I("Location Longitude: %d", static_cast<int32_t>(location->getLongtitude()));
        LOG_I("Collection ID: %llu", collectionId);
        LOG_I("Priority: %u", priority);
        /* Create NewDiag Trigger*/
        // const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
        const uint32_t nextTriggerId{triggerId};
        /* DiagTrigger(const DiagTrigger::DiagTriggerType type, const uint32_t priority, const DiagTrigger::DiagTriggerFunc func, const int32_t triggerId)*/
        const android::sp<DiagTrigger> pTrigger{new DiagTrigger(triggerType, priority, DiagTrigger::DiagTriggerFunc::DTC, nextTriggerId)};
        /* set trigger time*/
        pTrigger->setWarningTriggerTime(timeData);
        if(timeData >= 0){
            mWarningTriggerOccurrenceTime = static_cast<uint64_t>(timeData);
        } else {
            LOG_D("timeData is negative");
        }
        
        LOG_I("Retain time data: %lld sec", timeData);
        /* Set collection id*/
        pTrigger->setCollectionId(collectionId);
        LOG_I("Check DTC Collection ID: %llu", collectionId);

        pTrigger->setLatitude(location->getLatitude());
        pTrigger->setLongitude(location->getLongtitude());

        LOG_I("Retain Location data 0x%x 0x%x", static_cast<int32_t>(location->getLatitude()), static_cast<int32_t>(location->getLongtitude()));
        /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
        (void)mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_WARNING, pTrigger)->sendToTarget();

    } else {
        LOG_I("Trigger type is not WARNING_TRIGGER");
    }
}

void RemoteDTC::makeUploadData() {
    LOG_I("Make DTC upload data");
    uint32_t sizeOfFileCounter {0U};
    
    mDTCDataReq = std::shared_ptr<vccomif::rdg::v1::interfaces::UploadDtcDataRequest>(
        new vccomif::rdg::v1::interfaces::UploadDtcDataRequest());
    sizeOfFileCounter = mDTCDataReq->ByteSizeLong();

    /*RdgCommonRequestHeader*/
    // text_version
    mDTCDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
    // electronic_pf
    // geodesy_information
    // time_zone_offset
    mDTCDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
    mDTCDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());
    mDTCDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    mDTCDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(CommonUtils::getGeodesyInfo());
    /*interface_type*/
    mDTCDataReq->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_DTC_DATA);
    /*message_id*/
    mDTCDataReq->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_DTC_DATA, UploadManager::getInstance()->getCounterMessage()));
    /*counter_value*/
    mDTCDataReq->set_counter_value(UploadManager::getInstance()->getCounterValue());
    /*collection_condition_id*/
    mDTCDataReq->set_collection_condition_id(mColId);
    /*trigger_type*/
    if (mTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
        mDTCDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_WARNING_TRIGGER);
    } else if (mTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
        mDTCDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_IG_ON_TRIGGER);
    } else if (mTriggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER) {
        mDTCDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_OCCURRENCE_NOTIFICATION_TRIGGER);
    } else if ((mTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
            || (mTriggerType == DiagTrigger::DiagTriggerType::IGOFF_TRIGGER)
            || (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)) {
        mDTCDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER);
    } else {
        mDTCDataReq->set_trigger_type(RdgProtoInterface::TriggerType::TT_UNKNOWN);
    }
    /*diagnostics_acquisition_time || warning_trigger_occurrence_time*/
    if (mTriggerType != DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        mDTCDataReq->set_diagnostics_acquisition_time(mDiagnosticsAcquisitionTime);
    } else {
        mDTCDataReq->set_warning_trigger_occurrence_time(mWarningTriggerOccurrenceTime);
    }
    /*location*/
    RdgProtoInterface::Location* const location {mDTCDataReq->mutable_location()};
    if (mLocationData != nullptr) {
        location->set_latitude(mLocationData->getLatitude()); 
        location->set_longitude(mLocationData->getLongtitude());
    } else {
        LOG_E("mLocationData is null");
        location->set_latitude(0x7FFFFFFE); 
        location->set_longitude(0x7FFFFFFE);
    }

    /*obd2_installed_flag*/
    const bool bOBDFlag{mApp.getOBDStatus()};
    mDTCDataReq->set_obd2_installed_flag(bOBDFlag);
    /*oneof_odo_information*/
    uint32_t odo_value;
    uint32_t odo_unit;
    (void)VehicleManagerAdapter::getInstance()->getOdoInformation(odo_value, odo_unit); // odo value
    if (odo_unit == 2U)
    {
        mDTCDataReq->set_odo_information_mile(odo_value); // odo_unit = 10b ~ Mile
    }
    else if (odo_unit == 1U)
    {
        mDTCDataReq->set_odo_information_km(odo_value); // odo_unit = 01b ~ km
    }
    else
    {
        mDTCDataReq->set_odo_information_km(0xFFFFFFFF); // RDG30-R-1050 undefined value
    }
    /*under_repair_flag*/
    mDTCDataReq->set_under_repair_flag((mApp.getUnderRepair() == 0x01U) ? true:false);
    /*DiagnosticsMessage
        - status_code
        - ecu_address_information
        - user_data*/
    for(std::deque<std::pair<uint64_t, android::sp<UdsMessage>>>::iterator it {mDiagResp_2.begin()}; it != mDiagResp_2.end(); it++) {
        RdgProtoInterface::DiagnosticsMessage tmpDiagMessage{};
        const uint64_t transmissionId{it->first};
        const android::sp<UdsMessage> udsResponse{it->second};
        const TransmissionInter it_ptr {mDTCTransList.find(transmissionId)};
        if ((it_ptr != mDTCTransList.end()) && (it_ptr->second != nullptr))
        {
            LOG_D("transmissionId: %llx", transmissionId);
            if (it->second == nullptr) {
                LOG_D("SC_UNRESPONSIVE");
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_UNRESPONSIVE);
                android::sp<::Buffer> reqData{nullptr};
                if(it_ptr->second->mpUdsReqLast != nullptr) {
                    reqData = it_ptr->second->mpUdsReqLast->ToUdsData();
                } else {
                    LOG_D("mpUdsReqLast is null");
                }
                if ((reqData != nullptr) && (reqData->data() != nullptr))
                {
                    tmpDiagMessage.set_user_data(reqData->data(), reqData->size());
                } else {
                    LOG_D("reqData or reqData->data() is null");
                }
                const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{it_ptr->second->getEcuInformation().getCommProtocol()};
                ecuAddressInfo->set_communication_protocol(protocolType);
                if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                {
                    vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{it_ptr->second->getEcuInformation().getCommType()};
                    if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                        (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                    {
                        commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                    }
                    ecuAddressInfo->set_communication_type(commType);
                }
                ecuAddressInfo->set_target_address(it_ptr->second->getEcuInformation().getTargetAddress());
            }
            else if (it->second->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION)) 
            {
                LOG_D("UDS_PR_READ_DTC_INFORMATION");
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL);
                const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{it_ptr->second->getEcuInformation().getCommProtocol()};
                ecuAddressInfo->set_communication_protocol(protocolType);
                if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                {
                    vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{it_ptr->second->getEcuInformation().getCommType()};
                    if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                        (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                    {
                        commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                    }
                    ecuAddressInfo->set_communication_type(commType);
                }
                ecuAddressInfo->set_target_address(it_ptr->second->getRxAdd());
                uint8_t* const dtcUserData{(it->second->ToUdsData()->data())};
                if(dtcUserData != nullptr){
                    tmpDiagMessage.set_user_data(dtcUserData, it->second->ToUdsData()->size());
                } else {
                    LOG_D("dtcUserData is nullptr");
                }
            } 
            else if (it->second->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE)) 
            {
                LOG_D("UDS_NEGATIVE_RESPONSE");
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL_WITH_NEGATIVE);
                const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{it_ptr->second->getEcuInformation().getCommProtocol()};
                ecuAddressInfo->set_communication_protocol(protocolType);
                if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                {
                    vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{it_ptr->second->getEcuInformation().getCommType()};
                    if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                        (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                    {
                        commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                    }
                    ecuAddressInfo->set_communication_type(commType);
                }
                ecuAddressInfo->set_target_address(it_ptr->second->getRxAdd());
                uint8_t* const dtcUserData{(it->second->ToUdsData()->data())};
                if(dtcUserData != nullptr){
                    tmpDiagMessage.set_user_data(dtcUserData, it->second->ToUdsData()->size());
                } else {
                    LOG_D("dtcUserData is nullptr");
                }
                // mIsNeedUploadErrorData = true;
            }
            else if (it->second->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION_PHASE4))
            {
                LOG_D("UDS_PR_READ_DTC_INFORMATION_PHASE4");
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL);
                const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{it_ptr->second->getEcuInformation().getCommProtocol()};
                ecuAddressInfo->set_communication_protocol(protocolType);
                if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                {
                    vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{it_ptr->second->getEcuInformation().getCommType()};
                    if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                        (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                    {
                        commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                    }
                    ecuAddressInfo->set_communication_type(commType);
                }
                ecuAddressInfo->set_target_address(it_ptr->second->getRxAdd());
                uint8_t* const dtcUserData{(it->second->ToUdsData()->data())};
                if(dtcUserData != nullptr){
                    tmpDiagMessage.set_user_data(dtcUserData, it->second->ToUdsData()->size());
                } else {
                    LOG_D("dtcUserData is nullptr");
                }
            }
            else {
                // Do nothing
                LOG_D("Unknown SID");
            }

            // RDG30-R-0356
            const uint32_t tmp_size{static_cast<uint32_t>(tmpDiagMessage.ByteSizeLong())};
            if(sizeOfFileCounter > (UINT32_MAX - tmp_size)) {
                LOG_E("May wrap error");
            }
            if ((sizeOfFileCounter + tmp_size) > mMaxUploadFileSize) 
            {
                LOG_I("DTC upload data exceeds 4MB -> discard exceeds data");
                if (tmpDiagMessage.ByteSizeLong() > 0U)
                {
                    // RDG30-R-1171
                    tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL_WITH_EXCEEDED_SIZE);
                    // const vccomif::rdg::v1::interfaces::StatusCode tmp_statuscode{tmpDiagMessage.status_code()};
                    uint32_t sizeOfStatusCode{0U};
                    sizeOfStatusCode = sizeof(vccomif::rdg::v1::interfaces::StatusCode);
                    const uint32_t sizeOfEcuAddressInformation {static_cast<uint32_t>(tmpDiagMessage.ecu_address_information().ByteSizeLong())};
                    if(sizeOfFileCounter > (UINT32_MAX - sizeOfStatusCode)) {
                        LOG_E("May wrap error");
                    }
                    const uint32_t tmp{sizeOfFileCounter + sizeOfStatusCode};
                    if(sizeOfEcuAddressInformation > (UINT32_MAX - tmp)) {
                        LOG_E("May wrap error");
                    }
                    const uint32_t remainingBytes {mMaxUploadFileSize - (tmp + sizeOfEcuAddressInformation)};
                    uint8_t* const tmp_ptr{it->second->ToUdsData()->data()};
                    if(tmp_ptr != nullptr) {
                        tmpDiagMessage.set_user_data(tmp_ptr, remainingBytes < it->second->ToUdsData()->size() ? remainingBytes : it->second->ToUdsData()->size());
                    } else {
                        LOG_D("tmp_ptr is nullptr");
                    }
                    LOG_I("Assigne status_code = SC_SUCCESSFUL_WITH_EXCEEDED_SIZE");
                    (void)remainingBytes;
                }
                break;
            } else {
                const vccomif::rdg::v1::interfaces::StatusCode tmp_StatusCode  {tmpDiagMessage.status_code()};
                if(tmp_StatusCode != vccomif::rdg::v1::interfaces::StatusCode::SC_UNKNOWN) {
                    const uint32_t tmp{static_cast<uint32_t>(tmpDiagMessage.ByteSizeLong())};
                    if(sizeOfFileCounter > (UINT32_MAX - tmp)) {
                        LOG_E("May wrap error");
                    }
                    sizeOfFileCounter += tmp;
                    vccomif::rdg::v1::interfaces::DiagnosticsMessage* const diagMessage {mDTCDataReq->add_dtc_messages()};
                    diagMessage->CopyFrom(tmpDiagMessage);                
                } else {
                    LOG_E("StatusCode::SC_UNKNOWN");
                }
            }
        } else {
            LOG_I("Don't find id in mDTCTransList");
        }
    }

    std::string DTCDataReq_Str{""};
    google::protobuf::util::JsonOptions option{};
    option.always_print_primitive_fields = true;
    option.preserve_proto_field_names = true;
    (void)google::protobuf::util::MessageToJsonString(*mDTCDataReq, &DTCDataReq_Str, option);
    printData(DTCDataReq_Str);

    /*Save UploadDtcDataRequest to file*/ 
    const uint32_t uploadId {UploadManager::getInstance()->genRequestId()};
    // std::string file_dir {std::to_string(uploadId)};
    const uint64_t uploadCount {UploadManager::getInstance()->genCountUpload()};
    std::string file_dir {std::to_string(uploadCount)};
    (void)file_dir.append("_UploadDtcDataRequest.dat");
    uint32_t fileSize{0U};
    error_t bSaved{E_ERROR};
    const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
    if (region == LGE_REGION::LGE_REGION_CN)
    {
        bSaved = DataModel<UploadDtcDataRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG030, file_dir, *mDTCDataReq, fileSize);
    }
    else
    {
        fileSize = mDTCDataReq->ByteSizeLong();
        bSaved = DataModel<UploadDtcDataRequest>::saveUpload(file_dir, *mDTCDataReq);
    }
    if (bSaved == E_OK)
    {
        const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
        DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        const android::sp<UploadTask> task {new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG030);
        task->setUploadPatch(file_dir);
        // task->setUploadId(uploadId);
        task->setFileSize(static_cast<uint64_t>(fileSize));
        /*Set priority*/
        task->setUploadPrio(mPriority);
        // test_saveUploadData = task;
        (void)sizeOfFileCounter;
        UploadManager::getInstance()->requestUploadTask(task);
    }
    else
    {
        LOG_E("Save UploadDtcDataRequest file failed");
    }
    
}

void RemoteDTC::handleUnresponsiveEcu(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
    const uint16_t connectId {responseEventInfo->resInfo()->connectId()};

    const TransmissionInter it{mDTCTransList.find(mCurrentTransmissionId)};
    android::sp<DTCUdsTransmission> tmp_DtcTrans{nullptr};
    if (it != mDTCTransList.end())
    {
        tmp_DtcTrans = it->second;
    }
    else
    {
        LOG_E("NOT match transmissionId");
    }
    if (tmp_DtcTrans != nullptr)
    {
        if (tmp_DtcTrans->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS)
        {
            if (tmp_DtcTrans->getConnectId() == connectId)
            {
                tmp_DtcTrans->stopTimeoutTimer();
                mDiagResp_2.push_back({mCurrentTransmissionId, nullptr});
                if (tmp_DtcTrans->getEcuInformation().getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5)
                {
                    tmp_DtcTrans->changeToDefaultSS();
                }
                else
                {
                    tmp_DtcTrans->disconnect();
                    tmp_DtcTrans->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                    finishCurrentTransmission();
                }
            }
            else
            {
                LOG_D("connectId not matched");
            }
        }
        else if (tmp_DtcTrans->getState() == DTCUdsTransmission::State::DTC_TRANS_REMOTE_SS)
        {
            /**/
            tmp_DtcTrans->stopTimeoutTimer();
            tmp_DtcTrans->disconnect();
            tmp_DtcTrans->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
            mDiagResp_2.push_back({mCurrentTransmissionId, nullptr});
            finishCurrentTransmission();
        }
        else if (tmp_DtcTrans->getState() == DTCUdsTransmission::State::DTC_TRANS_DEFAULT_SS)
        {
            tmp_DtcTrans->stopTimeoutTimer();
            tmp_DtcTrans->disconnect();
            tmp_DtcTrans->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
            finishCurrentTransmission();
        }
        else
        {
            LOG_D("Other state");
        }
    }
    else
    {
        LOG_E("tmp_DtcTrans is nullptr");
    }
    (void)responseEventInfo;
    (void)udsResponse;
    (void)connectId;
}

void RemoteDTC::handleStopRDG() {
    LOG_I("handleStopRDG");
    if (mIsDTCRunning == RemoteDTC::DTC_RUNNING)
    {
        mIsDTCRunning = RemoteDTC::DTC_STOP;
        isDtcAcquireAborted = false;
        isDtcAcquireSuspend = false;
        mStopImmediately = false;
        LOG_I("Disconnect current TransmitionID: 0x%llx", mCurrentTransmissionId);
        const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
        android::sp<DTCUdsTransmission> tmp_DtcTrans{nullptr};
        if(it != mDTCTransList.end()) {
            tmp_DtcTrans = it->second;
        }
        if (tmp_DtcTrans != nullptr) {
            tmp_DtcTrans->stopTimeoutTimer();
            tmp_DtcTrans->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
            tmp_DtcTrans->disconnect();
        } else {
            LOG_I("NOT match transmissionId or ECU was disconnected");
        }
        mDTCTransList.clear();
        mDiagResp_CRC.clear();
        mDiagResp_2.clear();
        v_targetEcuList.clear();
        mSaveReq.clear();
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
    }
    else
    {
        LOG_D("DTC is not running");
    }
}

void RemoteDTC::testingMaxFileSize(const uint32_t fileSize) noexcept
{
    mMaxUploadFileSize = fileSize;
}

void RemoteDTC::makeErrorUploadData(void) {
    mDTCUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
    mDTCUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
    mDTCUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());
    mDTCUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    mDTCUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(CommonUtils::getGeodesyInfo());
    mDTCUploadErrorData->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
    mDTCUploadErrorData->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, UploadManager::getInstance()->getCounterMessage()));

    // GeodesyInformation geodesy_information  ===> GeodesyInformation::GI_UNKNOWN  (Need to confirm)
    // appCommonHeader->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_UNKNOWN);
    const sp<CommonDefine::RDGLocationData> mLocation{LocationManagerAdapter::getInstance()->getLocationData()};
    mDTCUploadErrorData->set_collection_condition_id(mColId); /*TBD*/
    mDTCUploadErrorData->set_counter_value(UploadManager::getInstance()->getCounterValue());
    // data_creation_date
        /*diagnostics_acquisition_time || warning_trigger_occurrence_time*/
    if (mTriggerType != DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        mDTCUploadErrorData->set_data_creation_date(mDiagnosticsAcquisitionTime);
    } else {
        mDTCUploadErrorData->set_data_creation_date(mWarningTriggerOccurrenceTime);
    }
    // set error trigger type
    vccomif::rdg::v1::interfaces::TriggerType errorTriggerType;
    if (mTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_WARNING_TRIGGER;
        mDTCUploadErrorData->set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_WARNING_TRIGGER);
    } else if (mTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_IG_ON_TRIGGER;
    } else if (mTriggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_OCCURRENCE_NOTIFICATION_TRIGGER;
        mDTCUploadErrorData->set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_OCCURRENCE_ROB_MONITORING);
    } else if ((mTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
            || (mTriggerType == DiagTrigger::DiagTriggerType::IGOFF_TRIGGER)
            || (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER;
        mDTCUploadErrorData->set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ALL_DIAG);
    } else {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_UNKNOWN;
    }
    mDTCUploadErrorData->set_trigger_type(errorTriggerType);
    // Set OBD2 flag
    const bool bOBDFlag{mApp.getOBDStatus()};
    mDTCUploadErrorData->set_obd2_installed_flag(bOBDFlag);
    mDTCUploadErrorData->set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
    mDTCUploadErrorData->set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ALL_DIAG);

    std::string DTCErrorData_Str{""};
    google::protobuf::util::JsonOptions option{};
    option.always_print_primitive_fields = true;
    option.preserve_proto_field_names = true;
    (void)google::protobuf::util::MessageToJsonString(*mDTCUploadErrorData, &DTCErrorData_Str, option);
    printData(DTCErrorData_Str);

    /*Save UploadDtcDataRequest to file*/ 
    const uint32_t uploadId {UploadManager::getInstance()->genRequestId()};
    const uint64_t uploadCount {UploadManager::getInstance()->genCountUpload()};
    std::string file_dir {std::to_string(uploadCount)};
    (void)file_dir.append("_UploadDtcDataError.dat");
    uint32_t fileSize{0U};
    error_t bSaved{E_ERROR};
    const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
    if (region == LGE_REGION::LGE_REGION_CN)
    {
        bSaved = DataModel<UploadErrorDataRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG160, file_dir, *mDTCUploadErrorData, fileSize);
    }
    else
    {
        fileSize = mDTCUploadErrorData->ByteSizeLong();
        bSaved = DataModel<UploadErrorDataRequest>::saveUpload(file_dir, *mDTCUploadErrorData);
    }
    if (bSaved == E_OK)
    {
        const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
        DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        const android::sp<UploadTask> task {new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
        task->setUploadPatch(file_dir);
        // task->setUploadId(uploadId);
        /*Set priority*/
        task->setUploadPrio(mPriority);
        task->setFileSize(static_cast<uint64_t>(fileSize));
        UploadManager::getInstance()->requestUploadTask(task);  
    }
    else
    {
        LOG_E("Save mDTCUploadErrorData file failed");
    }
    // test_saveUploadData = task;
}

void RemoteDTC::makeErrorUploadData(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colID) {
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(CommonUtils::getGeodesyInfo());
    errorData.mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
    errorData.mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, UploadManager::getInstance()->getCounterMessage()));

    // GeodesyInformation geodesy_information  ===> GeodesyInformation::GI_UNKNOWN  (Need to confirm)
    // appCommonHeader->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_UNKNOWN);
    const sp<CommonDefine::RDGLocationData> mLocation{LocationManagerAdapter::getInstance()->getLocationData()};
    errorData.set_collection_condition_id(mColId); /*TBD*/
    errorData.set_counter_value(UploadManager::getInstance()->getCounterValue());
    // data_creation_date
    if (mTriggerType != DiagTrigger::DiagTriggerType::WARNING_TRIGGER)
    {
        errorData.set_data_creation_date(mDiagnosticsAcquisitionTime);
    } else {
        errorData.set_data_creation_date(mWarningTriggerOccurrenceTime);
    }
    // Set OBD2 flag
    const bool bOBDFlag{mApp.getOBDStatus()};
    errorData.set_obd2_installed_flag(bOBDFlag);
    errorData.set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
    errorData.set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ALL_DIAG);
    // set error trigger type
    vccomif::rdg::v1::interfaces::TriggerType errorTriggerType;
    if (type == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_WARNING_TRIGGER;
        errorData.set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_WARNING_TRIGGER);
    } else if (type == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_IG_ON_TRIGGER;
    } else if (type == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_OCCURRENCE_NOTIFICATION_TRIGGER;
        errorData.set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_OCCURRENCE_ROB_MONITORING);
    } else if ((type == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
            || (type == DiagTrigger::DiagTriggerType::IGOFF_TRIGGER)
            || (type == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER;
        errorData.set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ALL_DIAG);
    } else {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_UNKNOWN;
    }
    errorData.set_trigger_type(errorTriggerType);
    /*Set collection condition*/
    errorData.set_collection_condition_id(colID);
    /*Print data*/

    std::string DTCErrorData_Str{""};
    google::protobuf::util::JsonOptions option{};
    option.always_print_primitive_fields = true;
    option.preserve_proto_field_names = true;
    (void)google::protobuf::util::MessageToJsonString(errorData, &DTCErrorData_Str, option);
    printData(DTCErrorData_Str);

    /*Save UploadDtcDataRequest to file*/ 
    const uint32_t uploadId {UploadManager::getInstance()->genRequestId()};
    const uint64_t uploadCount {UploadManager::getInstance()->genCountUpload()};
    std::string file_dir {std::to_string(uploadCount)};
    (void)file_dir.append("_UploadDtcDataError.dat");
    uint32_t fileSize{0U};
    error_t bSaved{E_ERROR};
    const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
    if (region == LGE_REGION::LGE_REGION_CN)
    {
        bSaved = DataModel<UploadErrorDataRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG160, file_dir, errorData, fileSize);
    }
    else
    {
        fileSize = errorData.ByteSizeLong();
        bSaved = DataModel<UploadErrorDataRequest>::saveUpload(file_dir, errorData);
    }
    if (bSaved == E_OK)
    {
        const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
        DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        const android::sp<UploadTask> task {new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
        task->setUploadPatch(file_dir);
        // task->setUploadId(uploadId);
        task->setFileSize(static_cast<uint64_t>(fileSize));
        /*Set priority*/
        task->setUploadPrio(mPriority);
        // test_saveUploadData = task;
        UploadManager::getInstance()->requestUploadTask(task);
    }
    else
    {
        LOG_E("Save mDTCUploadErrorData file failed");
    }
    
}

std::map<uint64_t, android::sp<UdsMessage>> RemoteDTC::getDiagResponseList() const noexcept {
    return mDiagResp_CRC;
}

void RemoteDTC::changedRemoteStatus(const int32_t what, const uint32_t info) {
    switch (what) {
        case WHAT_CHANGED_REPAIR_SATUS:
        {
            LOG_I("WHAT_CHANGED_REPAIR_SATUS");
            /*RDG30-R-1175*/
            if((mIsDTCRunning == 1U) && (info == 1U)) {
                if((mTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) ||
                    (mTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER)) {
                    LOG_I("Stop DTC and dont notify error");
                } else {
                    /*Stop DTC and notify error*/
                    LOG_I("Stop DTC and notify error");
                }
                abortDtcProcessing(vccomif::rdg::v1::interfaces::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR, false);
            } else {
                LOG_I("DTC is not running");
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

uint8_t RemoteDTC::getUnderRepairStatus() const noexcept{
    return mApp.getUnderRepair();
}

RemoteDTC::DTCUdsTransmission::DTCUdsTransmission(RemoteDTC& dtc
    , const CommonDefine::EcuInformation& ecuInformation_dat
    , const uint8_t aSID
    , const uint8_t aSFID
    , const uint8_t aDtcStatusMask)
: android::RefBase()
, connectId(0x0000U)
, transmissionId(0ULL)
, mState(State::DTC_TRANS_INIT)
, mDTC(dtc)
, mTimerHandler(dtc)
, mTimeOut(&mTimerHandler, TimerHandler::ID_TRANSMISSION_TIMEOUT)
, udsReq(aSID, aSFID, aDtcStatusMask)
, centerRxAdd(0U)
{
    this->mpUdsReqLast = nullptr;
    mTimeOut.setDuration(TRANSMISSION_TIME_OUT_DURATION, 0U);
    this->ecuInformation.setCanId(ecuInformation_dat.getCanId());
    this->ecuInformation.setCommProtocol(ecuInformation_dat.getCommProtocol());
    this->ecuInformation.setCommType(ecuInformation_dat.getCommType());
    this->ecuInformation.setecuActiveFlag(ecuInformation_dat.getecuActiveFlag() == true ? true : false);
    this->ecuInformation.setTargetAddress(ecuInformation_dat.getTargetAddress());

    const CommonDefine::DiagPhase diagPhase{ecuInformation_dat.getDiagPhase()};
    if((diagPhase == CommonDefine::DiagPhase::DP_PHASE_5) ||
        (diagPhase == CommonDefine::DiagPhase::DP_PHASE_6)) {
        /*If ECU Phase 4*/
        this->ecuInformation.setDiagPhase(diagPhase);
        transmissionId |= (0xFFFFFFFFFFFFFFFF & this->ecuInformation.getTargetAddress()) << 8U;
        transmissionId |= (0xFFFFFFFFFFFFFFFF & this->udsReq.getSFID());
    } else if(diagPhase == CommonDefine::DiagPhase::DP_PHASE_4) {
        /*If UCE phase 5 or 6*/
        this->ecuInformation.setDiagPhase(diagPhase);
        transmissionId |= (0xFFFFFFFFFFFFFFFF & this->ecuInformation.getTargetAddress()) << 8U;
        transmissionId |= (0xFFFFFFFFFFFFFFFF & this->udsReq.getSFID());
    } else {
        this->ecuInformation.setDiagPhase(diagPhase);
        LOG_E("Unknown diag phase");
    }
}

void RemoteDTC::DTCUdsTransmission::connect()
{
    LOG_I("Start connect, transmissionID = 0x%02llx", this->transmissionId);

    this->setState(DTCUdsTransmission::State::DTC_TRANS_CONNECT);
    
    const android::sp<OBCTransportInfo> obcTransportInfo {new OBCTransportInfo()};
    OBCCanInfo canInfo{};
    std::vector<std::string> ntaArray{};
    const uint8_t ProtocolType {RemoteEcuInformation::getInstance()->convertObcProtocolType(this->ecuInformation.getCommProtocol(), this->ecuInformation.getCommType(), this->ecuInformation.getTargetAddress())};
    ntaArray.clear();
    if ((ProtocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
        (ProtocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
        (ProtocolType == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
    {
        const uint16_t nTa{static_cast<uint16_t>(((this->ecuInformation.getTargetAddress() >> 8U) & 0xFFU))};
        std::stringstream ss{};
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << nTa;
        const std::string hexString{ss.str()}; // Convert to string
        for (size_t i{0U}; i < hexString.size(); i++)
        {
            const std::string tmp{std::string(1U, hexString[i])};
            ntaArray.push_back(tmp);
        }
    }
    canInfo.setData(this->ecuInformation.getTargetAddress(), ntaArray);
    obcTransportInfo->setData(ProtocolType
                            , canInfo
                            , false
                            , 0U);
    const android::sp<OBCConnectInfo> obj {new OBCConnectInfo()};
    const error_t res {OnboardclientAdapter::getInstance()->connect(obcTransportInfo, "RDG", obj)};
    
    tOBCConnectInfo info;
    obj->setDataFormat(info);
    if (info.response != OBCEnum::OBCErrCode::OBC_OK)
    {
        LOG_E("can't connect to targetAddress = 0x%02llx", this->ecuInformation.getTargetAddress());
        // OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
        this->mState = DTCUdsTransmission::State::DTC_TRANS_DONE;
        mDTC.mDiagResp_2.push_back({this->transmissionId, nullptr});
        (void)mDTC.mHandler->obtainMessage(MainHandler::CMD_DTC_FINISH_TRANSMISSION)->sendToTarget();
    } else {
        LOG_I("connect success transmissionID = 0x%02llx", this->transmissionId);
        this->connectId = info.connectId;
        
        // OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
        /*If ECU phase5: change session
        else: send UDS*/
        if(this->ecuInformation.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) {
            LOG_D("Phase 5");
            this->changeToRemoteSS();
        } else {
            LOG_D("Phase 6_4");
            this->send();
        }
    }
    ntaArray.clear();
    (void)res;
}

void RemoteDTC::DTCUdsTransmission::stopTimeoutTimer() {
    LOG_I("Stop timeout timer for transmittionID: 0x%llx", transmissionId);
    mTimeOut.stop();
}

void RemoteDTC::DTCUdsTransmission::changeToRemoteSS() {
    LOG_I("transmissionID = 0x%02llx", this->transmissionId);
    const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
    pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL));
    pUdsReq->setSFID(0x40U);
    const android::sp<::Buffer> udsData {pUdsReq->ToUdsData()};
    mpUdsReqLast = pUdsReq;

    const uint8_t res{OnboardclientAdapter::getInstance()->sendUdsData(this->connectId, udsData)};
    if (res != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK)) 
    {
        LOG_E("SendUdsData error = %d -> Disconnect", res);
        mDTC.mDiagResp_2.push_back({this->transmissionId, nullptr});
        this->disconnect();
        mDTC.finishCurrentTransmission();
    }
    else {
        this->setState(DTCUdsTransmission::State::DTC_TRANS_REMOTE_SS);
        mTimeOut.start();
        LOG_I("Start timeout timer for transmissionId 0x%02llx", this->transmissionId);
    }
}

void RemoteDTC::DTCUdsTransmission::changeToDefaultSS() {
    LOG_I("transmissionID = 0x%02llx", this->transmissionId);
    const android::sp<UdsMessage> pUdsReq {new UdsMessage()};
    pUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL));
    pUdsReq->setSFID(0x01U);
    const android::sp<::Buffer> udsData {pUdsReq->ToUdsData()};

    const uint8_t res{OnboardclientAdapter::getInstance()->sendUdsData(this->connectId, udsData)};
    if (res != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK)) 
    {
        LOG_E("SendUdsData error = %d -> Disconnect", res);
        // mDTC.mDiagResp_2.push_back({this->transmissionId, nullptr});
        this->disconnect();
        mDTC.finishCurrentTransmission();
    }
    else {
        this->setState(DTCUdsTransmission::State::DTC_TRANS_DEFAULT_SS);
        mTimeOut.start();
        LOG_I("Start timeout timer for transmissionId 0x%02llx", this->transmissionId);
    }
}

void RemoteDTC::DTCUdsTransmission::disconnect()
{
    this->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
    LOG_D("disconnect: 0x%02llx", this->transmissionId);
    (void)OnboardclientAdapter::getInstance()->disconnectECU(this->connectId);
    // OnboardclientAdapter::getInstance()->ReleaseObcResource();
    mState = DTCUdsTransmission::State::DTC_TRANS_DONE;
}

void RemoteDTC::DTCUdsTransmission::send()
{
    const android::sp<::Buffer> udsData {this->udsReq.ToUdsData()};
    this->mpUdsReqLast = new UdsMessage();
    (void)this->mpUdsReqLast->Parser(this->udsReq.ToUdsData());
    const uint8_t res{OnboardclientAdapter::getInstance()->sendUdsData(this->connectId, udsData)};
    if (res != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK)) 
    {
        LOG_E("SendUdsData error = %d -> Disconnect", res);
        mDTC.mDiagResp_2.push_back({this->transmissionId, nullptr});
        this->disconnect();
        mDTC.finishCurrentTransmission();
    }
    else {
        this->setState(DTCUdsTransmission::State::DTC_TRANS_SEND_UDS);
        mTimeOut.start();
        LOG_I("Start timeout timer for transmissionId 0x%02llx", this->transmissionId);
    }
}
RemoteDTC::TimerHandler::TimerHandler(RemoteDTC& dtc) noexcept: TimerTimeoutHandler(), mDTC(dtc) {

}
void RemoteDTC::TimerHandler::handlerFunction (const int32_t timerId)
{
    switch (timerId) {
        case ID_IG_ON_TRIGGER:
        {
            LOG_I("TimerId ID_IG_ON_TRIGGER expired");if(mDTC.getUnderRepairStatus() == 0U) {
                (void)mDTC.mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_IGON)->sendToTarget();
            } else {
                LOG_D("IG ON trigger is restricted due to under repaired. No notify Error");
            }
            break;
        }
        case ID_TRANSMISSION_TIMEOUT:{
            LOG_I("TimerId ID_TRANSMISSION_TIMEOUT expired");
            mDTC.onTransmissionTimeout();
            break;
        }
        default:
            break;
    }
}

}
