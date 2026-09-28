#include <arpa/inet.h>
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
#include <poll.h>
#include <netinet/tcp.h>

#include "utils/Logger.h"
#include "ParamsDef.h"
#include "FaServer.h"
#include "RemoteOTA.h"
#include "RegionManagerAdapter.h"

namespace rdgapp
{

    FaServer::FaServer(const uint16_t port, const android::sp<sl::Handler> aHandler)
        : android::RefBase(), mIsRunning(false), mIsActive(true), mShouldListeningData(false), mIsListeningDataRunning(false) // Communication between FA and RemoteDiag is enable by default
          ,
          mListeningSocket(-1), mClientFd(-1), mPort(port), mHandler(aHandler)
    {
    }

    void FaServer::startup()
    {
        LOG_I("startup");
        if (mIsRunning == false)
        {
            mConnectionListenerThread = std::thread(&FaServer::runLoop, this);
            mConnectionListenerThread.detach(); // Detach the thread to allow it to run independently
            mIsRunning = true;
        }
        else
        {
            LOG_E("Start Listener Thread failed");
        }
    }

    void FaServer::runLoop()
    {
        (void)pthread_setname_np(pthread_self(), "FaServer");
        mListeningSocket = socket(AF_INET, static_cast<int32_t>(SOCK_STREAM) | static_cast<int32_t>(SOCK_NONBLOCK) | static_cast<int32_t>(SOCK_CLOEXEC), 0);
        if (mListeningSocket < 0)
        {
            LOG_E("Failed to create a TCP socket");
        }
        else
        {
            constexpr int32_t keepidle{1};  // Start sending keepalives after 1 seconds
            constexpr int32_t keepintvl{3}; // Send keepalives every 3 seconds
            constexpr int32_t keepcnt{2};   // Send up to 2 keepalives before considering the connection dead
            constexpr int32_t optval{1};
            constexpr int32_t options{1};

            // Enable the SO_KEEPALIVE option
            if (setsockopt(mListeningSocket, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == -1)
            {
                LOG_E("setsockopt SO_KEEPALIVE error, errno = %m");
            }

            if (setsockopt(mListeningSocket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) == -1)
            {
                LOG_E("setsockopt TCP_KEEPIDLE error, errno = %m");
            }

            if (setsockopt(mListeningSocket, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) == -1)
            {
                LOG_E("setsockopt TCP_KEEPINTVL error, errno = %m");
            }

            if (setsockopt(mListeningSocket, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt)) == -1)
            {
                LOG_E("setsockopt TCP_KEEPCNT error, errno = %m");
            }

            int32_t ret{setsockopt(mListeningSocket, SOL_SOCKET, static_cast<int32_t>((static_cast<uint32_t>(SO_REUSEADDR) | static_cast<uint32_t>(SO_REUSEPORT))), &options, sizeof(options))};

            if (ret < 0)
            {
                LOG_E("Failed to set SO_REUSEADDR options %m");
                closeListeningSocket();
            }
            else
            {
                sockaddr_in serverAddr;
                (void)memset(&serverAddr, 0, sizeof(serverAddr));
                serverAddr.sin_family = static_cast<uint16_t>(AF_INET);
                serverAddr.sin_addr.s_addr = 0x00000000U; // INADDR_ANY
                serverAddr.sin_port = htons(mPort);

                ret = bind(mListeningSocket, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr));
                if (ret < 0)
                {
                    LOG_E("socket bind failed");
                    closeListeningSocket();
                }
                else
                {
                    ret = listen(mListeningSocket, 5);
                    if (ret < 0)
                    {
                        LOG_E("socket listen failed");
                        closeListeningSocket();
                    }
                    else
                    {
                        LOG_E("port: %u", mPort);

                        while (mIsRunning)
                        {
                            std::array<struct pollfd, 1> pollfds{};
                            pollfds[0].fd = mListeningSocket;
                            pollfds[0].events = POLLIN;

                            LOG_E("waiting client connection");
                            const int32_t pollCnt{poll(pollfds.data(), pollfds.size(), -1)};
                            if (pollCnt <= 0)
                            {
                                const int32_t savedErrno {errno};
                                if (savedErrno == EINTR)
                                {
                                    LOG_E("polling interupted %m");
                                    continue;
                                }
                                else
                                {
                                    LOG_E("polling error, errno = %m");
                                    closeListeningSocket();
                                    break;
                                }
                            }
                            else
                            {
                                if ((pollfds[0].revents & POLLIN) != 0)
                                {
                                    sockaddr_in clientAddr{};
                                    socklen_t clientLen;
                                    clientLen = sizeof(clientAddr);
                                    const int32_t newClientFd {accept(mListeningSocket, (struct sockaddr *)&clientAddr, &clientLen)};
                                    const uint16_t clientPort{ntohs(clientAddr.sin_port)};
                                    const uint8_t currRegion{RegionManagerAdapter::getInstance()->getNation()};
                                    const int32_t currentClientFd {mClientFd.load()};
                                    if (newClientFd < 0)
                                    {
                                        LOG_E("connection is rejected. Because, mClientFd = %d", newClientFd);
                                    }
                                    else if ((currRegion == LGE_REGION::LGE_REGION_CN) && ((clientPort < 42601U) || ((clientPort > 42700U) && (clientPort < 43601U)) || (clientPort > 47600U)))
                                    {
                                        LOG_E("Invalid client port, newClientFd = %d, addr = %s, port = %u", newClientFd, inet_ntoa(clientAddr.sin_addr), clientPort);
                                        (void)close(newClientFd);
                                    }
                                    else if ((currRegion != LGE_REGION::LGE_REGION_CN) && ((clientPort < 43601U) || (clientPort > 47600U)))
                                    {
                                        LOG_E("Invalid client port, newClientFd = %d, addr = %s, port = %u", newClientFd, inet_ntoa(clientAddr.sin_addr), clientPort);
                                        (void)close(newClientFd);
                                    }
                                    else if ((currentClientFd > 0) && (newClientFd != currentClientFd))
                                    {
                                        LOG_W("Send message to stop old connection");
                                        (void)mHandler->obtainMessage(RemoteOTA::MainHandler::CMD_STOP_OLD_CONNECTION)->sendToTarget();
                                        LOG_E("Client request new connection, mClientFd = %d, newClientFd = %d", currentClientFd, newClientFd);
                                        (void)close(currentClientFd);
                                        mClientFd = newClientFd;
                                    }
                                    else if (mIsActive == true)
                                    {
                                        mClientFd = newClientFd;
                                        LOG_E("newClientFd = %d, addr = %s, port = %u", newClientFd, inet_ntoa(clientAddr.sin_addr), clientPort);
                                        mDataListenerThread = std::thread(&FaServer::handleConnection, this);
                                        mDataListenerThread.detach(); // Detach the thread to allow it to run independently
                                    }
                                    else
                                    {
                                        LOG_E("connection is rejected. Because, FirewallManager disabled OTA function, mIsActive = %d", mIsActive);
                                        (void)close(newClientFd);
                                    }
                                    (void)currRegion;
                                    (void)newClientFd;
                                    (void)currentClientFd;
                                }
                            }
                        }
                        closeListeningSocket();
                    }
                }
            }
        }
    }

    void FaServer::stop()
    {
        mIsRunning = false;
        closeListeningSocket();
    }

    ::TIGER_ERR FaServer::notify(const android::sp<::Buffer> notifyData)
    {
        ::TIGER_ERR error{E_ERROR};
        if (notifyData == nullptr)
        {
            LOG_E("Can't Notify, input data null");
            error = E_BUFFER_EMPTY;
        }
        else
        {
            const uint32_t tempNotifyDataSize{notifyData->size()};
            if ((tempNotifyDataSize == 0U) || (notifyData->data() == nullptr) || (tempNotifyDataSize > ParamsDef::FA_SERVER_MAX_PACKET_SEND_SIZE))
            {
                LOG_E("Notify failed, invalid input data");
                error = E_INVALID_PARAM;
            }
            else
            {
                ssize_t byteSent{0};
                int32_t savedErrno{0};
                const int32_t currentClientFd {mClientFd.load()}; // Fix static issue CHECK_RETURN
                do {
                    if(currentClientFd > 0)
                    {
                        byteSent = send(currentClientFd, notifyData->data(), notifyData->size(), MSG_DONTWAIT);
                        savedErrno = errno;
                    }
                    else
                    {
                        byteSent = -1;
                        savedErrno = EINTR;
                    }
                } while((byteSent == -1) && (savedErrno == EINTR));
                const int32_t notifyDataSize{static_cast<int32_t>(tempNotifyDataSize)};
                if (byteSent == notifyDataSize)
                {
                    LOG_D("Notified %u bytes to FA client", notifyData->size());
                    error = E_OK;
                }
                else if (savedErrno == EAGAIN)
                {
                    error = E_PENDING;
                    LOG_E("socket buffer full, MM client or network has some problem : %m");
                }
                else
                {
                    LOG_E("Notify to FA client failed, byteSent = %ld, errno = %m", byteSent);
                    error = E_ERROR;
                    closeCurrentClient();
                }
            }
        }
        return error;
    }

    void FaServer::active(const bool action) noexcept
    {
        LOG_E("change mIsActive %d -> %d", mIsActive, action);
        mIsActive = action;
    }

    void FaServer::closeCurrentClient()
    {
        if (mClientFd.load() != -1)
        {
            (void)close(mClientFd.load());
            mClientFd = -1;
            LOG_W("Client socket closed");
        }
        else
        {
            LOG_W("Abort. Client socket already closed");
        }
    }

    void FaServer::closeListeningSocket()
    {
        if (mListeningSocket != -1)
        {
            (void)close(mListeningSocket);
            mListeningSocket = -1;
            LOG_W("Listening socket closed");
        }
        else
        {
            LOG_W("Abort. listening socket already closed");
        }
    }

    uint32_t FaServer::getPayloadSize(::Buffer &rawData)
    {
        uint32_t payloadSize{0U};
        if ((rawData.empty() == true) || (rawData.data() == nullptr))
        {
            LOG_E("OTA request data is empty");
        }
        else
        {
            const uint32_t rawDataSize{rawData.size()};
            if (rawDataSize < OtaMessageDefs::FA_PROTOCOL_HEADER_LENGHT)
            {
                LOG_E("OTA request data not enough (size < 8 bytes)");
            }
            else
            {
                uint8_t faProtoVer{0U};
                (void)std::memcpy(&faProtoVer, rawData.data() + OtaMessageDefs::FA_PROTO_VERSION_BYTE_MASK, 1U);
                if (faProtoVer > 1U)
                {
                    LOG_E("FA Protocol Version: %d is not supported", faProtoVer);
                }
                else
                {
                    uint8_t mid{0U};
                    (void)std::memcpy(&mid, rawData.data() + OtaMessageDefs::MID_BYTE_MASK, 1U);

                    if (mid == static_cast<uint8_t>(OTA_MID::OTA_MID_5_SEND_UDS_DATA_REQ))
                    {
                        const uint8_t payloadSizeHigh{*(rawData.data() + OtaMessageDefs::PAYLOAD_SIZE_BYTE_MASK)};
                        const uint8_t payloadSizeMid{*(rawData.data() + OtaMessageDefs::PAYLOAD_SIZE_BYTE_MASK + 1U)};
                        const uint8_t payloadSizeLow{*(rawData.data() + OtaMessageDefs::PAYLOAD_SIZE_BYTE_MASK + 2U)};

                        payloadSize |= static_cast<uint32_t>(payloadSizeHigh) << 16U;
                        payloadSize |= static_cast<uint32_t>(payloadSizeMid) << 8U;
                        payloadSize |= static_cast<uint32_t>(payloadSizeLow);
                        LOG_D("Get payload size %u", payloadSize);
                    }
                }
            }
        }
        return payloadSize;
    }

    void FaServer::handleConnection(void)
    {
        LOG_E("DataListenerThread started");
        bool isWaitingFragment{false};
        std::vector<uint8_t> tcpPayload{};
        uint32_t payloadSize{0U};
        uint32_t receivedLenghtCount{0U};
        mShouldListeningData = true;
        mIsListeningDataRunning = true;
        while ((mClientFd.load() > 0) && mShouldListeningData.load())
        {
            int32_t pollCnt{-1};
            std::array<struct pollfd, 1> pollClientfds{};
            pollClientfds[0].fd = mClientFd.load();
            pollClientfds[0].events = POLLIN;
            if (isWaitingFragment)
            {
                pollCnt = poll(pollClientfds.data(), pollClientfds.size(), 3000);
            }
            else
            {
                pollCnt = poll(pollClientfds.data(), pollClientfds.size(), 1000); // Wait for 1 seconds
            }

            if (pollCnt < 0)
            {
                const int32_t savedErrno {errno};
                if (savedErrno == EINTR)
                {
                    LOG_E("polling interupted");
                    mShouldListeningData = true;
                }
                else
                {
                    LOG_E("polling error, errno = %m");
                    closeCurrentClient();
                    mShouldListeningData = false;
                }
            }
            else if ((pollCnt == 0) && (isWaitingFragment == true))
            {
                LOG_E("polling timeout 3 seconds");
                // timeout, socket does not have anything to read
                const android::sp<::Buffer> faRawData{new ::Buffer()};
                if (receivedLenghtCount <= static_cast<uint32_t>(INT32_MAX))
                {
                    faRawData->setTo(tcpPayload.data(), static_cast<int32_t>(receivedLenghtCount));
                }
                else
                {
                    LOG_E("receivedLenghtCount out of range with int32_t, receivedLenghtCount = %u", receivedLenghtCount);
                }
                (void)mHandler->obtainMessage(RemoteOTA::MainHandler::CMD_OTA_FA_CLIENT_TIMEOUT, faRawData)->sendToTarget();
                (void)tcpPayload.clear();
                payloadSize = 0U;
                receivedLenghtCount = 0U;
                isWaitingFragment = false;
            }
            else if ((pollCnt == 0)) // (isWaitingFragment = false)
            {
                LOG_D("connection is idle");
                continue; // No data to read, continue to wait for next data

            }
            else if ((pollClientfds[0].revents & POLLNVAL) != 0)
            {
                LOG_E("Invalid polling request try again");
                continue;

            }
            else if ((pollClientfds[0].revents & POLLIN) != 0)
            {
                ::Buffer fragmentData{};

                
                fragmentData.setSize(static_cast<int32_t>(ParamsDef::FA_SERVER_MAX_PACKET_SIZE));

                ssize_t lenght{0};
                if (fragmentData.data() != nullptr) 
                {
                    const int32_t currentClientFd {mClientFd.load()}; // Fix static issue CHECK_RETURN
                    if (currentClientFd > 0)
                    {
                        lenght = recv(currentClientFd, fragmentData.data(), ParamsDef::FA_SERVER_MAX_PACKET_SIZE, MSG_DONTWAIT);
                    }
                    else
                    {
                        lenght = 0;
                    }
                }

                if (lenght == 0)
                {
                    LOG_E("Client disconnected");
                    closeCurrentClient();
                    mShouldListeningData = false;
                }
                else if (lenght == -1)
                {
                    const int32_t savedErrno {errno};
                    if ((savedErrno == EINTR) || (savedErrno == EAGAIN))
                    {
                        LOG_E("Socket buffer empty or interupted, errno = %m");
                        mShouldListeningData = true;
                    }
                    else
                    {
                        LOG_E("Read data from client failed, errno = %m");
                        closeCurrentClient();
                        mShouldListeningData = false;
                    }
                }
                else
                {
                    LOG_D("FA Sverver received data, lenght = %ld", lenght);
                    receivedLenghtCount += static_cast<uint32_t>(lenght);
                    if (tcpPayload.size() == 0U)
                    {
                        payloadSize = getPayloadSize(fragmentData);
                    }

                    (void)tcpPayload.insert(tcpPayload.cend(), fragmentData.data(), fragmentData.data() + lenght);

                    if ( ((payloadSize + 8U) <= UINT32_MAX) && (receivedLenghtCount >= (payloadSize + 8U)))
                    {
                        const android::sp<::Buffer> faRawData{new ::Buffer()};
                        if (receivedLenghtCount <= static_cast<uint32_t>(INT32_MAX))
                        {
                            faRawData->setTo(tcpPayload.data(), static_cast<int32_t>(receivedLenghtCount));
                        }
                        else
                        {
                            LOG_E("receivedLenghtCount out of range with int32_t, receivedLenghtCount = %u", receivedLenghtCount);
                        }
                        (void)mHandler->obtainMessage(RemoteOTA::MainHandler::CMD_OTA_EXECULTE_REQ, faRawData)->sendToTarget();
                        (void)tcpPayload.clear();
                        payloadSize = 0U;
                        receivedLenghtCount = 0U;
                        isWaitingFragment = false;
                    }
                    else
                    {
                        isWaitingFragment = true;
                        (void)mHandler->obtainMessage(RemoteOTA::MainHandler::CMD_OTA_WAITING_FRAGMENT_DATA)->sendToTarget();
                    }
                }
            }
            else if ((pollClientfds[0].revents & POLLERR) != 0) // An error occurred in the socket
            {
                LOG_E("clientSocket POLLERR: error occurred in the socket");
                closeCurrentClient();
                mShouldListeningData = false;
            }
            else
            {
                LOG_E("unknown POLL events = %hx", pollClientfds[0].revents);
                closeCurrentClient();
                mShouldListeningData = false;
            }
            (void)tcpPayload;
            (void)payloadSize;
            (void)receivedLenghtCount;
            (void)isWaitingFragment;
        }
        mIsListeningDataRunning = false;
        LOG_E("DataListenerThread finished");
    }
}
