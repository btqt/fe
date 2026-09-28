#include "RemoteEcuInformation.h"

namespace rdgapp {

RemoteEcuInformation *RemoteEcuInformation::mRemoteEcuInformation {nullptr};
RemoteEcuInformation::RemoteEcuInformation(const Remotediag &app, android::sp<sl::SLLooper> &privateLooper)
: android::RefBase(), RemoteDelegate()
, mApp(app)
, mHandler(new MainHandler(privateLooper, *this))
, mTimerHandler(new TimerHandler(*this))
, mState(EcuState::ECU_IDLE)
, mIsEcuRunning(false)
, mIsSuspending(false)
, mCurrentTransmissionId(0U)
, mPriority(0U)
, mColId(0U)
, mLastUpdateTime(0U)
, mIsNeedUploadErrorData(false)
, mTriggerId(0U)
, mTriggerType(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)
, m_srvc_disregard_flag(false)
{
    mRemoteEcuInformation = this;
    (void)mHandler->obtainMessage(MainHandler::CMD_INIT_ECU_INFORMATION)->sendToTarget();
}

RemoteEcuInformation::~RemoteEcuInformation() = default;

RemoteEcuInformation *RemoteEcuInformation::getInstance()
{
    if (mRemoteEcuInformation == nullptr)
    {
        LOG_E("mRemoteEcuInformation is nullptr");
    }
    return mRemoteEcuInformation;
}

void RemoteEcuInformation::printData(const std::string data) const
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
void RemoteEcuInformation::getEcuInformationList(std::list<CommonDefine::EcuInformation> &l_EcuInformation)
{
    l_EcuInformation = m_EcuInformationList;
}

void RemoteEcuInformation::MainHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
{
    const int32_t what{handlemsg->what};
    LOG_I({"handler is processing with what: %d"}, what);
    switch (what)
    {
        case CMD_INIT_ECU_INFORMATION:
        {
            LOG_I("CMD_INIT_ECU_INFORMATION");
            break;
        }
        case CMD_TRIGGER_FROM_CENTER:
        {
            LOG_I("CMD_TRIGGER_FROM_CENTER");
            if(handlemsg->arg1>=0)
            {
                const uint32_t prio{handlemsg->arg1>0 ? static_cast<uint32_t>(handlemsg->arg1) : 0U};
                DiagTrigger::DiagTriggerType trigType{DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN};
                if ((handlemsg->arg2 > static_cast<int32_t>(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)) && (handlemsg->arg2 < static_cast<int32_t>(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)))
                {
                    trigType = static_cast<DiagTrigger::DiagTriggerType>(handlemsg->arg2);
                }

                const android::sp<::Buffer> buf {new ::Buffer(handlemsg->buffer)};
                uint64_t colId{0U};
                if(buf->size()>0U)
                {
                    if(buf->data() != nullptr) {
                        (void)memcpy(&colId,buf->data(),sizeof(colId));
                    } else {
                        LOG_E("buf->data() is null");
                    }
                }
                else{
                    LOG_E("Buf data size = 0");
                }
                mECUInfo.triggerEcuInfo(prio, colId, trigType);
            }
            else{
                LOG_E("ERROR: handlemsg->arg1 < 0");
            }
            break;
        }
        case CMD_START_ECU_INFORMATION:
        {
            LOG_I("CMD_START_ECU_INFORMATION");
            (void)mECUInfo.startDiagTask();
            break;
        }
        case CMD_REQUEST_TO_PRIORITY_CONTROL:
        {
            LOG_I("CMD_REQUEST_TO_PRIORITY_CONTROL");
            sp<DiagTrigger> pTrigger {nullptr};
            handlemsg->getObject(pTrigger);
            LOG_D("Trigger Request have Type: %d Func: %d Prio: %d ID: %d ColID: %lld", 
                pTrigger->getType(), pTrigger->getFunc(), pTrigger->getPriority(), pTrigger->getTriggerId(), pTrigger->getCollectionID());
            PriorityControl::getInstance()->requestTriggerProcess(pTrigger);
            break;     
        }
        case CMD_MAKE_UPLOAD_REQUEST:
        {
            LOG_I("CMD_MAKE_UPLOAD_REQUEST");
            mECUInfo.makeUploadRequest();
            break;
        }
        case CMD_START_DID_ACQUISITION:
        {
            LOG_I("CMD_START_DID_ACQUISITION");
            mECUInfo.startDidAcquisition();
            break;
        }
        case CMD_CHANGE_IG_STATUS:
        {
            LOG_I("CMD_CHANGE_IG_STATUS");
            mECUInfo.changeIGStatus((handlemsg->arg1 != 0) ? true : false);
            break;
        }
        case CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE:
        {
            mECUInfo.handleUnderRepairStatusChange(handlemsg->arg1, handlemsg->arg2);
            break;
        }
        case CMD_RECEIVE_UDS_RESPONSE:
        {
            android::sp<OBCResponseEventInfo> udsRes{nullptr};
            handlemsg->getObject(udsRes);
            mECUInfo.handleReceiveUDS(udsRes);
            break;
        }
        case CMD_TRANSMISSION_TIMEOUT:
        {
            mECUInfo.handleTransmissionTimeout();
            break;
        }
        case CMD_ECUINFO_TRANSMISSION_FINISH:
        {
            LOG_V("CMD_ECUINFO_TRANSMISSION_FINISH");
            mECUInfo.finishCurrentTransmission();
            break;
        }
        default:
            break;
    }
}

void RemoteEcuInformation::TimerHandler::handlerFunction(const int32_t timerId)
{
    LOG_I("Timer id %d", timerId);
    switch (timerId)
    {
        case ID_TRANSMISSION_TIMEOUT:
        {
            mECUInfo.onTransmissionTimeout();
            break;
        }
        default:
        {
            break;
        }
    }
}

bool RemoteEcuInformation::startDiagTask()
{
    //Check pre-condition //RDG30-R-0190
    if (checkPrecondition() == false)
    {
        processErrorHandling(); //RDG30-R-0684
    } else
    {
        //Create empty ECU Information list //RDG30-R-0939
        clearEcuInformationList();

        //Load existed list //RDG30-R-0940 
        loadEcuInfoListFromFile();

        //Send UDS to all address in ECU_Candidate_List //RDG30-R-0573
        //start from index 0
        startEcuExistenceCheck();
    }
    return true;
}

void RemoteEcuInformation::startEcuExistenceCheck()
{
    // Get OBC resource
    const OBCResourceEventCode resEventInfo {OnboardclientAdapter::getInstance()->GetObcResource()};
    if ( resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK )
    {
        LOG_E("GetObcResource OBC_GET_RESOURCE_WAIT -> wait and check after");
        const android::sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_START_ECU_INFORMATION)};
        (void)mHandler->sendMessageDelayed(msg, 5000U);

    } 
    else 
    {
        LOG_D("Get obc resource success");
        // Lock OBC resource
        OnboardclientAdapter::getInstance()->TakeObcResource();
        this->mIsEcuRunning = true;
        mState = EcuState::ECU_EXISTENCE_CHECK;
        mEcuTransList.clear();
        mTransmissioIdList = {};
        constexpr uint8_t sid {static_cast<uint8_t>(UDS_SID::SID_3E_TESTER_PRESENT)};
        constexpr uint8_t sfid {0x00U};
        //add Phase5 ECU transmission list
        for (uint32_t i{0U}; i<ECU_CANDIDATE_NUMBER; i++)
        {
            const uint32_t canId {ECU_Candidate_List[i]};
            constexpr uint8_t nTa {static_cast<uint8_t>(0U)};
            const uint8_t type {static_cast<uint8_t>(convertObcProtocolType(CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN
                                                                , CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS, canId))};

            const android::sp<::Buffer> UdsData{new ::Buffer()};
            uint8_t temp;
            (void)memcpy(&temp, &sid, sizeof(uint8_t));
            UdsData->setTo(&temp, 1);
            (void)memcpy(&temp, &sfid, sizeof(uint8_t));
            UdsData->append(&temp, 1);

            const android::sp<EcuUdsTransmission> transmission {new EcuUdsTransmission(*this, canId, nTa, type, UdsData)};
            transmission->setRequestType(false);
            mEcuTransList[transmission->getTransmissionId()] = transmission;
            mTransmissioIdList.push(transmission->getTransmissionId());
        }

        // add Phase6 ECU CANFD transmission list
        constexpr uint32_t canId {0x18DBEFE1U};
        constexpr uint8_t nTa {static_cast<uint8_t>(0U)};
        uint8_t type {static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)};

        const android::sp<::Buffer> udsReqFD{new ::Buffer()};
        uint8_t temp{0U};
        (void)memcpy(&temp, &sid, sizeof(uint8_t));
        udsReqFD->setTo(&temp, 1);
        (void)memcpy(&temp, &sfid, sizeof(uint8_t));
        udsReqFD->append(&temp, 1);

        const android::sp<EcuUdsTransmission> ecuPhase6CanFD {new EcuUdsTransmission(*this, canId, nTa, type, udsReqFD)};
        ecuPhase6CanFD->setRequestType(true);
        mEcuTransList[ecuPhase6CanFD->getTransmissionId()] = ecuPhase6CanFD;
        mTransmissioIdList.push(ecuPhase6CanFD->getTransmissionId());


        // add Phase6 ECU CAN transmission list
        type = static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT);
        const android::sp<::Buffer> udsCan{new ::Buffer()};
        uint8_t canTemp{0U};
        (void)memcpy(&canTemp, &sid, sizeof(uint8_t));
        udsCan->setTo(&canTemp, 1);
        (void)memcpy(&canTemp, &sfid, sizeof(uint8_t));
        udsCan->append(&canTemp, 1);

        const android::sp<EcuUdsTransmission> ecuPhase6Can {new EcuUdsTransmission(*this, canId, nTa, type, udsCan)};
        ecuPhase6Can->setRequestType(true);
        mEcuTransList[ecuPhase6Can->getTransmissionId()] = ecuPhase6Can;
        mTransmissioIdList.push(ecuPhase6Can->getTransmissionId());

        //send first uds message
        LOG_D("mEcuTransList size = %d", mTransmissioIdList.size());
        if ( mTransmissioIdList.size() > 0U )
        {
            mCurrentTransmissionId = mTransmissioIdList.front();
            const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator it {mEcuTransList.find(mCurrentTransmissionId)};
            if (it != mEcuTransList.end())
            {
                it->second->connect();
            }
        } else {
            // Release OBC resource
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            if(mTriggerId<=static_cast<uint32_t>(INT32_MAX))
            {
                PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mTriggerType);
            }
        }
    }
}

