#ifndef RDG_UPLOAD_MANAGER_H
#define RDG_UPLOAD_MANAGER_H

#include <map>
#include <deque>

#include <utils/RefBase.h>
#include <utils/Timer.h>

#include <services/RegionManagerService/IRegionManagerService.h>
#include <services/RegionManagerService/IRegionManagerServiceType.h>
#include <services/RegionManagerService/RegionManager.h>
#include "services/HttpManagerAdapter.h"
#include "services/DiagManagerAdapter.h"
#include "services/ApplicationManagerAdapter.h"

#include "SchedulerType.h"
#include "SchedulerTime.h"
#include "SchedulerQueue.h"
#include "utils/Logger.h"
#include "Remotediag.h"
#include "DiagTrigger.h"
#include "UploadTask.h"
#include "DataModel.h"
#include "UploadPrioQueue.h"
#include "utils/Database.h"

namespace rdgapp {

class Remotediag;
class UploadPrioQueue;
class UploadTask;
class Database;

class UploadManager : public  android::RefBase {
public:
    static constexpr uint32_t MAX_REQUEST_ID{1000U};
    static constexpr uint64_t DTC_ROB_MAX_STORAGE{200000U}; // bytes = 200K bytes
    static constexpr uint64_t SSR_MAX_STORAGE{20000000U}; // bytes = 20M bytes
    static constexpr uint64_t DIRECTCOMMAND_MAX_STORAGE{10000000U}; // bytes = 10M bytes
    static constexpr uint64_t ROBSSR_MAX_STORAGE{80000000U}; // bytes = 80M bytes
    static constexpr uint64_t WARNING_MAX_STORAGE{2200U}; // bytes = 2.2K bytes
    static constexpr uint64_t ECU_INFOR_MAX_STORAGE{100000U}; // bytes = 100K bytes
    static constexpr uint64_t LAST_MAX_STORAGE{14000U}; // bytes = 14K bytes
    static constexpr uint64_t NOTIFICATION_MAX_STORAGE{40000U}; // bytes = 40K bytes
    static constexpr uint64_t ERROR_MAX_STORAGE{40000U}; // bytes = 40K bytes
    // static constexpr uint64_t DDR_MAX_STORAGE{2048U}; // bytes = 2KB
    // static constexpr uint64_t COLL_UPDATE_RESULT_NOTI_MAX_STORAGE{18432U}; // bytes = 18KB

    UploadManager(const Remotediag& app, android::sp<sl::SLLooper>& privateLooper);
    ~UploadManager() override;
    UploadManager(const UploadManager&) = default;
    UploadManager(UploadManager&&) = default;
    UploadManager& operator=(const UploadManager&) = default;
    UploadManager& operator=(UploadManager&&) = default;

    static android::sp<UploadManager> getInstance();
    void sendFile(const android::sp<UploadTask> task);
    void requestUploadTask(const android::sp<UploadTask> task);
    void restartUploadTask(const android::sp<UploadTask> task);
    void addTask(const android::sp<UploadTask> &pUploadTask, const bool needUpdateDB);
    void addTask_delayed(const android::sp<UploadTask> &pUploadTask);
    void discardAllTask();
    void deletePendingTask();
    void triggerUpload();
    void doUpload();
    uint32_t genRequestId();
    uint64_t genCountUpload();
    void receivedGrpcRes(const android::sp<GrpcResData> pGrpcResData);
    void onReceivedGrpcRes(const android::sp<GrpcResData> pGrpcResData);
    void onReceiveIG(const bool status);
    void onRdgStop(const bool isStop);
    void onServiceFlagChange();
    void onPPIChangedToFalse();
    // void onCommuRestore();
    void updateStorage(const android::sp<UploadTask> task, const bool isTaskAdded);
    void onRestartUploading();
    void operationA();
    void doCallIdTimeout(const int32_t callid);
    void onCallIdTimeout(const int32_t callid);
    GRPC_IF_TYPE convertIntToFileType(const uint32_t fileType);
    void setRetryTimer(const uint64_t uploadId, const uint64_t duration, const android::sp<UploadTask> task = nullptr);
    void stopRetryTimer(const uint64_t uploadId);
    void doRetryTimeOut(const int32_t timerId);
    void onRetryTimeOut(const int32_t timerId);
    static uint32_t getCounterValue() noexcept;
    uint32_t getCounterMessage() noexcept;
    void resetCounterByIgON() noexcept;
    void clearSentQueue();
    void handleOnReceiveIG(const bool status);
    void setRDGStop(const bool isStop) noexcept;
    bool getRDGStop() const noexcept;

