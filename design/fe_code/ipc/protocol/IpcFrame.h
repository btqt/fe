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
