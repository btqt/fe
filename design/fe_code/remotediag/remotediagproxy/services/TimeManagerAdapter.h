#ifndef REMOTEDIAGPROXY_TIMEMANAGERADAPTER_H
#define REMOTEDIAGPROXY_TIMEMANAGERADAPTER_H

#include <cstdint>
#include <ctime>
#include <memory>
#include <vector>

#include <utils/Mutex.h>

#include "../include/IpcMessageHandler.h"
#include "../include/ParamsDef.h"
#include "../include/ProxyIpcProtocol.h"
#include "../utils/Logger.h"

class TimeManagerAdapter {
public:
    TimeManagerAdapter();
    virtual ~TimeManagerAdapter() noexcept;
    TimeManagerAdapter(const TimeManagerAdapter &) = delete;
    TimeManagerAdapter &operator=(const TimeManagerAdapter &) = delete;
    TimeManagerAdapter(TimeManagerAdapter &&) = delete;
    TimeManagerAdapter &operator=(TimeManagerAdapter &&) = delete;
    static std::shared_ptr<TimeManagerAdapter> getInstance();

    struct tm getCurrentTime();
    int64_t getCurrentMilliSec();
    int32_t getOffset();

private:
    // Nested command handler for IPC
    class CommandHandler : public rdgipc::ICommandHandler,
                          public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(TimeManagerAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        TimeManagerAdapter* mAdapter;
        rdgipc::CommandResponse handleGetCurrentTime(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleGetCurrentMilliSec(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleGetOffset(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CommandHandler> mCommandHandler;

private:
    static std::shared_ptr<TimeManagerAdapter> instance;
    static android::Mutex mInstanceLock;
};

#endif // REMOTEDIAGPROXY_TIMEMANAGERADAPTER_H
