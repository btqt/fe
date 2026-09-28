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
    sqlite3_stmt* stmt {nullptr};
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
    if (stmt != nullptr) {
        const int32_t finalizeResult{sqlite3_finalize(stmt)};
        if (finalizeResult != SQLITE_OK) {
            LOG_E("sqlite3_finalize failed, reason = %d", finalizeResult);
            if (ret == SQLITE_OK) {
                ret = finalizeResult;
            }
        }
    }
    return ret;
}

int32_t Database::exec(const std::string& statement)
{
    char_t *errMsg{nullptr};
    const int32_t ret{sqlite3_exec(mSQLitePtr.get(), statement.c_str(), nullptr, nullptr, &errMsg)};
    if (ret != SQLITE_OK ) {
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
    else
    {
        if(size <= static_cast<uint32_t>(INT32_MAX)){
            ret = sqlite3_bind_blob(stmt, 1, data, static_cast<int32_t>(size), SQLITE_STATIC);
            if (ret != SQLITE_OK)
            {
                LOG_E("Error: %d", ret);
            }
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
                                                  , notification_id\
                                                  , location_latitude\
                                                  , location_longitude\
                                                  , diag_accqui_time\
                                                  , priority\
                                                  , target_collection_data) VALUES (?, ?, ?, ?, ?, ?, ?);"};

    sqlite3_stmt* stmt;
    char_t *errMsg{nullptr};
    int32_t ret{sqlite3_exec(mSQLitePtr.get(), "BEGIN", nullptr, nullptr, &errMsg)};
    if (ret != SQLITE_OK ) {
        LOG_E("Executing BEGIN failed, reason = %s", errMsg);
        sqlite3_free(errMsg);
    }

    ret = sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        LOG_E("Can not save notification to %s table", mName.c_str());
    } else {
        // save collectionConditionId
        const uint64_t collectId{notification->collectionConditionId()};
        const android::sp<::Buffer> serializedCocoId {SerializeUint64(collectId).getSerialized()};

        const uint32_t len {serializedCocoId->size()};
        if(len <= static_cast<uint32_t>(INT32_MAX)){
            ret = sqlite3_bind_blob(stmt, 1, serializedCocoId->data(), static_cast<int32_t>(len), SQLITE_STATIC);
            if (ret != SQLITE_OK)
            {
                LOG_E("Error: %d", ret); 
            }
        }
        else {
            // print error log
        }

        // save notification id
        const uint64_t notificationId{notification->getId()};
        const android::sp<::Buffer> serializedNotificationId {SerializeUint64(notificationId).getSerialized()};
        const uint32_t serializedDataLen {serializedNotificationId->size()};

        if(serializedDataLen <= static_cast<uint32_t>(INT32_MAX)){
            ret = sqlite3_bind_blob(stmt, 2, serializedNotificationId->data(), static_cast<int32_t>(serializedDataLen), SQLITE_STATIC);
            if (ret != SQLITE_OK)
            {
                LOG_E("Error: %d", ret); 
            }
        }
        else {
            // print error log
        }

        const int32_t latitude_1{notification->triggerLocation()->getLatitude()};
        const int32_t longitude_1{notification->triggerLocation()->getLongtitude()};
        ret = sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(latitude_1));
        if (ret != SQLITE_OK)
        {
            LOG_E("Error: %d", ret); 
        }
        ret = sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(longitude_1));
        if (ret != SQLITE_OK)
        {
            LOG_E("Error: %d", ret); 
        }
        ret = sqlite3_bind_int64(stmt, 5, notification->triggerTime());
        if (ret != SQLITE_OK)
        {
            LOG_E("Error: %d", ret); 
        }
        const int32_t tmpPrio {notification->priority()};
        constexpr int32_t MAX_PRIORITY_VALUE {255};
        if ((tmpPrio < 0) || (tmpPrio > MAX_PRIORITY_VALUE))
        {
            LOG_E("Invalid Priority %ld", tmpPrio);
        }
        ret = sqlite3_bind_int(stmt, 6, tmpPrio);
        if (ret != SQLITE_OK)
        {
            LOG_E("Error: %d", ret); 
        }
        (void)MAX_PRIORITY_VALUE;
        (void)tmpPrio;

        // save TargetCollectionDataRobSsr
        const TargetCollectionDataRobSsr aTargetCollectionDataRobSsr {notification->getTargetCollectionDataRobSsr()};
        const uint32_t bytesSize{static_cast<uint32_t>(aTargetCollectionDataRobSsr.ByteSizeLong())};
        std::vector<uint8_t> payload{};
        payload.resize(bytesSize);
        if(bytesSize <= static_cast<uint32_t>(INT32_MAX)){
            const bool serialResult{aTargetCollectionDataRobSsr.SerializeToArray(reinterpret_cast<uint8_t*>(payload.data()), static_cast<int32_t>(bytesSize))};
            if(serialResult == true){
                ret = sqlite3_bind_blob(stmt, 7, reinterpret_cast<uint8_t*>(payload.data()), static_cast<int32_t>(bytesSize), SQLITE_STATIC);
                if (ret != SQLITE_OK)
                {
                    LOG_E("Error: %d", ret); 
                }
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
        if(finalResult != SQLITE_OK) {
            LOG_E("sqlite3_finalize failed, reason = %d", finalResult);
        }
        ret = sqlite3_exec(mSQLitePtr.get(), "COMMIT", nullptr, nullptr, &errMsg);
        if (ret != SQLITE_OK ) {
            LOG_E("Executing COMMIT failed, reason = %s", errMsg);
            sqlite3_free(errMsg);
        }
    }
    return ret;
}

int32_t Database::getAllOccurrentRobNotification(OccurrentRobNotificationList& aList)
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
            uint64_t collectionConditionId {0U};
            const uint8_t* const blobCocoIdData{static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 1))};
            const int32_t blobCocoIdDataSize{sqlite3_column_bytes(stmt, 1)};

            if (blobCocoIdDataSize >= 8)
            {
                collectionConditionId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[7]) << 56;
                collectionConditionId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[6]) << 48;
                collectionConditionId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[5]) << 40;
                collectionConditionId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[4]) << 32;
                collectionConditionId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[3]) << 24;
                collectionConditionId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[2]) << 16;
                collectionConditionId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[1]) << 8;
                collectionConditionId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[0]);
            } else {
                LOG_E("get collectionConditionId failed, blobCocoIdDataSize=%d", blobCocoIdDataSize);
            }
            (void)blobCocoIdData;

            // const uint64_t collectionConditionId {SerializeUint64(blobCocoIdData, blobCocoIdDataSize).getU64()};
            // if (collectionConditionId == 0)
            // {
            //     LOG_E("get collectionConditionId failed, blobCocoIdDataSize=%d", blobCocoIdDataSize);
            // }

            const uint8_t* const blobNotificationIdData{static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 2))};
            const int32_t blobNotificationIdDataSize{sqlite3_column_bytes(stmt, 2)};
            const int64_t latitude_1{sqlite3_column_int64(stmt, 3)};
            int32_t latitude_2{0};
            
            if((latitude_1 >= static_cast<int64_t>(INT32_MIN)) && (latitude_1 <= static_cast<int64_t>(INT32_MAX))){
                latitude_2 = static_cast<int32_t>(latitude_1);
            }
            else{
                // print error log
            }
            const int64_t longitude_1{sqlite3_column_int64(stmt, 4)};
            int32_t longitude_2{0};
            if((longitude_1 >= static_cast<int64_t>(INT32_MIN)) && (longitude_1 <= static_cast<int64_t>(INT32_MAX))){
                longitude_2 = static_cast<int32_t>(longitude_1);
            }
            else{
                // print error log
            }
            const int64_t diagAccquiTime_1{sqlite3_column_int64(stmt, 5)};
            uint64_t diagAccquiTime_2{0U};
            if(diagAccquiTime_1 >= 0){
                diagAccquiTime_2 = static_cast<uint64_t>(diagAccquiTime_1);
            }
            else{
                // print error log
            }
            const int64_t priority_1{sqlite3_column_int64(stmt, 6)};
            int32_t priority_2{0};
            if((priority_1 <= INT32_MAX) && (priority_1 >= INT32_MIN)){
                priority_2 = static_cast<int32_t>(priority_1);
            }
            else{
                // print error log
            }
            const void* const blobData{sqlite3_column_blob(stmt, 7)};
            const int32_t blobSize{sqlite3_column_bytes(stmt, 7)};

            if (blobNotificationIdDataSize >= 0)
            {
                const uint64_t notificationId {SerializeUint64(blobNotificationIdData, static_cast<uint32_t>(blobNotificationIdDataSize)).getU64()};
                if (notificationId == 0U)
                {
                    LOG_E("get notificationId failed, blobNotificationIdDataSize=%d", blobNotificationIdDataSize);
                }
                const android::sp<CommonDefine::RDGLocationData> location {new CommonDefine::RDGLocationData()};
                const android::sp<OccurrentRobNotification> notification {new OccurrentRobNotification()};
                notification->setId(notificationId);
                notification->setCollectionConditionId(collectionConditionId);
                location->setLatitude(latitude_2);
                location->setLongitude(longitude_2);
                notification->setTriggerLocation(location);
                notification->setTriggerTime(static_cast<int64_t>(diagAccquiTime_2));
                notification->setPriority(priority_2);
                const bool parseResult{notification->mutableTargetCollectionDataRobSsr()->ParseFromArray(blobData, blobSize)};
                if(parseResult == false) {
                    LOG_E("ParseFromArray failed");
                }
                notification->setIsRecoveryRoB(true);
                aList[notificationId] = notification;
            }
        }
    }
    return ret;
}

