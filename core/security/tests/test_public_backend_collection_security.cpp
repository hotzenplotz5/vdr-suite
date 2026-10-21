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
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "addons.media.import", "backend-a"));
        HttpServerRequest request = browserGet(fixture);
        request.path = "/api/v1/backends/backend-a/capabilities/media-import";
        const auto decision = fixture.gate.evaluate(request);
        assert(decision.allowed);
        assert(decision.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a"}));
        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path = request.path;
        const auto blocked = fixture.gate.evaluate(anonymous);
        assert(!blocked.allowed);
        assert(blocked.rejection.statusCode == 401);
    }

    {
        // Protected A7.3 mutation inherits the same CSRF and accountability
        // behavior as other API-v1 operations.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "addons.media.import", "backend-a"));
        HttpServerRequest post;
        post.method = "POST";
        post.path = "/api/v1/backends/backend-a/media-imports";
        post.body = "{\"sourceId\":\"opaque_source_123456\"}";
        post.headers["Content-Type"] = "application/json";
        fixture.addBrowserAuthentication(post, true);
        const auto allowed = fixture.gate.evaluate(post);
        assert(allowed.allowed && allowed.protectedMutation);
        assert(allowed.authorizationDecision.permission ==
            "addons.media.import");
        assert(allowed.authorizationDecision.action == "media.import");
        assert(allowed.authorizationDecision.backendId == "backend-a");

        fixture.addBrowserAuthentication(post, false);
        post.headers.erase("X-CSRF-Token");
        const auto csrf = fixture.gate.evaluate(post);
        assert(!csrf.allowed && csrf.rejection.statusCode == 403);

        HttpServerRequest anonymous;
        anonymous.method = "POST";
        anonymous.path = post.path;
        anonymous.body = post.body;
        const auto unauthenticated = fixture.gate.evaluate(anonymous);
        assert(!unauthenticated.allowed);
        assert(unauthenticated.rejection.statusCode == 401);
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
