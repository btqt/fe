#ifndef REMOTEDIAG_APPLICATION_H
#define REMOTEDIAG_APPLICATION_H

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

#include <application/Application.h>
#ifndef ENABLE_LGE_LXC
#include <services/ApplicationManagerService/ISystemPostReceiver.h>
#include <services/ApplicationManagerService/IApplicationManagerService.h>
#endif /* ENABLE_LGE_LXC */
#include <services/TimeManagerService/ITimeManagerServiceType.h>
#include <services/TimeManagerService/TimeManager.h>
#include <services/RegionManagerService/IRegionManagerService.h>
#include <services/RegionManagerService/IRegionManagerServiceType.h>
#include <services/RegionManagerService/RegionManager.h>
#ifdef ENABLE_LGE_LXC
#include <services/PPIManagerService/IPPIManagerServiceType.h>
#endif /* ENABLE_LGE_LXC */
#include <services/PowerManagerService/PowerIndexEnum.h>

#include "include/common_def.h"
#include "ParamsDef.h"
#include "utils/Logger.h"
#include "utils/RemotediagHandler.h"
#include "utils/SchedulerManager.h"
#include "utils/CollectionCondition.h"
#include "utils/UploadManager.h"
#include "DiagQueue.h"
#include "PriorityControl.h"
#include "sldd/RemoteDiagSLDD.h"

// #include "services/AlarmManagerAdapter.h"
#ifdef ENABLE_LGE_LXC
#include "services/ApplicationManagerAdapter.h"
#include "services/CalibManagerAdapter.h"
// #include "services/ConfigurationManagerAdapter.h"
#include "services/DiagManagerAdapter.h"
#include "services/HttpManagerAdapter.h"
#include "services/LocationManagerAdapter.h"
#include "services/MqttManagerAdapter.h"
#include "services/OnboardclientManagerAdapter.h"
#include "services/PowerManagerAdapter.h"
#include "services/PPIManagerAdapter.h"
#include "services/VehicleManagerAdapter.h"
#include "services/SomeipManagerAdapter.h"
// #include "services/CommManagerAdapter.h"
#include "services/RegionManagerAdapter.h"
#else
#include "services_org/services/ApplicationManagerAdapter.h"
#include "services_org/services/CalibManagerAdapter.h"
// #include "services/ConfigurationManagerAdapter.h"
#include "services_org/services/DiagManagerAdapter.h"
#include "services_org/services/HttpManagerAdapter.h"
#include "services_org/services/LocationManagerAdapter.h"
#include "services_org/services/MqttManagerAdapter.h"
#include "services_org/services/OnboardclientManagerAdapter.h"
#include "services_org/services/PowerManagerAdapter.h"
#include "services_org/services/PPIManagerAdapter.h"
#include "services_org/services/VehicleManagerAdapter.h"
#include "services_org/services/SomeipManagerAdapter.h"
// #include "services/CommManagerAdapter.h"
#include "services_org/services/RegionManagerAdapter.h"
#endif /* ENABLE_LGE_LXC */
// #include "services/FirewallManagerAdapter.h"
#include "diagprocess/SSR/RemoteSSR.h"
#include "diagprocess/DTC/RemoteDTC.h"
#include "diagprocess/RoB/RemoteRoB.h"
#include "diagprocess/OTA/RemoteOTA.h"
#include "diagprocess/DirectCommand/RemoteDirectCommand.h"
#include "diagprocess/LastUpload/RemoteLastUpload.h"
#include "diagprocess/TriggerIDGenerator.h"
#include "diagprocess/EcuInformation/RemoteEcuInformation.h"
#include "diagprocess/RoBSSR/RemoteRoBSSR.h"
#include "diagprocess/Warning/RemoteWarning.h"
#include "diagprocess/CenterReqData.h"
#include "diagprocess/CenterReqDataType.h"
#include "diagprocess/RoBOccurrence/RoBOccurrence.h"
#include "diagprocess/RoBMonitoring/RoBMonitoring.h"
#include "CommonUtils.h"



namespace rdgapp {

class RemoteSSR;
class RemoteDTC;
class RemoteRoB;
class RemoteOTA;
class RemoteDirectCommand;
class RemotediagHandler;
class PriorityControl;
class RemoteEcuInformation;
class RemoteRoBSSR;
class SchedulerManager;
class RemoteWarning;
class CenterReqData;
class UploadManager;
class RemoteLastUpload;
class RoBOccurrence;
class RoBMonitoring;

class Remotediag : public Application
{
public:
    static constexpr int32_t WHAT_CHANGED_REPAIR_SATUS{1};

    Remotediag();
    Remotediag(Remotediag const &) = default;
    Remotediag &operator=(Remotediag const &) = default;
    Remotediag(Remotediag &&) = delete;
    Remotediag &operator=(Remotediag &&) = delete;
    static android::sp<Remotediag> getInstance();
    /**
     * Application has two lifecycle method, onCreate() and onDestroy()
     */
    void onCreate() override;
    void onDestroy() override;

    virtual void onPostReceived(const android::sp<::Post> &systemPost);
    error_t onFeatureActionPerformed(const FeatureAction action, const std::string feature) override;
    error_t onFeatureStatusChanged(const std::string feature, const FeatureStatus status) override;
    void doRemotediagHandler(const android::sp<sl::Message> &msg);