void RemoteEcuInformation::insertEcuInformation(const CommonDefine::EcuInformation ecu)
{
    m_EcuInformationList.push_back(ecu);
}

void RemoteEcuInformation::clearEcuInformationList()
{
    LOG_I("Clear ECU information list");
    m_EcuInformationList.clear();
    mEcuInformationMap.clear();
}

void RemoteEcuInformation::testLoadEcuInformationFromFile() {

    /* Get file size*/
    int64_t size {0};
    ifstream file{ParamsDef::ECU_INFORMATION_LIST_PATH_TEST, ios::binary | ios::ate};
    size = file.tellg();
    file.close();
    LOG_I("CHECK ECU file test size: %d", size);
    if(size > 1) {
        // Create an instance of EcuInformation struct
        std::ifstream ifs {};
        ifs.open(&ParamsDef::ECU_INFORMATION_LIST_PATH_TEST[0], std::ifstream::in);
        if (ifs.is_open())
        {
            CommonDefine::EcuInformation ecuInfo {};
            m_EcuInformationList.clear();
            mEcuInformationMap.clear();
            std::string line {};
            while (std::getline(ifs, line))
            {
                std::string tmp_targetAddress {};
                std::string tmp_canId {};
                bool tmp_ecuActiveFlag;
                int32_t tmp_commProtocol;
                int32_t tmp_commType;
                int32_t tmp_diagPhase;
                uint32_t tmp_targetAddress_int;
                uint32_t tmp_canId_int;
                std::istringstream iss{line};
                iss >> tmp_targetAddress >> tmp_canId >> tmp_ecuActiveFlag >> tmp_commProtocol >> tmp_commType >> tmp_diagPhase;
                tmp_targetAddress_int = std::stoul(tmp_targetAddress, nullptr, 16);
                tmp_canId_int = std::stoul(tmp_canId, nullptr, 16);
                
                ecuInfo.setCanId(tmp_canId_int);
                ecuInfo.setTargetAddress(tmp_targetAddress_int);
                ecuInfo.setecuActiveFlag(tmp_ecuActiveFlag);
                vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol tempCommProtocol{vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN};
                switch(tmp_commProtocol)
                {
                    case 1:
                        tempCommProtocol = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN;
                        break;
                    case 2:
                        tempCommProtocol = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD;
                        break;
                    case 3:
                        tempCommProtocol = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_IP;
                        break;
                    default:
                        break;
                }
                ecuInfo.setCommProtocol(tempCommProtocol);
                vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType tempCommType{vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_UNKNOWN};
                switch(tmp_commType){
                    case 1:
                        tempCommType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS;
                        break;
                    case 2:
                        tempCommType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS;
                        break;
                    default:
                        break;
                }
                ecuInfo.setCommType(tempCommType);
                CommonDefine::DiagPhase tempDiagPhase{CommonDefine::DiagPhase::DP_UNKNOW};
                switch(tmp_diagPhase)
                {
                    case 4:
                       tempDiagPhase = CommonDefine::DiagPhase::DP_PHASE_4;
                       break;
                    case 5:
                        tempDiagPhase = CommonDefine::DiagPhase::DP_PHASE_5;
                        break;
                    case 6:
                        tempDiagPhase = CommonDefine::DiagPhase::DP_PHASE_6;
                        break;
                    default:
                        tempDiagPhase = CommonDefine::DiagPhase::DP_OTHER;
                        break; 
                }
                ecuInfo.setDiagPhase(tempDiagPhase);
                m_EcuInformationList.push_back(ecuInfo);
                mEcuInformationMap[ecuInfo.getTargetAddress()] = ecuInfo;
                LOG_I("targetAddress: 0x%02x", tmp_targetAddress_int);
                LOG_I("canId: 0x%02x", tmp_canId_int);
                LOG_I("ecuActiveFlag: %d", tmp_ecuActiveFlag);
                LOG_I("commProtocol: %d", tmp_commProtocol);
                LOG_I("commType: %d", tmp_commType);
                LOG_I("diagPhase: %d", tmp_diagPhase);
            }
            ifs.close();
        }
        LOG_I("Check ecu list size: %d", m_EcuInformationList.size());
    }
}

void RemoteEcuInformation::deactiveAllEcu() noexcept
{
    std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()};
    for (; it != m_EcuInformationList.end(); ++it)
    {
        it->setecuActiveFlag(false);
    }

    std::unordered_map<uint32_t, CommonDefine::EcuInformation>::iterator it2 {mEcuInformationMap.begin()};
    for (; it2 != mEcuInformationMap.end(); ++it2)
    {
        it2->second.setecuActiveFlag(false);
    }
}

void RemoteEcuInformation::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
    (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UDS_RESPONSE, responseEventInfo)->sendToTarget();
    (void)udsResponse;
}

