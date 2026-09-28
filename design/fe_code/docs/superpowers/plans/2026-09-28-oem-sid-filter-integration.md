# OEM Server UDS SID Filter Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Integrate `SIDFilter` into `RemoteDirectCommand` to filter UDS requests coming from the OEM Server while allowing OTA and internal diagnostics to bypass filtering.

**Architecture:** Extend `SIDFilter` interface for UDS `Buffer` validation and call `SIDFilter::getInstance()->isAllowed(sid)` inside `RemoteDirectCommand::DirectCommandTransmission::sendUds()` before passing data to `OnboardclientAdapter`.

**Tech Stack:** C++11/14, POSIX/Android sp/Buffer, Google Test / C++ test runner script.

---

### Task 1: Extend SIDFilter Interface for UDS Buffer Validation

**Files:**
- Modify: `remotediag/sid_filter/SIDFilter.h`
- Modify: `remotediag/sid_filter/SIDFilter.cpp`

- [ ] **Step 1: Add overload `filterAndSendUdsData` for `android::sp<Buffer>` in `SIDFilter.h`**

Modify `remotediag/sid_filter/SIDFilter.h`:
```cpp
// remotediag/sid_filter/SIDFilter.h
#pragma once
#include <cstdint>
#include <vector>
#include <set>
#include <utils/RefBase.h>
#include <Buffer.h>

class SIDFilter {
public:
    static SIDFilter* getInstance();

    void setAllowedSIDs(const std::vector<uint8_t>& sids);
    void setBlockedSIDs(const std::vector<uint8_t>& sids);

    bool isAllowed(uint8_t serviceId) const;
    bool filterAndSendUdsData(uint16_t connectId, const std::vector<uint8_t>& udsData);
    bool isAllowedUdsBuffer(const android::sp<Buffer>& udsRequest) const;

private:
    SIDFilter();
    std::set<uint8_t> mAllowedSIDs;
    std::set<uint8_t> mBlockedSIDs;
    bool mDefaultAllow{false};
};
```

- [ ] **Step 2: Implement `isAllowedUdsBuffer` in `SIDFilter.cpp`**

Modify `remotediag/sid_filter/SIDFilter.cpp`:
```cpp
bool SIDFilter::isAllowedUdsBuffer(const android::sp<Buffer>& udsRequest) const {
    if (udsRequest == nullptr || udsRequest->size() == 0 || udsRequest->data() == nullptr) {
        return false;
    }
    uint8_t sid = udsRequest->data()[0];
    return isAllowed(sid);
}
```

---

### Task 2: Integrate SIDFilter into `RemoteDirectCommand`

**Files:**
- Modify: `remotediag/diagprocess/DirectCommand/RemoteDirectCommand.cpp`

- [ ] **Step 1: Add `#include "../../sid_filter/SIDFilter.h"` in `RemoteDirectCommand.cpp`**

Include `SIDFilter.h` near top of `RemoteDirectCommand.cpp`.

- [ ] **Step 2: Add SIDFilter validation in `DirectCommandTransmission::sendUds(void)`**

Modify `DirectCommandTransmission::sendUds()` in `remotediag/diagprocess/DirectCommand/RemoteDirectCommand.cpp`:
```cpp
    void RemoteDirectCommand::DirectCommandTransmission::sendUds(void)
    {
        LOG_I("DirectCommandTransmission::sendUds()");
        mSentUds = this->udsReqList.front();
        this->udsReqList.pop_front();

        // SIDFilter validation for OEM Direct Command
        if (mSentUds != nullptr && mSentUds->data() != nullptr && mSentUds->size() > 0)
        {
            uint8_t sid = mSentUds->data()[0];
            if (!SIDFilter::getInstance()->isAllowed(sid))
            {
                LOG_E("[SIDFilter] DirectCommand UDS SID 0x%02X is BLOCKED by SIDFilter! Aborting transmission.", sid);
                this->disconnect();
                return;
            }
            LOG_I("[SIDFilter] DirectCommand UDS SID 0x%02X is ALLOWED.", sid);
        }

        const uint8_t res{OnboardclientAdapter::getInstance()->sendUdsData(this->mConnectId, mSentUds)};
        if (res != static_cast<uint8_t>(OBCEnum::OBCErrCode::OBC_OK))
        {
            // TODO Handle exeptional case
            this->disconnect();
        }
        else
        {
            if(mSentUds->data() != nullptr) {
                if ((this->mEcuInformation.getDiagPhase() == CommonDefine::DiagPhase::DP_PHASE_5) &&
                    (mSentUds->data()[0] == 0x10U) && (mSentUds->data()[1] == 0x01U))
                {
                    bDefaultSession = true;
                }
                else
                {
                    bDefaultSession = false;
                }
            } else {
                LOG_D("udsRequest->data() is null");
            }

            LOG_I("SendUdsData success, isDefault %d", bDefaultSession);
            this->mState = DirectCommandTransmission::State::DIRECTCOMMAND_TRANS_SEND_UDS;
            RemoteDirectCommand::getInstance()->startTimer(DirectCommandTimerHandler::TIMER_ID_DIRECTCOMMAND_TRANSMISSION_TIMEOUT);
            LOG_I("Start timeout timer for connectId %lu", this->mConnectId);
        }
    }
```

---

### Task 3: Test Verification

**Files:**
- Modify: `tmp_script/test_ep_ip_ipc.cpp`
- Output: `tmp_output/test_result.txt`

- [ ] **Step 1: Update `test_ep_ip_ipc.cpp` to add test cases for `SIDFilter` with allowed vs blocked UDS SIDs**

- [ ] **Step 2: Compile & Run the test script**

Run: `g++ -std=c++14 tmp_script/test_ep_ip_ipc.cpp remotediag/sid_filter/SIDFilter.cpp -o tmp_script/test_ep_ip_ipc.exe`
Run: `./tmp_script/test_ep_ip_ipc.exe > tmp_output/test_result.txt`

- [ ] **Step 3: Confirm test output in `tmp_output/test_result.txt` passes with 0 errors**
