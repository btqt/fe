// ipc/protocol/IpcFrameCodec.cpp
#include "IpcFrameCodec.h"
#include <cstring>

static uint32_t toBigEndian32(uint32_t val) {
    return ((val >> 24) & 0x000000FF) |
           ((val >> 8)  & 0x0000FF00) |
           ((val << 8)  & 0x00FF0000) |
           ((val << 24) & 0xFF000000);
}

static uint16_t toBigEndian16(uint16_t val) {
    return ((val >> 8) & 0x00FF) |
           ((val << 8) & 0xFF00);
}

static uint32_t fromBigEndian32(uint32_t val) {
    return toBigEndian32(val);
}

static uint16_t fromBigEndian16(uint16_t val) {
    return toBigEndian16(val);
}

std::vector<uint8_t> IpcFrameCodec::encode(const IpcFrame& frame) {
    uint32_t payloadLen = static_cast<uint32_t>(frame.payload.size());
    uint32_t frameLen = 1 + 2 + 4 + 4 + payloadLen; // type(1) + msgId(2) + corrId(4) + payLen(4) + payload
    uint32_t totalLen = 4 + frameLen;               // header len prefix(4)

    std::vector<uint8_t> buf(totalLen);
    uint32_t nTotalLen = toBigEndian32(totalLen);
    uint16_t nMsgId = toBigEndian16(frame.messageId);
    uint32_t nCorrId = toBigEndian32(frame.correlationId);
    uint32_t nPayLen = toBigEndian32(payloadLen);

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
    uint32_t totalLen = fromBigEndian32(nTotalLen);

    if (len < totalLen) return 0; // Insufficient data

    outFrame.frameType = data[4];
    
    uint16_t nMsgId = 0;
    std::memcpy(&nMsgId, &data[5], 2);
    outFrame.messageId = fromBigEndian16(nMsgId);

    uint32_t nCorrId = 0;
    std::memcpy(&nCorrId, &data[7], 4);
    outFrame.correlationId = fromBigEndian32(nCorrId);

    uint32_t nPayLen = 0;
    std::memcpy(&nPayLen, &data[11], 4);
    outFrame.payloadLen = fromBigEndian32(nPayLen);

    outFrame.payload.clear();
    if (outFrame.payloadLen > 0) {
        outFrame.payload.assign(&data[15], &data[15 + outFrame.payloadLen]);
    }

    return totalLen;
}
