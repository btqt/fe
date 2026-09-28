#include "VehicleManagerAdapter.h"

#include <exception>

#include "utils/ProxyIpcServer.h"

#include "../remotediagproxy/include/ProxyIpcProtocol.h"

namespace rdgapp {

VehicleManagerAdapter::VehicleManagerAdapter()
{
    mOdoValue = 0xFFFFFFFFU;
    mOdoUnit = 0x01U;
    
    // Create and register IPC callback handler
    mCallbackHandler = std::make_shared<CallbackHandler>(this);
    mCallbackHandler->initialize();
}

VehicleManagerAdapter::~VehicleManagerAdapter()
{
    mHandler = nullptr;
    if (VehicleManagerAdapter::instance != nullptr)
    {
        VehicleManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<VehicleManagerAdapter> VehicleManagerAdapter::instance{nullptr};
android::Mutex VehicleManagerAdapter::mInstanceLock{};
std::shared_ptr<VehicleManagerAdapter> VehicleManagerAdapter::getInstance()
{
    if (instance == nullptr)
    {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr)
        {
            instance = std::make_shared<VehicleManagerAdapter>();
        }
    }
    return instance;
}

void VehicleManagerAdapter::registerService()
{
    mHandler = RemotediagHandler::getInstance();
}

error_t VehicleManagerAdapter::getOdoInformation(uint32_t &odo_value, uint32_t &odo_unit)
{
    const Mutex::Autolock lock{Mutex::Autolock(mOdoInfo)};
    odo_value = mOdoValue;
    odo_unit = mOdoUnit;
    LOG_V("getOdoInformation, mOdoValue %u, mOdoUnit %u", mOdoValue, mOdoUnit);
    return E_OK;
}

uint32_t VehicleManagerAdapter::getTimeCounter()
{
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::VehicleGetTimeCounter,
                                                      {},
                                                      response,
                                                      2000U))
    {
        LOG_W("VehicleManagerAdapter::getTimeCounter proxy request failed");
        return 0U;
    }

    const std::string payload{rdgipc::toString(response)};
    try
    {
        return static_cast<uint32_t>(std::stoul(payload));
    }
    catch (const std::exception &)
    {
        LOG_W("VehicleManagerAdapter::getTimeCounter invalid payload: %s", payload.c_str());
        return 0U;
    }
}

uint16_t VehicleManagerAdapter::getTripCounter()
{
    std::vector<uint8_t> response{};
    if (!ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::VehicleGetTripCounter,
                                                      {},
                                                      response,
                                                      2000U))
    {
        LOG_W("VehicleManagerAdapter::getTripCounter proxy request failed");
        return 0U;
    }

    const std::string payload{rdgipc::toString(response)};
    uint16_t tripCounter{0U};
    try
    {
        const uint32_t tripCounter_t{static_cast<uint32_t>(std::stoul(payload))};
        if(tripCounter_t <= 65535U)
        {
            tripCounter = static_cast<uint16_t>(tripCounter_t);
        }
        else 
        {
            // do nothing
        }
    }
    catch (const std::exception &)
    {
        LOG_W("VehicleManagerAdapter::getTripCounter invalid payload: %s", payload.c_str());
    }
    return tripCounter;
}
void VehicleManagerAdapter::setTimeoutOdoInfo(const uint32_t odoValue, const uint32_t odoUnit)
{
    const Mutex::Autolock lock{Mutex::Autolock(mOdoInfo)};
    mOdoValue = odoValue;
    mOdoUnit = odoUnit;
    LOG_V("setTimeoutOdoInfo, mOdoValue %u, mOdoUnit %u, odoValue, %u, odoUnit %u", mOdoValue, mOdoUnit, odoValue, odoUnit);
}

void VehicleManagerAdapter::handleOdoSignal(const android::sp<VehicleData> vehicleData)
{
    if (vehicleData != nullptr)
    {
        const uint32_t signalId{vehicleData->sigID};
        if (signalId == 0x0611U) //6-DCF-10-02-24DCM Global CAN Data Specification
        {
            uint32_t odoUnit{0U};
            uint32_t odoValue{0U};
            const android::sp<::Buffer> mbuffer {vehicleData->buffer};
            if ((mbuffer != nullptr) && (mbuffer->data() != nullptr) && (mbuffer->size() == 8U))
            {

                odoUnit = static_cast<uint32_t>((static_cast<uint32_t>(mbuffer->data()[3]) >> 4U) & 0x3U);// 7 6 5 4 3 2 1 0 (0011)
                if ((odoUnit == 0x01U) || (odoUnit == 0x02U)) // 01b --> km, 02b --> mile
                {
                    odoValue = static_cast<uint32_t>(static_cast<uint32_t>(static_cast<uint32_t>(mbuffer->data()[4]) << 24U) |
                                                     static_cast<uint32_t>(static_cast<uint32_t>(mbuffer->data()[5]) << 16U) |
                                                     static_cast<uint32_t>(static_cast<uint32_t>(mbuffer->data()[6]) << 8U) |
                                                     static_cast<uint32_t>(mbuffer->data()[7]));
                }
                else
                {
                    odoUnit = 0x01U;  //undefined value
                    odoValue = 0xFFFFFFFFU; //undefined value
                }
            }
            else
            {
                odoUnit = 0x01U;  //undefined value
                odoValue = 0xFFFFFFFFU; //undefined value
            }
            setTimeoutOdoInfo(odoValue, odoUnit);
        }
    }
}

