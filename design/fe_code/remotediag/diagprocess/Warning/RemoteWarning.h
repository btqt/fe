#ifndef REMOTE_WARNING_H
#define REMOTE_WARNING_H

#include <utils/Handler.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/Buffer.h>
#include <services/TimeManagerService/TimeManager.h>   
#include <unordered_map>

#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "utils/FileUtil.h"
#include "utils/PriorityControl.h"
#include "DiagTrigger.h"
#include "RemoteDelegate.h"
#include "Remotediag.h"

#include "diagprocess/Warning/WarningSignal.h"
#include "diagprocess/Warning/FilteringList.h"
#include "diagprocess/UDS/UdsMessage.h"
#include "services/VehicleManagerAdapter.h"
#include "services/DiagManagerAdapter.h"
#include "services/LocationManagerAdapter.h"
#include "services/DiagManagerAdapter.h"
#include "services/RegionManagerAdapter.h"
#include "utils/CollectionCondition.h"


namespace rdgapp {


const std::string PROTOBUF_VERSION {"3.11.4"};

const std::string WARNING_COUNTER_FILE {DATA_PATH + "warning_counter"};
const std::string WARNING_TABLE_FILE {DATA_PATH + "property_tbl_pf"};   //ID for save info PF

//class RemoteDiag;
class FilteringList;

class RemoteWarning : public android::RefBase, public RemoteDelegate {
protected:
    bool checkPrecondition() override; 
public: 
    RemoteWarning(const Remotediag& app, android::sp<sl::SLLooper>& privLooper);
    ~RemoteWarning() override;
    RemoteWarning(const RemoteWarning&) = default;
    RemoteWarning(RemoteWarning&&) = default;
    RemoteWarning& operator=(const RemoteWarning&) = default;
    RemoteWarning& operator=(RemoteWarning&&) = default;
    
    static android::sp<RemoteWarning> getInstance();

    void notifyBootComplete() const override;
    void onReceiveIG(const bool status) const override;
    void onReceiveUDS(const android::sp<OBCResponseEventInfo> responseEventInfo, const android::sp<UdsMessage> udsResponse)  noexcept override {};
    void onChangedRemoteInfo(const int32_t what, const int32_t info = 0) override;
    void onCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData) override;
    void onRdgStop(const bool isStop) const noexcept override;
    virtual uint8_t getAppId() const noexcept {return APP_ID;};
    std::map<uint64_t, android::sp<UdsMessage>> getDiagResponseList() const noexcept final {return std::map<uint64_t, android::sp<UdsMessage>>();};

    void onReceivedCanSignal(const uint32_t channel, const android::sp<VehicleData>& vehicleData);
    void onReceivedCanSignalTimeout(const uint32_t channel, const android::sp<VehicleData>& vehicleData);
    bool notifyTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff); 
    void testClearWarningCounter() const;
    void onServiceFlagChange();
    void onPPIReceived();
