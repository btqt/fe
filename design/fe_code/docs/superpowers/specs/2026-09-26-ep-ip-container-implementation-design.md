# Design Spec: EP/IP Container Implementation

**Date**: 2026-09-26
**Status**: Draft
**Scope**: Implement kiến trúc 2-container (EP + IP) cho RemoteDiag system

---

## 1. Overview

Tách hệ thống RemoteDiag từ 1 process thành 2 LXC container:
- **EP Container**: `RemoteDiagProxy` — forward Binder IPC data qua Unix Domain Socket
- **IP Container**: `RemoteDiag` — giữ nguyên `Remotediag` class, nhận data từ EP qua IPC

**Pattern**: Adapter Mirror — mỗi adapter trong IP giữ cùng public interface nhưng gọi IPC thay vì Binder. `Remotediag` class không cần sửa.

---

## 2. Directory Structure

```
fe_code/
├── ipc/                              # Shared IPC library (EP + IP cùng dùng)
│   ├── transport/
│   │   ├── ISocketCallback.h           # Interface: onDataReceived, onConnected, onDisconnected
│   │   ├── UnixSocketServer.h
│   │   ├── UnixSocketServer.cpp
│   │   ├── UnixSocketClient.h
│   │   └── UnixSocketClient.cpp
│   └── protocol/
│       ├── IpcFrame.h                  # Frame struct: type, messageId, correlationId, payload
│       ├── IpcFrameCodec.h             # Serialize/Deserialize IpcFrame ↔ byte stream
│       ├── IpcFrameCodec.cpp
│       ├── IpcConstants.h              # Frame types, callback IDs, command IDs (enum ranges)
│       ├── ProxyIpcServer.h            # IP side: requestAPICall(), receive callbacks
│       ├── ProxyIpcServer.cpp
│       ├── ProxyIpcClient.h            # EP side: sendCallback(), receive requests
│       └── ProxyIpcClient.cpp
│
├── remotediag/                        # IP Container (Remotediag class giữ nguyên)
│   ├── Remotediag.h                     # KHÔNG THAY ĐỔI
│   ├── Remotediag.cpp                   # KHÔNG THAY ĐỔI
│   ├── services/                        # Mirror adapters (cùng interface, IPC implementation)
│   │   ├── DiagManagerAdapter.h/.cpp      # Mirror: requestAPICall() thay vì Binder
│   │   ├── CalibManagerAdapter.h/.cpp
│   │   ├── PowerManagerAdapter.h/.cpp
│   │   ├── PPIManagerAdapter.h/.cpp
│   │   ├── LocationManagerAdapter.h/.cpp
│   │   ├── RegionManagerAdapter.h/.cpp
│   │   ├── MqttManagerAdapter.h/.cpp
│   │   ├── HttpManagerAdapter.h/.cpp
│   │   ├── OnboardclientManagerAdapter.h/.cpp
│   │   ├── VehicleManagerAdapter.h/.cpp
│   │   └── ApplicationManagerAdapter.h/.cpp
│   ├── ipc_integration/                 # IPC tích hợp
│   │   ├── RemoteDiagIpcBridge.h/.cpp     # Khởi tạo IPC server, connect mirrors
│   │   ├── ServerCallbackDispatcher.h/.cpp  # Route callbacks → mirror adapters
│   │   └── CallbackHandlerRegistry.h/.cpp   # Registry: callbackId → handler function
│   ├── sid_filter/                      # SID Filter
│   │   ├── SIDFilter.h
│   │   └── SIDFilter.cpp
│   ├── diagprocess/                     # KHÔNG THAY ĐỔI
│   └── utils/                           # KHÔNG THAY ĐỔI
│
├── remotediag_proxy/                  # EP Container (mới)
│   ├── RemoteDiagProxy.h                # Main proxy application
│   ├── RemoteDiagProxy.cpp
│   ├── RDPHandler.h                     # Message handler
│   ├── RDPHandler.cpp
│   ├── services/                        # Real adapters (Binder IPC - copy & adapt)
│   │   ├── DiagManagerAdapter.h/.cpp      # Real Binder adapter
│   │   ├── CalibManagerAdapter.h/.cpp
│   │   ├── PowerManagerAdapter.h/.cpp
│   │   ├── PPIManagerAdapter.h/.cpp
│   │   ├── LocationManagerAdapter.h/.cpp
│   │   ├── RegionManagerAdapter.h/.cpp
│   │   ├── MqttManagerAdapter.h/.cpp
│   │   ├── HttpManagerAdapter.h/.cpp
│   │   ├── OnboardclientManagerAdapter.h/.cpp
│   │   ├── VehicleManagerAdapter.h/.cpp
│   │   └── ApplicationManagerAdapter.h/.cpp
│   └── forwarding/                      # Forward logic
│       ├── CallbackForwarder.h/.cpp       # Adapter callback → IPC
│       ├── CommandHandler.h/.cpp          # IPC request → adapter API call
│       └── CommandHandlerRegistry.h/.cpp  # Registry: commandId → handler
```

