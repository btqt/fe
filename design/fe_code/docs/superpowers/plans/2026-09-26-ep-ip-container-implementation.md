# EP/IP Container Architecture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the 2-container architecture (EP + IP) with Unix Domain Socket IPC, Adapter Mirror Pattern, RemoteDiagProxy forwarding daemon, and UDS SID filtering.

**Architecture:** Shared low-level Unix socket transport and binary framing protocol in `ipc/`. IP container (`remotediag/`) utilizes mirror adapters that route API requests through `ProxyIpcServer` without modifying the core `Remotediag` application singleton. EP container (`remotediag_proxy/`) runs a lightweight proxy daemon that bridges Android Binder IPC services and the IPC socket channel.

**Tech Stack:** C++14/17, Linux Unix Domain Sockets (`AF_UNIX`), POSIX Threads/Mutexes, Custom Binary Protocol.

---

## Task Decomposition Overview

- **Task 1: Shared IPC Transport Layer** (`ipc/transport/`)
- **Task 2: Shared IPC Framing Protocol & Codec** (`ipc/protocol/`)
- **Task 3: High-Level IPC Server/Client Classes** (`ProxyIpcServer`, `ProxyIpcClient`)
- **Task 4: EP Container - RemoteDiagProxy Core Daemon & Forwarding Engine** (`remotediag_proxy/`)
- **Task 5: EP Container - Command Handlers & Real Binder Adapters** (`remotediag_proxy/forwarding/`, `remotediag_proxy/services/`)
- **Task 6: IP Container - RemoteDiag IPC Integration Engine** (`remotediag/ipc_integration/`)
- **Task 7: IP Container - Adapter Mirror Implementations** (`remotediag/services/`)
- **Task 8: IP Container - SID Filter Implementation** (`remotediag/sid_filter/`)

---

### Task 1: Shared IPC Transport Layer

**Files:**
- Create: `ipc/transport/ISocketCallback.h`
- Create: `ipc/transport/UnixSocketServer.h`
- Create: `ipc/transport/UnixSocketServer.cpp`
- Create: `ipc/transport/UnixSocketClient.h`
- Create: `ipc/transport/UnixSocketClient.cpp`

- [ ] **Step 1: Create `ISocketCallback.h` interface**

```cpp
// ipc/transport/ISocketCallback.h
#pragma once
#include <cstdint>
#include <cstddef>

class ISocketCallback {
public:
    virtual ~ISocketCallback() = default;
    virtual void onDataReceived(const uint8_t* data, size_t len) = 0;
    virtual void onConnected() = 0;
    virtual void onDisconnected() = 0;
};
```

- [ ] **Step 2: Create `UnixSocketServer.h` and `UnixSocketServer.cpp`**

`UnixSocketServer.h`:
```cpp
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
```

`UnixSocketServer.cpp`:
```cpp
// ipc/transport/UnixSocketServer.cpp
#include "UnixSocketServer.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <vector>

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
    strncpy(addr.sun_path, mSocketPath.c_str(), sizeof(addr.sun_path) - 1);

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
```

- [ ] **Step 3: Create `UnixSocketClient.h` and `UnixSocketClient.cpp`**

`UnixSocketClient.h`:
```cpp
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
```

`UnixSocketClient.cpp`:
```cpp
// ipc/transport/UnixSocketClient.cpp
#include "UnixSocketClient.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

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
        strncpy(addr.sun_path, mSocketPath.c_str(), sizeof(addr.sun_path) - 1);

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
```

---

### Task 2: Shared IPC Framing Protocol & Codec

**Files:**
- Create: `ipc/protocol/IpcFrame.h`
- Create: `ipc/protocol/IpcConstants.h`
- Create: `ipc/protocol/IpcFrameCodec.h`
- Create: `ipc/protocol/IpcFrameCodec.cpp`

- [ ] **Step 1: Create `IpcFrame.h`**

