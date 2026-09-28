#include "UploadTask.h"
#ifdef ENABLE_LGE_LXC
#include "RemoteFileStore.h"
#endif /* ENABLE_LGE_LXC */

namespace rdgapp {

UploadTask::UploadTask(const uint64_t i):
    android::RefBase()
    ,  mUploadId{i}
    ,  mUploadPrio{1U}
    ,  mUploadFileType{GRPC_IF_TYPE::DCIF_MAX}
    ,  mUploadPatch{""}
    ,  mUploadFileName{""}
    ,  mRetryCount{0U}
    ,  mRetryInterval{RETRY_TIMEOUT_FIRST}
    ,  mFileSize{0U}
    ,  mSRVCAC_Flag{false}
    ,  mResCode{grpc::StatusCode::DEADLINE_EXCEEDED}
    ,  mIsRestartUpload{false}
    ,  mIsOperationA{false}{
    LOG_I("Create UploadId: %llu", mUploadId);
}

UploadTask::~UploadTask() {
    LOG_I("Destroy UploadId: %llu", mUploadId);
};

void UploadTask::resetRetryCount() noexcept {
    mRetryCount = 0U;
}

bool UploadTask::canRetry_2() {
    bool res{true};
    if(mRetryCount >= MAX_RETRY){
        LOG_E("mRetryCount = %u is out of range", mRetryCount);
        res = false;
    } else {
        res = true;
    }
    mRetryCount++;
    return res;
}

uint64_t UploadTask::getRetryInterval() {
    if(mRetryCount == 0U){
        mRetryInterval = RETRY_TIMEOUT_FIRST;
    } else if(mRetryCount == 1U) {
        mRetryInterval = RETRY_TIMEOUT_FIRST;
    } else if(mRetryCount == 2U) {
        mRetryInterval = RETRY_TIMEOUT_SECOND;
    } else if(mRetryCount == 3U) {
        mRetryInterval = RETRY_TIMEOUT_THIRD;
    } else if(mRetryCount >= 4U) {
        mRetryInterval = UPLOAD_TIMING_TIME;
    } else {
        /*TBD*/
    }
    LOG_D("mRetryInterval: %llu", mRetryInterval);
    return mRetryInterval;
};

uint64_t UploadTask::getResTimeout() {
    uint64_t tmp_time{TIMEOUT_UPLOADING_DURATION};
    if(mRetryCount == 0U){
        tmp_time = RETRY_TIMEOUT_FIRST - DELAY_HTTP_RESPONSE_TIMEOUT;
    } else if(mRetryCount == 1U) {
        tmp_time = RETRY_TIMEOUT_SECOND - DELAY_HTTP_RESPONSE_TIMEOUT;
    } else if(mRetryCount == 2U) {
        tmp_time = RETRY_TIMEOUT_THIRD - DELAY_HTTP_RESPONSE_TIMEOUT;
    } else if(mRetryCount == 3U) {
        tmp_time = TIMEOUT_UPLOADING_DURATION;
    } else if(mRetryCount >= 4U) {
        LOG_D("Retry count >= 4");
    } else {
        /*TBD*/
    }
    //cppcheck-suppress invalidPrintfArgType_uint
    LOG_D("GRPC Response timeout: %llu", tmp_time);
    return tmp_time;
}

bool UploadTask::deteleUploadFile() {
    /*remove file, If success => retturn true*/
    const std::string mPath{UPLOAD_PATH + mUploadPatch};
    bool result {false};
#ifdef ENABLE_LGE_LXC
    if(RemoteFileStore::exists(mUploadPatch)) {
        LOG_I("File still exist. Delete file: %s", mPath.c_str());
        if(RemoteFileStore::removeFile(mUploadPatch)) {
#else
    if(FileUtil::isPathExist(mPath.c_str())) {
        LOG_I("File still exist. Delete file: %s", mPath.c_str());
        if(FileUtil::removeFile(mPath.c_str())) {
#endif /* ENABLE_LGE_LXC */
            LOG_I("remove success");
            result = true;
        } else {
            LOG_I("remove fail");
        }
    } else {
        LOG_I("File: %s do not exist", mPath.c_str());
    }
    return result;
}

bool UploadTask::getSRVC_Flag() const noexcept {
    return mSRVCAC_Flag;
}
uint32_t UploadTask::getRetryCount() const noexcept {
    return mRetryCount;
}
void UploadTask::increaseCount() noexcept {
    if(mRetryCount < UINT32_MAX){
        mRetryCount = mRetryCount + 1U;
    }
    else{
        mRetryCount = 0U;
    }
}
void UploadTask::setSrvcAcFlag(const bool srvc_ac_flag) noexcept {
    mSRVCAC_Flag = srvc_ac_flag;
}
uint64_t UploadTask::getFileSize() const noexcept {
    return mFileSize;
}
void UploadTask::setFileSize(const uint64_t sizeData) noexcept {
    mFileSize = sizeData;
}
void UploadTask::setUploadFileName(const std::string data){
    mUploadFileName = data;
}
std::string UploadTask::getUploadFileName() noexcept {
    return mUploadFileName;
}
void UploadTask::setUploadPatch(const std::string data){
    LOG_D("UploadId: %llu File: %s", mUploadId, data.c_str());
    mUploadPatch = data;
}
std::string UploadTask::getUploadPatch() noexcept {
    return mUploadPatch;
}
void UploadTask::setUploadFileType(const GRPC_IF_TYPE data) noexcept {
    mUploadFileType = data;
}
GRPC_IF_TYPE UploadTask::getUploadFileType() const noexcept {
    return mUploadFileType;
}
void UploadTask::setUploadPrio(const uint32_t data) noexcept {
    mUploadPrio = data;
}
uint32_t UploadTask::getUploadPrio() const noexcept {
    return mUploadPrio;
}
void UploadTask::setUploadId(const uint64_t data) noexcept {
    mUploadId = data;
}
uint64_t UploadTask::getUploadId() const noexcept {
    return mUploadId;
}
void UploadTask::setGRPCResCode(const grpc::StatusCode data) {
    mResCode = data;
}
grpc::StatusCode UploadTask::getGRPCResCode() const noexcept {
    return mResCode;
}

void UploadTask::setIsRestartUpload(const bool data) noexcept {
    mIsRestartUpload = data;
}

bool UploadTask::getIsRestartUpload() const noexcept {
    return mIsRestartUpload;
}
void UploadTask::setIsOperationA(const bool data) noexcept {
    mIsOperationA = data;
}
bool UploadTask::getIsOperationA() const noexcept {
    return mIsOperationA;
}
}
