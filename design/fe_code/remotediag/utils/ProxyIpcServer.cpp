#include "ProxyIpcServer.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <utils/Message.h>

#include "Logger.h"
#include "../remotediagproxy/include/UnixSocketTransport.h"
#include "../remotediagproxy/include/IpcMessageHandler.h"
#include <services/DcemqttproxyManagerService/DceNotification.h>
#include "RemotediagHandler.h"

namespace rdgapp {

namespace {
constexpr const char *kDefaultProxySocketPath{"/dev/socket/remotediag/remotediag_proxy.sock"};
constexpr size_t kMaxPendingCallbackFrames{64U};

std::string resolveSocketPath() {
    const char *envPath{std::getenv("RDG_PROXY_SOCKET_PATH")};
    if ((envPath != nullptr) && (envPath[0] != '\0')) {
        return std::string{envPath};
    }
    return std::string{kDefaultProxySocketPath};
}

} // namespace

ProxyIpcServer &ProxyIpcServer::getInstance() noexcept {
    static ProxyIpcServer instance{};
    return instance;
}

void ProxyIpcServer::registerCallbackHandler(std::shared_ptr<rdgipc::ICallbackHandler> handler,
                                             const std::vector<uint32_t> &callbackIds) {
    LOG_I("ProxyIpcServer: registering callback handler for %zu callbacks", callbackIds.size());
    mDispatcher.registerCallbackHandler(handler, callbackIds);
}

ProxyIpcServer::~ProxyIpcServer() noexcept {
    mRunning.store(false, std::memory_order_seq_cst);
    if (mTransport) {
        mTransport->stop();
    }
    // Acquire the queue mutex before notifying so a dispatch thread between its
    // predicate check and blocking cannot miss the wakeup and hang the join.
    {
        const std::lock_guard<std::mutex> lock{mCallbackQueueMutex};
    }
    mCallbackQueueCv.notify_all();
    if (mCallbackDispatchThread.joinable()) {
        mCallbackDispatchThread.join();
    }
    LOG_I("ProxyIpcServer: destroyed");
}

bool ProxyIpcServer::start() noexcept {
    bool expected{false};
    if (!mRunning.compare_exchange_strong(expected, true, std::memory_order_seq_cst)) {
        LOG_W("ProxyIpcServer: already started");
        return true;
    }

    // Create transport if not already created
    if (!mTransport) {
        mTransport = std::make_shared<rdgipc::UnixSocketServer>(resolveSocketPath());
    }

    // Set up the frame received callback
    mTransport->setFrameReceivedCallback(
        [this](const rdgipc::IpcTransport::Frame &frame) {
            onFrameReceived(frame);
        });

    // Start the transport (which creates the server socket and starts the reader thread)
    if (!mTransport->start()) {
        mRunning.store(false, std::memory_order_seq_cst);
        LOG_E("ProxyIpcServer: failed to start transport");
        return false;
    }

    mCallbackDispatchThread = std::thread([this]() { runCallbackDispatchLoop(); });

    LOG_I("ProxyIpcServer: started and listening");
    return true;
}

bool ProxyIpcServer::waitUntilReady(const uint32_t timeoutMs) noexcept {
    if (!mTransport) {
        return false;
    }
    return mTransport->waitUntilConnected(timeoutMs);
}

bool ProxyIpcServer::requestAPICall(const rdgipc::CommandId commandId,
                                    const std::vector<uint8_t> &payload,
                                    std::vector<uint8_t> &response,
                                    const uint32_t timeoutMs) noexcept {
    response.clear();

    if (!mTransport || !mTransport->isConnected()) {
        LOG_E("ProxyIpcServer: transport not connected request=%s", rdgipc::commandName(commandId));
        return false;
    }

    const std::lock_guard<std::mutex> requestLock{mRequestMutex};
    uint32_t correlationId{mNextRequestCorrelationId++};
    if (correlationId == 0U)
    {
        correlationId = mNextRequestCorrelationId++;
    }

    LOG_I("ProxyIpcServer: request start remotediag->proxy request=%s corr=%u payloadSize=%zu timeoutMs=%u",
          rdgipc::commandName(commandId), correlationId, payload.size(), timeoutMs);

    // Create request frame
    rdgipc::IpcTransport::Frame requestFrame{
        static_cast<uint32_t>(rdgipc::MessageType::Request),
        static_cast<uint32_t>(commandId),
        correlationId,
        payload
    };

    // Set up response waiting
    {
        const std::lock_guard<std::mutex> responseLock{mResponseMutex};
        mPendingResponseId = static_cast<uint32_t>(commandId);
        mPendingResponseCorrelationId = correlationId;
        mPendingResponsePayload.clear();
        mWaitingForResponse = true;
    }

    // Send request through transport
    if (!mTransport->sendFrame(requestFrame)) {
        const std::lock_guard<std::mutex> responseLock{mResponseMutex};
        mWaitingForResponse = false;
        mPendingResponseId = 0U;
        mPendingResponseCorrelationId = 0U;
        LOG_E("ProxyIpcServer: failed to send request=%s", rdgipc::commandName(commandId));
        return false;
    }

    // Wait for response
    std::unique_lock<std::mutex> responseLock{mResponseMutex};
    const bool gotResponse{mResponseCv.wait_for(
        responseLock,
        std::chrono::milliseconds(timeoutMs),
        [this]() { return !mWaitingForResponse; })};

    if (!gotResponse) {
        mWaitingForResponse = false;
        mPendingResponseId = 0U;
        mPendingResponseCorrelationId = 0U;
        LOG_W("ProxyIpcServer: timeout waiting for response request=%s", rdgipc::commandName(commandId));
        return false;
    }

    // Parse response: [status:1byte][payload:N bytes]
    const std::vector<uint8_t> responseBytes{std::move(mPendingResponsePayload)};
    mPendingResponseId = 0U;
    mPendingResponseCorrelationId = 0U;
    responseLock.unlock();

    if (responseBytes.empty()) {
        LOG_E("ProxyIpcServer: empty response from proxy request=%s", rdgipc::commandName(commandId));
        return false;
    }

    const bool ok{responseBytes[0] == 0x01};
    response.assign(responseBytes.begin() + 1, responseBytes.end());

        LOG_I("ProxyIpcServer: request complete remotediag->proxy request=%s corr=%u status=%s responseSize=%zu",
          rdgipc::commandName(commandId),
            correlationId,
          ok ? "OK" : "ERROR",
          response.size());
    return ok;
}

// ============================================================================
// NEW CALLBACK ARCHITECTURE - Frame received from transport
// ============================================================================

void ProxyIpcServer::onFrameReceived(const rdgipc::IpcTransport::Frame &frame)
{
    if (frame.messageType == static_cast<uint32_t>(rdgipc::MessageType::Callback)) {
        handleCallback(frame);
    } else if (frame.messageType == static_cast<uint32_t>(rdgipc::MessageType::Response)) {
        handleResponse(frame);
    } else {
        LOG_W("ProxyIpcServer: unexpected frame type=%u id=%u",
              frame.messageType, frame.messageId);
    }
}

void ProxyIpcServer::handleCallback(const rdgipc::IpcTransport::Frame &frame) noexcept {
    LOG_I("ProxyIpcServer: callback received remotediag<-proxy name=%s id=%u payloadSize=%zu",
          rdgipc::callbackName(frame.messageId),
          frame.messageId,
          frame.payload.size());

    // Hand off to the dispatch thread so slow callback handlers cannot block
    // response frames on the transport reader thread.
    {
        const std::lock_guard<std::mutex> lock{mCallbackQueueMutex};
        if (mCallbackQueue.size() >= kMaxPendingCallbackFrames) {
            LOG_E("ProxyIpcServer: callback queue full, dropping callback id=%u", frame.messageId);
            return;
        }
        mCallbackQueue.push_back(frame);
    }
    mCallbackQueueCv.notify_one();
}

void ProxyIpcServer::runCallbackDispatchLoop() noexcept {
    while (true) {
        rdgipc::IpcTransport::Frame frame{};
        {
            std::unique_lock<std::mutex> lock{mCallbackQueueMutex};
            mCallbackQueueCv.wait(lock, [this]() {
                return !mRunning.load(std::memory_order_seq_cst) || !mCallbackQueue.empty();
            });
            if (!mRunning.load(std::memory_order_seq_cst)) {
                return;
            }
            frame = std::move(mCallbackQueue.front());
            mCallbackQueue.pop_front();
        }
        // Route all callbacks through the dispatcher to self-registering handlers
        mDispatcher.dispatchCallback(frame.messageId, frame.payload);
    }
}

void ProxyIpcServer::handleResponse(const rdgipc::IpcTransport::Frame &frame)
{
    const std::lock_guard<std::mutex> responseLock{mResponseMutex};
    
    if (!mWaitingForResponse ||
        (frame.messageId != mPendingResponseId) ||
        (frame.correlationId != mPendingResponseCorrelationId)) {
        LOG_W("ProxyIpcServer: unexpected response id=%u corr=%u expectedId=%u expectedCorr=%u waiting=%d",
              frame.messageId,
              frame.correlationId,
              mPendingResponseId,
              mPendingResponseCorrelationId,
              mWaitingForResponse ? 1 : 0);
        return;
    }
    
    mPendingResponsePayload = frame.payload;
    mWaitingForResponse = false;
    mResponseCv.notify_all();
}

} // namespace rdgapp