private:
    enum mWarningSignal :uint16_t
    {
        WARNING_SIGNAL_TOTAL = 256U,
        WARNING_SIGNAL_INVALID_ID = 0xFFFFU
    };
    uint8_t WARNING_TABLE_EU[WARNING_SIGNAL_TOTAL]{0xcfU, 0xc2U, 0xc2U, 0xcfU, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xcfU, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,

                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xcfU, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,

                                    0xc2U, 0xc2U, 0xc2U, 0x00U, 0xc2U, 0xc2U, 0xc2U, 0x00U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xcfU, 0xc2U, 0xcfU, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,

                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xcfU, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U};
    uint8_t APP_ID;
    void printData(const std::string data) const;

    enum class EngineStartStatus : int32_t {
        ENGINE_OFF = 0,
        ENGINE_ON_PROGRESS = 1,
        ENGINE_ON_COMPLETE = 2
    };

    enum class Abort: uint8_t {
        ABORT_INIT = 0,
        ABORT_PRIORITY_DISCARDED,
        ABORT_IG_STATE_CHANGE,
        ABORT_WARNING_FLAG_CHANGE,
        ABORT_RDG_FLAG_CHANGE,
        ABORT_WARNING_TABLE_UPDATE,
        ABORT_UNDER_REPAIR
    };

    enum mWarningByte :uint8_t
    {
        WARNING_BYTE_TOTAL = 32U
    };
    enum CanSignal :uint32_t    //CANID in CAN spec 6-DCF-10-02_24DCM Global CAN Data Specification
    {
        CMD_CAN_SIGNAL_MET1S34 = 0x0520U, 
        CMD_CAN_SIGNAL_MET1S35 = 0x0521U,
        CMD_CAN_SIGNAL_MET1S36 = 0x0522U,
        CMD_CAN_SIGNAL_MET1S37 = 0x0523U,
        CMD_CAN_SIGNAL_ENG1G90 = 0x051EU,
        CMD_CAN_SIGNAL_MET1S02 = 0x0611U
    };

    // warning counter file should not be deleted after erasing ppi
    //static const std::string WARNING_COUNTER_FILE_NAME;

    enum TimerWarID :int32_t
    {
        TIMER_IG_ON_READY_ID = 1001,
        TIMER_ENG_ON_ID = 1002
    };

    uint8_t WARNING_TABLE_OTHERS[WARNING_SIGNAL_TOTAL] {0xcfU, 0xc2U, 0xc2U, 0xcfU, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xcfU, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xcfU, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                
                                                    0xc2U, 0xc2U, 0xc2U, 0x00U, 0xc2U, 0xc2U, 0xc2U, 0x00U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xcfU, 0xc2U, 0xcfU, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xcfU, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U,
                                                    0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U, 0xc2U};

    class MainHandler : public sl::Handler {
    public:

        static constexpr int32_t CMD_INIT_WARNING {2000};
        static constexpr int32_t CMD_CHANGE_IG_STATUS {2001};
        static constexpr int32_t CMD_END_WARNING {2002};
        static constexpr int32_t CMD_SIGNAL_RECEIVED {2003};
        static constexpr int32_t CMD_SIGNAL_TIMEOUT_RECEIVED {2004};
        static constexpr int32_t CMD_RECEIVE_STATUS_FROM_CENTER {2006};     //UnderRepair
        static constexpr int32_t CMD_WARFLAG_CHANGE{2007};
        static constexpr int32_t CMD_PPI_CHANGE{2008};
        static constexpr int32_t CMD_CENTER_COMMAND{2009};
        static constexpr int32_t CMD_TIMER_EXPIRED {2011};
        static constexpr int32_t CMD_REQUEST_TO_PRIORITY_CONTROL {2014};
        static constexpr int32_t CMD_WARNING_TRIGGER_TO_DTC {2015};
        static constexpr int32_t CMD_TRIGGER_FROM_CENTER{2016};             //For collection condition update complete
        static constexpr int32_t CMD_ODO_SIGNAL_RECEIVED{2017};
        static constexpr int32_t CMD_ODO_SIGNAL_TIMEOUT_RECEIVED{2018};
        static constexpr int32_t CMD_STOP_RDG{2019};

        //update

        explicit MainHandler(android::sp<sl::SLLooper>& looper, RemoteWarning& warning) noexcept
                : android::RefBase(), sl::Handler(looper), mWarning(warning) {}
        virtual ~MainHandler() override = default;
        MainHandler(const MainHandler&) = default;
        MainHandler(MainHandler&&) = default;
        MainHandler& operator=(const MainHandler&) = default;
        MainHandler& operator=(MainHandler&&) = default;

        void handleMessage (const android::sp<sl::Message>& handlemsg) override;
    private:
        RemoteWarning& mWarning;
    };

    class TimerHandler : public TimerTimeoutHandler {
    public:
        explicit TimerHandler(RemoteWarning& warning) noexcept : TimerTimeoutHandler(), mWarning(warning) {}
        virtual ~TimerHandler() override = default;
        TimerHandler(const TimerHandler&) = default;
        TimerHandler(TimerHandler&&) = default;
        TimerHandler& operator=(const TimerHandler&) = default;
        TimerHandler& operator=(TimerHandler&&) = default;

        void handlerFunction(const int32_t timerId) override {
            (void)mWarning.mHandler->obtainMessage(MainHandler::CMD_TIMER_EXPIRED, timerId)->sendToTarget(); //after duration timer => call this funtion to run expired timer and time ID
        }

    private:
        RemoteWarning& mWarning;
    };

    const Remotediag& mApp;
    // static RemoteWarning*       instance;
    static android::sp<RemoteWarning> mWarning;
    android::sp<MainHandler> mHandler;
    android::sp<CommonDefine::RDGLocationData> mLocationData;
    bool mIGStatus;
    std::unordered_map<uint32_t, android::sp<DiagTrigger>> mSaveReq;
    // bool mWarningPriority;  //Check Waring is run in priority function
    bool mIsUploading;  //Check is in uploading process
    // bool isRequestPrioriy;  //Check Warning is requested priority or not - For case trigger pending, warning send multi request priority 
    uint32_t mPriorityId;
    uint32_t mPriority;
    uint64_t mColID;
    bool isAbortWarning;

    //Service Flag
    bool mRDGFlag;
    bool mWARFlag;
    bool mConsentStatus;

    std::map<uint32_t, uint8_t> mWarningChange;
    TimerHandler* mTimerHandler;

    android::sp<Timer> mIGOnTimer;
    android::sp<Timer> mEngOnTimer;

    android::sp<FilteringList> mChangedList;

    bool mWarningStart;
    bool mIGOnReady;
    EngineStartStatus mEngOnStatus; 
    int32_t mUnderRepair;  
    bool mBuBState; //True -> disable RDG | False -> enable RDG  
    android::sp<WarningSignal> mSignalArr[WARNING_SIGNAL_TOTAL];   //an warning signal name (Total 256)

    void init();
    void handleIGStatus(const bool status);
    void stopWarning();
    void handleSignalReceived(const android::sp<sl::Message>& msg);
    void handleSignalTimeOutReceived(const android::sp<sl::Message>& msg);
    void startWarning();
    void handleFlagOff();

    void getWarningDataPart(Buffer& buf) const;
    void getWarningSetIndex(const uint32_t warningSetId, uint16_t& index, uint16_t& count) const noexcept;
    // uint16_t getSignalOffset(const uint32_t signalID) const noexcept; //DCM24 do need check ENG MCU, Only work for MET
    void updateWarningChange(const uint16_t start, const uint16_t end, const android::sp<::Buffer> mbuffer);
    void initRemoteWarning();
    void changedRemoteStatus(const int32_t what, const int32_t info);

    void handleEngOnStatus();
    void handleEngOffStatus();
    void uploadIfNeeded();

    bool handleWarningReceived(const android::sp<WarningSignal> warning, const uint8_t data);

    void initWarningCounter();
    void initWarningPropertyTable();
    void saveWarningCounterToMemory();
    void saveWarningPropertyTableToMemory();
    void addToConfirmList(const android::sp<WarningSignal> warning, const uint8_t filterMode);
    void removeFromConfirmList(const android::sp<WarningSignal> warning);
    char_t uint8ToChar(const uint8_t num) const noexcept;
    uint8_t charToUint8(const char_t c) const noexcept;
    android::sp<WarningSignal> findWarningSignal(const uint16_t warningID) const;
    void handleDataUpload();

    void requestDiagTriggerDTC(const android::sp<DiagTrigger> pDiagTrigger);

    void makeTableAndParse();
    void obtainTableInfoFromMemory();

    void setTimeAndLocation();

    void handleTimerExpired(const int32_t mtimerID);
    void requestPriority();
    void handleTrigger(const DiagTrigger::DiagTriggerState& pState, const int32_t& pTriggerId, const bool dueToIgOff);
    void handleUpdateCollectionCondition();
    uint64_t getCurrentColID() const noexcept;
    void abortUploadWarningInfo(const Abort abortReason);

    void getServiceFlag();

    //Package and upload data
    std::shared_ptr<vccomif::rdg::v1::interfaces::UploadWarningInformationRequest> mWarningInfoReq;
    std::shared_ptr<vccomif::rdg::v1::interfaces::UploadErrorDataRequest> mWarningUploadErrorData;
 
    void packageUploadData();
    void makeUploadErrorData(const vccomif::rdg::v1::interfaces::ResponseCode resCode);
    mutable Mutex mMutexWarning; //for case mSaveReq is clear while warning verify done
    void handleStopRDG(const int32_t state);
    void handleServiceFlagChange();
    void handlePPIReceived();
    void handleCenterCommandForward(const android::sp<CenterReqData>& pCenterReqData);
};

}
#endif // REMOTE_WARNING_H`
