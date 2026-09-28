
#ifndef RDG_DIAG_CRC16_H
#define RDG_DIAG_CRC16_H

#include <Typedef.h>
#include "Logger.h"
#include <utils/Buffer.h>
#include <utils/RefBase.h>

namespace rdgapp {

class CRC16 {
public:
    static uint16_t makeCRC16(const uint8_t* buf, const uint32_t len, const uint16_t defaultCRC = 0) noexcept;
    static uint16_t makeCRC16(const android::sp<Buffer> buf);
};
}
#endif //RDG_DIAG_CRC16_H