void VehicleManagerAdapter::onSignalReceived(const uint32_t channel,
                                             const uint32_t sigId,
                                             const std::vector<uint8_t> &bufferBytes)
{
    const android::sp<RemoteWarning> warning{RemoteWarning::getInstance()};
    if (warning == nullptr)
    {
        LOG_W("VehicleManagerAdapter::onSignalReceived warning is nullptr");
        return;
    }

    const android::sp<VehicleData> vehicleData{new VehicleData()};
    if (vehicleData == nullptr)
    {
        return;
    }

    vehicleData->sigID = sigId;
    const android::sp<::Buffer> buffer{new ::Buffer()};
    if ((buffer != nullptr) && !bufferBytes.empty())
    {
        buffer->setTo(bufferBytes.data(), static_cast<int32_t>(bufferBytes.size()));
    }
    vehicleData->buffer = buffer;
    warning->onReceivedCanSignal(channel, vehicleData);
}

void VehicleManagerAdapter::onSignalTimeout(const uint32_t channel,
                                            const uint32_t sigId,
                                            const std::vector<uint8_t> &bufferBytes)
{
    const android::sp<RemoteWarning> warning{RemoteWarning::getInstance()};
    if (warning == nullptr)
    {
        LOG_W("VehicleManagerAdapter::onSignalTimeout warning is nullptr");
        return;
    }

    const android::sp<VehicleData> vehicleData{new VehicleData()};
    if (vehicleData == nullptr)
    {
        return;
    }

    vehicleData->sigID = sigId;
    const android::sp<::Buffer> buffer{new ::Buffer()};
    if ((buffer != nullptr) && !bufferBytes.empty())
    {
        buffer->setTo(bufferBytes.data(), static_cast<int32_t>(bufferBytes.size()));
    }
    vehicleData->buffer = buffer;
    warning->onReceivedCanSignalTimeout(channel, vehicleData);
}

// ============================================================================
// NESTED CallbackHandler IMPLEMENTATION
// ============================================================================

VehicleManagerAdapter::CallbackHandler::CallbackHandler(VehicleManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("VehicleManagerAdapter::CallbackHandler: created");
}

void VehicleManagerAdapter::CallbackHandler::initialize() {
    // Register this handler for Vehicle callbacks
    std::vector<uint32_t> callbackIds = {
        static_cast<uint32_t>(rdgipc::CallbackId::VehicleSignalReceived),
        static_cast<uint32_t>(rdgipc::CallbackId::VehicleSignalTimeout)
    };
    
    rdgapp::ProxyIpcServer& server = rdgapp::ProxyIpcServer::getInstance();
    server.registerCallbackHandler(shared_from_this(), callbackIds);
    LOG_I("VehicleManagerAdapter::CallbackHandler: registered %zu callbacks", callbackIds.size());
}

VehicleManagerAdapter::CallbackHandler::~CallbackHandler() {
    LOG_I("VehicleManagerAdapter::CallbackHandler: destroyed");
}

void VehicleManagerAdapter::CallbackHandler::handle(uint32_t callbackId, const std::vector<uint8_t> &payload) {
    const rdgipc::CallbackId cbId = static_cast<rdgipc::CallbackId>(callbackId);
    
    LOG_I("VehicleManagerAdapter::CallbackHandler: handling callback=%s id=%u payloadSize=%zu",
          rdgipc::callbackName(cbId), callbackId, payload.size());
    
    switch (cbId) {
    case rdgipc::CallbackId::VehicleSignalReceived:
        handleSignalReceived(payload);
        break;
    case rdgipc::CallbackId::VehicleSignalTimeout:
        handleSignalTimeout(payload);
        break;
    default:
        LOG_W("VehicleManagerAdapter::CallbackHandler: unsupported callback id=%u", callbackId);
        break;
    }
}

void VehicleManagerAdapter::CallbackHandler::handleSignalReceived(const std::vector<uint8_t> &payload) {
    if (payload.size() < 5) {
        LOG_W("VehicleManagerAdapter::CallbackHandler: invalid signal received payload size=%zu", payload.size());
        return;
    }
    
    const uint32_t channel = static_cast<uint32_t>(payload[0]);
    const uint32_t sigId = (static_cast<uint32_t>(payload[1]) << 24U) |
                          (static_cast<uint32_t>(payload[2]) << 16U) |
                          (static_cast<uint32_t>(payload[3]) << 8U) |
                           static_cast<uint32_t>(payload[4]);
    
    std::vector<uint8_t> bufferBytes(payload.begin() + 5, payload.end());
    mAdapter->onSignalReceived(channel, sigId, bufferBytes);
}

void VehicleManagerAdapter::CallbackHandler::handleSignalTimeout(const std::vector<uint8_t> &payload) {
    if (payload.size() < 5) {
        LOG_W("VehicleManagerAdapter::CallbackHandler: invalid signal timeout payload size=%zu", payload.size());
        return;
    }
    
    const uint32_t channel = static_cast<uint32_t>(payload[0]);
    const uint32_t sigId = (static_cast<uint32_t>(payload[1]) << 24U) |
                          (static_cast<uint32_t>(payload[2]) << 16U) |
                          (static_cast<uint32_t>(payload[3]) << 8U) |
                           static_cast<uint32_t>(payload[4]);
    
    std::vector<uint8_t> bufferBytes(payload.begin() + 5, payload.end());
    mAdapter->onSignalTimeout(channel, sigId, bufferBytes);
}

}
