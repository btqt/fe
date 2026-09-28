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
        , mCurrentTransmissionId{0U}
        , mIsDTCRunning{RemoteDTC::DTC_STOP}
        , mColId{0ULL}
        , mCRCManager{nullptr}
        , mPriority{0U}
        , bUploadErrorData(false)
        , isDtcAcquireAborted(false)
        , mDTCUploadErrorData(std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest()))
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

void RemoteDTC::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse) {
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
        /*Get NRC*/
        /*Check NRC 0x78*/
        const uint8_t nrc{udsResponse->getNRC()};
        LOG_D("Check NRC: 0x%x", nrc);
        const uint32_t targetAddress {CommonUtils::calTargetAddressFromCanIdRx(protocolType, canInfo->canId())};
        uint32_t centerRxAdd{0U};
        const uint32_t tmpCenterRxAdd{responseEventInfo->getResInfo()->getCanInfo()->getCanId()};
        if (tmpCenterRxAdd <= static_cast<uint32_t>(UINT32_MAX))
        {
            centerRxAdd = tmpCenterRxAdd;
        }
        else
        {
            centerRxAdd = static_cast<uint32_t>(UINT32_MAX);
        }
        if(isDtcAcquireAborted == false) {
        if ((responseCode == OBCEnum::OBCErrCode::OBC_NEGATIVE) || (responseCode == OBCEnum::OBCErrCode::OBC_OK)) {
            if (responseCode == OBCEnum::OBCErrCode::OBC_NEGATIVE) {
                LOG_D("Received negative response from OBC, targetAddress = 0x%02x", targetAddress);
                LOG_D("Processing UDS data and Adding to Error upload data");
            }
            if (sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION))
            {
                /*Process ECU phase 5 and 6 positive response*/
                uint64_t transmissionId {0U};
                transmissionId |= (0xFFFFFFFFFFFFFFFF & targetAddress) << 8U;
                transmissionId |= (0xFFFFFFFFFFFFFFFF & udsResponse->getSFID());
                LOG_I("Check response SID: 0x%x SubFunc: 0x%x", udsResponse->getSID(), udsResponse->getSFID());
                LOG_I("transmissionId: 0x%02llx", transmissionId);
                const TransmissionInter it {mDTCTransList.find(transmissionId)};
                if ((it != mDTCTransList.end()) && (it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS)) {
                    LOG_I("DTC is running and match transmissionId. Process receive UDS");
                    /*Save RX target address to mDTCTransList*/
                    it->second->setRxAdd(centerRxAdd);
                    /* Save to Response List */
                    /*RDG30-R-0230*/
                    const android::sp<UdsMessage> udsResponse_crc{new UdsMessage(udsResponse)};
                    if(udsResponse_crc != nullptr) {
                        if(udsResponse_crc->ToUdsData()->data() != nullptr) {
                        uint8_t* const dtcUserData{&(udsResponse_crc->ToUdsData()->data()[0])};
                        if(dtcUserData != nullptr) {
                            for (uint32_t i {UDS_DTC_AND_STATUS_RECORD_MASK_FOR_DTC + 3U} ; i < udsResponse_crc->ToUdsData()->size(); i += 4U)
                            {
                                LOG_D("StatusOfDTC[%d] before mask status bits: 0x%x", i, dtcUserData[i]);
                                dtcUserData[i] &=  0x0DU; // masking StatusOfDTC  with 0x0D (keep pendingDTC,confirmedDTC and testFailed bits, other set to 0)
                                LOG_D("StatusOfDTC[%d] after mask status bits: 0x%x", i, dtcUserData[i]);
                            }
                        } else {
                            LOG_D("dtcUserData is nullptr");
                        }
                        (void)dtcUserData;
                        }
                        mDiagResp_CRC[transmissionId] = udsResponse_crc;
                    } else {
                        LOG_I("udsResponse_crc is nullptr");
                    }
                    mDiagResp_2.push_back({transmissionId, udsResponse});
                    v_targetEcuList.push_back(targetAddress);

                    if(transmissionId == mCurrentTransmissionId) {
                        it->second->stopTimeoutTimer();
                        it->second->disconnect();
                        it->second->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                        finishCurrentTransmission();
                    } else {
                        LOG_I("Receive expired response");
                    }
                } else {
                    LOG_I("NOT match transmissionId. Process receive UDS");
                }
            } else if((sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE)) && (nrc != 0x78U)) {
                LOG_D("Find transmissionId: 0x%llx", mCurrentTransmissionId);
                const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
                if ((it != mDTCTransList.end()) && (it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS))
                {
                    mDiagResp_CRC[mCurrentTransmissionId] = udsResponse;
                    it->second->setRxAdd(centerRxAdd);
                    if (it->second->getConnectId() == connectId) {
                        it->second->stopTimeoutTimer();
                        it->second->disconnect();
                        mDiagResp_2.push_back({mCurrentTransmissionId, udsResponse});
                        it->second->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                        finishCurrentTransmission();
                    }
                } else {
                    LOG_I("NOT match transmissionId");
                }
            } else if(sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION_PHASE4)) {
                /*Process ECU phase 4 positive response*/
                /*Compare received ECU targetAddress vs (mCurrentTransmissionId >> 8)*/
                LOG_I("targetAddress: %lld", static_cast<uint64_t>(targetAddress));
                LOG_I("targetAddress: %lld", ((mCurrentTransmissionId >> 8U) & 0x00000000FFFFFFFFU));
                if(static_cast<uint64_t>(targetAddress) == ((mCurrentTransmissionId >> 8U) & 0x00000000FFFFFFFFU)) {
                    LOG_I("matched");
                    const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
                    if ((it != mDTCTransList.end()) && (it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS))
                    {
                        it->second->setRxAdd(centerRxAdd);
                        if (it->second->getConnectId() == connectId) {
                            it->second->stopTimeoutTimer();
                            it->second->disconnect();
                            mDiagResp_2.push_back({mCurrentTransmissionId, udsResponse});
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
                const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
                if ((it != mDTCTransList.end()) && (it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS))
                {
                    if (it->second->getConnectId() == connectId) {
                        it->second->stopTimeoutTimer();
                        it->second->disconnect();
                        mDiagResp_2.push_back({mCurrentTransmissionId, nullptr});
                        it->second->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                        finishCurrentTransmission();
                    }
                } else {
                    LOG_I("NOT match transmissionId");
                }
            } 
        } else {
            LOG_D("Other OBCErrCode");
            const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
            if ((it != mDTCTransList.end()) && (it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS))
            {
                if (it->second->getConnectId() == connectId) {
                    it->second->stopTimeoutTimer();
                    it->second->disconnect();
                    mDiagResp_2.push_back({mCurrentTransmissionId, nullptr});
                    it->second->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
                    finishCurrentTransmission();
                }
            } else {
                LOG_I("NOT match transmissionId");
            }
        }
        // (void)responseCode;
        // (void)canInfo;
        // (void)connectId;
        // (void)protocolType;
        // (void)sid;
        // (void)targetAddress;
    } else {
        LOG_I("DTC is suspending");
        /*Stop timeout timer*/
        mIsDTCRunning = RemoteDTC::DTC_STOP;
        const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
        if ((it != mDTCTransList.end()) && (it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS))
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
    } else {
        LOG_D("DTC is not running => do not process UDS response");
    }
}

