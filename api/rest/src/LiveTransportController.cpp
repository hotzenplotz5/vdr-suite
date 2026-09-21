#include "LiveTransportController.h"

#include "ILiveTransport.h"

LiveTransportController::LiveTransportController(
    const ILiveTransport& transport)
    : transport_(transport)
{
}

ApiResponse LiveTransportController::getStream()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "text/event-stream";
    response.body = transport_.stream();

    return response;
}
