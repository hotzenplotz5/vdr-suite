#include "Database.h"
#include "EmbeddedBackendLifecycle.h"

#include <cassert>

int main()
{
    Database database;
    assert(database.open(":memory:"));

    assert(database.execute(
        "CREATE TABLE backend_agents ("
        "backend_id TEXT NOT NULL,"
        "backend_generation INTEGER NOT NULL"
        ");"));
    assert(database.execute(
        "INSERT INTO backend_agents(backend_id,backend_generation) "
        "VALUES('default',129);"));

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

    assert(lifecycle.stopBackend("default", 1033));
    state = lifecycle.statusForBackend("default", 1033);
    assert(!state.present);

    assert(lifecycle.startBackend("default", 1040));
    state = lifecycle.statusForBackend("default", 1040);
    assert(state.present);
    assert(!state.online);
    assert(state.backendGeneration == 131);

    assert(lifecycle.heartbeatBackend("default", true, 1041));
    state = lifecycle.statusForBackend("default", 1041);
    assert(state.online);
    assert(state.backendGeneration == 131);

    return 0;
}
