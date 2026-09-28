// remotediag/ipc_integration/CallbackHandlerRegistry.h
#pragma once
#include <cstdint>
#include <vector>
#include <functional>
#include <map>

class CallbackHandlerRegistry {
public:
    using CallbackFunc = std::function<void(const std::vector<uint8_t>& payload)>;

    static CallbackHandlerRegistry* getInstance();
    void registerHandler(uint16_t callbackId, CallbackFunc handler);
    void dispatchCallback(uint16_t callbackId, const std::vector<uint8_t>& payload);

private:
    CallbackHandlerRegistry() = default;
    std::map<uint16_t, CallbackFunc> mHandlers;
};
