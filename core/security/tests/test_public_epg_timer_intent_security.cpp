#include "SecurityHttpGateBrowserTestFixture.h"
#include <cassert>
#include <string>

namespace
{
HttpServerRequest createRequest(SecurityHttpGateBrowserTestFixture& fixture,
                                bool csrf = true,
                                const std::string& backendId = "home",
                                bool manual = false)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = manual
        ? "/api/v1/manual/timer-intents"
        : "/api/v1/epg/timer-intents";
    request.body = manual
        ? "{\"backendId\":\"" + backendId +
          "\",\"channelId\":\"c\",\"title\":\"Manual\",\"directory\":\"\","
          "\"startAt\":1795547700,\"stopAt\":1795552200,\"priority\":50,"
          "\"lifetime\":99,\"enabled\":true,\"vps\":false}"
        : "{\"backendId\":\"" + backendId +
          "\",\"channelId\":\"c\",\"eventId\":\"e\"}";
    request.headers["Content-Type"] = "application/json";
    if (!manual) request.headers["If-Match"] = "\"vsr-1\"";
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
        const auto manual = fixture.gate.evaluate(
            createRequest(fixture, true, "home", true));
        assert(manual.allowed && manual.protectedMutation);
        assert(manual.authorizationDecision.permission == "timers.create");
        assert(manual.authorizationDecision.action == "native_timers.create");
        result = fixture.gate.evaluate(
            createRequest(fixture, false, "home", true));
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
        assert(fixture.gate.evaluate(
            createRequest(fixture, true, "home", true)).allowed);
        denied = fixture.gate.evaluate(
            createRequest(fixture, true, "other", true));
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
        denied = fixture.gate.evaluate(
            createRequest(fixture, true, "home", true));
        assert(!denied.allowed && denied.rejection.statusCode == 403);
        HttpServerRequest anonymous = createRequest(fixture);
        anonymous.headers.erase("Cookie");
        denied = fixture.gate.evaluate(anonymous);
        assert(!denied.allowed && denied.rejection.statusCode == 401);
    }
    return 0;
}
