// ipc/protocol/ProxyIpcServer.h
#pragma once
#include "../transport/UnixSocketServer.h"
#include "IpcFrame.h"
#include <map>
#include <mutex>
#include <condition_variable>
#include <functional>

class ProxyIpcServer : public ISocketCallback {
public:
    using CallbackDispatcherFunc = std::function<void(uint16_t callbackId, const std::vector<uint8_t>& payload)>;

    static ProxyIpcServer* getInstance();

    bool start(const std::string& socketPath);
    void stop();
    void setCallbackDispatcher(CallbackDispatcherFunc dispatcher);

    struct ApiResponse {
        bool success{false};
        std::vector<uint8_t> payload;
    };

    ApiResponse requestAPICall(uint16_t commandId, const std::vector<uint8_t>& payload, uint32_t timeoutMs = 3000);

    // ISocketCallback
    void onDataReceived(const uint8_t* data, size_t len) override;
    void onConnected() override;
    void onDisconnected() override;

private:
    ProxyIpcServer() = default;
    UnixSocketServer mServer;
    CallbackDispatcherFunc mDispatcher;

    std::mutex mResponseMutex;
    std::condition_variable mResponseCv;
    std::map<uint32_t, ApiResponse> mPendingResponses;
    std::atomic<uint32_t> mNextCorrelationId{1};

    std::vector<uint8_t> mRecvBuf;
    std::mutex mRecvMutex;
};
