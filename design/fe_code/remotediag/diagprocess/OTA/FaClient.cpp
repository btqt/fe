#include "ParamsDef.h"
#include "FaClient.h"
#include <arpa/inet.h>

namespace rdgapp {

FaClient::FaClient(void) noexcept
{
    mSocket = 0;
}

void FaClient::startup()
{
    LOG_I("startup");
    struct sockaddr_in faServer;
    (void)memset(&faServer, 0, sizeof(faServer));
    faServer.sin_family = static_cast<uint16_t>(AF_INET);
    faServer.sin_port = htons(ParamsDef::FA_SERVER_DEFAULT_PORT);
    if ( mSocket <= 0)
    {
        mSocket = socket(AF_INET, static_cast<int32_t>((static_cast<uint32_t>(SOCK_STREAM)) | (static_cast<uint32_t>(SOCK_NONBLOCK))), 0);
        if (mSocket < 0) 
        {
            LOG_E("Failed to create a TCP socket");
            goto exit;
        }
        int32_t ret {-1};
        ret = inet_pton(AF_INET, static_cast<char_t const *>(ParamsDef::FA_SERVER_LOCAL_HOST), &(faServer.sin_addr));
        if (ret <= 0)
        {
            LOG_E("Invalid address/ Address not supported");
            goto exit;
        }
        
        ret = connect(mSocket, (struct sockaddr*)&faServer, sizeof(faServer));
        if ((ret < 0) || (mSocket <= 0))
        {
            LOG_E("Connect to FA server Failed, error = %d", ret);
        } else {
            LOG_I("Connect to FA server success");
        }
    }
exit:
    return;
}

android::sp<OtaMessage> FaClient::sendData(const android::sp<::Buffer> fadata)
{
    const android::sp<::Buffer> resBuff {new ::Buffer()};
    const android::sp<OtaMessage> otaReqMessage {new OtaMessage()};
    if ((mSocket > 0) && (fadata->size() > 0U))
    {
        ::Buffer tmp{};
        LOG_I("Send %d bytes to FA server", fadata->size());
        (void)send(mSocket, fadata->data(), fadata->size(), 0);

        (void)sleep(5U);

        tmp.setSize(static_cast<int32_t>(OtaMessageDefs::FA_PROTOCOL_MAX_DATA));
        const ssize_t resBytes {read(mSocket, tmp.data(), OtaMessageDefs::FA_PROTOCOL_MAX_DATA)};
        if (resBytes > 0)
        {
            LOG_I("Read response data, lenght = %d", resBytes);
            resBuff->setSize(static_cast<int32_t>(resBytes));
            resBuff->setTo(tmp.data(), resBytes);
            LOG_D("Raw data: ");
            for(uint16_t i {0U}; i < static_cast<uint16_t>(resBytes); i++) {
                LOG_D("0x%02X ", *(tmp.data() + i));
            }
            (void)otaReqMessage->Parser(resBuff);
        }
    } else {
        LOG_E("Send data failed, socket fd invalid");
    }
    return otaReqMessage;
}

void FaClient::stop()
{
    (void)close(mSocket);
    mSocket = 0;
}
}
