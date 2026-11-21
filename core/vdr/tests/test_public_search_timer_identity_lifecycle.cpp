#include "Database.h"
#include "PublicSearchTimerIdentityRepository.h"
#include "SearchTimer.h"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>
#include <unistd.h>

namespace
{
void sql(Database& database, const char* statement)
{
    assert(database.execute(statement));
}

int count(Database& database, const char* statement)
{
    sqlite3_stmt* query = nullptr;
    assert(sqlite3_prepare_v2(database.handle(), statement, -1,
        &query, nullptr) == SQLITE_OK);
    assert(sqlite3_step(query) == SQLITE_ROW);
    const int value = sqlite3_column_int(query, 0);
    assert(sqlite3_finalize(query) == SQLITE_OK);
    return value;
}

SearchTimer rule(const std::string& backend, const std::string& native,
    const std::string& query = "terra",
    SearchTimerState state = SearchTimerState::Active)
{
    SearchTimer result = SearchTimer::create(
        SearchTimerId::fromBackendNativeId(backend, native), query,
        query, state);
    result.comparisonOptions().setCompareTitle(true);
    return result;
}

std::string id(PublicSearchTimerIdentityRepository& repository,
    const std::string& backend, const std::vector<SearchTimer>& items)
{
    const auto ids = repository.reconcileAuthoritativeSnapshot(backend, items);
    assert(ids.has_value());
    assert(ids->size() == items.size());
    return ids->empty() ? std::string() : ids->at(0);
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    PublicSearchTimerIdentityRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.ensureSchema());

    // Simulate the two legacy mappings on the real yaVDR. They have no
    // authoritative fingerprint and the underlying rules have been deleted.
    sql(database,
        "INSERT INTO public_search_timer_identity "
        "VALUES('st_old0', 'default', '0'),"
        "('st_old1', 'default', '1');");
    assert(id(repository, "default", {}).empty());
    assert(count(database,
        "SELECT COUNT(*) FROM public_search_timer_identity "
        "WHERE backend_id='default';") == 0);
    assert(count(database,
        "SELECT COUNT(*) FROM public_search_timer_identity_retired;") == 2);

    // epgsearch reassigns native ID zero to a completely new rule.
    const std::string first = id(repository, "default",
        {rule("default", "0")});
    assert(first.rfind("st_", 0) == 0 && first.size() == 35);
    assert(first != "st_old0" && first != "st_old1");
    assert(id(repository, "default",
        {rule("default", "0", "terra", SearchTimerState::Inactive)}) == first);

    // Detect a changed rule even if deletion was missed between polls.
    const std::string replaced = id(repository, "default",
        {rule("default", "0", "new search")});
    assert(replaced != first);
    assert(count(database,
        "SELECT COUNT(*) FROM public_search_timer_identity_retired;") == 3);
    assert(id(repository, "default",
        {rule("default", "0", "new search")}) == replaced);

    // Two native IDs may have identical queries; both remain distinct.
    auto both = repository.reconcileAuthoritativeSnapshot("default",
        {rule("default", "0", "new search"),
         rule("default", "1", "new search")});
    assert(both.has_value() && both->size() == 2);
    assert(both->at(0) == replaced && both->at(1) != replaced);
    const auto second = both->at(1);

    // Backend isolation; a successful empty snapshot must affect only its
    // own backend's bindings.
    const std::string remote = id(repository, "other",
        {rule("other", "0")});
    assert(!remote.empty() && remote != replaced);
    assert(id(repository, "default", {}).empty());
    assert(id(repository, "other", {rule("other", "0")}) == remote);
    assert(count(database,
        "SELECT COUNT(*) FROM public_search_timer_identity "
        "WHERE backend_id='default';") == 0);
    assert(count(database,
        "SELECT COUNT(*) FROM public_search_timer_identity "
        "WHERE backend_id='other';") == 1);

    // Malformed/duplicate snapshots must not modify the database.
    assert(!repository.reconcileAuthoritativeSnapshot("other",
        {rule("other", "0"), rule("other", "0")}).has_value());
    assert(!repository.reconcileAuthoritativeSnapshot("other",
        {rule("WRONG", "0")}).has_value());
    assert(!repository.reconcileAuthoritativeSnapshot("", {}).has_value());
    assert(id(repository, "other", {rule("other", "0")}) == remote);

    // Roll back the entire reconciliation when a retire operation fails.
    sql(database,
        "CREATE TRIGGER deny_searchtimer_retire "
        "BEFORE INSERT ON public_search_timer_identity_retired "
        "BEGIN SELECT RAISE(ABORT, 'fixture denies retirement'); END;");
    assert(!repository.reconcileAuthoritativeSnapshot("other", {}).has_value());
    assert(count(database,
        "SELECT COUNT(*) FROM public_search_timer_identity "
        "WHERE backend_id='other';") == 1);
    sql(database, "DROP TRIGGER deny_searchtimer_retire;");
    assert(id(repository, "other", {}).empty());

    // A legacy mapping that was still present at deployment must rotate
    // on its first successful observation: continuity cannot be proven.
    sql(database,
        "INSERT INTO public_search_timer_identity "
        "VALUES('st_unverified', 'third', '0');");
    const std::string migrated = id(repository, "third",
        {rule("third", "0")});
    assert(migrated != "st_unverified");
    assert(id(repository, "third", {rule("third", "0")}) == migrated);

    // Simulate restart with a disposable on-disk DB.
    char path[] = "/tmp/vdr-suite-searchtimer-identity-XXXXXX";
    const int file = mkstemp(path);
    assert(file >= 0);
    assert(close(file) == 0);
    std::string persisted;
    {
        Database firstDatabase;
        assert(firstDatabase.open(path));
        PublicSearchTimerIdentityRepository firstRepository(firstDatabase);
        assert(firstRepository.ensureSchema());
        persisted = id(firstRepository, "default", {rule("default", "0")});
    }
    {
        Database reopened;
        assert(reopened.open(path));
        PublicSearchTimerIdentityRepository reopenedRepository(reopened);
        assert(reopenedRepository.ensureSchema());
        assert(id(reopenedRepository, "default",
            {rule("default", "0")}) == persisted);
        assert(id(reopenedRepository, "default", {}).empty());
        const auto replacement = id(reopenedRepository, "default",
            {rule("default", "0")});
        assert(replacement != persisted);
    }
    assert(unlink(path) == 0);
    (void)second;
    return 0;
}
