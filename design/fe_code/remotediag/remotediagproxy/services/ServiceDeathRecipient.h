#ifndef REMOTEDIAGPROXY_SERVICEDEATHRECIPIENT_H
#define REMOTEDIAGPROXY_SERVICEDEATHRECIPIENT_H

#include <functional>
#include <utils/Handler.h>

// namespace rdgapp {

class ServiceDeathRecipient : public android::IBinder::DeathRecipient {
    public:
        ServiceDeathRecipient(std::function<void (const android::wp<android::IBinder>& who)> const onBinderDied) : mOnBinderDied(onBinderDied) {}
        virtual ~ServiceDeathRecipient() = default;
        ServiceDeathRecipient(ServiceDeathRecipient const&) = default;
        ServiceDeathRecipient& operator=(ServiceDeathRecipient const&) = default;
        ServiceDeathRecipient(ServiceDeathRecipient&&) = delete;
        ServiceDeathRecipient& operator=(ServiceDeathRecipient&&) = delete;
        virtual void binderDied(const android::wp<android::IBinder>& who){
            mOnBinderDied(who);
        }
    private:
        std::function<void (const android::wp<android::IBinder>& who)> mOnBinderDied;
};
// }
#endif /* REMOTEDIAGPROXY_SERVICEDEATHRECIPIENT_H */
