#include "../include/IpcMessageHandler.h"
#include "Logger.h"

namespace rdgipc {

void MessageDispatcher::registerCommandHandler(std::shared_ptr<ICommandHandler> handler,
                                                const std::vector<uint32_t> &commandIds) {
    const std::lock_guard<std::mutex> lock{mMutex};
    for (uint32_t commandId : commandIds) {
        if (mCommandHandlers.find(commandId) != mCommandHandlers.end()) {
            LOG_W("MessageDispatcher: command id=%u already registered, overwriting", commandId);
        }
        mCommandHandlers[commandId] = handler;
        LOG_I("MessageDispatcher: registered command id=%u", commandId);
    }
}

void MessageDispatcher::registerCallbackHandler(std::shared_ptr<ICallbackHandler> handler,
                                                 const std::vector<uint32_t> &callbackIds) {
    const std::lock_guard<std::mutex> lock{mMutex};
    for (uint32_t callbackId : callbackIds) {
        if (mCallbackHandlers.find(callbackId) != mCallbackHandlers.end()) {
            LOG_W("MessageDispatcher: callback id=%u already registered, overwriting", callbackId);
        }
        mCallbackHandlers[callbackId] = handler;
        LOG_I("MessageDispatcher: registered callback id=%u", callbackId);
    }
}

CommandResponse MessageDispatcher::dispatchCommand(uint32_t commandId, const std::vector<uint8_t> &payload) {
    std::shared_ptr<ICommandHandler> handler;
    {
        const std::lock_guard<std::mutex> lock{mMutex};
        auto it = mCommandHandlers.find(commandId);
        if (it != mCommandHandlers.end()) {
            handler = it->second;
        }
    }
    
    if (handler) {
        LOG_I("MessageDispatcher: dispatching command id=%u to handler", commandId);
        try {
            return handler->handle(commandId, payload);
        } catch (const std::exception &e) {
            LOG_E("MessageDispatcher: command handler threw for id=%u: %s", commandId, e.what());
            return CommandResponse::err(ErrorCode::OperationFailed);
        } catch (...) {
            LOG_E("MessageDispatcher: command handler threw unknown exception for id=%u", commandId);
            return CommandResponse::err(ErrorCode::OperationFailed);
        }
    }

    LOG_W("MessageDispatcher: no handler found for command id=%u", commandId);
    return CommandResponse::err(ErrorCode::UnsupportedCommand);
}

void MessageDispatcher::dispatchCallback(uint32_t callbackId, const std::vector<uint8_t> &payload) {
    std::shared_ptr<ICallbackHandler> handler;
    {
        const std::lock_guard<std::mutex> lock{mMutex};
        auto it = mCallbackHandlers.find(callbackId);
        if (it != mCallbackHandlers.end()) {
            handler = it->second;
        }
    }
    
    if (handler) {
        LOG_I("MessageDispatcher: dispatching callback id=%u to handler", callbackId);
        try {
            handler->handle(callbackId, payload);
        } catch (const std::exception &e) {
            LOG_E("MessageDispatcher: callback handler threw for id=%u: %s", callbackId, e.what());
        } catch (...) {
            LOG_E("MessageDispatcher: callback handler threw unknown exception for id=%u", callbackId);
        }
        return;
    }

    LOG_W("MessageDispatcher: no handler found for callback id=%u", callbackId);
}

} // namespace rdgipc
