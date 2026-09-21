#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{

HttpServerRequest request(
    const SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& route,
    const std::string& backendId,
    bool csrf = true)
{
    HttpServerRequest value;
    value.method = "POST";
    value.path = route;
    value.body =
        "{\"backendId\":\"" + backendId +
        "\",\"operationId\":\"phase67-hbbtv-session\"}";
    value.headers["X-Request-ID"] = "phase67-hbbtv-session";
    value.headers["X-Correlation-ID"] = "phase67-hbbtv-session-corr";
    fixture.addBrowserAuthentication(value, csrf);
    return value;
}

HttpServerRequest sessionReadRequest(
    const SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& route,
    const std::string& backendId)
{
    HttpServerRequest value;
    value.method = "GET";
    value.path =
        route + "?backend=" + backendId +
        "&session=bas_test";
    if (route.find("/presentation") != std::string::npos)
        value.path += "&revision=0";
    value.headers["X-Request-ID"] =
        "phase67-hbbtv-session-read";
    value.headers["X-Correlation-ID"] =
        "phase67-hbbtv-session-read-corr";
    fixture.addBrowserAuthentication(value, false);
    return value;
}

HttpServerRequest presentationRequest(
    const SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& backendId)
{
    HttpServerRequest value;
    value.method = "GET";
    value.path =
        "/api/vdr/broadcast/hbbtv/sessions/presentation"
        "?backend=" + backendId +
        "&session=bas_test&revision=0";
    value.headers["X-Request-ID"] = "phase67-hbbtv-presentation";
    value.headers["X-Correlation-ID"] =
        "phase67-hbbtv-presentation-corr";
    fixture.addBrowserAuthentication(value, false);
    return value;
}

void expectAllowed(
    const std::string& route,
    const std::string& permission)
{
    SecurityHttpGateBrowserTestFixture fixture;
    assert(fixture.grantRepository.ensureGrant(
        fixture.actorId,
        permission,
        "backend-b"));

    const SecurityGateDecision decision =
        fixture.gate.evaluate(request(
            fixture,
            route,
            "backend-b"));

    assert(decision.allowed);
    assert(decision.protectedMutation);
    assert(decision.authorizationDecision.allowed);
    assert(decision.authorizationDecision.permission == permission);
    assert(decision.authorizationDecision.backendId == "backend-b");
}

}

int main()
{
    expectAllowed(
        "/api/vdr/broadcast/hbbtv/sessions",
        "broadcast.hbbtv.launch");
    expectAllowed(
        "/api/vdr/broadcast/hbbtv/sessions/input",
        "broadcast.hbbtv.input");
    expectAllowed(
        "/api/vdr/broadcast/hbbtv/sessions/status",
        "broadcast.session.manage_own");
    expectAllowed(
        "/api/vdr/broadcast/hbbtv/sessions/close",
        "broadcast.session.manage_own");
    expectAllowed(
        "/api/vdr/broadcast/hbbtv/sessions/media",
        "broadcast.session.manage_own");

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "broadcast.session.manage_own",
            "backend-b"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(sessionReadRequest(
                fixture,
                "/api/vdr/broadcast/hbbtv/sessions/media",
                "backend-b"));

        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "broadcast.session.manage_own");
        assert(decision.authorizationDecision.backendId ==
            "backend-b");
        assert(decision.authorizationDecision.action ==
            "broadcast.hbbtv.media");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "broadcast.session.manage_own",
            "backend-b"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                presentationRequest(fixture, "backend-b"));

        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "broadcast.session.manage_own");
        assert(decision.authorizationDecision.backendId == "backend-b");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "broadcast.session.manage_own",
            "backend-a"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(
                presentationRequest(fixture, "backend-b"));

        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "broadcast.hbbtv.launch",
            "backend-a"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(request(
                fixture,
                "/api/vdr/broadcast/hbbtv/sessions",
                "backend-b"));

        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "backend_scope_denied") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "broadcast.hbbtv.launch",
            "backend-b"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(request(
                fixture,
                "/api/vdr/broadcast/hbbtv/sessions",
                "backend-b",
                false));

        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.rejection.body.find(
            "csrf_validation_failed") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.read-only",
            "backend-b"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "broadcast.hbbtv.input",
            "backend-b"));

        const SecurityGateDecision decision =
            fixture.gate.evaluate(request(
                fixture,
                "/api/vdr/broadcast/hbbtv/sessions/input",
                "backend-b"));

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

        const SecurityGateDecision decision =
            fixture.gate.evaluate(request(
                fixture,
                "/api/vdr/broadcast/hbbtv/sessions/close",
                "backend-b"));

        assert(decision.allowed);
        assert(decision.authorizationDecision.reasonCode ==
            "role_permission_granted");
    }

    return 0;
}
