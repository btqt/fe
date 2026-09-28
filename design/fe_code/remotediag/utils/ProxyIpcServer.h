#ifndef RDG_PROXY_IPC_SERVER_H
#define RDG_PROXY_IPC_SERVER_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../remotediagproxy/include/ProxyIpcProtocol.h"
#include "../remotediagproxy/include/IpcTransport.h"
#include "../remotediagproxy/include/IpcMessageHandler.h"

class GrpcResData;

namespace rdgipc {
    class IpcTransport;
}

namespace rdgapp {

class ProxyIpcServer {
public:
    static ProxyIpcServer &getInstance() noexcept;

    bool start() noexcept;

    /**
     * Block until the proxy client connection is established
     *
     * @param timeoutMs Maximum wait in milliseconds
     * @return true if connected before the timeout
     */
    bool waitUntilReady(uint32_t timeoutMs) noexcept;

    bool requestAPICall(const rdgipc::CommandId commandId,
                        const std::vector<uint8_t> &payload,
                        std::vector<uint8_t> &response,
                        uint32_t timeoutMs = 2000U) noexcept;
    
    // ========================================================================
    // NEW API FOR SELF-REGISTERING HANDLERS
    // ========================================================================
    
    /**
     * Register a callback handler for specific callback IDs
     * 
     * This method is called by adapters during initialization to register
     * their callback handlers with the specific callback IDs they handle.
     * No need to modify ProxyIpcServer when adding new callbacks!
     * 
     * @param handler The callback handler to register
     * @param callbackIds List of callback IDs this handler will handle
     */
    void registerCallbackHandler(std::shared_ptr<rdgipc::ICallbackHandler> handler,
                                  const std::vector<uint32_t> &callbackIds);

private:
    ProxyIpcServer() = default;
    ~ProxyIpcServer() noexcept;
    ProxyIpcServer(const ProxyIpcServer &) = delete;
    ProxyIpcServer &operator=(const ProxyIpcServer &) = delete;

    void onFrameReceived(const rdgipc::IpcTransport::Frame &frame);
    void handleCallback(const rdgipc::IpcTransport::Frame &frame) noexcept;
    void handleResponse(const rdgipc::IpcTransport::Frame &frame);
    void runCallbackDispatchLoop() noexcept;

    mutable std::mutex mRequestMutex;
    mutable std::mutex mResponseMutex;
    std::condition_variable mResponseCv;
    uint32_t mPendingResponseId{0U};
    uint32_t mPendingResponseCorrelationId{0U};
    std::vector<uint8_t> mPendingResponsePayload{};
    bool mWaitingForResponse{false};
    uint32_t mNextRequestCorrelationId{1U};

    // Callbacks are dispatched on a worker thread so slow handlers cannot
    // block response frames on the transport reader thread.
    std::atomic<bool> mRunning{false};
    std::mutex mCallbackQueueMutex;
    std::condition_variable mCallbackQueueCv;
    std::deque<rdgipc::IpcTransport::Frame> mCallbackQueue;
    std::thread mCallbackDispatchThread;
    
    // ========================================================================
    // NEW ARCHITECTURE COMPONENTS
    // ========================================================================
    std::shared_ptr<rdgipc::IpcTransport> mTransport;  // Transport layer (stable)
    rdgipc::MessageDispatcher mDispatcher;              // Message routing (stable)
};

} // namespace rdgapp

#endif // RDG_PROXY_IPC_SERVER_H