---

## 3. Transport Layer

### 3.1. ISocketCallback

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

### 3.2. UnixSocketServer (IP side)

**Responsibilities:**
- Create + bind + listen on Unix Domain Socket path
- Accept single client connection
- Background recv thread: read data → `ISocketCallback::onDataReceived()`
- `send(data, len)`: send data to connected client
- `waitUntilConnected(timeout)`: block until client connected

**Key design decisions:**
- **Single client**: Chỉ chấp nhận 1 RemoteDiagProxy tại 1 thời điểm
- **Thread model**: 1 thread cho accept + recv loop
- **Socket path**: Cấu hình qua constructor parameter (ví dụ `/tmp/remotediag_ipc.sock`)
- **Graceful shutdown**: `stop()` close socket FDs, join thread

### 3.3. UnixSocketClient (EP side)

**Responsibilities:**
- Connect to Unix Domain Socket path
- Background recv thread: read data → `ISocketCallback::onDataReceived()`
- `send(data, len)`: send data to server
- `waitUntilConnected(timeout)`: block until connected
- **Auto-reconnect**: Khi disconnect, retry với exponential backoff (100ms → 200ms → 400ms → ... → max 5s)

---

## 4. Protocol Layer

### 4.1. IpcFrame

```cpp
// ipc/protocol/IpcFrame.h
#pragma once
#include <cstdint>
#include <vector>

struct IpcFrame {
    uint8_t  frameType;           // CALLBACK=0x01, REQUEST=0x02, RESPONSE=0x03
    uint16_t messageId;           // Callback ID hoặc Command ID
    uint32_t correlationId;       // Request/Response matching (0 cho callback)
    uint32_t payloadLen;          // Payload size in bytes
    std::vector<uint8_t> payload; // Serialized data

    static constexpr uint8_t TYPE_CALLBACK  = 0x01;
    static constexpr uint8_t TYPE_REQUEST   = 0x02;
    static constexpr uint8_t TYPE_RESPONSE  = 0x03;
};
```

### 4.2. Wire Format

```
[4 bytes: total_frame_len (network byte order)]
[1 byte:  frameType]
[2 bytes: messageId (network byte order)]
[4 bytes: correlationId (network byte order)]
[4 bytes: payloadLen (network byte order)]
[N bytes: payload]
```

Total header: 4 (length prefix) + 11 (frame header) = 15 bytes overhead.

### 4.3. IpcFrameCodec

```cpp
// ipc/protocol/IpcFrameCodec.h
class IpcFrameCodec {
public:
    // Encode IpcFrame → byte buffer (including length prefix)
    static std::vector<uint8_t> encode(const IpcFrame& frame);

    // Decode from byte stream. Returns number of bytes consumed, 0 if incomplete.
    // Handles partial reads (buffering internally).
    static size_t decode(const uint8_t* data, size_t len, IpcFrame& outFrame);
};
```

### 4.4. Callback & Command ID Constants

