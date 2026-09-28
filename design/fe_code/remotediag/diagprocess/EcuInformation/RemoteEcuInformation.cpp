#include "RemoteEcuInformation.h"

namespace rdgapp {

android::sp<RemoteEcuInformation> RemoteEcuInformation::mRemoteEcuInformation {nullptr};
RemoteEcuInformation::RemoteEcuInformation(const Remotediag &app, android::sp<sl::SLLooper> &privateLooper)
: android::RefBase(), RemoteDelegate()
, mApp(app)
, mHandler(new MainHandler(privateLooper, *this))
, mTimerHandler(new TimerHandler(*this))
, mState(EcuState::ECU_IDLE)
, mCurrentTransmissionId(0U)
, mPriority(0U)
, mColId(0U)
, mLastUpdateTime(0U)
, mDataCreationDate(0U)
, mIsNeedUploadErrorData(false)
, mTriggerId(0U)
, mTriggerType(DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)
, m_srvc_disregard_flag(false)
{
    mRemoteEcuInformation = this;
    (void)mHandler->obtainMessage(MainHandler::CMD_INIT_ECU_INFORMATION)->sendToTarget();
    mEcuPartNumberMap.clear();
}

RemoteEcuInformation::~RemoteEcuInformation() = default;

android::sp<RemoteEcuInformation> RemoteEcuInformation::getInstance()
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
            mECUInfo.loadEcuInfoListFromFile();
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
            mECUInfo.startDiagTask();
            break;
        }
        case CMD_REQUEST_TO_PRIORITY_CONTROL:
        {
            LOG_I("CMD_REQUEST_TO_PRIORITY_CONTROL");
            sp<DiagTrigger> pTrigger {nullptr};
            handlemsg->getObject(pTrigger);
            LOG_D("Trigger Request have Type: %d Func: %d Prio: %d ID: %d ColID: %llu", 
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
        case CMD_STOP_RDG:
        {
            const int32_t isStop{handlemsg->arg1};
            if(isStop == 1) {
                LOG_I("CMD_STOP_RDG");
                mECUInfo.handleStopRDG();
            } else {
                LOG_I("Power source enable RDG");
            }
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

void RemoteEcuInformation::startDiagTask()
{
    //save data creation date
    const int64_t mTempTime {CommonUtils::getCurrentAcquisiteTime()};
    if (mTempTime > 0)
    {   
        mDataCreationDate = static_cast<uint64_t>(mTempTime);
    } else
    {
        mDataCreationDate = 0U;
    }

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

        //Set active flag to false
        deactiveAllEcu();

        //Send UDS to all address in ECU_Candidate_List //RDG30-R-0573
        //start from index 0
        startEcuExistenceCheck();
    }
}

void RemoteEcuInformation::startEcuExistenceCheck()
{
    // Get OBC resource
    const OBCResourceEventCode resEventInfo {OnboardclientAdapter::getInstance()->GetObcResource()};
    if ( resEventInfo != OBCResourceEventCode::OBC_GET_RESOURCE_OK )
    {
        LOG_E("ECU wait ObcResource");
        const android::sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_START_ECU_INFORMATION)};
        (void)mHandler->sendMessageDelayed(msg, 5000U);

    } 
    else 
    {
        LOG_D("Get obc resource success");
        // Lock OBC resource
        OnboardclientAdapter::getInstance()->TakeObcResource();
        mState = EcuState::ECU_RUNNING;
        mEcuTransList.clear();
        mTransmissioIdList = {};
        //add Phase5 ECU transmission list
        for (uint32_t i{0U}; i<ECU_CANDIDATE_NUMBER; i++)
        {
            const uint32_t canId {ECU_Candidate_List[i]};
            uint8_t type {convertObcProtocolType(CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN
                                                , CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS
                                                , canId)};
            if (type > static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD))
            {
                type = OBCEnum::OBCProtocolType::UNKNOWN;
                LOG_E("Type is out of range");
            }

            const android::sp<EcuUdsTransmission> transmission {new EcuUdsTransmission(*this, canId, type, TransmissionType::TYPE_EXISTENCE)};
            mEcuTransList[transmission->getTransmissionId()] = transmission;
            mTransmissioIdList.push(transmission->getTransmissionId());
        }

        //add Phase6 ECU CANFD transmission list
        constexpr uint32_t canId {0x18DBEFE1U};
        uint8_t type {static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)};
        const android::sp<EcuUdsTransmission> ecuPhase6CanFD {new EcuUdsTransmission(*this, canId, type, TransmissionType::TYPE_EXISTENCE)};
        mEcuTransList[ecuPhase6CanFD->getTransmissionId()] = ecuPhase6CanFD;
        mTransmissioIdList.push(ecuPhase6CanFD->getTransmissionId());


        //add Phase6 ECU CAN transmission list
        type = static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT);
        const android::sp<EcuUdsTransmission> ecuPhase6Can {new EcuUdsTransmission(*this, canId, type, TransmissionType::TYPE_EXISTENCE)};
        mEcuTransList[ecuPhase6Can->getTransmissionId()] = ecuPhase6Can;
        mTransmissioIdList.push(ecuPhase6Can->getTransmissionId());

        //start first transmission
        LOG_D("mEcuTransList size = %zu", mTransmissioIdList.size());
        if ( mTransmissioIdList.size() > 0U )
        {
            mCurrentTransmissionId = mTransmissioIdList.front();
            const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator it {mEcuTransList.find(mCurrentTransmissionId)};
            if (it != mEcuTransList.end())
            {
                it->second->connect();
            }
        }
        else
        {
            finishAcquisition();
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
}

void RemoteEcuInformation::testLoadEcuInformationFromFile() {

    /* Get file size*/
    int64_t size {0};
    ifstream file{ECU_INFORMATION_LIST_PATH_TEST.c_str(), ios::binary | ios::ate};
    size = file.tellg();
    file.close();
    LOG_I("CHECK ECU file test size: %lld", size);
    if(size > 1) {
        // Create an instance of EcuInformation struct
        std::ifstream ifs {};
        ifs.open(ECU_INFORMATION_LIST_PATH_TEST.c_str(), std::ifstream::in);
        if (ifs.is_open())
        {
            CommonDefine::EcuInformation ecuInfo {};
            m_EcuInformationList.clear();
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
                LOG_I("targetAddress: 0x%02x", tmp_targetAddress_int);
                LOG_I("canId: 0x%02x", tmp_canId_int);
                LOG_I("ecuActiveFlag: %d", tmp_ecuActiveFlag);
                LOG_I("commProtocol: %d", tmp_commProtocol);
                LOG_I("commType: %d", tmp_commType);
                LOG_I("diagPhase: %d", tmp_diagPhase);
            }
            ifs.close();
        }
        LOG_I("Check ecu list size: %zu", m_EcuInformationList.size());
    }
}

