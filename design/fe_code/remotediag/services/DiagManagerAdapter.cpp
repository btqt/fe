#include "Remotediag.h"
#include "DiagManagerAdapter.h"

namespace rdgapp {

DiagManagerAdapter::DiagManagerAdapter()
{
    mDiagReceiver = new DiagMReceiver(*this);
    mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who)
                                                       { this->onBinderDied(who); });
}

DiagManagerAdapter::~DiagManagerAdapter()
{
    mDiagMService = nullptr;
    mDiagReceiver = nullptr;
    mServiceDeathRecipient = nullptr;

    if (DiagManagerAdapter::instance != nullptr)
    {
        DiagManagerAdapter::instance = nullptr;
    }
}

std::shared_ptr<DiagManagerAdapter> DiagManagerAdapter::instance{nullptr};
std::shared_ptr<DiagManagerAdapter> DiagManagerAdapter::getInstance()
{
    if (instance == nullptr)
    {
        instance = std::make_shared<DiagManagerAdapter>();
    }
    return instance;
}

android::sp<IDiagManagerService> DiagManagerAdapter::getService()
{
    mDiagMService = android::interface_cast<IDiagManagerService>(
        android::defaultServiceManager()->getService(
            android::String16(DIAG_SRV_NAME)));
    return mDiagMService;
}

void DiagManagerAdapter::registerService()
{
    LOG_I("DiagManagerAdapter::registerService");
    mHandler = RemotediagHandler::getInstance_2();

    if (mDiagMService != nullptr)
    {
        mDiagMService = nullptr;
    }

    if (mDiagReceiver == nullptr)
    {
        mDiagReceiver = android::sp<DiagMReceiver>(new DiagMReceiver(*this));
    }
    (void)getService();
    bool error{true};
    if (mDiagMService != nullptr)
    {
        LOG_D("DiagManagerAdapter registered");
        const android::status_t result{android::IInterface::asBinder(mDiagMService)->linkToDeath(mServiceDeathRecipient)};
        if (result == android::OK)
        {
            error = false;
        }
    }
    if (error)
    {
        LOG_E("Cannot register DiagM Service, try again after ms: %d", RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
        (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_DIAG_MGR),
                                           static_cast<uint64_t>(RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS));
    }
}

void DiagManagerAdapter::writeDidData(const uint16_t did, sp<::Buffer> didData)
{
    LOG_D("writeDidData: 0x%02X", did);
    if (mDiagMService != nullptr)
    {
        //DRBFM TMCDCMTF-13711: 3 times retry
        for (int32_t i{0}; i<3; i++)
        {
            if (mDiagMService->writeDidInternalBySource(1U, did, didData) == 0x00U) //E_OK
            {
                LOG_D("DiagManagerAdapter::writeDidData success");
                break;
            }
        }
    }
}

void DiagManagerAdapter::readDidData(const uint16_t did, sp<::Buffer> &didData)
{
    LOG_D("readDidData: 0x%02X", did);
    if (mDiagMService != nullptr)
    {
        //DRBFM TMCDCMTF-13711: 3 times retry
        for (int32_t i{0}; i<3; i++)
        {
            mDiagMService->readDidInternalBySource(1U, did, didData);
            if (didData->size() != 0U)
            {
                LOG_D("DiagManagerAdapter::readDidData success");
                break;
            }
        }
    }
}

uint8_t DiagManagerAdapter::getUnderRepairStatus()
{
    uint8_t ret{0x0U};
    if (mDiagMService != nullptr)
    {
        constexpr uint16_t did{static_cast<uint16_t>(OEM_DID_Under_repair_status)}; // 0x2043
        android::sp<Buffer> spBuf{new Buffer()};

        readDidData(did, spBuf);
        if (spBuf->size() > 0U)
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                const uint8_t underRepair{data[0]};
                if (underRepair == 0x00U)
                {
                    ret = 0x0U;
                }
                else
                {
                    ret = 0x1U;
                }
            }
            else
            {
                ret = UNDER_REPAIR_STATUS_DEFAULT;
            }

        }
        else
        {
            LOG_I("error : DID %X buf size is zero", did);
            ret = UNDER_REPAIR_STATUS_DEFAULT;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = UNDER_REPAIR_STATUS_DEFAULT;
    }
    return ret;
}

uint8_t DiagManagerAdapter::getRDGFlag()
{
    constexpr uint8_t RDGFLAG_DEFAULT{0U};
    uint8_t ret{RDGFLAG_DEFAULT};
    if (mDiagMService != nullptr)
    {
        android::sp<::Buffer> spBuf{new ::Buffer()};

        readDidData(PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);
        if (spBuf->size() > 0U)
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                ret = (data[0] >> 7U) & 1U;
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG);
            ret = RDGFLAG_DEFAULT;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = RDGFLAG_DEFAULT;
    }
    return ret;
}

uint8_t DiagManagerAdapter::getDTCFlag()
{
    constexpr uint8_t FLAG_DEFAULT{0U};
    uint8_t ret{FLAG_DEFAULT};
    if (mDiagMService != nullptr)
    {
        android::sp<::Buffer> spBuf{new ::Buffer()};

        readDidData(PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);

        if (spBuf->size() > 0U)
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                ret = (data[0] >> 6U) & 1U;
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG);
            ret = FLAG_DEFAULT;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = FLAG_DEFAULT;
    }
    return ret;
}

