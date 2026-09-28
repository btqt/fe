#ifndef REMOTEDIAG_REMOTEDIAGHANDLER_H
#define REMOTEDIAG_REMOTEDIAGHANDLER_H

#include <utils/Handler.h>
#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "../Remotediag.h"
#include "utils/RemoteDiagDatastore.h"

namespace rdgapp {

class Remotediag;
class RemotediagHandler : public sl::Handler
{
public:
    RemotediagHandler(sp<sl::SLLooper> &looper, Remotediag &app) noexcept;
    virtual ~RemotediagHandler() noexcept;
    RemotediagHandler(RemotediagHandler const &) = default;
    RemotediagHandler &operator=(RemotediagHandler const &) = default;
    RemotediagHandler(RemotediagHandler &&) = delete;
    RemotediagHandler &operator=(RemotediagHandler &&) = delete;
    virtual void handleMessage(const android::sp<sl::Message> &handlemsg);

    static void init(RemotediagHandler* const);
    static RemotediagHandler *getInstance();   // don't use now
    static RemotediagHandler *getInstance_2(); // use this function

private:
    static RemotediagHandler *instance;
    static RemotediagHandler *instance_2;
    Remotediag &mApp;
};
}
#endif /* REMOTEDIAG_REMOTEDIAGHANDLER_H */
