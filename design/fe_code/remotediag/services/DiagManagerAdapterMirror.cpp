// remotediag/services/DiagManagerAdapterMirror.cpp
#include "DiagManagerAdapterMirror.h"
#include "../../ipc/protocol/ProxyIpcServer.h"
#include "../../ipc/protocol/IpcConstants.h"
#include "../ipc_integration/CallbackHandlerRegistry.h"
#include <iostream>

DiagManagerAdapterMirror* DiagManagerAdapterMirror::getInstance() {
    static DiagManagerAdapterMirror instance;
    return &instance;
}

void DiagManagerAdapterMirror::registerService() {
    CallbackHandlerRegistry::getInstance()->registerHandler(CB_DIAG_STATUS_CHANGED,
        [](const std::vector<uint8_t>& payload) {
            std::cout << "[IP DiagManagerMirror] Received CB_DIAG_STATUS_CHANGED" << std::endl;
        });
}

uint8_t DiagManagerAdapterMirror::getRDGFlag() {
    auto resp = ProxyIpcServer::getInstance()->requestAPICall(CMD_DIAG_GET_RDG_FLAG, {}, 3000);
    if (resp.success && !resp.payload.empty()) {
        return resp.payload[0];
    }
    return 0;
}

void DiagManagerAdapterMirror::writeDidData(uint16_t param, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>(param >> 8));
    payload.push_back(static_cast<uint8_t>(param & 0xFF));
    payload.insert(payload.end(), data.begin(), data.end());

    ProxyIpcServer::getInstance()->requestAPICall(CMD_DIAG_WRITE_DID, payload, 3000);
}
