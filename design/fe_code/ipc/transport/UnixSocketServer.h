// ipc/transport/UnixSocketServer.h
#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include "ISocketCallback.h"

class UnixSocketServer {
public:
    UnixSocketServer();
    ~UnixSocketServer();

    bool start(const std::string& socketPath, ISocketCallback* callback);
    void stop();
    bool sendData(const uint8_t* data, size_t len);
    bool isConnected() const { return mIsConnected; }

private:
    void acceptAndRecvLoop();

    std::string mSocketPath;
    ISocketCallback* mCallback{nullptr};
    int mServerFd{-1};
    int mClientFd{-1};
    std::atomic<bool> mRunning{false};
    std::atomic<bool> mIsConnected{false};
    std::thread mLoopThread;
    std::mutex mSendMutex;
};
