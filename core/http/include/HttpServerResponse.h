#ifndef HTTP_SERVER_RESPONSE_H
#define HTTP_SERVER_RESPONSE_H

#include <map>
#include <memory>

class DeviceLiveEventSubscription;
class BrowserLiveEventSubscription;
#include <string>

struct HttpServerResponse {
    int statusCode = 200;
    std::map<std::string, std::string> headers;
    std::string body;

    // When non-empty the listener sends headers without Content-Length and
    // streams this already-authorized local FIFO/file until EOF or disconnect.
    // The path is never serialized into the HTTP response.
    std::string streamBodyPath;

    // Authenticated, backend-scoped incremental Device events. Never use a
    // media FIFO or a serialized credential URL for this response.
    std::shared_ptr<DeviceLiveEventSubscription> deviceEventSubscription;

    // One authorized browser SSE subscription, drained off the listener
    // thread with bounded backlog and a periodic reconnect for re-admission.
    std::shared_ptr<BrowserLiveEventSubscription> browserEventSubscription;
};

#endif
