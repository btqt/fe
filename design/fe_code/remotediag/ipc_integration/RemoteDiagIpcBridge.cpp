// remotediag/ipc_integration/RemoteDiagIpcBridge.cpp
#include "RemoteDiagIpcBridge.h"
#include "../../ipc/protocol/ProxyIpcServer.h"
#include "CallbackHandlerRegistry.h"

RemoteDiagIpcBridge* RemoteDiagIpcBridge::getInstance() {
    static RemoteDiagIpcBridge instance;
    return &instance;
}

bool RemoteDiagIpcBridge::initialize(const std::string& socketPath) {
    ProxyIpcServer::getInstance()->setCallbackDispatcher(
        [](uint16_t callbackId, const std::vector<uint8_t>& payload) {
            CallbackHandlerRegistry::getInstance()->dispatchCallback(callbackId, payload);
        });
    return ProxyIpcServer::getInstance()->start(socketPath);
}

void RemoteDiagIpcBridge::shutdown() {
    ProxyIpcServer::getInstance()->stop();
}