void RemoteEcuInformation::handleReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo)
{
    const android::sp<OBCCanInfo> canInfo {responseEventInfo->resInfo()->canInfo()};
    const uint16_t connectId {responseEventInfo->resInfo()->connectId()};
    const uint8_t protocolType {responseEventInfo->resInfo()->protocolType()};
    const android::sp<::Buffer> udsData{responseEventInfo->resInfo()->udsData()};
    const android::sp<UdsMessage> udsResponse{new UdsMessage()};
    if (udsData->size() > 0U)
    {
        (void)udsResponse->Parser(udsData);
    }

    if (this->mIsEcuRunning == true)
    {
        LOG_I("onReceiveUDS canId = 0x%02x", canInfo->canId());
        LOG_D("onReceiveUDS connectId = %d", connectId);
        
        const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
        if (itEcu != mEcuTransList.end())
        {
            const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
            if ((currentTransmission->getState() == EcuUdsTransmission::EcuTransState::ECU_TRANS_SEND_UDS) && (((currentTransmission->getConnectId() == connectId) || (currentTransmission->checkfunctionRequest() == true))))
            {          
                currentTransmission->stopTimeout();
                switch (mState)
                {
                    case EcuState::ECU_EXISTENCE_CHECK:
                    {
                        handleExistenceCheckResponse(protocolType, canInfo, udsResponse);
                        break;
                    }
                    case EcuState::ECU_DID_DATA_ACQUISITION:
                    {
                        handleReadDidResponse(protocolType, canInfo, udsResponse);
                        break;
                    }
                    default:
                    {
                        LOG_I("incorrect state: %d", mState);
                        break;
                    }
                }
                if(currentTransmission->checkfunctionRequest())
                {
                    currentTransmission->resetTimer();
                }
                else
                {        
                    if (udsResponse->getSID() != static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_SESSION_CONTROL))
                    {
                        currentTransmission->disconnect();
                    }
                    else
                    {
                        finishCurrentTransmission(); //do not disconnect after change to remote session
                    }
                }
            }
        }
        else
        {
            LOG_E("cannot find current transmission");
        }
    } else 
    {
        LOG_E("mIsEcuRunning is false");
    }
    (void) connectId;
    (void) protocolType;
}

void RemoteEcuInformation::handleExistenceCheckResponse(const uint8_t protocolType, const android::sp<OBCCanInfo> canInfo, const android::sp<UdsMessage> udsResponse)
{
    CommonDefine::EcuInformation ecuInformation {};
    const uint8_t sid {udsResponse->getSID()};
    const uint8_t sfid {udsResponse->getSFID()};
    switch (protocolType)
    {
        case OBCEnum::OBCProtocolType::DOCAN: //fall-through
        case OBCEnum::OBCProtocolType::DOCAN11BITEX:
        {
            //phase5 existence check response
            ecuInformation.setCanId(canInfo->canId()); //Rx CanId
            ecuInformation.setTargetAddress(canInfo->canId() & 0xFFF7FFFFU); //Rx -> Tx
            ecuInformation.setCommProtocol(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN);
            ecuInformation.setCommType(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS);
            ecuInformation.setecuActiveFlag(true);
            
            if (sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_TESTER_PRESENT))
            {
                ecuInformation.setDiagPhase(CommonDefine::DiagPhase::DP_PHASE_5);
                updateEcuInformationList(ecuInformation);
            } else if ((sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE)) && (sfid == static_cast<uint8_t>(UDS_SID::SID_3E_TESTER_PRESENT)))
            {
                if (udsResponse->getNRC() == 0x7FU)
                {
                    ecuInformation.setDiagPhase(CommonDefine::DiagPhase::DP_PHASE_5);
                    updateEcuInformationList(ecuInformation);
                } else
                {
                    ecuInformation.setDiagPhase(CommonDefine::DiagPhase::DP_PHASE_4);
                    updateEcuInformationList(ecuInformation);
                }
            } else
            {
                LOG_E("not support SID 0x%02x", sid);
            }
            break;
        }
        case OBCEnum::OBCProtocolType::DOCAN29BITCANFD:
        {
            //phase6 CANFD existence check response
            if ((sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_TESTER_PRESENT)) || (sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE)))
            {
                ecuInformation.setCanId(canInfo->canId());
                ecuInformation.setTargetAddress((canInfo->canId() & 0xFFFF0000U) | ((canInfo->canId() & 0xFFU) << 8U) | ((canInfo->canId() >> 8U) & 0xFFU));
                ecuInformation.setCommProtocol(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD);
                ecuInformation.setCommType(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS);
                ecuInformation.setDiagPhase(CommonDefine::DiagPhase::DP_PHASE_6);
                ecuInformation.setecuActiveFlag(true);

                updateEcuInformationList(ecuInformation);
            } else
            {
                LOG_E("not support SID 0x%02x", sid);
            }
            break;
        }
        case OBCEnum::OBCProtocolType::DOCAN29BIT:
        {
            //phase6 CAN existence check response
            if ((sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_TESTER_PRESENT)) || (sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE)))
            {
                ecuInformation.setCanId(canInfo->canId());
                ecuInformation.setTargetAddress((canInfo->canId() & 0xFFFF0000U) | ((canInfo->canId() & 0xFFU) << 8U) | ((canInfo->canId() >> 8U) & 0xFFU));
                ecuInformation.setCommProtocol(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN);
                ecuInformation.setCommType(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS);
                ecuInformation.setDiagPhase(CommonDefine::DiagPhase::DP_PHASE_6);
                ecuInformation.setecuActiveFlag(true);

                updateEcuInformationList(ecuInformation);
            } else
            {
                LOG_E("not support SID 0x%02x", sid);
            }
            break;
        }
        default:
        {
            LOG_I("unknown protocol type: %x", protocolType);
            break;
        }
    }
    (void) sid;
    (void) sfid;
}

void RemoteEcuInformation::updateEcuInformationList(const CommonDefine::EcuInformation ecuInformation)
{
    std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()};
    for (; it != m_EcuInformationList.end(); ++it)
    {
        if (it->getTargetAddress() == ecuInformation.getTargetAddress())
        {
            LOG_I("ECU is in the list, change flag to active and update information");
            it->setecuActiveFlag(true);
            const CommonDefine::DiagPhase diagPhase{ecuInformation.getDiagPhase()};
            if((diagPhase >= CommonDefine::DiagPhase::DP_UNKNOW) && (diagPhase <= CommonDefine::DiagPhase::DP_PHASE_6))
            {
                it->setDiagPhase(diagPhase);
            } else {
                LOG_E("Unknown diag phase");
            }
            it->setCommProtocol(ecuInformation.getCommProtocol());
            it->setCommType(ecuInformation.getCommType());
            break;
        }
    }
    if (it == m_EcuInformationList.end())
    {
        //the ECU was not in the list, add the ECU to the list
        LOG_I("ECU is not in the list, add new ecu");
        insertEcuInformation(ecuInformation);
    }

    const std::unordered_map<uint32_t, CommonDefine::EcuInformation>::iterator ecuInformationIt {mEcuInformationMap.find(ecuInformation.getCanId())};
    if (ecuInformationIt != mEcuInformationMap.end())
    {
        if (ecuInformationIt->second.getecuActiveFlag() == false)
        {
            ecuInformationIt->second.setecuActiveFlag(true);
        }
    } else {
        mEcuInformationMap[ecuInformation.getTargetAddress()] = ecuInformation;
    }
}

void RemoteEcuInformation::handleReadDidResponse(const uint8_t protocolType, const android::sp<OBCCanInfo> canInfo, const android::sp<UdsMessage> udsResponse)
{
    std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()};
    for (; it != m_EcuInformationList.end(); ++it)
    {
        if (it->getCanId() == canInfo->canId())
        {
            break;
        }
    }
    if (it == m_EcuInformationList.end())
    {
        LOG_E("ECU is not in the list");
    }
    else{
        const uint8_t sid {udsResponse->getSID()};
        const uint8_t sfid {udsResponse->getSFID()};
        const android::sp<::Buffer> udsPayload {udsResponse->GetUdsPayload()};

        if (sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_SESSION_CONTROL))
        {
            LOG_I("Session control response: 0x%x", sfid);
        } else if (sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER))
        {
            //update ECU sw/hw part number
            uint16_t did {0x0000U};
            if(udsPayload->data() != nullptr) {
                did = static_cast<uint16_t>(static_cast<uint16_t>((static_cast<uint16_t>(sfid) << 8U)) | static_cast<uint16_t>(udsPayload->data()[0]));
            } else {
                LOG_E("udsPayload->data() is null");
            }
            
            if ((did == 0xF181U) || (did == 0xF188U))
            {
                if(udsPayload->data() != nullptr) {
                    it->setSwPartNumber(std::string(&udsPayload->data()[1], &udsPayload->data()[1] + udsPayload->size() - 1U));
                    LOG_D("SWPN: %s", it->getSwPartNumber());
                } else {
                    LOG_E("udsPayload->data() is null");
                }
            } else if ((did == 0xF191U) || (did == 0x0105U))
            {
                if(udsPayload->data() != nullptr) {
                    it->setHwPartNumber(std::string(&udsPayload->data()[1], &udsPayload->data()[1] + udsPayload->size() - 1U));
                    LOG_D("HWPN: %s", it->getHwPartNumber());
                } else {
                    LOG_E("udsPayload->data() is null");
                }
            } else
            {
                LOG_E("DID not support: 0x%x", did);
            }
        } else if (sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE))
        {
            if(udsPayload->data() != nullptr) {
                LOG_E("Negative response: sid: %x, nrc: %x", sfid, udsPayload->data()[0]);
            } else {
                LOG_E("udsPayload->data() is null");
            }
        } else
        {
            LOG_E("SID %x is not support", sid);
        }
        (void)protocolType;
        (void)sfid;
    }
}