void RemoteDTC::onChangedRemoteInfo(const int32_t what, const int32_t info) {
    LOG_I("onChangedRemoteInfo");
    // CMD_RECEIVE_STATUS_FROM_CENTER
    (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_STATUS_FROM_CENTER, 
        what, info)->sendToTarget();
}

void RemoteDTC::onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) {
    LOG_I("DTC receive Center request");
    /*TBD: Process center data*/
    const uint32_t prio_data{pCenterReqData->getCenterReq_prio()};
    /*TBD: get warning trigger occurence*/
    const uint64_t tmp_centerReq_CollectionID{pCenterReqData->getCenterReq_CollectionID()};
    uint8_t colId_ptr[sizeof(tmp_centerReq_CollectionID)];
    const uint32_t tmp_size{sizeof(tmp_centerReq_CollectionID)};
    (void)memcpy(&colId_ptr[0], &tmp_centerReq_CollectionID, tmp_size);
    const android::sp<::Buffer> colId_sp {new ::Buffer()};
    if(tmp_size > static_cast<uint32_t>(INT32_MAX)) {
        LOG_E("tmp_size out of range INT32_MAX");
    }
    colId_sp->setTo(colId_ptr, static_cast<int32_t>(tmp_size));
    if(prio_data > static_cast<uint32_t>(INT32_MAX)) {
        LOG_E("prio_data out of range INT32_MAX");
    }
    const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_CENTER, static_cast<int32_t>(prio_data))};
    const uint32_t tmp_size_colId_sp{colId_sp->size()};
    if(tmp_size_colId_sp > static_cast<uint32_t>(INT32_MAX)) {
        LOG_E("colId_sp->size() out of range INT32_MAX");
    }
    msg->buffer.setTo(colId_sp->data(), static_cast<int32_t>(tmp_size_colId_sp));
    (void)msg->sendToTarget();
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
        if(DiagManagerAdapter::getInstance()->getUnderRepairStatus() == 0U) {
            LOG_I("IG ON TRIGGER after 5 mins");
            mTickTimer->start();
        } else {
            LOG_I("Discard IG ON TRIGGER due to Under repair");
        }
    } else {
        LOG_I("IG OFF. Stop IG ON TRIGGER 5min Timer.");
        mTickTimer->stop();
        if(mIsDTCRunning == RemoteDTC::DTC_RUNNING) {
            abortDtcProcessing(vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
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
    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
    LOG_I("Check mSaveReq size: %d", mSaveReq.size());
    if(!ret.second) {
        ret.first->second = pTrigger;
    }
    LOG_I("Check saved DTC trigger ID: %d", ret.first->first);
}

void RemoteDTC::trigger_DTC(const android::sp<DiagTrigger> pDiag) {
    /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
    /* Get trigger ID*/
    const uint32_t tmpTriggerId{pDiag->getTriggerId()};
    LOG_I("Trigger DTC start with TriggerId: %d prio: %d", tmpTriggerId, pDiag->getPriority());
    /* Save request to local*/
    (void)mSaveReq.emplace(tmpTriggerId, pDiag);
    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pDiag)->sendToTarget();
}

void RemoteDTC::sendDTC() 
{
    /* Start DTC */
    LOG_I("Start DTC");
    /* Get Diagnostic Acquisition Start Time*/
    TimeManager &mTimeManagerService {TimeManager::getInstance()};
    int64_t current_time_millis {0};
    current_time_millis = mTimeManagerService.getCurrentMilliSec();
    if(current_time_millis < static_cast<int64_t>(0x00)) {
        LOG_D("Get time fail. Set default time value");
        current_time_millis = 0;
        LOG_D("Check time: %lld", current_time_millis);
    }
    int64_t current_time_sec {ParamsDef::getCurrentAcquisiteTime()};
    if(current_time_sec > static_cast<int64_t>(0x00000000FFFFFFFF)) {
        /*RDG30-R-0063*/
        LOG_I("Time information out of range");
        current_time_sec = 0;
    } else {
        LOG_I("Time information valid");
    }
    mDiagnosticsAcquisitionTime = static_cast<uint64_t>(current_time_sec);
    LOG_I("Save Diagnostics Acquisition Time: %lld", current_time_sec);
    /* Get location*/
    const android::sp<CommonDefine::RDGLocationData> loc{LocationManagerAdapter::getInstance()->getLocationData()};
    if (mTriggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
        LOG_I("Trigger name: WARNING_TRIGGER");
    } else if (mTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
        LOG_I("Trigger name: IGON_TRIGGER");
        mLocationData = loc;
    } else if (mTriggerType == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER) {
        LOG_I("Trigger name: OCCURRENCE_NOTIFICATION_TRIGGER");
    } else if ((mTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
            || (mTriggerType == DiagTrigger::DiagTriggerType::IGOFF_TRIGGER)
            || (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)) {
        LOG_I("Trigger name: OTHER_TRIGGER");
        mLocationData = loc;
    } else {
        LOG_I("Trigger name: UNDEFINED TRIGGER");
    }
    LOG_I("Check location information when start acquire, 0x%08x 0x%08x", loc->getLatitude(), loc->getLongtitude());
    // Get OBC resource
    OBCResourceEventCode resEventInfo{OBCResourceEventCode::OBC_GET_RESOURCE_OK}; 
    resEventInfo = OnboardclientAdapter::getInstance()->GetObcResource();
    if ( resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK )
    {
        LOG_E("GetObcResource OBC_GET_RESOURCE_WAIT -> wait and check after");
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(MainHandler::CMD_SEND_DTC), static_cast<uint64_t>(1000U));

    } else {
        LOG_I("Get obc resource success");
        OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
        mIsDTCRunning = RemoteDTC::DTC_RUNNING;
    // Using EcuInfomation API to get ECU Candidate informations and remove bellow mockup code in production
    std::list<CommonDefine::EcuInformation> ecuInformationList{};
    RemoteEcuInformation::getInstance()->getEcuInformationList(ecuInformationList);
    LOG_I("Check ecuInformationList size = %d", ecuInformationList.size());

    std::list<CommonDefine::EcuInformation>::iterator it {ecuInformationList.begin()};
    mDTCTransList.clear();

    std::queue<uint64_t> empty_queue{};
    std::swap(mTransmissioIdList, empty_queue);
    LOG_I("Start generate DTC request message");
    while(it != ecuInformationList.end()) {
        LOG_I("Check ecuInformationList target_address  = 0x%02lx", it->getTargetAddress());
        if(it->getecuActiveFlag() == true) {
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
                LOG_E("ECU is Unknown phase. Don't make request message.");
                ++it;
                // continue;
            }
            (void)sid;
            (void)sfid;
            (void)dtcStatusMask;
        } else {
            LOG_I("ECU: 0x%02llx is not active", it->getTargetAddress());
            ++it;
            // continue;/*9564076 - MISRA C++-2008 Rule 6-6-3*/
        }
    }
    LOG_I("Start DTC request sequence");
    if(mTransmissioIdList.size() > 0U) {
        LOG_I("mDTCTransList size = %d", mTransmissioIdList.size());
        mCurrentTransmissionId = mTransmissioIdList.front();
        const TransmissionInter itTrans {mDTCTransList.find(mCurrentTransmissionId)};
        if (itTrans != mDTCTransList.end())
        {
            itTrans->second->connect();
        }
    } else {
        LOG_I("mTransmissioIdList is empty");
        // Release OBC resource
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        mIsDTCRunning = RemoteDTC::DTC_STOP;
        finishDTC();
    }
    }
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
        mDiagResp_CRC.clear();
        mDiagResp_2.clear();
    }
}

