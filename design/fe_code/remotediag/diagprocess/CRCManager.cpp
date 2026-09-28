#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <Typedef.h>

#include "CRCManager.h"
#include "ParamsDef.h"

namespace rdgapp {
CRCManager::CRCManager(RemoteDelegate& parent)
        : android::RefBase()
        , mNewCRC16{}
        , mSavedCRC16{}
        , mSavedCRC32{}
        , mCurKeyValueForPhase5{0U}
        , mNumberOfDTCForPhase5{0U}
        , mCurDTCNumberForPhase5{0U}
        , mParent{parent}{
        LOG_I("CRC object for RemoteDelegate, AppId = %d", static_cast<uint8_t>(mParent.getAppId()));
        mParent_APPID = mParent.getAppId();
        switch(mParent.getAppId())
        {
            case RDG_APPID::DTC:
                PATH_CRC_FILE = "/data/rdg/dtc.crc";
                break;
            case RDG_APPID::ROB:
                PATH_CRC_FILE = "/data/rdg/rob.crc";
                break;
            case RDG_APPID::SSR:
                PATH_CRC_FILE = "/data/rdg/ssr.crc";
                break;
            case RDG_APPID::ROBSSR:
                PATH_CRC_FILE = "/data/rdg/robssr.crc";
                break;
            default:
                break;
        }
}

// CRCManager::CRCManager(RemoteSSR& parent)
//         : android::RefBase()
//         , mNewCRC16{}
//         , mSavedCRC16{}
//         , mSavedCRC32{}
//         , mCurKeyValueForPhase5{0U}
//         , mNumberOfDTCForPhase5{0U}
//         , mCurDTCNumberForPhase5{0U}
//         // , mParent_SSR{parent}
//         , mParent{parent} {
//         LOG_I("CRC object for SSR");
//         // mParent_APPID = mParent_SSR.getAppId();
//         mParent_APPID = mParent.getAppId();
//         PATH_CRC_FILE = "/data/rdg/ssr.crc";
// }

// CRCManager::CRCManager(RemoteRoB& parent)
//         : android::RefBase()
//         , mNewCRC16{}
//         , mSavedCRC16{}
//         , mSavedCRC32{}
//         , mCurKeyValueForPhase5{0U}
//         , mNumberOfDTCForPhase5{0U}
//         , mCurDTCNumberForPhase5{0U}
//         // , mParent_RoB{parent} 
//         , mParent{parent} {
//         LOG_I("CRC object for RoB");
//         // mParent_APPID = mParent_RoB.getAppId();
//         mParent_APPID = mParent.getAppId();

//         PATH_CRC_FILE = "/data/rdg/rob.crc";
// }

// CRCManager::CRCManager(RemoteRoBSSR& parent)
//         : android::RefBase()
//         , mNewCRC16{}
//         , mSavedCRC16{}
//         , mCurKeyValueForPhase5{0U}
//         , mNumberOfDTCForPhase5{0U}
//         , mCurDTCNumberForPhase5{0U}
//         // , mParent_RoBSSR{parent}
//         , mParent{parent} {
//         LOG_I("CRC object for RoBSSR");
//         // mParent_APPID = mParent_RoBSSR.getAppId();
//         mParent_APPID = mParent.getAppId();
//         PATH_CRC_FILE = "/data/rdg/robssr.crc";
// }

void CRCManager::readCRC16FromFile() {
    LOG_I("Read CRC from file: %s", PATH_CRC_FILE.c_str());
    /* Get file size*/
    uint32_t size{0U};
    std::ifstream file{std::ifstream(PATH_CRC_FILE.c_str(), std::ios::binary | std::ios::ate)};
    const int64_t tmp_tell{file.tellg()};
    if((tmp_tell>=0) && (tmp_tell <= static_cast<int64_t>(UINT32_MAX)))
    {
        size = static_cast<uint32_t>(tmp_tell);
    }
    else
    {
        // do nothing
    }
    file.close();
    LOG_I("CHECK CRC size: %d", size);
    /* Read file content */
    const FileHandleType fileHdl {FileUtil::openFile(PATH_CRC_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN)};
    if(fileHdl != nullptr) {
        uint8_t mBuffer[size];
        const bool success{FileUtil::ReadBinFromFile(fileHdl, &mBuffer[0], size)};
        if(success == true) {
            LOG_I("Read file success");
            for (uint32_t i{0U}; i < size; i += sizeof(uint64_t) + sizeof(uint16_t)) {
                uint64_t key {0U};
                (void)std::memcpy(&key, &mBuffer[i], sizeof(uint64_t));
                uint16_t value {0U};
                (void)std::memcpy(&value, &mBuffer[i + sizeof(uint64_t)], sizeof(uint16_t));
                mSavedCRC16[key] = value;
                LOG_I("Check key: 0x%02llx CRC value: 0x%02x", key, value);
            }
        }
        (void)FileUtil::closeFile(fileHdl);
    } else {
        LOG_I("fileHdl is nullptr");
    }
    LOG_I("End read CRC");
}

void CRCManager::writeCRC16ToFile() {
    LOG_I("Write DTC crc to file %s", PATH_CRC_FILE.c_str());

    const FileHandleType fileHdl {FileUtil::openFile(PATH_CRC_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_BIN)};
    if(fileHdl != nullptr) {
        struct Crc16Pair {
        uint64_t first;
        uint16_t second;
        }__attribute__((packed));
        // std::vector<std::pair<uint32_t, uint16_t>> vSavedCRC(mSavedCRC16.begin(), mSavedCRC16.end());
        std::vector<Crc16Pair> vSavedCRC{};
        for(std::map<uint64_t, uint16_t>::iterator it {mSavedCRC16.begin()}; it !=mSavedCRC16.end(); it++ ) {
            Crc16Pair p;
            p.first = it->first;
            p.second = it->second;
            vSavedCRC.push_back(p);
            LOG_I("Check size of pair: %d", sizeof(p));
        }
        LOG_I("Check vSavedCRC size: %d", vSavedCRC.size());
        const uint32_t bufferSize {vSavedCRC.size() * sizeof(Crc16Pair)};
        LOG_I("Check bufferSize: %d", bufferSize);
        uint8_t mBuffer[bufferSize];
        (void)std::memcpy(&mBuffer[0], vSavedCRC.data(), bufferSize);

        const bool success {FileUtil::writeBinToFile(fileHdl, &mBuffer[0], bufferSize)};
        if (success != true) {
            LOG_I("Failed to write to file.");
        }
        (void)FileUtil::closeFile(fileHdl);
        /*TBD: Save RDG flag into DID */
    } else {
        LOG_I("fileHdl is nullptr");
    }
    return;
}

uint16_t CRCManager::calculateCRC16ForCommon(const android::sp<UdsMessage> msg) const {
    uint16_t crcValue{0U};
    const uint32_t num {msg->ToUdsData()->size()};
    for(uint32_t i{0U}; i< num; i++) {
        LOG_I("Check UDS payload[%d]: 0x%02x", i, msg->ToUdsData()->data()[i]);
    }   
    crcValue = CRC16::makeCRC16(msg->ToUdsData()->data(), msg->ToUdsData()->size(), crcValue);
    return crcValue;
}

uint32_t CRCManager::calculateCRC32ForCommon(const android::sp<UdsMessage> msg) const {
    uint32_t crcValue{0U};
    const uint32_t num {msg->ToUdsData()->size()};
    for(uint32_t i {0U}; i< num; i++) {
        LOG_I("Check UDS payload[%d]: 0x%02x", i, msg->ToUdsData()->data()[i]);
    }   
    crcValue = CRC32::makeCRC32(msg->ToUdsData()->data(), msg->ToUdsData()->size(), crcValue);
    return crcValue;
}

void CRCManager::calculateCRC16Data() {
    uint16_t crcValue {0U};
    (void)mNewCRC16.clear();
    std::map<uint64_t, android::sp<UdsMessage>> respList{};
    if(mParent_APPID == RDG_APPID::DTC) {
        LOG_I("calculateCRC16Data for DTC");
        // respList = mParent_DTC.getDiagResponseList();
        respList = mParent.getDiagResponseList();
    } else if(mParent_APPID == RDG_APPID::SSR) {
        LOG_I("calculateCRC16Data for SSR");
        // respList = mParent_SSR.getDiagResponseList();
        respList = mParent.getDiagResponseList();
    } else if (mParent_APPID == RDG_APPID::ROB) {
        LOG_I("calculateCRC16Data for RoB");
        // respList = mParent_RoB.getDiagResponseList();
        respList = mParent.getDiagResponseList();
    } else if (mParent_APPID == RDG_APPID::ROBSSR) {
        LOG_I("Don't caculte CRC16 for this function");
    } else {
        LOG_I("Undefined function");
    }
    
    for (std::map<uint64_t, android::sp<UdsMessage>>::const_iterator it{respList.cbegin()}; it != respList.cend(); ++it) {
        crcValue = calculateCRC16ForCommon(it->second);
        LOG_I("Key: 0x%02llx CRC: 0x%02x", it->first, crcValue);
        const uint64_t temp_It{it->first};
        mNewCRC16[temp_It] = crcValue;
    }
    mCurKeyValueForPhase5 = 0x00U;
    LOG_I("Latest CRC map size is %d", mNewCRC16.size());
    (void) crcValue;
}

bool CRCManager::compareCrc16Value() {
    bool isDifferent {false};
    for (std::map<uint64_t, uint16_t>::iterator it {mNewCRC16.begin()}; it != mNewCRC16.end(); ++it) {
        const std::map<uint64_t, uint16_t>::iterator savedIt {mSavedCRC16.find(it->first)};
        if (savedIt != mSavedCRC16.end()) {
            LOG_D("Found CRC key: 0x%02llx  New_CRC_value: 0x%x Old_CRC_value: 0x%x", it->first, it->second, savedIt->second);
            if (savedIt->second != it->second) {
                isDifferent = true;
                savedIt->second = it->second;
            }
        } else {
            LOG_D(" Do not found CRC key: 0x%02llx ", it->first);
            isDifferent = true;
            (void)mSavedCRC16.emplace(it->first, it->second);
        }
    }

    LOG_I({"CRC value is changed? %d"}, isDifferent);
    if (isDifferent) {
        writeCRC16ToFile();
        readCRC16FromFile();
    }
    return isDifferent;
}

void CRCManager::readCRC32FromFile() {
    LOG_I("Read CRC from file: %s", PATH_CRC_FILE.c_str());
    /* Get file size*/
    uint32_t size{0U};
    std::ifstream file{std::ifstream(PATH_CRC_FILE.c_str(), std::ios::binary | std::ios::ate)};
    const int64_t tmp_tell{file.tellg()};
    if((tmp_tell>=0) && (tmp_tell <= static_cast<int64_t>(UINT32_MAX)))
    {
        size = static_cast<uint32_t>(tmp_tell);
    }
    else
    {
        // do nothing
    }
    file.close();
    LOG_I("CHECK CRC size: %d", size);
    /* Read file content */
    const FileHandleType fileHdl {FileUtil::openFile(PATH_CRC_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_READ_BIN)};
    if(fileHdl != nullptr) {
        uint8_t mBuffer[size];
        const bool success {FileUtil::ReadBinFromFile(fileHdl, &mBuffer[0], size)};
        if(success == true) {
            LOG_I("Read file success");
            for (uint32_t i {0U}; i < size; i += sizeof(uint64_t) + sizeof(uint32_t)) {
                uint64_t key {0U};
                (void)std::memcpy(&key, &mBuffer[i], sizeof(uint64_t));
                uint16_t value {0U};
                (void)std::memcpy(&value, &mBuffer[i + sizeof(uint64_t)], sizeof(uint16_t));
                mSavedCRC32[key] = value;
                LOG_I("Check key: 0x%02llx CRC value: 0x%02x", key, value);
            }
        }
        (void)FileUtil::closeFile(fileHdl);
    } else {
        LOG_I("fileHdl is nullptr");
    }
    LOG_I("End read CRC");
}

void CRCManager::writeCRC32ToFile() {
    LOG_I("Write DTC crc to file %s", PATH_CRC_FILE.c_str());

    const FileHandleType fileHdl {FileUtil::openFile(PATH_CRC_FILE.c_str(), OPEN_FILE_MODE::OPEN_FILE_MODE_WRITE_BIN)};
    if(fileHdl != nullptr) {
        struct Crc32Pair {
        uint64_t first;
        uint32_t second;
        }__attribute__((packed));
        std::vector<Crc32Pair> vSavedCRC{};
        for(std::map<uint64_t, uint32_t>::iterator it {mSavedCRC32.begin()}; it !=mSavedCRC32.end(); it++ ) {
            Crc32Pair p;
            p.first = it->first;
            p.second = it->second;
            vSavedCRC.push_back(p);
            LOG_I("Check size of pair: %d", sizeof(p));
        }
        LOG_I("Check vSavedCRC size: %d", vSavedCRC.size());
        const uint32_t bufferSize {vSavedCRC.size() * sizeof(Crc32Pair)};
        LOG_I("Check bufferSize: %d", bufferSize);
        uint8_t mBuffer[bufferSize];
        (void)std::memcpy(&mBuffer[0], vSavedCRC.data(), bufferSize);

        const bool success {FileUtil::writeBinToFile(fileHdl, &mBuffer[0], bufferSize)};
        if (success != true) {
            LOG_I("Failed to write to file.");
        }
        (void)FileUtil::closeFile(fileHdl);
        /*TBD: Save RDG flag into DID */
    } else {
        LOG_I("fileHdl is nullptr");
    }
    return;
}

void CRCManager::calculateCRC32Data() {
    uint32_t crcValue {0U};
    (void)mNewCRC32.clear();
    if(mParent_APPID == RDG_APPID::ROBSSR) {
        LOG_I("calculateCRC32Data for RoBSSR");
        std::map<uint64_t, android::sp<UdsMessage>> respList{};
        // respList = mParent_RoBSSR.getDiagResponseList();
        respList = mParent.getDiagResponseList();
        for (std::map<uint64_t, android::sp<UdsMessage>>::const_iterator it{respList.cbegin()}; it != respList.cend(); ++it) {
            crcValue = calculateCRC32ForCommon(it->second);
            LOG_I("Key: 0x%02llx CRC: 0x%02x", it->first, crcValue);
            const uint64_t temp_It{it->first};
            mNewCRC32[temp_It] = crcValue;
        }
        mCurKeyValueForPhase5 = 0x00U;
        LOG_I("Latest CRC map size is %d", mNewCRC32.size());

    } else if ((mParent_APPID == RDG_APPID::DTC) ||
            (mParent_APPID == RDG_APPID::ROB) ||
            (mParent_APPID == RDG_APPID::SSR)) {
        LOG_I("Don't caculate CRC32 for this function");
    } else {
        LOG_I("Undefined function");
    }
    (void) crcValue;
}

bool CRCManager::compareCrc32Value() {
    bool isDifferent {false};
    for (std::map<uint64_t, uint32_t>::iterator it {mNewCRC32.begin()}; it != mNewCRC32.end(); ++it) {
        const std::map<uint64_t, uint32_t>::iterator savedIt {mSavedCRC32.find(it->first)};
        if (savedIt != mSavedCRC32.end()) {
            LOG_D("Found CRC key: 0x%02llx  New_CRC_value: 0x%x Old_CRC_value: 0x%x", it->first, it->second, savedIt->second);
            if (savedIt->second != it->second) {
                isDifferent = true;
                savedIt->second = it->second;
            }
        } else {
            LOG_D(" Do not found CRC key: 0x%02llx ", it->first);
            isDifferent = true;
            (void)mSavedCRC32.emplace(it->first, it->second);
        }
    }

    LOG_I({"CRC value is changed? %d"}, isDifferent);
    if (isDifferent) {
        writeCRC32ToFile();
        readCRC32FromFile();
    }
    return isDifferent;
}


void CRCManager::requestRemoveCRCFile() {
    LOG_I("Delete Saved CRC value AppId = %d", mParent_APPID);
    switch(mParent_APPID)
    {
        case RDG_APPID::DTC:
            mSavedCRC16.clear();
            break;
        case RDG_APPID::ROB:
            mSavedCRC16.clear();
            break;
        case RDG_APPID::SSR:
            mSavedCRC16.clear();
            break;
        case RDG_APPID::ROBSSR:
            mSavedCRC32.clear();
            break;
        default:
            break;
    }
    LOG_D("Delete Saved CRC value successfully");
    LOG_I("Delete CRC file: %s", PATH_CRC_FILE.c_str());
    if(FileUtil::isPathExist(PATH_CRC_FILE.c_str()) == true) {
        if(FileUtil::removeFile(PATH_CRC_FILE.c_str()) == true) {
            LOG_I("Delete file successfully");
        } else {
            LOG_I("Delete file fail");
        }
    } else {
        LOG_I("File do not exist");
    }
}
}