```cpp
// ipc/protocol/IpcFrame.h
#pragma once
#include <cstdint>
#include <vector>

struct IpcFrame {
    uint8_t  frameType{0};        // CALLBACK=0x01, REQUEST=0x02, RESPONSE=0x03
    uint16_t messageId{0};        // Callback ID or Command ID
    uint32_t correlationId{0};    // Request/Response matching (0 for callback)
    uint32_t payloadLen{0};       // Payload size
    std::vector<uint8_t> payload;

    static constexpr uint8_t TYPE_CALLBACK  = 0x01;
    static constexpr uint8_t TYPE_REQUEST   = 0x02;
    static constexpr uint8_t TYPE_RESPONSE  = 0x03;
};
```

- [ ] **Step 2: Create `IpcConstants.h`**

```cpp
// ipc/protocol/IpcConstants.h
#pragma once
#include <cstdint>

// Callback IDs (EP → IP, one-way)
enum DiagCallbackId : uint16_t {
    CB_DIAG_STATUS_CHANGED          = 0x0100,
    CB_DIAG_UNDER_REPAIR_CHANGED    = 0x0101,
    CB_DIAG_SERVICE_FLAG_CHANGED    = 0x0102,
    CB_DIAG_DID_READ_RESPONSE       = 0x0103,
};

enum CalibCallbackId : uint16_t {
    CB_CALIB_STATUS_CHANGED         = 0x0200,
};

enum PowerCallbackId : uint16_t {
    CB_POWER_IG_CHANGED             = 0x0300,
    CB_POWER_BATTERY_STATUS         = 0x0301,
};

enum PPICallbackId : uint16_t {
    CB_PPI_RECEIVED                 = 0x0400,
};

enum LocationCallbackId : uint16_t {
    CB_LOCATION_UPDATE              = 0x0500,
};

enum RegionCallbackId : uint16_t {
    CB_REGION_CHANGED               = 0x0600,
};

enum MqttCallbackId : uint16_t {
    CB_MQTT_NOTIFICATION            = 0x0700,
};

enum HttpCallbackId : uint16_t {
    CB_HTTP_GRPC_RESPONSE           = 0x0800,
};

enum OnboardClientCallbackId : uint16_t {
    CB_OBC_RESPONSE_EVENT           = 0x0900,
    CB_OBC_OBD2_EVENT               = 0x0901,
    CB_OBC_RESOURCE_EVENT           = 0x0902,
};

enum VehicleCallbackId : uint16_t {
    CB_VEHICLE_EVENT                = 0x0A00,
};

enum AppCallbackId : uint16_t {
    CB_APP_BOOT_COMPLETED           = 0x0B00,
    CB_APP_FEATURE_STATUS_CHANGED   = 0x0B01,
    CB_APP_POST_RECEIVED            = 0x0B02,
};

// Command IDs (IP → EP, synchronous request/response)
enum DiagCommandId : uint16_t {
    CMD_DIAG_GET_RDG_FLAG           = 0x8100,
    CMD_DIAG_GET_DTC_FLAG           = 0x8101,
    CMD_DIAG_GET_SSR_FLAG           = 0x8102,
    CMD_DIAG_GET_WAR_FLAG           = 0x8103,
    CMD_DIAG_GET_ROB_FLAG           = 0x8104,
    CMD_DIAG_GET_DDR_FLAG           = 0x8105,
    CMD_DIAG_GET_UNDER_REPAIR       = 0x8106,
    CMD_DIAG_GET_SRVC_AC            = 0x8107,
    CMD_DIAG_GET_SRVC_VC            = 0x8108,
    CMD_DIAG_GET_SRVC_PC            = 0x8109,
    CMD_DIAG_GET_SRVC_STT           = 0x810A,
    CMD_DIAG_READ_DID               = 0x810B,
    CMD_DIAG_WRITE_DID              = 0x810C,
};

enum CalibCommandId : uint16_t {
    CMD_CALIB_GET_VARIANT           = 0x8200,
};

enum PowerCommandId : uint16_t {
    CMD_POWER_GET_IG_STATUS         = 0x8300,
};

enum PPICommandId : uint16_t {
    CMD_PPI_WRITE_DID               = 0x8400,
};

enum LocationCommandId : uint16_t {
    CMD_LOCATION_GET_CURRENT        = 0x8500,
};

enum RegionCommandId : uint16_t {
    CMD_REGION_GET_CURRENT          = 0x8600,
};

enum MqttCommandId : uint16_t {
    CMD_MQTT_PUBLISH                = 0x8700,
};

enum HttpCommandId : uint16_t {
    CMD_HTTP_SEND_GRPC              = 0x8800,
};

enum OnboardClientCommandId : uint16_t {
    CMD_OBC_SEND_UDS                = 0x8900,
    CMD_OBC_CONNECT                 = 0x8901,
    CMD_OBC_DISCONNECT              = 0x8902,
    CMD_OBC_TAKE_RESOURCE           = 0x8903,
    CMD_OBC_RELEASE_RESOURCE        = 0x8904,
};

enum VehicleCommandId : uint16_t {
    CMD_VEHICLE_GET_STATUS          = 0x8A00,
};

enum AppCommandId : uint16_t {
    CMD_APP_REGISTER                = 0x8B00,
};
```