void RemoteDTC::abortDtcProcessing(const vccomif::rdg::v1::interfaces::ResponseCode resCode) {
    if(isDtcAcquireAborted == false) {
        LOG_I("Start abort DTC processing");
        /*Do abort process*/
        LOG_D("Check isDtcAcquireAborted: %d", isDtcAcquireAborted);
        isDtcAcquireAborted = true;
        mIsDTCRunning = RemoteDTC::DTC_STOP;
        /*Disconnect current ECU*/
        LOG_I("Disconnect current TransmitionID: 0x%llx", mCurrentTransmissionId);
        const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
        if ((it != mDTCTransList.end()) && (it->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS)) {
            /* Save to Response List */
            it->second->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
            it->second->stopTimeoutTimer();
            it->second->disconnect();
            /* TBD: Delete matched DTCUdsTransmission in mDTCTransList*/
        } else {
            LOG_I("NOT match transmissionId or ECU was disconnected");
        }
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
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
                LOG_D("");
            }

            LOG_D("Do not generate error upload data due to trigger name is IG ON or WARNING_TRIGGER");
        }
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
            mTriggerId = 0;
        } else {
            LOG_I("Can not find triggerId: %d in mSaveReq", mTriggerId);
        }
        LOG_I("Check mSaveReq size after: %d", mSaveReq.size());
        finalize();
    } else {
        LOG_D("DTC was aborted");
    }
    isDtcAcquireAborted = false;
    PriorityControl::getInstance()->notifyTriggerProcessDone(mTriggerId, mTriggerType);
}

