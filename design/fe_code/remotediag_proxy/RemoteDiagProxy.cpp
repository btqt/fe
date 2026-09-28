// remotediag_proxy/RemoteDiagProxy.cpp
#include "RemoteDiagProxy.h"
#include "../ipc/protocol/ProxyIpcClient.h"
#include "forwarding/CommandHandlerRegistry.h"
#include <iostream>

RemoteDiagProxy* RemoteDiagProxy::getInstance() {
    static RemoteDiagProxy instance;
    return &instance;
}

void RemoteDiagProxy::onCreate(const std::string& socketPath) {
    ProxyIpcClient::getInstance()->setCommandHandler(
        [](uint16_t cmdId, const std::vector<uint8_t>& payload, bool& success) {
            return CommandHandlerRegistry::getInstance()->handleCommand(cmdId, payload, success);
        });
    ProxyIpcClient::getInstance()->start(socketPath);
    std::cout << "[RemoteDiagProxy] Daemon initialized and connecting to Unix socket: " << socketPath << std::endl;
}

void RemoteDiagProxy::onDestroy() {
    ProxyIpcClient::getInstance()->stop();
}
