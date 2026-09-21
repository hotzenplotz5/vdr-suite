#include "SecurityHttpGateBrowserTestFixture.h"
#include "SeriesArtworkSettingsSecurityRequest.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* Permission =
    "backend.settings.series-artwork.modify";

HttpServerRequest browserRequest(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& path,
    const std::string& bodyBackend,
    bool includeCsrf)
{
    HttpServerRequest request = fixture.mutationRequest(
        path,
        bodyBackend);
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
    const auto unrelated =
        SeriesArtworkSettingsSecurityRequest::routeScope(
            "/api/vdr/remote/actions");
    assert(!unrelated.matched);
    assert(unrelated.backendId.empty());

    const auto valid =
        SeriesArtworkSettingsSecurityRequest::routeScope(
            "/api/backends/backend-b/settings/series-artwork?refresh=1");
    assert(valid.matched);
    assert(valid.backendId == "backend-b");

    const auto empty =
        SeriesArtworkSettingsSecurityRequest::routeScope(
            "/api/backends//settings/series-artwork");
    assert(empty.matched);
    assert(empty.backendId.empty());

    const auto encodedSlash =
        SeriesArtworkSettingsSecurityRequest::routeScope(
            "/api/backends/backend%2Fb/settings/series-artwork");
    assert(encodedSlash.matched);
    assert(encodedSlash.backendId.empty());

    const auto nestedSlash =
        SeriesArtworkSettingsSecurityRequest::routeScope(
            "/api/backends/backend/b/settings/series-artwork");
    assert(nestedSlash.matched);
    assert(nestedSlash.backendId.empty());

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "backend-b"));

        HttpServerRequest original = browserRequest(
            fixture,
            "/api/backends/backend-b/settings/series-artwork",
            "backend-a",
            true);
        const HttpServerRequest scoped =
            SeriesArtworkSettingsSecurityRequest::forAuthorization(original);

        assert(original.body.find(
            "\"backendId\":\"backend-a\"") != std::string::npos);
        assert(scoped.body.find(
            "\"backendId\":\"backend-b\"") <
            scoped.body.find(
                "\"backendId\":\"backend-a\""));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(scoped);
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.allowed);
        assert(decision.authorizationDecision.permission == Permission);
        assert(decision.authorizationDecision.backendId == "backend-b");
        assert(decision.authorizationDecision.action == Permission);
        assert(decision.operationId == "phase62-test-operation");

        assert(fixture.gate.appendProtectedMutationOutcome(
            decision,
            200));
        assert(fixture.gate.appendProtectedMutationOutcome(
            decision,
            400));

        assert(hasEvent(
            fixture.accountabilityRepository,
            "authorization.allowed",
            "role_permission_granted",
            "backend-b",
            "dispatch_authorized"));
        assert(hasEvent(
            fixture.accountabilityRepository,
            "operation.succeeded",
            "http_status_200",
            "backend-b",
            "succeeded"));
        assert(hasEvent(
            fixture.accountabilityRepository,
            "operation.failed",
            "http_status_400",
            "backend-b",
            "failed"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            Permission,
            "backend-a"));

        HttpServerRequest request = browserRequest(
            fixture,
            "/api/backends/backend-b/settings/series-artwork",
            "backend-a",
            true);
        const SecurityGateDecision decision = fixture.gate.evaluate(
            SeriesArtworkSettingsSecurityRequest::forAuthorization(request));

        assert(!decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "backend_scope_denied") != std::string::npos);
        assert(hasEvent(
            fixture.accountabilityRepository,
            "authorization.denied",
            "backend_scope_denied",
            "backend-b",
            "dispatch_denied"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.read-only",
            "backend-b"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            Permission,
            "backend-b"));

        HttpServerRequest request = browserRequest(
            fixture,
            "/api/backends/backend-b/settings/series-artwork",
            "backend-b",
            true);
        const SecurityGateDecision decision = fixture.gate.evaluate(
            SeriesArtworkSettingsSecurityRequest::forAuthorization(request));

        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "role_read_only") != std::string::npos);
        assert(hasEvent(
            fixture.accountabilityRepository,
            "authorization.denied",
            "role_read_only",
            "backend-b",
            "dispatch_denied"));
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "backend-b"));

        HttpServerRequest missingCsrf = browserRequest(
            fixture,
            "/api/backends/backend-b/settings/series-artwork",
            "backend-a",
            false);
        const SecurityGateDecision missingDecision = fixture.gate.evaluate(
            SeriesArtworkSettingsSecurityRequest::forAuthorization(
                missingCsrf));
        assert(!missingDecision.allowed);
        assert(missingDecision.rejection.statusCode == 403);
        assert(missingDecision.rejection.body.find(
            "csrf_validation_failed") != std::string::npos);
        assert(hasEvent(
            fixture.accountabilityRepository,
            "authorization.denied",
            "csrf_validation_failed",
            "backend-b",
            "dispatch_denied"));

        HttpServerRequest invalidCsrf = browserRequest(
            fixture,
            "/api/backends/backend-b/settings/series-artwork",
            "backend-a",
            false);
        invalidCsrf.headers["X-CSRF-Token"] = "invalid-test-token";
        const SecurityGateDecision invalidDecision = fixture.gate.evaluate(
            SeriesArtworkSettingsSecurityRequest::forAuthorization(
                invalidCsrf));
        assert(!invalidDecision.allowed);
        assert(invalidDecision.rejection.statusCode == 403);
        assert(invalidDecision.rejection.body.find(
            "csrf_validation_failed") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "backend-b"));

        HttpServerRequest queryRequest = browserRequest(
            fixture,
            "/api/backends/backend-b/settings/series-artwork?source=test",
            "backend-a",
            true);
        const SecurityGateDecision queryDecision = fixture.gate.evaluate(
            SeriesArtworkSettingsSecurityRequest::forAuthorization(
                queryRequest));
        assert(queryDecision.allowed);
        assert(queryDecision.authorizationDecision.backendId == "backend-b");

        HttpServerRequest encodedRequest = browserRequest(
            fixture,
            "/api/backends/backend%2Fb/settings/series-artwork",
            "backend-b",
            true);
        const SecurityGateDecision encodedDecision = fixture.gate.evaluate(
            SeriesArtworkSettingsSecurityRequest::forAuthorization(
                encodedRequest));
        assert(!encodedDecision.allowed);
        assert(encodedDecision.protectedMutation);
        assert(encodedDecision.rejection.statusCode == 400);
        assert(encodedDecision.rejection.body.find(
            "invalid_backend_scope") != std::string::npos);

        HttpServerRequest nestedRequest = browserRequest(
            fixture,
            "/api/backends/backend/b/settings/series-artwork",
            "backend-b",
            true);
        const SecurityGateDecision nestedDecision = fixture.gate.evaluate(
            SeriesArtworkSettingsSecurityRequest::forAuthorization(
                nestedRequest));
        assert(!nestedDecision.allowed);
        assert(nestedDecision.protectedMutation);
        assert(nestedDecision.rejection.statusCode == 400);
        assert(nestedDecision.rejection.body.find(
            "invalid_backend_scope") != std::string::npos);

        HttpServerRequest emptyRequest = browserRequest(
            fixture,
            "/api/backends//settings/series-artwork",
            "backend-b",
            true);
        const SecurityGateDecision emptyDecision = fixture.gate.evaluate(
            SeriesArtworkSettingsSecurityRequest::forAuthorization(
                emptyRequest));
        assert(!emptyDecision.allowed);
        assert(!emptyDecision.protectedMutation);
        assert(emptyDecision.rejection.statusCode == 503);
        assert(emptyDecision.rejection.body.find(
            "security_policy_not_migrated") != std::string::npos);
    }

    return 0;
}
