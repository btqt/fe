#ifndef DATABASE_H
#define DATABASE_H
#include <string>
#include <memory>
#include <vector>
#include <sqlite3.h>
#include <utils/RefBase.h>
#include <utils/Buffer.h>

#include "ParamsDef.h"
#include "common_def.h"
#include "FileUtil.h"
#include "diagprocess/RoBOccurrence/OccurrentRobNotification.h"

namespace rdgapp {

class Database: public android::RefBase
{
    public:
        Database(const std::string& name
                  , const std::string& filePath
                  , const char_t* const structDefine
                  , const int32_t flags = static_cast<int32_t>(SQLITE_OPEN_READONLY)
                  , const int32_t busyTimeoutMs = 0);

        ~Database() override = default;
        int32_t init();
        int32_t exec(const std::string& statement);
        int32_t clear();

        int32_t insertInt(const std::string& key, const uint32_t value);
        int32_t insertInt64(const std::string& key, const uint64_t value);
        int32_t insertStr(const std::string& key, const std::string& value);
        int32_t insertByteArray(const std::string& key, const uint8_t* const data, const uint32_t size);

        // int32_t retrieveInt64(uint32_t id,const std::string& key, uint64_t value);
        // int32_t retrieveByteArray(uint32_t id,const std::string& key, android::sp<::Buffer> output);

        int32_t getIds(std::vector<int32_t>& idList);

        int32_t saveOccurrentRobNotification(const android::sp<OccurrentRobNotification>& notification);
        int32_t getAllOccurrentRobNotification(OccurrentRobNotificationList& aList);

        int32_t getAllRoBSsrCrcInfo(std::unordered_map<uint64_t, std::shared_ptr<CrcInformation>>& crcList);
        int32_t saveRoBSsrCrcInfo(const uint64_t transId
                            , const uint32_t occurred_rob_ssr_crc
                            , const uint32_t time_series_rob_ssr_crc);
        uint32_t getUploadDB(std::vector<CommonDefine::UploadFileAttribute> &data);
        uint32_t saveUploadDB(const uint32_t prio, const uint32_t uploadfiletype, const uint64_t fileSize, const std::string uploadpatch);
        uint32_t deleteUploadDB(const std::string data);
        static std::string errorToString(const int32_t reason);

        struct Deleter
        {
            void operator()(sqlite3* const apSQLite) const;
        };

    private:
        std::string mName;
        std::string mFilePath;
        std::string mStruct;
        int32_t mFlags;
        int32_t mBusyTimeoutMs;
        std::unique_ptr<sqlite3, Deleter> mSQLitePtr; ///< Pointer to SQLite Database Connection Handle
};
}
#endif // DATABASE_H