- [ ] **Step 3: Create `IpcFrameCodec.h` and `IpcFrameCodec.cpp`**

`IpcFrameCodec.h`:
```cpp
// ipc/protocol/IpcFrameCodec.h
#pragma once
#include "IpcFrame.h"
#include <vector>
#include <cstddef>

class IpcFrameCodec {
public:
    static std::vector<uint8_t> encode(const IpcFrame& frame);
    static size_t decode(const uint8_t* data, size_t len, IpcFrame& outFrame);
};
```

`IpcFrameCodec.cpp`:
```cpp
// ipc/protocol/IpcFrameCodec.cpp
#include "IpcFrameCodec.h"
#include <arpa/inet.h>
#include <cstring>

std::vector<uint8_t> IpcFrameCodec::encode(const IpcFrame& frame) {
    uint32_t payloadLen = static_cast<uint32_t>(frame.payload.size());
    uint32_t frameLen = 1 + 2 + 4 + 4 + payloadLen; // type(1) + msgId(2) + corrId(4) + payLen(4) + payload
    uint32_t totalLen = 4 + frameLen;               // header len prefix(4)

    std::vector<uint8_t> buf(totalLen);
    uint32_t nTotalLen = htonl(totalLen);
    uint16_t nMsgId = htons(frame.messageId);
    uint32_t nCorrId = htonl(frame.correlationId);
    uint32_t nPayLen = htonl(payloadLen);

    size_t offset = 0;
    std::memcpy(&buf[offset], &nTotalLen, 4); offset += 4;
    buf[offset] = frame.frameType;            offset += 1;
    std::memcpy(&buf[offset], &nMsgId, 2);    offset += 2;
    std::memcpy(&buf[offset], &nCorrId, 4);   offset += 4;
    std::memcpy(&buf[offset], &nPayLen, 4);   offset += 4;

    if (payloadLen > 0) {
        std::memcpy(&buf[offset], frame.payload.data(), payloadLen);
    }
    return buf;
}

size_t IpcFrameCodec::decode(const uint8_t* data, size_t len, IpcFrame& outFrame) {
    if (len < 15) return 0; // 4 (length prefix) + 11 (header)

    uint32_t nTotalLen = 0;
    std::memcpy(&nTotalLen, data, 4);
    uint32_t totalLen = ntohl(nTotalLen);

    if (len < totalLen) return 0; // Insufficient data

    outFrame.frameType = data[4];
    
    uint16_t nMsgId = 0;
    std::memcpy(&nMsgId, &data[5], 2);
    outFrame.messageId = ntohs(nMsgId);

    uint32_t nCorrId = 0;
    std::memcpy(&nCorrId, &data[7], 4);
    outFrame.correlationId = ntohl(nCorrId);

    uint32_t nPayLen = 0;
    std::memcpy(&nPayLen, &data[11], 4);
    outFrame.payloadLen = ntohl(nPayLen);

    outFrame.payload.clear();
    if (outFrame.payloadLen > 0) {
        outFrame.payload.assign(&data[15], &data[15 + outFrame.payloadLen]);
    }

    return totalLen;
}
```

---

### Task 3: High-Level IPC Server/Client Classes

