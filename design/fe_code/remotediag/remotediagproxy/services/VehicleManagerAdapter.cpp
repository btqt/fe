#include "VehicleManagerAdapter.h"

#include <cstring>
#include <unistd.h>

#include "../utils/ProxyIpcClient.h"

namespace {
void writeUint32(uint8_t *data, const uint32_t value) {
	data[0] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
	data[1] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
	data[2] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
	data[3] = static_cast<uint8_t>(value & 0xFFU);
}

std::vector<uint8_t> encodeVehicleSignalPayload(const uint32_t channel,
												 const android::sp<VehicleData> &vehicleData) {
	if (vehicleData == nullptr) {
		return std::vector<uint8_t>{};
	}

	const android::sp<::Buffer> buffer{vehicleData->buffer};
	size_t bufferSize{0U};
	if (buffer != nullptr) {
		bufferSize = static_cast<size_t>(buffer->size());
	}

	std::vector<uint8_t> payload(5U + bufferSize);
	payload[0] = static_cast<uint8_t>(channel & 0xFFU);
	writeUint32(&payload[1], vehicleData->sigID);

	if ((buffer != nullptr) && (bufferSize > 0U) && (buffer->data() != nullptr)) {
		std::memcpy(&payload[5], buffer->data(), bufferSize);
	}

	return payload;
}
} // namespace

std::shared_ptr<VehicleManagerAdapter> VehicleManagerAdapter::instance{nullptr};
android::Mutex VehicleManagerAdapter::mInstanceLock{};

VehicleManagerAdapter::VehicleManagerAdapter() {
	mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who) {
		this->onBinderDied(who);
	});
	
	// Create and register IPC command handler
	mCommandHandler = std::make_shared<CommandHandler>(this);
	mCommandHandler->initialize();
}

VehicleManagerAdapter::~VehicleManagerAdapter() noexcept {
	if (VehicleManagerAdapter::instance != nullptr) {
		VehicleManagerAdapter::instance = nullptr;
	}
}

std::shared_ptr<VehicleManagerAdapter> VehicleManagerAdapter::getInstance() {
	if (instance == nullptr) {
		const android::AutoMutex _l{mInstanceLock};
		if (instance == nullptr) {
			instance = std::make_shared<VehicleManagerAdapter>();
		}
	}
	return instance;
}

void VehicleManagerAdapter::registerService() {
	mHandler = RemotediagProxyHandler::getInstance();
	if (!registerServiceLocked()) {
		LOG_E("Cannot register VCM Service, try again after 500ms");
		if (mHandler != nullptr) {
			(void)mHandler->sendMessageDelayed(
				mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_VEHICLE_MGR),
				static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
		}
	}
}

bool VehicleManagerAdapter::registerServiceLocked() {
	android::sp<ICommunicationManagerService> service{getService()};
	if (service == nullptr) {
		LOG_W("VehicleManagerAdapter: CommunicationManagerService not available");
		return false;
	}

	if (android::IInterface::asBinder(service)->linkToDeath(mServiceDeathRecipient) != android::OK) {
		LOG_E("VehicleManagerAdapter: linkToDeath failed");
		return false;
	}

	if (mVehicleReceiver == nullptr) {
		mVehicleReceiver = android::sp<IVehicleReceiver>{new VCMReceiver(*this)};
	}

	const uint32_t moduleId{static_cast<uint32_t>(getpid())};
	if (service->registerVehicleReceiver(moduleId, mVehicleReceiver) != android::OK) {
		LOG_E("VehicleManagerAdapter: registerVehicleReceiver failed");
		return false;
	}

	LOG_I("Registed Vehicle Manager Service with PID: %d", moduleId);
	if (service->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S02) == android::OK) {
		LOG_I("Registed MsgInd_CanRx_MET1S02");
	}
	if (service->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S34) == android::OK) {
		LOG_I("Registed MsgInd_CanRx_MET1S34");
	}
	if (service->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S35) == android::OK) {
		LOG_I("Registed MsgInd_CanRx_MET1S35");
	}
	if (service->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S36) == android::OK) {
		LOG_I("Registed MsgInd_CanRx_MET1S36");
	}
	if (service->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S37) == android::OK) {
		LOG_I("Registed MsgInd_CanRx_MET1S37");
	}
	if (service->addFrameIndex(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_ENG1G90) == android::OK) {
		LOG_I("Registed MsgInd_CanRx_ENG1G90");
	}

	if (service->addFrameIndexForTimeout(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S02) == android::OK) {
		LOG_I("Registed MsgInd_CanRx_MET1S02");
	}
	if (service->addFrameIndexForTimeout(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S34) == android::OK) {
		LOG_I("Registed Timeout MsgInd_CanRx_MET1S34");
	}
	if (service->addFrameIndexForTimeout(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S35) == android::OK) {
		LOG_I("Registed Timeout MsgInd_CanRx_MET1S35");
	}
	if (service->addFrameIndexForTimeout(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S36) == android::OK) {
		LOG_I("Registed Timeout MsgInd_CanRx_MET1S36");
	}
	if (service->addFrameIndexForTimeout(moduleId, TOYOTA_24DCM::MsgInd_Rx::MsgInd_CanRx_MET1S37) == android::OK) {
		LOG_I("Registed Timeout MsgInd_CanRx_MET1S37");
	}

	{
		const android::Mutex::Autolock lock{mServiceLock};
		mVCMService = service;
	}

	return true;
}