void RemoteEcuInformation::deactiveAllEcu() noexcept
{
    std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()};
    for (; it != m_EcuInformationList.end(); ++it)
    {
        it->setecuActiveFlag(false);
    }
}

void RemoteEcuInformation::onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)
{
    if ((mState == EcuState::ECU_RUNNING) || (mState == EcuState::ECU_SUSPEND_PENDING))
    {
        (void)mHandler->obtainMessage(MainHandler::CMD_RECEIVE_UDS_RESPONSE, responseEventInfo)->sendToTarget();
        (void)udsResponse;
    }
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

    if ((mState == EcuState::ECU_RUNNING) || (mState == EcuState::ECU_SUSPEND_PENDING))
    {
        LOG_I("onReceiveUDS connectId %u, canId = 0x%02x", connectId, canInfo->canId());
        
        const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
        if (itEcu != mEcuTransList.end())
        {
            const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
            if ((currentTransmission->getConnectId() == connectId) || (currentTransmission->checkfunctionRequest() == true))
            {          
                currentTransmission->stopTimeout();
                if ((responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_OK)
                    || ((responseEventInfo->errCode() == OBCEnum::OBCErrCode::OBC_NEGATIVE) && (udsResponse->getNRC() != 0x78U)))
                {
                    if (((udsResponse->getSID() == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE)) && (udsResponse->getSFID() == currentTransmission->getUdsReqSID()))
                        || (udsResponse->getSID() == (currentTransmission->getUdsReqSID() + static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_POSITIVE_RESPONSE_OFFSET))))
                    {
                        switch (currentTransmission->getTransmissionType())
                        {
                            case TransmissionType::TYPE_EXISTENCE:
                            {
                                handleExistenceCheckResponse(protocolType, canInfo, udsResponse);
                                break;
                            }
                            case TransmissionType::TYPE_DID_PHASE_5:
                            case TransmissionType::TYPE_DID_PHASE_6:
                            {
                                handleReadDidResponse(protocolType, canInfo, udsResponse);
                                break;
                            }
                            default:
                            {
                                LOG_I("incorrect transmission type: %u", currentTransmission->getTransmissionType());
                                break;
                            }
                        }
                        if (currentTransmission->checkfunctionRequest() == false)
                        {
                            currentTransmission->nextRequestIndex();
                            currentTransmission->sendUdsRequest();
                        }
                    }
                    else
                    {
                        onTransmissionTimeout();
                    }
                }
                else
                {
                    onTransmissionTimeout();
                }
            }
        }
        else
        {
            LOG_E("cannot find current transmission");
        }
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
            if ((it->getecuActiveFlag() == true) 
                && (it->getCommProtocol() == CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
                && (it->getTargetAddress() != 0x18DA2BE1U)) //DCM
            {
                LOG_I("ECU responded through CANFD => ignore CAN response");
            } else
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
            }
            break;
        }
    }
    if (it == m_EcuInformationList.end())
    {
        //the ECU was not in the list, add the ECU to the list
        LOG_I("ECU is not in the list, add new ecu");
        insertEcuInformation(ecuInformation);
    }
}

void RemoteEcuInformation::handleReadDidResponse(const uint8_t protocolType, const android::sp<OBCCanInfo> canInfo, const android::sp<UdsMessage> udsResponse)
{
    const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itTrans {mEcuTransList.find(mCurrentTransmissionId)};
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
    else if (itTrans == mEcuTransList.end())
    {
        LOG_E("TransmissionId not found: 0x%llx", mCurrentTransmissionId);
    }
    else
    {
        const uint8_t sid {udsResponse->getSID()};
        const uint8_t sfid {udsResponse->getSFID()};
        const android::sp<::Buffer> udsPayload {udsResponse->GetUdsPayload()};

        if (sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_SESSION_CONTROL))
        {
            LOG_I("Session control response sfid: 0x%x", sfid);
            if (sfid == static_cast<uint8_t>(EcuSession::SESSION_REMOTE))
            {
                itTrans->second->setSession(EcuSession::SESSION_REMOTE);
            } else
            {
                itTrans->second->setSession(EcuSession::SESSION_DEFAULT);
            }
        } else if (sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_PR_READ_DATA_BY_IDENTIFIER))
        {
            //update ECU sw/hw part number
            if ((udsPayload->data() != nullptr) && (udsPayload->size() > 1U)) //check did data is valid
            {
                const uint16_t did {static_cast<uint16_t>(static_cast<uint16_t>((static_cast<uint16_t>(sfid) << 8U)) | static_cast<uint16_t>(udsPayload->data()[0]))};
     
                if ((did == 0xF181U) || (did == 0xF188U))
                {
                    const uint32_t ecu_address{it->getTargetAddress()};
                    if (mEcuPartNumberMap.find(ecu_address) != mEcuPartNumberMap.end())
                    {
                        mEcuPartNumberMap[ecu_address].setSwPartNumber(std::string(&udsPayload->data()[1], &udsPayload->data()[1] + udsPayload->size() - 1U));
                    }
                    else
                    {
                        EcuPartNumber ecu_pn{};
                        ecu_pn.setSwPartNumber(std::string(&udsPayload->data()[1], &udsPayload->data()[1] + udsPayload->size() - 1U));
                        mEcuPartNumberMap[ecu_address] = ecu_pn;
                    }
                } else if ((did == 0xF191U) || (did == 0x0105U))
                {
                    const uint32_t ecu_address{it->getTargetAddress()};
                    if (mEcuPartNumberMap.find(ecu_address) != mEcuPartNumberMap.end())
                    {
                        mEcuPartNumberMap[ecu_address].setHwPartNumber(std::string(&udsPayload->data()[1], &udsPayload->data()[1] + udsPayload->size() - 1U));
                    }
                    else
                    {
                        EcuPartNumber ecu_pn{};
                        ecu_pn.setHwPartNumber(std::string(&udsPayload->data()[1], &udsPayload->data()[1] + udsPayload->size() - 1U));
                        mEcuPartNumberMap[ecu_address] = ecu_pn;
                    }
                } else {
                    LOG_E("DID not support: 0x%x", did);
                }
            } else {
                LOG_E("udsPayload invalid");
            }
        } else if (sid == static_cast<uint8_t>(UDS_RESPONSE_CODE::UDS_NEGATIVE_RESPONSE))
        {
            if ((udsPayload->data() != nullptr) && (udsPayload->size() > 0U)) //check uds message is valid
            {
                LOG_E("Negative response: sid: %x, nrc: %x", sfid, udsPayload->data()[0]);
                if (itTrans->second->getUdsReqSID() == static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL))
                {
                    if (itTrans->second->getUdsReqSFID() == static_cast<uint8_t>(EcuSession::SESSION_REMOTE))
                    {
                        itTrans->second->setSession(EcuSession::SESSION_FAIL_REMOTE);
                    } else
                    {
                        itTrans->second->setSession(EcuSession::SESSION_DEFAULT);
                    }
                }
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
    (void)itTrans;
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
        if ((trigger_type > DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN)
            && (trigger_type < DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX))
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

void RemoteEcuInformation::onRdgStop(const bool isStop) const noexcept {
    //obtain message to stop RDG
    const sp<sl::Message> msg {mHandler->obtainMessage(MainHandler::CMD_STOP_RDG, static_cast<int32_t>(isStop))};
    (void)msg->sendToTarget();
}

void RemoteEcuInformation::triggerEcuInfo(const uint32_t prio, const uint64_t colID, const DiagTrigger::DiagTriggerType trigType)
{
    LOG_I("Ecu information trigger");
    const uint32_t nextTriggerId {TriggerIDGenerator::getInstance().getNextId()};
    android::sp<DiagTrigger> pTrigger{new DiagTrigger(trigType,
                                                      prio,
                                                      DiagTrigger::DiagTriggerFunc::ECU_UPDATE_INFO,
                                                      nextTriggerId)};
    pTrigger->setCollectionId(colID);
    const std::pair<std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator, bool> ret {mSaveReq.emplace(nextTriggerId, pTrigger)};
    LOG_D("Check mSaveReq size: %zu", mSaveReq.size());
    if(!ret.second) {
        ret.first->second = pTrigger;
    }
    (void)mHandler->obtainMessage(MainHandler::CMD_REQUEST_TO_PRIORITY_CONTROL, pTrigger)->sendToTarget();
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
        CenterRequestEcuInformation EcuInformationReq{};
        const std::shared_ptr<CenterRequestJob> job {CollectionCondition::getInstance().getCenterRequestJob(mColId)};
        if ((job != nullptr) && (job->getMessageId() == MSG_ID_CENTERREQUESTECUINFORMATION))
        {
            
            const std::shared_ptr<google::protobuf::Message> message {job->getPayload()};
            if (message != nullptr)
            {
                const std::shared_ptr<CenterRequestEcuInformation> aCenterRequest { std::dynamic_pointer_cast<CenterRequestEcuInformation>(message)};
                if (aCenterRequest != nullptr)
                {
                    EcuInformationReq.CopyFrom(*aCenterRequest);
                }
            }
        }
        m_srvc_disregard_flag = EcuInformationReq.srvc_ac_disregard_flag();
    }
    else //routine trigger
    {
        m_srvc_disregard_flag = CollectionCondition::getInstance().getCollectionConditionEcuInformation()->srvc_ac_disregard_flag();
    }
    //get under repair status
    const uint8_t underRepairStatus{mApp.getUnderRepair()};

    const bool ret{(RDG_flag == 0x01U) && (IG_status == IG_STATUS_ON) && ((consent_status == true) || (m_srvc_disregard_flag == true)) && (underRepairStatus == 0x00U)};
    (void) IG_status;
    (void) consent_status;
    return ret;
}

