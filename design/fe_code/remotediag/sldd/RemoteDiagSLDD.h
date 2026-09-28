#ifndef REMOTEDIAG_SLDD_H
#define REMOTEDIAG_SLDD_H
#include <utils/RefBase.h>
#include <utils/Message.h>

#include <services/TimeManagerService/TimeManager.h>
#include <services/CommunicationManagerService/ICommunicationManagerService.h>

#include "include/ParamsDef.h"
#include "utils/RemotediagHandler.h"
#include "utils/UploadManager.h"
#include "diagprocess/DirectCommand/RemoteDirectCommand.h"
#include "diagprocess/EcuInformation/RemoteEcuInformation.h"
#include "diagprocess/Warning/RemoteWarning.h"
#include "diagprocess/DTC/RemoteDTC.h"
#include "diagprocess/TriggerIDGenerator.h"
#include "diagprocess/OTA/FaClient.h"
#include "diagprocess/RoBMonitoring/RoBMonitoring.h"
#include "services/VehicleManagerAdapter.h"
#include "services/DiagManagerAdapter.h"
#include "PriorityControl.h"
#include "DiagTrigger.h"

namespace rdgapp {

class RemotediagHandler;

class RemoteDiagSLDD : public android::RefBase
{
public:
    RemoteDiagSLDD();
    virtual ~RemoteDiagSLDD();
    RemoteDiagSLDD(RemoteDiagSLDD const &) = default;
    RemoteDiagSLDD &operator=(RemoteDiagSLDD const &) = default;
    RemoteDiagSLDD(RemoteDiagSLDD &&) = delete;
    RemoteDiagSLDD &operator=(RemoteDiagSLDD &&) = delete;
    static android::sp<RemoteDiagSLDD> getInstance();
    void runRemoteDiagSlddTesting(const int32_t what, const int32_t arg1 = 0, const int32_t arg2 = 0);

private:
    android::sp<RemotediagHandler> mHandler = nullptr;
    uint16_t mCurrentConnectId;
    static android::sp<RemoteDiagSLDD> mRemoteDiagSLDD;
    FaClient mFa;
};
}
#endif /* REMOTEDIAG_SLDD_H */
