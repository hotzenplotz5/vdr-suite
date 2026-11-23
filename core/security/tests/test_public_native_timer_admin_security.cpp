#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

namespace
{
const std::string Timer = "/api/v1/native-timers/C-1-1051-10301:0:1792447200:2100:2145";

HttpServerRequest browserRequest(
    SecurityHttpGateBrowserTestFixture& fixture,
    const std::string& method,
    const std::string& path,
    bool csrf = true)
{
    HttpServerRequest request;
    request.method = method;
    request.path = path;
    request.headers["X-Request-ID"] = "native-timer-admin-gate";
    if (method == "POST")
    {
        request.body = "{\"kind\":\"toggle\",\"enabled\":false}";
        request.headers["Content-Type"] = "application/json";
    }
    fixture.addBrowserAuthentication(request, csrf);
    return request;
}

void assertAllowed(
    const SecurityGateDecision& decision,
    const std::string& permission,
    bool mutation)
{
    assert(decision.allowed);
    assert(decision.authorizationDecision.allowed);
    assert(decision.authorizationDecision.permission == permission);
    assert(decision.authorizationDecision.backendId == "default");
    assert(decision.protectedMutation == mutation);
}

void assertDenied(const SecurityGateDecision& decision, int status)
{
    assert(!decision.allowed);
    assert(decision.rejection.statusCode == status);
}
}

int main()
{
    const std::string detail = Timer + "?backendId=default";
    const std::string aux = Timer + "/aux?backendId=default";
    const std::string modify = Timer + "/modifications?backendId=default";
    const std::string directUpdate = Timer + "/direct-update?backendId=default";
    const std::string directDelete = Timer + "/direct-delete?backendId=default";

    {
        // First-admin bootstrap really provisions role.admin at "*".
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "*"));

        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "GET", detail)),
            "timers.view", false);
        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "GET", aux)),
            "timers.view", false);

        const auto missingCsrf = fixture.gate.evaluate(
            browserRequest(fixture, "POST", modify, false));
        assertDenied(missingCsrf, 403);
        assert(missingCsrf.authorizationDecision.allowed == false);
        bool csrfAudit = false;
        for (const auto& event : fixture.accountabilityRepository.listAll())
        {
            if (event.reasonCode == "csrf_validation_failed" &&
                event.permission == "timers.modify")
            {
                csrfAudit = true;
            }
        }
        assert(csrfAudit);

        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "POST", modify)),
            "timers.modify", true);
        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "POST", directUpdate)),
            "timers.modify", true);
        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "POST", directDelete)),
            "timers.delete", true);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "POST", directUpdate, false)), 403);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "POST", directDelete, false)), 403);
        const auto deleteWithoutCsrf = fixture.gate.evaluate(
            browserRequest(fixture, "DELETE", detail, false));
        assertDenied(deleteWithoutCsrf, 403);
        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "DELETE", detail)),
            "timers.delete", true);

        // A valid, backend-scoped Administrator also works.
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "DELETE",
                Timer + "?backendId=default&backendId=foreign")), 400);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "default"));
        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "POST", modify)),
            "timers.modify", true);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "DELETE",
                Timer + "?backendId=foreign")), 403);
    }

    {
        // Existing backend-scoped Human timer grants retain legacy
        // editor parity on Public v1 without requiring role.admin.
        SecurityHttpGateBrowserTestFixture fixture;
        for (const char* permission :
            {"timers.view", "timers.modify", "timers.delete"})
        {
            assert(fixture.grantRepository.ensureGrant(
                fixture.actorId, permission, "default"));
        }
        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "GET", detail)),
            "timers.view", false);
        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "GET", aux)),
            "timers.view", false);
        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "POST", modify)),
            "timers.modify", true);
        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "DELETE", detail)),
            "timers.delete", true);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "POST", modify, false)), 403);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "DELETE", detail, false)), 403);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "POST",
                Timer + "/modifications?backendId=foreign")), 403);
    }

    {
        // A Human with timers.create may use public manual CREATE;
        // browser CSRF and backend scope still apply.
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "timers.create", "default"));
        auto manual = browserRequest(
            fixture, "POST", "/api/v1/manual/timer-intents");
        manual.body = R"json({"backendId":"default"})json";
        assertAllowed(fixture.gate.evaluate(manual), "timers.create", true);
        manual.headers.erase("X-CSRF-Token");
        assertDenied(fixture.gate.evaluate(manual), 403);
        manual = browserRequest(
            fixture, "POST", "/api/v1/manual/timer-intents");
        manual.body = R"json({"backendId":"foreign"})json";
        assertDenied(fixture.gate.evaluate(manual), 403);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "POST", modify)), 403);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "POST", directUpdate)), 403);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "POST", directDelete)), 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "GET", detail)), 403);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "GET", aux)), 403);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "POST", modify)), 403);
        assertDenied(fixture.gate.evaluate(
            browserRequest(fixture, "DELETE", detail)), 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest anonymous;
        anonymous.method = "POST";
        anonymous.path = modify;
        assertDenied(fixture.gate.evaluate(anonymous), 401);
    }
    return 0;
}
