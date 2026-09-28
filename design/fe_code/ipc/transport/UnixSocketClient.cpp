// ipc/transport/UnixSocketClient.cpp
#include "UnixSocketClient.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <cstring>

UnixSocketClient::UnixSocketClient() = default;
UnixSocketClient::~UnixSocketClient() { stop(); }

bool UnixSocketClient::start(const std::string& socketPath, ISocketCallback* callback) {
    if (mRunning) return false;
    mSocketPath = socketPath;
    mCallback = callback;
    mRunning = true;
    mLoopThread = std::thread(&UnixSocketClient::connectAndRecvLoop, this);
    return true;
}

void UnixSocketClient::stop() {
    mRunning = false;
    if (mClientFd >= 0) { close(mClientFd); mClientFd = -1; }
    if (mLoopThread.joinable()) mLoopThread.join();
    mIsConnected = false;
}

bool UnixSocketClient::sendData(const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(mSendMutex);
    if (!mIsConnected || mClientFd < 0) return false;
    ssize_t ret = write(mClientFd, data, len);
    return ret == static_cast<ssize_t>(len);
}

void UnixSocketClient::connectAndRecvLoop() {
    while (mRunning) {
        mClientFd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (mClientFd < 0) { sleep(1); continue; }

        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, mSocketPath.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(mClientFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(mClientFd);
            mClientFd = -1;
            usleep(200000); // retry after 200ms
            continue;
        }

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
