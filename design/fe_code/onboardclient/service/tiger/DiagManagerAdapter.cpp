#define LOG_TAG "DiagManagerAdapter"
#include <Log.h>
#include "DiagManagerAdapter.h"
#include "OnboardclientRxHandler.h"
namespace OBC
{
    DiagManagerAdapter *DiagManagerAdapter::mDiagManagerAdapter{nullptr};
    DiagManagerAdapter::DiagManagerAdapter() : android::RefBase()
    {
        LOGI("DiagManagerAdapter Constructor");
        mDiagManagerAdapter = this;
        mServiceDeathRecipient = new ServiceDeathRecipient([this](const android::wp<android::IBinder> &who)
                                                           { this->onBinderDied(who); });
    }

    DiagManagerAdapter::~DiagManagerAdapter() noexcept
    {
        if (DiagManagerAdapter::mDiagManagerAdapter != nullptr)
        {
            delete mDiagManagerAdapter;
            mDiagManagerAdapter = nullptr;
        }
    }

    DiagManagerAdapter *DiagManagerAdapter::getInstance()
    {
        if (mDiagManagerAdapter == nullptr)
        {
            mDiagManagerAdapter = new DiagManagerAdapter();
        }
        return mDiagManagerAdapter;
    }

    void DiagManagerAdapter::registerService()
    {
        LOGI("DiagManagerAdapter registerService");
        mOnboardclientInputHandler = OnboardclientInputManager::OnboardclientHandler::getInstance();
        mDiagMService = android::interface_cast<IDiagManagerService>(
            android::defaultServiceManager()->getService(android::String16("service_layer.DiagManagerService")));
        if (mDiagMService != nullptr)
        {
            LOGI("DiagManagerAdapter registered");
            if (mDiagReceiver != nullptr)
            {
                mDiagReceiver.clear();
            }

            mDiagReceiver = new DiagMReceiver(*this);
            (void)mDiagMService->registerDiagReceiver(mDiagReceiver, MASK_FOR_TRANSFER_DATA, 0x00U);
#ifdef _MORE_MARSHM
            (void)android::IInterface::asBinder(mDiagMService)->linkToDeath(mServiceDeathRecipient);
#else  // !(_MORE_MARSHM)
            (void)mDiagMService->asBinder()->linkToDeath(mServiceDeathRecipient);
#endif // _MORE_MARSHM
        }
        else
        {
            LOGE("Register diag service fail, try again after 500ms");
            if (mOnboardclientInputHandler != nullptr)
            {
                (void)mOnboardclientInputHandler->sendMessageDelayed(mOnboardclientInputHandler->obtainMessage(OBC_INPUT_HANDLER::MSG_OBC_CONNECT_TO_DIAGMGR),
                                                                     OBC_COMMON::TIME_SEND_RETRY_DELAY_MS);
            }
        }
    }

    void DiagManagerAdapter::onBinderDied(const android::wp<android::IBinder> &who)
    {
        LOGI("DiagManagerAdapter onBinderDied");
        const Mutex::Autolock lock{Mutex::Autolock(mDiedLock)};
#ifdef _MORE_MARSHM
        if ((mDiagMService != nullptr) && (android::IInterface::asBinder(mDiagMService) == who))
        {
#else  // !(_MORE_MARSHM)
        if ((mDiagMService != nullptr) && (mDiagMService->asBinder() == who))
        {
#endif // _MORE_MARSHM
            if (mOnboardclientInputHandler != nullptr)
            {
                (void)mOnboardclientInputHandler->obtainMessage(OBC_INPUT_HANDLER::MSG_OBC_CONNECT_TO_DIAGMGR)->sendToTarget();
            }
        }
        else
        {
            // Do nothing
        }
    }

    error_t DiagManagerAdapter::exeUdsRequest(const uint8_t source, const sp<Buffer> request, const sp<Buffer> response)
    {
        mDiagMService = android::interface_cast<IDiagManagerService>(
            android::defaultServiceManager()->getService(android::String16("service_layer.DiagManagerService")));
        error_t res{E_OK};
        if (mDiagMService != nullptr)
        {
            res = mDiagMService->executeUdsRequest(source, request, response);
        }
        else
        {
            res = E_ERROR;
            LOGE("Diag Manager is null!!");
        }

        if (res == static_cast<int32_t>(E_OK))
        {
        }

        return res;
    }
};
