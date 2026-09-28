// remotediag/ipc_integration/CallbackHandlerRegistry.cpp
#include "CallbackHandlerRegistry.h"

CallbackHandlerRegistry* CallbackHandlerRegistry::getInstance() {
    static CallbackHandlerRegistry instance;
    return &instance;
}

void CallbackHandlerRegistry::registerHandler(uint16_t callbackId, CallbackFunc handler) {
    mHandlers[callbackId] = handler;
}

void CallbackHandlerRegistry::dispatchCallback(uint16_t callbackId, const std::vector<uint8_t>& payload) {
    auto it = mHandlers.find(callbackId);
    if (it != mHandlers.end()) {
        it->second(payload);
    }
}
