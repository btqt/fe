#include <Error.h>
#include <string>
#include <memory>
#include <sqlite3.h>
#include <utils/RefBase.h>
#include <utils/Buffer.h>

#include "Database.h"
#include "Logger.h"
#include "diagprocess/RoBOccurrence/OccurrentRobNotification.h"

namespace rdgapp {

Database::Database(const std::string& name,
                   const std::string& filePath,
                   const char_t* const structDefine,
                   const int32_t flags,
                   const int32_t busyTimeoutMs)
    : android::RefBase()
    , mName(name)
    , mFilePath(filePath + ".db")
    , mStruct(structDefine)
    , mFlags(flags)
    , mBusyTimeoutMs(busyTimeoutMs)
{
    mStruct = "CREATE TABLE IF NOT EXISTS " + mName + structDefine;
}

int32_t Database::init()
{
    sqlite3* handle {nullptr};
    int32_t ret{sqlite3_open_v2(mFilePath.c_str(), &handle, mFlags, nullptr)};
    if ( ret != SQLITE_OK)
    {
        LOG_E("sqlite3_open_v2 error, %s", std::string(sqlite3_errstr(ret)).c_str());
        goto exit;
    }
    mSQLitePtr.reset(handle);
    ret = exec(mStruct);
    if ( ret != SQLITE_OK)
    {
        LOG_E("sqlite3_open_v2 error, %s", std::string(sqlite3_errstr(ret)).c_str());
        goto exit;
    }
    if (mBusyTimeoutMs > 0)
    {
        const int32_t result{sqlite3_busy_timeout(handle, mBusyTimeoutMs)};
        if(result != SQLITE_OK){
            LOG_E("sqlite3_busy_timeout failed");
        }
    } else {
        // do nothing
    }

exit:
    return ret;
}

void Database::Deleter::operator() (sqlite3* const apSQLite) const
{
    (void)sqlite3_close(apSQLite);
}

int32_t Database::clear()
{
    //char *errMsg = nullptr;
    const std::string cmd{"DELETE FROM " + mName + ";"};
    sqlite3_stmt* stmt;
    int32_t ret{sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr)};
    if (ret != SQLITE_OK)
    {
        LOG_E("clear failed, reason = %s", sqlite3_errmsg(mSQLitePtr.get()));
        goto exit;
    }
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        LOG_E("clear failed, reason = %s", sqlite3_errmsg(mSQLitePtr.get()));
    }
exit:
    return ret;
}

int32_t Database::exec(const std::string& statement)
{
    char_t *errMsg{nullptr};
    const int32_t ret{sqlite3_exec(mSQLitePtr.get(), statement.c_str(), nullptr, nullptr, &errMsg)};
    if (ret == SQLITE_OK ) {
        sqlite3_free(errMsg);
    } else {
        LOG_E("Executing sqlite3_exec failed, error = %s", errMsg);
        sqlite3_free(errMsg);
    }
    return ret;
}

int32_t Database::insertInt(const std::string& key, const uint32_t value)
{
    std::string cmd{};
    cmd = "INSERT INTO ";
    cmd += mName + " (" + key + ")";
    cmd += " VALUES (" + std::to_string(value)  + ");" ;
    return exec(cmd);
}
int32_t Database::insertInt64(const std::string& key, const uint64_t value)
{
    std::string cmd{};
    cmd = "INSERT INTO ";
    cmd += mName + " (" + key + ")";
    cmd += " VALUES (" + std::to_string(value) + ");" ;
    return exec(cmd);
}
int32_t Database::insertStr(const std::string& key, const std::string& value)
{
    std::string cmd{};
    cmd = "INSERT INTO ";
    cmd += mName + " (" + key + ")";
    cmd += " VALUES (" + value + ");" ;
    return exec(cmd);
}


int32_t Database::insertByteArray(const std::string& key, const uint8_t* const data, const uint32_t size)
{
    std::string cmd{};
    cmd = "INSERT INTO ";
    cmd += mName + " (" + key + ")";
    cmd += " VALUES (?);" ;
    sqlite3_stmt* stmt;
    int32_t ret{sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr)};
    if (ret != SQLITE_OK)
    {
        LOG_E("clear failed, reason = %s", sqlite3_errmsg(mSQLitePtr.get()));
    }
    if(size <= static_cast<uint32_t>(INT32_MAX)){
        ret = sqlite3_bind_blob(stmt, 1, data, static_cast<int32_t>(size), SQLITE_STATIC);
        LOG_D("sqlite3_bind_blob result: %s", Database::errorToString(ret).c_str());
    }
    else{
        // print error log
    }
    ret = sqlite3_step(stmt);
    if (ret == SQLITE_DONE)
    {
        LOG_D("Byte array insert successfully!");
    } else {
        LOG_E("Byte array insert failed, reason = %s", sqlite3_errmsg(mSQLitePtr.get()));
    }
    ret = sqlite3_finalize(stmt);
    if(ret != SQLITE_OK){
        LOG_E("sqlite3_finalize failed, reason = %d", ret);
    }
    return ret;
}