uint8_t DiagManagerAdapter::getSSRFlag()
{
    constexpr uint8_t FLAG_DEFAULT{0U};
    uint8_t ret{FLAG_DEFAULT};
    if (mDiagMService != nullptr)
    {
        android::sp<::Buffer> spBuf{new ::Buffer()};

        readDidData(PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);

        if (spBuf->size() > 0U)
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                ret = (data[0] >> 5U) & 1U;
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG);
            ret = FLAG_DEFAULT;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = FLAG_DEFAULT;
    }
    return ret;
}

uint8_t DiagManagerAdapter::getWARflag()
{
    constexpr uint8_t FLAG_DEFAULT{0U};
    uint8_t ret{FLAG_DEFAULT};
    if (mDiagMService != nullptr)
    {
        android::sp<::Buffer> spBuf{new ::Buffer()};

        readDidData(PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);

        if (spBuf->size() > 0U)
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                ret = (data[0] >> 4U) & 1U;
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG);
            ret = FLAG_DEFAULT;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = FLAG_DEFAULT;
    }
    return ret;
}

uint8_t DiagManagerAdapter::getRoBflag()
{
    constexpr uint8_t FLAG_DEFAULT{0U};
    uint8_t ret{FLAG_DEFAULT};
    if (mDiagMService != nullptr)
    {
        android::sp<::Buffer> spBuf{new ::Buffer()};

        readDidData(PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);

        if (spBuf->size() > 0U)
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                ret = (data[0] >> 3U) & 1U;
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG);
            ret = FLAG_DEFAULT;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = FLAG_DEFAULT;
    }
    return ret;
}

uint8_t DiagManagerAdapter::getDDRflag()
{
    constexpr uint8_t FLAG_DEFAULT{0U};
    uint8_t ret{FLAG_DEFAULT};
    if (mDiagMService != nullptr)
    {
        android::sp<::Buffer> spBuf{new ::Buffer()};

        readDidData(PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);

        if (spBuf->size() > 0U)
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                ret = (data[0] >> 2U) & 1U;
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG);
            ret = FLAG_DEFAULT;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = FLAG_DEFAULT;
    }
    return ret;
}

void DiagManagerAdapter::saveRDGFlag(const bool isActive)
{
    if (mDiagMService != nullptr)
    {

        android::sp<::Buffer> spBuf{new ::Buffer()};
        readDidData(PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);
        // const uint8_t *const data{spBuf->data()};
        if ((spBuf->data() != nullptr) && (spBuf->size() > 0U))
        {
            const uint8_t rdgValue{isActive == true ? 1U : 0U};
            spBuf->data()[0] &= 0x7FU;
            spBuf->data()[0] |= static_cast<uint8_t>(static_cast<uint32_t>(rdgValue) << 7U);
            writeDidData(PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);
        }
        else
        {
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
    }
}

void DiagManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who)
{
    LOG_I("DiagManagerAdapter::onBinderDied");
    NOTUSED(who);
    mDiagMService = nullptr;
    mDiagReceiver = nullptr;
    (void)mHandler->sendMessageDelayed(mHandler->obtainMessage(HANDLE_MESSAGE_REQUEST::MSG_REGISTER_DIAG_MGR),
                                       RDG_TIME::TIME_OBTAIN_MSG_DELAY_500MS);
}

void DiagManagerAdapter::onReceivedDiagReq(const sp<DiagData> &diagData)
{
    LOG_I("DiagManagerAdapter::onReceivedDiagReq");
    NOTUSED(diagData);
}

void DiagManagerAdapter::onSendDiagData(const sp<DiagData> diagData)
{
    LOG_I("DiagManagerAdapter::onSendDiagData");
    NOTUSED(diagData);
}

void DiagManagerAdapter::onClearDiagInfo(const uint8_t order)
{
    LOG_I("DiagManagerAdapter::onClearDiagInfo");
    NOTUSED(order);
}

bool DiagManagerAdapter::getAllUploadConsent()
{
    bool ret{false};
    if (mDiagMService != nullptr)
    {
        constexpr uint16_t did{static_cast<uint16_t>(PARAM_DID_PPI_CONSENT_STATE)}; // 0x1022
        android::sp<Buffer> spBuf{new Buffer()};

        readDidData(did, spBuf);

        if (spBuf->size() > 0U)
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                const uint8_t uploadConsent{(data[0] >> 6U) & 0b11U};
                if (uploadConsent == 0b10U)
                {
                    ret = true;
                }
                else
                {
                    ret = false;
                }
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", did);
            ret = false;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = false;
    }
    return ret;
}