void RemoteEcuInformation::onCenterCommandForward(const android::sp<CenterReqData> &pCenterReqData)
{
    LOG_I("EcuInformation receive Center request");
    const uint32_t prio_data{pCenterReqData->getCenterReq_prio()};
    const DiagTrigger::DiagTriggerType trigger_type {pCenterReqData->getTrigger_type()};
    const uint64_t mCenterReq_CollectionID {pCenterReqData->getCenterReq_CollectionID()};
    uint8_t colId_ptr[sizeof(mCenterReq_CollectionID)];
    (void)memcpy(&colId_ptr[0], &mCenterReq_CollectionID, sizeof(mCenterReq_CollectionID));
    const android::sp<::Buffer> colId_sp {new ::Buffer()};
    colId_sp->setTo(&colId_ptr[0], sizeof(mCenterReq_CollectionID));
    if(prio_data<= static_cast<uint32_t>(INT32_MAX))
    {
        if ((trigger_type >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)
            && (trigger_type <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX))
        {
            const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_TRIGGER_FROM_CENTER, static_cast<int32_t>(prio_data), static_cast<int32_t>(trigger_type))};
            msg->buffer.setTo(colId_sp->data(), sizeof(mCenterReq_CollectionID));
            (void)msg->sendToTarget();
        }
        else
        {
            LOG_E("trigger_type is invalid");
        }
    }
    else
    {
        LOG_E("Error : prio_data is out of range INT32");
    }
    (void) trigger_type;
}

void RemoteEcuInformation::triggerEcuInfo(const uint32_t prio, const uint64_t colID, const DiagTrigger::DiagTriggerType trigType)
{
    LOG_I("Ecu information trigger");
    const uint32_t nextTriggerId {TriggerIDGenerator::getInstance().getNextId()};
    android::sp<DiagTrigger> pTrigger{new DiagTrigger(trigType,
                                                      prio,
                                                      DiagTrigger::DiagTriggerFunc::ECU_UPDATE_INFO,
                                                      nextTriggerId)};
    // pTrigger->setTriggerTime(time);
    pTrigger->setCollectionId(colID);
    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
    LOG_D("Check mSaveReq size: %d", mSaveReq.size());
    if(!ret.second) {
        ret.first->second = pTrigger;
    }
    LOG_D("Check saved trigger ID: %d", ret.first->first);
}

bool RemoteEcuInformation::checkPrecondition()
{
    //get RDG active flag
    const uint8_t RDG_flag {DiagManagerAdapter::getInstance()->getRDGFlag()};
    //get IG status
    const IG_STATUS IG_status {PowerManagerAdapter::getInstance()->getIgnitionStatus()};
    //get data upload consent status
    const bool consent_status {DiagManagerAdapter::getInstance()->getAllUploadConsent()};
    //get SRVC_AC disregard flag
    if (mTriggerType == DiagTrigger::DiagTriggerType::CENTER_TRIGGER) //one-shot trigger
    {
        m_srvc_disregard_flag = CollectionCondition::getInstance().getCenterRequestEcuInformation()->srvc_ac_disregard_flag();
    }
    else //routine trigger
    {
        m_srvc_disregard_flag = CollectionCondition::getInstance().getCollectionConditionEcuInformation()->srvc_ac_disregard_flag();
    }
    //get under repair status
    const uint8_t underRepairStatus{DiagManagerAdapter::getInstance()->getUnderRepairStatus()};

    const bool ret{(RDG_flag == 0x01U) && (IG_status == IG_STATUS_ON) && ((consent_status == true) || (m_srvc_disregard_flag == true)) && (underRepairStatus == 0x00U)};
    (void) IG_status;
    (void) consent_status;
    return ret;
}

void RemoteEcuInformation::loadEcuInfoListFromFile()
{
    std::ifstream ifs {};
    static constexpr char_t ECU_INFORMATION_LIST_PATH[]{"/data/rdg/ecu_info_list"};
    ifs.open(&ECU_INFORMATION_LIST_PATH[0], std::ifstream::in);
    if (ifs.is_open())
    {
        CommonDefine::EcuInformation ecuInfo {};
        char_t tmp[sizeof(CommonDefine::EcuInformation)];

        //read first ecu
        (void)ifs.read(&tmp[0], sizeof(CommonDefine::EcuInformation));
        (void)memcpy(&ecuInfo, &tmp[0], sizeof(CommonDefine::EcuInformation));

        while (ifs.good())
        {
            //set active flag to false
            ecuInfo.setecuActiveFlag(false);

            //push to ecu list
            LOG_I("loaded ECU: address(0x%02x), activeFlag(%d)", ecuInfo.getTargetAddress(), ecuInfo.getecuActiveFlag());
            m_EcuInformationList.push_back(ecuInfo);
            mEcuInformationMap[ecuInfo.getTargetAddress()] = ecuInfo;

            //read next ecu
            (void)ifs.read(&tmp[0], sizeof(CommonDefine::EcuInformation));
            (void)memcpy(&ecuInfo, &tmp[0], sizeof(CommonDefine::EcuInformation));
        }

        ifs.close();
    } else
    {
        LOG_E("File open fail");
    }
}

void RemoteEcuInformation::saveEcuInfoListToFile()
{
    ofstream ofs {};
    static constexpr char_t ECU_INFORMATION_LIST_PATH[]{"/data/rdg/ecu_info_list"};
    ofs.open(&ECU_INFORMATION_LIST_PATH[0], std::ofstream::out);
    if (ofs.is_open())
    {
        CommonDefine::EcuInformation ecuInfo {};
        std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()};
        for (; it != m_EcuInformationList.end(); ++it)
        {
            ecuInfo = *it;
            char_t tmp[sizeof(CommonDefine::EcuInformation)];
            (void)memcpy(&tmp[0], &ecuInfo, sizeof(CommonDefine::EcuInformation));
            (void)ofs.write(&tmp[0], sizeof(CommonDefine::EcuInformation));
        }

        ofs.close();
    } else
    {
        LOG_E("File open fail");
    }
}