int32_t Database::getIds(std::vector<int32_t>& idList)
{
    // Prepare the query
    const std::string cmd{"SELECT id FROM " + mName + ";"};
    sqlite3_stmt* stmt;
    const int32_t ret{sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr)};
    if (ret != SQLITE_OK)
    {
        LOG_E("getIds failed, reason = %s", sqlite3_errmsg(mSQLitePtr.get()));
    }
    else{
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int32_t idValue{0};
            idValue = sqlite3_column_int(stmt, 0);
            idList.push_back(idValue);
        }
        const int32_t finalResult{sqlite3_finalize(stmt)};
        if(finalResult != SQLITE_OK){
            LOG_E("sqlite3_finalize failed, reason = %d", finalResult);
        }
    }
    return ret;
}

std::string Database::errorToString(const int32_t reason)
{
    return std::string(sqlite3_errstr(reason));
}

int32_t Database::saveOccurrentRobNotification(const android::sp<OccurrentRobNotification>& notification)
{
    const std::string cmd{"INSERT INTO " + mName + " (collection_condition_id\
                                                  , location_latitude\
                                                  , location_longitude\
                                                  , diag_accqui_time\
                                                  , priority\
                                                  , target_collection_data) VALUES (?, ?, ?, ?, ?, ?);"};

    sqlite3_stmt* stmt;
    char_t *errMsg{nullptr};
    int32_t ret{sqlite3_exec(mSQLitePtr.get(), "BEGIN", nullptr, nullptr, &errMsg)};
    if (ret == SQLITE_OK ) {
        sqlite3_free(errMsg);
    } else {
        LOG_E("Executing BEGIN failed, reason = %s", errMsg);
        sqlite3_free(errMsg);
    }

    ret = sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        LOG_E("Can not save notification to %s table", mName.c_str());
    }
    else{
        const uint64_t collectId{notification->collectionConditionId()};
        if(collectId <= static_cast<uint64_t>(INT64_MAX)){
            ret = sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(collectId));
        }
        else {
            // print error log
        }
        const int32_t latitude_1{notification->triggerLocation()->getLatitude()};
        const int32_t longitude_1{notification->triggerLocation()->getLongtitude()};
        ret = sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(latitude_1));
        ret = sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(longitude_1));
        ret = sqlite3_bind_int64(stmt, 4, notification->triggerTime());
        ret = sqlite3_bind_int(stmt, 5, notification->priority());

        const uint32_t bytesSize{notification->getTargetCollectionDataRobSsr()->ByteSizeLong()};
        std::vector<uint8_t> payload{};
        payload.resize(bytesSize);
        if(bytesSize <= static_cast<uint32_t>(INT32_MAX)){
            const bool serialResult{notification->getTargetCollectionDataRobSsr()->SerializeToArray(reinterpret_cast<uint8_t*>(payload.data()), static_cast<int32_t>(bytesSize))};
            if(serialResult == true){
                ret = sqlite3_bind_blob(stmt, 6, reinterpret_cast<uint8_t*>(payload.data()), static_cast<int32_t>(bytesSize), SQLITE_STATIC);
                LOG_D("sqlite3_bind_blob result: %s", Database::errorToString(ret).c_str());
            }
            else{
                LOG_E("SerializeToArray failed");
            }
        }
        else{
            // print error log
        }
        ret = sqlite3_step(stmt);
        if (ret != SQLITE_DONE )
        {
            LOG_E("Save data occurrent RoB event to memory failed, reason = %s", Database::errorToString(ret).c_str());
        }
        const int32_t finalResult{sqlite3_finalize(stmt)};
        if(finalResult != SQLITE_OK){
            LOG_E("sqlite3_finalize failed, reason = %d", finalResult);
        }
        ret = sqlite3_exec(mSQLitePtr.get(), "COMMIT", nullptr, nullptr, &errMsg);
        if (ret == SQLITE_OK ) {
            sqlite3_free(errMsg);
        } else {
            LOG_E("Executing COMMIT failed, reason = %s", errMsg);
            sqlite3_free(errMsg);
        }
    }
    return ret;
}

