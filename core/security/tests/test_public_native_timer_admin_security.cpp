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
    // F5 closeout: existing Human SearchTimer grants are sufficient for
    // the same canonical v1 routes, without role.admin. CSRF and independent
    // per-action/backend grants remain mandatory.
    const std::string search = "/api/v1/search-timers";
    const std::string rule = search + "/st_0123456789abcdef0123456789abcdef";
    const std::string scope = "?backend=default";
    {
        SecurityHttpGateBrowserTestFixture fixture;
        auto get = [&](const std::string& path) {
            return fixture.gate.evaluate(browserRequest(fixture, "GET", path));
        };
        auto post = [&](const std::string& path, const std::string& body, bool csrf = true) {
            auto request = browserRequest(fixture, "POST", path, csrf);
            request.body = body;
            return fixture.gate.evaluate(request);
        };
        assertDenied(get(search + scope), 403);
        assert(fixture.grantRepository.ensureGrant(fixture.actorId, "searchtimers.view", "default"));
        assertAllowed(get(search + scope), "searchtimers.view", false);
        assertAllowed(get(rule + scope), "searchtimers.view", false);
        assertAllowed(get(rule + "/version" + scope), "searchtimers.view", false);
        assertDenied(get(search + "?backend=foreign"), 403);
        assertDenied(get(rule + "/preview" + scope), 403);
        assert(fixture.grantRepository.ensureGrant(fixture.actorId, "epg.view", "default"));
        assertAllowed(get(rule + "/preview" + scope), "searchtimers.view", false);
        assertDenied(post(search + scope, "{}"), 403);
        assert(fixture.grantRepository.ensureGrant(fixture.actorId, "searchtimers.create", "default"));
        assertAllowed(post(search + scope, "{}"), "searchtimers.create", true);
        assertDenied(post(search + scope, "{}", false), 403);
        assertDenied(post(search + "?backend=foreign", "{}"), 403);
        const std::string mutation = rule + "/mutations" + scope;
        assertDenied(post(mutation, R"({"action":"update"})"), 403);
        assert(fixture.grantRepository.ensureGrant(fixture.actorId, "searchtimers.modify", "default"));
        assertAllowed(post(mutation, R"({"action":"update"})"), "searchtimers.modify", true);
        assertAllowed(post(mutation, R"({"action":"deactivate"})"), "searchtimers.modify", true);
        assertDenied(post(mutation, R"({"action":"activate"})"), 403);
        assertDenied(post(mutation, R"({"action":"delete"})"), 403);
        assert(fixture.grantRepository.ensureGrant(fixture.actorId, "searchtimers.execute", "default"));
        assert(fixture.grantRepository.ensureGrant(fixture.actorId, "searchtimers.delete", "default"));
        assertAllowed(post(mutation, R"({"action":"activate"})"), "searchtimers.execute", true);
        assertAllowed(post(mutation, R"({"action":"delete"})"), "searchtimers.delete", true);
        assertDenied(post(mutation, R"({"action":"delete"})", false), 403);
    }
    const std::string detail = Timer + "?backendId=default";
    const std::string aux = Timer + "/aux?backendId=default";
    const std::string modify = Timer + "/modifications?backendId=default";
    const std::string directUpdate = Timer + "/direct-update?backendId=default";
    const std::string directDelete = Timer + "/direct-delete?backendId=default";
    const std::string directCreate = "/api/v1/native-timers/actions/create";
    const std::string conflicts = "/api/v1/native-timers/conflicts/live?backendId=default";
    const std::string liveList = "/api/v1/native-timers/live/list?backendId=default";
    {
        SecurityHttpGateBrowserTestFixture fixture;
        assertDenied(fixture.gate.evaluate(browserRequest(fixture, "GET", liveList)), 403);
        assert(fixture.grantRepository.ensureGrant(fixture.actorId, "timers.view", "default"));
        assertAllowed(fixture.gate.evaluate(browserRequest(fixture, "GET", liveList)), "timers.view", false);
        assertDenied(fixture.gate.evaluate(browserRequest(fixture, "GET",
            "/api/v1/native-timers/live/list?backendId=foreign")), 403);
    }

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
        assertAllowed(fixture.gate.evaluate(
            browserRequest(fixture, "GET", conflicts)),
            "timers.view", false);
        assertDenied(fixture.gate.evaluate(browserRequest(fixture, "GET",
            conflicts+"&backendId=foreign")), 400);

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
            browserRequest(fixture, "GET", conflicts)),
            "timers.view", false);
        assertDenied(fixture.gate.evaluate(browserRequest(fixture, "GET",
            conflicts+"&backendId=foreign")), 400);
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
        auto nativeCreate = browserRequest(fixture, "POST", directCreate);
        nativeCreate.body = R"json({"backendId":"default","active":false,"vps":true})json";
        assertAllowed(fixture.gate.evaluate(nativeCreate), "timers.create", true);
        nativeCreate.headers.erase("X-CSRF-Token");
        assertDenied(fixture.gate.evaluate(nativeCreate), 403);
        nativeCreate = browserRequest(fixture, "POST", directCreate);
        nativeCreate.body = R"json({"backendId":"foreign"})json";
        assertDenied(fixture.gate.evaluate(nativeCreate), 403);
        assertDenied(fixture.gate.evaluate(browserRequest(fixture, "GET", conflicts)), 403);
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
