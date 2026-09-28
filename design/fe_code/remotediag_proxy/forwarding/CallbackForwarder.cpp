// remotediag_proxy/forwarding/CallbackForwarder.cpp
#include "CallbackForwarder.h"
#include "../../ipc/protocol/ProxyIpcClient.h"

CallbackForwarder* CallbackForwarder::getInstance() {
    static CallbackForwarder instance;
    return &instance;
}

void CallbackForwarder::forward(uint16_t callbackId, const std::vector<uint8_t>& payload) {
    ProxyIpcClient::getInstance()->sendCallback(callbackId, payload);
}
