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