**Files:**
- Create: `ipc/protocol/ProxyIpcServer.h`
- Create: `ipc/protocol/ProxyIpcServer.cpp`
- Create: `ipc/protocol/ProxyIpcClient.h`
- Create: `ipc/protocol/ProxyIpcClient.cpp`

- [ ] **Step 1: Create `ProxyIpcServer.h` and `ProxyIpcServer.cpp`**

`ProxyIpcServer.h`:
```cpp
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
```

`ProxyIpcServer.cpp`:
```cpp
// ipc/protocol/ProxyIpcServer.cpp
#include "ProxyIpcServer.h"
#include "IpcFrameCodec.h"
#include <iostream>

ProxyIpcServer* ProxyIpcServer::getInstance() {
    static ProxyIpcServer instance;
    return &instance;
}

bool ProxyIpcServer::start(const std::string& socketPath) {
    return mServer.start(socketPath, this);
}

void ProxyIpcServer::stop() {
    mServer.stop();
}

void ProxyIpcServer::setCallbackDispatcher(CallbackDispatcherFunc dispatcher) {
    mDispatcher = dispatcher;
}

ProxyIpcServer::ApiResponse ProxyIpcServer::requestAPICall(uint16_t commandId, const std::vector<uint8_t>& payload, uint32_t timeoutMs) {
    uint32_t corrId = mNextCorrelationId++;

    IpcFrame reqFrame;
    reqFrame.frameType = IpcFrame::TYPE_REQUEST;
    reqFrame.messageId = commandId;
    reqFrame.correlationId = corrId;
    reqFrame.payload = payload;
    reqFrame.payloadLen = static_cast<uint32_t>(payload.size());

    auto wireData = IpcFrameCodec::encode(reqFrame);

    std::unique_lock<std::mutex> lock(mResponseMutex);
    if (!mServer.sendData(wireData.data(), wireData.size())) {
        return {false, {}};
    }

    bool status = mResponseCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, corrId]() {
        return mPendingResponses.find(corrId) != mPendingResponses.end();
    });

    if (!status) {
        return {false, {}};
    }

    ApiResponse resp = mPendingResponses[corrId];
    mPendingResponses.erase(corrId);
    return resp;
}

void ProxyIpcServer::onDataReceived(const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(mRecvMutex);
    mRecvBuf.insert(mRecvBuf.end(), data, data + len);

    while (true) {
        IpcFrame frame;
        size_t consumed = IpcFrameCodec::decode(mRecvBuf.data(), mRecvBuf.size(), frame);
        if (consumed == 0) break;

        mRecvBuf.erase(mRecvBuf.begin(), mRecvBuf.begin() + consumed);

        if (frame.frameType == IpcFrame::TYPE_CALLBACK) {
            if (mDispatcher) {
                mDispatcher(frame.messageId, frame.payload);
            }
        } else if (frame.frameType == IpcFrame::TYPE_RESPONSE) {
            std::lock_guard<std::mutex> rLock(mResponseMutex);
            mPendingResponses[frame.correlationId] = {true, frame.payload};
            mResponseCv.notify_all();
        }
    }
}

void ProxyIpcServer::onConnected() {}
void ProxyIpcServer::onDisconnected() {}
```

- [ ] **Step 2: Create `ProxyIpcClient.h` and `ProxyIpcClient.cpp`**

`ProxyIpcClient.h`:
```cpp
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
```

