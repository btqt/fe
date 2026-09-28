// ipc/protocol/ProxyIpcClient.cpp
#include "ProxyIpcClient.h"
#include "IpcFrameCodec.h"

ProxyIpcClient* ProxyIpcClient::getInstance() {
    static ProxyIpcClient instance;
    return &instance;
}

bool ProxyIpcClient::start(const std::string& socketPath) {
    return mClient.start(socketPath, this);
}

void ProxyIpcClient::stop() {
    mClient.stop();
}

void ProxyIpcClient::setCommandHandler(CommandHandlerFunc handler) {
    mHandler = handler;
}

void ProxyIpcClient::sendCallback(uint16_t callbackId, const std::vector<uint8_t>& payload) {
    IpcFrame cbFrame;
    cbFrame.frameType = IpcFrame::TYPE_CALLBACK;
    cbFrame.messageId = callbackId;
    cbFrame.correlationId = 0;
    cbFrame.payload = payload;
    cbFrame.payloadLen = static_cast<uint32_t>(payload.size());

    auto wireData = IpcFrameCodec::encode(cbFrame);
    mClient.sendData(wireData.data(), wireData.size());
}

void ProxyIpcClient::onDataReceived(const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(mRecvMutex);
    mRecvBuf.insert(mRecvBuf.end(), data, data + len);

    while (true) {
        IpcFrame frame;
        size_t consumed = IpcFrameCodec::decode(mRecvBuf.data(), mRecvBuf.size(), frame);
        if (consumed == 0) break;

        mRecvBuf.erase(mRecvBuf.begin(), mRecvBuf.begin() + consumed);

        if (frame.frameType == IpcFrame::TYPE_REQUEST) {
            bool success = false;
            std::vector<uint8_t> respPayload;
            if (mHandler) {
                respPayload = mHandler(frame.messageId, frame.payload, success);
            }

            IpcFrame respFrame;
            respFrame.frameType = IpcFrame::TYPE_RESPONSE;
            respFrame.messageId = frame.messageId;
            respFrame.correlationId = frame.correlationId;
            respFrame.payload = respPayload;
            respFrame.payloadLen = static_cast<uint32_t>(respPayload.size());

            auto wireData = IpcFrameCodec::encode(respFrame);
            mClient.sendData(wireData.data(), wireData.size());
        }
    }
}

void ProxyIpcClient::onConnected() {}
void ProxyIpcClient::onDisconnected() {}
