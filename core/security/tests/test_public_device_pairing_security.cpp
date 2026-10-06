#include "SecurityHttpGateBrowserTestFixture.h"

#include <cassert>

int main()
{
    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest create;
        create.method = "POST";
        create.path = "/api/v1/device-pairings";
        create.headers["X-Request-ID"] = "mu10a-anonymous-create";
        create.headers["Content-Type"] = "application/json";
        create.body = "{\"displayName\":\"Hisense TV\",\"clientKind\":\"vidaa\"}";

        const SecurityGateDecision decision = fixture.gate.evaluate(create);
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(!decision.context.authenticated());
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest poll;
        poll.method = "GET";
        poll.path =
            "/api/v1/device-pairings/dpr_0123456789abcdef0123456789abcdef";
        poll.headers["X-VDR-Suite-Pairing-Token"] = "opaque-poll-token";

        const SecurityGateDecision decision = fixture.gate.evaluate(poll);
        assert(decision.allowed);
        assert(!decision.protectedMutation);
        assert(!decision.context.authenticated());
    }

    {
        SecurityHttpGateBrowserTestFixture fixture;
        HttpServerRequest create;
        create.method = "POST";
        create.path = "/api/v1/device-pairings";
        create.headers["Content-Type"] = "application/json";
        create.body = "{\"displayName\":\"Browser test\",\"clientKind\":\"test\"}";
        fixture.addBrowserAuthentication(create, false);

        const SecurityGateDecision decision = fixture.gate.evaluate(create);
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