```cpp
// ipc/protocol/IpcConstants.h

// --- Callback IDs (EP → IP, one-way) ---
// DiagManagerAdapter callbacks: 0x0100–0x01FF
enum DiagCallbackId : uint16_t {
    CB_DIAG_STATUS_CHANGED          = 0x0100,
    CB_DIAG_UNDER_REPAIR_CHANGED    = 0x0101,
    CB_DIAG_SERVICE_FLAG_CHANGED    = 0x0102,
    CB_DIAG_DID_READ_RESPONSE       = 0x0103,
};

// CalibManagerAdapter callbacks: 0x0200–0x02FF
enum CalibCallbackId : uint16_t {
    CB_CALIB_STATUS_CHANGED         = 0x0200,
};

// PowerManagerAdapter callbacks: 0x0300–0x03FF
enum PowerCallbackId : uint16_t {
    CB_POWER_IG_CHANGED             = 0x0300,
    CB_POWER_BATTERY_STATUS         = 0x0301,
};

// PPIManagerAdapter callbacks: 0x0400–0x04FF
enum PPICallbackId : uint16_t {
    CB_PPI_RECEIVED                 = 0x0400,
};

// LocationManagerAdapter callbacks: 0x0500–0x05FF
enum LocationCallbackId : uint16_t {
    CB_LOCATION_UPDATE              = 0x0500,
};

// RegionManagerAdapter callbacks: 0x0600–0x06FF
enum RegionCallbackId : uint16_t {
    CB_REGION_CHANGED               = 0x0600,
};

// MqttManagerAdapter callbacks: 0x0700–0x07FF
enum MqttCallbackId : uint16_t {
    CB_MQTT_NOTIFICATION            = 0x0700,
};

// HttpManagerAdapter callbacks: 0x0800–0x08FF
enum HttpCallbackId : uint16_t {
    CB_HTTP_GRPC_RESPONSE           = 0x0800,
};

// OnboardclientAdapter callbacks: 0x0900–0x09FF
enum OnboardClientCallbackId : uint16_t {
    CB_OBC_RESPONSE_EVENT           = 0x0900,
    CB_OBC_OBD2_EVENT               = 0x0901,
    CB_OBC_RESOURCE_EVENT           = 0x0902,
};

// VehicleManagerAdapter callbacks: 0x0A00–0x0AFF
enum VehicleCallbackId : uint16_t {
    CB_VEHICLE_EVENT                = 0x0A00,
};

// ApplicationManagerAdapter callbacks: 0x0B00–0x0BFF
enum AppCallbackId : uint16_t {
    CB_APP_BOOT_COMPLETED           = 0x0B00,
    CB_APP_FEATURE_STATUS_CHANGED   = 0x0B01,
    CB_APP_POST_RECEIVED            = 0x0B02,
};


// --- Command IDs (IP → EP, request/response) ---
// DiagManagerAdapter commands: 0x8100–0x81FF
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

// CalibManagerAdapter commands: 0x8200–0x82FF
enum CalibCommandId : uint16_t {
    CMD_CALIB_GET_VARIANT           = 0x8200,
};

// PowerManagerAdapter commands: 0x8300–0x83FF
enum PowerCommandId : uint16_t {
    CMD_POWER_GET_IG_STATUS         = 0x8300,
};

// PPIManagerAdapter commands: 0x8400–0x84FF
enum PPICommandId : uint16_t {
    CMD_PPI_WRITE_DID               = 0x8400,
};

// LocationManagerAdapter commands: 0x8500–0x85FF
enum LocationCommandId : uint16_t {
    CMD_LOCATION_GET_CURRENT        = 0x8500,
};

// RegionManagerAdapter commands: 0x8600–0x86FF
enum RegionCommandId : uint16_t {
    CMD_REGION_GET_CURRENT          = 0x8600,
};

// MqttManagerAdapter commands: 0x8700–0x87FF
enum MqttCommandId : uint16_t {
    CMD_MQTT_PUBLISH                = 0x8700,
};

// HttpManagerAdapter commands: 0x8800–0x88FF
enum HttpCommandId : uint16_t {
    CMD_HTTP_SEND_GRPC              = 0x8800,
};

// OnboardclientAdapter commands: 0x8900–0x89FF
enum OnboardClientCommandId : uint16_t {
    CMD_OBC_SEND_UDS                = 0x8900,
    CMD_OBC_CONNECT                 = 0x8901,
    CMD_OBC_DISCONNECT              = 0x8902,
    CMD_OBC_TAKE_RESOURCE           = 0x8903,
    CMD_OBC_RELEASE_RESOURCE        = 0x8904,
};

// VehicleManagerAdapter commands: 0x8A00–0x8AFF
enum VehicleCommandId : uint16_t {
    CMD_VEHICLE_GET_STATUS          = 0x8A00,
};

// ApplicationManagerAdapter commands: 0x8B00–0x8BFF
enum AppCommandId : uint16_t {
    CMD_APP_REGISTER                = 0x8B00,
};
```

### 4.5. ProxyIpcServer (IP side)