void RemoteEcuInformation::loadEcuInfoListFromFile()
{
    std::ifstream ifs {};
    ifs.open(ECU_INFORMATION_LIST_PATH.c_str(), std::ifstream::in);
    if (ifs.is_open())
    {
        CommonDefine::EcuInformation ecuInfo {};
        char_t buf[sizeof(CommonDefine::EcuInformation) + 4092U + 4092U] {0}; //part number max size = 4095 UDS max - 1 SID - 2 DID

        //read first ecu
        (void)ifs.read(&buf[0], sizeof(buf));

        while (ifs.good())
        {
            //ecu information
            (void)memcpy(&ecuInfo, &buf[0], sizeof(CommonDefine::EcuInformation));

            //push to ecu list
            LOG_I("loaded ECU: address(0x%02x), activeFlag(%d), diagPhase(%d), protocol(%d)"
                , ecuInfo.getTargetAddress(), ecuInfo.getecuActiveFlag(), ecuInfo.getDiagPhase(), ecuInfo.getCommProtocol());
            m_EcuInformationList.push_back(ecuInfo);

            //sw part number
            const std::string swpn {&buf[0] + sizeof(CommonDefine::EcuInformation), 4092U};
            const size_t swpn_len {swpn.find('\0')};
            if (swpn_len == std::string::npos)
            {
                mEcuPartNumberMap[ecuInfo.getTargetAddress()].setSwPartNumber(swpn);
            }
            else
            {
                mEcuPartNumberMap[ecuInfo.getTargetAddress()].setSwPartNumber(swpn.substr(0U, swpn_len)); //trim
            }

            //hw part number
            const std::string hwpn {&buf[0] + sizeof(CommonDefine::EcuInformation) + 4092U, 4092U};
            const size_t hwpn_len {hwpn.find('\0')};
            if (hwpn_len == std::string::npos)
            {
                mEcuPartNumberMap[ecuInfo.getTargetAddress()].setHwPartNumber(hwpn);
            }
            else
            {
                mEcuPartNumberMap[ecuInfo.getTargetAddress()].setHwPartNumber(hwpn.substr(0U, hwpn_len)); //trim
            }

            //read next ecu
            (void)ifs.read(&buf[0], sizeof(buf));
        }

        ifs.close();
    }
    else
    {
        LOG_I("File open fail, add DCM to ECU information list as initial value");

        //add DCM information to list
        clearEcuInformationList();
        CommonDefine::EcuInformation dcmInfo {};
        dcmInfo.setCommProtocol(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN);
        dcmInfo.setCommType(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS);
        dcmInfo.setTargetAddress(0x18DA2BE1U);
        dcmInfo.setCanId(0x18DAE12BU);
        dcmInfo.setDiagPhase(CommonDefine::DiagPhase::DP_PHASE_6);
        dcmInfo.setecuActiveFlag(true);
        updateEcuInformationList(dcmInfo);

        //store to eMMC
        saveEcuInfoListToFile();
    }
}

void RemoteEcuInformation::saveEcuInfoListToFile()
{
    ofstream ofs {};
    ofs.open(ECU_INFORMATION_LIST_PATH.c_str(), std::ofstream::out);
    if (ofs.is_open())
    {
        CommonDefine::EcuInformation ecuInfo {};
        std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()};
        for (; it != m_EcuInformationList.end(); ++it)
        {
            ecuInfo = *it;
            char_t buf[sizeof(CommonDefine::EcuInformation) + 4092U + 4092U] {0}; //part number max size = 4095 UDS max - 1 SID - 2 DID
            
            //ECU information
            (void)memcpy(&buf[0], &ecuInfo, sizeof(CommonDefine::EcuInformation));
            //sw part number
            const std::string swpn {mEcuPartNumberMap[ecuInfo.getTargetAddress()].getSwPartNumber()};
            const size_t swpn_len {(swpn.length() > 4092U) ? 4092U : swpn.length()};
            (void)memcpy(&buf[0] + sizeof(CommonDefine::EcuInformation), swpn.c_str(), swpn_len);
            //write hw part number
            const std::string hwpn {mEcuPartNumberMap[ecuInfo.getTargetAddress()].getHwPartNumber()};
            const size_t hwpn_len {(hwpn.length() > 4092U) ? 4092U : hwpn.length()};
            (void)memcpy(&buf[0] + sizeof(CommonDefine::EcuInformation) + 4092U, hwpn.c_str(), hwpn_len);

            (void)ofs.write(&buf[0], sizeof(buf));
        }
        (void)ofs.flush();
        ofs.close();
        sync();
    } else
    {
        LOG_E("File open fail");
    }
}

