// ipc/transport/UnixSocketClient.h
#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include "ISocketCallback.h"

class UnixSocketClient {
public:
    UnixSocketClient();
    ~UnixSocketClient();

    bool start(const std::string& socketPath, ISocketCallback* callback);
    void stop();
    bool sendData(const uint8_t* data, size_t len);
    bool isConnected() const { return mIsConnected; }

private:
    void connectAndRecvLoop();

    std::string mSocketPath;
    ISocketCallback* mCallback{nullptr};
    int mClientFd{-1};
    std::atomic<bool> mRunning{false};
    std::atomic<bool> mIsConnected{false};
    std::thread mLoopThread;
    std::mutex mSendMutex;
};