int32_t Database::getAllOccurrentRobNotification(std::deque<android::sp<OccurrentRobNotification>>& aQueue)
{
    sqlite3_stmt* stmt;
    const std::string cmd{"SELECT * FROM " + mName + ";"};

    const int32_t ret{sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr)};
    if (ret != SQLITE_OK)
    {
        LOG_E("Can not query %s table", mName.c_str());
    }
    else{
        while (sqlite3_step(stmt) == SQLITE_ROW) 
        {
            const int64_t collectionConditionId_1{sqlite3_column_int64(stmt, 1)};
            uint64_t collectionConditionId_2{0U};
            if(collectionConditionId_1 >= 0){
                collectionConditionId_2 = static_cast<uint64_t>(collectionConditionId_1);
            }
            else{
                // print error log
            }
            const int64_t latitude_1{sqlite3_column_int64(stmt, 2)};
            int32_t latitude_2{0};
            if((latitude_1 >= static_cast<int64_t>(INT32_MIN)) && (latitude_1 <= static_cast<int64_t>(INT32_MAX))){
                latitude_2 = static_cast<int32_t>(latitude_1);
            }
            else{
                // print error log
            }
            const int64_t longitude_1{sqlite3_column_int64(stmt, 3)};
            int32_t longitude_2{0};
            if((longitude_1 >= static_cast<int64_t>(INT32_MIN)) && (longitude_1 <= static_cast<int64_t>(INT32_MAX))){
                longitude_2 = static_cast<int32_t>(longitude_1);
            }
            else{
                // print error log
            }
            const int64_t diagAccquiTime_1{sqlite3_column_int64(stmt, 4)};
            uint64_t diagAccquiTime_2{0U};
            if(diagAccquiTime_1 >= 0){
                diagAccquiTime_2 = static_cast<uint64_t>(diagAccquiTime_1);
            }
            else{
                // print error log
            }
            const int64_t priority_1{sqlite3_column_int64(stmt, 5)};
            int32_t priority_2{0};
            if((priority_1 <= INT32_MAX) && (priority_1 >= INT32_MIN)){
                priority_2 = static_cast<int32_t>(priority_1);
            }
            else{
                // print error log
            }
            const void* const blobData{sqlite3_column_blob(stmt, 6)};
            const int32_t blobSize{sqlite3_column_bytes16(stmt, 6)};

            const android::sp<OccurrentRobNotification> notification {new OccurrentRobNotification()};
            const android::sp<CommonDefine::RDGLocationData> location {new CommonDefine::RDGLocationData()};
            notification->setCollectionConditionId(collectionConditionId_2);
            location->setLatitude(latitude_2);
            location->setLongitude(longitude_2);
            notification->setTriggerLocation(location);
            notification->setTriggerTime(static_cast<int64_t>(diagAccquiTime_2));
            notification->setPriority(priority_2);
            const bool parseResult{notification->getTargetCollectionDataRobSsr()->ParseFromArray(blobData, blobSize)};
            if(parseResult == false){
                LOG_E("ParseFromArray failed");
            }
            aQueue.push_back(notification);
        }
    }
    return ret;
}