void RemoteEcuInformation::startDidAcquisition()
{
    mEcuTransList.clear();
    mTransmissioIdList = {};
    mEcuPartNumberMap.clear();

    std::list<CommonDefine::EcuInformation>::iterator it{};
    for (it = m_EcuInformationList.begin(); it != m_EcuInformationList.end(); ++it)
    {
        if (it->getecuActiveFlag() == true)
        {
            //clear saved DID
            mEcuPartNumberMap[it->getTargetAddress()].setSwPartNumber(std::string(""));
            mEcuPartNumberMap[it->getTargetAddress()].setHwPartNumber(std::string(""));

            //add read DID requests to transmission list
            if (it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5)
            {
                uint8_t type {convertObcProtocolType(it->getCommProtocol(), it->getCommType(), it->getTargetAddress())};
                if (type > static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD))
                {
                    type = OBCEnum::OBCProtocolType::UNKNOWN;
                    LOG_E("Type is out of range");
                }
                const android::sp<EcuUdsTransmission> transmission {new EcuUdsTransmission(*this, it->getTargetAddress(), type, TransmissionType::TYPE_DID_PHASE_5)};
                mEcuTransList[transmission->getTransmissionId()] = transmission;
                mTransmissioIdList.push(transmission->getTransmissionId());
            } else if (it->getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_6)
            {
                uint8_t type  {convertObcProtocolType(it->getCommProtocol(), it->getCommType(), it->getTargetAddress())};
                if (type > static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD))
                {
                    type = OBCEnum::OBCProtocolType::UNKNOWN;
                    LOG_E("Type is out of range");
                }
                const android::sp<EcuUdsTransmission> transmission {new EcuUdsTransmission(*this, it->getTargetAddress(), type, TransmissionType::TYPE_DID_PHASE_6)};
                mEcuTransList[transmission->getTransmissionId()] = transmission;
                mTransmissioIdList.push(transmission->getTransmissionId());
            }
            else
            {
                LOG_E("ECU phase4, target address: 0x%x", it->getTargetAddress());
                mEcuPartNumberMap[it->getTargetAddress()].setSwPartNumber(std::string(""));
                mEcuPartNumberMap[it->getTargetAddress()].setHwPartNumber(std::string(""));
            }
        }
    }

    //start first transmission
    LOG_D("mEcuTransList size = %zu", mTransmissioIdList.size());
    if ( mTransmissioIdList.size() > 0U )
    {
        mCurrentTransmissionId = mTransmissioIdList.front();
        const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
        if (itEcu != mEcuTransList.end())
        {
            itEcu->second->connect();
        }
    }
    else
    {
        finishAcquisition();
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
            if ((mState == EcuState::ECU_SUSPENDING) || (mState == EcuState::ECU_SUSPEND_PENDING))
            {
                mState = EcuState::ECU_RUNNING;

                //send next uds message
                LOG_D("mEcuTransList size = %zu", mTransmissioIdList.size());
                if ( mTransmissioIdList.size() > 0U )
                {
                    mCurrentTransmissionId = mTransmissioIdList.front();
                    const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
                    if (itEcu != mEcuTransList.end())
                    {
                        itEcu->second->connect();
                    }
                }
                else
                {
                    finishAcquisition();
                }
            }
            else
            {
                LOG_I("TRIGGER_PROCESSING");
                mTriggerId = pTriggerId;
                mTriggerType = it->second->getType();
                mColId = it->second->getCollectionID();
                mPriority = it->second->getPriority();
                (void)mHandler->obtainMessage(MainHandler::CMD_START_ECU_INFORMATION)->sendToTarget();
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_SUSPENDED:
        {
            LOG_I("TRIGGER_SUSPENDED");
            if (mState == EcuState::ECU_RUNNING)
            {
                mState = EcuState::ECU_SUSPEND_PENDING;

                //disconnect ECU
                const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
                if (itEcu != mEcuTransList.end())
                {
                    const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
                    if (currentTransmission->checkfunctionRequest() == false) //RDG30-R-0409
                    {
                        currentTransmission->disconnect();
                    }
                }
                
                OnboardclientAdapter::getInstance()->ReleaseObcResource();
                if (mTriggerId <= static_cast<uint32_t>(INT32_MAX))
                {
                    const DiagTrigger::DiagTriggerType tmp_type{it->second->getType()};
                    if((tmp_type >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
                            (tmp_type <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)) {
                        LOG_I("Trigger type is valid");
                    } else {
                       LOG_I("Trigger type is out of range");
                    }
                    PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), tmp_type);
                }
                else
                {
                    LOG_E("mTriggerId is out of range INT32");
                }
            }
            break;
        }
        case DiagTrigger::DiagTriggerState::TRIGGER_DISCARDED:
        {
            LOG_I("TRIGGER_DISCARDED");

            mState = EcuState::ECU_IDLE;

            //disconnect ECU
            const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
            if (itEcu != mEcuTransList.end())
            {
                const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
                currentTransmission->disconnect();
            }
            
            OnboardclientAdapter::getInstance()->ReleaseObcResource();
            
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

RemoteEcuInformation::EcuUdsTransmission::EcuUdsTransmission( RemoteEcuInformation& ecu, const uint32_t canId, const uint8_t protocolType, const TransmissionType transType)
: android::RefBase()
, mECUInfo(ecu)
, mTimerHandler(ecu)
, mCanId(canId)
, mProtocolType(protocolType)
, mConnectId(0U)
, mTransmissionId(0U)
, mIsFunctionalRequest(false)
, mTimeOut(&mTimerHandler, TimerHandler::ID_TRANSMISSION_TIMEOUT)
, mUdsReqList()
, mCurrentRequestIndex(0U)
, mTransmissionType(transType)
, mSession(EcuSession::SESSION_DEFAULT)
{
    switch (mTransmissionType)
    {
        case TransmissionType::TYPE_EXISTENCE:
        {
            //existence check request
            const android::sp<UdsMessage> spUdsReq {new UdsMessage()};
            spUdsReq->setSID(static_cast<uint8_t>(UDS_SID::SID_3E_TESTER_PRESENT));
            spUdsReq->setSFID(0x00U);
            mUdsReqList.push_back(spUdsReq);
            break;
        }
        case TransmissionType::TYPE_DID_PHASE_5:
        {
            //remote session
            const android::sp<UdsMessage> spUdsReq_remote {new UdsMessage()};
            spUdsReq_remote->setSID(static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL));
            spUdsReq_remote->setSFID(static_cast<uint8_t>(EcuSession::SESSION_REMOTE));
            mUdsReqList.push_back(spUdsReq_remote);
            
            //SWPN request
            const android::sp<UdsMessage> spUdsReq_SWPN {new UdsMessage()};
            spUdsReq_SWPN->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
            spUdsReq_SWPN->setSFID(0xF1U);
            constexpr uint8_t temp_SWPN{0x81U};
            spUdsReq_SWPN->GetUdsPayload()->setTo(&temp_SWPN, 1);
            mUdsReqList.push_back(spUdsReq_SWPN);

            //HWPN request
            const android::sp<UdsMessage> spUdsReq_HWPN {new UdsMessage()};
            spUdsReq_HWPN->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
            spUdsReq_HWPN->setSFID(0x01U);
            constexpr uint8_t temp_HWPN{0x05U};
            spUdsReq_HWPN->GetUdsPayload()->setTo(&temp_HWPN, 1);
            mUdsReqList.push_back(spUdsReq_HWPN);

            //default session
            const android::sp<UdsMessage> spUdsReq_default {new UdsMessage()};
            spUdsReq_default->setSID(static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL));
            spUdsReq_default->setSFID(static_cast<uint8_t>(EcuSession::SESSION_DEFAULT));
            mUdsReqList.push_back(spUdsReq_default);

            break;
        }
        case TransmissionType::TYPE_DID_PHASE_6:
        {
            //SWPN request
            const android::sp<UdsMessage> spUdsReq_SWPN {new UdsMessage()};
            spUdsReq_SWPN->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
            spUdsReq_SWPN->setSFID(0xF1U);
            constexpr uint8_t temp_SWPN{0x88U};
            spUdsReq_SWPN->GetUdsPayload()->setTo(&temp_SWPN, 1);
            mUdsReqList.push_back(spUdsReq_SWPN);

            //HWPN request
            const android::sp<UdsMessage> spUdsReq_HWPN {new UdsMessage()};
            spUdsReq_HWPN->setSID(static_cast<uint8_t>(UDS_SID::SID_22_READ_DATA_BY_IDENTIFIER));
            spUdsReq_HWPN->setSFID(0xF1U);
            constexpr uint8_t temp_HWPN{0x91U};
            spUdsReq_HWPN->GetUdsPayload()->setTo(&temp_HWPN, 1);
            mUdsReqList.push_back(spUdsReq_HWPN);
            break;
        }
        default:
        {
            break;
        }
    }

    if (mCanId == 0x18DBEFE1U)
    {
        mIsFunctionalRequest = true;
    }
    mTransmissionId = ((static_cast<uint64_t>(this->mCanId) & 0xFFFFFFFFU) << 32U)
                    | ((static_cast<uint64_t>(this->mProtocolType) & 0xFFU) << 8U)
                    | static_cast<uint64_t>(this->mTransmissionType);
}

