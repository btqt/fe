#include "RemotediagProxyHandler.h"
#include "../include/ParamsDef.h"
#include "../services/ApplicationManagerAdapter.h"
#include "../services/CalibManagerAdapter.h"
#include "../services/DiagManagerAdapter.h"
#include "../services/HttpManagerAdapter.h"
#include "../services/MqttManagerAdapter.h"
#include "../services/SomeipManagerAdapter.h"
#include "../services/PPIManagerAdapter.h"
#include "../services/PowerManagerAdapter.h"
#include "../services/VehicleManagerAdapter.h"
#include "ProxyIpcClient.h"

namespace {
std::string encodeHex(const uint8_t *data, const size_t size)
{
    static const char *kHex{"0123456789ABCDEF"};
    std::string out{};
    out.reserve(size * 2U);
    for (size_t i{0U}; i < size; ++i)
    {
        const uint8_t value{data[i]};
        out.push_back(kHex[(value >> 4U) & 0x0FU]);
        out.push_back(kHex[value & 0x0FU]);
    }
    return out;
}

std::vector<uint8_t> encodePostPayload(const android::sp<::Post> &post)
{
    if (post == nullptr)
    {
        return std::vector<uint8_t>{};
    }

    const std::string result{std::to_string(post->arg1) + "," + encodeHex(post->buffer.data(), static_cast<size_t>(post->buffer.size()))};
    return std::vector<uint8_t>(result.begin(), result.end());
}

android::sp<::Post> getPost(const android::sp<sl::Message> &msg)
{
    android::sp<::Post> post{nullptr};
    try
    {
        msg->getObject(post);
    }
    catch (...)
    {
        LOG_E("Failed to get ApplicationManager callback post");
    }
    return post;
}
} // namespace

android::sp<RemotediagProxyHandler> RemotediagProxyHandler::mRemotediagProxyHandlerInstance {nullptr};
RemotediagProxyHandler::RemotediagProxyHandler(sp<sl::SLLooper> &looper) noexcept : Handler(looper)
{
    mRemotediagProxyHandlerInstance = this;
}

void RemotediagProxyHandler::handleMessage(const android::sp<sl::Message> &handlemsg)
{
    const int32_t what = handlemsg->what;
    switch (what)
    {
        case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_APPLICATION_MGR:
        {
            LOG_I("MSG_REGISTER_APPLICATION_MGR");
            ApplicationManagerAdapter::getInstance()->registerService();
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_POWER_MODE_MGR:
        {
            LOG_I("MSG_REGISTER_POWER_MODE_MGR");
            PowerManagerAdapter::getInstance()->registerService();
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_HTTP_MGR:
        {
            LOG_I("MSG_REGISTER_HTTP_MGR");
            HttpManagerAdapter::getInstance()->registerService();
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_PPI_MGR:
        {
            LOG_I("MSG_REGISTER_PPI_MGR");
            PPIManagerAdapter::getInstance()->registerService();
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_VEHICLE_MGR:
        {
            LOG_I("MSG_REGISTER_VEHICLE_MGR");
            VehicleManagerAdapter::getInstance()->registerService();
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_MQTT_MGR:
        {
            LOG_I("MSG_REGISTER_MQTT_MGR");
            MqttManagerAdapter::getInstance()->registerService();
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_SOMEIP_MGR:
        {
            LOG_I("MSG_REGISTER_SOMEIP_MGR");
            SomeipManagerAdapter::getInstance()->registerService();
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_DIAG_MGR:
        {
            LOG_I("MSG_REGISTER_DIAG_MGR");
            DiagManagerAdapter::getInstance()->registerService();
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_REGISTER_CALIB_MGR:
        {
            LOG_I("MSG_REGISTER_CALIB_MGR");
            CalibManagerAdapter::getInstance()->registerService();
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_BOOT_COMPLETED:
        {
            LOG_I("MSG_APPL_ON_BOOT_COMPLETED");
            if (!ProxyIpcClient::sendCallback(static_cast<uint32_t>(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_BOOT_COMPLETED)))
            {
                // Retry until IPC link is ready so app always receives boot-completed callback.
                (void)sendMessageDelayed(obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_BOOT_COMPLETED),
                                         static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
            }
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_STATUS_CHANGED:
        {
            LOG_I("MSG_APPL_ON_FEATURE_STATUS_CHANGED");
            const android::sp<::Post> post{getPost(handlemsg)};
            ProxyIpcClient::sendCallback(static_cast<uint32_t>(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_STATUS_CHANGED),
                            encodePostPayload(post));
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_ACTION_DELIVERED:
        {
            LOG_I("MSG_APPL_ON_FEATURE_ACTION_DELIVERED");
            const android::sp<::Post> post{getPost(handlemsg)};
            ProxyIpcClient::sendCallback(static_cast<uint32_t>(HANDLE_MESSAGE_REQUEST::MSG_APPL_ON_FEATURE_ACTION_DELIVERED),
                            encodePostPayload(post));
            break;
        }
        case HANDLE_MESSAGE_REQUEST::MSG_APPL_POST_APP_STATUS_CHANGED:
        {
            LOG_I("MSG_APPL_POST_APP_STATUS_CHANGED");
            const android::sp<::Post> post{getPost(handlemsg)};
            ProxyIpcClient::sendCallback(static_cast<uint32_t>(HANDLE_MESSAGE_REQUEST::MSG_APPL_POST_APP_STATUS_CHANGED),
                            encodePostPayload(post));
            break;
        }
        default:
            // TODO: add event mapping for other service callbacks when adapters are integrated.
            LOG_W("Unhandled RemotediagProxyHandler message: what=%d", what);
            
            break;
    }
}

android::sp<RemotediagProxyHandler> RemotediagProxyHandler::getInstance()
{
    if (mRemotediagProxyHandlerInstance == nullptr)
    {
        LOG_E("RemotediagProxyHandler instance is nullptr");
    }
    return mRemotediagProxyHandlerInstance;
}
