// remotediag_proxy/forwarding/CommandHandlerRegistry.cpp
#include "CommandHandlerRegistry.h"

CommandHandlerRegistry* CommandHandlerRegistry::getInstance() {
    static CommandHandlerRegistry instance;
    return &instance;
}

void CommandHandlerRegistry::registerHandler(uint16_t commandId, CommandFunc handler) {
    mHandlers[commandId] = handler;
}

std::vector<uint8_t> CommandHandlerRegistry::handleCommand(uint16_t commandId, const std::vector<uint8_t>& payload, bool& success) {
    auto it = mHandlers.find(commandId);
    if (it != mHandlers.end()) {
        return it->second(payload, success);
    }
    success = false;
    return {};
}
