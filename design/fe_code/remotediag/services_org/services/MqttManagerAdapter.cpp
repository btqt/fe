#include "MqttManagerAdapter.h"
#include "sldd/RemoteDiagSLDD.h"

#include "services/DiagManagerAdapter.h"

namespace rdgapp {

android::sp<MqttManagerAdapter> MqttManagerAdapter::mMqttManagerAdapter{nullptr};
android::Mutex MqttManagerAdapter::mInstanceLock{};
MqttManagerAdapter::MqttManagerAdapter() : android::RefBase()
{
    mMqttManagerAdapter = this;
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who)
                                                       { this->onBinderDied(who); });
}

MqttManagerAdapter::~MqttManagerAdapter()
{
    if (MqttManagerAdapter::mMqttManagerAdapter != nullptr)
    {
        mMqttManagerAdapter.clear();
    }
}

android::sp<MqttManagerAdapter> MqttManagerAdapter::getInstance()
{
    if (mMqttManagerAdapter == nullptr)
    {
        const android::AutoMutex _l{mInstanceLock};
        if (mMqttManagerAdapter == nullptr)
        {
            mMqttManagerAdapter = android::sp<MqttManagerAdapter>(new MqttManagerAdapter());
        }
    }
    return mMqttManagerAdapter;
}

void MqttManagerAdapter::registerService()
{
    LOG_I("MqttManagerAdapter::registerService");
    mHandler = RemotediagHandler::getInstance();
    mDcemqttproxy = android::interface_cast<IDcemqttproxyManagerService>(
        android::defaultServiceManager()->getService(android::String16("service_layer.DcemqttproxyManagerService")));
    if (mDcemqttproxy != nullptr)
    {
        LOG_I("MqttManagerAdapter registered");
        if (mDcemqttProxyReceiver != nullptr)
        {
            mDcemqttProxyReceiver.clear();
        }

        mDcemqttProxyReceiver = android::sp<DcemqttproxyReceiver>(new DcemqttproxyReceiver(*this));
        LOG_I("Register DCEMQTTPROXY receiver");
        (void)mDcemqttproxy->registerReceiverDcemqttproxyOnNotifyCb(mDcemqttProxyReceiver, APP_NAME);
        const std::string vinNum{DiagManagerAdapter::getInstance()->getVinNumber()};
        subscribeTopic(vinNum);
#ifdef _MORE_MARSHM
        (void)android::IInterface::asBinder(mDcemqttproxy)->linkToDeath(mServiceDeathRecipient);
#else  // !(_MORE_MARSHM)
        (void)mDcemqttproxy->asBinder()->linkToDeath(mServiceDeathRecipient);
#endif // _MORE_MARSHM
    }
    else
    {
        if (mHandler != nullptr)
        {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_MQTT_MGR),
                                               static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
        else
        {
            LOG_E("MqttManagerAdapter::registerService mHandler = nullptr");
        }
    }
}

void MqttManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who)
{
    LOG_I("MqttManagerAdapter::onBinderDied");
    const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
#ifdef _MORE_MARSHM
    if ((mDcemqttproxy != nullptr) && (android::IInterface::asBinder(mDcemqttproxy) == who))
    {
#else  // !(_MORE_MARSHM)
    if (mDcemqttproxy != nullptr && mDcemqttproxy->asBinder() == who)
    {
#endif // _MORE_MARSHM
        mDcemqttproxy = nullptr;
        if (mHandler != nullptr)
        {
            (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_MQTT_MGR), static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
        }
    }
    else
    {
        // Do nothing
    }
}

void MqttManagerAdapter::subscribeTopic(const std::string vinNum)
{
    std::string topicForSubscribe{""};
    topicForSubscribe = vinNum + "/C2V/DESTSW/remotediagnostics/DCIF-RDG015";
    std::vector<std::string> topicList{};
    topicList.push_back(topicForSubscribe);
    (void)mDcemqttproxy->invokeSubscribeAdd(APP_NAME, topicList);
}

DcemqttproxyReceiver::DcemqttproxyReceiver(MqttManagerAdapter &parent) noexcept : mParent(parent) 
{
}

void DcemqttproxyReceiver::onNotifyCb(const android::sp<DceNotification> message)
{
    mHandler = RemotediagHandler::getInstance();
    if (mHandler != nullptr)
    {
        const android::sp<DceNotification> pDataEvent{new DceNotification()};
        pDataEvent->setData(message->getTopicName(), message->getPayload(), message->getTopicResponse());
        (void)mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_CENTER_PUSH_RECEIVED, pDataEvent)->sendToTarget();
    }
}
}