```cpp
// ipc/protocol/ProxyIpcServer.h
class ProxyIpcServer : public ISocketCallback {
public:
    static ProxyIpcServer* getInstance();

    void start(const std::string& socketPath);
    void stop();
    bool waitUntilReady(uint32_t timeoutMs);

    // Gọi từ mirror adapters — synchronous request/response
    struct ApiResponse {
        bool success;
        std::vector<uint8_t> payload;
    };
    ApiResponse requestAPICall(uint16_t commandId,
                               const std::vector<uint8_t>& payload,
                               uint32_t timeoutMs);

    // Register callback handler
    void setCallbackDispatcher(ServerCallbackDispatcher* dispatcher);

    // ISocketCallback
    void onDataReceived(const uint8_t* data, size_t len) override;
    void onConnected() override;
    void onDisconnected() override;

private:
    UnixSocketServer mSocketServer;
    ServerCallbackDispatcher* mDispatcher;

    // Request/Response correlation
    std::mutex mResponseMutex;
    std::condition_variable mResponseCv;
    std::map<uint32_t, ApiResponse> mPendingResponses;  // correlationId → response
    std::atomic<uint32_t> mNextCorrelationId{1};

    // Frame buffer for partial reads
    std::vector<uint8_t> mRecvBuffer;
};
```

### 4.6. ProxyIpcClient (EP side)

```cpp
// ipc/protocol/ProxyIpcClient.h
class ProxyIpcClient : public ISocketCallback {
public:
    static ProxyIpcClient* getInstance();

    void start(const std::string& socketPath);
    void stop();
    bool waitUntilReady(uint32_t timeoutMs);

    // Gọi từ CallbackForwarder — one-way
    void sendCallback(uint16_t callbackId, const std::vector<uint8_t>& payload);

    // Register command handler
    void setCommandDispatcher(CommandHandlerRegistry* registry);

    // ISocketCallback
    void onDataReceived(const uint8_t* data, size_t len) override;
    void onConnected() override;
    void onDisconnected() override;

private:
    UnixSocketClient mSocketClient;
    CommandHandlerRegistry* mCommandRegistry;

    // Callback queue (async send)
    std::mutex mCallbackMutex;
    std::queue<IpcFrame> mPendingCallbacks;
    void flushCallbacks();

    // Frame buffer
    std::vector<uint8_t> mRecvBuffer;
};
```

---

## 5. Adapter Mirror Pattern (IP Side)

### 5.1. Nguyên tắc

Mỗi adapter trong `remotediag/services/` giữ **cùng public interface** (header) nhưng thay đổi implementation:
- **Sync API methods** (ví dụ `getRDGFlag()`, `writeDidData()`): Gọi `ProxyIpcServer::requestAPICall(CMD_XXX)` → serialize params → gửi qua IPC → nhận response → deserialize → return
- **Callback handling**: Register handler trong `CallbackHandlerRegistry` → khi nhận callback từ EP, deserialize payload → gọi vào `Remotediag` (giống như Binder callback handler cũ)

### 5.2. Ví dụ: DiagManagerAdapter Mirror

```cpp
// remotediag/services/DiagManagerAdapter.cpp (IP Mirror)

// Sync API — thay Binder bằng IPC
uint8_t DiagManagerAdapter::getRDGFlag() {
    auto resp = ProxyIpcServer::getInstance()->requestAPICall(
        CMD_DIAG_GET_RDG_FLAG, {}, DEFAULT_TIMEOUT_MS);
    if (resp.success && resp.payload.size() >= 1) {
        return resp.payload[0];
    }
    LOG_E("getRDGFlag IPC failed");
    return 0U;
}

void DiagManagerAdapter::writeDidData(uint16_t param, const android::sp<Buffer>& buf) {
    std::vector<uint8_t> payload;
    // Serialize: [2 bytes param][N bytes buffer data]
    payload.push_back(static_cast<uint8_t>(param >> 8));
    payload.push_back(static_cast<uint8_t>(param & 0xFF));
    if (buf != nullptr && buf->size() > 0) {
        payload.insert(payload.end(), buf->data(), buf->data() + buf->size());
    }
    ProxyIpcServer::getInstance()->requestAPICall(
        CMD_DIAG_WRITE_DID, payload, DEFAULT_TIMEOUT_MS);
}

// Callback handler — registered during registerService()
void DiagManagerAdapter::registerService() {
    auto& registry = CallbackHandlerRegistry::getInstance();
    registry.registerHandler(CB_DIAG_STATUS_CHANGED,
        [](uint16_t id, const std::vector<uint8_t>& payload) {
            // Deserialize → gọi Remotediag
            int32_t what = deserializeInt32(payload, 0);
            int32_t info = deserializeInt32(payload, 4);
            Remotediag::getInstance()->notifyChangedRemoteStatus(what, info);
        });
    registry.registerHandler(CB_DIAG_UNDER_REPAIR_CHANGED,
        [](uint16_t id, const std::vector<uint8_t>& payload) {
            int32_t status = deserializeInt32(payload, 0);
            Remotediag::getInstance()->onUnderRepairStatus(status);
        });
    // ... register other callbacks
}
```