`ProxyIpcClient.cpp`:
```cpp
// ipc/protocol/ProxyIpcClient.cpp
#include "ProxyIpcClient.h"
#include "IpcFrameCodec.h"

ProxyIpcClient* ProxyIpcClient::getInstance() {
    static ProxyIpcClient instance;
    return &instance;
}

bool ProxyIpcClient::start(const std::string& socketPath) {
    return mClient.start(socketPath, this);
}

void ProxyIpcClient::stop() {
    mClient.stop();
}

void ProxyIpcClient::setCommandHandler(CommandHandlerFunc handler) {
    mHandler = handler;
}

void ProxyIpcClient::sendCallback(uint16_t callbackId, const std::vector<uint8_t>& payload) {
    IpcFrame cbFrame;
    cbFrame.frameType = IpcFrame::TYPE_CALLBACK;
    cbFrame.messageId = callbackId;
    cbFrame.correlationId = 0;
    cbFrame.payload = payload;
    cbFrame.payloadLen = static_cast<uint32_t>(payload.size());

    auto wireData = IpcFrameCodec::encode(cbFrame);
    mClient.sendData(wireData.data(), wireData.size());
}

void ProxyIpcClient::onDataReceived(const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(mRecvMutex);
    mRecvBuf.insert(mRecvBuf.end(), data, data + len);

    while (true) {
        IpcFrame frame;
        size_t consumed = IpcFrameCodec::decode(mRecvBuf.data(), mRecvBuf.size(), frame);
        if (consumed == 0) break;

        mRecvBuf.erase(mRecvBuf.begin(), mRecvBuf.begin() + consumed);

        if (frame.frameType == IpcFrame::TYPE_REQUEST) {
            bool success = false;
            std::vector<uint8_t> respPayload;
            if (mHandler) {
                respPayload = mHandler(frame.messageId, frame.payload, success);
            }

            IpcFrame respFrame;
            respFrame.frameType = IpcFrame::TYPE_RESPONSE;
            respFrame.messageId = frame.messageId;
            respFrame.correlationId = frame.correlationId;
            respFrame.payload = respPayload;
            respFrame.payloadLen = static_cast<uint32_t>(respPayload.size());

            auto wireData = IpcFrameCodec::encode(respFrame);
            mClient.sendData(wireData.data(), wireData.size());
        }
    }
}

void ProxyIpcClient::onConnected() {}
void ProxyIpcClient::onDisconnected() {}
```

---

### Task 4: EP Container - RemoteDiagProxy Core Daemon & Forwarding Engine

**Files:**
- Create: `remotediag_proxy/RemoteDiagProxy.h`
- Create: `remotediag_proxy/RemoteDiagProxy.cpp`
- Create: `remotediag_proxy/forwarding/CallbackForwarder.h`
- Create: `remotediag_proxy/forwarding/CallbackForwarder.cpp`
- Create: `remotediag_proxy/forwarding/CommandHandlerRegistry.h`
- Create: `remotediag_proxy/forwarding/CommandHandlerRegistry.cpp`

- [ ] **Step 1: Create `CallbackForwarder.h` and `CallbackForwarder.cpp`**

`CallbackForwarder.h`:
```cpp
// remotediag_proxy/forwarding/CallbackForwarder.h
#pragma once
#include <cstdint>
#include <vector>

class CallbackForwarder {
public:
    static CallbackForwarder* getInstance();
    void forward(uint16_t callbackId, const std::vector<uint8_t>& payload);
};
```

`CallbackForwarder.cpp`:
```cpp
// remotediag_proxy/forwarding/CallbackForwarder.cpp
#include "CallbackForwarder.h"
#include "../../ipc/protocol/ProxyIpcClient.h"

CallbackForwarder* CallbackForwarder::getInstance() {
    static CallbackForwarder instance;
    return &instance;
}

void CallbackForwarder::forward(uint16_t callbackId, const std::vector<uint8_t>& payload) {
    ProxyIpcClient::getInstance()->sendCallback(callbackId, payload);
}
```

- [ ] **Step 2: Create `CommandHandlerRegistry.h` and `CommandHandlerRegistry.cpp`**

`CommandHandlerRegistry.h`:
```cpp
// remotediag_proxy/forwarding/CommandHandlerRegistry.h
#pragma once
#include <cstdint>
#include <vector>
#include <functional>
#include <map>

class CommandHandlerRegistry {
public:
    using CommandFunc = std::function<std::vector<uint8_t>(const std::vector<uint8_t>& payload, bool& success)>;

    static CommandHandlerRegistry* getInstance();
    void registerHandler(uint16_t commandId, CommandFunc handler);
    std::vector<uint8_t> handleCommand(uint16_t commandId, const std::vector<uint8_t>& payload, bool& success);

private:
    CommandHandlerRegistry() = default;
    std::map<uint16_t, CommandFunc> mHandlers;
};
```

