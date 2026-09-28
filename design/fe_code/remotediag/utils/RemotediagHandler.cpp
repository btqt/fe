#include "RemotediagHandler.h"

namespace rdgapp {

RemotediagHandler *RemotediagHandler::instance {nullptr};
RemotediagHandler *RemotediagHandler::instance_2 {nullptr};
RemotediagHandler::RemotediagHandler(sp<sl::SLLooper> &looper, Remotediag &app) noexcept : Handler(looper), mApp(app)
{
    instance = this;
    instance_2 = this;
}

RemotediagHandler::~RemotediagHandler() noexcept
{
    if (RemotediagHandler::instance != nullptr)
    {
        delete (RemotediagHandler::instance);
    }
    else
    {
        RemotediagHandler::instance = nullptr;
    }
}

void RemotediagHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
{
    LOG_D("RemotediagHandler::handleMessage");
    mApp.doRemotediagHandler(handlemsg);
}

void RemotediagHandler::init(RemotediagHandler* const handler_ptr)
{
    LOG_I("RemotediagHandler::init");
    instance_2 = handler_ptr;
}

RemotediagHandler *RemotediagHandler::getInstance()
{
    if (instance == nullptr)
    {
        LOG_I("RemotediagHandler instance is nullptr");
    }
    return instance;
}

RemotediagHandler *RemotediagHandler::getInstance_2()
{
    if (instance_2 == nullptr)
    {
        LOG_I("RemotediagHandler instance is nullptr");
    }
    return instance_2;
}
}
