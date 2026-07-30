#include "Database.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <string>

int main()
{
    Database database;
    assert(database.open(":memory:"));

    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());

    assert(identityRepository.ensureCompatibilityIdentity(
        "actor-a",
        ActorType::User,
        "Actor A",
        "device-a",
        "session-a",
        "credential-a"));

    assert(identityRepository.ensureCompatibilityIdentity(
        "actor-b",
        ActorType::User,
        "Actor B",
        "device-b",
        "session-b",
        "credential-b"));

    SecurityPermissionGrantRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.ensureSchema());

    const auto initiallyEmpty =
        repository.findActiveGrantsForActor("actor-a");
    assert(initiallyEmpty.available);
    assert(initiallyEmpty.grants.empty());

    assert(repository.ensureGrant(
        "actor-a",
        "remote.control",
        "default"));
    assert(repository.ensureGrant(
        "actor-a",
        "recordings.view",
        "*"));
    assert(repository.ensureGrant(
        "actor-b",
        "remote.control",
        "house-b"));
    assert(repository.ensureGrant(
        "actor-b",
        "role.admin",
        "house-b"));
    assert(repository.ensureGrant(
        "actor-b",
        "role.read-only",
        "house-c"));

    assert(repository.ensureGrant(
        "actor-a",
        "remote.control",
        "default"));

    const auto actorAGrants =
        repository.findActiveGrantsForActor("actor-a");
    assert(actorAGrants.available);
    assert(actorAGrants.grants.size() == 2);
    assert(actorAGrants.grants[0].permission ==
        "recordings.view");
    assert(actorAGrants.grants[0].backendId == "*");
    assert(actorAGrants.grants[1].permission ==
        "remote.control");
    assert(actorAGrants.grants[1].backendId ==
        "default");

    const auto actorBGrants =
        repository.findActiveGrantsForActor("actor-b");
    assert(actorBGrants.available);
    assert(actorBGrants.grants.size() == 3);
    assert(actorBGrants.grants[0].permission == "remote.control");
    assert(actorBGrants.grants[0].backendId == "house-b");
    assert(actorBGrants.grants[1].permission == "role.admin");
    assert(actorBGrants.grants[1].backendId == "house-b");
    assert(actorBGrants.grants[2].permission == "role.read-only");
    assert(actorBGrants.grants[2].backendId == "house-c");

    assert(repository.revokeGrant(
        "actor-a",
        "remote.control",
        "default"));
    assert(!repository.revokeGrant(
        "actor-a",
        "remote.control",
        "default"));

    const auto afterRevoke =
        repository.findActiveGrantsForActor("actor-a");
    assert(afterRevoke.available);
    assert(afterRevoke.grants.size() == 1);
    assert(afterRevoke.grants.front().permission ==
        "recordings.view");

    assert(repository.ensureGrant(
        "actor-a",
        "remote.control",
        "default"));

    const auto afterReactivation =
        repository.findActiveGrantsForActor("actor-a");
    assert(afterReactivation.available);
    assert(afterReactivation.grants.size() == 2);

    assert(!repository.ensureGrant("", "remote.control", "default"));
    assert(!repository.ensureGrant("actor-a", "", "default"));
    assert(!repository.ensureGrant("actor-a", "remote.control", ""));
    assert(!repository.revokeGrant(
        "actor-a",
        "missing.permission",
        "default"));

    Database closedDatabase;
    SecurityPermissionGrantRepository unavailableRepository(
        closedDatabase);

    const auto unavailable =
        unavailableRepository.findActiveGrantsForActor("actor-a");
    assert(!unavailable.available);
    assert(unavailable.grants.empty());
    assert(!unavailableRepository.ensureSchema());

    return 0;
}
