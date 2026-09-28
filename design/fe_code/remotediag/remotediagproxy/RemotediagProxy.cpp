#include "RemotediagProxy.h"
#include "services/ApplicationManagerAdapter.h"
#include "services/CalibManagerAdapter.h"
#include "services/DiagManagerAdapter.h"
#include "services/FileStoreAdapter.h"
#include "services/HttpManagerAdapter.h"
#include "services/LocationManagerAdapter.h"
#include "services/MqttManagerAdapter.h"
#include "services/SomeipManagerAdapter.h"
#include "services/PPIManagerAdapter.h"
#include "services/PowerManagerAdapter.h"
#include "services/VehicleManagerAdapter.h"
#include "services/RegionManagerAdapter.h"
#include "services/TimeManagerAdapter.h"
#include "services/TelephonyManagerAdapter.h"
#include "utils/ProxyIpcClient.h"

android::sp<Application> gApp{};

android::sp<RemotediagProxy> RemotediagProxy::mRemotediagProxyInstance {nullptr};
RemotediagProxy::RemotediagProxy()
{
    mRemotediagProxyInstance = this;
    initDLTLog();
}

void RemotediagProxy::onCreate()
{
    LOG_I("RemotediagProxy is onCreate");
    mLooper = sl::SLLooper::myLooper();
    mLooper->setName({"RemoteDiagProxy"});
    mRemotediagProxyHandler = new RemotediagProxyHandler(mLooper);
    mProxyIpcClient.reset(new ProxyIpcClient());

    (void)ApplicationManagerAdapter::getInstance();
    (void)LocationManagerAdapter::getInstance();
    (void)PowerManagerAdapter::getInstance();
    (void)DiagManagerAdapter::getInstance();
    (void)CalibManagerAdapter::getInstance();
    (void)FileStoreAdapter::getInstance();
    (void)HttpManagerAdapter::getInstance();
    (void)MqttManagerAdapter::getInstance();
    (void)SomeipManagerAdapter::getInstance();
    (void)PPIManagerAdapter::getInstance();
    (void)VehicleManagerAdapter::getInstance();
    (void)RegionManagerAdapter::getInstance();
    (void)TimeManagerAdapter::getInstance();
    (void)TelephonyManagerAdapter::getInstance();

    (void)mProxyIpcClient->start();

    ApplicationManagerAdapter::getInstance()->registerService();
    LocationManagerAdapter::getInstance()->registerService();
    PowerManagerAdapter::getInstance()->registerService();
    DiagManagerAdapter::getInstance()->registerService();
    CalibManagerAdapter::getInstance()->registerService();
    HttpManagerAdapter::getInstance()->registerService();
    MqttManagerAdapter::getInstance()->registerService();
    SomeipManagerAdapter::getInstance()->registerService();
    PPIManagerAdapter::getInstance()->registerService();
    VehicleManagerAdapter::getInstance()->registerService();
    RegionManagerAdapter::getInstance()->registerService();
}

void RemotediagProxy::onDestroy() 
{
    LOG_I("Remotediag is onDestroy");
    if (mProxyIpcClient != nullptr)
    {
        mProxyIpcClient->stop();
        mProxyIpcClient.reset(nullptr);
    }
}


android::sp<RemotediagProxy> RemotediagProxy::getInstance()
{
    if (mRemotediagProxyInstance == nullptr)
    {
        LOG_I("mRemotediagProxyInstance is nullptr");
    }
    return mRemotediagProxyInstance;
}

void RemotediagProxy::onPostReceived(const android::sp<::Post> &systemPost)
{
    const int32_t what{systemPost->what};
    LOG_I("onPostReceived %d", what);
}

error_t RemotediagProxy::onFeatureActionPerformed(const FeatureAction action, const std::string feature)
{
    return E_OK;
}

error_t RemotediagProxy::onFeatureStatusChanged(const std::string feature, const FeatureStatus status)
{
    return E_OK;
}

#ifdef __cplusplus
extern "C" class Application *createApplication()
{
    (void)printf("create RemotediagProxy");
    gApp = new RemotediagProxy;
    return gApp.get();
}

extern "C" void destroyApplication(class Application *const application)
{
    delete (RemotediagProxy *)application;
}
#endif
