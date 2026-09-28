#ifndef REMOTEDIAGPROXY_REMOTEDIAGHANDLER_H
#define REMOTEDIAGPROXY_REMOTEDIAGHANDLER_H

#include <utils/Handler.h>
#include "RemotediagProxy.h"


class RemotediagProxy;
class RemotediagProxyHandler : public sl::Handler
{
public:
    RemotediagProxyHandler(sp<sl::SLLooper> &looper) noexcept;
    ~RemotediagProxyHandler() override = default;
    RemotediagProxyHandler(RemotediagProxyHandler const &) = default;
    RemotediagProxyHandler &operator=(RemotediagProxyHandler const &) = default;
    RemotediagProxyHandler(RemotediagProxyHandler &&) = delete;
    RemotediagProxyHandler &operator=(RemotediagProxyHandler &&) = delete;
    virtual void handleMessage(const android::sp<sl::Message> &handlemsg);
    static android::sp<RemotediagProxyHandler> getInstance();

private:
    static android::sp<RemotediagProxyHandler> mRemotediagProxyHandlerInstance;
};
#endif /* REMOTEDIAGPROXY_REMOTEDIAGHANDLER_H */
