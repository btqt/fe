// remotediag_proxy/forwarding/CommandHandlerRegistry.h
#pragma once
#include <cstdint>
#include <vector>
#include <functional>
#include <map>

class CommandHandlerRegistry {
public:
    using CommandFunc = std::function<std::vector<uint8_t>(const std::vector<uint8_t>& payload, bool& success)>;

    static CommandHandlerRegistry* getInstance();
    void registerHandler(uint16_t commandId, CommandFunc handler);
    std::vector<uint8_t> handleCommand(uint16_t commandId, const std::vector<uint8_t>& payload, bool& success);

private:
    CommandHandlerRegistry() = default;
    std::map<uint16_t, CommandFunc> mHandlers;
};
