#include "CRC16.h"
#include "CRCTypeDef.h"

namespace rdgapp{
uint16_t CRC16::makeCRC16(const uint8_t* const buf, const uint32_t len, const uint16_t defaultCRC) noexcept {
    uint32_t crc {static_cast<uint32_t>(defaultCRC)};
    if (buf != nullptr) {
        for (uint32_t counter {0U}; counter < len; counter++) {
            crc = ((crc << 8U) & 0x0000FF00U) ^ crc16Table[(((crc >> 8U) & 0xFFU) ^ (buf[counter]))];
        }
    }
    uint16_t res{0U};
    if(crc <= static_cast<uint32_t>(UINT16_MAX)){
        res = static_cast<uint16_t>(crc);
    }
    return res;
}
uint16_t CRC16::makeCRC16(const android::sp<Buffer> buf) {
    uint32_t crc {0U};
    const uint32_t len {buf->size()};
    const uint8_t* const data {buf->data()};
    if ((len > 0U) && (data != nullptr)) {
        for (uint32_t counter {0U}; counter < len; counter++) {
            crc = ((crc << 8U) & 0x0000FF00U) ^ crc16Table[(((crc >> 8U) & 0xFFU) ^ (data[counter]))];
        }
    }
    uint16_t res{0U};
    if(crc <= static_cast<uint32_t>(UINT16_MAX)){
        res = static_cast<uint16_t>(crc);
    }
    (void)data;
    return res;
}
}
