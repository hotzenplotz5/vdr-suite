#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

int main()
{
    const std::string device =
        "/api/v1/devices/device_mu10e2/lifecycle";
    const std::string credential =
        "/api/v1/devices/device_mu10e2/credentials/"
        "credential_mu10e2/lifecycle";
    for (const std::string& path : {device, credential})
    {
        {
            SecurityHttpGateBrowserTestFixture f;
            HttpServerRequest req;
            req.method = "GET";
            req.path = path;
            const auto denied = f.gate.evaluate(req);
            assert(!denied.allowed);
            assert(denied.rejection.statusCode == 401);
        }
        {
            SecurityHttpGateBrowserTestFixture f;
            HttpServerRequest req;
            req.method = "GET";
            req.path = path;
            f.addBrowserAuthentication(req);
            const auto denied = f.gate.evaluate(req);
            assert(!denied.allowed);
            assert(denied.rejection.statusCode == 403);
        }
        {
            SecurityHttpGateBrowserTestFixture f;
            assert(f.grantRepository.ensureGrant(
                f.actorId, "role.admin", "*"));
            HttpServerRequest req;
            req.method = "GET";
            req.path = path;
            f.addBrowserAuthentication(req);
            const auto accepted = f.gate.evaluate(req);
            assert(accepted.allowed);
            assert(accepted.authorizationDecision.permission ==
                "devices.lifecycle.view");
        }
        {
            SecurityHttpGateBrowserTestFixture f;
            assert(f.grantRepository.ensureGrant(
                f.actorId, "role.admin", "*"));
            HttpServerRequest req;
            req.method = "POST";
            req.path = path;
            req.body = "{}";
            f.addBrowserAuthentication(req, false);
            const auto denied = f.gate.evaluate(req);
            assert(!denied.allowed);
            assert(denied.rejection.statusCode == 403);
            assert(denied.protectedMutation);
        }
        {
            SecurityHttpGateBrowserTestFixture f;
            assert(f.grantRepository.ensureGrant(
                f.actorId, "role.admin", "*"));
            HttpServerRequest req;
            req.method = "POST";
            req.path = path;
            req.body = "{}";
            f.addBrowserAuthentication(req, true);
            const auto accepted = f.gate.evaluate(req);
            assert(accepted.allowed);
            assert(accepted.protectedMutation);
            assert(accepted.authorizationDecision.permission ==
                (path == device ? "devices.revoke"
                                : "devices.credentials.revoke"));
        }
        {
            SecurityHttpGateBrowserTestFixture f;
            assert(f.grantRepository.ensureGrant(
                f.actorId, "role.admin", "*"));
            assert(f.grantRepository.ensureGrant(
                f.actorId, "role.read-only", "*"));
            HttpServerRequest req;
            req.method = "POST";
            req.path = path;
            req.body = "{}";
            f.addBrowserAuthentication(req, true);
            const auto denied = f.gate.evaluate(req);
            assert(!denied.allowed);
            assert(denied.rejection.statusCode == 403);
        }
    }
    return 0;
}
