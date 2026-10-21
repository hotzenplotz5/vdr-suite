#include "BackendAgentMediaImportCatalog.h"
#include <cassert>
#include <string>
#include <vector>
int main()
{
    BackendAgentMediaImportCatalog catalog;
    RequestSecurityContext actor;
    actor.authenticationState = AuthenticationState::Authenticated;
    actor.actor = {"actor-agent", ActorType::Agent, "Agent", true};
    BackendAgentRecord live;
    live.backendId = "backend";
    live.agentId = "agent";
    live.actorId = "actor-agent";
    live.agentInstanceId = "instance";
    live.backendGeneration = 7;
    live.leaseExpiresAt = 300;
    std::vector<std::string> items;
    assert(!catalog.list(live, "backend", 100, items));
    assert(catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000002", "source_00000000001"}, 100));
    assert(catalog.list(live, "backend", 100, items));
    assert(items == (std::vector<std::string>{"source_00000000001",
                                             "source_00000000002"}));
    assert(!catalog.list(live, "backend", 191, items));
    assert(items.empty());
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000001", "source_00000000001"}, 120));
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {"/etc/passwd"}, 120));
    actor.actor.actorId = "spoofed";
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000001"}, 120));
    actor.actor.actorId = "actor-agent";
    live.agentInstanceId = "restarted";
    assert(!catalog.list(live, "backend", 140, items));
    assert(catalog.publish(actor, live, "backend", "restarted", 7,
        {"source_00000000003"}, 140));
    live.backendGeneration = 8;
    assert(!catalog.list(live, "backend", 145, items));
    assert(!catalog.publish(actor, live, "backend", "restarted", 7,
        {"source_00000000003"}, 145));
    assert(catalog.publish(actor, live, "backend", "restarted", 8,
        {}, 145));
    assert(catalog.list(live, "backend", 145, items) && items.empty());
    live.revoked = true;
    assert(!catalog.list(live, "backend", 147, items));
    live.revoked = false;
    live.leaseExpiresAt = 146;
    assert(!catalog.list(live, "backend", 147, items));
    return 0;
}
