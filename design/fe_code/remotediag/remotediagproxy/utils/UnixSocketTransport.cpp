#include "../include/UnixSocketTransport.h"
#include "Logger.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <exception>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

namespace rdgipc {

namespace {
constexpr int32_t kInvalidSocketFd{-1};
constexpr size_t kFrameHeaderSize{16U};  // messageType(4) + messageId(4) + correlationId(4) + payloadSize(4)
constexpr size_t kMaxFrameSize{IpcTransport::kMaxPayloadSize};
constexpr uint32_t kConnectRetryMs{100U};
constexpr uint32_t kMaxConnectRetryMs{5000U};
constexpr uint32_t kAcceptPollTimeoutMs{500U};
constexpr time_t kSendTimeoutSec{5};

// Bounds send() blocking so a frozen peer cannot wedge threads holding the socket lock.
void applySendTimeout(const int32_t fd) noexcept {
    timeval sendTimeout{};
    sendTimeout.tv_sec = kSendTimeoutSec;
    if (::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sizeof(sendTimeout)) != 0) {
        LOG_W("IpcTransport: failed to set send timeout fd=%d errno=%d", fd, errno);
    }
}

uint32_t readUint32(const uint8_t *data) {
    return (static_cast<uint32_t>(data[0]) << 24U) |
           (static_cast<uint32_t>(data[1]) << 16U) |
           (static_cast<uint32_t>(data[2]) << 8U) |
           static_cast<uint32_t>(data[3]);
}

void writeUint32(uint8_t *data, const uint32_t value) {
    data[0] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
    data[1] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    data[2] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    data[3] = static_cast<uint8_t>(value & 0xFFU);
}

bool sendAll(const int32_t fd, const uint8_t *buffer, const size_t size) noexcept {
    size_t sent{0U};
    while (sent < size) {
        const ssize_t rc{::send(fd, buffer + sent, size - sent, 0)};
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (rc == 0) {
            return false;
        }
        sent += static_cast<size_t>(rc);
    }
    return true;
}

bool recvAll(const int32_t fd, uint8_t *buffer, const size_t size) noexcept {
    size_t received{0U};
    while (received < size) {
        const ssize_t rc{::recv(fd, buffer + received, size - received, 0)};
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (rc == 0) {
            return false;
        }
        received += static_cast<size_t>(rc);
    }
    return true;
}

std::vector<uint8_t> serializeFrame(const IpcTransport::Frame &frame) {
    std::vector<uint8_t> buffer(kFrameHeaderSize + frame.payload.size());
    writeUint32(&buffer[0], frame.messageType);
    writeUint32(&buffer[4], frame.messageId);
    writeUint32(&buffer[8], frame.correlationId);
    writeUint32(&buffer[12], static_cast<uint32_t>(frame.payload.size()));
    if (!frame.payload.empty()) {
        std::memcpy(&buffer[kFrameHeaderSize], frame.payload.data(), frame.payload.size());
    }
    return buffer;
}

void deserializeFrame(const uint8_t *header, const std::vector<uint8_t> &payload, IpcTransport::Frame &frame) {
    frame.messageType = readUint32(header);
    frame.messageId = readUint32(header + 4U);
    frame.correlationId = readUint32(header + 8U);
    frame.payload = payload;
}

std::string getParentDirectory(const std::string &path) {
    const std::string::size_type slashPos{path.find_last_of('/')};
    if (slashPos == std::string::npos) {
        return std::string{};
    }
    return path.substr(0U, slashPos);
}

} // anonymous namespace

// ============================================================================
// UnixSocketClient Implementation
// ============================================================================

UnixSocketClient::UnixSocketClient(const std::string &socketPath)
    : mSocketPath(socketPath), mSocketFd(kInvalidSocketFd), mRunning(false) {
}

UnixSocketClient::~UnixSocketClient() noexcept {
    stop();
}

