#include "SecurityHttpGateBrowserTestFixture.h"
#include <cassert>
#include <string>

int main() {
    const std::string path="/api/v1/devices/device_10001/playback-account";

    {
        SecurityHttpGateBrowserTestFixture f;
        HttpServerRequest get;
        get.method="GET";
        get.path=path;
        const auto d=f.gate.evaluate(get);
        assert(!d.allowed && d.rejection.statusCode==401);
    }
    {
        SecurityHttpGateBrowserTestFixture f;
        HttpServerRequest get;
        get.method="GET"; get.path=path;
        f.addBrowserAuthentication(get);
        const auto d=f.gate.evaluate(get);
        assert(!d.allowed && d.rejection.statusCode==403);
    }
    {
        SecurityHttpGateBrowserTestFixture f;
        assert(f.grantRepository.ensureGrant(f.actorId,"role.admin","*"));
        HttpServerRequest get;
        get.method="GET"; get.path=path;
        f.addBrowserAuthentication(get);
        const auto d=f.gate.evaluate(get);
        assert(d.allowed);
        assert(d.authorizationDecision.permission=="devices.grants.view");
    }
    {
        SecurityHttpGateBrowserTestFixture f;
        assert(f.grantRepository.ensureGrant(f.actorId,"role.admin","default"));
        HttpServerRequest get;
        get.method="GET"; get.path=path;
        f.addBrowserAuthentication(get);
        assert(f.gate.evaluate(get).rejection.statusCode==403);
    }
    {
        SecurityHttpGateBrowserTestFixture f;
        assert(f.grantRepository.ensureGrant(f.actorId,"role.admin","*"));
        HttpServerRequest post;
        post.method="POST"; post.path=path;
        post.body=R"({"accountId":"account_alice","expectedRevision":0})";
        f.addBrowserAuthentication(post,false);
        const auto d=f.gate.evaluate(post);
        assert(!d.allowed && d.rejection.statusCode==403);
        assert(d.protectedMutation);
    }
    {
        SecurityHttpGateBrowserTestFixture f;
        assert(f.grantRepository.ensureGrant(f.actorId,"role.admin","*"));
        HttpServerRequest post;
        post.method="POST"; post.path=path;
        post.body=R"({"accountId":"account_alice","expectedRevision":0})";
        f.addBrowserAuthentication(post,true);
        const auto d=f.gate.evaluate(post);
        assert(d.allowed && d.protectedMutation);
        assert(d.authorizationDecision.permission=="devices.grants.modify");
        assert(d.authorizationDecision.backendId=="*");
    }
    {
        SecurityHttpGateBrowserTestFixture f;
        assert(f.grantRepository.ensureGrant(f.actorId,"role.admin","*"));
        assert(f.grantRepository.ensureGrant(f.actorId,"role.read-only","*"));
        HttpServerRequest post;
        post.method="POST"; post.path=path;
        f.addBrowserAuthentication(post,true);
        assert(f.gate.evaluate(post).rejection.statusCode==403);
    }
    return 0;
}
