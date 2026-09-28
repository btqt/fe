#ifndef REMOTEDIAG_DATA_MODEL_H
#define REMOTEDIAG_DATA_MODEL_H

#include "ParamsDef.h"
#include <memory>
#include <utils/RefBase.h>
#include "FileUtil.h"
#include "services/HttpManagerAdapter.h"
#define CHINASHIFT
#include <tdm_ext_crypto.h>
#include "Remotediag.h"

namespace rdgapp {

template<class  T>
class DataModel {
    private:
        static error_t save(const std::string& path, const T& aT) noexcept;
        static std::shared_ptr<T> get(const std::string& path) noexcept;
        static error_t clear(const std::string& path) noexcept;
    public:
        static error_t saveUpload(const std::string& name, const T& aT) noexcept;
        static error_t saveData(const std::string& name, const T& aT) noexcept;
        static std::shared_ptr<T> getUpload(const std::string& name) noexcept;
        static std::shared_ptr<T> getData(const std::string& name) noexcept;
        static error_t clearUpload(const std::string& name) noexcept;
        static error_t clearData(const std::string& name) noexcept;
        static bool checkExist(const std::string& name) noexcept;
        static android::sp<Buffer> encryptData(const T& aT) noexcept;
        static error_t MakeEncryptRequestMsg(const GRPC_IF_TYPE pIFType, const std::string& file_dir, const T& pReqSerializedData, uint32_t &fileSize) noexcept;
};
}
#endif // REMOTEDIAG_DATA_MODEL_H
