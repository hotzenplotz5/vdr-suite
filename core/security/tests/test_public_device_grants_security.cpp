#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>
#include <string>

int main()
{
    const std::string path = "/api/v1/devices/device_10001/grants";

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest read;
        read.method = "GET";
        read.path = path;
        assert(!fixture.gate.evaluate(read).allowed);
        assert(fixture.gate.evaluate(read).rejection.statusCode == 401);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest read;
        read.method = "GET";
        read.path = path;
        fixture.addBrowserAuthentication(read);
        const auto decision = fixture.gate.evaluate(read);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "*"));
        HttpServerRequest read;
        read.method = "GET";
        read.path = path;
        fixture.addBrowserAuthentication(read);
        const auto decision = fixture.gate.evaluate(read);
        assert(decision.allowed);
        assert(decision.context.authenticated());
        assert(decision.authorizationDecision.permission ==
            "devices.grants.view");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "default"));
        HttpServerRequest read;
        read.method = "GET";
        read.path = path;
        fixture.addBrowserAuthentication(read);
        const auto decision = fixture.gate.evaluate(read);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "*"));
        HttpServerRequest post;
        post.method = "POST";
        post.path = path;
        post.body =
            "{\"permission\":\"channels.view\","
            "\"backendId\":\"default\",\"active\":true}";
        fixture.addBrowserAuthentication(post, false);
        const auto decision = fixture.gate.evaluate(post);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.protectedMutation);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "*"));
        HttpServerRequest post;
        post.method = "POST";
        post.path = path;
        post.body =
            "{\"permission\":\"channels.view\","
            "\"backendId\":\"default\",\"active\":true}";
        fixture.addBrowserAuthentication(post, true);
        const auto decision = fixture.gate.evaluate(post);
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.authorizationDecision.permission ==
            "devices.grants.modify");
        assert(decision.authorizationDecision.backendId == "*");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.admin", "*"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId, "role.read-only", "*"));
        HttpServerRequest post;
        post.method = "POST";
        post.path = path;
        fixture.addBrowserAuthentication(post, true);
        const auto decision = fixture.gate.evaluate(post);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
    }
    return 0;
}
