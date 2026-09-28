#include "SidFilter.h"
#include "UdsMessageDefine.h"
#include "utils/Logger.h"

namespace rdgapp
{

    namespace
    {
        // Programming session (SID 0x10, sub-function 0x02), suppressPosRspMsgIndicationBit ignored
        constexpr uint8_t SFID_02_PROGRAMMING_SESSION{0x02U};
        constexpr uint8_t SFID_SUPPRESS_POS_RSP_BIT_MASK{0x7FU};
    }

    bool SidFilter::isAllowed(const uint8_t *data, const uint32_t size) noexcept
    {
        if ((data == nullptr) || (size == 0U))
        {
            return false;
        }

        bool allowed{true};
        const uint8_t sid{data[SID_BYTE_MASK]};
        switch (static_cast<UDS_SID>(sid))
        {
        case UDS_SID::SID_11_ECU_RESET:
        case UDS_SID::SID_28_COMMUNICATION_CONTROL:
        case UDS_SID::SID_34_REQUEST_DOWNLOAD:
        case UDS_SID::SID_85_CONTROL_DTC_SETTING:
            allowed = false;
            break;
        case UDS_SID::SID_10_SESSION_CONTROL:
            if (size > SFID_BYTE_MASK)
            {
                const uint8_t sfid{static_cast<uint8_t>(data[SFID_BYTE_MASK] & SFID_SUPPRESS_POS_RSP_BIT_MASK)};
                allowed = (sfid != SFID_02_PROGRAMMING_SESSION);
            }
            break;
        default:
            break;
        }

        if (!allowed)
        {
            LOG_I("SidFilter: blocked reprogramming-related request SID %02X", sid);
        }
        return allowed;
    }

} // namespace rdgapp