int32_t Database::saveRoBSsrCrcInfo(const uint64_t transId
                                , const uint32_t occurred_rob_ssr_crc
                                , const uint32_t time_series_rob_ssr_crc)
{
    const std::string cmd{"INSERT INTO " + mName + " (trans_id\
                                                  , occurred_rob_ssr_crc\
                                                  , time_series_rob_ssr_crc) VALUES (?, ?, ?);"};
    sqlite3_stmt* stmt;
    char_t *errMsg{nullptr};
    int32_t ret{sqlite3_exec(mSQLitePtr.get(), "BEGIN", nullptr, nullptr, &errMsg)};
    if (ret != SQLITE_OK ) {
        LOG_E("Executing BEGIN failed, reason = %s", errMsg);
        sqlite3_free(errMsg);
    }

    ret = sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        LOG_E("Can not save notification to %s table", mName.c_str());
    } else {
        const android::sp<::Buffer> serializedTransId {SerializeUint64(transId).getSerialized()};
        const uint32_t len {serializedTransId->size()};
        if(len <= static_cast<uint32_t>(INT32_MAX)){
            ret = sqlite3_bind_blob(stmt, 1, serializedTransId->data(), static_cast<int32_t>(len), SQLITE_STATIC);
            if (ret != SQLITE_OK)
            {
                LOG_E("Error: %d", ret); 
            }
        }
        else {
            // print error log
        }
        ret = sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(occurred_rob_ssr_crc));
        if (ret != SQLITE_OK)
        {
            LOG_E("Error: %d", ret); 
        }
        ret = sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(time_series_rob_ssr_crc));
        if (ret != SQLITE_OK)
        {
            LOG_E("Error: %d", ret); 
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
        if (ret != SQLITE_OK ) {
            LOG_E("Executing COMMIT failed, reason = %s", errMsg);
            sqlite3_free(errMsg);
        }
    }
    return ret;
}
uint32_t Database::getUploadDB(std::vector<CommonDefine::UploadFileAttribute> &data) {
    sqlite3_stmt* stmt {nullptr};
    const std::string cmd{"SELECT * FROM " + mName + ";"};

    const int32_t ret{sqlite3_prepare_v2(mSQLitePtr.get(), cmd.c_str(), -1, &stmt, nullptr)};
    
    if (ret == SQLITE_OK) 
    {
        int32_t stepResult{SQLITE_OK};
        // Process rows one by one
        do 
        {
            stepResult = sqlite3_step(stmt);
            
            if (stepResult == SQLITE_ROW) 
            {
                uint32_t prio_2{0U};
                const int32_t prio_1{sqlite3_column_int(stmt, 1)};
                if(prio_1 >= 0)
                {
                    prio_2 = static_cast<uint32_t>(prio_1);
                }
                else
                {
                    LOG_E("Invalid priority value (negative): %d", prio_1);
                }

                uint32_t fileType_2{0U};
                const int32_t fileType_1{sqlite3_column_int(stmt, 2)};
                if(fileType_1 >= 0)
                {
                    fileType_2 = static_cast<uint32_t>(fileType_1);
                }
                else
                {
                    LOG_E("Invalid file type value (negative): %d", fileType_1);
                }

                uint64_t fileSize_2{0U};
                const int64_t fileSize_1{sqlite3_column_int64(stmt, 3)};
                if(fileSize_1 >= 0)
                {
                    fileSize_2 = static_cast<uint64_t>(fileSize_1);
                }
                else
                {
                    LOG_E("Invalid file size value (negative): %lld", fileSize_1);
                }

                const void* const blobData{sqlite3_column_blob(stmt, 4)};
                bool skipRow {false};
                
                if (blobData == nullptr) 
                {
                    LOG_E("NULL path data retrieved from database");
                    skipRow = true;
                }
                
                if (!skipRow)
                {
                    uint32_t blobSize_2{0U};
                    const int32_t blobSize_1{sqlite3_column_bytes(stmt, 4)};
                    if(blobSize_1 >= 0)
                    {
                        blobSize_2 = static_cast<uint32_t>(blobSize_1);
                        
                        const std::string filePath{reinterpret_cast<const char_t*>(blobData), blobSize_2};
                        CommonDefine::UploadFileAttribute file1{};
                        file1.setpriority(prio_2);
                        file1.setFileType(fileType_2);
                        file1.setFileSize(fileSize_2);
                        file1.setFilePath(filePath);
                        data.push_back(file1);
                        LOG_I("CHECK prio: %d fileType: %d FileSize: %lld filePath: %s", 
                              prio_2, fileType_2, fileSize_2, filePath.c_str());
                    }
                    else
                    {
                        LOG_E("Invalid blob size value (negative): %d", blobSize_1);
                    }
                    (void)blobSize_2;
                }
                (void)prio_2;
                (void)fileType_2;
                (void)fileSize_2;
            }
        } while (stepResult == SQLITE_ROW);
        
        // Check for any errors during statement execution
        if (stepResult != SQLITE_DONE) 
        {
            LOG_E("Error while fetching rows from %s: %s", mName.c_str(), sqlite3_errmsg(mSQLitePtr.get()));
        }

        LOG_I("Check vector size: %d", data.size());
        (void)stepResult;
    } 
    else 
    {
        LOG_E("Can not query %s table, reason = %s", mName.c_str(), sqlite3_errmsg(mSQLitePtr.get()));
    }
    
    // Always finalize the statement to prevent resource leaks
    if (stmt != nullptr) 
    {
        const int32_t finalizeResult {sqlite3_finalize(stmt)};
        if (finalizeResult != SQLITE_OK)
        {
            LOG_E("sqlite3_finalize failed for %s table, reason = %d: %s", 
                  mName.c_str(), finalizeResult, sqlite3_errmsg(mSQLitePtr.get()));
        }
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
    if (ret != SQLITE_OK ) {
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
            if (ret != SQLITE_OK)
            {
                LOG_E("Error: %d", ret); 
            }
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
        if (ret != SQLITE_OK ) {
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
    int32_t ret{SQLITE_ERROR};
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

int32_t Database::getAllRoBSsrCrcInfo(std::unordered_map<uint64_t, std::shared_ptr<CrcInformation>>& crcList)
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

            uint64_t transId {0U};

            const uint8_t* const blobCocoIdData{static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 1))};
            const int32_t blobCocoIdDataSize{sqlite3_column_bytes16(stmt, 1)};

            if (blobCocoIdDataSize >= 8)
            {
                transId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[7]) << 56;
                transId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[6]) << 48;
                transId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[5]) << 40;
                transId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[4]) << 32;
                transId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[3]) << 24;
                transId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[2]) << 16;
                transId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[1]) << 8;
                transId |= (0xFFFFFFFFFFFFFFFF & blobCocoIdData[0]);
            } else {
                LOG_E("get transId failed, blobCocoIdDataSize=%d", blobCocoIdDataSize);
            }
            (void)blobCocoIdData;

            const int64_t occurred_rob_ssr_crc_1{sqlite3_column_int64(stmt, 2)};
            uint32_t occurred_rob_ssr_crc_2{0U};
            if((occurred_rob_ssr_crc_1 >= 0) && (occurred_rob_ssr_crc_1 <= static_cast<int64_t>(UINT32_MAX))){
                occurred_rob_ssr_crc_2 = static_cast<uint32_t>(occurred_rob_ssr_crc_1);
            }
            else{
                LOG_D("occurred_rob_ssr_crc_1 out of range uint32_t");
            }
            const int64_t series_rob_ssr_crc_1{sqlite3_column_int64(stmt, 3)};
            uint32_t series_rob_ssr_crc_2{0U};
            if((series_rob_ssr_crc_1 >= 0) && (series_rob_ssr_crc_1 <= static_cast<int64_t>(UINT32_MAX))){
                series_rob_ssr_crc_2 = static_cast<uint32_t>(series_rob_ssr_crc_1);
            }
            else{
                LOG_D("series_rob_ssr_crc_1 out of range uint32_t");
            }
            const std::shared_ptr<CrcInformation> crcInfo{std::shared_ptr<CrcInformation>(new CrcInformation())};
            crcInfo->set_occurred_rob_ssr_crc(occurred_rob_ssr_crc_2);
            crcInfo->set_time_series_rob_ssr_crc(series_rob_ssr_crc_2);
            crcList[transId] = crcInfo;
        }
    }

    std::unordered_map<uint64_t, std::shared_ptr<CrcInformation>>::iterator crcIt {crcList.begin()};
    for (; crcIt != crcList.end(); crcIt++)
    {
        LOG_D("TransId = 0x%llX, occurred_rob_ssr_crc = %lu, time_series_rob_ssr_crc = %lu", crcIt->first, crcIt->second->occurred_rob_ssr_crc(), crcIt->second->time_series_rob_ssr_crc());
    }
    
    if (ret == SQLITE_OK) {
        const int32_t finalResult{sqlite3_finalize(stmt)};
        if(finalResult != SQLITE_OK){
            LOG_E("sqlite3_finalize failed, reason = %d", finalResult);
        }
    }
    return ret;
}
}
