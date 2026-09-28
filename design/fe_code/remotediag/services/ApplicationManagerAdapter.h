#ifndef REMOTEDIAG_REG_APPLICATIONMANAGERADAPTER_H
#define REMOTEDIAG_REG_APPLICATIONMANAGERADAPTER_H

#include <cstdint>
#include <string>
#include <utils/Mutex.h>
#include <utils/RefBase.h>

#include "Error.h"
#include "services/ApplicationManagerService/IApplicationManagerServiceType.h"

#include "../include/ParamsDef.h"
#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../remotediagproxy/include/IpcMessageHandler.h"
#include "../remotediagproxy/include/ProxyIpcProtocol.h"

namespace rdgapp {

class RemotediagHandler;
class ApplicationManagerAdapter : public android::RefBase {
public:
    ApplicationManagerAdapter();
    virtual ~ApplicationManagerAdapter() noexcept;
    ApplicationManagerAdapter(ApplicationManagerAdapter const&) = default;
    ApplicationManagerAdapter& operator=(ApplicationManagerAdapter const&) = default;
    ApplicationManagerAdapter(ApplicationManagerAdapter&&) = delete;
    ApplicationManagerAdapter& operator=(ApplicationManagerAdapter&&) = delete;    
    static std::shared_ptr<ApplicationManagerAdapter> getInstance();
    void registerService();
    int32_t queryActionForFeature(const std::string name);
    int32_t setFeatureStatus(const std::string appNames, const std::string feaName, const bool onOf);
    FeatureStatus getFeatureStatus(const std::string name);

private:
    // Nested CallbackHandler for self-registering callback handling
    class CallbackHandler : public rdgipc::ICallbackHandler,
                           public std::enable_shared_from_this<CallbackHandler> {
    public:
        explicit CallbackHandler(ApplicationManagerAdapter* adapter);
        ~CallbackHandler() override = default;
        void initialize();
        void handle(uint32_t callbackId, const std::vector<uint8_t>& payload) override;
    private:
        ApplicationManagerAdapter* mAdapter;
    };
    std::shared_ptr<CallbackHandler> mCallbackHandler;

    static std::shared_ptr<ApplicationManagerAdapter> instance;
    bool mIsBootCompleted{false};
    android::sp<RemotediagHandler> mHandler = nullptr;
    static android::Mutex mInstanceLock;
};
}
#endif /* REMOTEDIAG_REG_APPLICATIONMANAGERADAPTER_H */
