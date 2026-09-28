#ifndef RDG_UNIX_SOCKET_TRANSPORT_H
#define RDG_UNIX_SOCKET_TRANSPORT_H

#include "IpcTransport.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace rdgipc {

/**
 * @brief Unix socket client transport
 * 
 * This class handles client-side socket operations (connect, send, receive).
 * It is stable and doesn't need changes when adding new commands.
 */
class UnixSocketClient : public IpcTransport {
public:
    explicit UnixSocketClient(const std::string &socketPath);
    ~UnixSocketClient() noexcept override;

    bool start() noexcept override;
    void stop() noexcept override;
    bool sendFrame(const Frame &frame) noexcept override;
    void setFrameReceivedCallback(FrameReceivedCallback callback) noexcept override;
    bool isConnected() const noexcept override;
    bool waitUntilConnected(uint32_t timeoutMs) noexcept override;

private:
    void runReaderLoop();
    bool connectToServer();
    void closeSocket() noexcept;
    bool sendFrameInternal(const Frame &frame) noexcept;

    std::string mSocketPath;
    int32_t mSocketFd;
    std::atomic<bool> mRunning;
    mutable std::mutex mSocketMutex;  // Protects mSocketFd (mutable for const methods)
    std::condition_variable mConnectCv;  // Signalled when a connection is established
    std::mutex mSendMutex;
    mutable std::mutex mCallbackMutex;  // Protects mFrameCallback (mutable for const methods)
    std::thread mReaderThread;
    FrameReceivedCallback mFrameCallback;
};

/**
 * @brief Unix socket server transport
 * 
 * This class handles server-side socket operations (listen, accept, send, receive).
 * It is stable and doesn't need changes when adding new commands.
 */
class UnixSocketServer : public IpcTransport {
public:
    explicit UnixSocketServer(const std::string &socketPath);
    ~UnixSocketServer() noexcept override;

    bool start() noexcept override;
    void stop() noexcept override;
    bool sendFrame(const Frame &frame) noexcept override;
    void setFrameReceivedCallback(FrameReceivedCallback callback) noexcept override;
    bool isConnected() const noexcept override;
    bool waitUntilConnected(uint32_t timeoutMs) noexcept override;

private:
    void runReaderLoop();
    bool createServerSocket() noexcept;
    void closeClientSocket() noexcept;
    void closeServerSocket() noexcept;

    std::string mSocketPath;
    int32_t mServerFd;
    int32_t mClientFd;
    std::atomic<bool> mRunning;
    mutable std::mutex mSocketMutex;  // Protects mClientFd/mServerFd (mutable for const methods)
    std::condition_variable mConnectCv;  // Signalled when a client connection is accepted
    std::mutex mSendMutex;
    mutable std::mutex mCallbackMutex;  // Protects mFrameCallback (mutable for const methods)
    std::thread mReaderThread;
    FrameReceivedCallback mFrameCallback;
};

} // namespace rdgipc

#endif // RDG_UNIX_SOCKET_TRANSPORT_H