    //for SLDD
    void testSetUploadStorage(const int32_t type, const uint64_t value) noexcept;
    void testGetUploadStorage() noexcept;
    static void testCounterValue(const uint32_t valTest) noexcept;

private:
    static constexpr uint32_t IG_OFF_STOP_UPLOADING_DURATION {30U};/* 30 sec */
    static constexpr uint32_t INTERNAL_TIMEOUT_UPLOADING_DURATION {65U};/* 65 sec */
    std::map<uint32_t, GRPC_IF_TYPE> grpcValues;
    void printData(const std::string data) const;
    void init();
    void stopUploadingIG();
    void handleRestartUploadIG();
    void deleteOldestFile(std::deque<android::sp<UploadTask>> &oldTask);
    void handleOperationA();
    void loadBackupData();
    void resetStorageData();

    class MainHandler : public sl::Handler {
    public:
        static constexpr int32_t CMD_INIT_UPLOADMANAGER{2000};
        // static constexpr int32_t MSG_ID_HTTP_SEND_MSG{2001};
        static constexpr int32_t MSG_UPLOAD_FILE{2002};
        static constexpr int32_t MSG_ADD_UPLOAD_TASK{2003};
        static constexpr int32_t MSG_ID_DO_UPLOAD{2004};
        static constexpr int32_t MSG_ID_DELAYED_INSERT_MSG{2005};
        // static constexpr int32_t MSG_ADD_DELAYED_UPLOAD_TASK{2006};
        // static constexpr int32_t MSG_ADD_DELAYED_UPLOAD_TASK_DELAY{2007};
        static constexpr int32_t MSG_RESTART_UPLOADING_IG{2008};
        static constexpr int32_t MSG_DO_OPERATION_A{2009};
        static constexpr int32_t MSG_ID_TRIGGER_UPLOAD{2010};
        static constexpr int32_t MSG_ID_DETELE_PENDING_UPLOAD{2011};
        static constexpr int32_t MSG_RESTART_UPLOAD_TASK{2012};
        static constexpr int32_t MSG_RECEIVE_GRPC_RES{2013};
        static constexpr int32_t MSG_RETRY_TIMEOUT{2014};
        static constexpr int32_t MSG_CALLID_TIMEOUT{2015};
        static constexpr int32_t MSG_ON_RECEIVE_IG{2016};
        static constexpr int32_t MSG_LOAD_BACKUP_DATA{2017};
        static constexpr int32_t CMD_STOP_RDG{2018};

        explicit MainHandler(android::sp<sl::SLLooper>& privateLooper, UploadManager &uploader) noexcept
                : android::RefBase(), sl::Handler(privateLooper), mParent(uploader) {}
        ~MainHandler() override = default;
        MainHandler(const MainHandler&) = default;
        MainHandler(MainHandler&&) = default;
        MainHandler& operator=(const MainHandler&) = default;
        MainHandler& operator=(MainHandler&&) = default;
        void handleMessage (const android::sp<sl::Message>& handlemsg) override;
    private:
        UploadManager& mParent;
    };

    class TimerHandler : public TimerTimeoutHandler {
    public:
        static constexpr int32_t ID_IG_OFF_STOP_UPLOADING {3000};
        explicit TimerHandler (UploadManager& uploader) noexcept : TimerTimeoutHandler(), mUploadManager(uploader) {}
        ~TimerHandler() override = default;
        TimerHandler(const TimerHandler&) = default;
        TimerHandler(TimerHandler&&) = default;
        TimerHandler& operator=(const TimerHandler&) = default;
        TimerHandler& operator=(TimerHandler&&) = default;

        void handlerFunction(const int32_t timerId) override;
    private:
        UploadManager& mUploadManager;
    };