### 5.3. RemoteDiagIpcBridge

Tích hợp IPC server vào lifecycle của RemoteDiag (IP):

```cpp
// remotediag/ipc_integration/RemoteDiagIpcBridge.h
class RemoteDiagIpcBridge {
public:
    static RemoteDiagIpcBridge* getInstance();

    // Gọi từ Remotediag::onCreate() (hoặc trước đó)
    void initialize(const std::string& socketPath);
    void shutdown();

private:
    ProxyIpcServer* mIpcServer;
    ServerCallbackDispatcher* mCallbackDispatcher;
    CallbackHandlerRegistry* mCallbackRegistry;
};
```

---

## 6. EP Side — RemoteDiagProxy

### 6.1. RemoteDiagProxy Application

```cpp
// remotediag_proxy/RemoteDiagProxy.h
class RemoteDiagProxy : public Application {
public:
    static RemoteDiagProxy* getInstance();
    void onCreate() override;
    void onDestroy() override;

private:
    android::sp<RDPHandler> mHandler;
    ProxyIpcClient* mIpcClient;
    CallbackForwarder* mCallbackForwarder;
    CommandHandlerRegistry* mCommandRegistry;
};
```

### 6.2. Lifecycle

```cpp
void RemoteDiagProxy::onCreate() {
    // 1. Init IPC client
    mIpcClient = ProxyIpcClient::getInstance();
    mCommandRegistry = new CommandHandlerRegistry();
    mIpcClient->setCommandDispatcher(mCommandRegistry);

    // 2. Register command handlers (IP→EP requests)
    mCommandRegistry->registerHandler(new DiagCommandHandler());
    mCommandRegistry->registerHandler(new CalibCommandHandler());
    mCommandRegistry->registerHandler(new PowerCommandHandler());
    // ... 11 adapters

    // 3. Init callback forwarder
    mCallbackForwarder = CallbackForwarder::getInstance();
    mCallbackForwarder->setIpcClient(mIpcClient);

    // 4. Start IPC connection
    mIpcClient->start(SOCKET_PATH);
    mIpcClient->waitUntilReady(CONNECTION_TIMEOUT_MS);

    // 5. Register real Binder adapters (với callback forwarding)
    ApplicationManagerAdapter::getInstance()->registerService();
    DiagManagerAdapter::getInstance()->registerService();
    // ... 11 adapters
}
```

### 6.3. Real Adapter (EP) — Callback Forwarding

```cpp
// remotediag_proxy/services/DiagManagerAdapter.cpp (EP - Real Binder)
// Public interface giữ nguyên, thêm forward logic trong Receiver:

void DiagMReceiver::onDiagStatusChanged(int32_t what, int32_t info) {
    std::vector<uint8_t> payload;
    serializeInt32(payload, what);
    serializeInt32(payload, info);
    CallbackForwarder::getInstance()->forward(CB_DIAG_STATUS_CHANGED, payload);
}
```

### 6.4. Command Handler (IP→EP requests)

```cpp
// remotediag_proxy/forwarding/DiagCommandHandler.cpp
struct CommandResponse {
    bool success;
    std::vector<uint8_t> payload;
    static CommandResponse ok(std::vector<uint8_t> data) { return {true, data}; }
    static CommandResponse error() { return {false, {}}; }
};

class DiagCommandHandler : public ICommandHandler {
public:
    bool canHandle(uint16_t cmdId) override {
        return cmdId >= 0x8100 && cmdId <= 0x81FF;
    }

    CommandResponse handle(uint16_t cmdId, const std::vector<uint8_t>& payload) override {
        switch (cmdId) {
            case CMD_DIAG_GET_RDG_FLAG: {
                uint8_t flag = DiagManagerAdapter::getInstance()->getRDGFlag();
                return CommandResponse::ok({flag});
            }
            case CMD_DIAG_GET_DTC_FLAG: {
                uint8_t flag = DiagManagerAdapter::getInstance()->getDTCFlag();
                return CommandResponse::ok({flag});
            }
            case CMD_DIAG_WRITE_DID: {
                uint16_t param = deserializeUint16(payload, 0);
                auto buf = new Buffer();
                buf->setTo(payload.data() + 2, payload.size() - 2);
                DiagManagerAdapter::getInstance()->writeDidData(param, buf);
                return CommandResponse::ok({});
            }
            // ... other commands
        }
        return CommandResponse::error();
    }
};
```