void RemoteEcuInformation::EcuUdsTransmission::connect()
{
    LOG_I("Start connect, CanId = 0x%02x, protocol = %d", this->mCanId, this->mProtocolType);    
    const android::sp<OBCTransportInfo> obcTransportInfo {new OBCTransportInfo()};
    OBCCanInfo canInfo {};
    canInfo.canId() = this->mCanId;
    std::stringstream sstream {};
    sstream << &std::hex << 0U;
    canInfo.nTa().push_back(sstream.str());
    obcTransportInfo->setData(this->mProtocolType, canInfo, false, 0U);
    mTimeOut.setDuration(TRANSMISSION_TIME_OUT_DURATION, 0U);
    const android::sp<OBCConnectInfo> obj {new OBCConnectInfo()};
    const error_t res {OnboardclientAdapter::getInstance()->connect(obcTransportInfo, APP_NAME, obj)};
    (void) res;
    
    tOBCConnectInfo info;
    obj->setDataFormat(info);
    if (info.response != OBCEnum::OBCErrCode::OBC_OK)
    {
        LOG_E("can't connect to canid = 0x%02x", this->mCanId);
        (void)mECUInfo.mHandler->obtainMessage(MainHandler::CMD_ECUINFO_TRANSMISSION_FINISH)->sendToTarget();
    } else 
    {
        LOG_D("connect success transmissionID = 0x%llx", this->mTransmissionId);
        this->mConnectId = info.connectId;
        OnboardclientAdapter::getInstance()->TakeObcResource();
        this->sendUdsRequest();
    }
}

void RemoteEcuInformation::EcuUdsTransmission::stopTimeout()
{
    this->mTimeOut.stop();
}

void RemoteEcuInformation::EcuUdsTransmission::disconnect()
{
    (void)OnboardclientAdapter::getInstance()->disconnectECU(this->mConnectId);
    // TMCDCMTF-35635
    // OnboardclientAdapter::getInstance()->SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
    mSession = EcuSession::SESSION_DEFAULT;
    mCurrentRequestIndex = 0U;
    mECUInfo.finishCurrentTransmission();
}

void RemoteEcuInformation::EcuUdsTransmission::sendUdsRequest()
{
    if ((mCurrentRequestIndex < mUdsReqList.size()) && (mSession != EcuSession::SESSION_FAIL_REMOTE))
    {
        const android::sp<::Buffer> udsData {this->mUdsReqList[mCurrentRequestIndex]->ToUdsData()};

        const uint8_t err {OnboardclientAdapter::getInstance()->sendUdsData(this->mConnectId, udsData)};
        if (err != 0x00U) //E_OK
        {
            LOG_E("SendUdsData for transmissionId 0x%llx, index: %zu error = %d", this->mTransmissionId, this->mCurrentRequestIndex, err);
            this->nextRequestIndex();
            if (mCurrentRequestIndex >= mUdsReqList.size())
            {
                this->disconnect();
            } else
            {
                this->sendUdsRequest();
            }
        } else
        {
            LOG_I("SendUdsData for transmissionId 0x%llx, index: %zu success", this->mTransmissionId, this->mCurrentRequestIndex);
            mTimeOut.start();
        }
    } else
    {
        this->disconnect();
    }
}

uint64_t RemoteEcuInformation::EcuUdsTransmission::getTransmissionId() const noexcept
{
    return mTransmissionId;
}

uint16_t RemoteEcuInformation::EcuUdsTransmission::getConnectId() const noexcept
{
    return mConnectId;
}

