/*-----------------------------------------------------------------------------
    Header File Guarder
------------------------------------------------------------------------------*/
#ifndef FILE_UTIL_H
#define FILE_UTIL_H

/*-----------------------------------------------------------------------------
    File Inclusions
------------------------------------------------------------------------------*/
#include <cstdbool>
#include <sys/stat.h>
#include <map>
#include <Typedef.h>
// For using LGEFileIO library
#define USE_LGEFILEIO
#include <lgefileio.h>

namespace rdgapp {

/*-----------------------------------------------------------------------------
    Constant Definitions
------------------------------------------------------------------------------*/
enum class OPEN_FILE_MODE : uint8_t {
  OPEN_FILE_MODE_READ_TXT = 0,   /**< File open mode for reading text content from file. */
  OPEN_FILE_MODE_READ_BIN = 1,   /**< File open mode for reading binary content from file. */
  OPEN_FILE_MODE_WRITE_TXT = 2,  /**< File open mode for writing text content into file. */
  OPEN_FILE_MODE_WRITE_BIN = 3,  /**< File open mode for writing binary content into file. */
  OPEN_FILE_MODE_APPEND_TXT = 4, /**< File open mode for appending text content into file. */
  OPEN_FILE_MODE_APPEND_BIN = 5  /**< File open mode for appending binary content into file. */
};

/*-----------------------------------------------------------------------------
    Type Definitions
------------------------------------------------------------------------------*/

/* File handle type */
using FileHandleType = void*;

/*-----------------------------------------------------------------------------
    Function Declaration
------------------------------------------------------------------------------*/
/**
 * @brief FileUtil is Utility class in ProgManager for handing file in/out operations.
 *
 **/
class FileUtil {
    public :
        FileUtil(void) = default;
        virtual ~FileUtil() = default;
    
    public :
        static bool removeFile(const std::string filepath);
        static bool isPathExist(const std::string path) noexcept;
        static bool makeDir(const std::string path) noexcept;
        static FileHandleType openFile(const std::string path, const OPEN_FILE_MODE mode);
        static int32_t getFileDescriptor(const FileHandleType aHandle);
        static bool closeFile(const FileHandleType fileHdl);
        static bool appendFileBin(const FileHandleType fileHdl, const uint8_t* const buf, const uint32_t bufferSize);
        static bool writeBinToFile(const FileHandleType fileHdl, const uint8_t* const buf, const uint32_t bufferSize);
        static bool ReadBinFromFile(const FileHandleType fileHdl, uint8_t* const buf, const uint32_t bufferSize);

        static std::map<FILE* const, OPEN_FILE_MODE> m_FileHandles;
};
}
#endif /*FILE_UTIL_H*/
