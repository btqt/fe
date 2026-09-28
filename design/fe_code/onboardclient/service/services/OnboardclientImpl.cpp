#include <future>
#include "OnboardclientImpl.h"
#include "OnboardclientTxHandler.h"
#include "CommunicationManagerAdapter.h"
namespace OBC
{
    OnboardclientImpl *OnboardclientImpl::mOnboardclientImpl{nullptr};
    OnboardclientImpl::OnboardclientImpl(/* args */)
    {
        mOnboardclientImpl = this;
        attemptTransErr = 0U;
        attemptBusyErr = 0U;
        isProcessing = false;
        currentProcessingId = 0U;
        RECEIVE_QUEUE_MAX_SIZE = 3276800U; // 64kb * 50
    }

    OnboardclientImpl *OnboardclientImpl::getInstance(void)
    {
        if (mOnboardclientImpl == nullptr)
        {
            mOnboardclientImpl = new OnboardclientImpl();
        }
        return mOnboardclientImpl;
    }

    uint16_t OnboardclientImpl::makeConnectId(uint8_t protocolType, uint32_t canId)
    {
        uint16_t connectID{0U};
        std::random_device rd{};
        std::mt19937 gen{rd()};
        std::uniform_int_distribution<uint16_t> dis{std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max()};
        connectID = dis(gen);
        (void)mpConnectedCAN.emplace(connectID, std::pair<uint8_t, uint32_t>(protocolType, canId));
        (void)mpCanIdConnectedId.emplace(canId, std::pair<uint8_t, uint32_t>(protocolType, connectID));
        LOGI("connectID: %u, protocol type: %u, Can ID: 0x%02X", connectID, protocolType, canId);
        return connectID;
    }

    uint8_t OnboardclientImpl::getConnectedId(uint16_t &connectId, const uint32_t canId)
    {
        uint8_t isConnected{OBCEnum::OBCErrCode::OBC_OK};
        const std::unordered_map<uint32_t, std::pair<uint8_t, uint16_t>>::iterator it_connectedId{mpCanIdConnectedId.find(canId)};
        if (it_connectedId != mpCanIdConnectedId.end())
        {
            connectId = it_connectedId->second.second;
            isConnected = OBCEnum::OBCErrCode::OBC_OK;
        }
        else
        {
            isConnected = OBCEnum::OBCErrCode::OBC_ERR_NOT_CONNECTED;
        }
        LOGI("[OnboardclientImpl] [%s] isConnected = %u", __func__, isConnected);
        return isConnected;
    }

    uint8_t OnboardclientImpl::getConnectedCanID(const uint16_t connectId, uint8_t &protocolType, uint32_t &canId)
    {
        LOGI("[OnboardclientImpl] [%s]", __func__);
        const std::unordered_map<uint16_t, std::pair<uint8_t, uint32_t>>::iterator it_connectedCAN{mpConnectedCAN.find(connectId)};
        uint8_t isConnected{OBCEnum::OBCErrCode::OBC_OK};
        if (it_connectedCAN != mpConnectedCAN.end())
        {
            canId = it_connectedCAN->second.second;
            protocolType = it_connectedCAN->second.first;
            isConnected = OBCEnum::OBCErrCode::OBC_OK;
        }
        else
        {
            isConnected = OBCEnum::OBCErrCode::OBC_ERR_NOT_CONNECTED;
        }
        LOGI("[OnboardclientImpl] [%s] isConnected = %u", __func__, isConnected);
        return isConnected;
    }

    uint8_t OnboardclientImpl::getConnectedProtocolType(const uint32_t connectCanId)
    {
        const std::unordered_map<uint32_t, std::pair<uint8_t, uint16_t>>::iterator it_connectedProtocol{mpCanIdConnectedId.find(connectCanId)};
        uint8_t protocolType{0U};
        if (it_connectedProtocol != mpCanIdConnectedId.end())
        {
            protocolType = it_connectedProtocol->second.first;
        }
        LOGI("getConnectedProtocolType %u", protocolType);
        return protocolType;
    }

    bool OnboardclientImpl::deleteUdsDataFromMap(const uint16_t connectedId)
    {
        const Mutex::Autolock mLock{Mutex::Autolock(mMutexRequestData)};
        bool res{false};
        if (!mUdsReqQueue.empty())
        {
            if (connectedId == mUdsReqQueue.front().first)
            {
                mUdsReqQueue.pop_front();
                LOGI("UDS data of connectID %u is deleted from MAP", connectedId);
                res = true;
            }
        }
        else
        {
            LOGV("UDS request queue is empty");
        }

        return res;
    }

