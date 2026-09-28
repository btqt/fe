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
#include <atomic>
#include <binder/Parcel.h>
#include <utils/Buffer.h>
#include <utils/Handler.h>
#include <utils/Log.h>
#include <utils/SLLooper.h>

#include "OtaMessageDefine.h"

namespace rdgapp {

class FaServer : public android::RefBase 
{
    public:
    
        FaServer(const uint16_t port, const android::sp<sl::Handler> aHandler);
        ~FaServer() final = default;
        void startup();
        void stop();
        void runLoop();
        void handleConnection(void);
        void active(const bool action) noexcept;
        void closeCurrentClient();
        void closeListeningSocket();
        ::TIGER_ERR notify(const android::sp<::Buffer> notifyData);
        static uint32_t getPayloadSize(::Buffer& rawData);

        inline bool isRunning() const noexcept {return mIsRunning;};
    private:

        bool mIsRunning;
        bool mIsActive;
        std::atomic<bool> mShouldListeningData;
        std::atomic<bool> mIsListeningDataRunning;
        std::thread mConnectionListenerThread;
        std::thread mDataListenerThread;
        int32_t mListeningSocket;
        std::atomic<int32_t> mClientFd;
        uint16_t mPort;
        android::sp<sl::Handler> mHandler;
};
} // namespace rdgapp