std::string DiagManagerAdapter::getVinNumber()
{
    std::string strVIN{""};
    if (mDiagMService != nullptr)
    {
        android::sp<::Buffer> spBuf{new ::Buffer()};
        readDidData(PARAM_DID_VIN_NUMBER, spBuf);
        if ((spBuf->data() != nullptr) && (spBuf->size() == 17U))
        {
            char_t temp[spBuf->size() + 1U] {0,};
            (void)std::memcpy(&temp[0], spBuf->data(), spBuf->size());
            (void)strVIN.assign(&temp[0]);
            LOG_I("VIN: %s", strVIN.c_str());
        }
        else
        {
            LOG_E("Fail to read VIN, size %d", spBuf->size());
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
    }
    return strVIN;
}

bool DiagManagerAdapter::getLocationUploadConsent()
{
    bool ret{false};
    if (mDiagMService != nullptr)
    {
        constexpr uint16_t did{static_cast<uint16_t>(PARAM_DID_PPI_CONSENT_STATE)}; // 0x1022
        android::sp<Buffer> spBuf{new Buffer()};

        readDidData(did, spBuf);

        if (spBuf->size() > 0U)
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                const uint8_t uploadConsent{(data[0]) & 0b11U};
                if (uploadConsent == 0b10U)
                {
                    ret = true;
                }
                else
                {
                    ret = false;
                }
            }

        }
        else
        {
            LOG_I("error : DID %X buf size is zero", did);
            ret = false;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = false;
    }
    return ret;
}

bool DiagManagerAdapter::getSRVC_AC()
{
    bool ret{false};
    if (mDiagMService != nullptr)
    {
        constexpr uint16_t did{static_cast<uint16_t>(PARAM_DID_PPI_CONSENT_STATE)}; // 0x1022
        android::sp<Buffer> spBuf{new Buffer()};

        readDidData(did, spBuf);

        if (spBuf->size() > 0U)
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                const uint8_t uploadConsent{(data[0] >> 6U) & 0b11U};
                if (uploadConsent == 0b10U)
                {
                    ret = true;
                }
                else
                {
                    ret = false;
                }
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", did);
            ret = false;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = false;
    }
    return ret;
}

