#ifndef RDG_COMMON_UTILS_H_
#define RDG_COMMON_UTILS_H_
#include <iostream>
#include <algorithm>
#include <memory>
#include <utils/RefBase.h>
#include <utils/Timer.h>
#include <utils/Buffer.h>

#include "ParamsDef.h"
#include "common_def.h"

namespace rdgapp {

class CommonUtils
{
public:
    static uint32_t calTargetAddressFromCanIdRx(const uint8_t protocolType, const uint32_t rxCanId);
    static uint32_t getFreespace(const std::string& path);
    static void convertCurrentTimeToBuffer(const android::sp<::Buffer> &timeData);
    static uint8_t convertStringToInt(const std::string& str);
    static uint32_t makeSerializeUint32(const android::sp<::Buffer>& buf, const uint32_t pos);
    static uint16_t makeSerializeUint16(const android::sp<::Buffer>& buf, const uint32_t pos);
    static std::string setUploadMessId(const vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType interfaceType, const uint32_t counterValue);
    static std::string intergerToLeadingZeroString(const int32_t number);
};
}
#endif // RDG_COMMON_UTILS_H_
