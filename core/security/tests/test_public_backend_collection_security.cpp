#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
constexpr const char* Route = "/api/v1/backends";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = Route;
    request.headers["X-Request-ID"] =
        "phase69f-public-backend-collection";
    fixture.addBrowserAuthentication(request);
    return request;
}
}

int main()
{
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "channels.view", "backend-b"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "timers.view", "backend-a"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.read-only", "backend-c"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));

        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a", "backend-b"}));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "*", "*"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));

        assert(decision.allowed);
        assert(decision.authorizedBackendIds ==
            (std::vector<std::string>{"*"}));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.read-only", "backend-c"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));

        assert(decision.allowed);
        assert(decision.authorizedBackendIds.empty());
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "backend-d"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(browserGet(fixture));

        assert(decision.allowed);
        assert(decision.authorizedBackendIds ==
            (std::vector<std::string>{"backend-d"}));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path = Route;

        const SecurityGateDecision decision =
            fixture.gate.evaluate(anonymous);

        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
        assert(decision.rejection.body.find(
            "\"code\":\"unauthorized\"") != std::string::npos);
        assert(decision.rejection.headers.at("Content-Type") ==
            "application/problem+json");
    }

    return 0;
}
