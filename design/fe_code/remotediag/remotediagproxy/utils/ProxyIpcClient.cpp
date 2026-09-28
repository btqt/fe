#include "ProxyIpcClient.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "Logger.h"
#include "../include/UnixSocketTransport.h"
#include "../include/IpcMessageHandler.h"
#include <services/TimeManagerService/TimeManager.h>
#include <TelephonyManager.hpp>

namespace {
constexpr const char *kDefaultSocketPath{"/dev/socket/remotediag/remotediag_proxy.sock"};
constexpr size_t kMaxPendingCallbacks{64U};
constexpr size_t kMaxPendingRequests{64U};

std::string resolveSocketPath() {
    const char *envPath{std::getenv("RDG_PROXY_SOCKET_PATH")};
    if ((envPath != nullptr) && (envPath[0] != '\0')) {
        return std::string{envPath};
    }
    return std::string{kDefaultSocketPath};
}

} // namespace

ProxyIpcClient *ProxyIpcClient::sInstance{nullptr};

ProxyIpcClient::ProxyIpcClient()
    : mTransport(std::make_shared<rdgipc::UnixSocketClient>(resolveSocketPath()))
{
    sInstance = this;
}

ProxyIpcClient::~ProxyIpcClient() noexcept
{
    stop();
    if (sInstance == this)
    {
        sInstance = nullptr;
    }
}

ProxyIpcClient *ProxyIpcClient::getInstance() noexcept
{
    return sInstance;
}

void ProxyIpcClient::registerCommandHandler(std::shared_ptr<rdgipc::ICommandHandler> handler,
                                            const std::vector<uint32_t> &commandIds) {
    LOG_I("ProxyIpcClient: registering command handler for %zu commands", commandIds.size());
    mDispatcher.registerCommandHandler(handler, commandIds);
}

bool ProxyIpcClient::sendCallback(uint32_t callbackId, const std::vector<uint8_t> &payload) noexcept {
    ProxyIpcClient *client{getInstance()};
    if (client == nullptr) {
        LOG_W("ProxyIpcClient::sendCallback: no instance available callback=%s", rdgipc::callbackName(callbackId));
        return false;
    }

    size_t pendingCount{0U};
    {
        const std::lock_guard<std::mutex> callbackLock{client->mCallbackMutex};
        if (client->mPendingCallbacks.size() >= kMaxPendingCallbacks)
        {
            LOG_E("ProxyIpcClient: callback queue full, dropping callback id=%u", callbackId);
            return false;  // Return false instead of silently dropping
        }
        client->mPendingCallbacks.push_back(PendingCallback{callbackId, payload});
        pendingCount = client->mPendingCallbacks.size();
    }

    LOG_I("ProxyIpcClient: callback queued proxy->remotediag name=%s id=%u payloadSize=%zu pending=%zu",
          rdgipc::callbackName(callbackId),
          callbackId,
          payload.size(),
          pendingCount);

    // Best-effort immediate flush; the drain loop retries if this fails,
    // so a queued callback still counts as success.
    if (!client->flushCallbacks()) {
        LOG_V("ProxyIpcClient::sendCallback: delivery deferred callback=%s", rdgipc::callbackName(callbackId));
    }

    return true;
}

bool ProxyIpcClient::start()
{
    if (!mTransport) {
        LOG_E("ProxyIpcClient: transport not initialized");
        return false;
    }

    bool expected{false};
    if (!mRunning.compare_exchange_strong(expected, true, std::memory_order_seq_cst)) {
        LOG_W("ProxyIpcClient: already started");
        return true;
    }

    // Set up the frame received callback
    mTransport->setFrameReceivedCallback(
        [this](const rdgipc::IpcTransport::Frame &frame) {
            onFrameReceived(frame);
        });

    // Start the transport (which starts the reader thread)
    if (!mTransport->start()) {
        mRunning.store(false, std::memory_order_seq_cst);
        LOG_E("ProxyIpcClient: failed to start transport");
        return false;
    }

    mCallbackDrainThread = std::thread([this]() { runCallbackDrainLoop(); });
    mRequestDispatchThread = std::thread([this]() { runRequestDispatchLoop(); });

    LOG_I("ProxyIpcClient: started");
    // If callbacks were queued before the connection came up, try to drain them now.
    (void)flushCallbacks();
    return true;
}

bool ProxyIpcClient::waitUntilReady(const uint32_t timeoutMs) noexcept
{
    if (!mTransport) {
        return false;
    }
    return mTransport->waitUntilConnected(timeoutMs);
}

void ProxyIpcClient::stop() noexcept
{
    if (!mRunning.exchange(false, std::memory_order_seq_cst)) {
        return;
    }

    if (mTransport) {
        mTransport->stop();
    }
    // Acquire the queue mutex before notifying so a dispatch thread between its
    // predicate check and blocking cannot miss the wakeup and hang the join.
    {
        const std::lock_guard<std::mutex> lock{mRequestQueueMutex};
    }
    mRequestCv.notify_all();
    if (mCallbackDrainThread.joinable()) {
        mCallbackDrainThread.join();
    }
    if (mRequestDispatchThread.joinable()) {
        mRequestDispatchThread.join();
    }
    LOG_I("ProxyIpcClient: stopped");
}

