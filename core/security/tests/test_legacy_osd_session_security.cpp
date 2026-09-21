#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission = "osd.view";
constexpr const char* CreateRoute = "/api/vdr/legacy-osd/sessions";
constexpr const char* StatusRoute =
    "/api/vdr/legacy-osd/sessions/status";
constexpr const char* ViewerAttachRoute =
    "/api/vdr/legacy-osd/viewers";
constexpr const char* ViewerDetachRoute =
    "/api/vdr/legacy-osd/viewers/detach";
constexpr const char* ViewerFrameRoute =
    "/api/vdr/legacy-osd/viewers/frame";

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

HttpServerRequest browserViewerPost(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& route,
    const std::string& backendId,
    bool csrf = true)
{
    HttpServerRequest request =
        fixture.mutationRequest(route, backendId);
    fixture.addBrowserAuthentication(request, csrf);
    return request;
}

HttpServerRequest browserViewerFrame(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& backendId)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = std::string(ViewerFrameRoute) +
        "?backend=" + backendId +
        "&session=los_security_001&viewer=ovb_security_001&ack=0";
    request.headers["X-Request-ID"] = "phase68e-osd-viewer";
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

        const auto attach = fixture.gate.evaluate(
            browserViewerPost(fixture, ViewerAttachRoute, "default"));
        assert(attach.allowed);
        assert(!attach.protectedMutation);
        assert(attach.authorizationDecision.permission == Permission);
        assert(attach.authorizationDecision.action == "osd.viewer.attach");

        const auto frame =
            fixture.gate.evaluate(browserViewerFrame(fixture, "default"));
        assert(frame.allowed);
        assert(!frame.protectedMutation);
        assert(frame.authorizationDecision.permission == Permission);
        assert(frame.authorizationDecision.action == "osd.viewer.frame");

        const auto detach = fixture.gate.evaluate(
            browserViewerPost(fixture, ViewerDetachRoute, "default"));
        assert(detach.allowed);
        assert(!detach.protectedMutation);
        assert(detach.authorizationDecision.permission == Permission);
        assert(detach.authorizationDecision.action == "osd.viewer.detach");
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

        const auto viewerNoCsrf = fixture.gate.evaluate(
            browserViewerPost(
                fixture, ViewerAttachRoute, "default", false));
        assert(!viewerNoCsrf.allowed);
        assert(viewerNoCsrf.rejection.statusCode == 403);
        assert(viewerNoCsrf.rejection.body.find("csrf_validation_failed") !=
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
        const auto viewerDenied =
            fixture.gate.evaluate(browserViewerFrame(fixture, "default"));
        assert(!viewerDenied.allowed);
        assert(viewerDenied.rejection.statusCode == 403);

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
