#ifndef REMOTEDIAGPROXY_LOGGER_H
#define REMOTEDIAGPROXY_LOGGER_H
#include <dlt/dlt.h>
#include <cxxabi.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string>
#include <memory>
#include <cstdarg>
#include <Log.h>
#include <application/Application.h>

#if defined (__linux__)
#define REMOTEDIAGPROXY_THREAD_ID DLT_CSTRING("["); DLT_INT32(syscall(SYS_gettid)); DLT_CSTRING("]")
#elif defined (__WIN32__)
#define REMOTEDIAGPROXY_THREAD_ID DLT_CSTRING("["); DLT_CSTRING("]")
#endif

#define FILENAME ((__builtin_strrchr(__FILE__, 47) != nullptr) ? (__builtin_strrchr(__FILE__, 47) + 1) : __FILE__)

#define REMOTEDIAGPROXY_FILE_NAME(file, func, line) DLT_CSTRING("["); DLT_CSTRING((file)); DLT_CSTRING(":"); DLT_CSTRING((func)); DLT_CSTRING(":"); DLT_INT32((line)); DLT_CSTRING("]")

#ifdef PERF_VERSION
#define REMOTEDIAGPROXY_LOG_HEADER(file, func, line)
#else
#define REMOTEDIAGPROXY_LOG_HEADER(file, func, line) REMOTEDIAGPROXY_THREAD_ID; REMOTEDIAGPROXY_FILE_NAME((file), (func), (line))
#endif

#define LOG_E(...) dltWrapper(DLT_LOG_ERROR, FILENAME, __builtin_FUNCTION(), __LINE__, ##__VA_ARGS__)
#define LOG_W(...) dltWrapper(DLT_LOG_WARN, FILENAME, __builtin_FUNCTION(), __LINE__, ##__VA_ARGS__)
#define LOG_I(...) dltWrapper(DLT_LOG_INFO, FILENAME, __builtin_FUNCTION(), __LINE__, ##__VA_ARGS__)
#define LOG_D(...) dltWrapper(DLT_LOG_DEBUG, FILENAME, __builtin_FUNCTION(), __LINE__, ##__VA_ARGS__)
#define LOG_V(...) dltWrapper(DLT_LOG_VERBOSE, FILENAME, __builtin_FUNCTION(), __LINE__, ##__VA_ARGS__)
#define LOG_MEM_DUMP_I(buffer) dltMemoryDump(DLT_LOG_INFO, (buffer))
#define LOG_MEM_DUMP_D(buffer) dltMemoryDump(DLT_LOG_DEBUG, (buffer))
#define LOG_MEM_DUMP_V(buffer) dltMemoryDump(DLT_LOG_VERBOSE, (buffer))



DLT_IMPORT_CONTEXT(ctxRemoteDiagProxyReg);

std::string composeMessage(const std::string& input, va_list ap);
void initDLTLog();
std::string generateJson(std::string jsonString);
// void printData(const std::string data); 
void printDataDebug(const std::string data);
#ifdef __cplusplus
extern "C" {
#endif
    inline void dltWrapper(const DltLogLevelType type, const std::string& file, const std::string& func, const int32_t line, const std::string input, ...)
    {
        va_list args;
        va_start(args, input);
        DLT_LOG(ctxRemoteDiagProxyReg, type, REMOTEDIAGPROXY_LOG_HEADER(file.c_str(), func.c_str(), line); DLT_STRING(composeMessage(input,args).c_str()));
        va_end(args);
    }

    inline void dltMemoryDump(const DltLogLevelType type, const android::sp<::Buffer>& buffer)
    {
        const uint32_t numBlock {buffer->size()/1024U};
        const uint16_t remainingByte {static_cast<uint16_t>(buffer->size() % 1024U)};
        for (uint32_t i {0U}; i < numBlock; i++) {
            ::Buffer block {};
            block.setTo(buffer->data() + (i*1024U), 1024);
            const uint32_t sizeOfBlock{block.size()};
            if (sizeOfBlock <= static_cast<uint32_t>(UINT16_MAX))
            {
                DLT_LOG_RAW(ctxRemoteDiagProxyReg, type, block.data(), static_cast<uint16_t>(sizeOfBlock));
            }
            else
            {
                LOG_E("size of block is exceed");
            }
        }
        DLT_LOG_RAW(ctxRemoteDiagProxyReg, type, buffer->data() + numBlock * 1024U, remainingByte);
        (void)remainingByte;
    }
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* REMOTEDIAGPROXY_LOGGER_H */
