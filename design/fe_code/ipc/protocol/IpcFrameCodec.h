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
