#include "Database.h"
#include "BackendRuntimeGeneration.h"
#include "EmbeddedBackendLifecycle.h"

#include <cassert>

int main()
{
    Database database;
    assert(database.open(":memory:"));

    assert(database.execute(
        "CREATE TABLE backend_agents ("
        "backend_id TEXT NOT NULL,"
        "backend_generation INTEGER NOT NULL,"
        "lease_expires_at INTEGER NOT NULL,"
        "revoked_at INTEGER NOT NULL,"
        "updated_at INTEGER NOT NULL"
        ");"));
    assert(database.execute(
        "INSERT INTO backend_agents("
        "backend_id,backend_generation,lease_expires_at,revoked_at,updated_at"
        ") VALUES('default',129,900,0,900);"));

    EmbeddedBackendLifecycleService lifecycle(database);
    assert(lifecycle.ensureSchema());

    assert(lifecycle.startBackend("default", 1000));

    auto state = lifecycle.statusForBackend("default", 1000);
    assert(state.present);
    assert(!state.online);
    assert(state.backendGeneration == 130);
    assert(state.heartbeatSequence == 0);

    assert(lifecycle.heartbeatBackend("default", true, 1001));
    state = lifecycle.statusForBackend("default", 1001);
    assert(state.present);
    assert(state.online);
    assert(state.backendGeneration == 130);
    assert(state.heartbeatSequence == 1);
    assert(state.leaseExpiresAt == 1031);

    state = lifecycle.statusForBackend("default", 1032);
    assert(state.present);
    assert(!state.online);
    assert(state.backendGeneration == 130);

    BackendRuntimeGenerationRepository generations(database);
    const auto externalTakeover = generations.allocate("default", 1033);
    assert(externalTakeover.accepted);
    assert(externalTakeover.generation == 131);
    assert(!lifecycle.heartbeatBackend("default", true, 1033));
    state = lifecycle.statusForBackend("default", 1033);
    assert(!state.present);

    assert(lifecycle.stopBackend("default", 1034));
    state = lifecycle.statusForBackend("default", 1033);
    assert(!state.present);

    assert(lifecycle.startBackend("default", 1040));
    state = lifecycle.statusForBackend("default", 1040);
    assert(state.present);
    assert(!state.online);
    assert(state.backendGeneration == 132);

    assert(lifecycle.heartbeatBackend("default", true, 1041));
    state = lifecycle.statusForBackend("default", 1041);
    assert(state.online);
    assert(state.backendGeneration == 132);

    assert(database.execute(
        "UPDATE backend_agents SET lease_expires_at=2000,updated_at=2000 "
        "WHERE backend_id='default';"));
    EmbeddedBackendLifecycleService blocked(database);
    assert(blocked.ensureSchema());
    assert(!blocked.startBackend("default", 1050));

    return 0;
}