void RemoteEcuInformation::startDidAcquisition()
{
    mEcuTransList.clear();
    mTransmissioIdList = {};
    mState = EcuState::ECU_DID_DATA_ACQUISITION;

    std::list<CommonDefine::EcuInformation>::iterator it{};
    for (it = m_EcuInformationList.begin(); it != m_EcuInformationList.end(); ++it)
    {
        if (it->getecuActiveFlag() == true)
        {
            //add read DID requests to transmission list
            uint8_t sid {0x00U};
            uint8_t payload_SWPN[2] {0x00U};
            uint8_t payload_HWPN[2] {0x00U};
            const android::sp<::Buffer> udsData{new ::Buffer()};

            if (it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5)
            {
                //Phase5 (F181: SWPN, 0105: HWPN)
                payload_SWPN[0] = 0xF1U;
                payload_SWPN[1] = 0x81U;
                payload_HWPN[0] = 0x01U;
                payload_HWPN[1] = 0x05U;

                const uint8_t type {static_cast<uint8_t>(convertObcProtocolType(it->getCommProtocol(), it->getCommType(), it->getTargetAddress()))};

                //uds req to transition to the Remote Session
                sid  = static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL);
                uint8_t sfid {0x40U}; //remote session
                udsData->setTo(&sid, 1);
                udsData->append(&sfid, 1);
                const android::sp<EcuUdsTransmission> transmission {new EcuUdsTransmission(*this, it->getTargetAddress(), it->getNTa(), type, udsData)};
                mEcuTransList[transmission->getTransmissionId()] = transmission;
                mTransmissioIdList.push(transmission->getTransmissionId());

                //uds req to get DID data
                sid = static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER);

                udsData->setTo(&sid, 1);
                udsData->append(&payload_SWPN[0], sizeof(payload_SWPN));
                const android::sp<EcuUdsTransmission> transmission_SWPN {new EcuUdsTransmission(*this, it->getTargetAddress(), it->getNTa(), type, udsData)};
                mEcuTransList[transmission_SWPN->getTransmissionId()] = transmission_SWPN;
                mTransmissioIdList.push(transmission_SWPN->getTransmissionId());

                udsData->setTo(&sid, 1);
                udsData->append(&payload_HWPN[0], sizeof(payload_HWPN));
                const android::sp<EcuUdsTransmission> transmission_HWPN {new EcuUdsTransmission(*this, it->getTargetAddress(), it->getNTa(), type, udsData)};
                mEcuTransList[transmission_HWPN->getTransmissionId()] = transmission_HWPN;
                mTransmissioIdList.push(transmission_HWPN->getTransmissionId());
            } else if (it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6)
            {
                //Phase6 (F188: SWPN, F191: HWPN)
                payload_SWPN[0] = 0xF1U;
                payload_SWPN[1] = 0x88U;
                payload_HWPN[0] = 0xF1U;
                payload_HWPN[1] = 0x91U;

                const uint8_t type  {static_cast<uint8_t>(convertObcProtocolType(it->getCommProtocol(), it->getCommType(), it->getTargetAddress()))};

                //uds req to get DID data
                sid = static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER);

                udsData->setTo(&sid, 1);
                udsData->append(&payload_SWPN[0], sizeof(payload_SWPN));
                const android::sp<EcuUdsTransmission> transmission_SWPN {new EcuUdsTransmission(*this, it->getTargetAddress(), it->getNTa(), type, udsData)};
                mEcuTransList[transmission_SWPN->getTransmissionId()] = transmission_SWPN;
                mTransmissioIdList.push(transmission_SWPN->getTransmissionId());

                udsData->setTo(&sid, 1);
                udsData->append(&payload_HWPN[0], sizeof(payload_HWPN));
                const android::sp<EcuUdsTransmission> transmission_HWPN {new EcuUdsTransmission(*this, it->getTargetAddress(), it->getNTa(), type, udsData)};
                mEcuTransList[transmission_HWPN->getTransmissionId()] = transmission_HWPN;
                mTransmissioIdList.push(transmission_HWPN->getTransmissionId());
            }
            else{
                LOG_E("ECU phase4, set SWPN, HWPN to empty");
                it->setSwPartNumber("");
                it->setHwPartNumber("");
            }
            (void) sid;
            (void) payload_HWPN;
            (void) payload_SWPN;

        }
        else
        {
            // Do nothing
        }
    }

    //send first uds message
    LOG_D("mEcuTransList size = %d", mTransmissioIdList.size());
    if ( mTransmissioIdList.size() > 0U )
    {
        mCurrentTransmissionId = mTransmissioIdList.front();
        const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
        if (itEcu != mEcuTransList.end())
        {
            itEcu->second->connect();
        }
    } else {
        // Release OBC resource
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        if(mTriggerId<=static_cast<uint32_t>(INT32_MAX))
        {
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mTriggerType);
        }
    }
}

void RemoteEcuInformation::notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const uint32_t& pTriggerId, const bool dueToIgOff)
{
    /* Check if trigger is in the saved trigger*/
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(pTriggerId)};
    if(it != mSaveReq.end()) {
        LOG_I("notify Trigger state: %d TriggerID: %d", pState, pTriggerId);
        handleTrigger(pState, pTriggerId, dueToIgOff);
    } else {
        LOG_E("Can not find triggerID in saved list");
        if(pTriggerId > static_cast<uint32_t>(INT32_MAX)) {
            LOG_E("out of range INT32_MAX");
        }
        PriorityControl::getInstance()->notifyTriggerNoFound(static_cast<int32_t>(pTriggerId));
    }
}

void RemoteEcuInformation::handleTrigger(const DiagTrigger::DiagTriggerState& pState, const uint32_t& pTriggerId, const bool dueToIgOff)
{
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(pTriggerId)};
    if(it != mSaveReq.end()) {
        LOG_I("notify Trigger state: %d TriggerID: %d", pState, pTriggerId);
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
            if (mIsSuspending == true)
            {
                mIsEcuRunning = true;
                mIsSuspending = false;
                
                //send next uds message
                LOG_D("mEcuTransList size = %d", mTransmissioIdList.size());
                if ( mTransmissioIdList.size() > 0U )
                {
                    mCurrentTransmissionId = mTransmissioIdList.front();
                    const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
                    if (itEcu != mEcuTransList.end())
                    {
                        itEcu->second->connect();
                    }
                } else {
                    // Release OBC resource
                    OnboardclientAdapter::getInstance()->ReleaseObcResource();
                    if(mTriggerId<=static_cast<uint32_t>(INT32_MAX))
                    {
                        PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mTriggerType);
                    }
                }
            } else
            {
                LOG_I("TRIGGER_PROCESSING");
                const uint32_t prio_data{it->second->getPriority()};
                // android::sp<CommonDefine::RDGLocationData> location = LocationManagerAdapter::getInstance()->getLocationData();
                // TimeManager &mTimeManagerService = TimeManager::getInstance();
                // const int64_t currentTime = MS_TO_SEC(mTimeManagerService.getCurrentMilliSec());
                mTriggerId = pTriggerId;
                const DiagTrigger::DiagTriggerType pTriggerType {it->second->getType()};
                mTriggerType = pTriggerType;
                // triggerStartUp(pTriggerType, currentTime, location);
                const uint64_t colID{it->second->getCollectionID()};
                mColId = colID;
                mPriority = prio_data;
                (void)mHandler->obtainMessage(MainHandler::CMD_START_ECU_INFORMATION)->sendToTarget();
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED:
        {
            LOG_I("TRIGGER_SUSPENDED");
            if (mIsEcuRunning == true)
            {
                mIsEcuRunning = false;
                mIsSuspending = true;

                //disconnect ECU
                const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
                if (itEcu != mEcuTransList.end())
                {
                    const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
                    currentTransmission->disconnect();
                }
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
                if(mTriggerId<=static_cast<uint32_t>(INT32_MAX))
                {
                    PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mTriggerType);
                }
                else{
                    LOG_E("mTriggerId is out of range INT32");
                }
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
        {
            LOG_I("TRIGGER_DISCARDED");
            // Abort aquisition
            mIsEcuRunning = false;

            //disconnect ECU
            const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
            if (itEcu != mEcuTransList.end())
            {
                const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
                currentTransmission->disconnect();
            }

            while(mTransmissioIdList.empty() != true) {
                mTransmissioIdList.pop();
            }

            // mDiagResp.clear();

            mEcuTransList.clear();

            // Release OBC resource
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            if(mTriggerId<=static_cast<uint32_t>(INT32_MAX))
            {
                PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mTriggerType);
            }
            else{
                LOG_E("mTriggerId is out of range INT32");
            }
            
            mEcuUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
            if (mEcuUploadErrorData != nullptr)
            {
                if (dueToIgOff == true)
                {
                    LOG_I("The Ecu information acquisition sequence is aborted due to IG status");
                    mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
                }
                else
                {
                    LOG_I("The Ecu information acquisition sequence is aborted by Priority control");
                    mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_INTERRUPT_PROHIBITED);
                }
                mIsNeedUploadErrorData = true;
                (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
            } else {
                LOG_E("mEcuUploadErrorData is null");
            }
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
    (void)dueToIgOff;
}

RemoteEcuInformation::EcuUdsTransmission::EcuUdsTransmission( RemoteEcuInformation& ecu, const uint32_t mCanId, const uint8_t mNTa, const uint8_t mProtocolType, const sp<::Buffer> UdsData)
: android::RefBase()
, mECUInfo(ecu)
, mTimerHandler(ecu)
, nTa(mNTa)
, canId(mCanId)
, protocolType(mProtocolType)
, connectId(0U)
, transmissionId(0U)
, state(EcuTransState::ECU_TRANS_INIT)
, isFunctionalRequest(false)
, mTimeOut(&mTimerHandler, TimerHandler::ID_TRANSMISSION_TIMEOUT)
{
    if (udsReq.Parser(UdsData) != TIGER_ERR::E_OK)
    {
        LOG_E("Parser error");
    }

    const uint8_t sid {this->udsReq.getSID()};
    const uint8_t sfid {this->udsReq.getSFID()};
    const android::sp<::Buffer> udsPayload {udsReq.GetUdsPayload()};
    uint16_t did {0x0000U};
    if (sid == static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL))
    {
        did = static_cast<uint16_t>(static_cast<uint16_t>((static_cast<uint16_t>(sfid) << 8U)) | static_cast<uint16_t>(0U));
    }
    else if ((sid == static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER)) && (udsPayload->data() != nullptr))
    {
        did = static_cast<uint16_t>(static_cast<uint16_t>((static_cast<uint16_t>(sfid) << 8U)) | static_cast<uint16_t>(udsPayload->data()[0]));
    }
    else
    {
        did = 0x0000U;
    }

    transmissionId = ((static_cast<uint64_t>(this->canId) & 0xFFFFFFFFU) << 32U)
                    | ((static_cast<uint64_t>(this->protocolType) & 0xFFU) << 24U)
                    | ((static_cast<uint64_t>(sid) & 0xFFU) << 16U)
                    | (static_cast<uint64_t>(did) & 0xFFFFU);
    (void)sfid;
}

