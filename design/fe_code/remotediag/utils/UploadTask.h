#ifndef RDG_UPLOAD_TASK
#define RDG_UPLOAD_TASK

#include <map>
#include <string>
#include <utils/RefBase.h>

#include <services/HttpManagerService/IHttpManagerService.h>
#include <services/HttpManagerService/IHttpManagerServiceType.h>
#include <services/HttpManagerService/IGRPCReceiver.h>
#include <services/HttpManagerService/IHTTPReceiver.h>
#include "ParamsDef.h"
#include "common_def.h"
#include "utils/Logger.h"
#include "FileUtil.h"

namespace rdgapp {

class UploadTask : public android::RefBase {
public:
    constexpr static uint32_t MAX_RETRY{3U};
    constexpr static uint32_t RETRY_TIMEOUT_FIRST{10U};//10 sec - retry policy
    constexpr static uint32_t RETRY_TIMEOUT_SECOND{60U};//60 sec - retry policy
    constexpr static uint32_t RETRY_TIMEOUT_THIRD{300U};//300 sec - retry policy
    constexpr static uint32_t UPLOAD_TIMING_TIME{1800U};//30 min = 1800 sec
    explicit UploadTask(const uint64_t i);
    ~UploadTask() override;

    uint64_t getUploadId() const noexcept;
    void setUploadId(const uint64_t data) noexcept;
    uint32_t getUploadPrio() const noexcept;
    // uint32_t getPriority(){return mUploadPrio;};
    void setUploadPrio(const uint32_t data) noexcept;
    GRPC_IF_TYPE getUploadFileType() const noexcept;
    void setUploadFileType(const GRPC_IF_TYPE data) noexcept;
    std::string getUploadPatch() noexcept;
    void setUploadPatch(const std::string data);
    std::string getUploadFileName() noexcept;
    void setUploadFileName(const std::string data);
    uint64_t getFileSize() const noexcept;
    void setFileSize(const uint64_t sizeData) noexcept;
    void setSrvcAcFlag(const bool srvc_ac_flag) noexcept;
    void increaseCount() noexcept;
    void resetRetryCount() noexcept;
    uint32_t getRetryCount() const noexcept;
    bool canRetry_2();
    uint64_t getRetryInterval();
    void deteleUploadFile();
    bool getSRVC_Flag() const noexcept;

private:
    uint64_t mUploadId;
    uint32_t mUploadPrio;
    /*File type. For example: DCIF_RDG160*/
    GRPC_IF_TYPE mUploadFileType;
    /*TriggerType*/
    /*Full patch*/
    std::string mUploadPatch;
    /*File name*/
    std::string mUploadFileName;
    uint32_t mRetryCount;
    uint64_t mRetryInterval;
    uint64_t mFileSize;
    bool mSRVCAC_Flag;
};
}
#endif /* RDG_UPLOAD_TASK */
