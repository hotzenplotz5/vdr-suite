#include "SecurityHttpGateBrowserTestFixture.h"
#include <cassert>
#include <string>

int main()
{
    const std::string route="/api/v1/epg/timer-intents";
    SecurityHttpGateBrowserTestFixture fixture;
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,"timers.create","home"));
    HttpServerRequest request;
    request.method="POST"; request.path=route;
    request.body="{\\"backendId\\":\\"home\\",\\"channelId\\":\\"c\\",\\"eventId\\":\\"e\\"}";
    request.headers["Content-Type"]="application/json";
    request.headers["If-Match"]="\\\"vsr-1\\\"";
    request.headers["Idempotency-Key"]="idem";
    fixture.addBrowserAuthentication(request,true);
    // A Browser session with timers.create may use legacy Browser routes,
    // but never impersonates a paired Device for the new Client contract.
    auto denied=fixture.gate.evaluate(request);
    assert(!denied.allowed && denied.rejection.statusCode==403);
    assert(denied.rejection.body.find("device_credential_required")!=std::string::npos);
    request.headers.erase("Cookie");
    denied=fixture.gate.evaluate(request);
    assert(!denied.allowed && denied.rejection.statusCode==401);
    return 0;
}
