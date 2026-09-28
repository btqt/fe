#ifndef REMOTEDIAGPROXY_PROXYIPCCLIENT_H
#define REMOTEDIAGPROXY_PROXYIPCCLIENT_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../include/ProxyIpcProtocol.h"
#include "../include/IpcTransport.h"
#include "../include/IpcMessageHandler.h"

namespace rdgipc {
    class IpcTransport;
}

class ProxyIpcClient {
public:
    ProxyIpcClient();
    ~ProxyIpcClient() noexcept;

    static ProxyIpcClient *getInstance() noexcept;

    ProxyIpcClient(const ProxyIpcClient &) = delete;
    ProxyIpcClient &operator=(const ProxyIpcClient &) = delete;

    bool start();
    void stop() noexcept;

    /**
     * @brief Block until the connection to remotediag is established
     * @param timeoutMs Maximum wait in milliseconds
     * @return true if connected before the timeout
     */
    bool waitUntilReady(uint32_t timeoutMs) noexcept;
    
    /**
     * @brief Send a callback to the remotediag application
     * 
     * This is a static helper that handles instance management and error logging.
     * Safe to call from any adapter - automatically handles singleton access.
     * 
     * @param callbackId The callback ID to send
     * @param payload The callback payload data
     * @return true if callback was queued successfully, false otherwise
     */
    static bool sendCallback(uint32_t callbackId, const std::vector<uint8_t> &payload = std::vector<uint8_t>{}) noexcept;
    
    // ========================================================================
    // NEW API FOR SELF-REGISTERING HANDLERS
    // ========================================================================
    
    /**
     * Register a command handler for specific command IDs
     * 
     * This method is called by adapters during initialization to register
     * their command handlers with the specific command IDs they handle.
     * No need to modify ProxyIpcClient when adding new commands!
     * 
     * @param handler The command handler to register
     * @param commandIds List of command IDs this handler will handle
     */
    void registerCommandHandler(std::shared_ptr<rdgipc::ICommandHandler> handler,
                                const std::vector<uint32_t> &commandIds);

private:
    struct PendingCallback {
        uint32_t callbackId;
        std::vector<uint8_t> payload;
    };

    void runCallbackDrainLoop() noexcept;
    void runRequestDispatchLoop() noexcept;
    void onFrameReceived(const rdgipc::IpcTransport::Frame &frame);
    void handleRequest(const rdgipc::IpcTransport::Frame &frame);
    bool flushCallbacks() noexcept;

    static ProxyIpcClient *sInstance;
    std::atomic<bool> mRunning{false};
    std::mutex mCallbackMutex;
    std::mutex mFlushMutex;  // Serializes flushCallbacks() so front/send/pop stays atomic
    std::deque<PendingCallback> mPendingCallbacks;
    std::thread mCallbackDrainThread;

    // Requests are dispatched on a worker thread so slow handlers cannot
    // stall the transport reader thread.
    std::mutex mRequestQueueMutex;
    std::condition_variable mRequestCv;
    std::deque<rdgipc::IpcTransport::Frame> mRequestQueue;
    std::thread mRequestDispatchThread;
    
    // ========================================================================
    // NEW ARCHITECTURE COMPONENTS
    // ========================================================================
    std::shared_ptr<rdgipc::IpcTransport> mTransport;  // Transport layer (stable)
    rdgipc::MessageDispatcher mDispatcher;              // Message routing (stable)
};

#endif /* REMOTEDIAGPROXY_PROXYIPCCLIENT_H */
