#ifndef REMOTEDIAG_PPIMANAGERADAPTER_H
#define REMOTEDIAG_PPIMANAGERADAPTER_H

#include <memory>
#include <string>
#include <utils/Mutex.h>
#include <utils/Buffer.h>

#include <Error.h>

#include "../utils/RemotediagHandler.h"
#include "../utils/Logger.h"
#include "../include/ParamsDef.h"
#include "../remotediagproxy/include/IpcMessageHandler.h"
#include "../remotediagproxy/include/ProxyIpcProtocol.h"

namespace rdgapp {

class RemotediagHandler;
class PPIManagerAdapter
{
    public:
        PPIManagerAdapter();
        ~PPIManagerAdapter();
        PPIManagerAdapter(const PPIManagerAdapter& ) = delete;
        PPIManagerAdapter& operator=(const PPIManagerAdapter& ) = delete;
        PPIManagerAdapter(PPIManagerAdapter&& ) = delete;
        PPIManagerAdapter& operator=(PPIManagerAdapter&& ) = delete;
        
        static std::shared_ptr<PPIManagerAdapter> getInstance();

        void onStatusChanged(android::sp<::Buffer>& name) const;
        void registerService();
        uint32_t receivePPIErase(const char_t* const buf);
        void responsePPIErase(const uint32_t appType, const uint32_t appState);

    private:
        // Nested CallbackHandler for self-registering callback handling
        class CallbackHandler : public rdgipc::ICallbackHandler,
                               public std::enable_shared_from_this<CallbackHandler> {
        public:
            explicit CallbackHandler(PPIManagerAdapter* adapter);
            ~CallbackHandler() override = default;
            void initialize();
            void handle(uint32_t callbackId, const std::vector<uint8_t>& payload) override;
        private:
            PPIManagerAdapter* mAdapter;
        };
        std::shared_ptr<CallbackHandler> mCallbackHandler;

        static std::shared_ptr<PPIManagerAdapter> instance;
        android::sp<RemotediagHandler> mHandler = nullptr;
        static android::Mutex mInstanceLock;

};
}
#endif //REMOTEDIAG_PPIMANAGERADAPTER_H
