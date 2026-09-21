#include "ContinueWatchingSecurityRequest.h"

#include <cassert>

int main()
{
    HttpServerRequest progress;
    progress.method = "POST";
    progress.path = "/api/media/continue-watching";
    progress.body =
        "{\"operation\":\"progress\",\"backendId\":\"default\","
        "\"recordingId\":\"rec-1\"}";
    progress.headers["X-VDR-Suite-CSRF"] = "phase66-test-token";

    const HttpServerRequest authorized =
        ContinueWatchingSecurityRequest::forAuthorization(progress);
    assert(authorized.method == "POST");
    assert(authorized.path == "/api/media/sessions");
    assert(authorized.body == progress.body);
    assert(authorized.headers == progress.headers);

    HttpServerRequest queried = progress;
    queried.path = "/api/media/continue-watching?unused=1";
    assert(ContinueWatchingSecurityRequest::matches(queried));
    assert(ContinueWatchingSecurityRequest::forAuthorization(queried).path ==
        "/api/media/sessions");

    HttpServerRequest history = progress;
    history.path = "/api/media/recently-watched";
    history.body =
        "{\"operation\":\"activity\",\"backendId\":\"default\","
        "\"recordingId\":\"rec-1\"}";
    const HttpServerRequest historyAuthorized =
        ContinueWatchingSecurityRequest::forAuthorization(history);
    assert(ContinueWatchingSecurityRequest::matches(history));
    assert(historyAuthorized.path == "/api/media/sessions");
    assert(historyAuthorized.body == history.body);
    assert(historyAuthorized.headers == history.headers);

    HttpServerRequest historyQueried = history;
    historyQueried.path = "/api/media/recently-watched?unused=1";
    assert(ContinueWatchingSecurityRequest::matches(historyQueried));

    HttpServerRequest get = progress;
    get.method = "GET";
    assert(!ContinueWatchingSecurityRequest::matches(get));
    assert(ContinueWatchingSecurityRequest::forAuthorization(get).path ==
        get.path);

    HttpServerRequest unrelated = progress;
    unrelated.path = "/api/media/continue-watching-history";
    assert(!ContinueWatchingSecurityRequest::matches(unrelated));
    assert(ContinueWatchingSecurityRequest::forAuthorization(unrelated).path ==
        unrelated.path);

    return 0;
}
