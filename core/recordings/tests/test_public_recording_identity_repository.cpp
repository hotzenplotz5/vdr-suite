#include "PublicRecordingIdentityRepository.h"
#include "Database.h"

#include <cassert>
#include <sqlite3.h>
#include <cstdlib>
#include <unistd.h>
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
    assert(repository.findNativeForPublicId("backend-a", *a) ==
        std::optional<std::string>("/vdr/a"));
    assert(!repository.findNativeForPublicId("backend-b", *a).has_value());
    assert(!repository.findNativeForPublicId("backend-a", "rec_invalid").has_value());
    assert(!repository.findNativeForPublicId("", *a).has_value());
    assert(!repository.findNativeForPublicId("backend-a",
        "rec_00000000000000000000000000000000").has_value());

    const auto otherBackend = repository.resolveOrCreate("backend-b", "/vdr/a");
    assert(otherBackend.has_value() && otherBackend != a);
    const auto otherRecording = repository.resolveOrCreate("backend-a", "/vdr/b");
    assert(otherRecording.has_value() && otherRecording != a);
    assert(repository.findNativeForPublicId("backend-b", *otherBackend) ==
        std::optional<std::string>("/vdr/a"));
    assert(!repository.findNativeForPublicId("backend-a", *otherBackend)
        .has_value());
    assert(repository.findNativeForPublicId("backend-a", *otherRecording) ==
        std::optional<std::string>("/vdr/b"));

    // Existing Continue Watching state must follow a verified public-ID
    // rebind for *every* actor, without resetting the progress CAS revision.
    assert(database.execute(
        "CREATE TABLE continue_watching_state ("
        "actor_id TEXT, backend_id TEXT, recording_id TEXT, "
        "backend_native_id TEXT, position_seconds INTEGER, revision INTEGER);"));
    assert(database.execute(
        "INSERT INTO continue_watching_state VALUES "
        "('actor-one','backend-a','r1','/vdr/a',123,7),"
        "('actor-two','backend-a','r1','/vdr/a',456,2),"
        "('actor-other','backend-b','r1','/vdr/a',800,9);"));
    const auto progressRowCount = [&](const std::string& backend,
                                      const std::string& native) {
        sqlite3_stmt* statement = nullptr;
        const char* sql =
            "SELECT COUNT(*) FROM continue_watching_state "
            "WHERE backend_id=?1 AND backend_native_id=?2;";
        assert(sqlite3_prepare_v2(database.handle(),sql,-1,&statement,nullptr)
               == SQLITE_OK);
        assert(sqlite3_bind_text(statement,1,backend.c_str(),-1,SQLITE_TRANSIENT)
               == SQLITE_OK);
        assert(sqlite3_bind_text(statement,2,native.c_str(),-1,SQLITE_TRANSIENT)
               == SQLITE_OK);
        assert(sqlite3_step(statement) == SQLITE_ROW);
        const int count = sqlite3_column_int(statement,0);
        sqlite3_finalize(statement);
        return count;
    };
    assert(progressRowCount("backend-a", "/vdr/a") == 2);
    assert(progressRowCount("backend-b", "/vdr/a") == 1);

    // Collision: a second existing binding must not be overwritten by a move.
    assert(!repository.rebindAfterVerifiedMove("backend-a", "/vdr/a", "/vdr/b"));
    assert(repository.find("backend-a", "/vdr/a") == a);

    // Move is explicit, never inferred from matching title or fingerprints.
    assert(repository.rebindAfterVerifiedMove("backend-a", "/vdr/a", "/new/a"));
    assert(repository.find("backend-a", "/new/a") == a);
    assert(progressRowCount("backend-a", "/new/a") == 2);
    assert(progressRowCount("backend-a", "/vdr/a") == 0);
    assert(progressRowCount("backend-b", "/vdr/a") == 1);
    sqlite3_stmt* check = nullptr;
    assert(sqlite3_prepare_v2(database.handle(),
        "SELECT position_seconds,revision FROM continue_watching_state "
        "WHERE actor_id='actor-one';",-1,&check,nullptr) == SQLITE_OK);
    assert(sqlite3_step(check) == SQLITE_ROW);
    assert(sqlite3_column_int(check,0) == 123);
    assert(sqlite3_column_int(check,1) == 7);
    sqlite3_finalize(check);

    assert(repository.findNativeForPublicId("backend-a", *a) ==
        std::optional<std::string>("/new/a"));
    assert(!repository.find("backend-a", "/vdr/a").has_value());
    assert(!repository.rebindAfterVerifiedMove("backend-a", "/vdr/a", "/newer/a"));

    // Reuse of a previously deleted backend-native address must get a NEW ID.
    assert(repository.removeAfterVerifiedDeletion("backend-a", "/new/a"));
    assert(!repository.find("backend-a", "/new/a").has_value());
    assert(!repository.findNativeForPublicId("backend-a", *a).has_value());
    const auto replacement = repository.resolveOrCreate("backend-a", "/new/a");
    assert(replacement.has_value() && replacement != a);
    assert(!repository.findNativeForPublicId("backend-a", *a).has_value());
    assert(repository.findNativeForPublicId("backend-a", *replacement) ==
        std::optional<std::string>("/new/a"));
    assert(!repository.removeAfterVerifiedDeletion("backend-a", "/missing"));

    assert(!repository.resolveOrCreate("", "/vdr/a").has_value());
    assert(!repository.resolveOrCreate("backend-a", "").has_value());
    assert(!repository.find("backend-a", "").has_value());
    assert(!repository.rebindAfterVerifiedMove("backend-a", "/new/a", ""));

    char path[] = "/tmp/vdr-suite-recording-identity-XXXXXX";
    const int temporary = mkstemp(path);
    assert(temporary >= 0);
    assert(close(temporary) == 0);
    std::string persistentId;
    {
        Database persisted;
        assert(persisted.open(path));
        PublicRecordingIdentityRepository first(persisted);
        assert(first.ensureSchema());
        const auto id = first.resolveOrCreate("backend-a", "/persistent/recording");
        assert(id.has_value());
        persistentId = *id;
    }
    {
        Database reopened;
        assert(reopened.open(path));
        PublicRecordingIdentityRepository second(reopened);
        assert(second.ensureSchema());
        assert(second.find("backend-a", "/persistent/recording") ==
            std::optional<std::string>(persistentId));
        assert(second.findNativeForPublicId("backend-a", persistentId) ==
            std::optional<std::string>("/persistent/recording"));
    }
    assert(unlink(path) == 0);

    // Persistence failures roll back the public-ID rebind as well. Do not
    // publish a moved identity with partially migrated progress.
    Database damaged;
    assert(damaged.open(":memory:"));
    PublicRecordingIdentityRepository damagedIds(damaged);
    assert(damagedIds.ensureSchema());
    const auto beforeMove = damagedIds.resolveOrCreate("backend-d", "/before");
    assert(beforeMove);
    assert(damaged.execute(
        "CREATE TABLE continue_watching_state (actor_id TEXT);"));
    assert(!damagedIds.rebindAfterVerifiedMove("backend-d", "/before", "/after"));
    assert(damagedIds.find("backend-d", "/before") == beforeMove);
    assert(!damagedIds.find("backend-d", "/after"));

    return 0;
}