uint8_t RemoteEcuInformation::EcuUdsTransmission::getUdsReqSID() const noexcept
{
    uint8_t res {0x00U};
    if (mCurrentRequestIndex < mUdsReqList.size())
    {
        res = mUdsReqList[mCurrentRequestIndex]->getSID();
    }
    return res;
}

uint8_t RemoteEcuInformation::EcuUdsTransmission::getUdsReqSFID() const noexcept
{
    uint8_t res {0x00U};
    if (mCurrentRequestIndex < mUdsReqList.size())
    {
        res = mUdsReqList[mCurrentRequestIndex]->getSFID();
    }
    return res;
}

bool RemoteEcuInformation::EcuUdsTransmission::checkfunctionRequest() const noexcept
{
    return mIsFunctionalRequest;
}

void RemoteEcuInformation::finishCurrentTransmission(void)
{
    if (mState == EcuState::ECU_SUSPEND_PENDING)
    {
        mState = EcuState::ECU_SUSPENDING;
        // TMCDCMTF-35635
        OnboardclientAdapter::getInstance()->ReleaseObcResource();
        if(mTriggerId<=static_cast<uint32_t>(INT32_MAX))
        {
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), mTriggerType);
        }
        else{
            LOG_E("mTriggerId is out of range INT32");
        }
    } else if (mState == EcuState::ECU_RUNNING)
    {
        LOG_I("finish transmission 0x%llx, mTransmissioIdList size = %zu", mCurrentTransmissionId, mTransmissioIdList.size());
        if ( mTransmissioIdList.size() > 1U ) 
        {
            mTransmissioIdList.pop();
            mCurrentTransmissionId = mTransmissioIdList.front();
            const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator it {mEcuTransList.find(mCurrentTransmissionId)};
            if (it != mEcuTransList.end())
            {
                it->second->connect();
            }
        } else 
        {
            TransmissionType transType {TransmissionType::TYPE_NONE};
            if(mTransmissioIdList.empty() != true)
            {
                const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator it {mEcuTransList.find(mTransmissioIdList.front())};
                if (it != mEcuTransList.end())
                {
                    transType = it->second->getTransmissionType();
                }
                mTransmissioIdList.pop();
            }
            if (transType == TransmissionType::TYPE_EXISTENCE)
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
            } else if ((transType == TransmissionType::TYPE_DID_PHASE_5) || (transType == TransmissionType::TYPE_DID_PHASE_6))
            {
                //Release OBC resource
                // TMCDCMTF-35635
                // OnboardclientAdapter::getInstance()->ReleaseObcResource();

                //Save to NVM (RDG30-R-0949)
                saveEcuInfoListToFile();

                //save last update time
                const int64_t mTempTime {CommonUtils::getCurrentAcquisiteTime()};
                if (mTempTime > 0)
                {   
                    mLastUpdateTime = static_cast<uint64_t>(mTempTime);
                } else
                {
                    mLastUpdateTime = 0U;
                }

                //Upload
                (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
            } else
            {
                LOG_E("invalid transmission type");
            }
        }
    } else
    {
        LOG_E("invalid state %u", mState);
    }
}

void RemoteEcuInformation::onTransmissionTimeout(void)
{
    (void)mHandler->obtainMessage(MainHandler::CMD_TRANSMISSION_TIMEOUT)->sendToTarget();
}

void RemoteEcuInformation::handleTransmissionTimeout()
{
    LOG_I("Event Transmission Timeout, currentTrans = 0x%llx", mCurrentTransmissionId);
    const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator it {mEcuTransList.find(mCurrentTransmissionId)};
    if (it != mEcuTransList.end())
    {
        if (it->second->getUdsReqSID() == static_cast<uint8_t>(UDS_SID::SID_10_SESSION_CONTROL))
        {
            if (it->second->getUdsReqSFID() == static_cast<uint8_t>(EcuSession::SESSION_REMOTE))
            {
                it->second->setSession(EcuSession::SESSION_FAIL_REMOTE);
            } else
            {
                it->second->setSession(EcuSession::SESSION_DEFAULT);
            }
        }
        it->second->nextRequestIndex();
        it->second->sendUdsRequest();
    }
}

void RemoteEcuInformation::handleStopRDG() {
    if ((mState == EcuState::ECU_RUNNING) || (mState == EcuState::ECU_SUSPEND_PENDING))
    {
        mState = EcuState::ECU_IDLE;
        CollectionCondition::getInstance().onFinishCenterRequestJob(mColId);
        //disconnect ECU
        const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
        if (itEcu != mEcuTransList.end())
        {
            const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
            currentTransmission->disconnect();
        } else {
            LOG_E("mCurrentTransmissionId is not found in mEcuTransList");
        }
    } else {
        LOG_I("ECU function is not running");
    }
}

