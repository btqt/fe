#ifndef REMOTEDIAGPROXY_FILESTOREADAPTER_H
#define REMOTEDIAGPROXY_FILESTOREADAPTER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <utils/Mutex.h>

#include "../include/IpcMessageHandler.h"
#include "../utils/Logger.h"

/* Persists remotediag upload files on the EP partition (/data/rdg/).
 * The paths embedded in GrpcReqData resolve locally for HttpManagerService. */
class FileStoreAdapter
{
public:
    FileStoreAdapter();
    virtual ~FileStoreAdapter() noexcept;
    FileStoreAdapter(const FileStoreAdapter &) = delete;
    FileStoreAdapter &operator=(const FileStoreAdapter &) = delete;
    FileStoreAdapter(FileStoreAdapter &&) = delete;
    FileStoreAdapter &operator=(FileStoreAdapter &&) = delete;
    static std::shared_ptr<FileStoreAdapter> getInstance();

private:
    // Nested command handler for IPC
    class CommandHandler : public rdgipc::ICommandHandler,
                           public std::enable_shared_from_this<CommandHandler> {
    public:
        explicit CommandHandler(FileStoreAdapter* adapter);
        ~CommandHandler() override;
        void initialize();
        rdgipc::CommandResponse handle(uint32_t commandId, const std::vector<uint8_t> &payload) override;
    private:
        FileStoreAdapter* mAdapter;
        rdgipc::CommandResponse handleSave(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleGet(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleRemove(const std::vector<uint8_t> &payload);
        rdgipc::CommandResponse handleExists(const std::vector<uint8_t> &payload);
    };
    std::shared_ptr<CommandHandler> mCommandHandler;

    bool saveFile(const std::string &name, const uint8_t *data, size_t size) const;
    bool loadFile(const std::string &name, std::vector<uint8_t> &data) const;
    bool removeFile(const std::string &name) const;
    bool fileExists(const std::string &name) const;
    static bool isNameValid(const std::string &name);
    static std::string filePath(const std::string &name);

private:
    static std::shared_ptr<FileStoreAdapter> instance;
    static android::Mutex mInstanceLock;
};

#endif // REMOTEDIAGPROXY_FILESTOREADAPTER_H
