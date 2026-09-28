#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <fstream>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "utils/Logger.h"
#include "ParamsDef.h"
#include "FaServer.h"
#include "RemoteOTA.h"

namespace rdgapp {

FaServer::FaServer(const uint16_t port, const android::sp<sl::Handler> aHandler)
    : android::RefBase()
    , mIsRunning(false)
    , mIsActive(true) // Communication between FA and RemoteDiag is enable by default
    , mListeningSocket(0)
    , mClientFd(0)
    , mPort(port)
    , mHandler(aHandler)
{}

void FaServer::startup()
{
    LOG_I("startup");
    if (mIsRunning == false)
    {
        mListenerThread = std::thread(&FaServer::runLoop, this);
        mIsRunning = true;
    } else {
        LOG_I("Start Listener Thread failed");
    }

}

void FaServer::runLoop()
{   
    mListeningSocket = socket(AF_INET, static_cast<int32_t>(SOCK_STREAM) , 0);
    if (mListeningSocket < 0)
    {
        LOG_E("Failed to create a TCP socket");
    } else {
        constexpr int32_t options {1};
        int32_t ret {setsockopt(mListeningSocket, SOL_SOCKET, static_cast<int32_t>((static_cast<uint32_t>(SO_REUSEADDR) | static_cast<uint32_t>(SO_REUSEPORT))), &options, sizeof(options))};

        if (ret < 0)
        {
            LOG_E("Failed to set socket options");
            (void)close(mListeningSocket);
        } else {
            sockaddr_in serverAddr;
            (void)memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = static_cast<uint16_t>(AF_INET);
            serverAddr.sin_addr.s_addr = 0x00000000U; // INADDR_ANY
            serverAddr.sin_port = htons(mPort);
            ret = bind(mListeningSocket, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr));
            if (ret < 0)
            {
                LOG_E("socket bind failed");
                (void)close(mListeningSocket);
            } else {
                ret = listen(mListeningSocket, 5);
                if (ret < 0)
                {
                    LOG_E("socket listen failed");
                    (void)close(mListeningSocket);
                } else {
                    LOG_I("Started TCP server Listening on port %d success", mPort);

                    while(mIsRunning)
                    {
                        sockaddr_in clientAddr {};
                        socklen_t clientLen;
                        clientLen = sizeof(clientAddr);
                        mClientFd = accept(mListeningSocket, (struct sockaddr*)&clientAddr, &clientLen);
                        if (mClientFd < 0)
                        {
                            LOG_W("connection is rejected. Because, mClientFd = %d", mClientFd);

                        } else if (mIsActive == true) {
                            handleConnection(mClientFd);
                        } else {
                            LOG_I("connection is rejected. Because, FirewallManager disabled OTA function");
                        }
                    }
                }
            }
        }
    }
}

void FaServer::stop()
{
    mIsRunning = false;
    (void)close(mListeningSocket);
}

::TIGER_ERR FaServer::notify(const android::sp<::Buffer> notifyData)
{
    ::TIGER_ERR error{E_ERROR};
    if (notifyData == nullptr)
    {
        LOG_E("Can't Notify, input data null");
        error = E_BUFFER_EMPTY;
    } else {
        const uint32_t notifyDataSize {notifyData->size()};
        if (notifyDataSize <= 0U)
        {
            LOG_E("Can't Notify, input data empty");
            error = E_INPUT_EMPTY;
        } else {
            const ssize_t byteSent {send(mClientFd, notifyData->data(), notifyData->size(), 0)};
            if (byteSent < 0)
            {
                LOG_E("Notify to FA client failed, error = %d", byteSent);
                error = E_ERROR;
            } else {
                LOG_I("Notified %d bytes to FA client", notifyData->size()); 
                error = E_OK;
            }
        }
    }
    return error;
}

