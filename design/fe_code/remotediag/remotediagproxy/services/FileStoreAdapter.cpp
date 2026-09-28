#include "FileStoreAdapter.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/ParamsDef.h"
#include "../include/IpcTransport.h"
#include "../include/ProxyIpcProtocol.h"
#include "../utils/ProxyIpcClient.h"

namespace {
constexpr size_t kMaxFileNameLength{255U};
}

std::shared_ptr<FileStoreAdapter> FileStoreAdapter::instance{nullptr};
android::Mutex FileStoreAdapter::mInstanceLock{};

FileStoreAdapter::FileStoreAdapter() {
    /* Best effort: the platform normally provisions the upload directory */
    if ((::mkdir(UPLOAD_PATH.c_str(), 0700) != 0) && (errno != EEXIST)) {
        LOG_W("FileStoreAdapter: cannot create %s: %s", UPLOAD_PATH.c_str(), std::strerror(errno));
    }

    mCommandHandler = std::make_shared<CommandHandler>(this);
    mCommandHandler->initialize();
}

FileStoreAdapter::~FileStoreAdapter() noexcept {
    if (FileStoreAdapter::instance != nullptr) {
        FileStoreAdapter::instance = nullptr;
    }
}

std::shared_ptr<FileStoreAdapter> FileStoreAdapter::getInstance() {
    if (instance == nullptr) {
        const android::AutoMutex _l{mInstanceLock};
        if (instance == nullptr) {
            instance = std::make_shared<FileStoreAdapter>();
        }
    }
    return instance;
}

bool FileStoreAdapter::isNameValid(const std::string &name) {
    if (name.empty() || (name.size() > kMaxFileNameLength)) {
        return false;
    }
    if ((name.find('/') != std::string::npos) || (name.find("..") != std::string::npos)) {
        return false;
    }
    return true;
}

std::string FileStoreAdapter::filePath(const std::string &name) {
    return std::string(UPLOAD_PATH) + name;
}

bool FileStoreAdapter::saveFile(const std::string &name, const uint8_t *data, size_t size) const {
    const std::string path{filePath(name)};
    const std::string tmpPath{path + ".tmp"};

    const int32_t fd{::open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600)};
    if (fd < 0) {
        LOG_E("FileStoreAdapter: cannot open %s: %s", tmpPath.c_str(), std::strerror(errno));
        return false;
    }

    bool written{true};
    size_t offset{0U};
    while (offset < size) {
        const ssize_t n{::write(fd, &data[offset], size - offset)};
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_E("FileStoreAdapter: write failed for %s: %s", tmpPath.c_str(), std::strerror(errno));
            written = false;
            break;
        }
        offset += static_cast<size_t>(n);
    }

    if (written && (::fsync(fd) != 0)) {
        LOG_E("FileStoreAdapter: fsync failed for %s: %s", tmpPath.c_str(), std::strerror(errno));
        written = false;
    }
    (void)::close(fd);

    if (!written) {
        (void)::unlink(tmpPath.c_str());
        return false;
    }

    if (::rename(tmpPath.c_str(), path.c_str()) != 0) {
        LOG_E("FileStoreAdapter: rename %s -> %s failed: %s", tmpPath.c_str(), path.c_str(), std::strerror(errno));
        (void)::unlink(tmpPath.c_str());
        return false;
    }

    LOG_I("FileStoreAdapter: saved %s (%zu bytes)", path.c_str(), size);
    return true;
}

bool FileStoreAdapter::loadFile(const std::string &name, std::vector<uint8_t> &data) const {
    data.clear();
    const std::string path{filePath(name)};

    const int32_t fd{::open(path.c_str(), O_RDONLY | O_CLOEXEC)};
    if (fd < 0) {
        LOG_E("FileStoreAdapter: cannot open %s: %s", path.c_str(), std::strerror(errno));
        return false;
    }

    struct stat st{};
    if ((::fstat(fd, &st) != 0) || (st.st_size < 0) ||
        (static_cast<size_t>(st.st_size) > rdgipc::IpcTransport::kMaxPayloadSize)) {
        LOG_E("FileStoreAdapter: invalid size for %s", path.c_str());
        (void)::close(fd);
        return false;
    }

    data.resize(static_cast<size_t>(st.st_size));
    bool readOk{true};
    size_t offset{0U};
    while (offset < data.size()) {
        const ssize_t n{::read(fd, &data[offset], data.size() - offset)};
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_E("FileStoreAdapter: read failed for %s: %s", path.c_str(), std::strerror(errno));
            readOk = false;
            break;
        }
        if (n == 0) {
            LOG_E("FileStoreAdapter: unexpected EOF for %s", path.c_str());
            readOk = false;
            break;
        }
        offset += static_cast<size_t>(n);
    }
    (void)::close(fd);

    if (!readOk) {
        data.clear();
    }
    return readOk;
}

bool FileStoreAdapter::removeFile(const std::string &name) const {
    const std::string path{filePath(name)};
    if (!fileExists(name)) {
        LOG_W("FileStoreAdapter: remove requested but %s does not exist", path.c_str());
        return false;
    }
    if (::unlink(path.c_str()) != 0) {
        LOG_E("FileStoreAdapter: unlink %s failed: %s", path.c_str(), std::strerror(errno));
        return false;
    }
    LOG_I("FileStoreAdapter: removed %s", path.c_str());
    return true;
}