void RemoteDTC::discardTrigger(const android::sp<DiagTrigger> pTrigger) {
    LOG_I("Start discard DTC triggerID: %d", pTrigger->getTriggerId());
    vccomif::rdg::v1::interfaces::UploadErrorDataRequest mUploadErrorData{};
    mUploadErrorData.set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED);
    LOG_I("RC_REQUEST_ERROR_INTERRUPT_PROHIBITED");
    makeErrorUploadData(mUploadErrorData, pTrigger->getType(), pTrigger->getCollectionID());
}

void RemoteDTC::suspendDtcTrigger(const int32_t triggerId) {
    LOG_I("Start suspend DTC processing");
    isDtcAcquireAborted = true;
    // mIsDTCRunning = RemoteDTC::DTC_STOP;
    // int32_t tmp_triggerId{triggerId};
    /*Disconnect current ECU*/
    // LOG_I("Disconnect current TransmitionID: 0x%llx", mCurrentTransmissionId);
    const TransmissionInter it_ptr {mDTCTransList.find(mCurrentTransmissionId)};
    if ((it_ptr != mDTCTransList.end()) && (it_ptr->second->getState() == DTCUdsTransmission::State::DTC_TRANS_SEND_UDS)) {
        /* Save to Response List */
        // it_ptr->second->setState(DTCUdsTransmission::State::DTC_TRANS_DISCONNECT);
        // it_ptr->second->stopTimeoutTimer();
        // it_ptr->second->disconnect();
        /* TBD: Delete matched DTCUdsTransmission in mDTCTransList*/
        LOG_D("Wait response to suspend process");
    } else {
        LOG_I("NOT match transmissionId or ECU was disconnected");
    }
    // OnboardclientAdapter::getInstance()->ReleaseObcResource();
    // finalize();
    // if(tmp_triggerId < 0) {
    //     LOG_D("triggerId is out of range");
    //     tmp_triggerId = 0;
    // } else {
    //     /*do nothing*/
    // }
    // const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(static_cast<uint32_t>(tmp_triggerId))};
    // // // /* Make error upload data */
    // // // vccomif::rdg::v1::interfaces::UploadErrorDataRequest mUploadErrorData{};
    // // // if(dueToIgOff) {
    // // //     mUploadErrorData.set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
    // // //     LOG_I("RC_VEHICLE_ERROR_POWER_CONDITION");
    // // // } else {
    // // //     mUploadErrorData.set_response_code(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_UNPROVIDED_VEHICLE);
    // // //     LOG_I("RC_REQUEST_ERROR_UNPROVIDED_VEHICLE");
    // // // }
    // if(it != mSaveReq.end()) {
    //     // PriorityControl::getInstance()->notifyTriggerProcessDone(tmp_triggerId, it->second->getType());
    //     // makeErrorUploadData(mUploadErrorData, it->second->getType(), it->second->getCollectionID());
    // } else {
    //     LOG_I("Can not find triggerID in saved list");
    // }
    // void(tmp_triggerId);
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
        /*TBD: check PPI flag*/
    if(mTriggerId < 0) {
        LOG_I("mTriggerId is out of range");
        mTriggerId = 0;
    } else {
        /*Do nothing*/
    }
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(static_cast<uint32_t>(mTriggerId))};
    if(it != mSaveReq.end()) {
        mApp.triggerDTCToSSR(it->second->getType(),it->second->getTriggerTime(), mLocationData, mColId, mPriority, v_targetEcuList);
        v_targetEcuList.clear();
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
    if(it != mSaveReq.end()) {
        LOG_I("notify DTC Trigger state: %d TriggerID: %d TriggerTime: %lld", pState, pTriggerId_tmp, it->second->getTriggerTime());
        isDTCTrigger = true;
        handleTrigger(pState, pTriggerId_tmp, dueToIgOff);
    } else {
        LOG_I("Can not find triggerID in saved list");
        isDTCTrigger = false;
        PriorityControl::getInstance()->notifyTriggerNoFound(pTriggerId);
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
    if(it != mSaveReq.end()) {
        PriorityControl::getInstance()->notifyTriggerProcessDone(triggerId, it->second->getType());
        (void)mSaveReq.erase(it);
    } else {
        LOG_I("Can not find triggerId: %d in mSaveReq", triggerId);
    }
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
    if(it != mSaveReq.end()) {
        const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
        const uint64_t colID{it->second->getCollectionID()};
        const uint32_t prio_data{it->second->getPriority()};
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
            PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_PROCESSING:
        {
            LOG_I("TRIGGER_PROCESSING");
            /* DTC can run */
            const uint8_t rdgFlag{DiagManagerAdapter::getInstance()->getRDGFlag()};
            const IG_STATUS igStatus{PowerManagerAdapter::getInstance()->getIgnitionStatus()};
            const uint8_t dtcFlag{DiagManagerAdapter::getInstance()->getDTCFlag()};
            const bool allUploadConsent{DiagManagerAdapter::getInstance()->getAllUploadConsent()};
            const uint8_t underRepairStatus{DiagManagerAdapter::getInstance()->getUnderRepairStatus()};
            mTriggerId = pTriggerId;
            mTriggerType = pTriggerType;
            mColId = colID;
            mPriority = prio_data;
            isDtcAcquireAborted = false;
            const android::sp<CommonDefine::RDGLocationData> loc{new CommonDefine::RDGLocationData()};
            loc->setLatitude(it->second->getLatitude());
            loc->setLongitude(it->second->getLongtitude());
            mLocationData = loc;
            /*Check Pricondition for AllDiag and IG ON trigger only*/
            // bool precondition_checking{true};
            // if((pTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) || (pTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)) {
            //     if((rdgFlag == 1U) && (igStatus == IG_STATUS_ON) && (dtcFlag == 1U) && (allUploadConsent == true)) {
            //         precondition_checking = true;
            //     } else {
            //         precondition_checking = false;
            //     }
            // } else {

            // }
            if((pTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)) {
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
                    abortDtcProcessing(resCode);
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

                // if(underRepairStatus != 0U) {
                //     LOG_I("RC_OBE_ERROR_UNDER_REPAIR");
                //     if(resCode > vccomif::rdg::v1::interfaces::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR) {
                //         resCode = vccomif::rdg::v1::interfaces::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR;
                //     }
                // }
                if(allUploadConsent != true) {
                    LOG_I("All upload consent is not 10b");
                }
                abortDtcProcessing(resCode);
                PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            }
            } else if (pTriggerType == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
                LOG_I("DTC running by IG ON trigger");
                LOG_I("AllDiag executes Conditions is PASS");
                
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
            suspendDtcTrigger(mTriggerId);
            // PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
        {
            LOG_I("TRIGGER_DISCARDED");
            if((mIsDTCRunning == 1U) && (mTriggerId == pTriggerId)) {
                if(dueToIgOff == true) {
                    abortDtcProcessing(vccomif::rdg::v1::interfaces::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
                } else {
                    abortDtcProcessing(vccomif::rdg::v1::interfaces::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED);
                }
            } else {
                /*discard diag trigger*/
                discardTrigger(it->second);
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
                PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
                (void)mSaveReq.erase(it);
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DONE:
        {
            LOG_I("TRIGGER_DONE");
            PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_STATE_MAX:
        {
            LOG_I("TRIGGER_STATE_MAX");
            PriorityControl::getInstance()->notifyTriggerProcessDone(pTriggerId, pTriggerType);
            break;
        }
        }
        (void)prio_data;
        (void)pTriggerType;
        (void)colID;
        (void)pTriggerType;
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
                TimeManager &mTimeManagerService {TimeManager::getInstance()};
                int64_t current_time_millis {mTimeManagerService.getCurrentMilliSec()};
                if(current_time_millis < static_cast<int64_t>(0x00)) {
                    LOG_D("Get time fail. Set default time value");
                    current_time_millis = 0;
                    LOG_D("Check time: %lld", current_time_millis);
                }
                int64_t current_time {ParamsDef::getCurrentAcquisiteTime()};
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
                colId = static_cast<uint64_t>(diagCommon->collection_condition_id());
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
            LOG_I("Trigger Request have Type: %d Func: %d Prio: %d ID: %d ColID: %llu", 
                pTrigger->getType(), pTrigger->getFunc(), pTrigger->getPriority(), pTrigger->getTriggerId(), pTrigger->getCollectionID());

            mDTC.trigger_DTC(pTrigger);
            break;
        }
        case CMD_TRIGGER_FROM_CENTER:
        {
            LOG_I("CMD_TRIGGER_FROM_CENTER");
            /* Get current time */
            TimeManager &mTimeManagerService {TimeManager::getInstance()};
            int64_t current_time_millis {mTimeManagerService.getCurrentMilliSec()};
            if(current_time_millis < static_cast<int64_t>(0x00)) {
                LOG_D("Get time fail. Set default time value");
                current_time_millis = 0;
                LOG_D("Check time: %lld", current_time_millis);
            }
            int64_t current_time{0};
            current_time = ParamsDef::getCurrentAcquisiteTime();
            if(current_time < 0) {
                LOG_D("Time data is negative");
            }
            /* Get priority from Center Request */
            int32_t tmp{handlemsg->arg1};
            if(tmp < 0){
                LOG_E("out of range uint32_t");
                tmp = 0;
            }
            const uint32_t prio_tmp{static_cast<uint32_t>(tmp)};
            /* Get collection condition ID*/
            const android::sp<::Buffer> buf {new ::Buffer(handlemsg->buffer)};
            // uint64_t* const colId_ptr{reinterpret_cast<uint64_t*>(buf->data())};
            uint64_t colId {0ULL};
            uint8_t* const tmp_ptr{buf->data()};
            if(tmp_ptr != nullptr) {
                (void)std::memcpy(&colId, tmp_ptr, sizeof(uint64_t));
            } else {
                LOG_D("tmp_ptr is nullptr");
            }
            const android::sp<CommonDefine::RDGLocationData> loc{LocationManagerAdapter::getInstance()->getLocationData()};
            LOG_I("Check Col ID: %llu", colId);
            mDTC.trigger_DTC(DiagTrigger::DiagTriggerType::CENTER_TRIGGER, prio_tmp, colId, current_time, loc);
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
        default:
        {
            break;
        }
    }
}
void RemoteDTC::finishCurrentTransmission(void) {
    LOG_I("finish Current transmission ID: 0x%02llx, mTransmissioIdList size = %d", mCurrentTransmissionId, mTransmissioIdList.size());
    if ( mTransmissioIdList.size() > 1U ) {
        mTransmissioIdList.pop();
        /*If DTC was not aborted && is running=> connect next ECU*/
        LOG_D("Check isDtcAcquireAborted: %d", isDtcAcquireAborted);
        LOG_D("Check mIsDTCRunning: %d", mIsDTCRunning);
        if((isDtcAcquireAborted == false) && (mIsDTCRunning == RemoteDTC::DTC_RUNNING)) {
            mCurrentTransmissionId = mTransmissioIdList.front();
            const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
            if (it != mDTCTransList.end())
            {
                it->second->connect();
            }
        } else {
            LOG_I("DTC is aborted => dont process next ECU");
            // OnboardclientAdapter::getInstance()->ReleaseObcResource();
        }
    } else {
        if(mTransmissioIdList.empty() != true) {
            mTransmissioIdList.pop();
        }
        // Release OBC resource
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        mIsDTCRunning = RemoteDTC::DTC_STOP;
        LOG_I("mTransmissioIdList size = %d ->  finished the last ECU -> upload DTC package", mTransmissioIdList.size());
        finishDTC();
        mDTCTransList.clear();
    }
}

void RemoteDTC::onTransmissionTimeout(void) {
    LOG_I("Event Transmission Timeout, currentTrans = 0x%02llx", mCurrentTransmissionId);

    if(mIsDTCRunning == RemoteDTC::DTC_RUNNING) {
        mDiagResp_2.push_back({mCurrentTransmissionId, nullptr});
        const TransmissionInter it {mDTCTransList.find(mCurrentTransmissionId)};
        if (it != mDTCTransList.end())
        {
            it->second->disconnect();
        }
        finishCurrentTransmission();
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

void RemoteDTC::triggerFromWarning(const DiagTrigger::DiagTriggerType triggerType
    , const int64_t timeData
    , const android::sp<CommonDefine::RDGLocationData> location
    , const uint64_t collectionId
    , const uint32_t priority) {
    LOG_I("triggerFromWarning");
    if(triggerType == DiagTrigger::DiagTriggerType::WARNING_TRIGGER) {
        /* Create NewDiag Trigger*/
        const uint32_t nextTriggerId{TriggerIDGenerator::getInstance().getNextId()};
        /* DiagTrigger(const DiagTrigger::DiagTriggerType type, const uint32_t priority, const DiagTrigger::DiagTriggerFunc func, const int32_t triggerId)*/
        const android::sp<DiagTrigger> pTrigger{new DiagTrigger(triggerType, priority, DiagTrigger::DiagTriggerFunc::DTC, nextTriggerId)};
        /* set trigger time*/
        pTrigger->setTriggerTime(timeData);
        LOG_I("Retain time data: %lld sec", timeData);
        /* Set collection id*/
        pTrigger->setCollectionId(collectionId);
        LOG_I("Check DTC Collection ID: %llu", collectionId);

        pTrigger->setLatitude(location->getLatitude());
        pTrigger->setLongitude(location->getLongtitude());

        LOG_I("Retain Location data 0x%x 0x%x", static_cast<int32_t>(location->getLatitude()), static_cast<int32_t>(location->getLongtitude()));
        /* Obtain message:CMD_REQUEST_TO_PRIORITY_CONTROL + TriggerID + Diag Func + priority*/
        (void)mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_WARNING, pTrigger)->sendToTarget();

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
    mDTCDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    // electronic_pf
    // geodesy_information
    // time_zone_offset
    LOG_I("Check timezoneOffSet: %d", TimeManager::getInstance().getOffset());
    const int32_t tz{TimeManager::getInstance().getOffset()};
    const int32_t hour{tz/60};
    const int32_t mins{tz%60};
    mDTCDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(hour);
    mDTCDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(mins);
    mDTCDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    mDTCDataReq->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
    /*interface_type*/
    mDTCDataReq->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_DTC_DATA);
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    /*message_id*/
    mDTCDataReq->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_DTC_DATA, counterValue));
    /*counter_value*/
    mDTCDataReq->set_counter_value(counterValue);
    (void)counterValue;
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

    location->set_latitude(mLocationData->getLatitude()); 
    location->set_longitude(mLocationData->getLongtitude());

    /*obd2_installed_flag*/
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
        if (it_ptr != mDTCTransList.end())
        {
            LOG_D("transmissionId: %llu", transmissionId);
            if (it->second == nullptr) {
                LOG_D("SC_UNRESPONSIVE");
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_UNRESPONSIVE);
                const android::sp<::Buffer> reqData {it_ptr->second->udsReq.ToUdsData()};
                if ((reqData != nullptr) && (reqData->data() != nullptr))
                {
                    tmpDiagMessage.set_user_data(reqData->data(), reqData->size());
                } else {
                    LOG_D("reqData or reqData->data() is null");
                }
                ecuAddressInfo->set_communication_protocol(it_ptr->second->getEcuInformation().getCommProtocol());
                ecuAddressInfo->set_communication_type(it_ptr->second->getEcuInformation().getCommType());
                ecuAddressInfo->set_target_address(it_ptr->second->getEcuInformation().getTargetAddress());
            }
            else if (it->second->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DTC_INFORMATION)) 
            {
                LOG_D("UDS_PR_READ_DTC_INFORMATION");
                RdgProtoInterface::EcuAddressInformation* const ecuAddressInfo {tmpDiagMessage.mutable_ecu_address_information()};
                tmpDiagMessage.set_status_code(RdgProtoInterface::StatusCode::SC_SUCCESSFUL);
                ecuAddressInfo->set_communication_protocol(it_ptr->second->getEcuInformation().getCommProtocol());
                ecuAddressInfo->set_communication_type(it_ptr->second->getEcuInformation().getCommType());
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
                ecuAddressInfo->set_communication_protocol(it_ptr->second->getEcuInformation().getCommProtocol());
                ecuAddressInfo->set_communication_type(it_ptr->second->getEcuInformation().getCommType());
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
                ecuAddressInfo->set_communication_protocol(it_ptr->second->getEcuInformation().getCommProtocol());
                ecuAddressInfo->set_communication_type(it_ptr->second->getEcuInformation().getCommType());
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
            const uint32_t tmp_size{tmpDiagMessage.ByteSizeLong()};
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
                    const uint32_t sizeOfEcuAddressInformation {tmpDiagMessage.ecu_address_information().ByteSizeLong()};
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
                    const uint32_t tmp{tmpDiagMessage.ByteSizeLong()};
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
    std::string file_dir {std::to_string(uploadId)};
    (void)file_dir.append("_UploadDtcDataRequest.dat");
    (void)DataModel<UploadDtcDataRequest>::save(file_dir, *mDTCDataReq);
    const uint64_t fileSize{static_cast<uint64_t>(mDTCDataReq->ByteSizeLong())};
    const android::sp<UploadTask> task {new UploadTask(uploadId)};
    task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG030);
    task->setUploadPatch(file_dir);
    // task->setUploadId(uploadId);
    task->setFileSize(fileSize);
    /*Set priority*/
    task->setUploadPrio(mPriority);
    // test_saveUploadData = task;
    (void)sizeOfFileCounter;
    UploadManager::getInstance()->requestUploadTask(task);  
}


void RemoteDTC::testingMaxFileSize(const uint32_t fileSize) noexcept
{
    mMaxUploadFileSize = fileSize;
}

void RemoteDTC::makeErrorUploadData(void) {
    mDTCUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    LOG_I("Check timezoneOffSet: %d", TimeManager::getInstance().getOffset());
    const int32_t tz{TimeManager::getInstance().getOffset()};
    const int32_t hour{tz/60};
    const int32_t mins{tz%60};
    mDTCUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(hour);
    mDTCUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(mins);
    mDTCUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    mDTCUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
    mDTCUploadErrorData->mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    mDTCUploadErrorData->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, counterValue));

    // GeodesyInformation geodesy_information  ===> GeodesyInformation::GI_UNKNOWN  (Need to confirm)
    // appCommonHeader->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_UNKNOWN);
    const sp<CommonDefine::RDGLocationData> mLocation{LocationManagerAdapter::getInstance()->getLocationData()};
    mDTCUploadErrorData->set_collection_condition_id(mColId); /*TBD*/
    mDTCUploadErrorData->set_counter_value(counterValue);
    (void)counterValue;
    // data_creation_date
    int64_t current_time {0};
    current_time = ParamsDef::getCurrentAcquisiteTime();
    if (current_time >= 0)
    {
        mDTCUploadErrorData->set_data_creation_date(static_cast<uint64_t>(current_time));
    }
    else {
        //Do nothing
    }
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
    std::string file_dir {std::to_string(uploadId)};
    (void)file_dir.append("_UploadDtcDataError.dat");
    if (DataModel<UploadErrorDataRequest>::save(file_dir, *mDTCUploadErrorData) != E_OK) {
        LOG_E("save fail !");
    }
    const android::sp<UploadTask> task {new UploadTask(uploadId)};
    task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
    task->setUploadPatch(file_dir);
    // task->setUploadId(uploadId);
    /*Set priority*/
    task->setUploadPrio(mPriority);
    const uint64_t fileSize{static_cast<uint64_t>(mDTCUploadErrorData->ByteSizeLong())};
    task->setFileSize(fileSize);
    // test_saveUploadData = task;
    UploadManager::getInstance()->requestUploadTask(task);  
}

void RemoteDTC::makeErrorUploadData(vccomif::rdg::v1::interfaces::UploadErrorDataRequest errorData, const DiagTrigger::DiagTriggerType type, const uint64_t colID) {
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
    LOG_I("Check timezoneOffSet: %d", TimeManager::getInstance().getOffset());
    const int32_t tz{TimeManager::getInstance().getOffset()};
    const int32_t hour{tz/60};
    const int32_t mins{tz%60};
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(hour);
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(mins);
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
    errorData.mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
    errorData.mutable_rdg_common_request_header()->set_interface_type(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
    const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
    errorData.mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, counterValue));

    // GeodesyInformation geodesy_information  ===> GeodesyInformation::GI_UNKNOWN  (Need to confirm)
    // appCommonHeader->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_UNKNOWN);
    const sp<CommonDefine::RDGLocationData> mLocation{LocationManagerAdapter::getInstance()->getLocationData()};
    errorData.set_collection_condition_id(mColId); /*TBD*/
    errorData.set_counter_value(counterValue);
    (void)counterValue;
    // data_creation_date
    int64_t current_time {0};
    current_time = ParamsDef::getCurrentAcquisiteTime();
    if (current_time >= 0)
    {
        errorData.set_data_creation_date(static_cast<uint64_t>(current_time));
    }
    else {
        //Do nothing
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
    } else if (type == DiagTrigger::DiagTriggerType::IGON_TRIGGER) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_IG_ON_TRIGGER;
    } else if (type == DiagTrigger::DiagTriggerType::OCCURRENCE_NOTIFICATION_TRIGGER) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_OCCURRENCE_NOTIFICATION_TRIGGER;
    } else if ((type == DiagTrigger::DiagTriggerType::CENTER_TRIGGER)
            || (type == DiagTrigger::DiagTriggerType::IGOFF_TRIGGER)
            || (type == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)) {
        errorTriggerType = RdgProtoInterface::TriggerType::TT_OTHER_TRIGGER;
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
    std::string file_dir {std::to_string(uploadId)};
    (void)file_dir.append("_UploadDtcDataError.dat");
    if (DataModel<UploadErrorDataRequest>::save(file_dir, errorData) != E_OK) {
        LOG_E("save fail !");
    }
    const android::sp<UploadTask> task {new UploadTask(uploadId)};
    task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
    task->setUploadPatch(file_dir);
    // task->setUploadId(uploadId);
    const uint64_t fileSize{static_cast<uint64_t>(errorData.ByteSizeLong())};
    task->setFileSize(fileSize);
    /*Set priority*/
    task->setUploadPrio(mPriority);
    // test_saveUploadData = task;
    UploadManager::getInstance()->requestUploadTask(task);  
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
                abortDtcProcessing(vccomif::rdg::v1::interfaces::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR);
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
    canInfo.setData(this->ecuInformation.getTargetAddress(), ntaArray);
    obcTransportInfo->setData(static_cast<uint8_t>(RemoteEcuInformation::getInstance()->convertObcProtocolType(this->ecuInformation.getCommProtocol(), this->ecuInformation.getCommType(), this->ecuInformation.getTargetAddress()))
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
        this->send();
    }
    ntaArray.clear();
    (void)res;
}

void RemoteDTC::DTCUdsTransmission::stopTimeoutTimer() {
    LOG_I("Stop timeout timer for transmittionID: 0x%llx", transmissionId);
    mTimeOut.stop();
}

void RemoteDTC::DTCUdsTransmission::disconnect()
{
    LOG_D("disconnect: 0x%02llx", this->transmissionId);
    (void)OnboardclientAdapter::getInstance()->disconnectECU(this->connectId);
    // OnboardclientAdapter::getInstance()->ReleaseObcResource();
    mState = DTCUdsTransmission::State::DTC_TRANS_DONE;
}

void RemoteDTC::DTCUdsTransmission::send()
{
    const android::sp<::Buffer> udsData {this->udsReq.ToUdsData()};

    const uint8_t res{OnboardclientAdapter::getInstance()->sendUdsData(this->connectId, udsData)};
    if (res != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK)) 
    {
        LOG_E("SendUdsData error = %d -> Disconnect", res);
        mDTC.mDiagResp_2.push_back({this->transmissionId, nullptr});
        this->disconnect();
        mDTC.finishCurrentTransmission();
    }
    else {
        LOG_I("SendUdsData success");
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
