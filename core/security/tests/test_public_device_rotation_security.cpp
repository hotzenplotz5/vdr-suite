#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

int main() {
    const std::string path =
        "/api/v1/devices/device_mu10e3/credentials/credential_mu10e3/rotate";
    {
        SecurityHttpGateBrowserTestFixture f;
        HttpServerRequest req;
        req.method="POST";
        req.path=path;
        req.body="{}";
        const auto result=f.gate.evaluate(req);
        assert(!result.allowed);
        assert(result.rejection.statusCode==401);
    }
    {
        SecurityHttpGateBrowserTestFixture f;
        HttpServerRequest req;
        req.method="POST"; req.path=path; req.body="{}";
        f.addBrowserAuthentication(req,true);
        const auto result=f.gate.evaluate(req);
        assert(!result.allowed);
        assert(result.rejection.statusCode==403);
    }
    {
        SecurityHttpGateBrowserTestFixture f;
        assert(f.grantRepository.ensureGrant(f.actorId,"role.admin","*"));
        HttpServerRequest req;
        req.method="POST"; req.path=path; req.body="{}";
        f.addBrowserAuthentication(req,false);
        const auto result=f.gate.evaluate(req);
        assert(!result.allowed);
        assert(result.rejection.statusCode==403);
        assert(result.protectedMutation);
    }
    {
        SecurityHttpGateBrowserTestFixture f;
        assert(f.grantRepository.ensureGrant(f.actorId,"role.admin","*"));
        HttpServerRequest req;
        req.method="POST"; req.path=path; req.body="{}";
        f.addBrowserAuthentication(req,true);
        const auto result=f.gate.evaluate(req);
        assert(result.allowed);
        assert(result.protectedMutation);
        assert(result.authorizationDecision.permission==
            "devices.credentials.rotate");
    }
    {
        SecurityHttpGateBrowserTestFixture f;
        assert(f.grantRepository.ensureGrant(f.actorId,"role.admin","*"));
        assert(f.grantRepository.ensureGrant(f.actorId,"role.read-only","*"));
        HttpServerRequest req;
        req.method="POST";req.path=path;req.body="{}";
        f.addBrowserAuthentication(req,true);
        const auto result=f.gate.evaluate(req);
        assert(!result.allowed);
        assert(result.rejection.statusCode==403);
    }
}
