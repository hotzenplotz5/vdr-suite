#include "PublicRecordingIdentityRepository.h"
#include "Database.h"

#include <cassert>
#include <string>

int main()
{
    Database database;
    assert(database.open(":memory:"));
    PublicRecordingIdentityRepository repository(database);
    assert(repository.ensureSchema());

    const auto a = repository.resolveOrCreate("backend-a", "/vdr/a");
    assert(a.has_value());
    assert(a->rfind("rec_", 0) == 0);
    assert(a->size() == 36);
    assert(*a != "/vdr/a");
    assert(repository.resolveOrCreate("backend-a", "/vdr/a") == a);

    PublicRecordingIdentityRepository sameDatabase(database);
    assert(sameDatabase.find("backend-a", "/vdr/a") == a);

    const auto otherBackend = repository.resolveOrCreate("backend-b", "/vdr/a");
    assert(otherBackend.has_value() && otherBackend != a);
    const auto otherRecording = repository.resolveOrCreate("backend-a", "/vdr/b");
    assert(otherRecording.has_value() && otherRecording != a);

    // Collision: a second existing binding must not be overwritten by a move.
    assert(!repository.rebindAfterVerifiedMove("backend-a", "/vdr/a", "/vdr/b"));
    assert(repository.find("backend-a", "/vdr/a") == a);

    // Move is explicit, never inferred from matching title or fingerprints.
    assert(repository.rebindAfterVerifiedMove("backend-a", "/vdr/a", "/new/a"));
    assert(repository.find("backend-a", "/new/a") == a);
    assert(!repository.find("backend-a", "/vdr/a").has_value());
    assert(!repository.rebindAfterVerifiedMove("backend-a", "/vdr/a", "/newer/a"));

    // Reuse of a previously deleted backend-native address must get a NEW ID.
    assert(repository.removeAfterVerifiedDeletion("backend-a", "/new/a"));
    assert(!repository.find("backend-a", "/new/a").has_value());
    const auto replacement = repository.resolveOrCreate("backend-a", "/new/a");
    assert(replacement.has_value() && replacement != a);
    assert(!repository.removeAfterVerifiedDeletion("backend-a", "/missing"));

    assert(!repository.resolveOrCreate("", "/vdr/a").has_value());
    assert(!repository.resolveOrCreate("backend-a", "").has_value());
    assert(!repository.find("backend-a", "").has_value());
    assert(!repository.rebindAfterVerifiedMove("backend-a", "/new/a", ""));

    return 0;
}