---

## 7. SID Filter

### 7.1. Design

```cpp
// remotediag/sid_filter/SIDFilter.h
class SIDFilter {
public:
    static SIDFilter* getInstance();

    // Load config (whitelist/blacklist)
    void loadConfig(const std::string& configPath);

    // Check if a UDS Service ID is allowed
    bool isAllowed(uint8_t serviceId) const;

    // Filtered UDS send — wraps OnBoardClientAdapter
    uint8_t filteredSendUdsData(uint16_t connectId, const android::sp<Buffer>& udsRequest);

private:
    std::set<uint8_t> mAllowedSIDs;    // Whitelist
    std::set<uint8_t> mBlockedSIDs;    // Blacklist (priority over whitelist)
    bool mDefaultAllow{false};          // Default policy: allow or deny
};
```

### 7.2. Integration

`SIDFilter` wraps `OnboardclientAdapter` calls. Mỗi UDS request đi qua filter:

```cpp
uint8_t SIDFilter::filteredSendUdsData(uint16_t connectId, const android::sp<Buffer>& udsRequest) {
    if (udsRequest == nullptr || udsRequest->size() == 0) {
        return ERROR_INVALID;
    }
    uint8_t serviceId = udsRequest->data()[0];  // First byte = UDS Service ID
    if (!isAllowed(serviceId)) {
        LOG_W("SID 0x%02X blocked by filter", serviceId);
        return ERROR_FILTERED;
    }
    return OnboardclientAdapter::getInstance()->sendUdsData(connectId, udsRequest);
}
```

---

## 8. Error Handling

| Scenario | Handling |
|---|---|
| **IPC connection lost** | UnixSocketClient auto-reconnect (exponential backoff: 100ms→5s max). Mirror adapters return error/default values during disconnect. |
| **Request timeout** | `requestAPICall()` returns `{success=false}` after timeout. Mirror adapter logs error, returns default value. |
| **Invalid frame** | Drop frame, log error, continue recv loop. |
| **Adapter not ready (EP)** | CommandHandler returns error response. IP retries or uses cached value. |
| **Buffer overflow** | IpcFrameCodec validates payload_len against max (64KB default). Reject oversized frames. |

---

## 9. Implementation Order

| Phase | Scope | Dependencies |
|---|---|---|
| **Phase 1** | Transport Layer: `UnixSocketServer`, `UnixSocketClient`, `ISocketCallback` | None |
| **Phase 2** | Protocol Layer: `IpcFrame`, `IpcFrameCodec`, `IpcConstants` | Phase 1 |
| **Phase 3** | IPC Layer: `ProxyIpcServer`, `ProxyIpcClient` | Phase 2 |
| **Phase 4** | EP Side: `RemoteDiagProxy`, `RDPHandler`, real adapters (copy & adapt), `CallbackForwarder`, `CommandHandler` (11 adapters) | Phase 3 |
| **Phase 5** | IP Side: Mirror adapters (11), `RemoteDiagIpcBridge`, `ServerCallbackDispatcher`, `CallbackHandlerRegistry` | Phase 3 |
| **Phase 6** | SID Filter: `SIDFilter` + integration | Phase 5 |

---

## 10. Constraints & Assumptions

- `Remotediag` class (IP) **KHÔNG thay đổi** — chỉ thêm `RemoteDiagIpcBridge` khởi tạo IPC trước `onCreate()`
- Socket path cấu hình qua constant hoặc config file
- Max payload size: 64KB (đủ cho tất cả UDS/DID/gRPC data hiện tại)
- Single EP↔IP connection (1 proxy, 1 remotediag)
- Thread-safe: tất cả shared state bảo vệ bằng mutex
