#include "RemotediagHandler.h"

namespace rdgapp {

android::sp<RemotediagHandler> RemotediagHandler::instance {nullptr};
RemotediagHandler::RemotediagHandler(sp<sl::SLLooper> &looper, Remotediag &app) noexcept : Handler(looper), mApp(app)
{
    instance = this;
}

void RemotediagHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
{
    mApp.doRemotediagHandler(handlemsg);
}

android::sp<RemotediagHandler> RemotediagHandler::getInstance()
{
    if (instance == nullptr)
    {
        LOG_I("RemotediagHandler instance is nullptr");
    }
    return instance;
}

android::sp<RemotediagHandler> RemotediagHandler::getInstance_2()
{
    if (instance == nullptr)
    {
        LOG_I("RemotediagHandler instance is nullptr");
    }
    return instance;
}
}
