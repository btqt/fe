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
    bool isAllowedUdsData(const uint8_t* data, size_t size) const;
    bool filterAndSendUdsData(uint16_t connectId, const std::vector<uint8_t>& udsData);

private:
    SIDFilter();
    std::set<uint8_t> mAllowedSIDs;
    std::set<uint8_t> mBlockedSIDs;
    bool mDefaultAllow{false};
};
