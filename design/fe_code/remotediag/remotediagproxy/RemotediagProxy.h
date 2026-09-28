#ifndef REMOTEDIAGPROXY_APPLICATION_H
#define REMOTEDIAGPROXY_APPLICATION_H

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <deque>
#include <iostream>
#include <atomic>

#include <fstream>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <Error.h>
#include <binder/IServiceManager.h>
#include <utils/Handler.h>
#include "utils/Post.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <application/Application.h>
#include "utils/RemotediagProxyHandler.h"

#include <fstream>
#include <iostream>
#include <string>

#include "utils/Logger.h"

class RemotediagProxyHandler;
class ProxyIpcClient;

class RemotediagProxy : public Application
{
public:

    RemotediagProxy();
    RemotediagProxy(RemotediagProxy const &) = default;
    RemotediagProxy &operator=(RemotediagProxy const &) = default;
    RemotediagProxy(RemotediagProxy &&) = delete;
    RemotediagProxy &operator=(RemotediagProxy &&) = delete;
    static android::sp<RemotediagProxy> getInstance();
    /**
     * Application has two lifecycle method, onCreate() and onDestroy()
     */
    void onCreate() override;
    void onDestroy() override;

    virtual void onPostReceived(const android::sp<::Post> &systemPost);
    error_t onFeatureActionPerformed(const FeatureAction action, const std::string feature) override;
    error_t onFeatureStatusChanged(const std::string feature, const FeatureStatus status) override;
 
private:
    static android::sp<RemotediagProxy> mRemotediagProxyInstance;
    android::sp<RemotediagProxyHandler> mRemotediagProxyHandler;
    android::sp<sl::SLLooper> mLooper;
    std::unique_ptr<ProxyIpcClient> mProxyIpcClient;
};
#endif /* REMOTEDIAGPROXY_APPLICATION_H */
