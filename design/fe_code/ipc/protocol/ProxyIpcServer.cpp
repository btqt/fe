// ipc/protocol/ProxyIpcServer.cpp
#include "ProxyIpcServer.h"
#include "IpcFrameCodec.h"
#include <iostream>
#include <chrono>

ProxyIpcServer* ProxyIpcServer::getInstance() {
    static ProxyIpcServer instance;
    return &instance;
}

bool ProxyIpcServer::start(const std::string& socketPath) {
    return mServer.start(socketPath, this);
}

void ProxyIpcServer::stop() {
    mServer.stop();
}

void ProxyIpcServer::setCallbackDispatcher(CallbackDispatcherFunc dispatcher) {
    mDispatcher = dispatcher;
}

ProxyIpcServer::ApiResponse ProxyIpcServer::requestAPICall(uint16_t commandId, const std::vector<uint8_t>& payload, uint32_t timeoutMs) {
    uint32_t corrId = mNextCorrelationId++;

    IpcFrame reqFrame;
    reqFrame.frameType = IpcFrame::TYPE_REQUEST;
    reqFrame.messageId = commandId;
    reqFrame.correlationId = corrId;
    reqFrame.payload = payload;
    reqFrame.payloadLen = static_cast<uint32_t>(payload.size());

    auto wireData = IpcFrameCodec::encode(reqFrame);

    std::unique_lock<std::mutex> lock(mResponseMutex);
    if (!mServer.sendData(wireData.data(), wireData.size())) {
        return {false, {}};
    }

    bool status = mResponseCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this, corrId]() {
        return mPendingResponses.find(corrId) != mPendingResponses.end();
    });

    if (!status) {
        return {false, {}};
    }

    ApiResponse resp = mPendingResponses[corrId];
    mPendingResponses.erase(corrId);
    return resp;
}

void ProxyIpcServer::onDataReceived(const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(mRecvMutex);
    mRecvBuf.insert(mRecvBuf.end(), data, data + len);

    while (true) {
        IpcFrame frame;
        size_t consumed = IpcFrameCodec::decode(mRecvBuf.data(), mRecvBuf.size(), frame);
        if (consumed == 0) break;

        mRecvBuf.erase(mRecvBuf.begin(), mRecvBuf.begin() + consumed);

        if (frame.frameType == IpcFrame::TYPE_CALLBACK) {
            if (mDispatcher) {
                mDispatcher(frame.messageId, frame.payload);
            }
        } else if (frame.frameType == IpcFrame::TYPE_RESPONSE) {
            std::lock_guard<std::mutex> rLock(mResponseMutex);
            mPendingResponses[frame.correlationId] = {true, frame.payload};
            mResponseCv.notify_all();
        }
    }
}

void ProxyIpcServer::onConnected() {}
void ProxyIpcServer::onDisconnected() {}
