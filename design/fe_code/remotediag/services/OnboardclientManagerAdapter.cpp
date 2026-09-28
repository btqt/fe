#include "OnboardclientManagerAdapter.h"
#include "sldd/RemoteDiagSLDD.h"

#include "services/DiagManagerAdapter.h"
#include "diagprocess/OTA/RemoteOTA.h"

namespace rdgapp {

android::sp<OnboardclientAdapter> OnboardclientAdapter::mOnboardclientAdapter{nullptr};
OnboardclientAdapter::OnboardclientAdapter() : android::RefBase()
{
    mOnboardclientAdapter = this;
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who)
                                                       { this->onBinderDied(who); });
    initTimer();
}

OnboardclientAdapter::~OnboardclientAdapter() noexcept
{
    if (OnboardclientAdapter::mOnboardclientAdapter != nullptr)
    {
        mOnboardclientAdapter.clear();
    }
}

android::sp<OnboardclientAdapter> OnboardclientAdapter::getInstance()
{
    if (mOnboardclientAdapter == nullptr)
    {
        mOnboardclientAdapter = android::sp<OnboardclientAdapter>(new OnboardclientAdapter());
    }
    return mOnboardclientAdapter;
}

void OnboardclientAdapter::registerService()
{
    LOG_I("OnboardclientAdapter::registerService");
    mHandler = RemotediagHandler::getInstance();
    mOnboardclient = android::interface_cast<IOnboardclientManagerService>(
        android::defaultServiceManager()->getService(android::String16("service_layer.OnboardclientManagerService")));
    if (mOnboardclient != nullptr)
    {
        LOG_I("OnboardclientAdapter registered");
        if (mOnboardClientReceiver != nullptr)
        {
            mOnboardClientReceiver.clear();
        }

        mOnboardClientReceiver = android::sp<OnboardClientReceiver>(new OnboardClientReceiver(*this));
        LOG_I("Register DCEMQTTPROXY receiver");
        (void)mOnboardclient->registerReceiverOnboardClientReceiverOnNotifyOBD2Event(mOnboardClientReceiver);
        (void)mOnboardclient->registerReceiverOnboardClientReceiverOnResponseEvent(mOnboardClientReceiver);

#ifdef _MORE_MARSHM
        (void)android::IInterface::asBinder(mOnboardclient)->linkToDeath(mServiceDeathRecipient);
#else  // !(_MORE_MARSHM)
        (void)mOnboardclient->asBinder()->linkToDeath(mServiceDeathRecipient);
#endif // _MORE_MARSHM
    }
    else
    {
        if (mHandler != nullptr)
        {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_OBC_MGR),
                                               static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
        else
        {
            LOG_E("OnboardclientAdapter::registerService mHandler = nullptr");
        }
    }
}

