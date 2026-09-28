#ifndef DTC_CRC_MANAGER
#define DTC_CRC_MANAGER

#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <Typedef.h>
#include <utils/RefBase.h>
#include <utils/Log.h>
#include <utils/Buffer.h>
#include "utils/CRC16.h"
#include "utils/CRC32.h"
// #include "RemoteDTC.h"
// #include "RemoteSSR.h"
// #include "RemoteRoB.h"
// #include "diagprocess/RoBSSR/RemoteRoBSSR.h"
#include "UdsMessage.h"
#include "utils/Logger.h"
#include "utils/FileUtil.h"
#include "RemoteDelegate.h"


namespace rdgapp {

class CRCManager : public android::RefBase {
public:

    explicit CRCManager(RemoteDelegate& parent);
    // explicit CRCManager(RemoteSSR& parent);
    // explicit CRCManager(RemoteRoB& parent);
    // explicit CRCManager(RemoteRoBSSR& parent);
    ~CRCManager() override = default;
    CRCManager(const CRCManager&) = default;
    CRCManager(CRCManager&&) = default;
    CRCManager& operator=(const CRCManager&) = default;
    CRCManager& operator=(CRCManager&&) = default;

    void readCRC16FromFile();
    void writeCRC16ToFile();
    void calculateCRC16Data();
    bool compareCrc16Value();
    bool compareCrc16ValueSSR(const uint64_t keyCompare);

    void readCRC32FromFile();
    void writeCRC32ToFile();
    void calculateCRC32Data();
    bool compareCrc32Value();
    uint32_t calculateCRC32ForCommon(const android::sp<UdsMessage> msg) const;
    uint32_t calculateCRC32ForCommon(const android::sp<::Buffer> buf) const;

    void requestRemoveCRCFile();
    void saveCRC16();
    void saveCRC32();

private:
    std::string PATH_CRC_FILE;
    uint16_t calculateCRC16ForCommon(const android::sp<UdsMessage> msg) const;
    std::map<uint64_t, uint16_t> mNewCRC16;
    std::map<uint64_t, uint16_t> mSavedCRC16;
    std::map<uint64_t, uint32_t> mNewCRC32;
    std::map<uint64_t, uint32_t> mSavedCRC32;
    // android::sp<GenericFile> mCrcFile;

    uint16_t mCurKeyValueForPhase5;
    uint8_t mNumberOfDTCForPhase5;
    uint8_t mCurDTCNumberForPhase5;

    RemoteDelegate& mParent;
    // RemoteDTC& mParent_DTC;
    // RemoteSSR& mParent_SSR;
    // RemoteRoB& mParent_RoB;
    // RemoteRoBSSR& mParent_RoBSSR;
    uint8_t mParent_APPID;
    bool mIsCRCChanged;
};
}
#endif // DTC_CRC_MANAGER
