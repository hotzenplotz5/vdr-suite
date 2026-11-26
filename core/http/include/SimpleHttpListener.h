#ifndef SIMPLE_HTTP_LISTENER_H
#define SIMPLE_HTTP_LISTENER_H

#include "IHttpServer.h"

#include <chrono>
#include <functional>
#include <memory>

class DeviceLiveEventSubscription;
class BrowserLiveEventSubscription;
#include <string>
#include <sys/types.h>

class SimpleHttpListener {
public:
    SimpleHttpListener(
        std::string host,
        int port,
        IHttpServer& server);

    SimpleHttpListener(
        std::string host,
        int port,
        IHttpServer& server,
        std::function<bool()> shouldStop);

    SimpleHttpListener(
        std::string host,
        int port,
        IHttpServer& server,
        std::function<bool()> shouldStop,
        std::function<void()> onTick);

    SimpleHttpListener(
        std::string host,
        int port,
        IHttpServer& server,
        std::function<bool()> shouldStop,
        std::function<void()> onTick,
        std::chrono::milliseconds clientIoTimeout);

    void configureLocalOsdSocket(std::string path, uid_t uid);
    int runUntilStopped();

private:
    std::string host_;
    int port_;
    IHttpServer& server_;
    std::function<bool()> shouldStop_;
    std::function<void()> onTick_;
    std::chrono::milliseconds clientIoTimeout_;

    std::string localOsdPath_;
    uid_t localOsdUid_ = 0;
    int createLocalOsdSocket() const;
    int createListeningSocket() const;
    bool prepareClientResponse(
        int clientSocket,
        bool localOsdPeer,
        std::string& rawResponse,
        bool& imageResponse,
        std::function<std::string()>& deferredImageResponse,
        std::string& streamBodyPath,
        std::shared_ptr<DeviceLiveEventSubscription>& deviceEvents,
        std::shared_ptr<BrowserLiveEventSubscription>& browserEvents) const;
};

#endif
