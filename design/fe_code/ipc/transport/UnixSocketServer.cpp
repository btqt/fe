// ipc/transport/UnixSocketServer.cpp
#include "UnixSocketServer.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>

UnixSocketServer::UnixSocketServer() = default;
UnixSocketServer::~UnixSocketServer() { stop(); }

bool UnixSocketServer::start(const std::string& socketPath, ISocketCallback* callback) {
    if (mRunning) return false;
    mSocketPath = socketPath;
    mCallback = callback;

    unlink(mSocketPath.c_str());
    mServerFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (mServerFd < 0) return false;

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, mSocketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(mServerFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(mServerFd);
        mServerFd = -1;
        return false;
    }

    if (listen(mServerFd, 1) < 0) {
        close(mServerFd);
        mServerFd = -1;
        return false;
    }

    mRunning = true;
    mLoopThread = std::thread(&UnixSocketServer::acceptAndRecvLoop, this);
    return true;
}

void UnixSocketServer::stop() {
    mRunning = false;
    if (mServerFd >= 0) { close(mServerFd); mServerFd = -1; }
    if (mClientFd >= 0) { close(mClientFd); mClientFd = -1; }
    if (mLoopThread.joinable()) mLoopThread.join();
    mIsConnected = false;
}

bool UnixSocketServer::sendData(const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(mSendMutex);
    if (!mIsConnected || mClientFd < 0) return false;
    ssize_t ret = write(mClientFd, data, len);
    return ret == static_cast<ssize_t>(len);
}

void UnixSocketServer::acceptAndRecvLoop() {
    while (mRunning) {
        mClientFd = accept(mServerFd, nullptr, nullptr);
        if (mClientFd < 0) break;
        mIsConnected = true;
        if (mCallback) mCallback->onConnected();

        std::vector<uint8_t> buf(4096);
        while (mRunning && mIsConnected) {
            ssize_t bytes = read(mClientFd, buf.data(), buf.size());
            if (bytes <= 0) {
                mIsConnected = false;
                close(mClientFd);
                mClientFd = -1;
                if (mCallback) mCallback->onDisconnected();
                break;
            }
            if (mCallback) mCallback->onDataReceived(buf.data(), static_cast<size_t>(bytes));
        }
    }
}
