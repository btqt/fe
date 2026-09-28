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
#include "DiagTrigger.h"
#ifndef ENABLE_LGE_LXC
#include <services/TimeManagerService/TimeManager.h>
#endif /* ENABLE_LGE_LXC */

namespace rdgapp {
class SerializeUint64 : public android::RefBase 
{
public:
    explicit SerializeUint64(const uint64_t aValue);

    SerializeUint64(const uint8_t *const serialivedData, const size_t lenght);
    ~SerializeUint64() final = default;

    const android::sp<::Buffer> getSerialized() noexcept;
    
    inline uint64_t getU64() const noexcept {return mU.valueU64;}
private:
    union U {
        uint64_t valueU64;
        uint8_t data[8];
    };

    U mU;
};

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
    static vccomif::common::v1::AppCommonHeaderVehicleToCenter_GeodesyInformation getGeodesyInfo();
    static std::string uint32ToHexString(const uint32_t value);
    static uint32_t hexStringToUint32(const std::string& hexStr);
    static uint8_t getOperation(const DiagTrigger::DiagTriggerType triggerType) noexcept;
    static int64_t getCurrentAcquisiteTime();
    static int32_t getTimeZoneOffsetHour();
    static int32_t getTimeZoneOffsetMinutes();
    static const uint64_t generateRandomUint64();
};
}
#endif // RDG_COMMON_UTILS_H_
