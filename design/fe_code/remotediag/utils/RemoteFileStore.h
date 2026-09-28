#ifndef REMOTEDIAG_REMOTE_FILE_STORE_H
#define REMOTEDIAG_REMOTE_FILE_STORE_H

#include <cstdint>
#include <string>
#include <vector>

namespace rdgapp {

/* Upload files are persisted on the EP partition by remotediagproxy;
 * every access goes through the proxy IPC using the relative file name. */
class RemoteFileStore {
    public:
        static bool saveFile(const std::string& name, const uint8_t* const data, const uint32_t size) noexcept;
        static bool loadFile(const std::string& name, std::vector<uint8_t>& data) noexcept;
        static bool removeFile(const std::string& name) noexcept;
        static bool exists(const std::string& name) noexcept;
};
}
#endif // REMOTEDIAG_REMOTE_FILE_STORE_H
