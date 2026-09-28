#ifndef COMMON_DEF_H
#define COMMON_DEF_H

#include <rdg/v1/interfaces/RDG_common_message_definition.pb.h>
#include <Typedef.h>
namespace rdgapp {

namespace CommonDefine
{
    // enum CommunicationProtocol
    // {
    //     CP_UNKNOWN = 0, /* Unknown (default) / 未設定（デフォルト値） */
    //     CP_CAN = 1,     /* Classic CAN       / クラシックCAN          */
    //     CP_CAN_FD = 2,  /* CAN FD            / CAN-FD                 */
    //     CP_IP = 3,      /* IP                / IP                     */
    // };

    // enum CommunicationType
    // {
    //     CT_UNKNOWN = 0,        /* Unknown (default) / 未設定（デフォルト値） */
    //     CT_CAN_ID_11_BITS = 1, /* 11bits CAN ID     / 11ビットCAN-ID         */
    //     CT_CAN_ID_29_BITS = 2, /* 29bits CAN ID     / 29ビットCAN-ID         */
    // };

    enum class DiagPhase : int32_t
    {
        DP_UNKNOW = 0,
        DP_PHASE_4 = 4, 
        DP_PHASE_5 = 5, 
        DP_PHASE_6 = 6, 
        DP_OTHER = 99   
    };

    class EcuInformation
    {
        public:
            EcuInformation() noexcept
            : ecuActiveFlag(false)
            , commProtocol(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol_CP_UNKNOWN)
            , commType(vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType_CT_UNKNOWN)
            , targetAddress(0U)
            , canId(0U)
            , nTa(static_cast<uint8_t>(0U))
            , diagnosticPhase(DiagPhase::DP_UNKNOW)
            {}

            void setecuActiveFlag(const bool val) noexcept {ecuActiveFlag = val;}
            bool getecuActiveFlag() const noexcept {return ecuActiveFlag;}
            void setCommProtocol(const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol val) noexcept {commProtocol = val;}
            vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol getCommProtocol() const noexcept {return commProtocol;}
            void setCommType(const vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType val) noexcept {commType = val;}
            vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType getCommType() const noexcept {return commType;}
            void setTargetAddress(const uint32_t val) noexcept {targetAddress = val;}
            uint32_t getTargetAddress() const noexcept {return targetAddress;}
            void setCanId(const uint32_t val) noexcept {canId = val;}
            uint32_t getCanId() const noexcept {return canId;}
            void setNTa(const uint8_t val) noexcept {nTa = val;}
            uint8_t getNTa() const noexcept {return nTa;}
            void setDiagPhase(const DiagPhase val) noexcept {diagnosticPhase = val;}
            DiagPhase getDiagPhase() const noexcept {return diagnosticPhase;}
        private:
            bool ecuActiveFlag;
            vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationProtocol commProtocol;
            vccomif::rdg::v1::interfaces::EcuAddressInformation_CommunicationType commType;
            uint32_t targetAddress;
            uint32_t canId;
            uint8_t nTa;
            DiagPhase diagnosticPhase;
    } __attribute__((__packed__));

    class UploadFileAttribute
    {
        public:
            void setpriority(const uint32_t val)noexcept{priority = val;};
            void setFileType(const uint32_t val)noexcept{fileType = val;};
            void setFileSize(const uint64_t val)noexcept{fileSize = val;};
            void setFilePath(const std::string val)noexcept{filePath = val;};
            uint32_t getPriority()const noexcept{return priority;};
            uint32_t getFileType()const noexcept{return fileType;};
            uint64_t getFileSize()const noexcept{return fileSize;};
            std::string getFilePath()const noexcept{return filePath;};
        private:
            uint32_t priority;
            uint32_t fileType;
            uint64_t fileSize;
            std::string filePath;
    };

    class RDGLocationData : public android::RefBase
    {
        public:
            RDGLocationData() noexcept
            {
                latitude = 0;
                longitude = 0;
            }

            void setLatitude(const int32_t val) noexcept {latitude = val;};
            void setLongitude(const int32_t val) noexcept {longitude = val;};
            int32_t getLatitude() const noexcept {return latitude;};
            int32_t getLongtitude() const noexcept {return longitude;};
            
        private:
            int32_t latitude;
            int32_t longitude;
    } __attribute__((__packed__));
}
}
#endif // COMMON_DEF_H
