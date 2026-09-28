#include "DataModel.h"
#include "CollectionCondition.h"
#include "Logger.h"

namespace rdgapp {

template class DataModel<CollectionConditionRes>;
template class DataModel<CenterRequestAllDtcSsr>;
template class DataModel<CenterRequestAllRob>;
template class DataModel<CenterRequestEcuInformation>;
template class DataModel<CollectionConditionDiagCommon>;
template class DataModel<CollectionConditionRobRobSsrDidEvent>;
template class DataModel<CollectionConditionEcuInformation>;
template class DataModel<CollectionConditionWarningInformation>;
template class DataModel<CollectionConditionDirectCommand>;
template class DataModel<GetCollectionConditionRequest>;

template class DataModel<NotifyCollectionConditionUpdateResultRequest>;
template class DataModel<UploadDtcDataRequest>;
template class DataModel<UploadSsrDataRequest>;
template class DataModel<UploadRobDataRequest>;
template class DataModel<UploadRobSsrDataRequest>;
template class DataModel<UploadDirectCommandDataRequest>;
template class DataModel<UploadWarningInformationRequest>;
template class DataModel<UploadEcuInformationRequest>;
template class DataModel<UploadLastDataRequest>;
template class DataModel<UploadRequestResponseNotificationRequest>;
template class DataModel<UploadDdrDataRequest>;
template class DataModel<UploadErrorDataRequest>;

template <class T> error_t DataModel<T>::save(const string& name, const T& aT) noexcept
{
    error_t error {E_ERROR};
    std::string path {std::string(DATA_PATH)};
    (void)path.append(name);
    const FileHandleType handle {FileUtil::openFile(path, OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_BIN)};
    int32_t fd{-1};
    if(handle != nullptr){
        fd = FileUtil::getFileDescriptor(handle);
    }
    if(fd == -1){
        LOG_E("save data failed because can't open file %s", path.c_str());
    }
    else{
        if (aT.SerializeToFileDescriptor(fd) == true){
            error = E_OK;
        }
        const bool res{FileUtil::closeFile(handle)};
        if(res == false){
            LOG_E("fail to close file");
        }
    }
    LOG_D("Save %s, error = %d", path.c_str(), static_cast<int32_t>(error));
    return error;
}

template <class T> auto DataModel<T>::get(const string& name) -> std::shared_ptr<T>
{
    std::shared_ptr<T> outPut {nullptr};
    std::string path {std::string(DATA_PATH)};
    (void)path.append(name);
    if (FileUtil::isPathExist(path) == true) {
        int32_t fd {-1};
        const FileHandleType handle {FileUtil::openFile(path, OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN)};
        if(handle != nullptr){
            fd = FileUtil::getFileDescriptor(handle);
        }
        if(fd == -1){
            LOG_E("get data failed because can't open file %s", path.c_str());
        }
        else{
            LOG_D("Get %s, File Descriptor = %d", path.c_str(), fd);
            outPut = std::shared_ptr<T>(new T());
            const bool parseResult {outPut->ParseFromFileDescriptor(fd)};
            if(parseResult == false){
                LOG_E("fail to parse file");
            }
            const bool closeResult {FileUtil::closeFile(handle)};
            if(closeResult == false){
                LOG_E("fail to close file");
            }
        }
    }
    return outPut;
}

template <class T> error_t DataModel<T>::clear(const string& name) noexcept
{
    error_t res{0};
    std::string path {std::string(DATA_PATH)};
    (void)path.append(name);
    if (FileUtil::isPathExist(path) == true)
    {
        const bool ret {FileUtil::removeFile(path)};
        if(ret == false)
        {
            LOG_E("Clear path = %s failed", path.c_str());
            res = E_ERROR;
        }
    } else {
        LOG_E("Clear path = %s failed, because it is not exit", path.c_str());
        res = E_ERROR;
    }
    return res;
}

template <class T> bool DataModel<T>::checkExist(const string& name) noexcept
{
    std::string path {std::string(DATA_PATH)};
    (void)path.append(name);
    return FileUtil::isPathExist(path);
}
}
