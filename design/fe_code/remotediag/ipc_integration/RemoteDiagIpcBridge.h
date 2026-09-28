// remotediag/ipc_integration/RemoteDiagIpcBridge.h
#pragma once
#include <string>

class RemoteDiagIpcBridge {
public:
    static RemoteDiagIpcBridge* getInstance();
    bool initialize(const std::string& socketPath);
    void shutdown();
};
