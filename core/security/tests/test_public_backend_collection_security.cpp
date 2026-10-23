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
        // Readback is restricted to a global administrator Browser Session.
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest get;
        get.method = "GET";
        get.path = "/api/v1/addons/rectools/activation?backendId=backend-a";
        fixture.addBrowserAuthentication(get, false);
        assert(fixture.gate.evaluate(get).rejection.statusCode == 403);
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "*"));
        const auto allowed = fixture.gate.evaluate(get);
        assert(allowed.allowed && !allowed.protectedMutation);
        assert(allowed.authorizationDecision.action == "addons.intent.read");
        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path = get.path;
        assert(fixture.gate.evaluate(anonymous).rejection.statusCode == 401);
    }
    {
        // Admin add-on activation MUST require a Browser Session, CSRF and
        // explicit global role.admin. No caller-provided trusted evidence.
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest post;
        post.method = "POST";
        post.path = "/api/v1/addons/rectools/activation";
        post.headers["Content-Type"] = "application/json";
        post.body = R"({"backendId":"backend-a","desiredEnabled":true,"expectedRevision":0})";
        fixture.addBrowserAuthentication(post, true);
        const auto denied = fixture.gate.evaluate(post);
        assert(!denied.allowed && denied.rejection.statusCode == 403);
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "*"));
        const auto allowed = fixture.gate.evaluate(post);
        assert(allowed.allowed && allowed.protectedMutation);
        assert(allowed.browserAuthenticated);
        assert(allowed.authorizationDecision.permission == "role.admin");
        assert(allowed.authorizationDecision.action == "addons.intent.update");
        assert(allowed.authorizationDecision.backendId == "*");
        post.headers.erase("X-CSRF-Token");
        const auto csrf = fixture.gate.evaluate(post);
        assert(!csrf.allowed && csrf.rejection.statusCode == 403);
        HttpServerRequest anonymous;
        anonymous.method = "POST";
        anonymous.path = post.path;
        anonymous.body = post.body;
        const auto unauthenticated = fixture.gate.evaluate(anonymous);
        assert(!unauthenticated.allowed &&
               unauthenticated.rejection.statusCode == 401);
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
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "addons.media.import", "backend-a"));
        HttpServerRequest request = browserGet(fixture);
        request.path = "/api/v1/backends/backend-a/media-import-sources";
        const auto decision = fixture.gate.evaluate(request);
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a"}));
        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path = request.path;
        assert(fixture.gate.evaluate(anonymous).rejection.statusCode == 401);
    }

    {
        // M2.2 GET capability and result belong to the existing backend
        // discovery scope. The read-only media check needs no new grant type.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "media.recording.play", "backend-a"));
        HttpServerRequest request = browserGet(fixture);
        request.path = "/api/v1/backends/backend-a/capabilities/media-check";
        const auto capability = fixture.gate.evaluate(request);
        assert(capability.allowed && !capability.protectedMutation);
        assert(capability.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a"}));

        request.path = "/api/v1/backends/backend-a/media-checks/"
            "chk_0123456789abcdef0123456789abcdef";
        const auto status = fixture.gate.evaluate(request);
        assert(status.allowed && !status.protectedMutation);
        assert(status.authorizationDecision.permission ==
            "media.recording.play");
        assert(status.authorizationDecision.action ==
            "media.recording.check.read");
        assert(status.authorizationDecision.backendId == "backend-a");
        assert(status.authorizedBackendIds ==
            (std::vector<std::string>{"backend-a"}));

        HttpServerRequest anonymous;
        anonymous.method = "GET";
        anonymous.path = request.path;
        assert(fixture.gate.evaluate(anonymous).rejection.statusCode == 401);
    }

    {
        // Generic discovery rights or playback rights for another backend
        // must not expose a saved Media Check report.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "channels.view", "backend-a"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "media.recording.play", "backend-b"));
        HttpServerRequest request = browserGet(fixture);
        request.path = "/api/v1/backends/backend-a/media-checks/"
            "chk_0123456789abcdef0123456789abcdef";
        const auto denied = fixture.gate.evaluate(request);
        assert(!denied.allowed && denied.rejection.statusCode == 403);

        request.path = "/api/v1/backends/backend-b/media-checks/"
            "chk_0123456789abcdef0123456789abcdef";
        const auto allowed = fixture.gate.evaluate(request);
        assert(allowed.allowed && !allowed.protectedMutation);
        assert(allowed.authorizationDecision.backendId == "backend-b");
    }

    {
        // The real HTTP gate must authorize the read-only check with the
        // same backend recording permission and CSRF requirement as playback.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "media.recording.play", "backend-a"));
        HttpServerRequest request;
        request.method = "POST";
        request.path = "/api/v1/backends/backend-a/media-checks";
        request.body = "{\"recordingId\":"
            "\"rec_0123456789abcdef0123456789abcdef\","
            "\"mode\":\"structure\"}";
        request.headers["Content-Type"] = "application/json";
        fixture.addBrowserAuthentication(request, true);
        const auto decision = fixture.gate.evaluate(request);
        assert(decision.allowed && decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "media.recording.play");
        assert(decision.authorizationDecision.action ==
            "media.recording.check");
        assert(decision.authorizationDecision.backendId == "backend-a");

        fixture.addBrowserAuthentication(request, false);
        request.headers.erase("X-CSRF-Token");
        const auto csrf = fixture.gate.evaluate(request);
        assert(!csrf.allowed && csrf.rejection.statusCode == 403);

        HttpServerRequest anonymous;
        anonymous.method = "POST";
        anonymous.path = request.path;
        anonymous.body = request.body;
        const auto unauthenticated = fixture.gate.evaluate(anonymous);
        assert(!unauthenticated.allowed);
        assert(unauthenticated.rejection.statusCode == 401);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest request;
        request.method = "POST";
        request.path = "/api/v1/backends/backend-a/media-checks";
        request.body = "{\"recordingId\":"
            "\"rec_0123456789abcdef0123456789abcdef\","
            "\"mode\":\"structure\"}";
        request.headers["Content-Type"] = "application/json";
        fixture.addBrowserAuthentication(request, true);
        const auto missingGrant = fixture.gate.evaluate(request);
        assert(!missingGrant.allowed && missingGrant.rejection.statusCode == 403);
    }

    {
        // HEVC planning is read-only but still authenticated, backend-scoped,
        // CSRF protected and audited. It never grants stage/publish rights.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "media.recording.play", "backend-a"));
        HttpServerRequest request = browserGet(fixture);
        request.path = "/api/v1/backends/backend-a/capabilities/media-shrink";
        const auto capability = fixture.gate.evaluate(request);
        assert(capability.allowed && !capability.protectedMutation);
        request.path = "/api/v1/backends/backend-a/media-shrink-plans";
        request.method = "POST";
        request.body = "{\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\"}";
        request.headers["Content-Type"] = "application/json";
        fixture.addBrowserAuthentication(request, true);
        const auto plan = fixture.gate.evaluate(request);
        assert(plan.allowed && plan.protectedMutation);
        assert(plan.authorizationDecision.permission == "media.recording.play");
        assert(plan.authorizationDecision.action ==
            "media.recording.shrink.plan");
        assert(plan.authorizationDecision.backendId == "backend-a");
        fixture.addBrowserAuthentication(request, false);
        request.headers.erase("X-CSRF-Token");
        assert(fixture.gate.evaluate(request).rejection.statusCode == 403);
        HttpServerRequest anonymous;
        anonymous.method = "POST";
        anonymous.path = request.path;
        anonymous.body = request.body;
        assert(fixture.gate.evaluate(anonymous).rejection.statusCode == 401);
        HttpServerRequest read = browserGet(fixture);
        read.path = "/api/v1/backends/backend-a/media-shrink-reviews/"
            "shk_0123456789abcdef0123456789abcdef";
        const auto review = fixture.gate.evaluate(read);
        assert(review.allowed && !review.protectedMutation);
        assert(review.authorizationDecision.permission == "media.recording.play");
        assert(review.authorizationDecision.action ==
               "media.recording.shrink.review.read");
        assert(review.authorizationDecision.backendId == "backend-a");
        read.path = "/api/v1/backends/backend-b/media-shrink-reviews/"
            "shk_0123456789abcdef0123456789abcdef";
        assert(fixture.gate.evaluate(read).rejection.statusCode == 403);
        HttpServerRequest anonymousRead;
        anonymousRead.method = "GET";
        anonymousRead.path =
            "/api/v1/backends/backend-a/media-shrink-reviews/"
            "shk_0123456789abcdef0123456789abcdef";
        assert(fixture.gate.evaluate(anonymousRead).rejection.statusCode == 401);
    }
    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest request;
        request.method = "POST";
        request.path = "/api/v1/backends/backend-a/media-shrink-plans";
        request.body = "{\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\"}";
        request.headers["Content-Type"] = "application/json";
        fixture.addBrowserAuthentication(request, true);
        assert(fixture.gate.evaluate(request).rejection.statusCode == 403);
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
