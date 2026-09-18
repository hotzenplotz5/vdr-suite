#include "BackendRuntimeGeneration.h"
#include "Database.h"

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

    BackendRuntimeGenerationRepository generations(database);
    assert(generations.ensureSchema());

    const auto first = generations.allocate("default", 1000);
    assert(first.accepted);
    assert(first.generation == 130);

    const auto second = generations.allocate("default", 1001);
    assert(second.accepted);
    assert(second.generation == 131);

    const auto other = generations.allocate("secondary", 1002);
    assert(other.accepted);
    assert(other.generation == 1);

    return 0;
}