void RemoteEcuInformation::makeUploadRequest()
{
    if (mIsNeedUploadErrorData == true)
    {
        mEcuUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
        mEcuUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
        mEcuUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(CommonUtils::getGeodesyInfo());
        mEcuUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
        mEcuUploadErrorData->mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());
        mEcuUploadErrorData->mutable_rdg_common_request_header()->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA);
        mEcuUploadErrorData->mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ERROR_DATA, UploadManager::getInstance()->getCounterMessage()));
        mEcuUploadErrorData->set_collection_condition_id(mColId);
        mEcuUploadErrorData->set_counter_value(UploadManager::getInstance()->getCounterValue());
        mEcuUploadErrorData->set_data_creation_date(mDataCreationDate);
        mEcuUploadErrorData->set_obd2_installed_flag(mApp.getOBDStatus());
        mEcuUploadErrorData->set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
        mEcuUploadErrorData->set_function_type(UploadErrorDataRequestFunctionType::UploadErrorDataRequest_FunctionType_FT_ECU_INFORMATION_UPDATE_REQUEST);
        mEcuUploadErrorData->set_trigger_type(vccomif::rdg::v1::interfaces::TriggerType::TT_OTHER_TRIGGER);

        /*Save upload data to file*/ 
        const uint32_t uploadId {UploadManager::getInstance()->genRequestId()};
        const uint64_t uploadCount {UploadManager::getInstance()->genCountUpload()};
        std::string file_dir {std::to_string(uploadCount)};
        (void)file_dir.append("_UploadErrorDataRequest.dat");
        uint32_t fileSize{0U};
        error_t bSaved{E_ERROR};
        const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
        if (region == LGE_REGION::LGE_REGION_CN)
        {
            bSaved = DataModel<UploadErrorDataRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG160, file_dir, *mEcuUploadErrorData, fileSize);
        }
        else
        {
            fileSize = mEcuUploadErrorData->ByteSizeLong();
            bSaved = DataModel<UploadErrorDataRequest>::saveUpload(file_dir, *mEcuUploadErrorData);
        }
        if (bSaved == E_OK)
        {
            const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
            // Self-Diag
            DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
            const android::sp<UploadTask> task {new UploadTask(uploadId)};
            task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG160);
            task->setUploadPatch(file_dir);
            task->setUploadId(static_cast<uint64_t>(uploadId));
            /*Set priority*/
            task->setUploadPrio(mPriority);
            task->setSrvcAcFlag(m_srvc_disregard_flag);
            task->setFileSize(static_cast<uint64_t>(fileSize));
            UploadManager::getInstance()->requestUploadTask(task);
        }
        else
        {
            LOG_E("Save upload error data file fail");
        }

        //print data
        LOG_I("error upload data:");
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        std::string log_str{};
        (void)google::protobuf::util::MessageToJsonString(*mEcuUploadErrorData, &log_str, option);
        printData(log_str);
        (void) uploadId;
        (void) fileSize;
    } else
    {

        //package data
        vccomif::rdg::v1::interfaces::UploadEcuInformationRequest uploadRequest{};
        uploadRequest.mutable_rdg_common_request_header()->mutable_app_common_header()->set_text_version(HttpManagerAdapter::getInstance()->getProtoTextVersion());
        uploadRequest.mutable_rdg_common_request_header()->mutable_app_common_header()->set_electronic_pf(EPF_19EPF);
        uploadRequest.mutable_rdg_common_request_header()->mutable_app_common_header()->set_geodesy_information(CommonUtils::getGeodesyInfo());
        uploadRequest.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_hours(CommonUtils::getTimeZoneOffsetHour());
        uploadRequest.mutable_rdg_common_request_header()->mutable_app_common_header()->mutable_time_zone_offset()->set_minutes(CommonUtils::getTimeZoneOffsetMinutes());
        uploadRequest.mutable_rdg_common_request_header()->set_interface_type(vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ECU_INFORMATION);
        uploadRequest.mutable_rdg_common_request_header()->set_message_id(CommonUtils::setUploadMessId(RequestHeaderInterfaceType::RdgCommonRequestHeader_InterfaceType_IT_UPLOAD_ECU_INFORMATION, UploadManager::getInstance()->getCounterMessage()));
        uploadRequest.set_collection_condition_id(mColId);
        uploadRequest.set_counter_value(UploadManager::getInstance()->getCounterValue()); 
        uploadRequest.set_data_creation_date(mDataCreationDate);
        uploadRequest.set_obd2_installed_flag(mApp.getOBDStatus());
        uploadRequest.set_under_repair_flag((mApp.getUnderRepair() == 1U) ? true : false);
        for (std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()}; it != m_EcuInformationList.end(); ++it)
        {
            vccomif::rdg::v1::interfaces::UploadEcuInformationRequest_EcuInformation* const ecuInformation {uploadRequest.add_ecu_information()};
            ecuInformation->set_last_update_date(mLastUpdateTime);
            const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol protocolType{it->getCommProtocol()};
            ecuInformation->mutable_ecu_address_information()->set_communication_protocol(protocolType);
            if(protocolType != vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol::EcuAddressInformation_CommunicationProtocol_CP_CAN_FD)
            {
                vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType{it->getCommType()};
                if ((commType < vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_11_BITS) ||
                    (commType > vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_CAN_ID_29_BITS))
                {
                    commType = vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN;
                }
                ecuInformation->mutable_ecu_address_information()->set_communication_type(commType);
            }

            ecuInformation->mutable_ecu_address_information()->set_target_address(it->getTargetAddress());
            const CommonDefine::DiagPhase diagPhase{it->getDiagPhase()};
            if((diagPhase >= CommonDefine::DiagPhase::DP_UNKNOW) && (diagPhase <= CommonDefine::DiagPhase::DP_PHASE_6))
            {
                ecuInformation->set_diagnostics_phase(static_cast<vccomif::rdg::v1::interfaces::DiagnosticsPhase>(diagPhase));
            } else {
                LOG_E("Unknown diag phase");
            }
            ecuInformation->set_ecu_active_flag(it->getecuActiveFlag() == true ? true : false);
            const uint32_t ecu_address{it->getTargetAddress()};
            if (mEcuPartNumberMap.find(ecu_address) != mEcuPartNumberMap.end())
            {
                ecuInformation->set_hardware_part_number(mEcuPartNumberMap[ecu_address].getHwPartNumber());
                ecuInformation->set_software_part_number(mEcuPartNumberMap[ecu_address].getSwPartNumber());
            }
        }

        /*Save upload data to file*/ 
        const uint32_t uploadId {UploadManager::getInstance()->genRequestId()};
        const uint64_t uploadCount {UploadManager::getInstance()->genCountUpload()};
        std::string file_dir {std::to_string(uploadCount)};
        (void)file_dir.append("_UploadEcuDataRequest.dat");
        uint32_t fileSize{0U};
        error_t bStored{E_OK};
        const uint8_t region{RegionManagerAdapter::getInstance()->getNation()};
        if (region == LGE_REGION::LGE_REGION_CN)
        {
            bStored = DataModel<UploadEcuInformationRequest>::MakeEncryptRequestMsg(GRPC_IF_TYPE::DCIF_RDG100, file_dir, uploadRequest, fileSize);
        }
        else
        {
            fileSize = uploadRequest.ByteSizeLong();
            bStored = DataModel<UploadEcuInformationRequest>::saveUpload(file_dir, uploadRequest);
        }
        if (bStored == E_OK)
        {
            const uint8_t operation{CommonUtils::getOperation(mTriggerType)};
            // Self-Diag
            DiagManagerAdapter::getInstance()->selfDiagSuccessCreateFile(operation);
            const android::sp<UploadTask> task{new UploadTask(uploadId)};
            task->setUploadFileType(GRPC_IF_TYPE::DCIF_RDG100);
            task->setUploadPatch(file_dir);
            task->setUploadId(static_cast<uint64_t>(uploadId));
            /*Set priority*/
            task->setUploadPrio(mPriority);
            task->setSrvcAcFlag(m_srvc_disregard_flag);
            task->setFileSize(static_cast<uint64_t>(fileSize));
            UploadManager::getInstance()->requestUploadTask(task);
        }
        else
        {
            LOG_E("Save upload ECU data file fail");            
        }
        //print data
        LOG_I("ECU upload data:");
        google::protobuf::util::JsonOptions option{};
        option.always_print_primitive_fields = true;
        option.preserve_proto_field_names = true;
        std::string log_str{};
        (void)google::protobuf::util::MessageToJsonString(uploadRequest, &log_str, option);
        printData(log_str);
        (void) uploadId;
        (void) fileSize;
    }
    if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
    {
        const int64_t current_time {CommonUtils::getCurrentAcquisiteTime()};
        LOG_I("Collection ID: %llu - Last complete time: %lld", mColId, current_time);
        if (current_time > 0)
        {
            mApp.notifyLastOpComplTime(mColId, current_time, mIsNeedUploadErrorData == false);
        }
    }

    finishAcquisition();
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
                const uint16_t CanId{static_cast<uint16_t>((targetAddress >> 16U) & 0xFFFFU)};
                if (CanId == 0x0770U)
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
    std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()};
    for (; it != m_EcuInformationList.end(); ++it)
    {
        if (it->getTargetAddress() == targetAddress)
        {
            break;
        }
    }
    // RDG30-R-1267
    if (it != m_EcuInformationList.end())
    {
        protocol = it->getCommProtocol();
        type = it->getCommType();
    }
    // Convert OBCProtocolType of OBC spec
    oBCProtocolType = convertObcProtocolType(protocol, type, targetAddress);
    LOG_D("Get communication protocol information success, type = %d", static_cast<uint8_t>(oBCProtocolType));
    return oBCProtocolType;
}

