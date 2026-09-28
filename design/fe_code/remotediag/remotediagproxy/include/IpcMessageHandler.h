#ifndef RDG_IPC_MESSAGE_HANDLER_H
#define RDG_IPC_MESSAGE_HANDLER_H

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace rdgipc {

/**
 * @brief Error codes for command responses
 */
enum class ErrorCode : uint8_t {
    InvalidPayload = 0x01,
    UnsupportedCommand = 0x02,
    OperationFailed = 0x03,
    PayloadTooLarge = 0x04,
};

/**
 * @brief Response structure for command handlers
 */
struct CommandResponse {
    bool success;
    std::vector<uint8_t> payload;

    static CommandResponse ok(const std::vector<uint8_t> &data = {}) {
        return CommandResponse{true, data};
    }

    static CommandResponse ok(const std::string &data) {
        return CommandResponse{true, std::vector<uint8_t>(data.begin(), data.end())};
    }

    static CommandResponse ok(int32_t value) {
        const std::string str = std::to_string(value);
        return CommandResponse{true, std::vector<uint8_t>(str.begin(), str.end())};
    }

    static CommandResponse err(ErrorCode code) {
        return CommandResponse{false, {static_cast<uint8_t>(code)}};
    }
};

/**
 * @brief Interface for handling specific commands
 * 
 * Service adapters should inherit from this interface to handle their commands.
 * This makes it easy to add new commands without modifying the transport layer.
 * 
 * Example:
 * class ApplicationManagerAdapter::CommandHandler : public ICommandHandler {
 *     CommandResponse handle(uint32_t commandId, const vector<uint8_t> &payload) override;
 * };
 */
class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;

    /**
     * @brief Handle a command request
     * @param commandId The command ID
     * @param payload The request payload
     * @return Response to send back
     */
    virtual CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) = 0;
};

/**
 * @brief Interface for handling callbacks
 * 
 * Service adapters should inherit from this interface to handle their callbacks.
 * This makes it easy to add new callbacks without modifying the transport layer.
 * 
 * Example:
 * class PowerManagerAdapter::CallbackHandler : public ICallbackHandler {
 *     void handle(uint32_t callbackId, const vector<uint8_t> &payload) override;
 * };
 */
class ICallbackHandler {
public:
    virtual ~ICallbackHandler() = default;

    /**
     * @brief Handle a callback notification
     * @param callbackId The callback ID
     * @param payload The callback payload
     */
    virtual void handle(uint32_t callbackId, const std::vector<uint8_t> &payload) = 0;
};

/**
 * @brief Dispatcher for routing messages to appropriate handlers
 * 
 * This class maintains a direct mapping from command/callback IDs to handlers.
 * Each handler registers the specific IDs it handles - no need for canHandle() checks.
 */
class MessageDispatcher {
public:
    /**
     * @brief Register a command handler for specific command IDs
     * @param handler Shared pointer to the handler
     * @param commandIds List of command IDs this handler will handle
     */
    void registerCommandHandler(std::shared_ptr<ICommandHandler> handler, 
                                const std::vector<uint32_t> &commandIds);

    /**
     * @brief Register a callback handler for specific callback IDs
     * @param handler Shared pointer to the handler
     * @param callbackIds List of callback IDs this handler will handle
     */
    void registerCallbackHandler(std::shared_ptr<ICallbackHandler> handler,
                                  const std::vector<uint32_t> &callbackIds);

    /**
     * @brief Dispatch a command to the appropriate handler
     * @param commandId The command ID
     * @param payload The request payload
     * @return Response from the handler
     */
    CommandResponse dispatchCommand(uint32_t commandId, const std::vector<uint8_t> &payload);

    /**
     * @brief Dispatch a callback to the appropriate handler
     * @param callbackId The callback ID
     * @param payload The callback payload
     */
    void dispatchCallback(uint32_t callbackId, const std::vector<uint8_t> &payload);

private:
    mutable std::mutex mMutex;  // Protects handler maps
    std::map<uint32_t, std::shared_ptr<ICommandHandler>> mCommandHandlers;
    std::map<uint32_t, std::shared_ptr<ICallbackHandler>> mCallbackHandlers;
};

} // namespace rdgipc

#endif // RDG_IPC_MESSAGE_HANDLER_H
