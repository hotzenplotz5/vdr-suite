#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    if new in text:
        return
    if old not in text:
        raise SystemExit(f"expected block not found in {path}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


identity_inc = r'''constexpr long long EpgIdentityTimeToleranceSeconds = 5;

struct EpgEventIdentityReplacement
{
    std::string channelId;
    std::string previousEventId;
    std::string currentEventId;
};

std::string eventIdentityKey(
    const std::string& channelId,
    const std::string& eventId)
{
    return channelId + '\x1f' + eventId;
}

bool executeBoundStatement(
    Database& database,
    const char* sql,
    const std::vector<std::string>& values)
{
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    bool bound = true;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        bound = bound && sqlite3_bind_text(
            statement,
            static_cast<int>(index + 1),
            values[index].c_str(),
            -1,
            SQLITE_TRANSIENT) == SQLITE_OK;
    }

    const bool ok = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

bool migrateSupersededEventData(
    Database& database,
    const std::string& backendId,
    const EpgEventIdentityReplacement& replacement)
{
    const std::vector<std::string> oldKey = {
        backendId,
        replacement.channelId,
        replacement.previousEventId};

    if (database.tableExists("epg_event_artwork") &&
        !executeBoundStatement(
            database,
            "INSERT OR IGNORE INTO epg_event_artwork "
            "(backend_id,channel_id,event_id,provider,path,width,height,resolved_at,updated_at) "
            "SELECT backend_id,channel_id,?,provider,path,width,height,resolved_at,CURRENT_TIMESTAMP "
            "FROM epg_event_artwork WHERE backend_id=? AND channel_id=? AND event_id=?;",
            {replacement.currentEventId,
             backendId,
             replacement.channelId,
             replacement.previousEventId}))
    {
        return false;
    }

    if (database.tableExists("epg_scraper_metadata_cache"))
    {
        const std::string oldToken =
            "&eventId=" + replacement.previousEventId + "&";
        const std::string newToken =
            "&eventId=" + replacement.currentEventId + "&";
        if (!executeBoundStatement(
                database,
                "INSERT OR IGNORE INTO epg_scraper_metadata_cache "
                "(backend_id,channel_id,event_id,public_json,resolved_at,updated_at) "
                "SELECT backend_id,channel_id,?,REPLACE(public_json,?,?),resolved_at,CURRENT_TIMESTAMP "
                "FROM epg_scraper_metadata_cache "
                "WHERE backend_id=? AND channel_id=? AND event_id=?;",
                {replacement.currentEventId,
                 oldToken,
                 newToken,
                 backendId,
                 replacement.channelId,
                 replacement.previousEventId}))
        {
            return false;
        }
    }

    if (database.tableExists("epg_scraper_metadata_images") &&
        !executeBoundStatement(
            database,
            "INSERT OR IGNORE INTO epg_scraper_metadata_images "
            "(backend_id,channel_id,event_id,kind,image_index,provider,path,width,height,resolved_at,updated_at) "
            "SELECT backend_id,channel_id,?,kind,image_index,provider,path,width,height,resolved_at,CURRENT_TIMESTAMP "
            "FROM epg_scraper_metadata_images "
            "WHERE backend_id=? AND channel_id=? AND event_id=?;",
            {replacement.currentEventId,
             backendId,
             replacement.channelId,
             replacement.previousEventId}))
    {
        return false;
    }

    if (database.tableExists("suite_metadata_target_bindings"))
    {
        if (database.tableExists("suite_metadata_genre_assignments") &&
            !executeBoundStatement(
                database,
                "UPDATE suite_metadata_genre_assignments "
                "SET assignment_state='stale',updated_at=CURRENT_TIMESTAMP "
                "WHERE metadata_target_id IN ("
                "SELECT metadata_target_id FROM suite_metadata_target_bindings "
                "WHERE backend_id=? AND target_type='program-event' "
                "AND channel_id=? AND native_id=?"
                ") AND assignment_state IN ('active','unknown','conflict');",
                oldKey))
        {
            return false;
        }

        if (database.tableExists("suite_metadata_targets") &&
            !executeBoundStatement(
                database,
                "UPDATE suite_metadata_targets "
                "SET lifecycle_state='retired',updated_at=CURRENT_TIMESTAMP "
                "WHERE metadata_target_id IN ("
                "SELECT metadata_target_id FROM suite_metadata_target_bindings "
                "WHERE backend_id=? AND target_type='program-event' "
                "AND channel_id=? AND native_id=?"
                ");",
                oldKey))
        {
            return false;
        }

        if (!executeBoundStatement(
                database,
                "UPDATE suite_metadata_target_bindings "
                "SET lifecycle_state='retired',updated_at=CURRENT_TIMESTAMP "
                "WHERE backend_id=? AND target_type='program-event' "
                "AND channel_id=? AND native_id=?;",
                oldKey))
        {
            return false;
        }
    }

    for (const char* table : {
             "epg_event_artwork",
             "epg_scraper_metadata_cache",
             "epg_scraper_metadata_images"})
    {
        if (database.tableExists(table))
        {
            const std::string sql =
                std::string("DELETE FROM ") + table +
                " WHERE backend_id=? AND channel_id=? AND event_id=?;";
            if (!executeBoundStatement(database, sql.c_str(), oldKey))
            {
                return false;
            }
        }
    }

    return executeBoundStatement(
        database,
        "DELETE FROM epg_events "
        "WHERE backend_id=? AND channel_id=? AND event_id=?;",
        oldKey);
}

bool reconcileSupersededEpgEvents(
    Database& database,
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    std::set<std::string> incomingIdentities;
    for (const VdrEvent& event : events)
    {
        if (!event.channelId.empty())
        {
            incomingIdentities.insert(eventIdentityKey(
                event.channelId,
                stableEventId(event)));
        }
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT event_id FROM epg_events "
        "WHERE backend_id=? AND channel_id=? AND event_id<>? "
        "AND title=? AND subtitle=? "
        "AND ABS(CAST(start_time AS INTEGER)-CAST(? AS INTEGER))<=? "
        "AND ABS(CAST(end_time AS INTEGER)-CAST(? AS INTEGER))<=?;";
    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    std::vector<EpgEventIdentityReplacement> replacements;
    std::set<std::string> replacedIdentities;
    for (const VdrEvent& event : events)
    {
        if (event.channelId.empty()) continue;

        const std::string currentEventId = stableEventId(event);
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
        bindText(statement, 1, backendId);
        bindText(statement, 2, event.channelId);
        bindText(statement, 3, currentEventId);
        bindText(statement, 4, event.title);
        bindText(statement, 5, event.subtitle);
        bindText(statement, 6, event.startTime);
        sqlite3_bind_int64(statement, 7, EpgIdentityTimeToleranceSeconds);
        bindText(statement, 8, event.endTime);
        sqlite3_bind_int64(statement, 9, EpgIdentityTimeToleranceSeconds);

        while (sqlite3_step(statement) == SQLITE_ROW)
        {
            const std::string previousEventId = columnText(statement, 0);
            const std::string previousKey = eventIdentityKey(
                event.channelId,
                previousEventId);
            if (incomingIdentities.find(previousKey) != incomingIdentities.end() ||
                !replacedIdentities.insert(previousKey).second)
            {
                continue;
            }

            replacements.push_back({
                event.channelId,
                previousEventId,
                currentEventId});
        }
    }
    sqlite3_finalize(statement);

    for (const EpgEventIdentityReplacement& replacement : replacements)
    {
        if (!migrateSupersededEventData(database, backendId, replacement))
        {
            return false;
        }
    }
    return true;
}
'''

inc_path = ROOT / "core/vdr/src/EpgEventRepositoryIdentity.inc"
if not inc_path.exists():
    inc_path.write_text(identity_inc, encoding="utf-8")
elif inc_path.read_text(encoding="utf-8") != identity_inc:
    raise SystemExit(f"unexpected existing content in {inc_path}")

repository_cpp = ROOT / "core/vdr/src/EpgEventRepository.cpp"
replace_once(
    repository_cpp,
    "#include <sstream>\n#include <string>\n#include <vector>\n",
    "#include <set>\n#include <sstream>\n#include <string>\n#include <vector>\n")
replace_once(
    repository_cpp,
    "    return output.str();\n}\n\n}\n",
    "    return output.str();\n}\n\n#include \"EpgEventRepositoryIdentity.inc\"\n\n}\n")
replace_once(
    repository_cpp,
    "    const std::string normalizedBackendId = normalizeBackendId(backendId);\n\n    for (const VdrEvent& event : events)\n",
    "    const std::string normalizedBackendId = normalizeBackendId(backendId);\n\n"
    "    if (!reconcileSupersededEpgEvents(\n"
    "            database_,\n"
    "            normalizedBackendId,\n"
    "            events))\n"
    "    {\n"
    "        sqlite3_finalize(stmt);\n"
    "        database_.execute(\"ROLLBACK;\");\n"
    "        return false;\n"
    "    }\n\n"
    "    for (const VdrEvent& event : events)\n")

repository_test = ROOT / "core/vdr/tests/test_epg_event_repository.cpp"
replace_once(
    repository_test,
    "#include \"EpgEventRepository.h\"\n\n#include <cassert>\n",
    "#include \"EpgEventRepository.h\"\n\n#include <sqlite3.h>\n\n#include <cassert>\n")
replace_once(
    repository_test,
    "static void test_window_and_cleanup_are_backend_scoped()\n",
    r'''static std::string scalar_text(
    Database& database,
    const std::string& sql)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(
        database.handle(), sql.c_str(), -1, &statement, nullptr) == SQLITE_OK);
    std::string value;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        const unsigned char* text = sqlite3_column_text(statement, 0);
        value = text ? reinterpret_cast<const char*>(text) : std::string{};
    }
    sqlite3_finalize(statement);
    return value;
}

static int scalar_int(
    Database& database,
    const std::string& sql)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(
        database.handle(), sql.c_str(), -1, &statement, nullptr) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);
    const int value = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return value;
}

static void test_changed_event_ids_supersede_old_cache_identity()
{
    std::remove("/tmp/vdr-suite-epg-event-repository-identity-test.db");

    Database database;
    assert(database.open(
        "/tmp/vdr-suite-epg-event-repository-identity-test.db"));

    EpgEventRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.execute(
        "CREATE TABLE epg_event_artwork(backend_id TEXT,channel_id TEXT,event_id TEXT,provider TEXT,path TEXT,width INTEGER,height INTEGER,resolved_at INTEGER,updated_at TEXT,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_scraper_metadata_cache(backend_id TEXT,channel_id TEXT,event_id TEXT,public_json TEXT,resolved_at INTEGER,updated_at TEXT,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_scraper_metadata_images(backend_id TEXT,channel_id TEXT,event_id TEXT,kind TEXT,image_index INTEGER,provider TEXT,path TEXT,width INTEGER,height INTEGER,resolved_at INTEGER,updated_at TEXT,PRIMARY KEY(backend_id,channel_id,event_id,kind,image_index));"
        "CREATE TABLE suite_metadata_targets(metadata_target_id TEXT PRIMARY KEY,lifecycle_state TEXT,updated_at TEXT);"
        "CREATE TABLE suite_metadata_target_bindings(metadata_target_id TEXT PRIMARY KEY,target_type TEXT,backend_id TEXT,channel_id TEXT,native_id TEXT,lifecycle_state TEXT,updated_at TEXT);"
        "CREATE TABLE suite_metadata_genre_assignments(metadata_target_id TEXT,assignment_state TEXT,updated_at TEXT);"));

    const VdrEvent previous = make_event(
        "38845", "channel-1", "The Great Wall", "1784924163", "1784930894");
    assert(repository.upsertEventsForBackend("default", {previous}));
    assert(database.execute(
        "INSERT INTO epg_event_artwork VALUES('default','channel-1','38845','tvscraper','/tmp/art.jpg',1280,720,100,'old');"
        "INSERT INTO epg_scraper_metadata_cache VALUES('default','channel-1','38845','{\"available\":true,\"preferredArtwork\":{\"url\":\"/api/epg/cache/metadata/image?backend=default&channelId=channel-1&eventId=38845&kind=preferred&index=0\"}}',100,'old');"
        "INSERT INTO epg_scraper_metadata_images VALUES('default','channel-1','38845','preferred',0,'tvscraper','/tmp/art.jpg',1280,720,100,'old');"
        "INSERT INTO suite_metadata_targets VALUES('target-old','active','old');"
        "INSERT INTO suite_metadata_target_bindings VALUES('target-old','program-event','default','channel-1','38845','active','old');"
        "INSERT INTO suite_metadata_genre_assignments VALUES('target-old','active','old');"));

    const VdrEvent current = make_event(
        "39568", "channel-1", "The Great Wall", "1784924162", "1784930892");
    assert(repository.upsertEventsForBackend("default", {current}));

    const std::vector<VdrEvent> events = repository.findWindowForBackend(
        "default", "channel-1", "1784924000", "1784931000", 10);
    assert(events.size() == 1);
    assert(events.front().id == "39568");
    assert(repository.countForBackend("default") == 1);

    assert(scalar_int(database,
        "SELECT COUNT(*) FROM epg_event_artwork WHERE event_id='38845';") == 0);
    assert(scalar_int(database,
        "SELECT COUNT(*) FROM epg_event_artwork WHERE event_id='39568';") == 1);
    assert(scalar_int(database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_images WHERE event_id='38845';") == 0);
    assert(scalar_int(database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_images WHERE event_id='39568';") == 1);
    assert(scalar_text(database,
        "SELECT public_json FROM epg_scraper_metadata_cache WHERE event_id='39568';")
        .find("eventId=39568") != std::string::npos);
    assert(scalar_text(database,
        "SELECT lifecycle_state FROM suite_metadata_target_bindings WHERE metadata_target_id='target-old';") == "retired");
    assert(scalar_text(database,
        "SELECT lifecycle_state FROM suite_metadata_targets WHERE metadata_target_id='target-old';") == "retired");
    assert(scalar_text(database,
        "SELECT assignment_state FROM suite_metadata_genre_assignments WHERE metadata_target_id='target-old';") == "stale");

    const VdrEvent laterRepeat = make_event(
        "later", "channel-1", "The Great Wall", "1784935000", "1784941000");
    assert(repository.upsertEventsForBackend("default", {laterRepeat}));
    assert(repository.countForBackend("default") == 2);
}

static void test_window_and_cleanup_are_backend_scoped()
''')
replace_once(
    repository_test,
    "    test_upsert_updates_only_matching_backend();\n    test_window_and_cleanup_are_backend_scoped();\n",
    "    test_upsert_updates_only_matching_backend();\n"
    "    test_changed_event_ids_supersede_old_cache_identity();\n"
    "    test_window_and_cleanup_are_backend_scoped();\n")

frontend = ROOT / "web/frontend/epg-metadata-detail.js"
replace_once(
    frontend,
    "  const requestCache = new Map();\n",
    "  const requestCache = new Map();\n"
    "  const pendingRetryDelaysMs = Object.freeze([250, 500, 750, 1000, 1500, 2000]);\n")
replace_once(
    frontend,
    r'''  function fetchMetadata(backendId, channelId, nativeEventId) {
    const key = backendId + '\n' + channelId + '\n' + nativeEventId;
    if (requestCache.has(key)) return requestCache.get(key);

    const client = global.VdrSuiteClientApi;
    if (!client || typeof client.requestJson !== 'function') {
      return Promise.reject(new Error('EPG-Metadaten-Client ist nicht verfügbar.'));
    }

    const request = client.requestJson('/api/epg/cache/metadata', {
      query: {backend: backendId, channelId: channelId, eventId: nativeEventId},
      cache: 'no-store',
      credentials: 'same-origin'
    }).catch(function (error) {
      requestCache.delete(key);
      throw error;
    });

    requestCache.set(key, request);
    return request;
  }
''',
    r'''  function fetchMetadata(backendId, channelId, nativeEventId, attempt) {
    const key = backendId + '\n' + channelId + '\n' + nativeEventId;
    const retryAttempt = Number.isInteger(attempt) && attempt >= 0 ? attempt : 0;
    if (requestCache.has(key)) return requestCache.get(key);

    const client = global.VdrSuiteClientApi;
    if (!client || typeof client.requestJson !== 'function') {
      return Promise.reject(new Error('EPG-Metadaten-Client ist nicht verfügbar.'));
    }

    const request = client.requestJson('/api/epg/cache/metadata', {
      query: {backend: backendId, channelId: channelId, eventId: nativeEventId},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function (metadata) {
      if (metadata && metadata.available !== true && metadata.status === 'pending') {
        requestCache.delete(key);
        if (retryAttempt < pendingRetryDelaysMs.length &&
            typeof global.setTimeout === 'function') {
          return new Promise(function (resolve) {
            global.setTimeout(resolve, pendingRetryDelaysMs[retryAttempt]);
          }).then(function () {
            return fetchMetadata(
              backendId,
              channelId,
              nativeEventId,
              retryAttempt + 1
            );
          });
        }
      }
      return metadata;
    }).catch(function (error) {
      requestCache.delete(key);
      throw error;
    });

    requestCache.set(key, request);
    return request;
  }
''')
replace_once(
    frontend,
    "      if (!metadata || metadata.available !== true) {\n"
    "        status.textContent = 'Für diese Sendung sind keine erweiterten TVScraper-Daten verfügbar.';\n"
    "        return;\n"
    "      }\n",
    "      if (!metadata || metadata.available !== true) {\n"
    "        status.textContent = metadata && metadata.status === 'pending'\n"
    "          ? 'TVScraper-Metadaten sind noch nicht materialisiert. Bitte die Sendung erneut öffnen.'\n"
    "          : 'Für diese Sendung sind keine erweiterten TVScraper-Daten verfügbar.';\n"
    "        return;\n"
    "      }\n")

frontend_test = ROOT / "web/frontend/tests/test_epg_metadata_detail.js"
replace_once(
    frontend_test,
    "let metadataRequest = null;\nlet recordingSearch = null;\n",
    "let metadataRequest = null;\nlet metadataRequestCount = 0;\nlet recordingSearch = null;\n")
replace_once(
    frontend_test,
    "    requestJson(pathName, options) {\n"
    "      metadataRequest = {pathName, options};\n"
    "      return Promise.resolve(metadata);\n"
    "    },\n",
    "    requestJson(pathName, options) {\n"
    "      metadataRequest = {pathName, options};\n"
    "      metadataRequestCount += 1;\n"
    "      return Promise.resolve(metadataRequestCount < 3\n"
    "        ? {available: false, status: 'pending'}\n"
    "        : metadata);\n"
    "    },\n")
replace_once(
    frontend_test,
    "const window = {\n  VdrSuitePlatform: {\n",
    "const window = {\n"
    "  setTimeout(callback) {\n"
    "    callback();\n"
    "    return 1;\n"
    "  },\n"
    "  VdrSuitePlatform: {\n")
replace_once(
    frontend_test,
    "  await Promise.resolve();\n  await Promise.resolve();\n\n  assert.ok(metadataRequest);\n",
    "  for (let index = 0; index < 12; index += 1) {\n"
    "    await Promise.resolve();\n"
    "  }\n\n"
    "  assert.ok(metadataRequest);\n"
    "  assert.strictEqual(metadataRequestCount, 3);\n")

print("Phase 61 EPG event identity and pending metadata fixes applied.")
