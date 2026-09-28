#include <sys/statvfs.h>
#include "CommonUtils.h"
#include "services/OnboardclientManagerAdapter.h"
#include "Logger.h"
#include <cerrno>

namespace rdgapp
{

    uint32_t CommonUtils::calTargetAddressFromCanIdRx(const uint8_t protocolType, const uint32_t rxCanId)
    {
        uint32_t txCanId{0U};
        if ((protocolType == OBCEnum::OBCProtocolType::DOCAN29BIT) || (protocolType == OBCEnum::OBCProtocolType::DOCAN29BITCANFD))
        {
            const uint8_t byte1{static_cast<uint8_t>((rxCanId >> 8U) & 0xFFU)};
            const uint8_t byte2{static_cast<uint8_t>(rxCanId & 0xFFU)};
            txCanId = (rxCanId & 0xFFFF0000U) | static_cast<uint32_t>(static_cast<uint32_t>(byte2) << 8U) | static_cast<uint32_t>(byte1);
        }
        else if ((protocolType == OBCEnum::OBCProtocolType::DOCAN) || (protocolType == OBCEnum::OBCProtocolType::DOCAN11BITEX))
        {
            if (rxCanId < 0x80000U)
            {
                // print error log
            }
            else
            {
                txCanId = rxCanId - 0x80000U;
            }
        }
        else
        {
            LOG_D("Invalid Protocol Type!");
        }

        LOG_D("TX CAN ID: 0x%02X", txCanId);

        return txCanId;
    }

    uint32_t CommonUtils::getFreespace(const std::string &path)
    {
        struct statvfs fiData;
        uint32_t freespaceSize32{0U};
        if (statvfs(path.c_str(), &fiData) < 0)
        {
            LOG_E("Failed to stat: %s", path.c_str());
            freespaceSize32 = 0U;
        }
        else
        {
            LOG_D("Disk: %s", path.c_str());

            if ((fiData.f_bavail * 1024U) <= static_cast<uint64_t>(UINT32_MAX))
            {
                freespaceSize32 = static_cast<uint32_t>(fiData.f_bavail * 1024U);
            }
            else
            {
                freespaceSize32 = UINT32_MAX;
            }
            LOG_D("Freespace size: %lu Bytes", freespaceSize32);
        }
        return freespaceSize32;
    }

    uint8_t CommonUtils::convertStringToInt(const std::string &str)
    {
        uint32_t val{0U};
        uint8_t res{0xFFU};
        try
        {
            val = std::stoul(str, nullptr, 16);
        }
        catch (std::invalid_argument const& ex)
        {
            LOG_E("Invalid argument: %s", ex.what());
        }
        catch(std::out_of_range const& ex)
        {
            LOG_E("Out of range: %s", ex.what());
        }
        res = static_cast<uint8_t>(val & 0xFFU);
        return res;
    }

    void CommonUtils::convertCurrentTimeToBuffer(const android::sp<::Buffer> &timeData)
    {
        const struct tm stTimeData
        {
            TimeManager::getInstance().getCurrentTime() // LCOV_EXCL_BR_LINE
        };
        std::string strYear{};
        if (stTimeData.tm_year <= (INT32_MAX - 1900))
        {
            strYear = std::to_string((stTimeData.tm_year + 1900) % 100);
        }
        else
        {
            // print error log
        }
        std::string strMon{};
        if (stTimeData.tm_mon <= INT32_MAX - 1)
        {
            strMon = std::to_string(stTimeData.tm_mon + 1);
        }
        else
        {
            // print error log
        }
        const std::string strDay{std::to_string(stTimeData.tm_mday)};
        const std::string strHour{std::to_string(stTimeData.tm_hour)};
        const std::string strMin{std::to_string(stTimeData.tm_min)};
        const std::string strSec{std::to_string(stTimeData.tm_sec)};
        uint8_t hexYear{0U};
        uint8_t hexMon{0U};
        uint8_t hexDay{0U};
        uint8_t hexHour{0U};
        uint8_t hexMin{0U};
        uint8_t hexSec{0U};
        hexYear = convertStringToInt(strYear);
        hexMon = convertStringToInt(strMon);
        hexDay = convertStringToInt(strDay);
        hexHour = convertStringToInt(strHour);
        hexMin = convertStringToInt(strMin);
        hexSec = convertStringToInt(strSec);
        timeData->append(&hexYear, 1);
        timeData->append(&hexMon, 1);
        timeData->append(&hexDay, 1);
        timeData->append(&hexHour, 1);
        timeData->append(&hexMin, 1);
        timeData->append(&hexSec, 1);
        LOG_D("hexYear: 0x%02X hexMon: 0x%02X hexDay: 0x%02X hexHour: 0x%02X hexMin: 0x%02X hexSec: 0x%02X",
              hexYear, hexMon, hexDay, hexHour, hexMin, hexSec);
    }

    uint32_t CommonUtils::makeSerializeUint32(const android::sp<::Buffer> &buf, const uint32_t pos)
    {
        union
        {
            uint32_t value;
            uint8_t data[4];
        } temp;
        std::vector<uint8_t> tempData{};
        (void)tempData.insert(tempData.cend(), buf->data() + pos, buf->data() + pos + 4U);
        temp.data[0] = tempData[3];
        temp.data[1] = tempData[2];
        temp.data[2] = tempData[1];
        temp.data[3] = tempData[0];
        return temp.value;
    }

    uint16_t CommonUtils::makeSerializeUint16(const android::sp<::Buffer> &buf, const uint32_t pos)
    {
        std::vector<uint8_t> tempData{};
        (void)tempData.insert(tempData.cend(), buf->data() + pos, buf->data() + pos + 2U);
        union
        {
            uint16_t value;
            uint8_t data[2];
        } temp;
        temp.data[0] = tempData[1];
        temp.data[1] = tempData[0];
        return temp.value;
    }

    std::string CommonUtils::intergerToLeadingZeroString(const int32_t number)
    {
        std::stringstream ss{};
        ss << std::setw(2) << std::setfill('0') << number;
        return ss.str();
    }

    std::string CommonUtils::setUploadMessId(const vccomif::rdg::v1::interfaces::RdgCommonRequestHeader_InterfaceType interfaceType, const uint32_t counterValue)
    {
        // TimeManager &mTimeManagerService{TimeManager::getInstance()};
        const tm stTimeData{TimeManager::getInstance().getCurrentTime()};
        const std::string strYear{std::to_string((stTimeData.tm_year <= INT32_MAX - 1900) ? stTimeData.tm_year + 1900 : 0)};
        const std::string strMon{intergerToLeadingZeroString((stTimeData.tm_mon < 12) ? stTimeData.tm_mon + 1 : 0)};
        const std::string strDay{intergerToLeadingZeroString(stTimeData.tm_mday)};
        const std::string strHour{intergerToLeadingZeroString(stTimeData.tm_hour)};
        const std::string strMin{intergerToLeadingZeroString(stTimeData.tm_min)};
        const std::string strSec{intergerToLeadingZeroString(stTimeData.tm_sec)};
        int32_t tempCounterValue {0};
        if (counterValue > static_cast<uint32_t>(INT32_MAX))
        {
            tempCounterValue = INT32_MAX;
        } else {
            tempCounterValue = static_cast<int32_t>(counterValue);
        }
        const std::string messId{intergerToLeadingZeroString(static_cast<int32_t>(interfaceType)) + "-" + strYear + strMon + strDay + strHour + strMin + strSec + "-" + intergerToLeadingZeroString(tempCounterValue)};
        return messId;
    }
}
