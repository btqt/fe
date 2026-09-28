// remotediag_proxy/forwarding/DiagCommandHandler.cpp
#include "DiagCommandHandler.h"
#include "CommandHandlerRegistry.h"
#include "../../ipc/protocol/IpcConstants.h"

void DiagCommandHandler::registerHandlers() {
    auto registry = CommandHandlerRegistry::getInstance();

    registry->registerHandler(CMD_DIAG_GET_RDG_FLAG, [](const std::vector<uint8_t>& payload, bool& success) {
        success = true;
        return std::vector<uint8_t>{0x01}; // Stub response for RDG Flag
    });

    registry->registerHandler(CMD_DIAG_WRITE_DID, [](const std::vector<uint8_t>& payload, bool& success) {
        success = true;
        return std::vector<uint8_t>{0x00}; // OK
    });
}
