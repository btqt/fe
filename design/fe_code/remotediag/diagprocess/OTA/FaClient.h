#pragma once

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <chrono>
#include <functional>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <thread>
#include <binder/Parcel.h>
#include <utils/Buffer.h>
#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>
#include "utils/Logger.h"
#include "diagprocess/OTA/OtaMessage.h"

namespace rdgapp {

class FaClient : public android::RefBase 
{
    public:
        FaClient(void) noexcept;
        ~FaClient() = default;
        void startup();
        void stop();
        android::sp<OtaMessage> sendData(const android::sp<::Buffer> fadata);
    private:
        int32_t mSocket;
};
} // namespace rdgapp
