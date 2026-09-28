#include "UploadTask.h"

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
    ,  mSRVCAC_Flag{false}{
    LOG_I("Create Upload Id: %lld", mUploadId);
}

UploadTask::~UploadTask() {
    LOG_I("Destroy Upload Id: %lld", mUploadId);
    // std::string mPath{"/data/rdg/" + mUploadPatch};
    // if(FileUtil::isPathExist(mPath.c_str())) {
    //     LOG_I("File still exist when UploadTask destroy=> delete file: %s", mPath.c_str());
    //     if(FileUtil::removeFile(mPath.c_str())) {
    //         LOG_I("remove success");
    //     } else {
    //         LOG_I("remove fail");
    //     };
    // } else {
    //     LOG_I("File: %s do not exist", mPath.c_str());
    // };
};

// void UploadTask::deleteUploadTask() {
//     // mUploadPatch.clear();
// }

// bool UploadTask::canRetry() {
//     if(mRetryCount < MAX_RETRY) {
//         mRetryCount++;
//         if(mRetryCount == 0U){
//             mRetryInterval = 0U;
//         } else if(mRetryCount == 1U) {
//             mRetryInterval = RETRY_TIMEOUT_FIRST;
//         } else if(mRetryCount == 2U) {
//             mRetryInterval = RETRY_TIMEOUT_SECOND;
//         } else if(mRetryCount == 3U) {
//             mRetryInterval = RETRY_TIMEOUT_THIRD;
//         }
//         return true;
//     } else {
//         return false;
//     }
// }

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
    LOG_D("mRetryInterval: %lld", mRetryInterval);
    return mRetryInterval;
};

void UploadTask::deteleUploadFile() {
    const std::string mPath{"/data/rdg/" + mUploadPatch};
    if(FileUtil::isPathExist(mPath.c_str())) {
        LOG_I("File still exist. Delete file: %s", mPath.c_str());
        if(FileUtil::removeFile(mPath.c_str())) {
            LOG_I("remove success");
        } else {
            LOG_I("remove fail");
        }
    } else {
        LOG_I("File: %s do not exist", mPath.c_str());
    }
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
}