    class TimerHandler_Uploading : public TimerTimeoutHandler {
    public:
        explicit TimerHandler_Uploading (UploadManager& uploader) noexcept : TimerTimeoutHandler(), mUploadManager(uploader) {}
        ~TimerHandler_Uploading() override = default;
        TimerHandler_Uploading(const TimerHandler_Uploading&) = default;
        TimerHandler_Uploading(TimerHandler_Uploading&&) = default;
        TimerHandler_Uploading& operator=(const TimerHandler_Uploading&) = default;
        TimerHandler_Uploading& operator=(TimerHandler_Uploading&&) = default;

        void handlerFunction(const int32_t timerId) override ;
    private:
        UploadManager& mUploadManager;
    };

    class TimerHandler_Retry : public TimerTimeoutHandler {
    public:
        explicit TimerHandler_Retry (UploadManager& uploader) noexcept : TimerTimeoutHandler(), mUploadManager(uploader) {}
        ~TimerHandler_Retry() override = default;
        TimerHandler_Retry(const TimerHandler_Retry&) = default;
        TimerHandler_Retry(TimerHandler_Retry&&) = default;
        TimerHandler_Retry& operator=(const TimerHandler_Retry&) = default;
        TimerHandler_Retry& operator=(TimerHandler_Retry&&) = default;

        void handlerFunction (const int32_t timerId) override ;
    private:
        UploadManager& mUploadManager;
    };

    mutable android::Mutex mLock;
    mutable android::Mutex mLock_OpA;
    mutable android::Mutex mLock_SentTask;
    mutable android::Mutex mLock_ReceivedRes;
    mutable android::Mutex mLock_Retry;
    static android::sp<UploadManager> mUploadManager;
    const Remotediag& mApp;
    android::sp<MainHandler> mHandler;
    android::sp<UploadPrioQueue> mMspTaskQueue;
    uint32_t currentRequestId;
    std::multimap<int32_t, android::sp<UploadTask>> mSentTask;
    std::map<int64_t, bool> mRetryCheck;
    std::shared_ptr<TimerHandler> mTimerHandler;
    android::sp<Timer> mTickTimer;
    std::unique_ptr<Database> mUploadingDB;
    uint64_t m_dtc_rob_max_storage;
    uint64_t m_ssr_max_storage;
    uint64_t m_directcommand_max_storage;
    uint64_t m_robssr_max_storage;
    uint64_t m_warning_max_storage;
    uint64_t m_ecu_infor_max_storage;
    uint64_t m_last_max_storage;
    uint64_t m_notification_max_storage;
    uint64_t m_error_max_storage;
    uint64_t m_ddr_max_storage;
    uint64_t m_coll_update_result_noti_max_storage;
    std::shared_ptr<TimerHandler_Uploading> mTimerHandler_Uploading;
    std::multimap<int32_t, android::sp<Timer>> mTimers_Uploading;
    android::sp<Timer> mTickTimer_Uploading;
    std::shared_ptr<TimerHandler_Retry> mpTimerHandler_Retry;
    std::map<int32_t, std::pair<android::sp<Timer>, android::sp<UploadTask>>> mTimers_RetryMap;
    android::sp<Timer> mTickTimer_Retry;
    std::deque<android::sp<UploadTask>> mListDtcRobFilePath;
    std::deque<android::sp<UploadTask>> mListSsrFilePath;
    std::deque<android::sp<UploadTask>> mListDirectCommandFilePath;
    std::deque<android::sp<UploadTask>> mListRoBSSRFilePath;
    std::deque<android::sp<UploadTask>> mListWarningFilePath;
    std::deque<android::sp<UploadTask>> mListEcuInforFilePath;
    std::deque<android::sp<UploadTask>> mListLastUploadFilePath;
    std::deque<android::sp<UploadTask>> mListNotificationFilePath;
    std::deque<android::sp<UploadTask>> mListErrorFilePath;
    std::deque<android::sp<UploadTask>> mqOperationA;
    std::deque<android::sp<UploadTask>> mqRestartUploadIG;
    uint32_t mCounterMess;
    std::vector<CommonDefine::UploadFileAttribute> mBackupUpload;
    bool mIgStoppedUpload;
    bool mRdgStop;
};
}
#endif /* RDG_UPLOAD_MANAGER_H */
