#ifndef ONBOARDCLIENT_SERVICEDEATHRECIPIENT_H
#define ONBOARDCLIENT_SERVICEDEATHRECIPIENT_H

#include <functional>
#include <utils/Handler.h>
namespace OBC
{
    class ServiceDeathRecipient : public android::IBinder::DeathRecipient
    {
    public:
        ServiceDeathRecipient(std::function<void(const android::wp<android::IBinder> &who)> const onBinderDied) : mOnBinderDied(onBinderDied) {}
        ~ServiceDeathRecipient() override = default;
        ServiceDeathRecipient(ServiceDeathRecipient const &) = delete;
        ServiceDeathRecipient &operator=(ServiceDeathRecipient const &) = delete;
        ServiceDeathRecipient(ServiceDeathRecipient &&) = delete;
        ServiceDeathRecipient &operator=(ServiceDeathRecipient &&) = delete;
        void binderDied(const android::wp<android::IBinder> &who) final
        {
            mOnBinderDied(who);
        }

    private:
        std::function<void(const android::wp<android::IBinder> &who)> mOnBinderDied;
    };
};
#endif /* ONBOARDCLIENT_SERVICEDEATHRECIPIENT_H */