    uint8_t getIGStatus() const; /* ON: 1 OFF: 0*/
    bool getInternalIGStatus() const noexcept;
    uint8_t getUnderRepair() const noexcept;
    void setUnderRepair(const uint8_t status) noexcept;
    void notifyReceiveIG(const bool status);
    void notifyReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo);
    void onNotifyStatus(const DiagTrigger &pDiagTrigger, const bool dueToIgOff = false) const;
    void notifyTriggerStatus(const DiagTrigger::DiagTriggerFunc &pFunc,
                             const DiagTrigger::DiagTriggerState &pState,
                             const int32_t &pTriggerId, const bool dueToIgOff = false);
    void notifyReceiveGrpcRes(const android::sp<GrpcResData> &pGrpcResData) const;
    void notifyChangedRemoteStatus(const int32_t what, const int32_t info);
    void notifyLastOpComplTime(const uint64_t schedIndex, const int64_t completeTime, const bool isCompleted) const;

    void triggerWarningToDTC(const uint32_t triggerID, const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData,const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority) const; /* RDG30-R-0081: Trigger DTC from Warning */
    void triggerDTCToSSR(const uint32_t triggerID,const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData,const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority,const std::vector<pair<uint32_t, uint32_t>> v_targetEcu) const;     /* TRigger DTC to SSR*/
    void triggerSSRToRoB(const uint32_t triggerID, const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData,const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority) const;     /* TRigger SSR to RoB*/
    void triggerLastUpload(const uint32_t triggerID, const DiagTrigger::DiagTriggerType triggerType, const int64_t timeData,const android::sp<CommonDefine::RDGLocationData> location,const uint64_t collectionId,const uint32_t priority) const;   /*Trigger to lastupload*/
    void forwardWarningToDtc(const DiagTrigger::DiagTriggerState pState, const int32_t pTriggerId, const bool dueToIgOff) const;
    void forwardDtcToSsr(const DiagTrigger::DiagTriggerState pState, const int32_t pTriggerId, const bool dueToIgOff) const;
    void forwardSsrToRob(const DiagTrigger::DiagTriggerState pState, const int32_t pTriggerId, const bool dueToIgOff) const;
    void receiveCenterCommnad(const android::sp<CenterReqData> pCenterReqData);
    void loadNewCenterRequest();
    void loadNewSchedData(const bool isOnlyLoadSched, const bool isBooting);
    void onScheduleReceived(const android::sp<CenterReqData> pCenterReqData) const;
    void onUnderRepairStatus(const int32_t status);
    void onServiceFlagChange(const android::sp<Buffer> didData);
    void onPPIReceived(const android::sp<Buffer> didData);
    bool getOBDStatus() const noexcept;
    void notifyDiagDoneToLastUpload(const uint32_t triggerID) const;
    // void notifyCommunicationRestore();
    void onGRPCCommReconnect();
	void onGRPCCommDisconnect() const;
    uint32_t getPPIFlag() const noexcept;
    void setPPIFlag(const uint32_t PPIFlag) noexcept;
    void setOperationA(const bool status) noexcept;
private:
    static constexpr uint8_t APP_SIZE{8U};
    static android::sp<Remotediag> mRemotediag;
    void initRemoteDiagApp();
    void initApp();
    void doBootCompleted();
    void handleFeatureStatusChange(const std::string feature, const FeatureStatus status);
    void handlePPIErase(const uint32_t ppiFlag) const;
    static void makeFolder();
    void handleFeatureActionPerformed(const std::string featureName);
    void stopRDG(const bool isStop);
    // void initHandler();
    /* Private Atrribute*/
    bool mIsBootCompleted;
    bool mRdgCanRun;
    android::sp<sl::SLLooper> mLooper;

    android::sp<RemotediagHandler> mRemotediagHandler = nullptr;
    android::sp<SchedulerManager> mSchedMgr;

    android::sp<RemoteSSR> mSSR;
    android::sp<RemoteRoB> mRoB;
    android::sp<RemoteOTA> mOTA;
    android::sp<RemoteDTC> mDTC;
    android::sp<RemoteDirectCommand> mRemoteDirectCommand;
    android::sp<PriorityControl> mPriorityControl;
    android::sp<RemoteEcuInformation> mEcuInformation;
    android::sp<RemoteRoBSSR> mRoBSSR;
    android::sp<RemoteWarning> mWarning;
    android::sp<UploadManager> mUploader;
    android::sp<RemoteLastUpload> mRemoteLastUpload;
    android::sp<RoBOccurrence> mRoBOccurrence;
    android::sp<RoBMonitoring> mMonitoring;
    android::sp<CollectionCondition> mCollectionCondition;
    std::unordered_map<uint8_t, RemoteDelegate *> mDelegate;
    bool mIGStatus{false};
    uint8_t mRDGActiveFlag{0U};
    uint8_t mDTCFlag{0U};
    uint8_t mSSRFlag{0U};
    uint8_t mWARflag{0U};
    uint8_t mRoBflag{0U};
    uint8_t mDDRflag{0U};
    uint8_t mUnderRepair{0U};
    bool mSRVC_AC{false};
    bool mSRVC_VC{false};
    bool mSRVC_PC{false};
    bool mSRVC_STT{false};
    bool bOBDEventStatus{false};
    bool mRemotediagInitComplete{false};
    std::map<std::string, int32_t> mWaitingList;
    bool isRestartUploading{true};
    std::atomic<bool> isDoOperationA{false};
    uint32_t mPPIflag{IPPIManagerServiceType::PPI_APP_STATUS_INIT};
    mutable Mutex mMutexPPIFlag;
    mutable Mutex mMutexUnderRepair;
};
}
#endif /* REMOTEDIAG_APPLICATION_H */