bool UnixSocketClient::start() noexcept {
    bool expected{false};
    if (!mRunning.compare_exchange_strong(expected, true, std::memory_order_seq_cst)) {
        LOG_W("UnixSocketClient: already started");
        return true;
    }

    mReaderThread = std::thread([this]() { runReaderLoop(); });
    LOG_I("UnixSocketClient: started path=%s", mSocketPath.c_str());
    return true;
}

void UnixSocketClient::stop() noexcept {
    if (!mRunning.exchange(false, std::memory_order_seq_cst)) {
        return;
    }

    closeSocket();
    if (mReaderThread.joinable()) {
        mReaderThread.join();
    }
    LOG_I("UnixSocketClient: stopped");
}

bool UnixSocketClient::sendFrame(const Frame &frame) noexcept {
    const std::lock_guard<std::mutex> lock{mSendMutex};
    return sendFrameInternal(frame);
}

bool UnixSocketClient::sendFrameInternal(const Frame &frame) noexcept {
    if (frame.payload.size() > kMaxFrameSize) {
        LOG_E("UnixSocketClient: frame too large to send size=%zu", frame.payload.size());
        return false;
    }

    const std::vector<uint8_t> buffer{serializeFrame(frame)};

    // Hold the socket lock across the write so a concurrent close/reconnect
    // cannot recycle the FD mid-send.
    const std::lock_guard<std::mutex> lock{mSocketMutex};
    if (mSocketFd < 0) {
        LOG_W("UnixSocketClient: cannot send, not connected");
        return false;
    }

    if (!sendAll(mSocketFd, buffer.data(), buffer.size())) {
        LOG_E("UnixSocketClient: send failed errno=%d", errno);
        // The reader thread owns close/reconnect; shutdown just wakes it.
        (void)::shutdown(mSocketFd, SHUT_RDWR);
        return false;
    }

    LOG_V("UnixSocketClient: sent frame type=%u id=%u payloadSize=%zu",
          frame.messageType, frame.messageId, frame.payload.size());
    return true;
}

void UnixSocketClient::setFrameReceivedCallback(FrameReceivedCallback callback) noexcept {
    const std::lock_guard<std::mutex> lock{mCallbackMutex};
    mFrameCallback = callback;
}

bool UnixSocketClient::isConnected() const noexcept {
    const std::lock_guard<std::mutex> lock{mSocketMutex};
    return mSocketFd >= 0;
}

bool UnixSocketClient::waitUntilConnected(const uint32_t timeoutMs) noexcept {
    std::unique_lock<std::mutex> lock{mSocketMutex};
    return mConnectCv.wait_for(lock,
                               std::chrono::milliseconds(timeoutMs),
                               [this]() { return mSocketFd >= 0; });
}

void UnixSocketClient::runReaderLoop() {
    uint32_t backoffMs{kConnectRetryMs};

    while (mRunning.load(std::memory_order_seq_cst)) {
        int32_t socketFd{kInvalidSocketFd};
        {
            const std::lock_guard<std::mutex> lock{mSocketMutex};
            socketFd = mSocketFd;
        }
        
        if (socketFd < 0) {
            if (!connectToServer()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
                backoffMs = std::min(backoffMs * 2U, kMaxConnectRetryMs);
                continue;
            }
            backoffMs = kConnectRetryMs;
            // Re-read the fresh FD on the next iteration.
            continue;
        }

        uint8_t header[kFrameHeaderSize]{};
        if (!recvAll(socketFd, header, kFrameHeaderSize)) {
            LOG_W("UnixSocketClient: failed to read frame header errno=%d", errno);
            closeSocket();
            continue;
        }

        const uint32_t payloadSize{readUint32(header + 12U)};
        if (payloadSize > kMaxFrameSize) {
            LOG_E("UnixSocketClient: frame too large size=%u", payloadSize);
            closeSocket();
            continue;
        }

        std::vector<uint8_t> payload(payloadSize);
        if ((payloadSize > 0U) && !recvAll(socketFd, payload.data(), payloadSize)) {
            LOG_W("UnixSocketClient: failed to read frame payload errno=%d", errno);
            closeSocket();
            continue;
        }

        Frame frame{};
        deserializeFrame(header, payload, frame);
        LOG_V("UnixSocketClient: received frame type=%u id=%u corr=%u payloadSize=%u",
            frame.messageType, frame.messageId, frame.correlationId, static_cast<uint32_t>(frame.payload.size()));

        FrameReceivedCallback callback;
        {
            const std::lock_guard<std::mutex> lock{mCallbackMutex};
            callback = mFrameCallback;
        }
        if (callback) {
            try {
                callback(frame);
            } catch (const std::exception &e) {
                LOG_E("UnixSocketClient: frame callback threw: %s", e.what());
            } catch (...) {
                LOG_E("UnixSocketClient: frame callback threw unknown exception");
            }
        }
    }

    closeSocket();
}

