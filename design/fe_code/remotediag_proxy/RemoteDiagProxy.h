// remotediag_proxy/RemoteDiagProxy.h
#pragma once
#include <string>

class RemoteDiagProxy {
public:
    static RemoteDiagProxy* getInstance();
    void onCreate(const std::string& socketPath);
    void onDestroy();
};