int32_t Database::saveRoBSsrCrcInfo(const uint32_t targetAddress
                                , const uint32_t occurred_rob_ssr_crc
                                , const uint32_t time_series_rob_ssr_crc)
{
    const std::string cmd{"INSERT INTO " + mName + " (target_address\
                                                  , occurred_rob_ssr_crc\
                                                  , time_series_rob_ssr_crc) VALUES (?, ?, ?);"};
    sqlite3_stmt* stmt;
    char_t *errMsg{nullptr};
    int32_t ret{sqlite3_exec(mSQLitePtr.get(), "BEGIN", nullptr, nullptr, &errMsg)};
    if (ret == SQLITE_OK ) {
        sqlite3_free(errMsg);
    } else {
        LOG_E("Executing BEGIN failed, reason = %s", errMsg);
        sqlite3_free(errMsg);
    }

    ret = sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        LOG_E("Can not save notification to %s table", mName.c_str());
    }
    else{
        if(targetAddress <= static_cast<uint32_t>(INT32_MAX)){
            ret = sqlite3_bind_int(stmt, 1, static_cast<int32_t>(targetAddress));
        }
        else{
            // print error log
        }
        if(occurred_rob_ssr_crc <= static_cast<uint32_t>(INT32_MAX)){
            ret = sqlite3_bind_int(stmt, 2, static_cast<int32_t>(occurred_rob_ssr_crc));
        }
        else{
            // print error log
        }
        if(time_series_rob_ssr_crc <= static_cast<uint32_t>(INT32_MAX)){
            ret = sqlite3_bind_int(stmt, 3, static_cast<int32_t>(time_series_rob_ssr_crc));
        }
        else{
            // print error log
        }
        ret = sqlite3_step(stmt);
        if (ret != SQLITE_DONE )
        {
            LOG_E("Save data RoBSSR CRC to memory failed, reason = %s", Database::errorToString(ret).c_str());
        }
        const int32_t finalResult{sqlite3_finalize(stmt)};
        if(finalResult != SQLITE_OK){
            LOG_E("sqlite3_finalize failed, reason = %d", finalResult);
        }
        ret = sqlite3_exec(mSQLitePtr.get(), "COMMIT", nullptr, nullptr, &errMsg);
        if (ret == SQLITE_OK ) {
            sqlite3_free(errMsg);
        } else {
            LOG_E("Executing COMMIT failed, reason = %s", errMsg);
            sqlite3_free(errMsg);
        }
    }
    return ret;
}
uint32_t Database::getUploadDB(std::vector<CommonDefine::UploadFileAttribute> &data) {
    sqlite3_stmt* stmt;
    const std::string cmd{"SELECT * FROM " + mName + ";"};

    const int32_t ret{sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr)};
    if (ret != SQLITE_OK)
    {
        LOG_E("Can not query %s table", mName.c_str());
    }
    else{
        while (sqlite3_step(stmt) == SQLITE_ROW) 
        {
            uint32_t prio_2{0U};
            const int32_t prio_1{sqlite3_column_int(stmt, 1)};
            if(prio_1 >= 0){
                prio_2 = static_cast<uint32_t>(prio_1);
            }
            else{
                // print error log
            }
            uint32_t fileType_2{0U};
            const int32_t fileType_1{sqlite3_column_int(stmt, 2)};
            if(fileType_1 >= 0){
                fileType_2 = static_cast<uint32_t>(fileType_1);
            }
            else{
                // print error log
            }
            uint64_t fileSize_2{0U};
            const int64_t fileSize_1{sqlite3_column_int64(stmt, 3)};
            if(fileSize_1 >= 0){
                fileSize_2 = static_cast<uint64_t>(fileSize_1);
            }
            else{
                // print error log
            }
            const void* const blobData{sqlite3_column_blob(stmt, 4)};      
            uint32_t blobSize_2{0U};
            const int32_t blobSize_1{sqlite3_column_bytes(stmt, 4)};
            if(blobSize_1 >= 0){
                blobSize_2 = static_cast<uint32_t>(blobSize_1);
            }
            else{
                // print error log
            }
            const std::string filePath{reinterpret_cast<const char_t*>(blobData), blobSize_2};
            CommonDefine::UploadFileAttribute file1{};
            file1.setpriority(prio_2);
            file1.setFileType(fileType_2);
            file1.setFileSize(fileSize_2);
            file1.setFilePath(filePath);
            data.push_back(file1);
            LOG_I("CHECK prio: %d fileType: %d FileSize: %lld filePath: %s", prio_2, fileType_2, fileSize_2, filePath.c_str());
        }
        LOG_I("Check vector size: %d", data.size());
    }
    if(ret < 0){
        // print error log
    }
    return static_cast<uint32_t>(ret);
}