bool UnixSocketClient::connectToServer() {
    closeSocket();

    const int32_t sock{::socket(AF_UNIX, SOCK_STREAM, 0)};
    if (sock < 0) {
        LOG_W("UnixSocketClient: socket failed errno=%d", errno);
        return false;
    }

    sockaddr_un serverAddr{};
    serverAddr.sun_family = AF_UNIX;
    if (mSocketPath.empty() || (mSocketPath.size() >= sizeof(serverAddr.sun_path))) {
        LOG_E("UnixSocketClient: invalid socket path=%s", mSocketPath.c_str());
        (void)::close(sock);
        return false;
    }
    std::snprintf(serverAddr.sun_path, sizeof(serverAddr.sun_path), "%s", mSocketPath.c_str());

    if (::connect(sock, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) != 0) {
        LOG_V("UnixSocketClient: connect failed path=%s errno=%d", mSocketPath.c_str(), errno);
        (void)::close(sock);
        return false;
    }

    applySendTimeout(sock);

    {
        const std::lock_guard<std::mutex> lock{mSocketMutex};
        mSocketFd = sock;
    }
    mConnectCv.notify_all();
    LOG_I("UnixSocketClient: connected fd=%d", sock);
    return true;
}

void UnixSocketClient::closeSocket() noexcept {
    const std::lock_guard<std::mutex> lock{mSocketMutex};
    if (mSocketFd >= 0) {
        const int32_t fd = mSocketFd;
        mSocketFd = kInvalidSocketFd;
        // shutdown() wakes any thread blocked in recv(); close() alone does not.
        (void)::shutdown(fd, SHUT_RDWR);
        (void)::close(fd);
        LOG_V("UnixSocketClient: socket closed fd=%d", fd);
    }
}

// ============================================================================
// UnixSocketServer Implementation
// ============================================================================

UnixSocketServer::UnixSocketServer(const std::string &socketPath)
    : mSocketPath(socketPath), mServerFd(kInvalidSocketFd), mClientFd(kInvalidSocketFd), mRunning(false) {
}

UnixSocketServer::~UnixSocketServer() noexcept {
    stop();
}

bool UnixSocketServer::start() noexcept {
    if (!createServerSocket()) {
        LOG_E("UnixSocketServer: failed to create server socket");
        return false;
    }

    bool expected{false};
    if (!mRunning.compare_exchange_strong(expected, true, std::memory_order_seq_cst)) {
        LOG_W("UnixSocketServer: already started");
        return true;
    }

    mReaderThread = std::thread([this]() { runReaderLoop(); });
    LOG_I("UnixSocketServer: started and listening path=%s", mSocketPath.c_str());
    return true;
}

void UnixSocketServer::stop() noexcept {
    mRunning.store(false, std::memory_order_seq_cst);
    closeClientSocket();
    closeServerSocket();
    
    if (mReaderThread.joinable()) {
        mReaderThread.join();
    }
    LOG_I("UnixSocketServer: stopped");
}