void RemoteEcuInformation::EcuUdsTransmission::connect()
{
    LOG_I("Start connect, CanId = 0x%02x, protocol = %d", this->canId, this->protocolType);
    this->state = EcuUdsTransmission::EcuTransState::ECU_TRANS_CONNECT;
    
    const android::sp<OBCTransportInfo> obcTransportInfo {new OBCTransportInfo()};
    OBCCanInfo canInfo {};
    canInfo.canId() = this->canId;
    std::stringstream sstream {};
    sstream << &std::hex << this->nTa;
    canInfo.nTa().push_back(sstream.str());
    if (isFunctionalRequest)
    {
        obcTransportInfo->setData(this->protocolType, canInfo, false, 0U);
        mTimeOut.setDurationMs(TRANSMISSION_TIME_OUT_DURATION, 0U);
    }
    else
    {
        obcTransportInfo->setData(this->protocolType, canInfo, false, 1U);
        mTimeOut.setDurationMs(TRANSMISSION_TIME_OUT_DURATION, 0U);
    }
    const android::sp<OBCConnectInfo> obj {new OBCConnectInfo()};
    const error_t res {OnboardclientAdapter::getInstance()->connect(obcTransportInfo, APP_NAME, obj)};
    (void) res;
    
    tOBCConnectInfo info;
    obj->setDataFormat(info);
    if (info.response != OBCEnum::OBCErrCode::OBC_OK)
    {
        LOG_E("can't connect to canid = 0x%02x", this->canId);
        (void)mECUInfo.mHandler->obtainMessage(MainHandler::CMD_ECUINFO_TRANSMISSION_FINISH)->sendToTarget();;
    } else {
        LOG_D("connect success transmissionID = 0x%02llx", this->transmissionId);
        this->connectId = info.connectId;
        OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
        this->send();
    }
}

void RemoteEcuInformation::EcuUdsTransmission::stopTimeout()
{
    this->mTimeOut.stop();
}

void RemoteEcuInformation::EcuUdsTransmission::resetTimer()
{
    this->mTimeOut.stop();
    this->mTimeOut.setDurationMs(TRANSMISSION_TIME_OUT_DURATION, 0U);
    this->mTimeOut.start();
}

void RemoteEcuInformation::EcuUdsTransmission::disconnect()
{
    (void)OnboardclientAdapter::getInstance()->disconnectECU(this->connectId);
    OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
    state = EcuUdsTransmission::EcuTransState::ECU_TRANS_DONE;
    mECUInfo.finishCurrentTransmission();
}

void RemoteEcuInformation::EcuUdsTransmission::send()
{
    const android::sp<::Buffer> udsData {this->udsReq.ToUdsData()};

    const uint8_t err {OnboardclientAdapter::getInstance()->sendUdsData(this->connectId, udsData)};
    if (err != 0x00U) //E_OK
    {
        LOG_E("SendUdsData for transmissionId 0x%02llx error = %d -> Disconnect", err);
        this->disconnect();
    }
    else {
        LOG_I("SendUdsData for transmissionId 0x%02llx success", this->transmissionId);
        this->state = EcuUdsTransmission::EcuTransState::ECU_TRANS_SEND_UDS;
        mTimeOut.start();
        LOG_D("start transmissionId 0x%02llx timeout success", this->transmissionId);
    }
}
uint64_t RemoteEcuInformation::EcuUdsTransmission::getTransmissionId() const noexcept
{
    return transmissionId;
}
RemoteEcuInformation::EcuUdsTransmission::EcuTransState RemoteEcuInformation::EcuUdsTransmission::getState() const noexcept
{
    return state;
}
uint16_t RemoteEcuInformation::EcuUdsTransmission::getConnectId() const noexcept
{
    return connectId;
}

void RemoteEcuInformation::EcuUdsTransmission::setRequestType(const bool isFunctional) noexcept
{
    this->isFunctionalRequest = isFunctional;
}

bool RemoteEcuInformation::EcuUdsTransmission::checkfunctionRequest() const noexcept
{
    return isFunctionalRequest;
}

void RemoteEcuInformation::finishCurrentTransmission(void)
{
    if (this->mIsEcuRunning == true) {
        LOG_I("finish Current transmission, mTransmissioIdList size = %d", mTransmissioIdList.size());
        if ( mTransmissioIdList.size() > 1U ) 
        {
            mTransmissioIdList.pop();
            LOG_D("finish Current transId = 0x%02llx", mCurrentTransmissionId);
            mCurrentTransmissionId = mTransmissioIdList.front();
            const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator it {mEcuTransList.find(mCurrentTransmissionId)};
            if (it != mEcuTransList.end())
            {
                it->second->connect();
            }
        } else  {
            if(mTransmissioIdList.empty() != true) {
                mTransmissioIdList.pop();
            }
            if (mState == EcuState::ECU_EXISTENCE_CHECK)
            {
                //add DCM information to list
                LOG_I("add DCM information to ecu list");
                CommonDefine::EcuInformation dcmInfo {};
                dcmInfo.setCommProtocol(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN);
                dcmInfo.setCommType(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS);
                dcmInfo.setTargetAddress(0x18DA2BE1U);
                dcmInfo.setCanId(0x18DAE12BU);
                dcmInfo.setDiagPhase(CommonDefine::DiagPhase::DP_PHASE_6);
                dcmInfo.setecuActiveFlag(true);
                updateEcuInformationList(dcmInfo);

                //add DID acquisition transmission
                (void)mHandler->obtainMessage(MainHandler::CMD_START_DID_ACQUISITION)->sendToTarget();
            } else if (mState == EcuState::ECU_DID_DATA_ACQUISITION)
            {
                //Release OBC resource
                // OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
                OnboardclientAdapter::getInstance()->ReleaseObcResource();

                //Save to NVM (RDG30-R-0949)
                saveEcuInfoListToFile();

                //save last update time
                TimeManager &mTimeManagerService {TimeManager::getInstance()};
                int64_t mTempTime{};
                mTempTime = static_cast<int64_t>(ParamsDef::getCurrentAcquisiteTime());
                if(mTempTime>=0)
                {   
                    mLastUpdateTime = static_cast<uint64_t>(mTempTime);
                }
                else{
                    // print error log
                }

                mState = EcuState::ECU_IDLE;

                /* Check if pTriggerId is in savedID then notify done to priorityControl*/
                LOG_I("Finish Ecu info Acquisition triggerID: %d", mTriggerId);
                LOG_D("Check mSaveReq size before: %d", mSaveReq.size());
                const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(mTriggerId)};
                if(it != mSaveReq.end()) {
                    if(mTriggerId<=static_cast<uint32_t>(INT32_MAX))
                    {
                        PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), it->second->getType());
                    }
                    else{
                        LOG_E("mTriggerId is out of range INT32");
                    }
                    (void)mSaveReq.erase(it);
                } else {
                    LOG_D("Can not find triggerId: %d in mSaveReq", mTriggerId);
                }
                LOG_D("Check mSaveReq size after: %d", mSaveReq.size());

                //Upload
                (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
                (void) mTimeManagerService;
            } else
            {
                LOG_E("invalid state");
            }
        }
    }
}