uint32_t Database::saveUploadDB(const uint32_t prio, const uint32_t uploadfiletype, const uint64_t fileSize, const std::string uploadpatch) {
    const std::string cmd{"INSERT INTO " + mName + " (prio\
                                                  , uploadfiletype\
                                                  , filesize\
                                                  , uploadpatch) VALUES (?, ?, ?, ?);"};
    sqlite3_stmt* stmt;
    char_t *errMsg{nullptr};
    int32_t ret{sqlite3_exec(mSQLitePtr.get(), "BEGIN", nullptr, nullptr, &errMsg)};
    if (ret == SQLITE_OK ) {
        sqlite3_free(errMsg);
    } else {
        LOG_E("Executing BEGIN failed, reason = %s", errMsg);
        sqlite3_free(errMsg);
    }

    ret = sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        LOG_E("Can not save notification to %s table", mName.c_str());
    }
    else{
        if(prio <= static_cast<uint32_t>(INT32_MAX)){
            ret = sqlite3_bind_int(stmt, 1, static_cast<int32_t>(prio));
        }
        else{
            // print error log
        }
        if(uploadfiletype <= static_cast<uint32_t>(INT32_MAX)){
            ret = sqlite3_bind_int(stmt, 2, static_cast<int32_t>(uploadfiletype));
        }
        else{
            // print error log
        }
        if(fileSize <= static_cast<uint64_t>(INT64_MAX)){
            ret = sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(fileSize));
            LOG_D("sqlite3_bind_int64 result: %s", Database::errorToString(ret).c_str());
        }
        else{
            // print error log
        }
        ret = sqlite3_bind_text(stmt, 4, uploadpatch.c_str(), -1, nullptr);
        ret = sqlite3_step(stmt);
        if (ret != SQLITE_DONE )
        {
            LOG_E("Save data UploadDB to memory failed, reason = %s", Database::errorToString(ret).c_str());
        }
        const int32_t finalResult{sqlite3_finalize(stmt)};
        if(finalResult != SQLITE_OK){
            LOG_E("sqlite3_finalize failed, reason = %d", finalResult);
        }
        ret = sqlite3_exec(mSQLitePtr.get(), "COMMIT", nullptr, nullptr, &errMsg);
        if (ret == SQLITE_OK ) {
            sqlite3_free(errMsg);
        } else {
            LOG_E("Executing COMMIT failed, reason = %s", errMsg);
            sqlite3_free(errMsg);
        }
    }
    if(ret < 0){
        // print error log
    }
    return static_cast<uint32_t>(ret);
}

uint32_t Database::deleteUploadDB(const std::string data) {
    int32_t ret{0};
    const std::string cmd{"DELETE FROM " + mName + " WHERE uploadpatch = ?;"};
    sqlite3_stmt* stmt;

    ret = sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        LOG_E("Can not delete to %s table", mName.c_str());
    }
    else{
        ret = sqlite3_bind_text(stmt, 1, data.c_str(), -1, nullptr);
        ret = sqlite3_step(stmt);
        if (ret != SQLITE_DONE )
        {
            LOG_E("Fail to delete, reason = %s", Database::errorToString(ret).c_str());
        }
        const int32_t finalResult{sqlite3_finalize(stmt)};
        if(finalResult != SQLITE_OK){
            LOG_E("sqlite3_finalize failed, reason = %d", finalResult);
        }
        // sqlite3_close(db);
    }
    if(ret < 0){
        // print error log
    }
    return static_cast<uint32_t>(ret);
}

int32_t Database::getAllRoBSsrCrcInfo(std::unordered_map<uint32_t, std::shared_ptr<CrcInformation>>& crcList)
{
    sqlite3_stmt* stmt;
    const std::string cmd{"SELECT * FROM " + mName + ";"};

    const int32_t ret{sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr)};
    if (ret != SQLITE_OK)
    {
        LOG_E("Can not query %s table", mName.c_str());
    }
    else{
        while (sqlite3_step(stmt) == SQLITE_ROW) 
        {
            const int32_t targetAddress_1{sqlite3_column_int(stmt, 1)};
            uint32_t targetAddress_2{0U};
            if(targetAddress_1 >= 0){
                targetAddress_2 = static_cast<uint32_t>(targetAddress_1);
            }
            else{
                // print error log
            }
            const int32_t occurred_rob_ssr_crc_1{sqlite3_column_int(stmt, 2)};
            uint32_t occurred_rob_ssr_crc_2{0U};
            if(occurred_rob_ssr_crc_1 >= 0){
                occurred_rob_ssr_crc_2 = static_cast<uint32_t>(occurred_rob_ssr_crc_1);
            }
            else{
                // print error log
            }
            const int32_t series_rob_ssr_crc_1{sqlite3_column_int(stmt, 3)};
            uint32_t series_rob_ssr_crc_2{0U};
            if(series_rob_ssr_crc_1 >= 0){
                series_rob_ssr_crc_2 = static_cast<uint32_t>(series_rob_ssr_crc_1);
            }
            else{
                // print error log
            }
            const std::shared_ptr<CrcInformation> crcInfo{std::shared_ptr<CrcInformation>(new CrcInformation())};
            crcInfo->set_occurred_rob_ssr_crc(occurred_rob_ssr_crc_2);
            crcInfo->set_time_series_rob_ssr_crc(series_rob_ssr_crc_2);
            crcList[targetAddress_2] = crcInfo;
        }
    }
    return ret;
}
}