`CommandHandlerRegistry.cpp`:
```cpp
// remotediag_proxy/forwarding/CommandHandlerRegistry.cpp
#include "CommandHandlerRegistry.h"

CommandHandlerRegistry* CommandHandlerRegistry::getInstance() {
    static CommandHandlerRegistry instance;
    return &instance;
}

void CommandHandlerRegistry::registerHandler(uint16_t commandId, CommandFunc handler) {
    mHandlers[commandId] = handler;
}

std::vector<uint8_t> CommandHandlerRegistry::handleCommand(uint16_t commandId, const std::vector<uint8_t>& payload, bool& success) {
    auto it = mHandlers.find(commandId);
    if (it != mHandlers.end()) {
        return it->second(payload, success);
    }
    success = false;
    return {};
}
```

- [ ] **Step 3: Create `RemoteDiagProxy.h` and `RemoteDiagProxy.cpp`**

`RemoteDiagProxy.h`:
```cpp
// remotediag_proxy/RemoteDiagProxy.h
#pragma once
#include <string>

class RemoteDiagProxy {
public:
    static RemoteDiagProxy* getInstance();
    void onCreate(const std::string& socketPath);
    void onDestroy();
};
```

`RemoteDiagProxy.cpp`:
```cpp
// remotediag_proxy/RemoteDiagProxy.cpp
#include "RemoteDiagProxy.h"
#include "../ipc/protocol/ProxyIpcClient.h"
#include "forwarding/CommandHandlerRegistry.h"
#include <iostream>

RemoteDiagProxy* RemoteDiagProxy::getInstance() {
    static RemoteDiagProxy instance;
    return &instance;
}

void RemoteDiagProxy::onCreate(const std::string& socketPath) {
    ProxyIpcClient::getInstance()->setCommandHandler(
        [](uint16_t cmdId, const std::vector<uint8_t>& payload, bool& success) {
            return CommandHandlerRegistry::getInstance()->handleCommand(cmdId, payload, success);
        });
    ProxyIpcClient::getInstance()->start(socketPath);
    std::cout << "[RemoteDiagProxy] Service started and connected to IPC path: " << socketPath << std::endl;
}

void RemoteDiagProxy::onDestroy() {
    ProxyIpcClient::getInstance()->stop();
}
```

---

### Task 5: EP Container - Command Handlers & Real Binder Adapters

**Files:**
- Create: `remotediag_proxy/forwarding/DiagCommandHandler.h`
- Create: `remotediag_proxy/forwarding/DiagCommandHandler.cpp`

- [ ] **Step 1: Create `DiagCommandHandler.h` and `DiagCommandHandler.cpp`**

`DiagCommandHandler.h`:
```cpp
// remotediag_proxy/forwarding/DiagCommandHandler.h
#pragma once

class DiagCommandHandler {
public:
    static void registerHandlers();
};
```

`DiagCommandHandler.cpp`:
```cpp
// remotediag_proxy/forwarding/DiagCommandHandler.cpp
#include "DiagCommandHandler.h"
#include "CommandHandlerRegistry.h"
#include "../../ipc/protocol/IpcConstants.h"

void DiagCommandHandler::registerHandlers() {
    auto registry = CommandHandlerRegistry::getInstance();

    registry->registerHandler(CMD_DIAG_GET_RDG_FLAG, [](const std::vector<uint8_t>& payload, bool& success) {
        success = true;
        return std::vector<uint8_t>{0x01}; // Stub response for RDG Flag
    });

    registry->registerHandler(CMD_DIAG_WRITE_DID, [](const std::vector<uint8_t>& payload, bool& success) {
        success = true;
        return std::vector<uint8_t>{0x00}; // OK
    });
}
```

---

### Task 6: IP Container - RemoteDiag IPC Integration Engine

**Files:**
- Create: `remotediag/ipc_integration/RemoteDiagIpcBridge.h`
- Create: `remotediag/ipc_integration/RemoteDiagIpcBridge.cpp`
- Create: `remotediag/ipc_integration/CallbackHandlerRegistry.h`
- Create: `remotediag/ipc_integration/CallbackHandlerRegistry.cpp`

