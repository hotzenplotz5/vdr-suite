#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>
#include <vector>

namespace {
constexpr const char* Permission = "recordings.view";
constexpr const char* Route = "/api/v1/recordings";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& target)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = target;
    request.headers["X-Request-ID"] = "r2-recording-security";
    fixture.addBrowserAuthentication(request);
    return request;
}

bool hasDecision(
    const AccountabilityEventRepository& repository,
    const std::string& reason,
    const std::string& backend)
{
    for (const AccountabilityEvent& event : repository.listAll())
    {
        if (event.permission == Permission &&
            event.backendId == backend && event.reasonCode == reason)
            return true;
    }
    return false;
}
}

int main()
{
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-a"));
        const auto decision = fixture.gate.evaluate(
            browserGet(fixture, std::string(Route) +
                "?backendId=backend-a&limit=50"));
        assert(decision.allowed);
        assert(decision.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a"}));
        assert(hasDecision(fixture.accountabilityRepository,
            "permission_granted", "backend-a"));
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-a"));
        const auto decision = fixture.gate.evaluate(
            browserGet(fixture, std::string(Route) +
                "?backendId=backend-a&view=folders&limit=30&offset=0"));
        assert(decision.allowed);
        assert(decision.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a"}));
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "channels.view", "backend-a"));
        const auto decision = fixture.gate.evaluate(
            browserGet(fixture, std::string(Route) +
                "?backendId=backend-a"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.authorizedBackendIds.empty());
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-a"));
        const auto decision = fixture.gate.evaluate(
            browserGet(fixture, std::string(Route) +
                "?backendId=backend-b"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(hasDecision(fixture.accountabilityRepository,
            "backend_scope_denied", "backend-b"));
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "*"));
        const auto decision = fixture.gate.evaluate(
            browserGet(fixture, std::string(Route) +
                "?backendId=backend-a"));
        assert(decision.allowed);
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        for (const std::string& invalid : {
            std::string(Route),
            std::string(Route) + "?backendId=",
            std::string(Route) + "?backendId=a&backendId=b",
            std::string(Route) + "?backendId=%2Fprivate",
        })
        {
            const auto decision = fixture.gate.evaluate(
                browserGet(fixture, invalid));
            assert(!decision.allowed);
            assert(decision.rejection.statusCode == 400);
        }
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path = std::string(Route) + "?backendId=backend-a";
        const auto decision = fixture.gate.evaluate(anonymous);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    // Public-v1 Genre collections use the same recordings.view authorization.
    for (const std::string& target : {
        "/api/v1/genres?backendId=backend-a&limit=30",
        "/api/v1/genres/recordings?backendId=backend-a&genreId=crime&limit=30"
    })
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-a"));
        const auto decision = fixture.gate.evaluate(
            browserGet(fixture, target));
        assert(decision.allowed);
        assert(decision.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a"}));
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "backend-a"));
        const auto decision = fixture.gate.evaluate(
            browserGet(fixture, "/api/v1/genres?backendId=backend-b"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
    }
    return 0;
}
