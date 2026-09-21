#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission = "osd.view";
constexpr const char* CreateRoute = "/api/vdr/legacy-osd/sessions";
constexpr const char* StatusRoute =
    "/api/vdr/legacy-osd/sessions/status";

HttpServerRequest browserCreate(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& backendId,
    bool csrf = true)
{
    HttpServerRequest request =
        fixture.mutationRequest(CreateRoute, backendId);
    fixture.addBrowserAuthentication(request, csrf);
    return request;
}

HttpServerRequest browserStatus(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& backendId)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = std::string(StatusRoute) +
        "?backend=" + backendId + "&session=los_security_001";
    request.headers["X-Request-ID"] = "phase68d-osd-view";
    fixture.addBrowserAuthentication(request);
    return request;
}
}

int main()
{
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "default"));

        const auto create =
            fixture.gate.evaluate(browserCreate(fixture, "default"));
        assert(create.allowed);
        assert(!create.protectedMutation);
        assert(create.authorizationDecision.permission == Permission);
        assert(create.authorizationDecision.action == "osd.session.create");

        const auto status =
            fixture.gate.evaluate(browserStatus(fixture, "default"));
        assert(status.allowed);
        assert(!status.protectedMutation);
        assert(status.authorizationDecision.permission == Permission);
        assert(status.authorizationDecision.action == "osd.session.status");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "other"));
        const auto denied =
            fixture.gate.evaluate(browserCreate(fixture, "default"));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
        assert(denied.rejection.body.find("backend_scope_denied") !=
            std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "default"));
        const auto noCsrf =
            fixture.gate.evaluate(browserCreate(fixture, "default", false));
        assert(!noCsrf.allowed);
        assert(noCsrf.rejection.statusCode == 403);
        assert(noCsrf.rejection.body.find("csrf_validation_failed") !=
            std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.read-only", "default"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "default"));
        assert(fixture.gate.evaluate(
            browserCreate(fixture, "default")).allowed);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "default"));
        const auto denied =
            fixture.gate.evaluate(browserStatus(fixture, "default"));
        assert(!denied.allowed);
        assert(denied.rejection.statusCode == 403);
        assert(denied.rejection.body.find("permission_denied") !=
            std::string::npos);

        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, Permission, "default"));
        const auto explicitView =
            fixture.gate.evaluate(browserStatus(fixture, "default"));
        assert(explicitView.allowed);
        assert(explicitView.authorizationDecision.reasonCode ==
            "permission_granted");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        const auto invalid =
            fixture.gate.evaluate(browserStatus(fixture, "bad/backend"));
        assert(!invalid.allowed);
        assert(invalid.rejection.statusCode == 400);
        assert(invalid.rejection.body.find("invalid_backend_scope") !=
            std::string::npos);
    }
    return 0;
}