- [ ] **Step 1: Create `CallbackHandlerRegistry.h` and `CallbackHandlerRegistry.cpp`**

`CallbackHandlerRegistry.h`:
```cpp
// remotediag/ipc_integration/CallbackHandlerRegistry.h
#pragma once
#include <cstdint>
#include <vector>
#include <functional>
#include <map>

class CallbackHandlerRegistry {
public:
    using CallbackFunc = std::function<void(const std::vector<uint8_t>& payload)>;

    static CallbackHandlerRegistry* getInstance();
    void registerHandler(uint16_t callbackId, CallbackFunc handler);
    void dispatchCallback(uint16_t callbackId, const std::vector<uint8_t>& payload);

private:
    CallbackHandlerRegistry() = default;
    std::map<uint16_t, CallbackFunc> mHandlers;
};
```

`CallbackHandlerRegistry.cpp`:
```cpp
// remotediag/ipc_integration/CallbackHandlerRegistry.cpp
#include "CallbackHandlerRegistry.h"

CallbackHandlerRegistry* CallbackHandlerRegistry::getInstance() {
    static CallbackHandlerRegistry instance;
    return &instance;
}

void CallbackHandlerRegistry::registerHandler(uint16_t callbackId, CallbackFunc handler) {
    mHandlers[callbackId] = handler;
}

void CallbackHandlerRegistry::dispatchCallback(uint16_t callbackId, const std::vector<uint8_t>& payload) {
    auto it = mHandlers.find(callbackId);
    if (it != mHandlers.end()) {
        it->second(payload);
    }
}
```

- [ ] **Step 2: Create `RemoteDiagIpcBridge.h` and `RemoteDiagIpcBridge.cpp`**

`RemoteDiagIpcBridge.h`:
```cpp
// remotediag/ipc_integration/RemoteDiagIpcBridge.h
#pragma once
#include <string>

class RemoteDiagIpcBridge {
public:
    static RemoteDiagIpcBridge* getInstance();
    bool initialize(const std::string& socketPath);
    void shutdown();
};
```

`RemoteDiagIpcBridge.cpp`:
```cpp
// remotediag/ipc_integration/RemoteDiagIpcBridge.cpp
#include "RemoteDiagIpcBridge.h"
#include "../../ipc/protocol/ProxyIpcServer.h"
#include "CallbackHandlerRegistry.h"

RemoteDiagIpcBridge* RemoteDiagIpcBridge::getInstance() {
    static RemoteDiagIpcBridge instance;
    return &instance;
}

bool RemoteDiagIpcBridge::initialize(const std::string& socketPath) {
    ProxyIpcServer::getInstance()->setCallbackDispatcher(
        [](uint16_t callbackId, const std::vector<uint8_t>& payload) {
            CallbackHandlerRegistry::getInstance()->dispatchCallback(callbackId, payload);
        });
    return ProxyIpcServer::getInstance()->start(socketPath);
}

void RemoteDiagIpcBridge::shutdown() {
    ProxyIpcServer::getInstance()->stop();
}
```

---

### Task 7: IP Container - Adapter Mirror Implementations

**Files:**
- Create: `remotediag/services/DiagManagerAdapterMirror.h`
- Create: `remotediag/services/DiagManagerAdapterMirror.cpp`

- [ ] **Step 1: Create `DiagManagerAdapterMirror.h` and `DiagManagerAdapterMirror.cpp`**

`DiagManagerAdapterMirror.h`:
```cpp
// remotediag/services/DiagManagerAdapterMirror.h
#pragma once
#include <cstdint>
#include <vector>

class DiagManagerAdapterMirror {
public:
    static DiagManagerAdapterMirror* getInstance();
    void registerService();

    uint8_t getRDGFlag();
    void writeDidData(uint16_t param, const std::vector<uint8_t>& data);
};
```

