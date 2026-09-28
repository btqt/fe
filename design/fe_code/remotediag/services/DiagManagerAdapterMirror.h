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
