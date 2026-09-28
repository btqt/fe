#ifndef REMOTEDIAG_UDS_SID_FILTER_H
#define REMOTEDIAG_UDS_SID_FILTER_H

#include <cstdint>

namespace rdgapp
{

    /**
     * SID filter for diag commands received from center.
     * Blocks reprogramming-related UDS requests before they reach
     * PriorityControl / OnboardClient.
     */
    class SidFilter final
    {
    public:
        SidFilter() = delete;

        /**
         * @brief Check whether a UDS request from center is allowed
         * @param data Raw UDS request bytes (data[0] = SID)
         * @param size Number of bytes in the request
         * @return true if allowed, false if the request is reprogramming-related
         */
        static bool isAllowed(const uint8_t *data, const uint32_t size) noexcept;
    };

} // namespace rdgapp

#endif // REMOTEDIAG_UDS_SID_FILTER_H
