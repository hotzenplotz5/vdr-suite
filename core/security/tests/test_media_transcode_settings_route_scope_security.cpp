#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* ReadPermission =
    "backend.settings.media-transcode.read";
constexpr const char* ModifyPermission =
    "backend.settings.media-transcode.modify";

HttpServerRequest browserGet(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& path)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = path;
    request.headers["X-Request-ID"] = "phase65-media-settings-get";
    fixture.addBrowserAuthentication(request);
    return request;
}

HttpServerRequest browserPost(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& path,
    const std::string& bodyBackend,
    bool includeCsrf)
{
    HttpServerRequest request = fixture.mutationRequest(
        path,
        bodyBackend);
    request.body =
        "{\"backendId\":\"" + bodyBackend +
        "\",\"videoEncoderMode\":\"software\","
        "\"operationId\":\"phase65-media-settings-operation\"}";
    fixture.addBrowserAuthentication(request, includeCsrf);
    return request;
}

bool hasEvent(
    const AccountabilityEventRepository& repository,
    const std::string& eventType,
    const std::string& reasonCode,
    const std::string& backendId,
    const std::string& outcome)
{
    for (const AccountabilityEvent& event : repository.listAll())
    {
        if ((!eventType.empty() && event.eventType != eventType) ||
            (!reasonCode.empty() && event.reasonCode != reasonCode) ||
            (!backendId.empty() && event.backendId != backendId) ||
            (!outcome.empty() && event.outcome != outcome))
        {
            continue;
        }
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
            fixture.actorId,
            "role.admin",
            "backend-b"));

        const SecurityGateDecision decision = fixture.gate.evaluate(
            browserGet(
                fixture,
                "/api/backends/backend-b/settings/media-transcode"));
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(decision.authorizationDecision.permission == ReadPermission);
        assert(decision.authorizationDecision.backendId == "backend-b");
        assert(hasEvent(
            fixture.accountabilityRepository,
            "authorization.allowed",
            "role_permission_granted",
            "backend-b",
            "dispatch_authorized"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            ReadPermission,
            "backend-a"));

        const SecurityGateDecision decision = fixture.gate.evaluate(
            browserGet(
                fixture,
                "/api/backends/backend-b/settings/media-transcode"));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "backend_scope_denied") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "backend-b"));

        const SecurityGateDecision decision = fixture.gate.evaluate(
            browserPost(
                fixture,
                "/api/backends/backend-b/settings/media-transcode",
                "backend-a",
                true));
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(decision.authorizationDecision.permission == ModifyPermission);
        assert(decision.authorizationDecision.backendId == "backend-b");
        assert(decision.operationId == "phase65-media-settings-operation");
        assert(fixture.gate.appendProtectedMutationOutcome(decision, 200));
        assert(hasEvent(
            fixture.accountabilityRepository,
            "operation.succeeded",
            "http_status_200",
            "backend-b",
            "succeeded"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.read-only",
            "backend-b"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            ModifyPermission,
            "backend-b"));

        const SecurityGateDecision decision = fixture.gate.evaluate(
            browserPost(
                fixture,
                "/api/backends/backend-b/settings/media-transcode",
                "backend-b",
                true));
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "role_read_only") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "backend-b"));

        const SecurityGateDecision missingCsrf = fixture.gate.evaluate(
            browserPost(
                fixture,
                "/api/backends/backend-b/settings/media-transcode",
                "backend-b",
                false));
        assert(!missingCsrf.allowed);
        assert(missingCsrf.rejection.statusCode == 403);
        assert(missingCsrf.rejection.body.find(
            "csrf_validation_failed") != std::string::npos);

        HttpServerRequest invalid = browserPost(
            fixture,
            "/api/backends/backend-b/settings/media-transcode",
            "backend-b",
            false);
        invalid.headers["X-CSRF-Token"] = "invalid-test-token";
        const SecurityGateDecision invalidCsrf = fixture.gate.evaluate(invalid);
        assert(!invalidCsrf.allowed);
        assert(invalidCsrf.rejection.statusCode == 403);
        assert(invalidCsrf.rejection.body.find(
            "csrf_validation_failed") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "backend-b"));

        const SecurityGateDecision encoded = fixture.gate.evaluate(
            browserGet(
                fixture,
                "/api/backends/backend%2Fb/settings/media-transcode"));
        assert(!encoded.allowed);
        assert(encoded.rejection.statusCode == 400);
        assert(encoded.rejection.body.find(
            "invalid_backend_scope") != std::string::npos);

        const SecurityGateDecision nested = fixture.gate.evaluate(
            browserGet(
                fixture,
                "/api/backends/backend/b/settings/media-transcode"));
        assert(!nested.allowed);
        assert(nested.rejection.statusCode == 400);
        assert(nested.rejection.body.find(
            "invalid_backend_scope") != std::string::npos);
    }

    return 0;
}
