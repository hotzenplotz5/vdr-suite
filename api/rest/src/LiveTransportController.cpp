#include "LiveTransportController.h"

#include "ILiveTransport.h"

std::shared_ptr<BrowserLiveEventSubscription>
LiveTransportController::subscribeBrowser(const std::string& lastEventId) const
{
    return transport_.subscribeBrowser(lastEventId);
}

LiveTransportController::LiveTransportController(
    const ILiveTransport& transport)
    : transport_(transport)
{
}

ApiResponse LiveTransportController::getStream(const std::string& lastEventId)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "text/event-stream";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Accel-Buffering"] = "no";
    response.body = transport_.replay(lastEventId);

    return response;
}
