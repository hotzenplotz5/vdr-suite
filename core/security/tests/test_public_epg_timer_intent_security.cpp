#include "SecurityHttpGateBrowserTestFixture.h"
#include <cassert>
#include <string>

namespace
{
HttpServerRequest createRequest(SecurityHttpGateBrowserTestFixture& fixture,
                                bool csrf = true,
                                const std::string& backendId = "home")
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = "/api/v1/epg/timer-intents";
    request.body = "{\"backendId\":\"" + backendId +
        "\",\"channelId\":\"c\",\"eventId\":\"e\"}";
    request.headers["Content-Type"] = "application/json";
    request.headers["If-Match"] = "\"vsr-1\"";
    request.headers["Idempotency-Key"] = "idem";
    fixture.addBrowserAuthentication(request, csrf);
    return request;
}
}

int main()
{
    {
        // A real administrator Browser Session can use the same native
        // timer CREATE contract as a paired Device, but with Browser CSRF.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "*"));
        auto result = fixture.gate.evaluate(createRequest(fixture));
        assert(result.allowed);
        assert(result.protectedMutation);
        assert(result.authorizationDecision.allowed);
        assert(result.authorizationDecision.permission == "timers.create");
        assert(result.authorizationDecision.action == "native_timers.create");
        assert(result.authorizationDecision.backendId == "home");
        result = fixture.gate.evaluate(createRequest(fixture, false));
        assert(!result.allowed && result.rejection.statusCode == 403);
    }
    {
        // Backend-scoped administrative authority cannot cross backends.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "home"));
        assert(fixture.gate.evaluate(createRequest(fixture)).allowed);
        auto denied = fixture.gate.evaluate(
            createRequest(fixture, true, "other"));
        assert(!denied.allowed && denied.rejection.statusCode == 403);
    }
    {
        // Ordinary Human users cannot become administrators just by
        // receiving a backend-scoped timers.create grant.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "timers.create", "home"));
        auto denied = fixture.gate.evaluate(createRequest(fixture));
        assert(!denied.allowed && denied.rejection.statusCode == 403);
        HttpServerRequest anonymous = createRequest(fixture);
        anonymous.headers.erase("Cookie");
        denied = fixture.gate.evaluate(anonymous);
        assert(!denied.allowed && denied.rejection.statusCode == 401);
    }
    return 0;
}
