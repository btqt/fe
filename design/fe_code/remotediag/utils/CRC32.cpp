#include "CRC32.h"
#include "CRCTypeDef.h"

namespace rdgapp{
uint32_t CRC32::makeCRC32(const uint8_t* const buf, const uint32_t len, const uint32_t defaultCRC) noexcept {
    uint32_t crc {defaultCRC ^ 0xFFFFFFFF};
    if (buf != nullptr) {
        for (uint32_t counter {0U}; counter < len; counter++) {
            crc = ((crc << 8U) & 0xFFFFFF00) ^ crc32Table[(((crc >> 24U)&0xFFU) ^ (buf[counter]))];
        }
    }
    return crc;
}
uint32_t CRC32::makeCRC32(const android::sp<Buffer> buf) {
    uint32_t crc {0U};
    const uint32_t len {buf->size()};
    const uint8_t* const data {buf->data()};
    if ((len > 0U) && (data != nullptr)) {
        for (uint32_t counter {0U}; counter < len; counter++) {
            crc = ((crc << 8U) & 0xFFFFFF00) ^ crc32Table[(((crc >> 24U)&0xFFU) ^ (data[counter]))];
        }
    }
    (void)data;
    return crc;
}
}