void RemoteEcuInformation::onTransmissionTimeout(void)
{
    (void)mHandler->obtainMessage(MainHandler::CMD_TRANSMISSION_TIMEOUT)->sendToTarget();
}

void RemoteEcuInformation::handleTransmissionTimeout()
{
    LOG_I("Event Transmission Timeout, currentTrans = 0x%02llx", mCurrentTransmissionId);
    const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator it {mEcuTransList.find(mCurrentTransmissionId)};
    if ((it != mEcuTransList.end()) && (it->second->getState() == EcuUdsTransmission::EcuTransState::ECU_TRANS_SEND_UDS))
    {
        it->second->disconnect();
    }
}

void RemoteEcuInformation::makeUploadRequest()
{
    if (mIsNeedUploadErrorData == true)
    {
        mEcuUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
        mEcuUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
        mEcuUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
        const int32_t tz{TimeManager::getInstance().getOffset()};
        mEcuUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(tz / 60);
        mEcuUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(tz % 60);
        mEcuUploadErrorData->mutable_rdg_common_request_header()->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
        const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
        mEcuUploadErrorData->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, counterValue));
        mEcuUploadErrorData->set_collection_condition_id(mColId);
        mEcuUploadErrorData->set_counter_value(counterValue);
        (void)counterValue;
        const int64_t currentTime {static_cast<int64_t>(ParamsDef::getCurrentAcquisiteTime())};
        mEcuUploadErrorData->set_data_creation_date(currentTime > 0 ? static_cast<uint64_t>(currentTime) : 0U);
        mEcuUploadErrorData->set_obd2_installed_flag(mApp.getOBDStatus());
        mEcuUploadErrorData->set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
        mEcuUploadErrorData->set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ECU_INFORMATION_UPDATE_REQUEST);
        mEcuUploadErrorData->set_trigger_type(vccomif::rdg::v1::interfaces::TriggerType::TT_OTHER_TRIGGER);

        /*Save upload data to file*/ 
        const uint32_t uploadId {UploadManager::getInstance()->genRequestId()};
        std::string file_dir {std::to_string(uploadId)};
        (void)file_dir.append("_UploadErrorDataRequest.dat");
        if (DataModel<UploadErrorDataRequest>::save(file_dir, *mEcuUploadErrorData) != E_OK)
        {
            LOG_E("save fail !");
        }
        const android::sp<UploadTask> task {new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
        task->setUploadPatch(file_dir);
        task->setUploadId(static_cast<uint64_t>(uploadId));
        /*Set priority*/
        task->setUploadPrio(mPriority);
        task->setSrvcAcFlag(m_srvc_disregard_flag);
        const uint64_t fileSize{static_cast<uint64_t>(mEcuUploadErrorData->ByteSizeLong())};
        task->setFileSize(fileSize);
        UploadManager::getInstance()->requestUploadTask(task);

        //print data
        LOG_I("error upload data:");
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        std::string log_str{};
        (void)google::protobuf::util::MessageToJsonString(*mEcuUploadErrorData, &log_str, option);
        printData(log_str);
    } else
    {

        //package data
        vccomif::rdg::v1::interfaces::UploadEcuInformationRequest uploadRequest{};
        uploadRequest.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(PROTOBUF_MESSAGE_DEFINITION_VERSION);
        uploadRequest.mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
        uploadRequest.mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation::AppCommonHeaderVehicleToCenter_GeodesyInformation_GI_WGS84);
        const int32_t tz{TimeManager::getInstance().getOffset()};
        uploadRequest.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(tz / 60);
        uploadRequest.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(tz % 60);        uploadRequest.mutable_rdg_common_request_header()->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ECU_INFORMATION);
        const uint32_t counterValue {UploadManager::getInstance()->getCounterValue()};
        uploadRequest.mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ECU_INFORMATION, counterValue));
        uploadRequest.set_collection_condition_id(mColId);
        uploadRequest.set_counter_value(counterValue); 
        (void)counterValue;
        const int64_t currentTime {static_cast<int64_t>(ParamsDef::getCurrentAcquisiteTime())};
        uploadRequest.set_data_creation_date(currentTime > 0 ? static_cast<uint64_t>(currentTime) : 0U);
        uploadRequest.set_obd2_installed_flag(mApp.getOBDStatus());
        uploadRequest.set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
        for (std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()}; it != m_EcuInformationList.end(); ++it)
        {
            vccomif::rdg::v1::interfaces::UploadEcuInformationRequest_EcuInformation* const ecuInformation {uploadRequest.add_ecu_information()};
            ecuInformation->set_last_update_date(mLastUpdateTime);
            ecuInformation->mutable_ecu_address_information()->set_communication_protocol(it->getCommProtocol());
            ecuInformation->mutable_ecu_address_information()->set_communication_type(it->getCommType());
            ecuInformation->mutable_ecu_address_information()->set_target_address(it->getTargetAddress());
            const CommonDefine::DiagPhase diagPhase{it->getDiagPhase()};
            if((diagPhase >= CommonDefine::DiagPhase::DP_UNKNOW) && (diagPhase <= CommonDefine::DiagPhase::DP_PHASE_6))
            {
                ecuInformation->set_diagnostics_phase(static_cast<vccomif::rdg::v1::interfaces::DiagnosticsPhase>(diagPhase));
            } else {
                LOG_E("Unknown diag phase");
            }
            ecuInformation->set_ecu_active_flag(it->getecuActiveFlag() == true ? true : false);
            ecuInformation->set_hardware_part_number(it->getHwPartNumber());
            ecuInformation->set_software_part_number(it->getSwPartNumber());
        }

        /*Save upload data to file*/ 
        const uint32_t uploadId {UploadManager::getInstance()->genRequestId()};
        std::string file_dir {std::to_string(uploadId)};
        (void)file_dir.append("_UploadEcuDataRequest.dat");
        const error_t bStored{DataModel<UploadEcuInformationRequest>::save(file_dir, uploadRequest)};
        if (bStored == E_OK)
        {
            const uint8_t operation{getOperation()};
            // Self-Diag
            DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
        }
        const android::sp<UploadTask> task{new UploadTask(uploadId)};
        task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG100);
        task->setUploadPatch(file_dir);
        task->setUploadId(static_cast<uint64_t>(uploadId));
        /*Set priority*/
        task->setUploadPrio(mPriority);
        task->setSrvcAcFlag(m_srvc_disregard_flag);
        const uint64_t fileSize{static_cast<uint64_t>(uploadRequest.ByteSizeLong())};
        task->setFileSize(fileSize);
        UploadManager::getInstance()->requestUploadTask(task);

        //print data
        LOG_I("ECU upload data:");
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        std::string log_str{};
        (void)google::protobuf::util::MessageToJsonString(uploadRequest, &log_str, option);
        printData(log_str);
    }

    //reset variable
    mIsNeedUploadErrorData = false;
    mIsEcuRunning = false;

    if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
    {
        const int64_t current_time {ParamsDef::getCurrentAcquisiteTime()};
        LOG_I("Collection ID: %llu - Last complete time: %lld", mColId, current_time);
        mApp.notifyLastOpComplTime(mColId, current_time);
    }
}

