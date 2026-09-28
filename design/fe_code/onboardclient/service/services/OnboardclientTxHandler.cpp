#include "OnboardclientTxHandler.h"
#include "OnboardclientImpl.h"
#include "CommunicationManagerAdapter.h"
#include "DiagManagerAdapter.h"
namespace OBC
{
    OnboardclientTxHandler *OnboardclientTxHandler::mOnboardclientTxHandler{nullptr};

    OnboardclientTxHandler::OnboardclientTxHandler(sp<sl::SLLooper> &looper, const android::sp<OnboardclientManagerService> onboardclientMgrService)
        : Handler(looper), mOnboardclientMgrService(onboardclientMgrService)
    {
        mOnboardclientTxHandler = this;
        attemptNoAck = 0U;
        initTimer();
    }

    OnboardclientTxHandler *OnboardclientTxHandler::getInstance()
    {
        if (mOnboardclientTxHandler == nullptr)
        {
            LOGE("mOnboardclientTxHandler is nullptr");
        }
        return mOnboardclientTxHandler;
    }

    void OnboardclientTxHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
    {
        const int32_t what{handlemsg->what};
        switch (what)
        {
        case OBC_TX_HANDLER::CMD_TX_INIT:
        {
            LOGV("OBC_TX_HANDLER::CMD_TX_INIT");
            break;
        }
        case OBC_TX_HANDLER::CMD_TX_SEND_UDS:
        {
            LOGV("OBC_TX_HANDLER::CMD_TX_SEND_UDS, ConnectId %u", handlemsg->arg1);
            const int32_t tmpConnectId{handlemsg->arg1};
            if ((tmpConnectId < 0) || (tmpConnectId > static_cast<int32_t>(UINT16_MAX)))
            {
                LOGV("invalid ConnectId value");
            }
            else
            {
                const uint16_t connectedId{static_cast<uint16_t>(tmpConnectId)};
                const android::sp<Buffer> udsReqData{new Buffer()};
                const bool res{OnboardclientImpl::getInstance()->getUdsReqFromQueue(connectedId, udsReqData)};
                if (res == true)
                {
                    const sp<CommunicationData> commData{new CommunicationData()};
                    uint32_t txCanId{0U};
                    uint8_t protocolType{0U};
                    (void)OnboardclientImpl::getInstance()->getConnectedCanID(connectedId, protocolType, txCanId);
                    OnboardclientImpl::getInstance()->convertToCommData(txCanId, udsReqData, commData);
                    OnboardclientImpl::getInstance()->setCurrentProcessingId(connectedId);
                    const error_t errCode{CommunicationManagerAdapter::getInstance()->sendUdsDataToMcu(commData)};
                    if (errCode == E_OK)
                    {
                        startTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_ACK_RESPONSE);
                    }
                    else
                    {
                        if (OnboardclientTxHandler::getInstance()->attemptNoAck >= MAX_RETRY_COUNT_NO_ACK)
                        {
                            LOGV("Max retry count for send UDS to Comm");
                            OnboardclientTxHandler::getInstance()->attemptNoAck = 0U;
                            OnboardclientImpl::getInstance()->responseErrorEventToApp(OBCEnum::OBCErrCode::OBC_ERR_SEND_UDS_DATA);
                            (void)OnboardclientImpl::getInstance()->deleteUdsDataFromMap(connectedId);
                        }
                        else
                        {
                            LOGV("Retry send UDS to Comm");
                            OnboardclientTxHandler::getInstance()->attemptNoAck++;
                            (void)mOnboardclientTxHandler->sendMessageDelayed(mOnboardclientTxHandler->obtainMessage(OBC_TX_HANDLER::CMD_TX_SEND_UDS, static_cast<int32_t>(connectedId)), 1000U);
                        }
                    }
                }
                else
                {
                    (void)udsReqData;
                    OnboardclientImpl::getInstance()->processingNextRequest();
                    LOGV("Dont find any request");
                }
            }
            break;
        }
        case OBC_TX_HANDLER::CMD_TX_SEND_ACK_ROB:
        {
            LOGV("OBC_TX_HANDLER::CMD_TX_SEND_ACK_ROB");
            const int32_t tempData{handlemsg->arg1};
            if ((tempData >= 0) && (tempData <= static_cast<int32_t>(UINT8_MAX)))
            {
                const sp<Buffer> payload{new Buffer()};
                const uint8_t data{static_cast<uint8_t>(tempData)};
                payload->setTo(&data, 1);
                (void)CommunicationManagerAdapter::getInstance()->sendUdsDataToMcu(static_cast<uint8_t>(TYPE_REQUEST), OBC_COMM_ENUM::CATEGORY_OBC, OBC_COMM_ENUM::ACK_ROB_DETECT, OBC_COMM_ENUM::CMD2_OBC_DEFAULT, payload);
            }
            else
            {
                LOGV("invalid arg1 value");
            }

            break;
        }
        case OBC_TX_HANDLER::CMD_TX_SEND_UDS_TO_DIAG:
        {
            LOGV("OBC_TX_HANDLER::CMD_TX_SEND_UDS_TO_DIAG, ConnectId %u", handlemsg->arg1);
            const int32_t tmpConnectId{handlemsg->arg1};
            if ((tmpConnectId < 0) || (tmpConnectId > static_cast<int32_t>(UINT16_MAX)))
            {
                LOGV("invalid ConnectId value");
            }
            else
            {
                const uint16_t connectedId{static_cast<uint16_t>(tmpConnectId)};
                const android::sp<Buffer> udsReqData{new Buffer()};
                const bool res{OnboardclientImpl::getInstance()->getUdsReqFromQueue(connectedId, udsReqData)};
                if (res == true)
                {
                    const sp<Buffer> result{new Buffer()};
                    OnboardclientImpl::getInstance()->setCurrentProcessingId(connectedId);
                    OnboardclientImpl::getInstance()->startTimeOutUdsRes(connectedId);
                    const error_t errCode{DiagManagerAdapter::getInstance()->exeUdsRequest(UDS_COMMAND_SOURCE::UDS_COMMAND_SOURCE_ETHERNET, udsReqData, result)};
                    if (errCode == E_OK)
                    {
                        OnboardclientTxHandler::getInstance()->attemptNoAck = 0U;
                        (void)OnboardclientImpl::getInstance()->deleteUdsDataFromMap(connectedId);
                    }
                    else
                    {
                        stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE);
                        if (OnboardclientTxHandler::getInstance()->attemptNoAck >= MAX_RETRY_COUNT_NO_ACK)
                        {
                            LOGV("Max retry count for send UDS to Diag");
                            OnboardclientTxHandler::getInstance()->attemptNoAck = 0U;
                            OnboardclientImpl::getInstance()->responseErrorEventToApp(OBCEnum::OBCErrCode::OBC_ERR_SEND_UDS_DATA);
                            (void)OnboardclientImpl::getInstance()->deleteUdsDataFromMap(connectedId);
                        }
                        else
                        {
                            OnboardclientTxHandler::getInstance()->attemptNoAck++;
                            (void)mOnboardclientTxHandler->sendMessageDelayed(mOnboardclientTxHandler->obtainMessage(OBC_TX_HANDLER::CMD_TX_SEND_UDS_TO_DIAG, static_cast<int32_t>(connectedId)), 1000U);
                        }
                    }
                }
                else
                {
                    (void)connectedId;
                    (void)udsReqData;
                    OnboardclientImpl::getInstance()->processingNextRequest();
                    LOGV("Dont find any request");
                }
            }
            break;
        }
        case OBC_TX_HANDLER::CMD_TX_START_MONITORING_WIRE_CONNECTION:
        {
            LOGV("OBC_TX_HANDLER::CMD_TX_START_MONITORING_WIRE_CONNECTION");
            CommunicationManagerAdapter::getInstance()->requestWireMonitoring();
            startTimer(OnboardclientTxTimerHandler::OBC_TX_TIMER_MONITORING_WIRE_CONNECTION);
            break;
        }
        default:
        {
            break;
        }
        }
    }

    void OnboardclientTxHandler::initTimer(void)
    {
        LOGI("Init timer for retry process");
        mOnboardclientTxTimerHandler = new OnboardclientTxTimerHandler();
        if (mOnboardclientTxTimerHandler != nullptr)
        {
            mTimeOutWireConenction = new Timer(mOnboardclientTxTimerHandler, OnboardclientTxTimerHandler::OBC_TX_TIMER_MONITORING_WIRE_CONNECTION);
            mTimeOutSendUds = new Timer(mOnboardclientTxTimerHandler, OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE);
            mTimeOutSendBusy = new Timer(mOnboardclientTxTimerHandler, OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_BUSY_RESPONSE);
            mTimeOutSendFail = new Timer(mOnboardclientTxTimerHandler, OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_COMMUNICATION_ERROR);
            mTimeOutNoAck = new Timer(mOnboardclientTxTimerHandler, OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_ACK_RESPONSE);
        }
    }

    void OnboardclientTxHandler::startTimer(const int32_t timerId, const uint32_t duration)
    {
        // TimeManager &time_manager = TimeManager::getInstance();
        // const int64_t currentTime{time_manager.getCurrentMilliSec()};

        LOGD("OnboardclientTxHandler::startTimer timerId = %lu, duration = %lu", timerId, duration);

        switch (timerId)
        {
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_MONITORING_WIRE_CONNECTION:
        {
            mTimeOutWireConenction->stop();
            mTimeOutWireConenction->setDuration(20U, 0U);
            mTimeOutWireConenction->start();
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_COMMUNICATION_ERROR:
        {
            mTimeOutSendFail->stop();
            mTimeOutSendFail->setDurationMs(100U, 0U);
            mTimeOutSendFail->start();
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_BUSY_RESPONSE:
        {
            mTimeOutSendBusy->stop();
            mTimeOutSendBusy->setDuration(1U, 0U);
            mTimeOutSendBusy->start();
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE:
        {
            mTimeOutSendUds->stop();
            mTimeOutSendUds->setDuration(duration, 0U);
            mTimeOutSendUds->start();
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_ACK_RESPONSE:
        {
            mTimeOutNoAck->stop();
            mTimeOutNoAck->setDuration(1U, 0U);
            mTimeOutNoAck->start();
            break;
        }
        default:
            break;
        }
    }

    void OnboardclientTxHandler::stopTimer(const int32_t timerId)
    {
        LOGI("Stop timer with timerId = %d", timerId);
        switch (timerId)
        {
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_MONITORING_WIRE_CONNECTION:
        {
            mTimeOutWireConenction->stop();
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_COMMUNICATION_ERROR:
        {
            mTimeOutSendFail->stop();
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_BUSY_RESPONSE:
        {
            mTimeOutSendBusy->stop();
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE:
        {
            mTimeOutSendUds->stop();
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_ACK_RESPONSE:
        {
            mTimeOutNoAck->stop();
            break;
        }
        default:
            break;
        }
    }

    OnboardclientTxHandler::OnboardclientTxTimerHandler::OnboardclientTxTimerHandler() noexcept
    {
    }

    void OnboardclientTxHandler::OnboardclientTxTimerHandler::handlerFunction(const int32_t timerId)
    {
        LOGV("OnboardclientTxTimerHandler is received");
        switch (timerId)
        {
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_MONITORING_WIRE_CONNECTION:
        {
            LOGV("OnboardclientTxTimerHandler OBC_TX_TIMER_MONITORING_WIRE_CONNECTION");
            (void)mOnboardclientTxHandler->obtainMessage(OBC_TX_HANDLER::CMD_TX_START_MONITORING_WIRE_CONNECTION)->sendToTarget();
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_COMMUNICATION_ERROR:
        {
            LOGV("OnboardclientTxTimerHandler OBC_TX_TIMER_SEND_UDS_COMMUNICATION_ERROR");
            // check and start timer again
            const uint16_t connectedId{OnboardclientImpl::getInstance()->getCurrentProcessingId()};
            if (connectedId != 0U)
            {
                (void)OnboardclientTxHandler::getInstance()->obtainMessage(OBC_TX_HANDLER::CMD_TX_SEND_UDS, static_cast<int32_t>(connectedId))->sendToTarget();
            }
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_SEND_UDS_BUSY_RESPONSE:
        {
            LOGV("OnboardclientTxTimerHandler OBC_TX_TIMER_SEND_UDS_BUSY_RESPONSE");
            // check and start timer again
            const uint16_t connectedId{OnboardclientImpl::getInstance()->getCurrentProcessingId()};
            if (connectedId != 0U)
            {
                (void)OnboardclientTxHandler::getInstance()->obtainMessage(OBC_TX_HANDLER::CMD_TX_SEND_UDS, static_cast<int32_t>(connectedId))->sendToTarget();
            }

            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE:
        {
            LOGV("OnboardclientTxTimerHandler OBC_TX_TIMER_TIMEOUT_RESPONSE");
            OnboardclientImpl::getInstance()->responseErrorEventToApp(OBCEnum::OBCErrCode::OBC_TIMEOUT);
            OnboardclientImpl::getInstance()->processingNextRequest();
            break;
        }
        case OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_ACK_RESPONSE:
        {
            LOGV("OnboardclientTxTimerHandler OBC_TX_TIMER_TIMEOUT_ACK_RESPONSE");
            // check and start timer again
            const uint16_t connectedId{OnboardclientImpl::getInstance()->getCurrentProcessingId()};
            if ((connectedId != 0U))
            {
                if (OnboardclientTxHandler::getInstance()->attemptNoAck < MAX_RETRY_COUNT_NO_ACK)
                {
                    OnboardclientTxHandler::getInstance()->attemptNoAck++;
                    (void)OnboardclientTxHandler::getInstance()->obtainMessage(OBC_TX_HANDLER::CMD_TX_SEND_UDS, static_cast<int32_t>(connectedId))->sendToTarget();
                }
                else
                {
                    OnboardclientTxHandler::getInstance()->attemptNoAck = 0U;
                    OnboardclientImpl::getInstance()->responseErrorEventToApp(OBCEnum::OBCErrCode::OBC_ERR_SEND_UDS_DATA);
                    (void)OnboardclientImpl::getInstance()->deleteUdsDataFromMap(connectedId);
                }
            }
            break;
        }
        default:
        {
            LOGV("OnboardclientTxTimerHandler Undefined TimerId");
            break;
        }
        }
    }
    void OnboardclientTxHandler::resetCounterNoAck()
    {
        attemptNoAck = 0U;
    }
};
