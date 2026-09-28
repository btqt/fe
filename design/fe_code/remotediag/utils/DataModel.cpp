#include "DataModel.h"
#include "CollectionCondition.h"
#include "Logger.h"
#ifdef ENABLE_LGE_LXC
#include "ProxyIpcServer.h"
#include "RemoteFileStore.h"
#include "../remotediagproxy/include/ProxyIpcProtocol.h"

namespace {

std::string requestTelephonyValue(const rdgipc::CommandId commandId)
{
    std::vector<uint8_t> response{};
    if (!rdgapp::ProxyIpcServer::getInstance().requestAPICall(commandId, {}, response, 2000U))
    {
        return std::string{};
    }

    return rdgipc::toString(response);
}

}
#else
#include <TelephonyManager.hpp>
#endif /* ENABLE_LGE_LXC */

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
template class DataModel<vccomif::rdg::v1::interfaces::UploadDtcDataRequestEncryption>;
template class DataModel<vccomif::rdg::v1::interfaces::UploadSsrDataRequestEncryption>;
template class DataModel<vccomif::rdg::v1::interfaces::UploadRobDataRequestEncryption>;
template class DataModel<vccomif::rdg::v1::interfaces::UploadRobSsrDataRequestEncryption>;
template class DataModel<vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequestEncryption>;
template class DataModel<vccomif::rdg::v1::interfaces::UploadWarningInformationRequestEncryption>;
template class DataModel<vccomif::rdg::v1::interfaces::UploadEcuInformationRequestEncryption>;
template class DataModel<vccomif::rdg::v1::interfaces::UploadLastDataRequestEncryption>;
template class DataModel<vccomif::rdg::v1::interfaces::UploadRequestResponseNotificationRequestEncryption>;
template class DataModel<vccomif::rdg::v1::interfaces::UploadDdrDataRequestEncryption>;
template class DataModel<vccomif::rdg::v1::interfaces::UploadErrorDataRequestEncryption>;


template <class T> error_t DataModel<T>::save(const string& path, const T& aT) noexcept
{
    error_t error {E_ERROR};
    const FileHandleType handle {FileUtil::openFile(path, OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_BIN)};
    int32_t fd{-1};
    LOG_I("Save %s, size file = %u", path.c_str(), aT.ByteSizeLong());
    if(handle != nullptr){
        fd = FileUtil::getFileDescriptor(handle);
    }
    if(fd == -1){
        LOG_E("save data failed because can't open file %s", path.c_str());
    }
    else{
        if (aT.SerializeToFileDescriptor(fd) == true)
        {
            if (FileUtil::syncFile(handle))
            {
                error = E_OK;
            }
            else
            {
                LOG_E("save data failed because can't sync file %s to disk", path.c_str());
                error = E_ERROR;
            }
        }
        const bool res{FileUtil::closeFile(handle)};
        if(res == false){
            LOG_E("fail to close file");
        }
    }
    LOG_D("Save %s, error = %d", path.c_str(), static_cast<int32_t>(error));
    return error;
}

template <class T> error_t DataModel<T>::saveUpload(const string& name, const T& aT) noexcept
{
    error_t error {E_ERROR};
    if (Remotediag::getInstance()->getPPIFlag() == IPPIManagerServiceType::PPI_APP_STATUS_INIT)
    {
#ifdef ENABLE_LGE_LXC
        /* Upload files are stored on the EP partition via the proxy */
        std::string bytes{};
        LOG_I("Save upload %s via proxy, size file = %u", name.c_str(), static_cast<uint32_t>(aT.ByteSizeLong()));
        if (aT.SerializeToString(&bytes) == true)
        {
            if (RemoteFileStore::saveFile(name, reinterpret_cast<const uint8_t*>(bytes.data()), static_cast<uint32_t>(bytes.size())) == true)
            {
                error = E_OK;
            }
            else
            {
                LOG_E("save upload data failed because proxy store rejected %s", name.c_str());
            }
        }
        else
        {
            LOG_E("save upload data failed because can't serialize %s", name.c_str());
        }
#else
        std::string path {std::string(UPLOAD_PATH)};
        (void)path.append(name);
        error = DataModel<T>::save(path, aT);
#endif /* ENABLE_LGE_LXC */
    }
    else
    {
        LOG_E("Error due to PPI deletion");
    }
    return error;
}