CommunicationType RemoteEcuInformation::getCommType(const uint32_t canId)
{
    CommunicationType type {CommunicationType::EcuAddressInformation_CommunicationType_CT_UNKNOWN};
    std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()};
    for (; it != m_EcuInformationList.end(); ++it)
    {
        if(it->getCanId() == canId)
        {
            type = it->getCommType();
            break;
        }
    }
    LOG_V("[getCommType] CAN ID: 0x%02x, commType: %d", canId, type);
    return type;
}

uint32_t RemoteEcuInformation::getCanId(const uint32_t targetAddress)
{
    uint32_t canId {0U};

    std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()};
    for (; it != m_EcuInformationList.end(); ++it)
    {
        if(it->getTargetAddress() == targetAddress)
        {
            canId = it->getCanId();
            LOG_V("[getCanId] canId: %d", canId);
            break;
        }
    }

    return canId;    
}

bool RemoteEcuInformation::isEcuExisted(const uint32_t targetAddress) noexcept
{
    bool isExisted {false};
    std::list<CommonDefine::EcuInformation>::iterator it {m_EcuInformationList.begin()};
    for (; it != m_EcuInformationList.end(); ++it)
    {
        if(it->getTargetAddress() == targetAddress)
        {
            isExisted = true;
            break;
        }
    }
    return isExisted;
}

void RemoteEcuInformation::onReceiveIG(const bool status) const
{
    (void)mHandler->obtainMessage(MainHandler::CMD_CHANGE_IG_STATUS, static_cast<int32_t>(status))->sendToTarget();
}

void RemoteEcuInformation::changeIGStatus(const bool status)
{
    LOG_I("changeIGStatus = %s", status ? "IGN_ON" : "IGN_OFF");
    if ((status == false) && ((mState == EcuState::ECU_RUNNING) || (mState == EcuState::ECU_SUSPEND_PENDING)))
    {
        mState = EcuState::ECU_IDLE;

        //disconnect ECU
        const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
        if (itEcu != mEcuTransList.end())
        {
            const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
            currentTransmission->disconnect();
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
    mEcuUploadErrorData = std::shared_ptr<UploadErrorDataRequest>(new UploadErrorDataRequest());
    if (mEcuUploadErrorData != nullptr)
    {
        int32_t abortCode{0};
        if (DiagManagerAdapter::getInstance()->getRDGFlag() == 0x00U)
        {
            abortCode = 1;
            mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_REQUEST_ERROR_UNPROVIDED_VEHICLE);
            mIsNeedUploadErrorData = true;
            (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
        } else if (PowerManagerAdapter::getInstance()->getIgnitionStatus() != IG_STATUS::IG_STATUS_ON)
        {
            abortCode = 2;
            mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_VEHICLE_ERROR_POWER_CONDITION);
            mIsNeedUploadErrorData = true;
            (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
        } else if (mApp.getUnderRepair() == 0x01U)
        {
            abortCode = 3;
            mEcuUploadErrorData->set_response_code(RdgProtoInterface::ResponseCode::RC_OBE_ERROR_UNDER_REPAIR);
            mIsNeedUploadErrorData = true;
            (void)mHandler->obtainMessage(MainHandler::CMD_MAKE_UPLOAD_REQUEST)->sendToTarget();
        } else
        {
            abortCode = 4;
            if (mTriggerType == DiagTrigger::DiagTriggerType::ROUTINE_TRIGGER)
            {
                const int64_t current_time {CommonUtils::getCurrentAcquisiteTime()};
                if (current_time > 0)
                {
                    mApp.notifyLastOpComplTime(mColId, current_time, false);
                }
            }
            finishAcquisition();
        }
        LOG_E("ECU abortCode: %d", abortCode);
    } else {
        LOG_E("mEcuUploadErrorData is null");
    }
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
        if (((mState == EcuState::ECU_RUNNING) || (mState == EcuState::ECU_SUSPEND_PENDING)) && (status == 1))
        {
            mState = EcuState::ECU_IDLE;
            
            //disconnect ECU
            const std::unordered_map<uint64_t, android::sp<EcuUdsTransmission>>::iterator itEcu {mEcuTransList.find(mCurrentTransmissionId)};
            if (itEcu != mEcuTransList.end())
            {
                const android::sp<EcuUdsTransmission> currentTransmission {itEcu->second};
                currentTransmission->disconnect();
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

void RemoteEcuInformation::finishAcquisition()
{
    LOG_I("Finish Ecu info Acquisition triggerID: %d", mTriggerId);
    // TMCDCMTF-35635
    OnboardclientAdapter::getInstance()->ReleaseObcResource();
    const std::unordered_map<uint32_t, android::sp<DiagTrigger>>::iterator it {mSaveReq.find(mTriggerId)};
    if (it != mSaveReq.end())
    {
        CollectionCondition::getInstance().onFinishCenterRequestJob(mColId);
        if (mTriggerId <= static_cast<uint32_t>(INT32_MAX))
        {
            const DiagTrigger::DiagTriggerType tmp_type {it->second->getType()};
            if((tmp_type >= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MIN) &&
                    (tmp_type <= DiagTrigger::DiagTriggerType::DIAG_TRIGGER_TYPE_MAX)) {
                LOG_I("Trigger type is valid");
            } else {
                LOG_I("Trigger type is out of range");
            }
            PriorityControl::getInstance()->notifyTriggerProcessDone(static_cast<int32_t>(mTriggerId), tmp_type);
        }
        else
        {
            LOG_E("mTriggerId is out of range INT32");
        }
        (void)mSaveReq.erase(it);
        mEcuTransList.clear();
        mTransmissioIdList = {};
        mState = EcuState::ECU_IDLE;
        mIsNeedUploadErrorData = false;
        mEcuPartNumberMap.clear();
    }
    else
    {
        LOG_D("Can not find triggerId: %d in mSaveReq", mTriggerId);
    }
}

void RemoteEcuInformation::testSaveEcuInformationToFile()
{
    saveEcuInfoListToFile();
}
}
