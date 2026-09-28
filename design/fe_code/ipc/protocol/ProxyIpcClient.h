// ipc/protocol/ProxyIpcClient.h
#pragma once
#include "../transport/UnixSocketClient.h"
#include "IpcFrame.h"
#include <functional>
#include <vector>
#include <mutex>

class ProxyIpcClient : public ISocketCallback {
public:
    using CommandHandlerFunc = std::function<std::vector<uint8_t>(uint16_t commandId, const std::vector<uint8_t>& payload, bool& success)>;

    static ProxyIpcClient* getInstance();

    bool start(const std::string& socketPath);
    void stop();
    void setCommandHandler(CommandHandlerFunc handler);

    void sendCallback(uint16_t callbackId, const std::vector<uint8_t>& payload);

    // ISocketCallback
    void onDataReceived(const uint8_t* data, size_t len) override;
    void onConnected() override;
    void onDisconnected() override;

private:
    ProxyIpcClient() = default;
    UnixSocketClient mClient;
    CommandHandlerFunc mHandler;

    std::vector<uint8_t> mRecvBuf;
    std::mutex mRecvMutex;
};
