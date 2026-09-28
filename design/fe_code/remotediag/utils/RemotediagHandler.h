#ifndef REMOTEDIAG_REMOTEDIAGHANDLER_H
#define REMOTEDIAG_REMOTEDIAGHANDLER_H

#include <utils/Handler.h>
#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "../Remotediag.h"

namespace rdgapp {

class Remotediag;
class RemotediagHandler : public sl::Handler
{
public:
    RemotediagHandler(sp<sl::SLLooper> &looper, Remotediag &app) noexcept;
    ~RemotediagHandler() override = default;
    RemotediagHandler(RemotediagHandler const &) = default;
    RemotediagHandler &operator=(RemotediagHandler const &) = default;
    RemotediagHandler(RemotediagHandler &&) = delete;
    RemotediagHandler &operator=(RemotediagHandler &&) = delete;
    virtual void handleMessage(const android::sp<sl::Message> &handlemsg);

    static android::sp<RemotediagHandler> getInstance();
    static android::sp<RemotediagHandler> getInstance_2();

private:
    static android::sp<RemotediagHandler> instance;
    Remotediag &mApp;
};
}
#endif /* REMOTEDIAG_REMOTEDIAGHANDLER_H */