    uint8_t OnboardclientImpl::convertProtocolType(const uint8_t origin, const bool isForMcu)
    {
        LOGI("[OnboardclientImpl] [convertProtocolType]");
        uint8_t newType{0U};

        if (isForMcu)
        {
            switch (origin)
            {
            case OBCEnum::OBCProtocolType::DOCAN: // 1U
                newType = OBCEnum::MCUProtocolType::MCU_DOCAN;
                break;
            case OBCEnum::OBCProtocolType::DOCAN29BIT: // 3U
                newType = OBCEnum::MCUProtocolType::MCU_DOCAN29BIT;
                break;
            case OBCEnum::OBCProtocolType::DOCAN11BITEX: // 4U
                newType = OBCEnum::MCUProtocolType::MCU_DOCAN11BITEX;
                break;
            case OBCEnum::OBCProtocolType::DOCAN29BITCANFD: // 6U
                newType = OBCEnum::MCUProtocolType::MCU_DOCAN29BITCANFD;
                break;
            default:
                LOGE("[%s] Invalid Protocol Type from App: %d", __func__, origin);
                break;
            }
        }
        else
        {
            switch (origin)
            {
            case OBCEnum::MCUProtocolType::MCU_DOCAN: // 0U
                newType = static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN);
                break;
            case OBCEnum::MCUProtocolType::MCU_DOCAN29BIT: // 1U
                newType = static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT);
                break;
            case OBCEnum::MCUProtocolType::MCU_DOCAN11BITEX: // 2U
                newType = static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX);
                break;
            case OBCEnum::MCUProtocolType::MCU_DOCAN29BITCANFD: // 5U
                newType = static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD);
                break;
            default:
                LOGE("Invalid Protocol Type from MCU: %d", origin);
                break;
            }
        }

        return newType;
    }
    uint32_t OnboardclientImpl::adjustCanIdFormatForObc(const uint16_t index, const android::sp<Buffer> payload)
    {
        uint32_t canId{0U};
        if (payload != nullptr)
        {
            canId = static_cast<uint32_t>(static_cast<uint32_t>(payload->data()[index]) << 24U) |
                    static_cast<uint32_t>(static_cast<uint32_t>(payload->data()[index + 1U]) << 16U) |
                    static_cast<uint32_t>(static_cast<uint32_t>(payload->data()[index + 2U]) << 8U) |
                    static_cast<uint32_t>(payload->data()[index + 3U]);
            LOGI("[adjustCanIdFormatForObc] CAN ID: 0x%02X", canId);
        }
        return canId;
    }

    uint16_t OnboardclientImpl::adjustNumForObc(const uint16_t index, const bool isUdsLen, const android::sp<Buffer> payload)
    {
        LOGI("[OnboardclientImpl] [adjustNumForObc]");
        uint16_t num{0U};
        if (payload != nullptr)
        {
            if (isUdsLen) // cal UDS Len
            {
                if (payload->data()[index] == 0x00U)
                {
                    num = static_cast<uint16_t>(payload->data()[index + 1U]);
                }
                else
                {
                    num = static_cast<uint16_t>(static_cast<uint16_t>(payload->data()[index]) << 8U) |
                          static_cast<uint16_t>(payload->data()[index + 1U]);
                }

                LOGI("[adjustNumForObc] UDS response length: 0x%02X", num);
            }
            else // cal Num of UDS response
            {
                if (payload->data()[index + 1U] == 0x00U)
                {
                    num = static_cast<uint16_t>((payload->data()[index]));
                }
                else
                {
                    num = static_cast<uint16_t>(static_cast<uint16_t>(payload->data()[index]) << 8U) |
                          static_cast<uint16_t>(payload->data()[index + 1U]);
                }

                LOGI("[adjustNumForObc] Number of UDS response: 0x%02X", num);
            }
        }
        return num;
    }

    void OnboardclientImpl::connectCanClient(android::sp<OBCTransportInfo> transportInfo, android::sp<OBCConnectInfo> &connectInfo)
    {
        LOGI("Start connectCanClient");
        if (transportInfo != nullptr)
        {
            if (connectInfo == nullptr)
            {
                LOGE("connectInfo null");
                connectInfo = new OBCConnectInfo();
            }
            // clean connect queue
            while (!mConnectQueue.empty())
            {
                mConnectQueue.pop();
            }
            std::thread mConnectCanClient{&OnboardclientImpl::startConnectCanThread, this, transportInfo, connectInfo};
            mConnectCanClient.join();
            LOGV("connectInfo connectId : %d, response : %02X", connectInfo->getConnectId(), connectInfo->getResponse());
            if ((transportInfo->udsResTimeout() != 0U) && (connectInfo->getResponse() == static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK)))
            {
                mpConnectIdTimeout[connectInfo->getConnectId()] = transportInfo->udsResTimeout();
            }
        }
        else
        {
            connectInfo->setData(OBCEnum::OBCErrCode::OBC_ERR_INVALID_PARAMETERS, 0U);
            LOGE("transportInfo null");
        }
    }

    uint8_t OnboardclientImpl::disconnectCanClient(const uint16_t connectId)
    {
        LOGI("Start disconnectCanClient");

        while (!mDisconnectQueue.empty())
        {
            mDisconnectQueue.pop();
        }

        uint8_t responseCode{0U};

        std::thread mDisconnectCanClient{&OnboardclientImpl::startDisconnectCanThread, this, connectId, std::ref(responseCode)};
        mDisconnectCanClient.join();
        LOGV("Disconnect result: %d", responseCode);

        return responseCode;
    }

    void OnboardclientImpl::onUdsAckResponse(void)
    {
        LOGI("[OnboardclientImpl] [onUdsAckResponse]");
        sp<CommunicationData> tmpCommData{new CommunicationData()};
        popFromQueue(OBC_COMM_ENUM::ACK_UDS_REQUEST, tmpCommData);
        if (tmpCommData->cmd != OBC_COMM_ENUM::ACK_UDS_REQUEST)
        {
            LOGE("[ERROR] [%s] Received command is 0x%02X", __func__, tmpCommData->cmd);
        }
        else if (tmpCommData->type == 0x82U)
        {
            LOGE("[ERROR] [%s] Received NRC from MCU", __func__);
        }
        else
        {
            if (tmpCommData->payload->size() == 5U)
            {
                const uint32_t canId{adjustCanIdFormatForObc(0U, tmpCommData->payload)};
                uint16_t connectedId{0U};
                (void)getConnectedId(connectedId, canId);
                if (!mUdsReqQueue.empty())
                {
                    if (connectedId != mUdsReqQueue.front().first)
                    {
                        LOGV("Not match connectedId");
                    }
                    else
                    {
                        OnboardclientTxHandler::getInstance()->resetCounterNoAck();
                        OnboardclientTxHandler::getInstance()->stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_COMMUNICATION_ERROR);
                        OnboardclientTxHandler::getInstance()->stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_BUSY_RESPONSE);
                        OnboardclientTxHandler::getInstance()->stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_ACK_RESPONSE);
                        OBCEnum::OBCErrCode errCode{OBCEnum::OBCErrCode::OBC_OK};
                        const uint8_t retVal{tmpCommData->payload->data()[4U]};
                        if (retVal >= 14U)
                        {
                            errCode = OBCEnum::OBCErrCode::OBC_ERR_FAILED;
                        }
                        else
                        {
                            errCode = static_cast<OBCEnum::OBCErrCode>(retVal);
                        }
                        (void)retVal;
                        if (errCode == OBCEnum::OBCErrCode::OBC_OK)
                        {
                            OnboardclientImpl::getInstance()->startTimeOutUdsRes(connectedId);
                            LOGI("[onUdsAckResponse] MCU normally transmit UDS request");
                            const android::sp<Buffer> udsReq{mUdsReqQueue.front().second};
                            if (deleteUdsDataFromMap(connectedId))
                            {
                                (void)udsReq;
                                attemptTransErr = 0U;
                                attemptBusyErr = 0U;
                                LOGI("[onUdsAckResponse] Success to delete UDS data");
                            }
                            if ((udsReq->data()[0U] == 0x3EU) && (udsReq->data()[1U] == 0x80U))
                            {
                                OnboardclientTxHandler::getInstance()->stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE);
                                processingNextRequest();
                            }
                        }
                        else if (errCode == OBCEnum::OBCErrCode::OBC_ERR_SEND_UDS_DATA)
                        {
                            LOGV("[onUdsAckResponse] MCU NACK: %u, attemptTransErr: %u", errCode, attemptTransErr);
                            processingRetry(errCode, connectedId);
                        }
                        else if (errCode == OBCEnum::OBCErrCode::OBC_ERR_BUSY)
                        {
                            LOGV("[onUdsAckResponse] MCU NACK: %u, attemptBusyErr: %u", errCode, attemptBusyErr);
                            processingRetry(errCode, connectedId);
                        }
                        else
                        {
                            LOGV("[onUdsAckResponse] MCU NACK: %u, attemptTransErr: %u, attemptBusyErr: %u", errCode, attemptTransErr, attemptBusyErr);
                            attemptTransErr = 0U;
                            attemptBusyErr = 0U;
                            OnboardclientTxHandler::getInstance()->stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE);
                            responseErrorEventToApp(errCode);
                            processingNextRequest();
                        }
                    }
                }
                else
                {
                    LOGV("mUdsReqQueue is empty");
                }
            }
            else
            {
                LOGE("[OnboardclientImpl] [%s] CommMgr response invalid payload", __func__);
            }
        }
    }

    void OnboardclientImpl::onUdsResponse(void)
    {
        LOGI("[OnboardclientImpl] [onUdsResponse]");
        sp<CommunicationData> tmpCommData{new CommunicationData()};
        popFromQueue(OBC_COMM_ENUM::SEND_UDS_RESPONSE, tmpCommData);

        if (tmpCommData->cmd != OBC_COMM_ENUM::SEND_UDS_RESPONSE)
        {
            LOGE("[ERROR] [%s] Received command is 0x%02X", __func__, tmpCommData->cmd);
        }
        else if (tmpCommData->type == 0x82U)
        {
            LOGE("[ERROR] [%s] Received NRC from MCU", __func__);
        }
        else
        {
            LOGI("[onUdsResponse] Receive UDS Response from MCU");
            const uint16_t responseNum{adjustNumForObc(0U, false, tmpCommData->payload)};
            if (responseNum <= 0U)
            {
                LOGE("[OnboardclientImpl] [%s] Invalid UDS response", __func__);
            }
            else
            {
                sp<OBCResponseEventInfo> resEventInfo{new OBCResponseEventInfo()};
                uint16_t index{2U};
                OnboardclientTxHandler::getInstance()->stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE);
                for (uint16_t idx{0U}; idx < responseNum; idx++)
                {
                    index = convertToObcData(index, tmpCommData->payload, resEventInfo);
                    if (index == 0U)
                    {
                        LOGE("[onUdsResponse] Invalid index");
                        break;
                    }
                    else
                    {
                        (void)OnboardclientManagerService::instance()->queryReceiverByOnboardClientReceiverOnResponseEvent(resEventInfo);
                    }
                }
                processingNextRequest();
            }
        }
    }

    void OnboardclientImpl::convertToCommData(const uint8_t protocolType, const uint32_t canId, android::sp<Buffer> connectPayload)
    {
        if (connectPayload == nullptr)
        {
            connectPayload = new Buffer();
        }

        uint8_t mProtocolType{static_cast<uint8_t>(0U)};
        mProtocolType = convertProtocolType(protocolType, true);

        uint8_t retCanId[4];
        retCanId[0] = static_cast<uint8_t>((canId >> 24U) & 0xFFU);
        retCanId[1] = static_cast<uint8_t>((canId >> 16U) & 0xFFU);
        retCanId[2] = static_cast<uint8_t>((canId >> 8U) & 0xFFU);
        retCanId[3] = static_cast<uint8_t>((canId) & 0xFFU);
        connectPayload->setTo(&mProtocolType, 1);
        connectPayload->append(&retCanId[0], 4);
    }

    void OnboardclientImpl::convertToCommData(const uint32_t connectedCanId, const android::sp<Buffer> udsData, const android::sp<CommunicationData> &commData)
    {
        if (udsData != nullptr)
        {
            commData->type = static_cast<uint8_t>(TYPE_REQUEST);
            commData->category = CATEGORY_OBC;
            commData->cmd = SEND_UDS_REQUEST;
            commData->cmd2 = OBC_COMM_ENUM::CMD2_OBC_DEFAULT;

            const uint8_t protocolType{getConnectedProtocolType(connectedCanId)};
            uint8_t mProtocolType{static_cast<uint8_t>(0U)};
            mProtocolType = convertProtocolType(protocolType, true);

            uint8_t retCanId[4];
            retCanId[0] = static_cast<uint8_t>((connectedCanId >> 24U) & 0xFFU);
            retCanId[1] = static_cast<uint8_t>((connectedCanId >> 16U) & 0xFFU);
            retCanId[2] = static_cast<uint8_t>((connectedCanId >> 8U) & 0xFFU);
            retCanId[3] = static_cast<uint8_t>((connectedCanId) & 0xFFU);

            uint8_t udsLen[2];
            udsLen[0] = 0U;
            const uint32_t tmpUdsLen{udsData->size()};
            if (tmpUdsLen > 0xFFU) // If length is 0x0FFF, transmit to MCU as 0xFFU 0x0F.
            {
                udsLen[0] = static_cast<uint8_t>(tmpUdsLen & 0xFFU);
                udsLen[1] = static_cast<uint8_t>((tmpUdsLen >> 8U) & 0xFFU);
            }
            else // If length is 3, transmit to MCU as 0x03 0xFFU.
            {
                udsLen[0] = static_cast<uint8_t>(tmpUdsLen & 0xFFU);
                udsLen[1] = static_cast<uint8_t>(0xFFU);
            }
            commData->payload->setTo(&mProtocolType, 1);
            commData->payload->append(&retCanId[0], 4);
            commData->payload->append(&udsLen[0], 2);
            if (tmpUdsLen <= static_cast<uint32_t>(INT32_MAX))
            {
                commData->payload->append(udsData->data(), static_cast<int32_t>(tmpUdsLen));
            }
            else
            {
                // Error
            }

            LOGI("Protocol: %x, connectedCanId: %x, comdata_size: %d", protocolType, connectedCanId, commData->payload->size());
        }
        else
        {
            LOGE("UDS data is null");
        }
    }

    uint32_t OnboardclientImpl::calTxCanId(const uint8_t protocolType, const uint32_t rxCanId)
    {
        uint32_t txCanId{0U};

        if ((protocolType == OBCEnum::OBCProtocolType::DOCAN29BIT) || (protocolType == OBCEnum::OBCProtocolType::DOCAN29BITCANFD))
        {
            const uint8_t byte1{static_cast<uint8_t>((rxCanId >> 8U) & 0xFFU)};
            const uint8_t byte2{static_cast<uint8_t>(rxCanId & 0xFFU)};
            txCanId = (rxCanId & 0xFFFF0000U) | static_cast<uint32_t>(static_cast<uint32_t>(byte2) << 8U) | static_cast<uint32_t>(byte1);
        }
        else if ((protocolType == OBCEnum::OBCProtocolType::DOCAN) || (protocolType == OBCEnum::OBCProtocolType::DOCAN11BITEX))
        {
            if (rxCanId >= 0x80000U)
            {
                txCanId = rxCanId - 0x80000U; // 11 0778FFFF - 0770FFFF = 80000, 11 extend 077803FF - 077003FF = 80000
            }
            else
            {
                // Handle the underflow case (e.g., set txCanId to a default value)
            }
        }
        else
        {
            LOGE("[calTxCanId] Invalid Protocol Type!");
            txCanId = 0U;
        }

        LOGI("[calTxCanId] TX CAN ID: 0x%02X", txCanId);

        return txCanId;
    }

    uint32_t OnboardclientImpl::calRxCanId(const uint8_t protocolType, const uint32_t txCanId)
    {
        uint32_t rxCanId{0U};

        if ((protocolType == OBCEnum::OBCProtocolType::DOCAN29BIT) || (protocolType == OBCEnum::OBCProtocolType::DOCAN29BITCANFD))
        {
            const uint8_t byte1{static_cast<uint8_t>((txCanId >> 8U) & 0xFFU)};
            const uint8_t byte2{static_cast<uint8_t>(txCanId & 0xFFU)};
            rxCanId = (txCanId & 0xFFFF0000U) | static_cast<uint32_t>(static_cast<uint32_t>(byte2) << 8U) | static_cast<uint32_t>(byte1);
        }
        else if ((protocolType == OBCEnum::OBCProtocolType::DOCAN) || (protocolType == OBCEnum::OBCProtocolType::DOCAN11BITEX))
        {
            if (txCanId <= UINT32_MAX - 0x80000U)
            {
                rxCanId = txCanId + 0x80000U;
            }
            else
            {
                // Handle the overflow case (e.g., set rxCanId to a default value)
            }
        }
        else
        {
            LOGE("[calRxCanId] Invalid Protocol Type!");
            rxCanId = 0U;
        }

        LOGI("[calRxCanId] RX CAN ID: 0x%02X", rxCanId);

        return rxCanId;
    }

    uint16_t OnboardclientImpl::convertToObcData(const uint16_t index, const android::sp<Buffer> commResData, android::sp<OBCResponseEventInfo> &obcEventResInfo)
    {
        uint16_t res{0U};
        if (commResData == nullptr)
        {
            LOGE("[convertToObcData] Payload is empty!");
            res = 0U;
        }
        else
        {
            if (obcEventResInfo == nullptr)
            {
                obcEventResInfo = new OBCResponseEventInfo();
            }
            obcEventResInfo->errCode() = commResData->data()[index];
            obcEventResInfo->resInfo()->protocolType() = convertProtocolType(commResData->data()[index + 1U], false);
            obcEventResInfo->resInfo()->canInfo()->canId() = adjustCanIdFormatForObc(static_cast<uint16_t>((index + static_cast<uint16_t>(2U)) & 0xFFFFU), commResData);
            /* To do - nTA value should be adjust by the spec */
            obcEventResInfo->resInfo()->canInfo()->nTa().clear();
            if ((obcEventResInfo->resInfo()->protocolType() == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
                (obcEventResInfo->resInfo()->protocolType() == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
                (obcEventResInfo->resInfo()->protocolType() == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
            {
                const uint16_t nTa{static_cast<uint16_t>(((obcEventResInfo->resInfo()->canInfo()->canId() >> 8U) & 0xFFU))};
                std::stringstream ss{};
                ss << &std::hex << &std::uppercase << std::setw(2) << std::setfill('0') << nTa;
                const std::string hexString{ss.str()}; // Convert to string
                for (size_t idx{0U}; idx < hexString.size(); idx++)
                {
                    const std::string tmp{std::string(1U, hexString[idx])};
                    obcEventResInfo->resInfo()->canInfo()->nTa().push_back(tmp);
                }
            }

            const uint32_t txCanId{calTxCanId(obcEventResInfo->resInfo()->protocolType(), obcEventResInfo->resInfo()->canInfo()->canId())};
            const uint8_t isConnected{getConnectedId(obcEventResInfo->resInfo()->connectId(), txCanId)};
            if (isConnected == static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK))
            {
                obcEventResInfo->resInfo()->responseType() = OBCEnum::OBCUdsResponseType::NORMAL;
            }
            else
            {
                obcEventResInfo->resInfo()->responseType() = OBCEnum::OBCUdsResponseType::EVENT;
            }

            const uint16_t udsLen{adjustNumForObc(static_cast<uint16_t>((index + static_cast<uint16_t>(6U)) & 0xFFFFU), true, commResData)};

            /* To do - Handling for wrong UDS length */
            if ((udsLen > OBC_DIAG_ENUM::BLOCK_SIZE) || (udsLen < 1U))
            {
                LOGE("[convertToObcData] Invalid Payload length: %ld", udsLen);
                res = 0U;
            }
            else
            {
                uint8_t data[udsLen];
                data[0] = 0x00U;
                for (uint16_t idx{0U}; idx < udsLen; idx++)
                {
                    data[idx] = commResData->data()[index + 8U + idx];
                }
                obcEventResInfo->resInfo()->udsData()->setTo(&data[0], static_cast<int32_t>(udsLen));

                /* Send RoB Ack to MCU */
                if ((data[0] == 0x62U) && (data[1] == 0xA0U) && (data[2] == 0x06U))
                {
                    const android::sp<Buffer> payload{new Buffer()};
                    uint8_t tmpCanId[5];
                    tmpCanId[0] = 0U;

                    for (uint8_t idx{0U}; idx < 4U; idx++)
                    {
                        tmpCanId[idx] = commResData->data()[(index + 2U) + idx];
                    }
                    tmpCanId[4] = 1U;

                    payload->setTo(&tmpCanId[0], 5);

                    (void)CommunicationManagerAdapter::getInstance()->sendUdsDataToMcu(static_cast<uint8_t>(TYPE_REQUEST), OBC_COMM_ENUM::CATEGORY_OBC, OBC_COMM_ENUM::ACK_ROB_DETECT, OBC_COMM_ENUM::CMD2_OBC_DEFAULT, payload);
                }
                res = ((index + 8U + udsLen) & 0xFFFFU);
            }
        }
        return res;
    }

    void OnboardclientImpl::popFromQueue(const uint8_t want, sp<CommunicationData> &tmpData)
    {
        if (tmpData == nullptr)
        {
            tmpData = new CommunicationData();
        }

        if (want == OBC_COMM_ENUM::RESPONSE_CONNECT)
        {
            while (!mConnectQueue.empty())
            {
                LOGI("Pop data for connect");
                tmpData = mConnectQueue.front();
                mConnectQueue.pop();
                break;
            }
        }
        else if (want == OBC_COMM_ENUM::RESPONSE_DISCONNECT)
        {
            while (!mDisconnectQueue.empty())
            {
                LOGI("Pop data for disconnect");
                tmpData = mDisconnectQueue.front();
                mDisconnectQueue.pop();
                break;
            }
        }
        else if (want == OBC_COMM_ENUM::ACK_UDS_REQUEST)
        {
            while (!mUdsAckQueue.empty())
            {
                LOGI("Pop data for UDS Ack");
                tmpData = mUdsAckQueue.front();
                mUdsAckQueue.pop();
                break;
            }
        }
        else if (want == OBC_COMM_ENUM::SEND_UDS_RESPONSE)
        {
            while (!mUdsResponseQueue.empty())
            {
                LOGI("Pop data for UDS response");
                tmpData = mUdsResponseQueue.front();
                mUdsResponseQueue.pop_front();
                break;
            }
        }
        else
        {
            LOGE("[popFromQueue] Invalid type");
        }
    }

    bool OnboardclientImpl::startConnectCanThread(const android::sp<OBCTransportInfo> transportInfo, const android::sp<OBCConnectInfo> connectInfo)
    {
        int32_t checkStatusRetryCount{0};
        bool ret{false};
        LOGI("[OnboardclientImpl] Start Connect CAN Client Thread!!");
        while (true)
        {
            if (handleConnectCanClient(transportInfo, connectInfo) || (checkStatusRetryCount >= 1))
            {
                if (checkStatusRetryCount >= 1)
                {
                    LOGI("[OnboardclientImpl] checkStatusRetryCount > 2 !!");
                }
                else
                {
                    LOGI("[OnboardclientImpl] checkStatusRetryCount < 2 !!");
                    ret = true;
                }
                break;
            }
            else
            {
                checkStatusRetryCount++; // Retry MAX 2 times
                LOGI("[OnboardclientImpl] handleConnectCanClient false !!");
            }
        }

        return ret;
    }

    bool OnboardclientImpl::startDisconnectCanThread(const uint16_t connectId, uint8_t &responseCode)
    {
        uint8_t checkStatusRetryCount{0U};
        bool ret{false};
        LOGI("[OnboardclientImpl] Start Disconnect CAN Client Thread!!");
        while (true)
        {
            responseCode = handleDisconnectCanClient(connectId);
            if ((responseCode == static_cast<uint8_t>(OBCEnum::OBC_OK)) || (checkStatusRetryCount >= 1U))
            {
                if (checkStatusRetryCount >= 1U)
                {
                    LOGI("[OnboardclientImpl] checkStatusRetryCount > 2 !!");
                }
                else
                {
                    LOGI("[OnboardclientImpl] checkStatusRetryCount < 2 !!");
                    ret = true;
                }
                break;
            }
            else
            {
                checkStatusRetryCount++; // Retry MAX 2 times
                LOGI("[OnboardclientImpl] disconnect can client false !!");
            }
        }

        return ret;
    }

    bool OnboardclientImpl::handleConnectCanClient(const android::sp<OBCTransportInfo> transportInfo, android::sp<OBCConnectInfo> connectInfo)
    {
        LOGI("[OnboardclientImpl] handleConnectCanClient");

        bool result{false};
        uint8_t mresponse{0U};
        uint16_t mconnectId{0U};
        const sp<Buffer> payload{new Buffer()};
        const sp<CommunicationData> commData{new CommunicationData()};
        const uint8_t protocolType{transportInfo->protocolType()};
        const uint32_t canId{transportInfo->canInfo().canId()};

        LOGI("[OnboardclientImpl] [%s] TransportInfo: protocolType: %02X, canId: %02X", __func__, protocolType, canId);

        convertToCommData(protocolType, canId, payload);
        (void)CommunicationManagerAdapter::getInstance()->sendUdsDataToMcu(static_cast<uint8_t>(TYPE_REQUEST), OBC_COMM_ENUM::CATEGORY_OBC, REQUEST_CONNECT, OBC_COMM_ENUM::CMD2_OBC_DEFAULT, payload);
        if (receiveMessageFromMcu(OBC_COMM_ENUM::RESPONSE_CONNECT, commData) == true)
        {
            // Handle MCU's status
            LOGI("[OnboardclientImpl] [%s] SUCCESS to Connect CAN CLient", __func__);
            if (commData->payload->size() == 5U)
            {
                LOGI("size comm data is 5");
                const uint32_t resCanId{adjustCanIdFormatForObc(0U, commData->payload)};
                if (resCanId != canId)
                {
                    LOGE("[OnboardclientImpl] [%s] Wrong CAN ID", __func__);
                    mresponse = OBCEnum::OBC_ERR_INVALID_PARAMETERS;
                    connectInfo->setData(mresponse, mconnectId);
                    result = false;
                }
                else
                {
                    if (connectInfo == nullptr)
                    {
                        LOGI("[OnboardclientImpl] [%s] connectInfo null", __func__);
                        connectInfo = new OBCConnectInfo();
                    }
                    // Make Connect ID
                    mresponse = commData->payload->data()[4U];
                    if (mresponse == OBCEnum::OBCErrCode::OBC_OK)
                    {
                        mconnectId = makeConnectId(protocolType, canId);
                    }
                    connectInfo->setData(mresponse, mconnectId);
                    LOGI("[OnboardclientImpl] [%s] connectId : %lu, response : %02X", __func__, mconnectId, mresponse);
                    result = true;
                }
            }
            else
            {
                mresponse = OBCEnum::OBCErrCode::OBC_ERR_FAILED;
                connectInfo->setData(mresponse, mconnectId);
                result = false;
            }
        }
        else
        {
            mresponse = OBCEnum::OBCErrCode::OBC_ERR_FAILED;
            result = false;
            connectInfo->setData(mresponse, mconnectId);
            LOGE("[OnboardclientImpl] [%s] Fail to Connect CAN CLient", __func__);
        }

        return result;
    }

    uint8_t OnboardclientImpl::handleDisconnectCanClient(const uint16_t connectId)
    {
        LOGI("[OnboardclientImpl] handleDisconnectCanClient");

        const android::sp<Buffer> payload{new Buffer()};
        const android::sp<CommunicationData> commData{new CommunicationData()};
        OBCEnum::OBCErrCode responseCode{OBCEnum::OBCErrCode::OBC_ERR_NOT_CONNECTED};
        uint8_t protocolType{0U};
        uint32_t canId{0U};

        responseCode = static_cast<OBCEnum::OBCErrCode>(OnboardclientImpl::getInstance()->getConnectedCanID(connectId, protocolType, canId));
        if (responseCode == OBCEnum::OBCErrCode::OBC_OK)
        {
            convertToCommData(protocolType, canId, payload);
            (void)CommunicationManagerAdapter::getInstance()->sendUdsDataToMcu(static_cast<uint8_t>(TYPE_REQUEST), OBC_COMM_ENUM::CATEGORY_OBC, REQUEST_DISCONNECT, OBC_COMM_ENUM::CMD2_OBC_DEFAULT, payload);
            if (receiveMessageFromMcu(OBC_COMM_ENUM::RESPONSE_DISCONNECT, commData) == true)
            {
                // Handle MCU's status
                LOGI("[OnboardclientImpl] [%s] SUCCESS to Disconnect CAN CLient", __func__);
                if (commData->payload->size() == 5U)
                {
                    LOGV("[OnboardclientImpl] [%s] size comm data is 5", __func__);
                    const uint32_t resCanId{adjustCanIdFormatForObc(0U, commData->payload)};

                    const uint8_t mcuRes{commData->payload->data()[4U]};
                    if (mcuRes <= static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_ERR_MAX))
                    {
                        responseCode = static_cast<OBCEnum::OBCErrCode>(mcuRes);
                    }
                    (void)mcuRes;
                    if (resCanId != canId)
                    {
                        LOGE("[OnboardclientImpl] [%s] Wrong CAN ID, response CanId=0x%02X but required canId=0x%02X", __func__, resCanId, canId);
                        responseCode = OBCEnum::OBCErrCode::OBC_ERR_FAILED;
                    }
                    else
                    {
                        LOGV("[OnboardclientImpl] [%s] connectId : %lu, canId: 0x%02X, response code: %02X", __func__, connectId, canId, responseCode);
                        const std::unordered_map<uint16_t, std::pair<uint8_t, uint32_t>>::iterator it_connectedCAN{mpConnectedCAN.find(connectId)};
                        const std::unordered_map<uint32_t, std::pair<uint8_t, uint16_t>>::iterator it_connectedId{mpCanIdConnectedId.find(canId)};
                        const std::unordered_map<uint16_t, uint16_t>::iterator it_resTimeOut{mpConnectIdTimeout.find(connectId)};
                        if (it_connectedCAN != mpConnectedCAN.end())
                        {
                            (void)mpConnectedCAN.erase(it_connectedCAN);
                        }
                        if (it_connectedId != mpCanIdConnectedId.end())
                        {
                            (void)mpCanIdConnectedId.erase(it_connectedId);
                        }
                        if (it_resTimeOut != mpConnectIdTimeout.end())
                        {
                            (void)mpConnectIdTimeout.erase(it_resTimeOut);
                        }
                        if (!mUdsReqQueue.empty())
                        {
                            mUdsReqQueue.clear();
                        }
                        if (isProcessing == true)
                        {
                            isProcessing = false;
                            OnboardclientTxHandler::getInstance()->stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE);
                        }
                    }
                }
                else
                {
                    LOGE("[OnboardclientImpl] [%s] CommMgr response invalid payload", __func__);
                    responseCode = OBCEnum::OBCErrCode::OBC_ERR_FAILED;
                }
            }
            else
            {
                responseCode = OBCEnum::OBCErrCode::OBC_ERR_FAILED;
                LOGE("[OnboardclientImpl] [%s] Fail to Disonnect CAN CLient", __func__);
            }
        }
        return static_cast<uint8_t>(responseCode);
    }

    size_t OnboardclientImpl::commDataMessageCount(const uint8_t want)
    {
        size_t queueSize{0U};
        const std::lock_guard<std::mutex> guard{mMutexCommDataQueue};

        switch (want)
        {
        case OBC_COMM_ENUM::RESPONSE_CONNECT:
        {
            LOGV("commDataMessageCount, %d", mConnectQueue.size());
            queueSize = mConnectQueue.size();
            break;
        }
        case OBC_COMM_ENUM::RESPONSE_DISCONNECT:
        {
            LOGV("commDataMessageCount, %d", mDisconnectQueue.size());
            queueSize = mDisconnectQueue.size();
            break;
        }
        default:
            break;
        }

        return queueSize;
    }

    void OnboardclientImpl::handleUdsRespFromDiagMgr(const sp<Buffer> udsResp)
    {
        LOGI("[handleUdsRespFromDiagMgr]");
        const std::unique_lock<std::mutex> lock{mMutexDiagData};
        if (udsResp != nullptr)
        {
            const sp<OBCResponseEventInfo> obcEventResInfo{new OBCResponseEventInfo()};
            /* CAN ID for DCM*/
            obcEventResInfo->resInfo()->protocolType() = OBCEnum::OBCProtocolType::DOCAN29BITCANFD;
            obcEventResInfo->resInfo()->canInfo()->canId() = 0x18DAE12BU;
            /* To do - nTA value should be adjust by the spec */
            obcEventResInfo->resInfo()->canInfo()->nTa().clear();
            if ((obcEventResInfo->resInfo()->protocolType() == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
                (obcEventResInfo->resInfo()->protocolType() == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
                (obcEventResInfo->resInfo()->protocolType() == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
            {
                const uint16_t nTa{static_cast<uint16_t>(((obcEventResInfo->resInfo()->canInfo()->canId() >> 8U) & 0xFFU))};
                std::stringstream ss{};
                ss << &std::hex << &std::uppercase << std::setw(2) << std::setfill('0') << nTa;
                const std::string hexString{ss.str()}; // Convert to string
                for (size_t idx{0U}; idx < hexString.size(); idx++)
                {
                    const std::string tmp{std::string(1U, hexString[idx])};
                    obcEventResInfo->resInfo()->canInfo()->nTa().push_back(tmp);
                }
            }
            const uint8_t isConnected{getConnectedId(obcEventResInfo->resInfo()->connectId(), 0x18DA2BE1U)};
            (void)isConnected;
            obcEventResInfo->resInfo()->responseType() = OBCEnum::OBCUdsResponseType::NORMAL;

            /* UDS payload set */
            if ((udsResp->size() >= OBC_DIAG_ENUM::BLOCK_SIZE) || (udsResp->size() < 1U))
            {
                LOGE("[handleUdsRespFromDiagMgr] Invalid UDS response size: %d", udsResp->size());
                obcEventResInfo->errCode() = 1U;

                uint8_t negativeResponse[3];
                negativeResponse[0] = 0x7FU;
                negativeResponse[1] = 0x36U; // tmp
                negativeResponse[2] = 0xFFU;

                obcEventResInfo->resInfo()->udsData()->setTo(&negativeResponse[0], 3);
            }
            else
            {
                if (udsResp->data()[0] == 0x7FU)
                {
                    obcEventResInfo->errCode() = 1U;
                }
                else
                {
                    obcEventResInfo->errCode() = 0U;
                }
                const uint32_t udsResSize{udsResp->size()};
                if (udsResSize <= static_cast<uint32_t>(INT32_MAX))
                {
                    obcEventResInfo->resInfo()->udsData()->setTo(udsResp->data(), static_cast<int32_t>(udsResSize));
                }
                else
                {
                    LOGV("Overflow data size");
                }
                (void)udsResSize;
            }

            (void)OnboardclientManagerService::instance()->queryReceiverByOnboardClientReceiverOnResponseEvent(obcEventResInfo);
            processingNextRequest();
        }
        else
        {
            LOGE("[handleUdsRespFromDiagMgr] UDS response is null");
        }
    }

    void OnboardclientImpl::pushToQueue(const sp<CommunicationData> commData, const OBC_COMMON::COMM_RESPONSE_TYPE type)
    {
        const std::unique_lock<std::mutex> lock{std::unique_lock<std::mutex>(mMutexCommDataQueue)};

        switch (type)
        {
        case OBC_COMMON::COMM_RESPONSE_TYPE::COMM_RES_CONNECT:
        {
            LOGV("pushToQueue COMM_RES_CONNECT");
            mConnectQueue.push(commData);
            mWaitConditionCommDataQueue.notify_one();
            break;
        }
        case OBC_COMMON::COMM_RESPONSE_TYPE::COMM_RES_DISCONNECT:
        {
            LOGV("pushToQueue COMM_RES_DISCONNECT");
            mDisconnectQueue.push(commData);
            mWaitConditionCommDataQueue.notify_one();
            break;
        }
        case OBC_COMMON::COMM_RESPONSE_TYPE::COMM_RES_ACK_UDS_REQUEST:
        {
            LOGV("pushToQueue COMM_RES_ACK_UDS_REQUEST");
            mUdsAckQueue.push(commData);
            onUdsAckResponse();
            break;
        }
        case OBC_COMMON::COMM_RESPONSE_TYPE::COMM_RES_UDS_REQUEST:
        {
            LOGV("pushToQueue COMM_RES_UDS_REQUEST");
            const uint32_t totalSize{commData->payload->size() + getReceiveQueueSize()};
            if (totalSize <= RECEIVE_QUEUE_MAX_SIZE)
            {
                mUdsResponseQueue.push_back(commData);
                onUdsResponse();
            }
            else
            {
                LOGE("Response queue is full");
            }

            break;
        }
        default:
        {
            LOGE("Invalid type %u", type);
            break;
        }
        }
    }

    bool OnboardclientImpl::receiveMessageFromMcu(const uint8_t want, const sp<CommunicationData> commData)
    {
        bool ret{false};
        int32_t count{0};

        sp<CommunicationData> tmpCommData{new CommunicationData()};

        for (count = 0; count < 3; count++) // MAX_RETRY_COUNT_MCU = 10 // TBD
        {
            if (commDataMessageCount(want) <= 0U)
            {
                // int32_t retryCount{0};
                std::unique_lock<std::mutex> lock{mMutexCommDataQueue};
                if (mWaitConditionCommDataQueue.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout)
                {
                    // Cannot receive response during 1 sec from MCU
                    // retryCount += 1;
                    // if (retryCount == 3)
                    // {
                    //     LOGE("Cannot receive resp from MCU during 3 sec!!!");
                    //     return false;
                    // }
                    continue;
                }
                else
                {
                    // Receive message from MCU
                    // retryCount = 0;
                }
            }

            // pop data from queue
            popFromQueue(want, tmpCommData);

            // pack comm data
            commData->type = tmpCommData->type;
            commData->category = tmpCommData->category;
            commData->cmd = tmpCommData->cmd;
            commData->cmd2 = tmpCommData->cmd2;
            const uint32_t payloadSize{tmpCommData->payload->size()};
            if (payloadSize <= static_cast<uint32_t>(INT32_MAX))
            {
                commData->payload->setTo(tmpCommData->payload->data(), static_cast<int32_t>(payloadSize));
            }
            else
            {
                LOGE("[receiveMessageFromMcu] Overflow data size");
            }
            (void)payloadSize;
            if ((commData->cmd == want) && (commData->type == 0x82U))
            {
                LOGE("[ERROR] [%s] Received NRC from MCU for 0x%02X message", __func__, want);
                count = 3;
            }
            else if (commData->cmd != want)
            {
                LOGE("[ERROR] [%s] Received command is 0x%02X, but wait for 0x%02X", __func__, commData->cmd, want);
            }
            else if (commData->type == 0x82U)
            {
                LOGE("[ERROR] [%s] Received NRC from MCU for other 0x%02X cmd", __func__, commData->cmd);
            }
            else
            {
                break;
            }
        }

        if (count == 3) // ERROR case MAX_RETRY_COUNT_MCU
        {
            ret = false;
        }
        else // SUCCESS case
        {
            ret = true;
            for (uint32_t idx{0U}; idx < commData->payload->size(); idx++)
            {
                LOGI("Payload from MCU [%d] : 0x%02X", idx, commData->payload->data()[idx]);
            }
        }

        return ret;
    }

    void OnboardclientImpl::pushToTransQueue(const uint16_t connectedId, const android::sp<Buffer> udsReq)
    {
        const Mutex::Autolock mLock{Mutex::Autolock(mMutexRequestData)};
        LOGI("pushToTransQueue, %u", connectedId);
        mUdsReqQueue.push_back({connectedId, udsReq});
        if (isProcessing == false)
        {
            isProcessing = true;
            const uint16_t requestId{mUdsReqQueue.front().first};
            uint32_t connectedCanId{0U};
            uint8_t protocolType{0U};
            const uint8_t isConnected{OnboardclientImpl::getInstance()->getConnectedCanID(requestId, protocolType, connectedCanId)};
            if (isConnected == OBCEnum::OBCErrCode::OBC_OK)
            {
                if ((udsReq->data()[0U] == OBCEnum::OBC_UDS_SID::SID_36_TRANSFER_DATA) && (connectedCanId == 0x18DA2BE1U))
                {
                    (void)OnboardclientTxHandler::getInstance()->obtainMessage(OBC_TX_HANDLER::CMD_TX_SEND_UDS_TO_DIAG, static_cast<int32_t>(requestId))->sendToTarget();
                }
                else
                {
                    (void)connectedCanId;
                    (void)OnboardclientTxHandler::getInstance()->obtainMessage(OBC_TX_HANDLER::CMD_TX_SEND_UDS, static_cast<int32_t>(requestId))->sendToTarget();
                }
            }
            else
            {
                LOGI("Not connect to send UDS req");
                (void)connectedCanId;
                (void)protocolType;
            }
        }
    }

    bool OnboardclientImpl::getUdsReqFromQueue(const uint16_t connectedId, const android::sp<Buffer> &udsReqData)
    {
        bool res{true};
        if (!mUdsReqQueue.empty())
        {
            const uint16_t latestConnectId{mUdsReqQueue.front().first};
            if (latestConnectId == connectedId)
            {
                const uint32_t udsSize{mUdsReqQueue.front().second->size()};
                if (udsSize <= static_cast<uint32_t>(INT32_MAX))
                {
                    res = true;
                    udsReqData->setTo(mUdsReqQueue.front().second->data(), static_cast<int32_t>(udsSize));
                }
                else
                {
                    res = false;
                    LOGE("[getUdsReqFromQueue] size of UDS is incorrect: %lu", udsSize);
                }
            }
            LOGV("[getUdsReqFromQueue] connectedId: %u, res: %d, udsSize: %lu", connectedId, res, udsReqData->size());
        }
        else
        {
            res = false;
            LOGV("[getUdsReqFromQueue] mUdsReqQueue is empty ");
        }

        return res;
    }

    uint16_t OnboardclientImpl::getCurrentProcessingId(void) const noexcept
    {
        LOGI("getCurrentProcessingId, currentId %u", this->currentProcessingId);
        return this->currentProcessingId;
    }

    void OnboardclientImpl::setCurrentProcessingId(const uint16_t processingId) noexcept
    {
        this->currentProcessingId = processingId;
    }

    void OnboardclientImpl::processingNextRequest(void)
    {
        const Mutex::Autolock mLock{Mutex::Autolock(mMutexRequestData)};
        attemptTransErr = 0U;
        attemptBusyErr = 0U;
        if (!mUdsReqQueue.empty())
        {
            LOGI("Process next request");
            const uint16_t requestId{mUdsReqQueue.front().first};
            uint32_t connectedCanId{0U};
            uint8_t protocolType{0U};
            const uint8_t isConnected{OnboardclientImpl::getInstance()->getConnectedCanID(requestId, protocolType, connectedCanId)};
            if (isConnected == OBCEnum::OBCErrCode::OBC_OK)
            {
                const android::sp<Buffer> udsReq{mUdsReqQueue.front().second};
                if ((udsReq->data()[0U] == OBCEnum::OBC_UDS_SID::SID_36_TRANSFER_DATA) && (connectedCanId == 0x18DA2BE1U))
                {
                    (void)OnboardclientTxHandler::getInstance()->obtainMessage(OBC_TX_HANDLER::CMD_TX_SEND_UDS_TO_DIAG, static_cast<int32_t>(requestId))->sendToTarget();
                }
                else
                {
                    (void)connectedCanId;
                    (void)OnboardclientTxHandler::getInstance()->obtainMessage(OBC_TX_HANDLER::CMD_TX_SEND_UDS, static_cast<int32_t>(requestId))->sendToTarget();
                }
            }
            else
            {
                LOGI("Not connect to send UDS req");
                (void)connectedCanId;
                (void)protocolType;
            }
        }
        else
        {
            LOGI("End sending UDS request");
            isProcessing = false;
        }
    }

    void OnboardclientImpl::responseErrorEventToApp(const OBCEnum::OBCErrCode errCode)
    {
        const android::sp<OBCResponseEventInfo> obcEventResInfo{new OBCResponseEventInfo()};
        obcEventResInfo->errCode() = errCode;
        obcEventResInfo->resInfo()->connectId() = getCurrentProcessingId();
        uint8_t protocolType{0U};
        uint32_t canId{0U};
        (void)getConnectedCanID(obcEventResInfo->resInfo()->connectId(), protocolType, canId);
        obcEventResInfo->resInfo()->protocolType() = protocolType;

        const uint32_t rxCanId{calRxCanId(protocolType, canId)};
        obcEventResInfo->resInfo()->canInfo()->canId() = rxCanId;

        obcEventResInfo->resInfo()->canInfo()->nTa().clear();
        if ((obcEventResInfo->resInfo()->protocolType() == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN11BITEX)) ||
            (obcEventResInfo->resInfo()->protocolType() == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BIT)) ||
            (obcEventResInfo->resInfo()->protocolType() == static_cast<uint8_t>(OBCEnum::OBCProtocolType::DOCAN29BITCANFD)))
        {
            const uint16_t nTa{static_cast<uint16_t>(((obcEventResInfo->resInfo()->canInfo()->canId() >> 8U) & 0xFFU))};
            std::stringstream ss{};
            ss << &std::hex << &std::uppercase << std::setw(2) << std::setfill('0') << nTa;
            const std::string hexString{ss.str()}; // Convert to string
            for (size_t idx{0U}; idx < hexString.size(); idx++)
            {
                const std::string tmp{std::string(1U, hexString[idx])};
                obcEventResInfo->resInfo()->canInfo()->nTa().push_back(tmp);
            }
        }
        obcEventResInfo->resInfo()->responseType() = OBCEnum::OBCUdsResponseType::NORMAL;
        obcEventResInfo->resInfo()->udsData()->setSize(0);
        (void)OnboardclientManagerService::instance()->queryReceiverByOnboardClientReceiverOnResponseEvent(obcEventResInfo);
    }

    uint32_t OnboardclientImpl::getTransQueueSize(void)
    {
        const Mutex::Autolock mLock{Mutex::Autolock(mMutexRequestData)};
        uint32_t sizeOfQueue{0U};
        for (std::deque<std::pair<uint16_t, android::sp<Buffer>>>::iterator it{mUdsReqQueue.begin()}; it != mUdsReqQueue.end(); it++)
        {
            sizeOfQueue += it->second->size();
        }
        LOGI("getTransQueueSize Trans queue size %lu", sizeOfQueue);
        return sizeOfQueue;
    }

    void OnboardclientImpl::processingRetry(const OBCEnum::OBCErrCode errType, const uint16_t connectedId)
    {
        uint8_t maxRetryCount{0U};
        int32_t timerId{0};
        uint8_t *attemptCount{nullptr};
        bool res{false};
        if (errType == OBCEnum::OBCErrCode::OBC_ERR_BUSY)
        {

            attemptCount = &attemptBusyErr;
            maxRetryCount = MAX_RETRY_COUNT_BUSY_ERR;
            timerId = OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_BUSY_RESPONSE;
            res = true;
        }
        else if (errType == OBCEnum::OBCErrCode::OBC_ERR_SEND_UDS_DATA)
        {
            attemptCount = &attemptTransErr;
            maxRetryCount = MAX_RETRY_COUNT_TRANS_ERR;
            timerId = OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_COMMUNICATION_ERROR;
            res = true;
        }
        else
        {
            res = false;
        }
        if (res == true)
        {
            LOGI("[processingRetry] errType %u, connectedId %u, attemptCount %u, maxRetryCount %u", errType, connectedId, (*attemptCount), maxRetryCount);
            if ((*attemptCount) < maxRetryCount) // (*attemptCount) <= maxRetryCount
            {
                (*attemptCount)++;
                OnboardclientTxHandler::getInstance()->startTimer(timerId);
            }
            else
            {
                (void)timerId;
                LOGV("[processingRetry] Over retry policy. Stop send UDS");

                (*attemptCount) = 0U;
                OnboardclientTxHandler::getInstance()->stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE);
                responseErrorEventToApp(errType);
                (void)deleteUdsDataFromMap(connectedId);
                LOGV("[processingRetry] Success to delete UDS data");
                processingNextRequest();
            }
        }
        (void)attemptCount;
        (void)timerId;
        (void)maxRetryCount;
    }

    uint32_t OnboardclientImpl::getReceiveQueueSize(void)
    {
        const Mutex::Autolock mLock{Mutex::Autolock(mMutexResponseData)};
        uint32_t sizeOfUdsRes{0U};
        for (uint32_t idx{0U}; idx < mUdsResponseQueue.size(); idx++)
        {
            const uint32_t sizeOfData{mUdsResponseQueue[idx]->payload->size()};
            if (sizeOfUdsRes <= UINT32_MAX - sizeOfData)
            {
                sizeOfUdsRes += sizeOfData;
            }
            else
            {
                (void)sizeOfData;
            }
        }
        return sizeOfUdsRes;
    }

    void OnboardclientImpl::startTimeOutUdsRes(const uint16_t requestId)
    {
        uint16_t duration{190U};
        const std::unordered_map<uint16_t, uint16_t>::iterator it_resTimeOut{mpConnectIdTimeout.find(requestId)};
        if (it_resTimeOut != mpConnectIdTimeout.end())
        {
            duration = it_resTimeOut->second;
        }
        OnboardclientTxHandler::getInstance()->startTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE, static_cast<uint32_t>(duration));
    }

    void OnboardclientImpl::setTempMaxExceedSize(const uint32_t queueSize) noexcept
    {
        RECEIVE_QUEUE_MAX_SIZE = queueSize;
        LOGV("[setTempMaxExceedSize] Receive queue size: %lu", RECEIVE_QUEUE_MAX_SIZE);
    }
};
