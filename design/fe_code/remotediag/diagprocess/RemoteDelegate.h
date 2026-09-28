#ifndef REMOTEDIAG_DELEGATE_H
#define REMOTEDIAG_DELEGATE_H

#include <cstdint>
#include <map>
#include <memory>
#include <deque>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <Error.h>
#include <string>
#include <utils/StrongPointer.h>
#include <utils/external/mindroid/lang/String.h>

#include <services/OnboardclientManagerService/OBCEnum.h>
#include <services/OnboardclientManagerService/OBCCanInfo.h>
#include <services/OnboardclientManagerService/OBCResponseEventInfo.h>
#include "diagprocess/UDS/UdsMessage.h"
#include "diagprocess/CenterReqData.h"
#include "diagprocess/CenterReqDataType.h"

namespace rdgapp {

class CenterReqData;

class RemoteDelegate {
public:

    using Interator = std::unordered_map<uint8_t, RemoteDelegate*>::iterator;

    RemoteDelegate() = default;
    ~RemoteDelegate() = default;
    RemoteDelegate(RemoteDelegate const&) = delete;
    RemoteDelegate(RemoteDelegate&&) = delete;
protected:
    RemoteDelegate& operator=(RemoteDelegate const&) = delete;
    RemoteDelegate& operator=(RemoteDelegate&&) = delete;
    virtual bool checkPrecondition()  = 0;
public:
    virtual void notifyBootComplete() const = 0;
    virtual void onReceiveIG(const bool status) const = 0;
    virtual void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse ) = 0;
    virtual void onChangedRemoteInfo(const int32_t what, const int32_t info = 0) = 0;
    virtual void onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) = 0;
    virtual void onRdgStop(const bool isStop) const = 0;
    virtual uint8_t getAppId() const = 0;
    virtual std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept = 0;
};
}
#endif // REMOTEDIAG_DELEGATE_H