void ProxyIpcClient::runCallbackDrainLoop() noexcept
{
    while (mRunning.load(std::memory_order_seq_cst))
    {
        (void)flushCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ProxyIpcClient::runRequestDispatchLoop() noexcept
{
    while (true)
    {
        rdgipc::IpcTransport::Frame frame{};
        {
            std::unique_lock<std::mutex> lock{mRequestQueueMutex};
            mRequestCv.wait(lock, [this]() {
                return !mRunning.load(std::memory_order_seq_cst) || !mRequestQueue.empty();
            });
            if (!mRunning.load(std::memory_order_seq_cst))
            {
                return;
            }
            frame = std::move(mRequestQueue.front());
            mRequestQueue.pop_front();
        }
        handleRequest(frame);
    }
}

bool ProxyIpcClient::flushCallbacks() noexcept
{
    // Serialize concurrent flushers (drain loop, senders, reader thread) so
    // the front/send/pop sequence cannot duplicate or drop callbacks.
    const std::lock_guard<std::mutex> flushLock{mFlushMutex};

    while (true)
    {
        PendingCallback callback{};
        {
            const std::lock_guard<std::mutex> callbackLock{mCallbackMutex};
            if (mPendingCallbacks.empty())
            {
                return true;
            }
            callback = mPendingCallbacks.front();
        }

        if (!mTransport || !mTransport->isConnected())
        {
            LOG_V("ProxyIpcClient: callback waiting for connection name=%s id=%u",
                rdgipc::callbackName(callback.callbackId),
                callback.callbackId);
            return false;
        }

        // Oversized callbacks can never be sent; drop them instead of
        // blocking the queue head forever.
        if (callback.payload.size() > rdgipc::IpcTransport::kMaxPayloadSize)
        {
            LOG_E("ProxyIpcClient: dropping oversized callback id=%u size=%zu",
                  callback.callbackId, callback.payload.size());
            const std::lock_guard<std::mutex> callbackLock{mCallbackMutex};
            mPendingCallbacks.pop_front();
            continue;
        }

        // Send callback frame
        rdgipc::IpcTransport::Frame frame{
            static_cast<uint32_t>(rdgipc::MessageType::Callback),
            callback.callbackId,
            0U,
            callback.payload
        };

        if (!mTransport->sendFrame(frame))
        {
            LOG_W("ProxyIpcClient: callback send failed name=%s id=%u",
                  rdgipc::callbackName(callback.callbackId),
                  callback.callbackId);
            return false;
        }

        LOG_I("ProxyIpcClient: callback sent proxy->remotediag name=%s id=%u payloadSize=%zu",
              rdgipc::callbackName(callback.callbackId),
              callback.callbackId,
              callback.payload.size());

        {
            const std::lock_guard<std::mutex> callbackLock{mCallbackMutex};
            mPendingCallbacks.pop_front();
        }
    }
}

// ============================================================================
// NEW CALLBACK ARCHITECTURE - Frame received from transport
// ============================================================================

void ProxyIpcClient::onFrameReceived(const rdgipc::IpcTransport::Frame &frame)
{
    (void)flushCallbacks();

    if (frame.messageType == static_cast<uint32_t>(rdgipc::MessageType::Request)) {
        // Hand off to the dispatch thread so slow handlers cannot stall the
        // transport reader thread.
        {
            const std::lock_guard<std::mutex> lock{mRequestQueueMutex};
            if (mRequestQueue.size() >= kMaxPendingRequests) {
                LOG_E("ProxyIpcClient: request queue full, dropping request id=%u", frame.messageId);
                return;
            }
            mRequestQueue.push_back(frame);
        }
        mRequestCv.notify_one();
    } else {
        LOG_W("ProxyIpcClient: unexpected frame type=%u id=%u",
              frame.messageType, frame.messageId);
    }
}

void ProxyIpcClient::handleRequest(const rdgipc::IpcTransport::Frame &frame)
{
    const uint32_t commandId = frame.messageId;
    const auto& payload = frame.payload;

    LOG_I("ProxyIpcClient: request received remotediag->proxy name=%s id=%u payloadSize=%zu",
          rdgipc::commandName(static_cast<rdgipc::CommandId>(commandId)),
          commandId,
          payload.size());

    // Dispatch through the self-registering handler system
    const auto response = mDispatcher.dispatchCommand(commandId, payload);
    
    // Serialize response: [status:1byte][payload:N bytes]
    std::vector<uint8_t> responseData(1 + response.payload.size());
    responseData[0] = response.success ? 0x01 : 0x00;
    if (!response.payload.empty()) {
        std::memcpy(&responseData[1], response.payload.data(), response.payload.size());
    }

    // Send response frame
    rdgipc::IpcTransport::Frame responseFrame{
        static_cast<uint32_t>(rdgipc::MessageType::Response),
        commandId,
        frame.correlationId,
        responseData
    };

    if (!mTransport || !mTransport->sendFrame(responseFrame)) {
        LOG_W("ProxyIpcClient: failed to send response id=%u", commandId);
    } else {
        LOG_I("ProxyIpcClient: response sent proxy->remotediag name=%s id=%u status=%s payloadSize=%zu",
              rdgipc::commandName(static_cast<rdgipc::CommandId>(commandId)),
              commandId,
              response.success ? "OK" : "ERROR",
              response.payload.size());
    }
}
