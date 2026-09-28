# Design Spec: OEM Server UDS SID Filter Integration

**Date**: 2026-09-28  
**Status**: Approved  
**Scope**: Integrate `SIDFilter` specifically for OEM Server Direct Commands (`RemoteDirectCommand`) while preserving unfiltered execution for OTA and internal diagnostics.

---

## 1. Context & Objectives

- `SIDFilter` (`remotediag/sid_filter/SIDFilter.h` & `SIDFilter.cpp`) maintains Whitelist and Blacklist configurations for UDS Service IDs (SIDs).
- **Requirement**: `SIDFilter` must ONLY validate diagnostic commands arriving from the OEM Server (`RemoteDirectCommand`).
- **Exclusion**: OTA diagnostics (`RemoteOTA`) and internal self-diagnostics (`RemoteDTC`, `RemoteSSR`, `RemoteRoB`, etc.) must bypass `SIDFilter` and send UDS requests directly to `OnboardclientAdapter`.

---

## 2. Architecture & Design

### 2.1. Integration Point
The UDS transmission logic for OEM Direct Commands is handled inside `RemoteDirectCommand::DirectCommandTransmission::sendUds()`.

- **File**: `remotediag/diagprocess/DirectCommand/RemoteDirectCommand.cpp`
- **Method**: `RemoteDirectCommand::DirectCommandTransmission::sendUds(void)`

### 2.2. SIDFilter Interface Enhancements
To seamlessly interface with `android::sp<Buffer>` used in `remotediag`:
- Add `#include "sid_filter/SIDFilter.h"` in `RemoteDirectCommand.cpp`.
- Add an overload in `SIDFilter`:
  ```cpp
  bool filterAndSendUdsData(uint16_t connectId, const android::sp<Buffer>& udsRequest);
  ```
  or perform validation using `SIDFilter::getInstance()->isAllowed(sid)` directly inside `sendUds()`.

### 2.3. Control Flow

```
RemoteDirectCommand::DirectCommandTransmission::sendUds()
 └── Extract SID from mSentUds->data()[0]
 └── Check SIDFilter::getInstance()->isAllowed(sid)
      ├── [ALLOWED] -> Call OnboardclientAdapter::getInstance()->sendUdsData(mConnectId, mSentUds)
      └── [BLOCKED] -> Log error/warning, set state to DISCONNECT/ABORT, do NOT send data to OBC/CAN.
```

---

## 3. Verification Plan

1. Build & verify C++ syntax in `RemoteDirectCommand.cpp` and `SIDFilter`.
2. Run unit/integration test script in `tmp_script/test_ep_ip_ipc.cpp` (or update test script to verify `SIDFilter` with allowed vs blocked SIDs on direct command flow).
3. Ensure no regression to `RemoteOTA` or other diagnostic modules.