template <class T> error_t DataModel<T>::saveData(const string& name, const T& aT) noexcept
{
    std::string path {std::string(DATA_PATH)};
    (void)path.append(name);
    return DataModel<T>::save(path, aT);
}

template <class T> std::shared_ptr<T> DataModel<T>::get(const string& path) noexcept
{
    std::shared_ptr<T> outPut {nullptr};
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

template <class T> std::shared_ptr<T> DataModel<T>::getUpload(const string& name) noexcept
{
    std::shared_ptr<T> outPut {nullptr};
    if (Remotediag::getInstance()->getPPIFlag() == IPPIManagerServiceType::PPI_APP_STATUS_INIT)
    {
#ifdef ENABLE_LGE_LXC
        /* Upload files are stored on the EP partition via the proxy */
        std::vector<uint8_t> bytes{};
        if (RemoteFileStore::loadFile(name, bytes) == true)
        {
            outPut = std::shared_ptr<T>(new T());
            const bool parseResult {outPut->ParseFromArray(bytes.data(), static_cast<int32_t>(bytes.size()))};
            if(parseResult == false){
                LOG_E("fail to parse file");
            }
        }
#else
        std::string path {std::string(UPLOAD_PATH)};
        (void)path.append(name);
        outPut = DataModel<T>::get(path);
#endif /* ENABLE_LGE_LXC */
    }
    else
    {
        LOG_E("Error due to PPI deletion");
    }
    return outPut;
}

template <class T> std::shared_ptr<T> DataModel<T>::getData(const string& name) noexcept
{
    std::string path {std::string(DATA_PATH)};
    (void)path.append(name);
    return DataModel<T>::get(path);
}

template <class T> error_t DataModel<T>::clear(const string& path) noexcept
{
    error_t res{E_ERROR};
    if (FileUtil::isPathExist(path) == true)
    {
        const bool ret {FileUtil::removeFile(path)};
        if(ret == false)
        {
            LOG_E("Clear path = %s failed", path.c_str());
            res = E_ERROR;
        } 
        else
        {
            res = E_OK;
        }
    } else {
        LOG_E("Clear path = %s failed, because it is not exit", path.c_str());
        res = E_ERROR;
    }
    return res;
}

template <class T> error_t DataModel<T>::clearUpload(const string& name) noexcept
{
    error_t res{E_ERROR};
    if (Remotediag::getInstance()->getPPIFlag() == IPPIManagerServiceType::PPI_APP_STATUS_INIT)
    {
#ifdef ENABLE_LGE_LXC
        /* Upload files are stored on the EP partition via the proxy */
        if (RemoteFileStore::removeFile(name) == true)
        {
            res = E_OK;
        }
        else
        {
            LOG_E("Clear upload %s failed", name.c_str());
        }
#else
        std::string path {std::string(UPLOAD_PATH)};
        (void)path.append(name);
        res = DataModel<T>::clear(path);
#endif /* ENABLE_LGE_LXC */
    }
    else
    {
        LOG_E("Error due to PPI deletion");
    }
    return res;
}

template <class T> error_t DataModel<T>::clearData(const string& name) noexcept
{
    std::string path {std::string(DATA_PATH)};
    (void)path.append(name);
    return DataModel<T>::clear(path);
}

template <class T> bool DataModel<T>::checkExist(const string& name) noexcept
{
    std::string path {std::string(DATA_PATH)};
    (void)path.append(name);
    return FileUtil::isPathExist(path);
}

template <class T>
android::sp<Buffer> DataModel<T>::encryptData(const T &aT) noexcept
{
    const android::sp<Buffer> spBuf{new Buffer()};
    const uint32_t iDataSize{static_cast<uint32_t>(aT.ByteSizeLong())};
    const std::shared_ptr<uint8_t[]> upLoadData {new uint8_t[iDataSize]};
    if (iDataSize <= static_cast<uint32_t>(INT32_MAX))
    {
        (void)aT.SerializeToArray(upLoadData.get(), static_cast<int32_t>(iDataSize));
    }
    LOG_I("Data size to encrypt %u", iDataSize);
#ifdef ENABLE_LGE_LXC
    const std::string iImeiStr{requestTelephonyValue(rdgipc::CommandId::TelephonyGetImei)};
    const std::string iEidStr{requestTelephonyValue(rdgipc::CommandId::TelephonyGetEidDefault)};
#else
    const std::string iImeiStr{telephony::TelephonyManager::getImei()};
    const std::string iEidStr{telephony::TelephonyManager::getEid(telephony::TelephonyManager::SlotIdType::DEFAULT_ID)};
#endif /* ENABLE_LGE_LXC */
    const std::shared_ptr<uint8_t[]> iImei {new uint8_t[iImeiStr.size()]};
    const std::shared_ptr<uint8_t[]> iEid {new uint8_t[iEidStr.size()]};
    (void)std::memcpy(iImei.get(), iImeiStr.c_str(), iImeiStr.size());
    (void)std::memcpy(iEid.get(), iEidStr.c_str(), iEidStr.size());
    // ret = tdm_ext_sm4_enc("RemoteDiagChinaShiftEncryption", upLoadData.get(), iDataSize, &encBuffer, &encBufSize, iImei.get(), iEid.get());

    tdm_error_t ret{TDM_ERROR_OK};
    uint8_t *encBuffer{nullptr};
    uint32_t encBufSize{0U};
    ret = tdm_ext_aes_enc("RemoteDiagChinaShiftEncryption", upLoadData.get(), iDataSize, &encBuffer, &encBufSize, iImei.get(), iEid.get());
    if ((ret != TDM_ERROR_OK) || (encBufSize == 0U))
    {
        LOG_E("TDM failed to encrypt upload data [Error code: %d][Encryption buffer size: %u]", ret, encBufSize);
    }
    else
    {
        if (encBufSize <= static_cast<uint32_t>(INT32_MAX))
        {
            spBuf->setTo(&encBuffer[0], static_cast<int32_t>(encBufSize));
        }
        LOG_E("Encrypt upload data [Error code: %d][Encryption buffer size: %u]", ret, spBuf->size());
    }
    if (encBuffer != nullptr)
    {
        delete[] encBuffer;
    }
    return spBuf;
}

template <class T>
error_t DataModel<T>::MakeEncryptRequestMsg(const GRPC_IF_TYPE pIFType, const std::string &file_dir, const T &pReqSerializedData, uint32_t &fileSize) noexcept
{
    constexpr vccomif::rdg::v1::interfaces::EncryptionType iRdgEncryptionType{vccomif::rdg::v1::interfaces::EncryptionType::ET_AES_128};
    const android::sp<Buffer> iReqSerializedData{DataModel<T>::encryptData(pReqSerializedData)};
    error_t res{E_ERROR};
    if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
    {
        switch (pIFType)
        {
        case GRPC_IF_TYPE::DCIF_RDG030:
        {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadDtcDataRequestEncryption> iMsg {std::shared_ptr<vccomif::rdg::v1::interfaces::UploadDtcDataRequestEncryption>(new vccomif::rdg::v1::interfaces::UploadDtcDataRequestEncryption())};
            if (iMsg.get() != nullptr)
            {
                iMsg->set_encryption_type(iRdgEncryptionType);
                if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
                {
                    iMsg->set_request_body(iReqSerializedData->data(), iReqSerializedData->size());
                }
                fileSize = iMsg->ByteSizeLong();
                res = DataModel<vccomif::rdg::v1::interfaces::UploadDtcDataRequestEncryption>::saveUpload(file_dir, *(iMsg.get()));
            }
        }
        break;

        case GRPC_IF_TYPE::DCIF_RDG040:
        {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadSsrDataRequestEncryption> iMsg {std::shared_ptr<vccomif::rdg::v1::interfaces::UploadSsrDataRequestEncryption>(new vccomif::rdg::v1::interfaces::UploadSsrDataRequestEncryption())};
            if (iMsg.get() != nullptr)
            {
                iMsg->set_encryption_type(iRdgEncryptionType);
                if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
                {
                    iMsg->set_request_body(iReqSerializedData->data(), iReqSerializedData->size());
                }
                fileSize = iMsg->ByteSizeLong();
                res = DataModel<vccomif::rdg::v1::interfaces::UploadSsrDataRequestEncryption>::saveUpload(file_dir, *(iMsg.get()));
            }
        }
        break;

        case GRPC_IF_TYPE::DCIF_RDG050:
        {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadRobDataRequestEncryption> iMsg {std::shared_ptr<vccomif::rdg::v1::interfaces::UploadRobDataRequestEncryption>(new vccomif::rdg::v1::interfaces::UploadRobDataRequestEncryption())};
            if (iMsg.get() != nullptr)
            {
                iMsg->set_encryption_type(iRdgEncryptionType);
                if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
                {
                    iMsg->set_request_body(iReqSerializedData->data(), iReqSerializedData->size());
                }
                fileSize = iMsg->ByteSizeLong();
                res = DataModel<vccomif::rdg::v1::interfaces::UploadRobDataRequestEncryption>::saveUpload(file_dir, *(iMsg.get()));
            }
        }
        break;

        case GRPC_IF_TYPE::DCIF_RDG060:
        {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadRobSsrDataRequestEncryption> iMsg {std::shared_ptr<vccomif::rdg::v1::interfaces::UploadRobSsrDataRequestEncryption>(new vccomif::rdg::v1::interfaces::UploadRobSsrDataRequestEncryption())};
            if (iMsg.get() != nullptr)
            {
                iMsg->set_encryption_type(iRdgEncryptionType);
                if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
                {
                    iMsg->set_request_body(iReqSerializedData->data(), iReqSerializedData->size());
                }
                fileSize = iMsg->ByteSizeLong();
                res = DataModel<vccomif::rdg::v1::interfaces::UploadRobSsrDataRequestEncryption>::saveUpload(file_dir, *(iMsg.get()));
            }
        }
        break;

        case GRPC_IF_TYPE::DCIF_RDG070:
        {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequestEncryption> iMsg {std::shared_ptr<vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequestEncryption>(new vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequestEncryption())};
            if (iMsg.get() != nullptr)
            {
                iMsg->set_encryption_type(iRdgEncryptionType);
                if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
                {
                    iMsg->set_request_body(iReqSerializedData->data(), iReqSerializedData->size());
                }
                fileSize = iMsg->ByteSizeLong();
                res = DataModel<vccomif::rdg::v1::interfaces::UploadDirectCommandDataRequestEncryption>::saveUpload(file_dir, *(iMsg.get()));
            }
        }
        break;

        case GRPC_IF_TYPE::DCIF_RDG080:
        {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadWarningInformationRequestEncryption> iMsg {std::shared_ptr<vccomif::rdg::v1::interfaces::UploadWarningInformationRequestEncryption>(new vccomif::rdg::v1::interfaces::UploadWarningInformationRequestEncryption())};
            if (iMsg.get() != nullptr)
            {
                iMsg->set_encryption_type(iRdgEncryptionType);
                if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
                {
                    iMsg->set_request_body(iReqSerializedData->data(), iReqSerializedData->size());
                }
                fileSize = iMsg->ByteSizeLong();
                res = DataModel<vccomif::rdg::v1::interfaces::UploadWarningInformationRequestEncryption>::saveUpload(file_dir, *(iMsg.get()));
            }
        }
        break;

        case GRPC_IF_TYPE::DCIF_RDG100:
        {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadEcuInformationRequestEncryption> iMsg {std::shared_ptr<vccomif::rdg::v1::interfaces::UploadEcuInformationRequestEncryption>(new vccomif::rdg::v1::interfaces::UploadEcuInformationRequestEncryption())};
            if (iMsg.get() != nullptr)
            {
                iMsg->set_encryption_type(iRdgEncryptionType);
                if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
                {
                    iMsg->set_request_body(iReqSerializedData->data(), iReqSerializedData->size());
                }
                fileSize = iMsg->ByteSizeLong();
                res = DataModel<vccomif::rdg::v1::interfaces::UploadEcuInformationRequestEncryption>::saveUpload(file_dir, *(iMsg.get()));
            }
        }
        break;

        case GRPC_IF_TYPE::DCIF_RDG120:
        {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadLastDataRequestEncryption> iMsg {std::shared_ptr<vccomif::rdg::v1::interfaces::UploadLastDataRequestEncryption>(new vccomif::rdg::v1::interfaces::UploadLastDataRequestEncryption())};
            if (iMsg.get() != nullptr)
            {
                iMsg->set_encryption_type(iRdgEncryptionType);
                if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
                {
                    iMsg->set_request_body(iReqSerializedData->data(), iReqSerializedData->size());
                }
                fileSize = iMsg->ByteSizeLong();
                res = DataModel<vccomif::rdg::v1::interfaces::UploadLastDataRequestEncryption>::saveUpload(file_dir, *(iMsg.get()));
            }
        }
        break;

        case GRPC_IF_TYPE::DCIF_RDG130:
        {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadRequestResponseNotificationRequestEncryption> iMsg {std::shared_ptr<vccomif::rdg::v1::interfaces::UploadRequestResponseNotificationRequestEncryption>(new vccomif::rdg::v1::interfaces::UploadRequestResponseNotificationRequestEncryption())};
            if (iMsg.get() != nullptr)
            {
                iMsg->set_encryption_type(iRdgEncryptionType);
                if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
                {
                    iMsg->set_request_body(iReqSerializedData->data(), iReqSerializedData->size());
                }
                fileSize = iMsg->ByteSizeLong();
                res = DataModel<vccomif::rdg::v1::interfaces::UploadRequestResponseNotificationRequestEncryption>::saveUpload(file_dir, *(iMsg.get()));
            }
        }
        break;

        case GRPC_IF_TYPE::DCIF_RDG160:
        {
            const std::shared_ptr<vccomif::rdg::v1::interfaces::UploadErrorDataRequestEncryption> iMsg {std::shared_ptr<vccomif::rdg::v1::interfaces::UploadErrorDataRequestEncryption>(new vccomif::rdg::v1::interfaces::UploadErrorDataRequestEncryption())};
            if (iMsg.get() != nullptr)
            {
                iMsg->set_encryption_type(iRdgEncryptionType);
                if ((iReqSerializedData->data() != nullptr) && (iReqSerializedData->size() > 0U))
                {
                    iMsg->set_request_body(iReqSerializedData->data(), iReqSerializedData->size());
                }
                fileSize = iMsg->ByteSizeLong();
                res = DataModel<vccomif::rdg::v1::interfaces::UploadErrorDataRequestEncryption>::saveUpload(file_dir, *(iMsg.get()));
            }
        }
        break;
        default:
        {
            (void)iReqSerializedData;
            (void)iRdgEncryptionType;
            LOG_E("MakeEncryptRequestMsg: invaild interface type");
        }
        break;
        }
    }
    else
    {
        (void)iReqSerializedData;
        (void)iRdgEncryptionType;
        LOG_E("MakeEncryptRequestMsg: fail to get encrypted data");
    }
    return res;
}


}
