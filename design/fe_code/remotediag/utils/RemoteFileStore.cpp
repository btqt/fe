#include "RemoteFileStore.h"

#include <cstring>

#include "Logger.h"
#include "ProxyIpcServer.h"
#include "../remotediagproxy/include/ProxyIpcProtocol.h"

namespace rdgapp {

namespace {

constexpr uint32_t kFileTransferTimeoutMs{10000U};
constexpr uint32_t kFileQueryTimeoutMs{3000U};

std::vector<uint8_t> toPayload(const std::string &name) {
    return std::vector<uint8_t>(name.begin(), name.end());
}

}

bool RemoteFileStore::saveFile(const std::string& name, const uint8_t* const data, const uint32_t size) noexcept {
    bool result{false};
    if (name.empty() || ((data == nullptr) && (size > 0U))) {
        LOG_E("RemoteFileStore::saveFile invalid arguments for %s", name.c_str());
    } else {
        /* Frame: uint32 LE name length + name bytes + file bytes */
        std::vector<uint8_t> payload{};
        payload.reserve(4U + name.size() + size);
        const uint32_t nameLen{static_cast<uint32_t>(name.size())};
        payload.push_back(static_cast<uint8_t>(nameLen & 0xFFU));
        payload.push_back(static_cast<uint8_t>((nameLen >> 8U) & 0xFFU));
        payload.push_back(static_cast<uint8_t>((nameLen >> 16U) & 0xFFU));
        payload.push_back(static_cast<uint8_t>((nameLen >> 24U) & 0xFFU));
        (void)payload.insert(payload.end(), name.begin(), name.end());
        if (size > 0U) {
            (void)payload.insert(payload.end(), &data[0], &data[size]);
        }

        std::vector<uint8_t> response{};
        if (ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::FileSaveUpload, payload, response, kFileTransferTimeoutMs)) {
            result = true;
        } else {
            LOG_E("RemoteFileStore::saveFile proxy request failed for %s", name.c_str());
        }
    }
    return result;
}

bool RemoteFileStore::loadFile(const std::string& name, std::vector<uint8_t>& data) noexcept {
    bool result{false};
    data.clear();
    if (name.empty()) {
        LOG_E("RemoteFileStore::loadFile empty file name");
    } else {
        if (ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::FileGetUpload, toPayload(name), data, kFileTransferTimeoutMs)) {
            result = true;
        } else {
            LOG_E("RemoteFileStore::loadFile proxy request failed for %s", name.c_str());
            data.clear();
        }
    }
    return result;
}

bool RemoteFileStore::removeFile(const std::string& name) noexcept {
    bool result{false};
    if (name.empty()) {
        LOG_E("RemoteFileStore::removeFile empty file name");
    } else {
        std::vector<uint8_t> response{};
        if (ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::FileRemoveUpload, toPayload(name), response, kFileQueryTimeoutMs)) {
            result = true;
        } else {
            LOG_E("RemoteFileStore::removeFile proxy request failed for %s", name.c_str());
        }
    }
    return result;
}

bool RemoteFileStore::exists(const std::string& name) noexcept {
    bool result{false};
    if (name.empty()) {
        LOG_E("RemoteFileStore::exists empty file name");
    } else {
        std::vector<uint8_t> response{};
        if (ProxyIpcServer::getInstance().requestAPICall(rdgipc::CommandId::FileExistsUpload, toPayload(name), response, kFileQueryTimeoutMs)) {
            result = (!response.empty()) && (response[0] == static_cast<uint8_t>('1'));
        } else {
            LOG_E("RemoteFileStore::exists proxy request failed for %s", name.c_str());
        }
    }
    return result;
}

}