void OnboardclientAdapter::onBinderDied(const android::wp<android::IBinder> &who)
{
    LOG_I("OnboardclientAdapter::onBinderDied");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
#ifdef _MORE_MARSHM
    if ((mOnboardclient != nullptr) && (android::IInterface::asBinder(mOnboardclient) == who))
    {
#else  // !(_MORE_MARSHM)
    if (mOnboardclient != nullptr && mOnboardclient->asBinder() == who)
    {
#endif // _MORE_MARSHM
        mOnboardclient = nullptr;
        if (mOnboardClientReceiver != nullptr)
        {
            mOnboardClientReceiver.clear();
        }
        if (mHandler != nullptr)
        {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_OBC_MGR), static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
    }
    else
    {
        // Do nothing
    }
}

uint8_t OnboardclientAdapter::sendUdsData(const uint16_t connectId, const android::sp<::Buffer> udsRequest)
{
    LOG_I("OnboardclientAdapter::sendUdsData");
    mOnboardclient = android::interface_cast<IOnboardclientManagerService>(
        android::defaultServiceManager()->getService(android::String16("service_layer.OnboardclientManagerService")));
    uint8_t res{2U};
    if (mOnboardclient != nullptr)
    {
        LOG_MEM_DUMP_D(udsRequest);
        res = mOnboardclient->sendUdsData(connectId, udsRequest);
    }
    LOG_I("OnboardclientAdapter::sendUdsData res %u", res);
    return res;
}

uint8_t OnboardclientAdapter::disconnectECU(const uint16_t connectionID)
{
    mOnboardclient = android::interface_cast<IOnboardclientManagerService>(
        android::defaultServiceManager()->getService(android::String16("service_layer.OnboardclientManagerService")));
    uint8_t res{0U};
    if (mOnboardclient != nullptr)
    {
        res = mOnboardclient->disconnect(connectionID);
        LOG_I("Return value %d", res);
    }
    return res;
}

error_t OnboardclientAdapter::connect(const android::sp<OBCTransportInfo> transportInfo, const std::string appNames, android::sp<OBCConnectInfo> connectInfo) noexcept
{
    mOnboardclient = android::interface_cast<IOnboardclientManagerService>(
        android::defaultServiceManager()->getService(android::String16("service_layer.OnboardclientManagerService")));
    error_t res{E_OK};
    if (mOnboardclient != nullptr)
    {
        res = mOnboardclient->connect(transportInfo, appNames, connectInfo);
        // LOG_I("Return value %d, %d", res.response, res.connectId);
    }
    return res;
}

void OnboardclientAdapter::initTimer(void)
{
    LOGI("Init timer for retry process");
    mOnboardclientManagerAdapterTimer = new OnboardclientManagerAdapterTimer();
    if (mOnboardclientManagerAdapterTimer != nullptr)
    {
        if (OnboardclientManagerAdapterTimer::OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION <= 2147483647U)
        {
            mTimeOutWireConenction = new Timer(mOnboardclientManagerAdapterTimer, static_cast<int32_t>(OnboardclientManagerAdapterTimer::OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION));
        }
        else
        {
            // do nothing
        }
    }
}

void OnboardclientAdapter::startTimer(const uint32_t timerId)
{
    // TimeManager &time_manager = TimeManager::getInstance();
    // const int64_t currentTime{time_manager.getCurrentMilliSec()};

    LOG_D("OnboardclientAdapter::startTimer timerId = %d ", timerId);
    switch (timerId)
    {
    case OnboardclientManagerAdapterTimer::OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION:
    {
        mTimeOutWireConenction->stop();
        mTimeOutWireConenction->setDuration(5U, 0U);
        mTimeOutWireConenction->start();
        break;
    }
    default:
        break;
    }
}

void OnboardclientAdapter::stopTimer(const uint32_t timerId)
{
    LOG_I("Stop timer with timerId = %d", timerId);
    switch (timerId)
    {
    case OnboardclientManagerAdapterTimer::OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION:
    {
        mTimeOutWireConenction->stop();
        break;
    }
    default:
        break;
    }
}

OnboardclientAdapter::OnboardclientManagerAdapterTimer::OnboardclientManagerAdapterTimer() noexcept
{
}

void OnboardclientAdapter::OnboardclientManagerAdapterTimer::handlerFunction(const int32_t timerId)
{
    switch (timerId)
    {
    case OnboardclientManagerAdapterTimer::OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION:
    {
        LOGV("OnboardclientManagerAdapterTimer OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION");
        (void)RemotediagHandler::getInstance()->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_OBC_OBD_EVENT_RECEIVED, 0)->sendToTarget();
        break;
    }
    default:
    {
        LOGV("OnboardclientManagerAdapterTimer Undefined TimerId");
        break;
    }
    }
}

OnboardClientReceiver::OnboardClientReceiver(OnboardclientAdapter &parent) noexcept : mParent(parent)
{
}

void OnboardClientReceiver::onNotifyOBD2Event() noexcept
{
    mHandler = RemotediagHandler::getInstance();
    if (mHandler != nullptr)
    {
        OnboardclientAdapter::getInstance()->startTimer(OnboardclientAdapter::OnboardclientManagerAdapterTimer::OBC_ADAPTER_TIMER_MONITORING_WIRE_CONNECTION);
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_OBC_OBD_EVENT_RECEIVED, 1)->sendToTarget();
    }
}

void OnboardClientReceiver::onResponseEvent(const android::sp<OBCResponseEventInfo> message) noexcept
{
    mHandler = RemotediagHandler::getInstance();
    if (mHandler != nullptr)
    {
        const android::sp<OBCResponseEventInfo> pDataEvent{new OBCResponseEventInfo()};
        // pDataEvent->setData(*message);
        pDataEvent->setData(message->errCode(), message->resInfo());
        LOG_I("errCode %u, UDS size %d", pDataEvent->errCode(), pDataEvent->resInfo()->udsData()->size());
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_OBC_UDS_RESPONSE_RECEIVED, pDataEvent)->sendToTarget();
    }
}

void OnboardclientAdapter::TakeObcResource(void)
{
    LOG_D("TakeObcResource");
    SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_WAIT);
}

void OnboardclientAdapter::ReleaseObcResource(void)
{
    // RDG30-R-0916, RDG30-R-0917
    LOG_D("ReleaseObcResource");
    if (RemoteOTA::getInstance().isWaitingObcResource())
    {
        RemoteOTA::getInstance().ocbResourceEventNotify(OBCResourceEventCode::OBC_RELEASE_RESOURCE_COMPLETE);
    }

    SetObcResource(OBCResourceEventCode::OBC_GET_RESOURCE_OK);
}

void OnboardclientAdapter::SetObcResource(const OBCResourceEventCode status)
{
    LOG_D("SetObcResource, %d", status);
    obcResourceStatus = status;
}
}