bool DiagManagerAdapter::getSRVC_VC()
{
    bool ret{false};
    if (mDiagMService != nullptr)
    {
        constexpr uint16_t did{static_cast<uint16_t>(PARAM_DID_PPI_CONSENT_STATE)}; // 0x1022
        android::sp<Buffer> spBuf{new Buffer()};

        readDidData(did, spBuf);

        if ((spBuf->size() > 0U))
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                const uint8_t uploadConsent{(data[0] >> 4U) & 0b11U};
                if (uploadConsent == 0b10U)
                {
                    ret = true;
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", did);
            ret = false;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = false;
    }
    return ret;
}

bool DiagManagerAdapter::getSRVC_PC()
{
    bool ret{false};
    if (mDiagMService != nullptr)
    {
        constexpr uint16_t did{static_cast<uint16_t>(PARAM_DID_PPI_CONSENT_STATE)}; // 0x1022
        android::sp<Buffer> spBuf{new Buffer()};

        readDidData(did, spBuf);

        if ((spBuf->data() != nullptr) && (spBuf->size() > 0U))
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                const uint8_t uploadConsent{(data[0] >> 2U) & 0b11U};
                if (uploadConsent == 0b10U)
                {
                    ret = true;
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", did);
            ret = false;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = false;
    }
    return ret;
}

bool DiagManagerAdapter::getSRVC_STT()
{
    bool ret{false};
    if (mDiagMService != nullptr)
    {
        constexpr uint16_t did{static_cast<uint16_t>(PARAM_DID_PPI_CONSENT_STATE)}; // 0x1022
        android::sp<Buffer> spBuf{new Buffer()};

        readDidData(did, spBuf);

        if ((spBuf->data() != nullptr) && (spBuf->size() > 0U))
        {
            const uint8_t *const data{spBuf->data()};
            if (data != nullptr)
            {
                const uint8_t uploadConsent{(data[0]) & 0b11U};
                if (uploadConsent == 0b10U)
                {
                    ret = true;
                }
                else
                {
                    ret = false;
                }
            }
            else
            {
                ret = false;
            }
        }
        else
        {
            LOG_I("error : DID %X buf size is zero", did);
            ret = false;
        }
    }
    else
    {
        LOG_I("error : diagMgr is nullptr");
        ret = false;
    }
    return ret;
}

void DiagManagerAdapter::selfDiagIgOnOffTimes(const bool isIgOn, const uint16_t type)
{
    const android::sp<::Buffer> timeData{new ::Buffer()};
    android::sp<::Buffer> spBuf{new ::Buffer()};
    const std::string RDG_IGON_OFF_INDEX_PROP{"remotediag.prop.OnOffIndex"};
    const std::string RDG_ALL_IGON_OFF_INDEX_PROP{"remotediag.prop.AllOnOffIndex"};
    const std::string didIdxProperty{(type == SELFDIAG_DID_TYPE_10_TIMES) ? RDG_IGON_OFF_INDEX_PROP : RDG_ALL_IGON_OFF_INDEX_PROP};
    const char_t *const index{Remotediag::getInstance()->getProperty(didIdxProperty.c_str())};
    int16_t currentIdx{0};

    if (index != nullptr)
    {
        std::string::size_type sz{}; // alias of size_t
        const int32_t tmp{std::stoi(index, &sz)};
        if (tmp <= INT16_MAX)
        {
            currentIdx = static_cast<int16_t>(tmp);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set property, set to default");
        currentIdx = -14;
    }

    CommonUtils::convertCurrentTimeToBuffer(timeData);
    const uint16_t did{(type == SELFDIAG_DID_TYPE_10_TIMES) ? PARAM_DID_IG_ON_OFF_TIME : PARAM_DID_ALL_IG_ON_OFF_TIMES};

    readDidData(did, spBuf);
    const int16_t maxSize{(type == SELFDIAG_DID_TYPE_10_TIMES) ? 140 : 3920};
    if ((spBuf->data() != nullptr) && (spBuf->size() == static_cast<uint32_t>(maxSize)))
    {
        if (isIgOn == true)
        {
            currentIdx += 14;
            const android::sp<Buffer> newData{new Buffer()};
            if (currentIdx > (maxSize - 14))
            {
                currentIdx = maxSize - 14;
                newData->setTo(&spBuf->data()[14], static_cast<int32_t>(currentIdx));
                uint8_t initData[14]{0U,};
                (void)memset(&initData[0], 0, sizeof(initData));
                newData->append(&initData[0], sizeof(initData));
            }
            else
            {
                newData->setTo(spBuf->data(), static_cast<int32_t>(maxSize));
            }
            LOG_V("[selfDiagIgOnOffTimes] IG ON, Current property %d, did: 0x%02X", currentIdx, did);
            const uint16_t tripCount{VehicleManagerAdapter::getInstance()->getTripCounter()};
            if (newData->data() != nullptr)
            {
                newData->data()[currentIdx] = static_cast<uint8_t>((tripCount >> 8U) & 0xFFU); //CID 9337056 CID 9337524
                newData->data()[currentIdx + 1] = static_cast<uint8_t>(tripCount & 0xFFU);
            }
            (void)tripCount;

            for (uint32_t i{0U}; i < timeData->size(); i++)
            {
                if ((currentIdx >= 0) && (newData->data() != nullptr) && (timeData->data() != nullptr)) 
                {
                    newData->data()[static_cast<uint32_t>(currentIdx) + 2U + i] = timeData->data()[i];
                }
                else
                {
                    LOG_I("Fail to convert from int32 to uint32: ", currentIdx);
                }
            }
            spBuf->setTo(newData->data(), static_cast<int32_t>(maxSize));
        }
        else
        {
            LOG_I("[selfDiagIgOnOffTimes] IG OFF, Current property %d", currentIdx);
            if (currentIdx < 0)
            {
                currentIdx = 0;
            }
            for (uint32_t i{0U}; i < timeData->size(); i++)
            {
                const uint32_t tmpIdx {static_cast<uint32_t>(currentIdx) + 8U};
                if ((tmpIdx <= static_cast<uint32_t>(UINT32_MAX) - i) && (timeData->data() != nullptr))
                {
                    spBuf->data()[tmpIdx + i] = timeData->data()[i]; //CID 9390154
                }
                else
                {
                    LOG_E("Over flow");                
                }
            }
        }
        writeDidData(did, spBuf);
        LOG_I("Current property %d", currentIdx);
        Remotediag::getInstance()->setProperty(didIdxProperty.c_str(), static_cast<int32_t>(currentIdx), true);
    }
    else
    {
        (void)currentIdx;
        LOG_E("spBuf size is invalid: %lu", spBuf->size());
    }
    (void)maxSize;
}

void DiagManagerAdapter::selfDiagCollectionCondition(const uint64_t collectionConditionId)
{
    const android::sp<::Buffer> timeData{new ::Buffer()};
    android::sp<::Buffer> spBuf{new ::Buffer()};
    const std::string RDG_DID_FAILURE_COLLECTION_CONDITION_PROP{"remotediag.prop.DidCollection"};
    const std::string didName{RDG_DID_FAILURE_COLLECTION_CONDITION_PROP};
    const std::string RDG_DID_FAILURE_COLLECTION_CONDITION_INDEX_PROP{"remotediag.prop.DidCollIdx"};
    const std::string didIndex{RDG_DID_FAILURE_COLLECTION_CONDITION_INDEX_PROP};
    const char_t *const currDid{Remotediag::getInstance()->getProperty(didName.c_str())};
    const char_t *const index{Remotediag::getInstance()->getProperty(didIndex.c_str())};
    int16_t currentIdx{0};
    uint16_t did{0U};

    if (index != nullptr)
    {
        std::string::size_type sz{}; // alias of size_t
        const int32_t tmp{std::stoi(index, &sz)};
        if (tmp <= INT16_MAX)
        {
            currentIdx = static_cast<int16_t>(tmp);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set property, set to default");
        currentIdx = -14;
    }
    if (currDid != nullptr)
    {
        const uint64_t did_t{std::stoul(currDid, nullptr, 0)};
        if (did_t <= static_cast<uint16_t>(UINT16_MAX))
        {
            did = static_cast<uint16_t>(did_t);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set Did, set to default");
        did = PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_1;
    }
    CommonUtils::convertCurrentTimeToBuffer(timeData);
    currentIdx += 14;
    if ((did >= PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_32) && (currentIdx > 4088 - 14))
    {
        did = PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_1;
        currentIdx = 0;
    }
    else if ((did < PARAM_DID_FAILURE_TO_DECIPHER_COLLECTION_CONDITION_32) && (currentIdx > 4088 - 14))
    {
        did += 1U;
        currentIdx = 0;
    }
    else
    {
        // Do nothing
    }
    readDidData(did, spBuf);
    if ((spBuf->data() != nullptr) && (spBuf->size() == 4088U) && (timeData->data() != nullptr))
    {
        for (uint32_t i{0U}; i < timeData->size(); i++)
        {
            spBuf->data()[static_cast<uint32_t>(currentIdx) + i] = timeData->data()[i];
        }
        // collectionConditionId
        const uint64_t big_endian{static_cast<uint64_t>(__builtin_bswap64(collectionConditionId))};
        uint8_t result[sizeof(uint64_t)];
        (void)std::memcpy(result, &big_endian, sizeof(big_endian));
        for (uint32_t i{0U}; i < 8U; i++)
        {
            if (currentIdx >= 0)
            {
                spBuf->data()[static_cast<uint32_t>(currentIdx) +6U + i] = result[i]; // CID 9389936
            }
            else
            {
                // do nothing
            }
        }

        writeDidData(did, spBuf);
        LOG_I("Current property %d, current did 0x%02X", currentIdx, did);
        Remotediag::getInstance()->setProperty(didName.c_str(), static_cast<int32_t>(did), true);
        Remotediag::getInstance()->setProperty(didIndex.c_str(), static_cast<int32_t>(currentIdx), true);
    }
    else
    {
        (void)currentIdx;
        LOG_E("spBuf size is invalid: %lu", spBuf->size());
    }
}

void DiagManagerAdapter::selfDiagSuccessReadNotification(const uint8_t triggerType)
{
    const android::sp<::Buffer> timeData{new ::Buffer()};
    android::sp<::Buffer> spBuf{new ::Buffer()};
    const std::string RDG_DID_SUCCESS_READ_NOTIFICATION_TRIGGER_PROP{"remotediag.prop.DidReadNoti"};
    const std::string didName{RDG_DID_SUCCESS_READ_NOTIFICATION_TRIGGER_PROP};
    const std::string RDG_DID_SUCCESS_READ_NOTIFICATION_TRIGGER_INDEX_PROP{"remotediag.prop.DidReadNotiIdx"};
    const std::string didIndex{RDG_DID_SUCCESS_READ_NOTIFICATION_TRIGGER_INDEX_PROP};
    const char_t *const currDid{Remotediag::getInstance()->getProperty(didName.c_str())};
    const char_t *const index{Remotediag::getInstance()->getProperty(didIndex.c_str())};
    int16_t currentIdx{0};
    uint16_t did{0U};

    if (index != nullptr)
    {
        std::string::size_type sz{}; // alias of size_t
        const int32_t tmp{std::stoi(index, &sz)};
        if (tmp <= INT16_MAX)
        {
            currentIdx = static_cast<int16_t>(tmp);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set property, set to default");
        currentIdx = -7;
    }
    if (currDid != nullptr)
    {
        const uint64_t did_t{std::stoul(currDid, nullptr, 0)};
        if (did_t <= static_cast<uint16_t>(UINT16_MAX))
        {
            did = static_cast<uint16_t>(did_t);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set Did, set to default");
        did = PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_1;
    }
    CommonUtils::convertCurrentTimeToBuffer(timeData);
    currentIdx += 7;
    if ((did >= PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_32) && (currentIdx > 4088 - 7))
    {
        did = PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_1;
        currentIdx = 0;
    }
    else if ((did < PARAM_DID_SUCCESSFULLY_READ_NOTIFICATION_TRIGGER_32) && (currentIdx > 4088 - 7))
    {
        did += 1U;
        currentIdx = 0;
    }
    else
    {
        // Do nothing
    }
    readDidData(did, spBuf);
    if ((spBuf->data() != nullptr) && (spBuf->size() == 4088U) && (timeData->data() != nullptr))
    {
        for (uint32_t i{0U}; i < timeData->size(); i++)
        {
            spBuf->data()[static_cast<uint32_t>(currentIdx) + i] = timeData->data()[i];
        }
        spBuf->data()[currentIdx + 6] = triggerType;

        writeDidData(did, spBuf);
        LOG_I("Current property %d, current did 0x%02X", currentIdx, did);
        Remotediag::getInstance()->setProperty(didName.c_str(), static_cast<int32_t>(did), true);
        Remotediag::getInstance()->setProperty(didIndex.c_str(), static_cast<int32_t>(currentIdx), true);
    }
    else
    {
        (void)currentIdx;
        LOG_E("Invalid buffer size: %lu", spBuf->size());
    }

}

void DiagManagerAdapter::selfDiagSuccessCreateFile(const uint8_t triggerType)
{
    const android::sp<::Buffer> timeData{new ::Buffer()};
    android::sp<::Buffer> spBuf{new ::Buffer()};
    const std::string RDG_DID_SUCCESSFULLY_FILE_CREATION_PROP{"remotediag.prop.DIDFile"};
    const std::string didName{RDG_DID_SUCCESSFULLY_FILE_CREATION_PROP};
    const std::string RDG_DID_SUCCESSFULLY_FILE_CREATION_INDEX_PROP{"remotediag.prop.DIDFileIdx"};
    const std::string didIndex{RDG_DID_SUCCESSFULLY_FILE_CREATION_INDEX_PROP};
    const char_t *const currDid{Remotediag::getInstance()->getProperty(didName.c_str())};
    const char_t *const index{Remotediag::getInstance()->getProperty(didIndex.c_str())};
    int16_t currentIdx{0};
    uint16_t did{0U};

    if (index != nullptr)
    {
        std::string::size_type sz{}; // alias of size_t
        const int32_t tmp{std::stoi(index, &sz)};
        if (tmp <= INT16_MAX)
        {
            currentIdx = static_cast<int16_t>(tmp);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set property, set to default");
        currentIdx = -7;
    }
    if (currDid != nullptr)
    {
        const uint64_t did_t{std::stoul(currDid, nullptr, 0)};
        if (did_t <= static_cast<uint16_t>(UINT16_MAX))
        {
            did = static_cast<uint16_t>(did_t);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set Did, set to default");
        did = PARAM_DID_SUCCESSFULLY_FILE_CREATION_1;
    }
    CommonUtils::convertCurrentTimeToBuffer(timeData);
    currentIdx += 7;
    if ((did >= PARAM_DID_SUCCESSFULLY_FILE_CREATION_32) && (currentIdx > 4088 - 7))
    {
        did = PARAM_DID_SUCCESSFULLY_FILE_CREATION_1;
        currentIdx = 0;
    }
    else if ((did < PARAM_DID_SUCCESSFULLY_FILE_CREATION_32) && (currentIdx > 4088 - 7))
    {
        did += 1U;
        currentIdx = 0;
    }
    else
    {
        // Do nothing
    }
    readDidData(did, spBuf);
    if ((spBuf->data() != nullptr) && (spBuf->size() == 4088U) && (timeData->data() != nullptr))
    {
        for (uint32_t i{0U}; i < timeData->size(); i++)
        {
            spBuf->data()[static_cast<uint32_t>(currentIdx) + i] = timeData->data()[i];
        }
        spBuf->data()[currentIdx + 6] = triggerType;

        writeDidData(did, spBuf);
        LOG_I("Current property %d, current did 0x%02X", currentIdx, did);
        Remotediag::getInstance()->setProperty(didName.c_str(), static_cast<int32_t>(did), true);
        Remotediag::getInstance()->setProperty(didIndex.c_str(), static_cast<int32_t>(currentIdx), true);
    }
    else
    {
        (void)currentIdx;
        LOG_E("Invalid buffer size: %lu", spBuf->size());
    }
}

void DiagManagerAdapter::selfDiagStopOpeartion(const uint8_t operation)
{
    const android::sp<::Buffer> timeData{new ::Buffer()};
    android::sp<::Buffer> spBuf{new ::Buffer()};
    const std::string RDG_DID_STOP_OPERATION_PROP{"remotediag.prop.DidStopOper"};
    const std::string didName{RDG_DID_STOP_OPERATION_PROP};
    const std::string RDG_DID_STOP_OPERATION_INDEX_PROP{"remotediag.prop.DidStopOperIdx"};
    const std::string didIndex{RDG_DID_STOP_OPERATION_INDEX_PROP};
    const char_t *const currDid{Remotediag::getInstance()->getProperty(didName.c_str())};
    const char_t *const index{Remotediag::getInstance()->getProperty(didIndex.c_str())};
    int16_t currentIdx{0};
    uint16_t did{0U};

    if (index != nullptr)
    {
        std::string::size_type sz{}; // alias of size_t
        const int32_t tmp{std::stoi(index, &sz)};
        if (tmp <= INT16_MAX)
        {
            currentIdx = static_cast<int16_t>(tmp);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set property, set to default");
        currentIdx = -7;
    }
    if (currDid != nullptr)
    {
        const uint64_t did_t{std::stoul(currDid, nullptr, 0)};
        if (did_t <= static_cast<uint16_t>(UINT16_MAX))
        {
            did = static_cast<uint16_t>(did_t);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set Did, set to default");
        did = PARAM_DID_STOP_OPERATION_1;
    }
    CommonUtils::convertCurrentTimeToBuffer(timeData);
    currentIdx += 7;
    if ((did >= PARAM_DID_STOP_OPERATION_32) && (currentIdx > 4088 - 7))
    {
        did = PARAM_DID_STOP_OPERATION_1;
        currentIdx = 0;
    }
    else if ((did < PARAM_DID_STOP_OPERATION_32) && (currentIdx > 4088 - 7))
    {
        did += 1U;
        currentIdx = 0;
    }
    else
    {
        // Do nothing
    }
    readDidData(did, spBuf);
    if ((spBuf->data() != nullptr) && (spBuf->size() == 4088U) && (timeData->data() != nullptr))
    {
        for (uint32_t i{0U}; i < timeData->size(); i++)
        {
            spBuf->data()[static_cast<uint32_t>(currentIdx) + i] = timeData->data()[i];
        }
        spBuf->data()[currentIdx + 6] = operation;

        writeDidData(did, spBuf);
        LOG_I("Current property %d, current did 0x%02X", currentIdx, did);
        Remotediag::getInstance()->setProperty(didName.c_str(), static_cast<int32_t>(did), true);
        Remotediag::getInstance()->setProperty(didIndex.c_str(), static_cast<int32_t>(currentIdx), true);
    }
    else
    {
        (void)currentIdx;
        LOG_E("Invalid buffer size: %lu", spBuf->size());
    }
}

void DiagManagerAdapter::selfDiagNoCenterResponse()
{
    const android::sp<::Buffer> timeData{new ::Buffer()};
    android::sp<::Buffer> spBuf{new ::Buffer()};
    const std::string RDG_DID_NO_CENTER_RESPONSE_PROP{"remotediag.prop.DidNoResp"};
    const std::string didName{RDG_DID_NO_CENTER_RESPONSE_PROP};
    const std::string RDG_DID_NO_CENTER_RESPONSE_INDEX_PROP{"remotediag.prop.DidNoRespIdx"};
    const std::string didIndex{RDG_DID_NO_CENTER_RESPONSE_INDEX_PROP};
    const char_t *const currDid{Remotediag::getInstance()->getProperty(didName.c_str())};
    const char_t *const index{Remotediag::getInstance()->getProperty(didIndex.c_str())};
    int16_t currentIdx{0};
    uint16_t did{0U};

    if (index != nullptr)
    {
        std::string::size_type sz{}; // alias of size_t
        const int32_t tmp{std::stoi(index, &sz)};
        if (tmp <= INT16_MAX)
        {
            currentIdx = static_cast<int16_t>(tmp);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set property, set to default");
        currentIdx = -6;
    }
    if (currDid != nullptr)
    {
        const uint64_t did_t{std::stoul(currDid, nullptr, 0)};
        if (did_t <= static_cast<uint16_t>(UINT16_MAX))
        {
            did = static_cast<uint16_t>(did_t);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set Did, set to default");
        did = PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_1;
    }
    CommonUtils::convertCurrentTimeToBuffer(timeData);
    currentIdx += 6;
    if ((did >= PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_32) && (currentIdx > 4092 - 6))
    {
        did = PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_1;
        currentIdx = 0;
    }
    else if ((did < PARAM_DID_CENTER_RESPONSE_NOT_RECEIVED_32) && (currentIdx > 4092 - 6))
    {
        did += 1U;
        currentIdx = 0;
    }
    else
    {
        // Do nothing
    }
    readDidData(did, spBuf);
    if ((spBuf->data() != nullptr) && (spBuf->size() == 4092U) && (timeData->data() != nullptr))
    {
        for (uint32_t i{0U}; i < timeData->size(); i++)
        {
            spBuf->data()[static_cast<uint32_t>(currentIdx) + i] = timeData->data()[i];
        }
        writeDidData(did, spBuf);
        LOG_I("Current property %d, current did 0x%02X", currentIdx, did);
        Remotediag::getInstance()->setProperty(didName.c_str(), static_cast<int32_t>(did), true);
        Remotediag::getInstance()->setProperty(didIndex.c_str(), static_cast<int32_t>(currentIdx), true);
    }
    else
    {
        (void)currentIdx;
        LOG_E("Invalid buffer size: %lu", spBuf->size());
    }
}

void DiagManagerAdapter::storeCollectionConditionId(const uint64_t collectionConditionId)
{
    const android::sp<Buffer> timeData{new Buffer()};
    android::sp<Buffer> spBuf{new Buffer()};
    constexpr uint16_t did{PARAM_DID_COLLECTION_CONDITIONS};
    const std::string RDG_DID_STORE_COLLECTION_CONDITION_INDEX_PROP{"remotediag.prop.StoreCollIdx"};
    const std::string didIndex{RDG_DID_STORE_COLLECTION_CONDITION_INDEX_PROP};
    const char_t *const index{Remotediag::getInstance()->getProperty(didIndex.c_str())};
    int16_t currentIdx{0};

    if (index != nullptr)
    {
        std::string::size_type sz{}; // alias of size_t
        const int32_t tmp{std::stoi(index, &sz)};
        if (tmp <= INT16_MAX)
        {
            currentIdx = static_cast<int16_t>(tmp);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set property, set to default");
        currentIdx = -8;
    }

    currentIdx += 8;
    if (currentIdx > (4088 - 8))
    {
        currentIdx = 0;
    }
    readDidData(did, spBuf);
    if ((spBuf->data() != nullptr) && (spBuf->size() == 4088U) && (timeData->data() != nullptr))
    {
        for (uint32_t i{0U}; i < timeData->size(); i++)
        {
            spBuf->data()[static_cast<uint32_t>(currentIdx) + i] = timeData->data()[i];
        }
        // collectionConditionId
        const uint64_t big_endian{static_cast<uint64_t>(__builtin_bswap64(collectionConditionId))};
        uint8_t result[sizeof(uint64_t)];
        (void)std::memcpy(result, &big_endian, sizeof(big_endian));
        for (int32_t i{0}; i < 8; i++)
        {
            spBuf->data()[currentIdx + i] = result[i];
        }

        writeDidData(did, spBuf);
        LOG_I("[storeCollectionConditionId] Current property %d", currentIdx);
        Remotediag::getInstance()->setProperty(didIndex.c_str(), static_cast<int32_t>(currentIdx), true);
    }
    else
    {
        (void)currentIdx;
        LOG_E("Invalid buffer size: %lu", spBuf->size());
    }
}

void DiagManagerAdapter::selfDiagEcuUserDefMemoryDTC(const android::sp<OccurrentRobNotification> notification)
{

    const android::sp<::Buffer> timeData{new ::Buffer()};
    android::sp<::Buffer> spBuf{new ::Buffer()};
    const std::string RDG_DID_ECU_USER_DEF_MEMORY_DTC_PROP{"remotediag.prop.UserDefDTC"};
    const std::string didName{RDG_DID_ECU_USER_DEF_MEMORY_DTC_PROP};
    const std::string RDG_DID_ECU_USER_DEF_MEMORY_DTC_INDEX_PROP{"remotediag.prop.UserDefDTCIdx"};
    const std::string didIndex{RDG_DID_ECU_USER_DEF_MEMORY_DTC_INDEX_PROP};
    const char_t *const currDid{Remotediag::getInstance()->getProperty(didName.c_str())};
    const char_t *const index{Remotediag::getInstance()->getProperty(didIndex.c_str())};
    int16_t currentIdx{0};
    uint16_t did{0U};

    if (index != nullptr)
    {
        std::string::size_type sz{}; // alias of size_t
        const int32_t tmp{std::stoi(index, &sz)};
        if (tmp <= INT16_MAX)
        {
            currentIdx = static_cast<int16_t>(tmp);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set property, set to default");
        currentIdx = -16;
    }
    if (currDid != nullptr)
    {
        const uint64_t did_t{std::stoul(currDid, nullptr, 0)};
        if (did_t <= static_cast<uint16_t>(UINT16_MAX))
        {
            did = static_cast<uint16_t>(did_t);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        LOG_I("No set Did, set to default");
        did = PARAM_DID_DATETIME_OF_ECU_USERDEF_MEMORY_DTC_1;
    }
    CommonUtils::convertCurrentTimeToBuffer(timeData);
    currentIdx += 16;
    if ((did >= PARAM_DID_DATETIME_OF_ECU_USERDEF_MEMORY_DTC_3) && (currentIdx > 4080 - 16))
    {
        did = PARAM_DID_DATETIME_OF_ECU_USERDEF_MEMORY_DTC_1;
        currentIdx = 0;
    }
    else if ((did < PARAM_DID_DATETIME_OF_ECU_USERDEF_MEMORY_DTC_3) && (currentIdx > 4080 - 16))
    {
        did += 1U;
        currentIdx = 0;
    }
    else
    {
        // Do nothing
    }
    readDidData(did, spBuf);
    if ((spBuf->data() != nullptr) && (spBuf->size() == 4088U) && (timeData->data() != nullptr))
    {
        const uint32_t canId{notification->getTargetCollectionDataRobSsr()->ecu_address_information().target_address()};
        const uint32_t memorySelection{notification->getTargetCollectionDataRobSsr()->rob_information().memory_selection()};
        const uint32_t userDefMemoryDTC{notification->getTargetCollectionDataRobSsr()->rob_information().rob()};
        spBuf->data()[currentIdx] = 0x00U;
        spBuf->data()[currentIdx + 1] = static_cast<uint8_t>((canId >> 24U) & 0xFFU);
        spBuf->data()[currentIdx + 2] = static_cast<uint8_t>((canId >> 16U) & 0xFFU);
        spBuf->data()[currentIdx + 3] = static_cast<uint8_t>((canId >> 8U) & 0xFFU);
        spBuf->data()[currentIdx + 4] = static_cast<uint8_t>(canId & 0xFFU);

        spBuf->data()[currentIdx + 5] = static_cast<uint8_t>(memorySelection & 0xFFU);

        spBuf->data()[currentIdx + 6] = static_cast<uint8_t>((userDefMemoryDTC >> 16U) & 0xFFU);
        spBuf->data()[currentIdx + 7] = static_cast<uint8_t>((userDefMemoryDTC >> 8U) & 0xFFU);
        spBuf->data()[currentIdx + 8] = static_cast<uint8_t>(userDefMemoryDTC & 0xFFU);
        spBuf->data()[currentIdx + 9] = 0x00U;
        for (uint32_t i{0U}; i < timeData->size(); i++)
        {
            if (currentIdx >= 0)
            {
                spBuf->data()[static_cast<uint32_t>(currentIdx) +10U + i] = timeData->data()[i]; // CID 9389958
            }
            else
            {
                // do nothing
            }
        }

        writeDidData(did, spBuf);
        LOG_I("Current property %d, current did 0x%02X", currentIdx, did);
        Remotediag::getInstance()->setProperty(didName.c_str(), static_cast<int32_t>(did), true);
        Remotediag::getInstance()->setProperty(didIndex.c_str(), static_cast<int32_t>(currentIdx), true);
    }
    else
    {
        (void)currentIdx;
        LOG_E("Invalid buffer size: %lu", spBuf->size());
    }
}

void DiagManagerAdapter::setSRVC(const bool isACFlag, const bool flag)
{
    android::sp<Buffer> spBuf {new Buffer()};
    readDidData(PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);
    LOG_I("setSRVC isAC flag %d, flag: %d", isACFlag, flag);
    if ((spBuf->data() != nullptr) && (spBuf->size() == 2U))
    {
        uint8_t SRVC_flag{0U};
        if (isACFlag)
        {
            SRVC_flag = spBuf->data()[0] & 0xFDU;
            spBuf->data()[0] = SRVC_flag | (flag == true ? 0x02U : 0U); //10b
        }
        else
        {
            SRVC_flag = spBuf->data()[0] & 0xFEU;
            spBuf->data()[0] = SRVC_flag | (flag == true ? 0x01U : 0U);
        }

        writeDidData(PARAM_DID_DATA_COLLECTION_FUNCTION_SERVICE_FLAG, spBuf);
    }
    else
    {
        LOG_E("Invalid buffer size: %lu", spBuf->size());
    }
}

}
