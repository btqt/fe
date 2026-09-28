// remotediag_proxy/forwarding/CallbackForwarder.h
#pragma once
#include <cstdint>
#include <vector>

class CallbackForwarder {
public:
    static CallbackForwarder* getInstance();
    void forward(uint16_t callbackId, const std::vector<uint8_t>& payload);
};
