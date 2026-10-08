#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>

int main()
{
    const std::string itemPath =
        "/api/v1/device-pairings/"
        "dpr_0123456789abcdef0123456789abcdef";

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest create;
        create.method = "POST";
        create.path = "/api/v1/device-pairings";
        create.headers["X-Request-ID"] =
            "mu10a-anonymous-create";
        create.headers["Content-Type"] =
            "application/json";
        create.body =
            "{\"displayName\":\"Hisense TV\","
            "\"clientKind\":\"vidaa\"}";

        const SecurityGateDecision decision =
            fixture.gate.evaluate(create);
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(!decision.context.authenticated());
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest poll;
        poll.method = "GET";
        poll.path = itemPath;
        poll.headers["X-VDR-Suite-Pairing-Token"] =
            "opaque-poll-token";

        const SecurityGateDecision decision =
            fixture.gate.evaluate(poll);
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(!decision.context.authenticated());
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest activation;
        activation.method = "POST";
        activation.path = itemPath + "/credential";
        activation.headers["X-VDR-Suite-Pairing-Token"] =
            "opaque-activation-token";
        const SecurityGateDecision decision =
            fixture.gate.evaluate(activation);
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(!decision.context.authenticated());
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest activation;
        activation.method = "POST";
        activation.path = itemPath + "/credential";
        const SecurityGateDecision decision =
            fixture.gate.evaluate(activation);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest unrelated;
        unrelated.method = "POST";
        unrelated.path = "/api/v1/device-pairings/any/credential/more";
        unrelated.headers["X-VDR-Suite-Pairing-Token"] = "opaque-token";
        const SecurityGateDecision decision =
            fixture.gate.evaluate(unrelated);
        assert(!decision.allowed);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest collection;
        collection.method = "GET";
        collection.path = "/api/v1/device-pairings";

        const SecurityGateDecision decision =
            fixture.gate.evaluate(collection);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest item;
        item.method = "GET";
        item.path = itemPath;

        const SecurityGateDecision decision =
            fixture.gate.evaluate(item);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest collection;
        collection.method = "GET";
        collection.path = "/api/v1/device-pairings";
        fixture.addBrowserAuthentication(collection, false);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(collection);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));

        HttpServerRequest collection;
        collection.method = "GET";
        collection.path = "/api/v1/device-pairings";
        fixture.addBrowserAuthentication(collection, false);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(collection);
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.context.authenticated());
        assert(
            decision.authorizationDecision.permission ==
            "device.pairing.view");
        assert(
            decision.authorizationDecision.backendId ==
            "*");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));

        HttpServerRequest mutation;
        mutation.method = "POST";
        mutation.path = itemPath;
        mutation.body =
            "{\"decision\":\"approve\"}";
        fixture.addBrowserAuthentication(mutation, false);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(mutation);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.protectedMutation);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));

        HttpServerRequest mutation;
        mutation.method = "POST";
        mutation.path = itemPath;
        mutation.body =
            "{\"decision\":\"approve\"}";
        fixture.addBrowserAuthentication(mutation, true);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(mutation);
        assert(decision.allowed);
        assert(decision.protectedMutation);
        assert(decision.context.authenticated());
        assert(
            decision.authorizationDecision.permission ==
            "device.pairing.decide");
        assert(
            decision.authorizationDecision.backendId ==
            "*");
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.admin",
            "*"));
        assert(fixture.grantRepository.ensureGrant(
            fixture.actorId,
            "role.read-only",
            "*"));

        HttpServerRequest mutation;
        mutation.method = "POST";
        mutation.path = itemPath;
        mutation.body =
            "{\"decision\":\"reject\"}";
        fixture.addBrowserAuthentication(mutation, true);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(mutation);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 403);
        assert(decision.protectedMutation);
        assert(decision.rejection.body.find(
            "role_read_only") != std::string::npos);
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest create;
        create.method = "POST";
        create.path = "/api/v1/device-pairings";
        create.headers["Content-Type"] =
            "application/json";
        create.body =
            "{\"displayName\":\"Browser test\","
            "\"clientKind\":\"test\"}";
        fixture.addBrowserAuthentication(create, false);

        const SecurityGateDecision decision =
            fixture.gate.evaluate(create);
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(decision.context.authenticated());
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest ordinaryPublic;
        ordinaryPublic.method = "GET";
        ordinaryPublic.path = "/api/v1/backends";

        const SecurityGateDecision decision =
            fixture.gate.evaluate(ordinaryPublic);
        assert(!decision.allowed);
        assert(decision.rejection.statusCode == 401);
    }

    return 0;
}
