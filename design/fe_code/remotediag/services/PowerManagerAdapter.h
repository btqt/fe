#ifndef REMOTEDIAG_REG_ADAPTER_POWERMANAGER_H
#define REMOTEDIAG_REG_ADAPTER_POWERMANAGER_H

#include <cstdint>
#include <memory>
#include <vector>

#include <services/PowerManagerService/PowerIndexEnum.h>
#include <services/PowerManagerService/IPowerManagerServiceType.h>
#include <utils/Mutex.h>

#include "../include/ParamsDef.h"
#include "../remotediagproxy/include/ProxyIpcProtocol.h"
#include "../remotediagproxy/include/IpcMessageHandler.h"
#include "../utils/Logger.h"
#include "../utils/ProxyIpcServer.h"
#include "../utils/RemotediagHandler.h"

namespace rdgapp {

class PowerManagerAdapter {
private:
    static std::shared_ptr<PowerManagerAdapter> instance;
    android::sp<RemotediagHandler> mHandler = nullptr;
    bool mCurBubStatus = false;
    static android::Mutex mInstanceLock;

    void IGN_changedHandler(const IG_STATUS status);

public:
    PowerManagerAdapter();
    virtual ~PowerManagerAdapter() noexcept;
    PowerManagerAdapter(PowerManagerAdapter const&) = default;
    PowerManagerAdapter& operator=(PowerManagerAdapter const&) = default;
    PowerManagerAdapter(PowerManagerAdapter&&) = delete;
    PowerManagerAdapter& operator=(PowerManagerAdapter&&) = delete;
    static std::shared_ptr<PowerManagerAdapter> getInstance();

    void registerService();
    void acquirePowerLock();
    void releasePowerLock();
    void onPowerStateChanged(const int32_t newState, const int32_t reason);
    static void onErrControlPower(const int32_t err_reason, const int32_t errPowerID, const int32_t currPowerID);
    static void onPowerModeChanged(const int32_t newMode);
    void onExtValueChanged(const int32_t listenIndex, const int32_t value);
    static void onPowerLockRelease();
    IG_STATUS getIgnitionStatus(void);
    void bubTrigger(const bool value);
    
private:
    // ========================================================================
    // NESTED IPC CALLBACK HANDLER (handles IPC callbacks for this adapter)
    // ========================================================================
    class CallbackHandler : public rdgipc::ICallbackHandler,
                           public std::enable_shared_from_this<CallbackHandler> {
    public:
        explicit CallbackHandler(PowerManagerAdapter* adapter);
        ~CallbackHandler() override;
        void initialize();
        
        void handle(uint32_t callbackId, const std::vector<uint8_t> &payload) override;
        
    private:
        PowerManagerAdapter* mAdapter;
        
        void handlePowerStateChanged(const std::vector<uint8_t> &payload);
        void handlePowerErrControl(const std::vector<uint8_t> &payload);
        void handlePowerModeChanged(const std::vector<uint8_t> &payload);
        void handlePowerExtValueChanged(const std::vector<uint8_t> &payload);
        void handlePowerLockReleased(const std::vector<uint8_t> &payload);
        
        static bool parseIntListPayload(const std::string &value, std::vector<int32_t> &numbers);
    };
    
    std::shared_ptr<CallbackHandler> mCallbackHandler;
};

}

#endif /* REMOTEDIAG_REG_ADAPTER_POWERMANAGER_H */
