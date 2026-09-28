#ifndef RDG_LAST_UPLOAD
#define RDG_LAST_UPLOAD

#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <unordered_map>
#include <list>
#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>

#include <services/TimeManagerService/TimeManager.h>
#include <services/OnboardclientManagerService/IOnboardclientManagerService.h>
#include <services/OnboardclientManagerService/IOnboardClientReceiver.h>
#include <services/OnboardclientManagerService/OBCResponseEventInfo.h>
#include <services/OnboardclientManagerService/OBCUDSResInfo.h>
#include <services/OnboardclientManagerService/OBCCanInfo.h>
#include <services/OnboardclientManagerService/OBCTransportInfo.h>
#include "services/HttpManagerAdapter.h"
#include "diagprocess/EcuInformation/RemoteEcuInformation.h"
#include "diagprocess/EcuInformation/EcuInformationDataType.h"

#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "RemoteDelegate.h"
#include "DiagTrigger.h"
#include "Remotediag.h"
#include "utils/PriorityControl.h"
#include "UdsMessage.h"
#include "CRCManager.h"
#include "utils/UploadManager.h"
#include "utils/UploadTask.h"
#include "DataModel.h"

class Remotediag;
// class CRCManager;
// class UploadTask;

namespace rdgapp {

class RemoteLastUpload : public android::RefBase {
public:
    RemoteLastUpload(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper);
    ~RemoteLastUpload() override;
    RemoteLastUpload(const RemoteLastUpload&) = default;
    RemoteLastUpload(RemoteLastUpload&&) = default;
    RemoteLastUpload& operator=(const RemoteLastUpload&) = default;
    RemoteLastUpload& operator=(RemoteLastUpload&&) = default;
    // static RemoteLastUpload* getInstance(void);
    // bool notifyTrigger(const DiagTrigger::DiagTriggerState& pState,
    //     const int32_t& pTriggerId, const bool dueToIgOff);
    // void handleTrigger(const DiagTrigger::DiagTriggerState& pState,
    //     const int32_t& pTriggerId, const bool dueToIgOff);
    void triggerLastUpload(const DiagTrigger::DiagTriggerType triggerType, const int64_t time, const android::sp<CommonDefine::RDGLocationData> location);
    void triggerLastUpload(const DiagTrigger::DiagTriggerType triggerType
    , const int64_t timeData
    , const android::sp<CommonDefine::RDGLocationData> location
    , const uint64_t collectionId
    , const uint32_t priority);
    void onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData);
private:
    void printData(const std::string data) const;
    void trigger_LU(const DiagTrigger::DiagTriggerType type, const uint32_t prio, const uint64_t colId, const int64_t time);
    void makeUploadData();
    class MainHandler : public sl::Handler {
    public:
        static constexpr int32_t CMD_TRIGGER_FROM_CENTER {2002};
        static constexpr int32_t CMD_TRIGGER_FROM_IGON {2003};
        static constexpr int32_t CMD_TRIGGER_FROM_WARNING {2004};
        static constexpr int32_t CMD_MAKE_DATA {2005};
        explicit MainHandler(android::sp<sl::SLLooper>& privateLooper, RemoteLastUpload &uploadData) noexcept
                : android::RefBase(), sl::Handler(privateLooper), mLU(uploadData) {}
        ~MainHandler() override = default;
        MainHandler(const MainHandler&) = default;
        MainHandler(MainHandler&&) = default;
        MainHandler& operator=(const MainHandler&) = default;
        MainHandler& operator=(MainHandler&&) = default;
        void handleMessage (const android::sp<sl::Message>& handlemsg) override;
    private:
        RemoteLastUpload& mLU;
    };

    const Remotediag& mApp;
    android::sp<MainHandler> mHandler;
    //static RemoteLastUpload *mRemoteLastUpload;
    std::shared_ptr<::vccomif::rdg::v1::interfaces::UploadLastDataRequest> mLastUploadDataReq;
    int32_t mTriggerId;
    DiagTrigger::DiagTriggerType mTriggerType;
    uint32_t mPriority;
    uint64_t mColId;
    android::sp<CommonDefine::RDGLocationData> mLocationData;
    uint64_t mWarningTriggerOccurrenceTime;
    uint64_t mDiagnosticsAcquisitionTime;
    std::unordered_map<uint32_t, android::sp<DiagTrigger>> mSaveReq;
};
}
#endif // RDG_LAST_UPLOAD