bool UnixSocketServer::sendFrame(const Frame &frame) noexcept {
    const std::lock_guard<std::mutex> sendLock{mSendMutex};

    if (frame.payload.size() > kMaxFrameSize) {
        LOG_E("UnixSocketServer: frame too large to send size=%zu", frame.payload.size());
        return false;
    }

    const std::vector<uint8_t> buffer{serializeFrame(frame)};

    // Hold the socket lock across the write so a concurrent close/re-accept
    // cannot recycle the FD mid-send.
    const std::lock_guard<std::mutex> socketLock{mSocketMutex};
    if (mClientFd < 0) {
        LOG_W("UnixSocketServer: cannot send, no client connected");
        return false;
    }

    if (!sendAll(mClientFd, buffer.data(), buffer.size())) {
        LOG_E("UnixSocketServer: send failed errno=%d", errno);
        // The reader thread owns close/re-accept; shutdown just wakes it.
        (void)::shutdown(mClientFd, SHUT_RDWR);
        return false;
    }

    LOG_V("UnixSocketServer: sent frame type=%u id=%u payloadSize=%zu",
          frame.messageType, frame.messageId, frame.payload.size());
    return true;
}

void UnixSocketServer::setFrameReceivedCallback(FrameReceivedCallback callback) noexcept {
    const std::lock_guard<std::mutex> lock{mCallbackMutex};
    mFrameCallback = callback;
}

bool UnixSocketServer::isConnected() const noexcept {
    const std::lock_guard<std::mutex> lock{mSocketMutex};
    return mClientFd >= 0;
}

bool UnixSocketServer::waitUntilConnected(const uint32_t timeoutMs) noexcept {
    std::unique_lock<std::mutex> lock{mSocketMutex};
    return mConnectCv.wait_for(lock,
                               std::chrono::milliseconds(timeoutMs),
                               [this]() { return mClientFd >= 0; });
}

void UnixSocketServer::runReaderLoop() {
    while (mRunning.load(std::memory_order_seq_cst)) {
        int32_t clientFd{kInvalidSocketFd};
        int32_t serverFd{kInvalidSocketFd};
        {
            const std::lock_guard<std::mutex> socketLock{mSocketMutex};
            clientFd = mClientFd;
            serverFd = mServerFd;
        }

        if (clientFd == kInvalidSocketFd) {
            if (serverFd == kInvalidSocketFd) {
                break;
            }

            // Wait for client connection
            fd_set readFds{};
            FD_ZERO(&readFds);
            FD_SET(serverFd, &readFds);

            timeval timeout{};
            timeout.tv_sec = static_cast<time_t>(kAcceptPollTimeoutMs / 1000U);
            timeout.tv_usec = static_cast<suseconds_t>((kAcceptPollTimeoutMs % 1000U) * 1000U);

            const int32_t rc{::select(serverFd + 1, &readFds, nullptr, nullptr, &timeout)};
            if (rc <= 0) {
                continue;
            }
            if (!mRunning.load(std::memory_order_seq_cst)) {
                break;
            }

            const int32_t acceptedFd{::accept(serverFd, nullptr, nullptr)};
            if (acceptedFd < 0) {
                LOG_W("UnixSocketServer: accept failed errno=%d", errno);
                continue;
            }

            applySendTimeout(acceptedFd);

            {
                const std::lock_guard<std::mutex> socketLock{mSocketMutex};
                mClientFd = acceptedFd;
            }
            mConnectCv.notify_all();
            LOG_I("UnixSocketServer: client connected fd=%d", acceptedFd);
            continue;
        }

        uint8_t header[kFrameHeaderSize]{};
        if (!recvAll(clientFd, header, kFrameHeaderSize)) {
            LOG_W("UnixSocketServer: failed to read frame header errno=%d", errno);
            closeClientSocket();
            continue;
        }

        const uint32_t payloadSize{readUint32(header + 12U)};
        if (payloadSize > kMaxFrameSize) {
            LOG_E("UnixSocketServer: frame too large size=%u", payloadSize);
            closeClientSocket();
            continue;
        }

        std::vector<uint8_t> payload(payloadSize);
        if ((payloadSize > 0U) && !recvAll(clientFd, payload.data(), payloadSize)) {
            LOG_W("UnixSocketServer: failed to read frame payload errno=%d", errno);
            closeClientSocket();
            continue;
        }

        Frame frame{};
        deserializeFrame(header, payload, frame);
        LOG_V("UnixSocketServer: received frame type=%u id=%u corr=%u payloadSize=%u",
            frame.messageType, frame.messageId, frame.correlationId, static_cast<uint32_t>(frame.payload.size()));

        FrameReceivedCallback callback;
        {
            const std::lock_guard<std::mutex> lock{mCallbackMutex};
            callback = mFrameCallback;
        }
        if (callback) {
            try {
                callback(frame);
            } catch (const std::exception &e) {
                LOG_E("UnixSocketServer: frame callback threw: %s", e.what());
            } catch (...) {
                LOG_E("UnixSocketServer: frame callback threw unknown exception");
            }
        }
    }
}

