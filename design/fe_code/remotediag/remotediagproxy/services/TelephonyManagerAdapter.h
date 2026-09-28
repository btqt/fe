#ifndef REMOTEDIAGPROXY_TELEPHONYMANAGERADAPTER_H
#define REMOTEDIAGPROXY_TELEPHONYMANAGERADAPTER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <utils/Mutex.h>

#include "../include/IpcMessageHandler.h"
#include "../include/ParamsDef.h"
#include "../include/ProxyIpcProtocol.h"
#include "../utils/Logger.h"

class TelephonyManagerAdapter {
public:
    TelephonyManagerAdapter();
    virtual ~TelephonyManagerAdapter() noexcept;
    TelephonyManagerAdapter(const TelephonyManagerAdapter &) = delete;
    TelephonyManagerAdapter &operator=(const TelephonyManagerAdapter &) = delete;
    TelephonyManagerAdapter(TelephonyManagerAdapter &&) = delete;
    TelephonyManagerAdapter &operator=(TelephonyManagerAdapter &&) = delete;
    static std::shared_ptr<TelephonyManagerAdapter> getInstance();

    std::string getNetworkTime();
    std::string getImei();
    std::string getEidDefault();

private:
    // Nested command handler for IPC
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(TelephonyManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        TelephonyManagerAdapter* mAdapter;
        rdgipc::CommandResponse handleGetNetworkTime(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleGetImei(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleGetEidDefault(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CommandHandler> mCommandHandler;

private:
    static std::shared_ptr<TelephonyManagerAdapter> instance;
    static android::Mutex mInstanceLock;
};

#endif // REMOTEDIAGPROXY_TELEPHONYMANAGERADAPTER_H
