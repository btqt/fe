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
    mAllowedSIDs = std::set<uint8_t>(sids.begin(), sids.end());
}

void SIDFilter::setBlockedSIDs(const std::vector<uint8_t>& sids) {
    mBlockedSIDs = std::set<uint8_t>(sids.begin(), sids.end());
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

bool SIDFilter::isAllowedUdsData(const uint8_t* data, size_t size) const {
    if (data == nullptr || size == 0) {
        return false;
    }
    return isAllowed(data[0]);
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