bool FileStoreAdapter::fileExists(const std::string &name) const {
    struct stat st{};
    return ::stat(filePath(name).c_str(), &st) == 0;
}

// ============================================================================
// NESTED CommandHandler IMPLEMENTATION
// ============================================================================

FileStoreAdapter::CommandHandler::CommandHandler(FileStoreAdapter* adapter)
    : mAdapter(adapter) {
    LOG_I("FileStoreAdapter::CommandHandler: created");
}

FileStoreAdapter::CommandHandler::~CommandHandler() {
    LOG_I("FileStoreAdapter::CommandHandler: destroyed");
}

void FileStoreAdapter::CommandHandler::initialize() {
    std::vector<uint32_t> commandIds = {
        static_cast<uint32_t>(rdgipc::CommandId::FileSaveUpload),
        static_cast<uint32_t>(rdgipc::CommandId::FileGetUpload),
        static_cast<uint32_t>(rdgipc::CommandId::FileRemoveUpload),
        static_cast<uint32_t>(rdgipc::CommandId::FileExistsUpload)
    };

    ProxyIpcClient* client = ProxyIpcClient::getInstance();
    if (client != nullptr) {
        client->registerCommandHandler(shared_from_this(), commandIds);
        LOG_I("FileStoreAdapter::CommandHandler: registered %zu commands", commandIds.size());
    } else {
        LOG_E("FileStoreAdapter::CommandHandler: ProxyIpcClient not initialized");
    }
}

rdgipc::CommandResponse FileStoreAdapter::CommandHandler::handle(uint32_t commandId, const std::vector<uint8_t> &payload) {
    LOG_I("FileStoreAdapter::CommandHandler: handling command id=%u payloadSize=%zu",
          commandId, payload.size());

    switch (static_cast<rdgipc::CommandId>(commandId)) {
    case rdgipc::CommandId::FileSaveUpload:
        return handleSave(payload);
    case rdgipc::CommandId::FileGetUpload:
        return handleGet(payload);
    case rdgipc::CommandId::FileRemoveUpload:
        return handleRemove(payload);
    case rdgipc::CommandId::FileExistsUpload:
        return handleExists(payload);
    default:
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::UnsupportedCommand);
    }
}

rdgipc::CommandResponse FileStoreAdapter::CommandHandler::handleSave(const std::vector<uint8_t> &payload) {
    /* Frame: uint32 LE name length + name bytes + file bytes */
    if (payload.size() < 4U) {
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::InvalidPayload);
    }
    const uint32_t nameLen{static_cast<uint32_t>(payload[0]) |
                           (static_cast<uint32_t>(payload[1]) << 8U) |
                           (static_cast<uint32_t>(payload[2]) << 16U) |
                           (static_cast<uint32_t>(payload[3]) << 24U)};
    if ((nameLen == 0U) || ((4U + static_cast<size_t>(nameLen)) > payload.size())) {
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::InvalidPayload);
    }

    const std::string name{reinterpret_cast<const char*>(&payload[4]), nameLen};
    if (!isNameValid(name)) {
        LOG_E("FileStoreAdapter: invalid file name in save request");
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::InvalidPayload);
    }

    const size_t dataOffset{4U + static_cast<size_t>(nameLen)};
    const size_t dataSize{payload.size() - dataOffset};
    const uint8_t *data{(dataSize > 0U) ? &payload[dataOffset] : nullptr};

    if (!mAdapter->saveFile(name, data, dataSize)) {
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::OperationFailed);
    }
    return rdgipc::CommandResponse::ok();
}

rdgipc::CommandResponse FileStoreAdapter::CommandHandler::handleGet(const std::vector<uint8_t> &payload) {
    const std::string name{payload.begin(), payload.end()};
    if (!isNameValid(name)) {
        LOG_E("FileStoreAdapter: invalid file name in get request");
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::InvalidPayload);
    }

    std::vector<uint8_t> data{};
    if (!mAdapter->loadFile(name, data)) {
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::OperationFailed);
    }
    return rdgipc::CommandResponse::ok(data);
}

rdgipc::CommandResponse FileStoreAdapter::CommandHandler::handleRemove(const std::vector<uint8_t> &payload) {
    const std::string name{payload.begin(), payload.end()};
    if (!isNameValid(name)) {
        LOG_E("FileStoreAdapter: invalid file name in remove request");
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::InvalidPayload);
    }

    if (!mAdapter->removeFile(name)) {
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::OperationFailed);
    }
    return rdgipc::CommandResponse::ok();
}

rdgipc::CommandResponse FileStoreAdapter::CommandHandler::handleExists(const std::vector<uint8_t> &payload) {
    const std::string name{payload.begin(), payload.end()};
    if (!isNameValid(name)) {
        LOG_E("FileStoreAdapter: invalid file name in exists request");
        return rdgipc::CommandResponse::err(rdgipc::ErrorCode::InvalidPayload);
    }

    return rdgipc::CommandResponse::ok(mAdapter->fileExists(name) ? std::string{"1"} : std::string{"0"});
}