bool UnixSocketServer::createServerSocket() noexcept {
    {
        const std::lock_guard<std::mutex> lock{mSocketMutex};
        if (mServerFd != kInvalidSocketFd) {
            return true;
        }
    }

    const std::string parentDir{getParentDirectory(mSocketPath)};
    if (!parentDir.empty() && (::access(parentDir.c_str(), F_OK) != 0)) {
        if ((::mkdir(parentDir.c_str(), 0775) != 0) && (errno != EEXIST)) {
            LOG_E("UnixSocketServer: mkdir failed path=%s errno=%d", parentDir.c_str(), errno);
            return false;
        }
    }

    const int32_t serverFd{::socket(AF_UNIX, SOCK_STREAM, 0)};
    if (serverFd < 0) {
        LOG_E("UnixSocketServer: socket() failed errno=%d", errno);
        return false;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (mSocketPath.empty() || (mSocketPath.size() >= sizeof(addr.sun_path))) {
        LOG_E("UnixSocketServer: invalid socket path=%s", mSocketPath.c_str());
        (void)::close(serverFd);
        return false;
    }

    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", mSocketPath.c_str());
    (void)::unlink(mSocketPath.c_str());

    if (::bind(serverFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        LOG_E("UnixSocketServer: bind failed path=%s errno=%d", mSocketPath.c_str(), errno);
        (void)::close(serverFd);
        return false;
    }

    if (::listen(serverFd, 1) != 0) {
        LOG_E("UnixSocketServer: listen failed path=%s errno=%d", mSocketPath.c_str(), errno);
        (void)::close(serverFd);
        (void)::unlink(mSocketPath.c_str());
        return false;
    }

    {
        const std::lock_guard<std::mutex> lock{mSocketMutex};
        mServerFd = serverFd;
    }
    LOG_I("UnixSocketServer: listening fd=%d path=%s", serverFd, mSocketPath.c_str());
    return true;
}

void UnixSocketServer::closeClientSocket() noexcept {
    const std::lock_guard<std::mutex> lock{mSocketMutex};
    if (mClientFd != kInvalidSocketFd) {
        const int32_t fd = mClientFd;
        mClientFd = kInvalidSocketFd;
        // shutdown() wakes any thread blocked in recv(); close() alone does not.
        (void)::shutdown(fd, SHUT_RDWR);
        (void)::close(fd);
        LOG_V("UnixSocketServer: client socket closed fd=%d", fd);
    }
}

void UnixSocketServer::closeServerSocket() noexcept {
    const std::lock_guard<std::mutex> lock{mSocketMutex};
    if (mServerFd != kInvalidSocketFd) {
        const int32_t fd = mServerFd;
        mServerFd = kInvalidSocketFd;
        (void)::close(fd);
        (void)::unlink(mSocketPath.c_str());
        LOG_V("UnixSocketServer: server socket closed fd=%d", fd);
    }
}

} // namespace rdgipc