void FaServer::handleConnection(const int32_t socket)
{
   fd_set read_sd{};
   FD_ZERO(&read_sd);
   if (socket > 0) {
    FD_SET(socket, &read_sd);
    bool isWaitingFragment {false};
    std::vector<uint8_t> tcpPayload{};
    uint32_t payloadSize {0U};
    uint32_t receivedLenghtCount {0U};
    while(mClientFd > 0)
    {
        fd_set rsd{};
        (void)std::memcpy(&rsd, &read_sd, sizeof(fd_set));
        if ((socket+1) < INT32_MAX) {

            int32_t sel {-1};

            if (isWaitingFragment) {
                struct timeval timeout;
                timeout.tv_sec = 3;
                sel  = select(socket+1, &rsd, nullptr, nullptr, &timeout);
            } else {
                sel  = select(socket+1, &rsd, nullptr, nullptr, nullptr);
            }

            if (sel > 0) {
                ::Buffer fragmentData {};
                fragmentData.setSize(static_cast<int32_t>(ParamsDef::FA_SERVER_MAX_PACKET_SIZE));
                const ssize_t lenght {recv(mClientFd, fragmentData.data(), ParamsDef::FA_SERVER_MAX_PACKET_SIZE, 0)};
                if (lenght <= 0)
                {
                    LOG_E("Can't received any data from socket -> client disconnected");
                    (void)mHandler->obtainMessage(RemoteOTA::MainHandler::CMD_OTA_FA_CLIENT_DISCONNECTED)->sendToTarget();
                    mClientFd = 0;
                    break;
                } else {
                    LOG_I("FA Sverver received data, lenght = %d", lenght);
                    receivedLenghtCount += static_cast<uint32_t>(lenght);
                    if (tcpPayload.size() == 0U)
                    {
                        payloadSize = getPayloadSize(fragmentData);
                    }

                    (void)tcpPayload.insert(tcpPayload.cend(), fragmentData.data(), fragmentData.data() + lenght);

                    if (receivedLenghtCount >= (payloadSize + 8U))
                    {
                        const android::sp<::Buffer> faRawData {new ::Buffer()};
                        if (receivedLenghtCount <= static_cast<uint32_t>(INT32_MAX))
                        {
                            faRawData->setTo(tcpPayload.data(), static_cast<int32_t>(receivedLenghtCount));
                        } else {
                            LOG_E("receivedLenghtCount out of range with int32_t, receivedLenghtCount = %d", payloadSize);
                        }
                        (void)mHandler->obtainMessage(RemoteOTA::MainHandler::CMD_OTA_EXECULTE_REQ, faRawData)->sendToTarget();
                        (void)tcpPayload.clear();
                        payloadSize = 0U;
                        receivedLenghtCount = 0U;
                        isWaitingFragment = false;
                    } else {
                        isWaitingFragment = true;
                        (void)mHandler->obtainMessage(RemoteOTA::MainHandler::CMD_OTA_WAITING_FRAGMENT_DATA)->sendToTarget();
                    }
                }
            }
            else if (sel == 0)
            {
                // timeout, socket does not have anything to read
                const android::sp<::Buffer> faRawData {new ::Buffer()};
                if (receivedLenghtCount <= static_cast<uint32_t>(INT32_MAX))
                {
                    faRawData->setTo(tcpPayload.data(), static_cast<int32_t>(receivedLenghtCount));
                } else {
                    LOG_E("receivedLenghtCount out of range with int32_t, receivedLenghtCount = %d", payloadSize);
                }
                (void)mHandler->obtainMessage(RemoteOTA::MainHandler::CMD_OTA_FA_CLIENT_TIMEOUT, faRawData)->sendToTarget();
                (void)tcpPayload.clear();
                payloadSize = 0U;
                receivedLenghtCount = 0U;
                isWaitingFragment = false;
            }
            else {
                // error occured
                LOG_E("Unknow error occured");
                (void)mHandler->obtainMessage(RemoteOTA::MainHandler::CMD_OTA_FA_CLIENT_DISCONNECTED)->sendToTarget();
                mClientFd = 0;
            }
        } else {
            LOG_E("socket+1 Out of range");
        }
    }
    (void)tcpPayload;
    (void)payloadSize;
    (void)receivedLenghtCount;
    (void)isWaitingFragment;
   } else {
        LOG_E("handleConnection failed, socket <= 0");
   }
}

void FaServer::active(const bool action) noexcept
{
    mIsActive = action;
}

void FaServer::closeCurrentClient()
{
    if (mClientFd > 0) {
        (void)close(mClientFd);
        LOG_D("Close current client socket successfully");
    } else {
        LOG_D("Abort. Client socket already closed");
    }
    mClientFd = 0;
}

uint32_t FaServer::getPayloadSize(::Buffer& rawData) 
{
    uint32_t payloadSize {0U};
    if (rawData.empty() == true)
    {
        LOG_E("OTA request data is empty");
    } 
    else 
    {
        const uint32_t rawDataSize {rawData.size()};
        if (rawDataSize < OtaMessageDefs::FA_PROTOCOL_HEADER_LENGHT)
        {
            LOG_E("OTA request data not enough (size < 8 bytes)");
        } else {
            uint8_t faProtoVer {0U};
            (void)std::memcpy(&faProtoVer, rawData.data() + OtaMessageDefs::FA_PROTO_VERSION_BYTE_MASK, 1U);
            if (faProtoVer > 1U)
            {
                LOG_E("FA Protocol Version: %d is not supported", faProtoVer);
            } else {
                uint8_t mid {0U};
                (void)std::memcpy(&mid, rawData.data() + OtaMessageDefs::MID_BYTE_MASK, 1U);

                if (mid == static_cast<uint8_t>(OTA_MID::OTA_MID_5_SEND_UDS_DATA_REQ))
                {
                    const uint8_t payloadSizeHigh {*(rawData.data() + OtaMessageDefs::PAYLOAD_SIZE_BYTE_MASK)};
                    const uint8_t payloadSizeMid {*(rawData.data() + OtaMessageDefs::PAYLOAD_SIZE_BYTE_MASK + 1U)};
                    const uint8_t payloadSizeLow {*(rawData.data() + OtaMessageDefs::PAYLOAD_SIZE_BYTE_MASK + 2U)};

                    payloadSize |= static_cast<uint32_t>(payloadSizeHigh) << 16U;
                    payloadSize |= static_cast<uint32_t>(payloadSizeMid) << 8U;
                    payloadSize |= static_cast<uint32_t>(payloadSizeLow);
                    LOG_D("Get payload size %d", payloadSize);
                }
            }
        }
    }
    return payloadSize;
}
}