OBCEnum::OBCProtocolType RemoteEcuInformation::convertObcProtocolType(const CommunicationProtocol protocol, const CommunicationType type, const uint32_t targetAddress) const noexcept
{
    OBCEnum::OBCProtocolType oBCProtocolType {OBCEnum::OBCProtocolType::DOCAN};
    // Convert OBCProtocolType of OBC spec
    switch(protocol) 
    {
        case CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN:
        {
            if (type == CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) {
                // for DOCAN 11bit
                oBCProtocolType = OBCEnum::OBCProtocolType::DOCAN;
                const uint8_t nTa{static_cast<uint8_t>((targetAddress >> 8U) & 0xFFU)};
                if (nTa != 0xFFU)
                {
                    oBCProtocolType = OBCEnum::OBCProtocolType::DOCAN11BITEX;
                }
            } 
            else if (type == CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS) {
                // for DOCAN 29bit
                oBCProtocolType = OBCEnum::OBCProtocolType::DOCAN29BIT;
            } 
            else {
                oBCProtocolType = OBCEnum::OBCProtocolType::DOCAN;
            }
            break;
        }
        case CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD:
        {
            oBCProtocolType = OBCEnum::OBCProtocolType::DOCAN29BITCANFD;
            break;
        }
        default:
        {
            oBCProtocolType = OBCEnum::OBCProtocolType::DOCAN;
            break;
        }
    }
    return oBCProtocolType;
}

OBCEnum::OBCProtocolType RemoteEcuInformation::getObcProtocolType(const uint32_t targetAddress)
{
    OBCEnum::OBCProtocolType oBCProtocolType {OBCEnum::OBCProtocolType::DOCAN};
    CommunicationProtocol protocol {CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN};
    CommunicationType type {CommunicationType::EcuAddressInformation_CommunicationType_CT_UNKNOWN};
    const std::unordered_map<uint32_t, CommonDefine::EcuInformation>::iterator it {mEcuInformationMap.find(targetAddress)};
    // RDG30-R-1267
    if (it != mEcuInformationMap.end())
    {
        protocol = it->second.getCommProtocol();
        type = it->second.getCommType();
    }
    // Convert OBCProtocolType of OBC spec
    oBCProtocolType = convertObcProtocolType(protocol, type, targetAddress);
    LOG_D("Get communication protocol information success, type = %d", static_cast<uint8_t>(oBCProtocolType));
    return oBCProtocolType;
}

CommunicationType RemoteEcuInformation::getCommType(const uint32_t canId)
{
    LOG_V("[getCommType] CAN ID: 0x%02x", canId);

    CommunicationType type {CommunicationType::EcuAddressInformation_CommunicationType_CT_UNKNOWN};

    for( std::unordered_map<uint32_t, CommonDefine::EcuInformation>::iterator it {mEcuInformationMap.begin()}; it != mEcuInformationMap.end(); it++)
    {
        if(it->second.getCanId() == canId)
        {
            type = it->second.getCommType();
            LOG_V("[getCommType] commType: %d", type);
            break;
        }
    }

    return type;
}

uint32_t RemoteEcuInformation::getCanId(const uint32_t targetAddress)
{
    LOG_V("[getCanId] targetAddress: 0x%02x", targetAddress);

    uint32_t canId {0U};

    for( std::unordered_map<uint32_t, CommonDefine::EcuInformation>::iterator it {mEcuInformationMap.begin()}; it != mEcuInformationMap.end(); it++)
    {
        if(it->second.getTargetAddress() == targetAddress)
        {
            canId = it->second.getCanId();
            LOG_V("[getCanId] canId: %d", canId);
            break;
        }
    }

    return canId;    
}

void RemoteEcuInformation::onReceiveIG(const bool status) const
{
    (void)mHandler->obtainMessage(MainHandler::CMD_CHANGE_IG_STATUS, static_cast<int32_t>(status))->sendToTarget();
}

void RemoteEcuInformation::changeIGStatus(const bool status)
{
    LOG_I("changeIGStatus = %s", status ? "IGN_ON" : "IGN_OFF");
    if ((status == false) && (mIsEcuRunning == true))
    {
        // Abort aquisition
        mIsEcuRunning = false;

        //disconnect ECU
        const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
        if (itEcu != mEcuTransList.end())
        {
            const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
            currentTransmission->disconnect();
        }

        while(mTransmissioIdList.empty() != true) {
            mTransmissioIdList.pop();
        }

        // mDiagResp.clear();

        mEcuTransList.clear();

        // Release OBC resource
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        if(mTriggerId<=static_cast<uint32_t>(INT32_MAX))
        {
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mTriggerType);
        }
        
        mEcuUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
        if (mEcuUploadErrorData != nullptr)
        {
            //RDG30-R-0938
            mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
            LOG_I("The Ecu information acquisition sequence is aborted due to IG status");
            mIsNeedUploadErrorData = true;
            (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
        } else {
            LOG_E("mEcuUploadErrorData is null");
        }
    }
}

void RemoteEcuInformation::processErrorHandling()
{
    // Abort aquisition
    mIsEcuRunning = false;

    while(mTransmissioIdList.empty() != true) {
        mTransmissioIdList.pop();
    }

    // mDiagResp.clear();

    mEcuTransList.clear();

    // Release OBC resource
    OnboardclientAdapter::getInstance()->ReleaseObcResource();
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(mTriggerId)};
    if(it != mSaveReq.end())
    {
        if(mTriggerId<=static_cast<uint32_t>(INT32_MAX))
        {
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), it->second->getType());
        }
        (void)mSaveReq.erase(it);
    }
    
    mEcuUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
    if (mEcuUploadErrorData != nullptr)
    {
        if (DiagManagerAdapter::getInstance()->getRDGFlag() == 0x00U)
        {
            LOG_E("The Ecu information acquisition sequence is aborted due to RDG flag");
            mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_UNPROVIDED_VEHICLE);
        } else if (PowerManagerAdapter::getInstance()->getIgnitionStatus() != IG_STATUS::IG_STATUS_ON)
        {
            LOG_E("The Ecu information acquisition sequence is aborted due to IG status");
            mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
        } else if (DiagManagerAdapter::getInstance()->getUnderRepairStatus() == 0x01U)
        {
            LOG_E("The Ecu information acquisition sequence is aborted due to Under repair status");
            mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR);
        } else
        {
            LOG_E("The Ecu information acquisition sequence is aborted due to consent status");
            mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_OTHER_ERROR);
        }
        mIsNeedUploadErrorData = true;
        (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
    } else {
        LOG_E("mEcuUploadErrorData is null");
    }
}

uint8_t RemoteEcuInformation::getOperation() const noexcept
{
    uint8_t operation{DiagManagerAdapter::COLLECTION_CONDITIONS};
    if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
    {
        operation = DiagManagerAdapter::RD_SCHEDULE_TRIGGER;
    }
    return operation;
}

void RemoteEcuInformation::onChangedRemoteInfo(const int32_t what, const int32_t info)
{
    LOG_I("onChangedRemoteInfo");
    (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UNDER_REPAIR_FLAG_CHANGE, what, info)->sendToTarget();
}

void RemoteEcuInformation::handleUnderRepairStatusChange(const int32_t& what, const int32_t& status)
{
    LOG_D("handleUnderRepairStatusChange");
    switch (what)
    {
    case WHAT_CHANGED_REPAIR_SATUS:
    {
        if (mIsEcuRunning == true)
        {
            // Abort aquisition
            mIsEcuRunning = false;

            //disconnect ECU
            const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
            if (itEcu != mEcuTransList.end())
            {
                const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
                currentTransmission->disconnect();
            }

            while(mTransmissioIdList.empty() != true) {
                mTransmissioIdList.pop();
            }

            // mDiagResp.clear();

            mEcuTransList.clear();

            // Release OBC resource
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            if(mTriggerId<=static_cast<uint32_t>(INT32_MAX))
            {
                PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mTriggerType);
            }
            
            mEcuUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
            if (mEcuUploadErrorData != nullptr)
            {
                //RDG30-R-0938
                mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR);
                LOG_I("The Ecu information acquisition sequence is aborted due to under repair state");
                mIsNeedUploadErrorData = true;
                (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
            } else {
                LOG_E("mEcuUploadErrorData is null");
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }
    (void)status;
}

void RemoteEcuInformation::testSaveEcuInformationToFile()
{
    saveEcuInfoListToFile();
}
}
