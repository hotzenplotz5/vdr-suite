#include "SecurityHttpGateBrowserTestFixture.h"

#include "AccountabilityEventRepository.h"
#include "BrowserSessionAuthenticator.h"
#include "Database.h"
#include "SecurityHttpGate.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <string>

namespace
{
HttpServerRequest getRequest()
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = "/api/backends";
    return request;
}

SecurityConfiguration enforcedConfiguration()
{
    SecurityConfiguration configuration;
    configuration.mode = SecurityMode::Enforced;
    configuration.expectedAuthorizationHeader =
        SecurityHttpGateBrowserTestFixture::legacyCredential;
    configuration.grants = {
        PermissionGrant{"remote.control", "default"}
    };
    return configuration;
}

void setLiveChannelBody(HttpServerRequest& request, const std::string& backendId)
{
    request.body =
        "{\"backendId\":\"" + backendId +
        "\",\"resourceKind\":\"live-channel\""
        ",\"operationId\":\"phase65-live-security-test\"}";
}
}

int main()
{
    SecurityHttpGateBrowserTestFixture fixture;

    const SecurityGateDecision anonymous =
        fixture.gate.evaluate(getRequest());
    assert(!anonymous.allowed);
    assert(anonymous.rejection.statusCode == 401);
    assert(anonymous.rejection.body.find(
        "authentication_required") !=
        std::string::npos);
    assert(anonymous.rejection.headers.find(
        "WWW-Authenticate") ==
        anonymous.rejection.headers.end());

    HttpServerRequest legacyGet = getRequest();
    fixture.addLegacyAuthentication(legacyGet);

    const SecurityGateDecision legacyAllowed =
        fixture.gate.evaluate(legacyGet);
    assert(legacyAllowed.allowed);
    assert(legacyAllowed.context.actor.actorId ==
        "legacy-local-web");

    HttpServerRequest browserPreferred = getRequest();
    fixture.addBrowserAuthentication(browserPreferred);
    browserPreferred.headers["Authorization"] =
        "Basic invalid-fallback";

    const SecurityGateDecision browserAllowed =
        fixture.gate.evaluate(browserPreferred);
    assert(browserAllowed.allowed);
    assert(browserAllowed.browserSessionPresented);
    assert(browserAllowed.browserAuthenticated);
    assert(browserAllowed.context.actor.actorId ==
        fixture.actorId);

    HttpServerRequest invalidBrowser = getRequest();
    invalidBrowser.headers["Cookie"] =
        "vdr_suite_session=" + fixture.tokenId +
        ".invalid-session-secret";
    fixture.addLegacyAuthentication(invalidBrowser);

    const SecurityGateDecision invalidBrowserDecision =
        fixture.gate.evaluate(invalidBrowser);
    assert(!invalidBrowserDecision.allowed);
    assert(invalidBrowserDecision.browserSessionPresented);
    assert(invalidBrowserDecision.rejection.statusCode == 401);
    assert(invalidBrowserDecision.rejection.headers.find(
        "WWW-Authenticate") ==
        invalidBrowserDecision.rejection.headers.end());

    Database closedGrantDatabase;
    SecurityPermissionGrantRepository unavailableGrants(
        closedGrantDatabase);
    BrowserSessionAuthenticator unavailableBrowserAuthenticator(
        fixture.browserRepository,
        unavailableGrants);
    SecurityHttpGate unavailableGrantGate(
        SecurityHttpGateBrowserTestFixture::configuration(),
        fixture.accountabilityRepository,
        &fixture.identityResolver,
        nullptr,
        &unavailableBrowserAuthenticator);

    HttpServerRequest unavailableGrantRequest = getRequest();
    fixture.addBrowserAuthentication(
        unavailableGrantRequest);

    const SecurityGateDecision unavailableGrantDecision =
        unavailableGrantGate.evaluate(
            unavailableGrantRequest);
    assert(!unavailableGrantDecision.allowed);
    assert(
        unavailableGrantDecision.rejection.statusCode ==
        503);
    assert(unavailableGrantDecision.rejection.body.find(
        "permission_grants_unavailable") !=
        std::string::npos);

    HttpServerRequest missingCsrf =
        fixture.mutationRequest(
            "/api/vdr/remote/actions",
            "default");
    fixture.addBrowserAuthentication(missingCsrf);

    const SecurityGateDecision missingCsrfDecision =
        fixture.gate.evaluate(missingCsrf);
    assert(!missingCsrfDecision.allowed);
    assert(missingCsrfDecision.protectedMutation);
    assert(missingCsrfDecision.rejection.statusCode == 403);
    assert(missingCsrfDecision.rejection.body.find(
        "csrf_validation_failed") !=
        std::string::npos);

    HttpServerRequest playbackMissingCsrf =
        fixture.mutationRequest(
            "/api/media/sessions",
            "default");
    fixture.addBrowserAuthentication(playbackMissingCsrf);

    const SecurityGateDecision playbackMissingCsrfDecision =
        fixture.gate.evaluate(playbackMissingCsrf);
    assert(!playbackMissingCsrfDecision.allowed);
    assert(!playbackMissingCsrfDecision.protectedMutation);
    assert(playbackMissingCsrfDecision.rejection.statusCode == 403);
    assert(playbackMissingCsrfDecision.rejection.body.find(
        "csrf_validation_failed") !=
        std::string::npos);

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "media.recording.play",
        "default"));

    HttpServerRequest playback =
        fixture.mutationRequest(
            "/api/media/sessions",
            "default");
    fixture.addBrowserAuthentication(playback, true);

    const SecurityGateDecision playbackAllowed =
        fixture.gate.evaluate(playback);
    assert(playbackAllowed.allowed);
    assert(!playbackAllowed.protectedMutation);
    assert(playbackAllowed.authorizationDecision.allowed);
    assert(playbackAllowed.authorizationDecision.permission ==
        "media.recording.play");
    assert(playbackAllowed.authorizationDecision.backendId ==
        "default");
    assert(playbackAllowed.authorizationDecision.action ==
        "media.recording.play");
    assert(!fixture.gate.appendProtectedMutationOutcome(
        playbackAllowed,
        201));

    HttpServerRequest livePlaybackWithoutGrant =
        fixture.mutationRequest(
            "/api/media/sessions",
            "default");
    setLiveChannelBody(livePlaybackWithoutGrant, "default");
    fixture.addBrowserAuthentication(livePlaybackWithoutGrant, true);

    const SecurityGateDecision livePlaybackWithoutGrantDecision =
        fixture.gate.evaluate(livePlaybackWithoutGrant);
    assert(!livePlaybackWithoutGrantDecision.allowed);
    assert(!livePlaybackWithoutGrantDecision.protectedMutation);
    assert(livePlaybackWithoutGrantDecision.rejection.statusCode == 403);
    assert(livePlaybackWithoutGrantDecision.rejection.body.find(
        "permission_denied") != std::string::npos);

    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        "media.live.play",
        "default"));

    HttpServerRequest livePlayback =
        fixture.mutationRequest(
            "/api/media/sessions",
            "default");
    setLiveChannelBody(livePlayback, "default");
    fixture.addBrowserAuthentication(livePlayback, true);

    const SecurityGateDecision livePlaybackAllowed =
        fixture.gate.evaluate(livePlayback);
    assert(livePlaybackAllowed.allowed);
    assert(!livePlaybackAllowed.protectedMutation);
    assert(livePlaybackAllowed.authorizationDecision.allowed);
    assert(livePlaybackAllowed.authorizationDecision.permission ==
        "media.live.play");
    assert(livePlaybackAllowed.authorizationDecision.backendId ==
        "default");
    assert(livePlaybackAllowed.authorizationDecision.action ==
        "media.live.play");

    HttpServerRequest playbackWrongScope =
        fixture.mutationRequest(
            "/api/media/sessions",
            "house-b");
    fixture.addBrowserAuthentication(playbackWrongScope, true);

    const SecurityGateDecision playbackWrongScopeDecision =
        fixture.gate.evaluate(playbackWrongScope);
    assert(!playbackWrongScopeDecision.allowed);
    assert(!playbackWrongScopeDecision.protectedMutation);
    assert(playbackWrongScopeDecision.rejection.statusCode == 403);
    assert(playbackWrongScopeDecision.rejection.body.find(
        "backend_scope_denied") !=
        std::string::npos);

    HttpServerRequest playbackMissingBackend =
        fixture.mutationRequest(
            "/api/media/sessions",
            "",
            false);
    fixture.addBrowserAuthentication(playbackMissingBackend, true);

    const SecurityGateDecision playbackMissingBackendDecision =
        fixture.gate.evaluate(playbackMissingBackend);
    assert(!playbackMissingBackendDecision.allowed);
    assert(!playbackMissingBackendDecision.protectedMutation);
    assert(playbackMissingBackendDecision.rejection.statusCode == 400);
    assert(playbackMissingBackendDecision.rejection.body.find(
        "invalid_backend_scope") !=
        std::string::npos);

    HttpServerRequest missingPermission =
        fixture.mutationRequest(
            "/api/vdr/remote/actions",
            "default");
    fixture.addBrowserAuthentication(
        missingPermission,
        true);

    const SecurityGateDecision missingPermissionDecision =
        fixture.gate.evaluate(missingPermission);
    assert(!missingPermissionDecision.allowed);
    assert(
        missingPermissionDecision.rejection.statusCode ==
        403);
    assert(missingPermissionDecision.rejection.body.find(
        "permission_denied") !=
        std::string::npos);

    HttpServerRequest unmigrated =
        fixture.mutationRequest(
            "/api/phase62/unmapped-mutation",
            "default");
    fixture.addBrowserAuthentication(
        unmigrated,
        true);

    const SecurityGateDecision unmigratedBrowser =
        fixture.gate.evaluate(unmigrated);
    assert(!unmigratedBrowser.allowed);
    assert(unmigratedBrowser.rejection.statusCode == 503);
    assert(unmigratedBrowser.rejection.body.find(
        "security_policy_not_migrated") !=
        std::string::npos);

    HttpServerRequest legacyUnmigrated =
        fixture.mutationRequest(
            "/api/phase62/unmapped-mutation",
            "default");
    fixture.addLegacyAuthentication(legacyUnmigrated);
    assert(fixture.gate.evaluate(
        legacyUnmigrated).allowed);

    HttpServerRequest remote =
        fixture.mutationRequest(
            "/api/vdr/remote/actions",
            "default");
    fixture.addLegacyAuthentication(remote);

    SecurityHttpGate enforcedGate(
        enforcedConfiguration(),
        fixture.accountabilityRepository,
        &fixture.identityResolver);

    const SecurityGateDecision remoteAllowed =
        enforcedGate.evaluate(remote);
    assert(remoteAllowed.allowed);
    assert(remoteAllowed.protectedMutation);
    assert(remoteAllowed.context.requestId ==
        "phase62-test-request");
    assert(remoteAllowed.context.correlationId ==
        "phase62-test-correlation");
    assert(remoteAllowed.authorizationDecision.allowed);
    assert(remoteAllowed.authorizationDecision.permission ==
        "remote.control");
    assert(remoteAllowed.authorizationDecision.backendId ==
        "default");
    assert(remoteAllowed.authorizationDecision.action ==
        "remote.control");
    assert(remoteAllowed.operationId ==
        "phase62-test-operation");

    assert(!enforcedGate.appendProtectedMutationOutcome(
        legacyAllowed,
        200));
    assert(enforcedGate.appendProtectedMutationOutcome(
        remoteAllowed,
        204));
    assert(enforcedGate.appendProtectedMutationOutcome(
        remoteAllowed,
        503));

    HttpServerRequest wrongScope =
        fixture.mutationRequest(
            "/api/vdr/remote/actions",
            "house-b");
    fixture.addLegacyAuthentication(wrongScope);

    const SecurityGateDecision wrongScopeDecision =
        enforcedGate.evaluate(wrongScope);
    assert(!wrongScopeDecision.allowed);
    assert(wrongScopeDecision.rejection.statusCode == 403);
    assert(wrongScopeDecision.rejection.body.find(
        "backend_scope_denied") !=
        std::string::npos);

    HttpServerRequest missingBackend =
        fixture.mutationRequest(
            "/api/vdr/remote/actions",
            "");
    fixture.addLegacyAuthentication(missingBackend);

    const SecurityGateDecision missingBackendDecision =
        enforcedGate.evaluate(missingBackend);
    assert(!missingBackendDecision.allowed);
    assert(
        missingBackendDecision.rejection.statusCode ==
        400);
    assert(missingBackendDecision.rejection.body.find(
        "invalid_backend_scope") !=
        std::string::npos);

    bool sawRemoteAllowed = false;
    bool sawUnmigratedDenied = false;
    bool sawPlaybackAllowed = false;
    bool sawLivePlaybackDenied = false;
    bool sawLivePlaybackAllowed = false;
    bool sawRemoteSucceeded = false;
    bool sawRemoteFailed = false;

    for (const AccountabilityEvent& event :
         fixture.accountabilityRepository.listAll())
    {
        sawRemoteAllowed = sawRemoteAllowed ||
            (event.permission == "remote.control" &&
             event.outcome == "dispatch_authorized");
        sawUnmigratedDenied = sawUnmigratedDenied ||
            (event.reasonCode ==
                 "security_policy_not_migrated" &&
             event.outcome == "dispatch_denied");
        sawPlaybackAllowed = sawPlaybackAllowed ||
            (event.permission == "media.recording.play" &&
             event.backendId == "default" &&
             event.action == "media.recording.play" &&
             event.outcome == "dispatch_authorized");
        sawLivePlaybackDenied = sawLivePlaybackDenied ||
            (event.permission == "media.live.play" &&
             event.backendId == "default" &&
             event.action == "media.live.play" &&
             event.outcome == "dispatch_denied" &&
             event.reasonCode == "permission_denied");
        sawLivePlaybackAllowed = sawLivePlaybackAllowed ||
            (event.permission == "media.live.play" &&
             event.backendId == "default" &&
             event.action == "media.live.play" &&
             event.outcome == "dispatch_authorized");
        sawRemoteSucceeded = sawRemoteSucceeded ||
            (event.eventType == "operation.succeeded" &&
             event.permission == "remote.control" &&
             event.backendId == "default" &&
             event.action == "remote.control" &&
             event.operationId == "phase62-test-operation" &&
             event.requestId == "phase62-test-request" &&
             event.correlationId == "phase62-test-correlation" &&
             event.reasonCode == "http_status_204" &&
             event.outcome == "succeeded");
        sawRemoteFailed = sawRemoteFailed ||
            (event.eventType == "operation.failed" &&
             event.permission == "remote.control" &&
             event.backendId == "default" &&
             event.action == "remote.control" &&
             event.operationId == "phase62-test-operation" &&
             event.requestId == "phase62-test-request" &&
             event.correlationId == "phase62-test-correlation" &&
             event.reasonCode == "http_status_503" &&
             event.outcome == "failed");

        assert(event.permission.find("Basic ") ==
            std::string::npos);
        assert(event.reasonCode.find(
            SecurityHttpGateBrowserTestFixture::
                sessionSecret) ==
            std::string::npos);
        assert(event.reasonCode.find(
            SecurityHttpGateBrowserTestFixture::
                csrfSecret) ==
            std::string::npos);
    }

    assert(sawRemoteAllowed);
    assert(sawUnmigratedDenied);
    assert(sawPlaybackAllowed);
    assert(sawLivePlaybackDenied);
    assert(sawLivePlaybackAllowed);
    assert(sawRemoteSucceeded);
    assert(sawRemoteFailed);

    Database closedAuditDatabase;
    AccountabilityEventRepository unavailableAudit(
        closedAuditDatabase);
    SecurityHttpGate unavailableAuditGate(
        enforcedConfiguration(),
        unavailableAudit,
        &fixture.identityResolver);

    const SecurityGateDecision auditFailure =
        unavailableAuditGate.evaluate(remote);
    assert(!auditFailure.allowed);
    assert(auditFailure.rejection.statusCode == 503);
    assert(auditFailure.rejection.body.find(
        "accountability_unavailable") !=
        std::string::npos);

    assert(!unavailableAuditGate.appendProtectedMutationOutcome(
        remoteAllowed,
        200));
    const HttpServerResponse outcomeAuditFailure =
        unavailableAuditGate.outcomeAccountabilityUnavailableResponse(
            remoteAllowed.context);
    assert(outcomeAuditFailure.statusCode == 503);
    assert(outcomeAuditFailure.body.find(
        "accountability_unavailable") !=
        std::string::npos);
    assert(outcomeAuditFailure.headers.at("X-Request-ID") ==
        "phase62-test-request");
    assert(outcomeAuditFailure.headers.at("X-Correlation-ID") ==
        "phase62-test-correlation");

    assert(fixture.browserRepository.revokeBySessionId(
        fixture.sessionId));

    const SecurityGateDecision replayDenied =
        fixture.gate.evaluate(browserPreferred);
    assert(!replayDenied.allowed);
    assert(replayDenied.rejection.statusCode == 401);

    return 0;
}