uint32_t VehicleManagerAdapter::getTimeCounter() {
	uint32_t timeCounter{0U};
	android::sp<VehicleData> vehicleData{new VehicleData()};

	android::sp<ICommunicationManagerService> service{nullptr};
	{
		const android::Mutex::Autolock lock{mServiceLock};
		service = mVCMService;
	}

	if (service != nullptr) {
		vehicleData = service->getVehicleValue(TOYOTA_24DCM::SigInd_CanRx_TIME_CNT);
	}

	if ((vehicleData != nullptr) && (vehicleData->valType == TYPE_UINT32)) {
		timeCounter = vehicleData->valUint32;
	}

	LOG_V("valType %d, timeCounter: %u", (vehicleData != nullptr) ? vehicleData->valType : -1, timeCounter);
	return timeCounter;
}

uint16_t VehicleManagerAdapter::getTripCounter() {
	uint16_t tripCounter{0U};
	android::sp<VehicleData> vehicleData{new VehicleData()};

	android::sp<ICommunicationManagerService> service{nullptr};
	{
		const android::Mutex::Autolock lock{mServiceLock};
		service = mVCMService;
	}

	if (service != nullptr) {
		vehicleData = service->getVehicleValue(TOYOTA_24DCM::SigInd_CanRx_TRIP_CNT);
	}

	if ((vehicleData != nullptr) && (vehicleData->valType == TYPE_UINT32)) {
		const uint32_t tripCounterTemp{vehicleData->valUint32};
		if (tripCounterTemp <= 65535U) {
			tripCounter = static_cast<uint16_t>(tripCounterTemp);
		}
	}

	LOG_V("valType %d, tripCounter: %u", (vehicleData != nullptr) ? vehicleData->valType : -1, tripCounter);
	return tripCounter;
}

void VehicleManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who) {
	NOTUSED(who);
	LOG_I("VehicleManagerAdapter::onBinderDied");
	{
		const android::Mutex::Autolock lock{mServiceLock};
		mVCMService = nullptr;
		mVehicleReceiver = nullptr;
	}

	if (mHandler != nullptr) {
		(void)mHandler->sendMessageDelayed(
			mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_VEHICLE_MGR),
			static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
	}
}

void VehicleManagerAdapter::onReceived(const uint32_t channel,
									   const android::sp<VehicleData> &vehicleData) {
	if ((channel != data_channel::CAN) && (channel != data_channel::ETHERNET)) {
		return;
	}

	const std::vector<uint8_t> payload{encodeVehicleSignalPayload(channel, vehicleData)};
	if (payload.empty()) {
		return;
	}

	(void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::VehicleSignalReceived), payload);
}

void VehicleManagerAdapter::onReceiveTimeout(const uint32_t channel,
											 const android::sp<VehicleData> &vehicleData) {
	if ((channel != data_channel::CAN) && (channel != data_channel::ETHERNET)) {
		return;
	}

	const std::vector<uint8_t> payload{encodeVehicleSignalPayload(channel, vehicleData)};
	if (payload.empty()) {
		return;
	}

	(void)ProxyIpcClient::sendCallback(static_cast<uint32_t>(rdgipc::CallbackId::VehicleSignalTimeout), payload);
}

android::sp<ICommunicationManagerService> VehicleManagerAdapter::getService() const {
	return android::interface_cast<ICommunicationManagerService>(
		android::defaultServiceManager()->getService(android::String16("service_layer.CommunicationManagerService")));
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

VehicleManagerAdapter::CommandHandler::CommandHandler(VehicleManagerAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("VehicleManagerAdapter::CommandHandler: created");
}

void VehicleManagerAdapter::CommandHandler::initialize() {
    LOG_I("VehicleManagerAdapter::CommandHandler: registering with ProxyIpcClient");
    
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::VehicleGetTimeCounter),
        static_cast<uint32_t>(rdgipc::CommandId::VehicleGetTripCounter)
    };
    
    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("VehicleManagerAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("VehicleManagerAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

VehicleManagerAdapter::CommandHandler::~CommandHandler() {
    LOG_I("VehicleManagerAdapter::CommandHandler: destroyed");
}

rdgipc::CommandResponse VehicleManagerAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    const rdgipc::CommandId cmdId = static_cast<rdgipc::CommandId>(commandId);
    
    LOG_I("VehicleManagerAdapter::CommandHandler: handling command=%s id=%u payloadSize=%zu",
          rdgipc::commandName(cmdId), commandId, payload.size());
    
    switch (cmdId) {
    case rdgipc::CommandId::VehicleGetTimeCounter:
        return handleGetTimeCounter(payload);
    case rdgipc::CommandId::VehicleGetTripCounter:
        return handleGetTripCounter(payload);
    default:
        LOG_W("VehicleManagerAdapter::CommandHandler: unsupported command id=%u", commandId);
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse VehicleManagerAdapter::CommandHandler::handleGetTimeCounter(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    const uint32_t timeCounter = mAdapter->getTimeCounter();
    return rdgipc::CommandResponse::ok(timeCounter);
}

rdgipc::CommandResponse VehicleManagerAdapter::CommandHandler::handleGetTripCounter(const std::vector<uint8_t> &payload) {
    NOTUSED(payload);
    const uint16_t tripCounter = mAdapter->getTripCounter();
    return rdgipc::CommandResponse::ok(tripCounter);
}
