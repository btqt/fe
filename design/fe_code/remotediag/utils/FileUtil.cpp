#include <iostream>
#include <cerrno>
#include "Logger.h"
#include "FileUtil.h"

namespace rdgapp {

std::map<FILE* const, OPEN_FILE_MODE> FileUtil::m_FileHandles{};

/**
 * \brief Remove file.
 *
 * Function remove a file with given path.
 *
 * \param[in]      path         complete path of directory or file
 *
 * \retval        true  file is removed success.
 * \retval        false file is removed failed.
 * \remarks none
 *
 * \lhref
 *
 */
bool FileUtil::removeFile(const std::string filepath) {
    bool res{true};
    const int32_t err{remove(filepath.c_str())};
    if (err != 0) {
        LOG_I("[removeFile] fail to delete file (%s)", filepath.c_str());
        res = false;
    }
    return res;
}

/**
 * \brief Check path is exist or not.
 *
 * Function check path is exist or not with given path.
 *
 * \param[in]      path         complete path of directory or file
 *
 * \retval        true  file is exist
 * \retval        false file is not exist
 * \remarks none
 *
 * \lhref
 *
 */
bool FileUtil::isPathExist(const std::string path) noexcept {
    bool res{true};
    struct stat buffer;
    const int32_t err{stat (path.c_str(), &buffer)};
    if(err != 0){
        res = false;
    }
    return res;
}

/**
 * \brief Make directory.
 *
 * Function make directory with given path.
 *
 * \param[in]      path         complete path of directory
 *
 * \retval        true  everything is ok
 * \retval        false error
 * \remarks none
 *
 * \lhref
 *
 */
bool FileUtil::makeDir(const std::string path) noexcept {
    bool res{true};
    const int32_t err {mkdir(path.c_str(), 511U)};
    if ((err < 0) && (errno != EEXIST)) {
        // cannot assure target dir.
        res = false;
    }
    return res;
}

/**
 * \brief Open file.
 *
 * Function opens file with given open mode.
 *
 * \param[in]      path         complete path of file name
 * \param[in]      mode         open mode
 *
 * \retval        NULL      error
 * \retval        Not NULL  everything is ok
 * \remarks none
 *
 * \lhref
 *
 */
FileHandleType FileUtil::openFile(const std::string path, const OPEN_FILE_MODE mode) {
    FileHandleType fileptr{nullptr};
    // check input parameters
    /* File Mode */
    char_t openMode[3]{0};
    bool isModeValid{true};
    switch (mode) {
    case OPEN_FILE_MODE::OPEN_FILE_MODE_READ_TXT:
    case OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN:
        (void)memcpy(&openMode[0], "rb", 2U);
        break;
    case OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_TXT:
        (void)memcpy(&openMode[0], "w", 1U);
        break;
    case OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_BIN:
        (void)memcpy(&openMode[0], "wb", 2U);
        break;
    case OPEN_FILE_MODE::OPEN_FILE_MODE_APPEND_TXT:
    case OPEN_FILE_MODE::OPEN_FILE_MODE_APPEND_BIN:
        /* In order to update and overwrite the existing file content we use r+ mode here. */
        (void)memcpy(&openMode[0], "r+", 2U);
        break;
    default:
        isModeValid = false;
        break;
    }
    if(isModeValid){
        /* Open File */
        FILE* const file {fopen(path.c_str(), openMode)};
        /* Check error*/
        if (file != nullptr) {
            // register
            m_FileHandles[file] = mode;
            fileptr = static_cast<FileHandleType>(file);
        }
        else{
            LOG_E("file is nullptr");
        }
    }
    return fileptr;
}

/**
 * \brief Close file.
 *
 * Function closes file with given open mode.
 *
 * \param[in]      fileHdl         file handle of file
 *
 * \retval        True      everything is ok
 * \retval        False     error
 * \remarks none
 *
 * \lhref
 *
 */
bool FileUtil::closeFile(const FileHandleType fileHdl) {
    bool res{true};
    /* Check input parameter */
    /* Check file list */
    if ((fileHdl == nullptr) || (m_FileHandles.empty())){
        res = false;
    }
    else{
        /* Find the file handle in the list */
        const std::map<FILE* const, OPEN_FILE_MODE>::iterator it {m_FileHandles.find(static_cast<FILE* const>(fileHdl))};
        /* Not found */
        if (it == m_FileHandles.end()) {
            res = false;
        }
        else{
            (void)fclose(it->first);
            (void)m_FileHandles.erase(it);
        }
    }
    return res;
}

/**
 * \brief Append binary content in the end of file.
 *
 * Function appends binary content in the end of file.
 * The file must be opened with Mode OPEN_FILE_MODE_WRITE_BIN.
 * For other open modes this function will return FALSE back.
 * After call of this function given string to write must be flushed into file immediately.
 *
 * \param[in]     fileHdl         file handle.
 * \param[in]     buffer          binary content to write into file
 * \param[in]     bufferSize      size of binary content
 *
 * \retval        true  everything is ok
 * \retval        false error
 * \remarks none
 *
 * \lhref
 *
 */
bool FileUtil::appendFileBin(const FileHandleType fileHdl, const uint8_t* const buf, const uint32_t bufferSize) {
    bool res{true};
    LOG_I("appendFileBin: start ....");
    /* Check file handle */
    if (fileHdl == nullptr) {
        LOG_E("[appendFileBin] file handle is null");
        res = false;
    }
    // validate input parameters
    if (res && ((buf == nullptr) || (bufferSize < 1U))) {
        LOG_E("[appendFileBin] invalid input parameters");
        res = false;
    }
    if(res && (fseek(static_cast<FILE*>(fileHdl), 0, SEEK_END) != 0)){
        LOG_E("[appendFileBin] fail to move end of file");
        res = false;
    }
    if(res){
        res = writeBinToFile(fileHdl, buf, bufferSize);
    }
    return res;
}

bool FileUtil::writeBinToFile(const FileHandleType fileHdl, const uint8_t* const buf, const uint32_t bufferSize)
{
    bool res{true};
    if (fileHdl != nullptr) {
        const uint32_t buf_size{fwrite(buf, sizeof(uint8_t), bufferSize, static_cast<FILE*>(fileHdl))};
        if(buf_size == bufferSize){
            if(fflush(static_cast<FILE*>(fileHdl)) != 0){
                res = false;
                LOG_E("fail to fflush changes to file");
            }
        }
        else{
            res = false;
            LOG_E("fail to write buffer to file");
        }
    }
    else{
        LOG_E("file handle is null");
    }
    return res;
}

bool FileUtil::ReadBinFromFile(const FileHandleType fileHdl, uint8_t* const buf, const uint32_t bufferSize)
{
    bool res{true};
    if (fileHdl != nullptr) {
        if(fread(buf, sizeof(uint8_t), bufferSize, static_cast<FILE*>(fileHdl)) == bufferSize){
            if(fflush(static_cast<FILE*>(fileHdl)) != 0){
                res = false;
                LOG_E("fail to fflush changes to file");
            }
        }
        else{
            res = false;
            LOG_E("fail to read buffer from file");
        }
    }
    else{
        LOG_E("file handle is null");
    }
    return res;
}

int32_t FileUtil::getFileDescriptor(const FileHandleType aHandle)
{
    const int32_t res{fileno(static_cast<FILE*>(aHandle))};
    return res;
}
}
