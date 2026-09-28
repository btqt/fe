
#ifndef RDG_DIAG_CRC32_H
#define RDG_DIAG_CRC32_H

#include <Typedef.h>
#include <utils/Buffer.h>
#include <utils/RefBase.h>

namespace rdgapp {

class CRC32 {
public:
    static uint32_t makeCRC32(const uint8_t* const buf, const uint32_t len, const uint32_t defaultCRC = 0) noexcept;
    static uint32_t makeCRC32(const android::sp<Buffer> buf);
};
}
#endif //RDG_DIAG_CRC32_H
