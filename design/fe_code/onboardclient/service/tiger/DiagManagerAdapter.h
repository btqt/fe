#ifndef DIAG_MANAGER_ADAPTER_H
#define DIAG_MANAGER_ADAPTER_H

#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include <utils/Handler.h>
#include <utils/SLLooper.h>
#include <utils/Timer.h>
#include <utils/external/mindroid/lang/String.h>
#include <utils/Mutex.h>

#include <binder/IServiceManager.h>

#include <services/DiagManagerService/IDiagManagerService.h>
#include <services/DiagManagerService/IDiagManagerServiceType.h>
#include <services/DiagManagerService/IDiagManagerReceiver.h>
#include <services/DiagManagerService/DiagDtcItemList.h>
#include <services/DiagManagerService/DiagType.h>
#include <services/DiagManagerService/OEM_DiagData_Config.h>
#include "OnboardclientInputManager.h"
#include "ServiceDeathReceipient.hpp"
namespace OBC
{
    class DiagManagerAdapter : public android::RefBase
    {
    public:
        DiagManagerAdapter();
        ~DiagManagerAdapter() noexcept override;
        DiagManagerAdapter(DiagManagerAdapter const &) = default;
        DiagManagerAdapter &operator=(DiagManagerAdapter const &) = default;
        DiagManagerAdapter(DiagManagerAdapter &&) = delete;
        DiagManagerAdapter &operator=(DiagManagerAdapter &&) = delete;
        static DiagManagerAdapter *getInstance();
        void registerService();
        error_t exeUdsRequest(const uint8_t source, const sp<Buffer> request, const sp<Buffer> response);

    private:
        void onBinderDied(const android::wp<android::IBinder> &who);
        static DiagManagerAdapter *mDiagManagerAdapter;
        android::sp<sl::Handler> mOnboardclientInputHandler;
        android::sp<ServiceDeathRecipient> mServiceDeathRecipient{nullptr};
        android::sp<IDiagManagerReceiver> mDiagReceiver{nullptr};
        android::sp<IDiagManagerService> mDiagMService{nullptr};
        mutable android::Mutex mDiedLock;
    };

    class DiagMReceiver : public BnDiagManagerReceiver
    {
    public:
        DiagMReceiver(DiagManagerAdapter &sDiagMRC) noexcept : diagMRC(sDiagMRC) {}
        virtual ~DiagMReceiver() = default;
        DiagMReceiver(DiagMReceiver const &) = default;
        DiagMReceiver &operator=(DiagMReceiver const &) = default;
        DiagMReceiver(DiagMReceiver &&) = delete;
        DiagMReceiver &operator=(DiagMReceiver &&) = delete;

    private:
        DiagManagerAdapter &diagMRC;
    };
};
#endif // DIAG_MANAGER_ADAPTER_H
