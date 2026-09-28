#ifndef REMOTEDIAG_UDS_MESSAGE_H
#define REMOTEDIAG_UDS_MESSAGE_H
#include <cstdint>
// #include <cstdio>
#include <cstdlib>
#include <Error.h>
#include <string>
#include <vector>
#include <binder/Parcel.h>
#include <utils/Buffer.h>

#include "UdsMessageDefine.h"

namespace rdgapp {

class UdsMessage : public android::RefBase 
{
public:
    // Base constructor that handles all initialization
    UdsMessage(const uint8_t aSID, const uint8_t aSFID, const uint8_t aDtcStatusMask, const uint8_t aMemorySelection, const uint8_t aNRC)
        : android::RefBase()
        , mSID(aSID)
        , mSFID(aSFID)
        , mDtcStatusMask(aDtcStatusMask)
        , mMemorySelection(aMemorySelection)
        , mNRC(aNRC)
        , mOptionData(new ::Buffer())
        , mUdsPayload(new ::Buffer()) {
            if (mDtcStatusMask != 0U) {
                mOptionData->append(&mDtcStatusMask, 1);
            }
            if (mMemorySelection != 0U) {
                mOptionData->append(&mMemorySelection, 1);
            }
        }

    // Default constructor delegating to the base constructor
    UdsMessage() noexcept
        : UdsMessage(0U, 0U, 0U, 0U, 0U) {}

    // Constructor with some default values
    UdsMessage(const uint8_t aSID, const uint8_t aSFID) noexcept
        : UdsMessage(aSID, aSFID, 0U, 0U, 0U) {}


    UdsMessage(const uint8_t aSID, const uint8_t aSFID, const uint8_t aDtcStatusMask) noexcept
        : UdsMessage(aSID, aSFID, aDtcStatusMask, 0U, 0U) {}

    UdsMessage(const uint8_t aSID, const uint8_t aSFID, const uint8_t aDtcStatusMask, const uint8_t aMemorySelection) noexcept
        : UdsMessage(aSID, aSFID, aDtcStatusMask, aMemorySelection, 0U) {}

    UdsMessage(const android::sp<UdsMessage> udsResponse);

    error_t Parser(const android::sp<::Buffer>& udsData);
    error_t Parser(const std::vector<uint8_t>& udsData);
    android::sp<::Buffer> ToUdsData(void);
    android::sp<::Buffer> GetUdsPayload(void) const noexcept;
    // Service Identifier
    void setSID(const uint8_t aSID) noexcept; 
    uint8_t getSID() const noexcept;
    // Sub-Function Identifier
    void setSFID(const uint8_t aSFID) noexcept;
    uint8_t getSFID() const noexcept;
    // Negative response code     
    uint8_t getNRC() const noexcept;
    
    const uint8_t getMemorySelectionRes() noexcept;
    inline const uint8_t getMemorySelectionReq() const noexcept {return mMemorySelection;}
                
    // android::sp<::Buffer>& optionData();
    android::sp<::Buffer> getOptionData(void) const noexcept;
 
private:
    uint8_t mSID;                    // Service Identifier
    uint8_t mSFID;                   // Sub-Function Identifier
    uint8_t mDtcStatusMask;
    uint8_t mMemorySelection;        // This parameter is an echo of the mMemorySelection parameter provided in the request message from the client.
                                    // This parameter shall be used to address the respective user defined DTC memory when retrieving DTCs

    uint8_t mNRC;                    // Negative response code                          
    android::sp<::Buffer> mOptionData;
    android::sp<::Buffer> mUdsPayload;
    android::sp<::Buffer> mUdsData;
};
}
#endif /* REMOTEDIAG_UDS_MESSAGE_H */