`DiagManagerAdapterMirror.cpp`:
```cpp
// remotediag/services/DiagManagerAdapterMirror.cpp
#include "DiagManagerAdapterMirror.h"
#include "../../ipc/protocol/ProxyIpcServer.h"
#include "../../ipc/protocol/IpcConstants.h"
#include "../ipc_integration/CallbackHandlerRegistry.h"
#include <iostream>

DiagManagerAdapterMirror* DiagManagerAdapterMirror::getInstance() {
    static DiagManagerAdapterMirror instance;
    return &instance;
}

void DiagManagerAdapterMirror::registerService() {
    CallbackHandlerRegistry::getInstance()->registerHandler(CB_DIAG_STATUS_CHANGED,
        [](const std::vector<uint8_t>& payload) {
            std::cout << "[IP DiagManagerMirror] Received CB_DIAG_STATUS_CHANGED" << std::endl;
        });
}

uint8_t DiagManagerAdapterMirror::getRDGFlag() {
    auto resp = ProxyIpcServer::getInstance()->requestAPICall(CMD_DIAG_GET_RDG_FLAG, {}, 3000);
    if (resp.success && !resp.payload.empty()) {
        return resp.payload[0];
    }
    return 0;
}

void DiagManagerAdapterMirror::writeDidData(uint16_t param, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>(param >> 8));
    payload.push_back(static_cast<uint8_t>(param & 0xFF));
    payload.insert(payload.end(), data.begin(), data.end());

    ProxyIpcServer::getInstance()->requestAPICall(CMD_DIAG_WRITE_DID, payload, 3000);
}
```

---

### Task 8: IP Container - SID Filter Implementation

**Files:**
- Create: `remotediag/sid_filter/SIDFilter.h`
- Create: `remotediag/sid_filter/SIDFilter.cpp`

- [ ] **Step 1: Create `SIDFilter.h` and `SIDFilter.cpp`**

`SIDFilter.h`:
```cpp
// remotediag/sid_filter/SIDFilter.h
#pragma once
#include <cstdint>
#include <vector>
#include <set>

class SIDFilter {
public:
    static SIDFilter* getInstance();

    void setAllowedSIDs(const std::vector<uint8_t>& sids);
    void setBlockedSIDs(const std::vector<uint8_t>& sids);

    bool isAllowed(uint8_t serviceId) const;
    bool filterAndSendUdsData(uint16_t connectId, const std::vector<uint8_t>& udsData);

private:
    SIDFilter();
    std::set<uint8_t> mAllowedSIDs;
    std::set<uint8_t> mBlockedSIDs;
    bool mDefaultAllow{false};
};
```

`SIDFilter.cpp`:
```cpp
// remotediag/sid_filter/SIDFilter.cpp
#include "SIDFilter.h"
#include <iostream>

SIDFilter::SIDFilter() {
    // Whitelist default UDS Service IDs
    mAllowedSIDs = {0x10, 0x11, 0x19, 0x22, 0x2E, 0x31, 0x3E};
}

SIDFilter* SIDFilter::getInstance() {
    static SIDFilter instance;
    return &instance;
}

void SIDFilter::setAllowedSIDs(const std::vector<uint8_t>& sids) {
    mAllowedSIDs.assign(sids.begin(), sids.end());
}

void SIDFilter::setBlockedSIDs(const std::vector<uint8_t>& sids) {
    mBlockedSIDs.assign(sids.begin(), sids.end());
}

bool SIDFilter::isAllowed(uint8_t serviceId) const {
    if (mBlockedSIDs.count(serviceId) > 0) {
        return false;
    }
    if (mAllowedSIDs.count(serviceId) > 0) {
        return true;
    }
    return mDefaultAllow;
}

bool SIDFilter::filterAndSendUdsData(uint16_t connectId, const std::vector<uint8_t>& udsData) {
    if (udsData.empty()) return false;

    uint8_t sid = udsData[0];
    if (!isAllowed(sid)) {
        std::cerr << "[SIDFilter] UDS SID 0x" << std::hex << static_cast<int>(sid) << " BLOCKED!" << std::dec << std::endl;
        return false;
    }

    std::cout << "[SIDFilter] UDS SID 0x" << std::hex << static_cast<int>(sid) << " ALLOWED" << std::dec << std::endl;
    return true;
}
```

---
