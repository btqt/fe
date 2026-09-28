#include "OnboardclientRxHandler.h"
#include "OnboardclientTxHandler.h"
#include "CommunicationManagerAdapter.h"
#include "OnboardclientImpl.h"
namespace OBC
{
    OnboardclientRxHandler *OnboardclientRxHandler::mOnboardclientRxHandler{nullptr};

    OnboardclientRxHandler::OnboardclientRxHandler(sp<sl::SLLooper> &looper, const android::sp<OnboardclientManagerService> onboardclientMgrService)
        : Handler(looper), mOnboardclientMgrService(onboardclientMgrService)
    {
        mOnboardclientRxHandler = this;
    }

    OnboardclientRxHandler *OnboardclientRxHandler::getInstance()
    {
        if (mOnboardclientRxHandler == nullptr)
        {
            LOGE("mOnboardclientRxHandler is nullptr");
        }
        return mOnboardclientRxHandler;
    }

    void OnboardclientRxHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
    {
        const int32_t what{handlemsg->what};
        switch (what)
        {
        case OBC_RX_HANDLER::CMD_RX_INIT:
        {
            LOGI("OBC_RX_HANDLER::CMD_RX_INIT");
            break;
        }
        case OBC_RX_HANDLER::MSG_OBC_RECEIVE_FROM_COMM:
        {
            LOGV("OBC_RX_HANDLER::MSG_OBC_RECEIVE_FROM_COMM");
            sp<CommunicationData> commData{nullptr};
            handlemsg->getObject(commData);
            const uint8_t commCmd{commData->cmd};
            switch (commCmd)
            {
            case RESPONSE_INITIALIZE:
            {
                LOGV("RESPONSE_INITIALIZE");
                OnboardclientTxHandler::getInstance()->stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_MONITORING_WIRE_CONNECTION);
                break;
            }
            case OBC_COMM_ENUM::RESPONSE_CONNECT:
            {
                LOGV("OBC_COMM_ENUM::RESPONSE_CONNECT");
                OnboardclientImpl::getInstance()->pushToQueue(commData, OBC_COMMON::COMM_RESPONSE_TYPE::COMM_RES_CONNECT);
                break;
            }
            case OBC_COMM_ENUM::RESPONSE_DISCONNECT:
            {
                LOGV("OBC_COMM_ENUM::RESPONSE_DISCONNECT");
                OnboardclientImpl::getInstance()->pushToQueue(commData, OBC_COMMON::COMM_RESPONSE_TYPE::COMM_RES_DISCONNECT);
                break;
            }
            case OBC_COMM_ENUM::ACK_UDS_REQUEST:
            {
                LOGV("OBC_COMM_ENUM::ACK_UDS_REQUEST");
                OnboardclientImpl::getInstance()->pushToQueue(commData, OBC_COMMON::COMM_RESPONSE_TYPE::COMM_RES_ACK_UDS_REQUEST);
                break;
            }
            case OBC_COMM_ENUM::SEND_UDS_RESPONSE:
            {
                LOGV("OBC_COMM_ENUM::ACK_UDS_REQUEST");
                OnboardclientImpl::getInstance()->pushToQueue(commData, OBC_COMMON::COMM_RESPONSE_TYPE::COMM_RES_UDS_REQUEST);
                break;
            }
            case WIRED_CONNECTION_PROGRESS:
            {
                /* (1) 1: True (Detect wired connection)     */
                /* (2) 0: False (Detect wired disconnection) */
                const uint8_t isObd{commData->payload->data()[0]};
                if (isObd == 1U)
                {
                    LOGV(":WIRED_CONNECTION_PROGRESS");
                    (void)OnboardclientManagerService::instance()->queryReceiverByOnboardClientReceiverOnNotifyOBD2Event();
                }
                else
                {
                    /*do nothing*/
                }
                break;
            }
            default:
            {
                break;
            }
            }
            break;
        }
        case OBC_RX_HANDLER::MSG_OBC_RECEIVE_FROM_DIAG:
        {
            sp<Buffer> udsResp{new Buffer()};
            handlemsg->getObject(udsResp);
            OnboardclientTxHandler::getInstance()->stopTimer(OnboardclientTxHandler::OnboardclientTxTimerHandler::OBC_TX_TIMER_TIMEOUT_RESPONSE);
            OnboardclientImpl::getInstance()->handleUdsRespFromDiagMgr(udsResp);
            break;
        }
        default:
        {
            break;
        }
        }
    }
};
