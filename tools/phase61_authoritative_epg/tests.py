from .common import replace_once

# ---------------------------------------------------------------------------
# Focused repository tests.
# ---------------------------------------------------------------------------

replace_once(
    "core/vdr/tests/test_epg_event_repository.cpp",
    '#include <cassert>\n',
    '#include <sqlite3.h>\n\n#include <cassert>\n'
)

repo_test = r'''
static int scalar_int(Database& database, const std::string& sql)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(
        database.handle(),
        sql.c_str(),
        -1,
        &statement,
        nullptr) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);
    const int value = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return value;
}

static void test_authoritative_window_removes_only_missing_native_ids()
{
    std::remove("/tmp/vdr-suite-epg-authoritative-window-test.db");

    Database database;
    assert(database.open(
        "/tmp/vdr-suite-epg-authoritative-window-test.db"));

    EpgEventRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.execute(
        "CREATE TABLE epg_event_artwork("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,provider TEXT,path TEXT,"
        "width INTEGER,height INTEGER,resolved_at INTEGER,updated_at TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_scraper_metadata_cache("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,public_json TEXT,"
        "resolved_at INTEGER,updated_at TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_scraper_metadata_images("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,kind TEXT,image_index INTEGER,"
        "provider TEXT,path TEXT,width INTEGER,height INTEGER,resolved_at INTEGER,"
        "updated_at TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id,kind,image_index));"
        "CREATE TABLE suite_metadata_targets("
        "metadata_target_id TEXT PRIMARY KEY,lifecycle_state TEXT,updated_at TEXT);"
        "CREATE TABLE suite_metadata_target_bindings("
        "metadata_target_id TEXT PRIMARY KEY,target_type TEXT,backend_id TEXT,"
        "resource_key TEXT,native_id TEXT,channel_id TEXT,start_time INTEGER,"
        "end_time INTEGER,lifecycle_state TEXT,updated_at TEXT);"
        "CREATE TABLE suite_metadata_genre_assignments("
        "metadata_target_id TEXT,assignment_state TEXT,updated_at TEXT);"));

    const VdrEvent oldEvent = make_event(
        "38845", "channel-1", "The Great Wall", "1000", "2000");
    const VdrEvent currentEvent = make_event(
        "39568", "channel-1", "The Great Wall", "1000", "2000");
    const VdrEvent outsideWindow = make_event(
        "outside", "channel-1", "Later", "3000", "4000");
    const VdrEvent otherChannel = make_event(
        "other", "channel-2", "Other", "1000", "2000");

    assert(repository.upsertEventsForBackend(
        "default",
        {oldEvent, outsideWindow, otherChannel}));
    assert(database.execute(
        "INSERT INTO epg_event_artwork VALUES("
        "'default','channel-1','38845','tvscraper','/tmp/old.jpg',1280,720,1,'old');"
        "INSERT INTO epg_scraper_metadata_cache VALUES("
        "'default','channel-1','38845','{}',1,'old');"
        "INSERT INTO epg_scraper_metadata_images VALUES("
        "'default','channel-1','38845','preferred',0,'tvscraper','/tmp/old.jpg',"
        "1280,720,1,'old');"
        "INSERT INTO suite_metadata_targets VALUES('target-old','active','old');"
        "INSERT INTO suite_metadata_target_bindings VALUES("
        "'target-old','program-event','default','channel-1\\n38845','38845',"
        "'channel-1',1000,2000,'active','old');"
        "INSERT INTO suite_metadata_genre_assignments VALUES("
        "'target-old','missing','old');"));

    const EpgAuthoritativeWindowResult result =
        repository.replaceAuthoritativeWindowForBackend(
            "default",
            "900",
            "2100",
            {"channel-1"},
            {currentEvent});

    assert(result.stored);
    assert(result.removedEvents.size() == 1);
    assert(result.removedEvents.front().eventId == "38845");
    assert(repository.containsEventForBackend(
        "default", "channel-1", "39568"));
    assert(!repository.containsEventForBackend(
        "default", "channel-1", "38845"));
    assert(repository.containsEventForBackend(
        "default", "channel-1", "outside"));
    assert(repository.containsEventForBackend(
        "default", "channel-2", "other"));
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM epg_event_artwork WHERE event_id='38845';") == 0);
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_cache WHERE event_id='38845';") == 0);
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_images WHERE event_id='38845';") == 0);
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM suite_metadata_target_bindings "
        "WHERE metadata_target_id='target-old' AND lifecycle_state='retired';") == 1);
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM suite_metadata_genre_assignments "
        "WHERE metadata_target_id='target-old' AND assignment_state='stale';") == 1);
}
'''

replace_once(
    "core/vdr/tests/test_epg_event_repository.cpp",
    'static void test_window_and_cleanup_are_backend_scoped()\n',
    repo_test + '\nstatic void test_window_and_cleanup_are_backend_scoped()\n'
)

replace_once(
    "core/vdr/tests/test_epg_event_repository.cpp",
    '''    test_upsert_updates_only_matching_backend();
    test_window_and_cleanup_are_backend_scoped();
''',
    '''    test_upsert_updates_only_matching_backend();
    test_authoritative_window_removes_only_missing_native_ids();
    test_window_and_cleanup_are_backend_scoped();
'''
)

# Cache service tests: authoritative vs capped/truncated.
cache_test = r'''
static void test_authoritative_refresh_reconciles_replaced_native_ids()
{
    std::remove("/tmp/vdr-suite-epg-cache-service-authoritative-test.db");

    Database database;
    assert(database.open(
        "/tmp/vdr-suite-epg-cache-service-authoritative-test.db"));

    EpgEventRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.upsertEventsForBackend(
        "default",
        {make_event("old-id", "channel-1", "Old", "1000", "2000")}));

    VdrEvent current = make_event(
        "current-id", "channel-1", "Current", "1000", "2000");
    MockEventAdapter adapter;
    adapter.events = {current};
    VdrService vdrService(adapter);
    EpgCacheService service(repository, vdrService);

    VdrEventQuery query;
    query.from = 900;
    query.timespan = 1200;
    query.channelEventLimit = 10;

    const EpgCacheRefreshResult result =
        service.refreshBackendWindow("default", query);

    assert(result.accepted);
    assert(result.authoritative);
    assert(result.stored);
    assert(result.removedEventCount == 1);
    assert(!service.containsEventForBackend(
        "default", "channel-1", "old-id"));
    assert(service.containsEventForBackend(
        "default", "channel-1", "current-id"));
}

static void test_truncated_channel_is_not_reconciled()
{
    std::remove("/tmp/vdr-suite-epg-cache-service-truncated-test.db");

    Database database;
    assert(database.open(
        "/tmp/vdr-suite-epg-cache-service-truncated-test.db"));

    EpgEventRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.upsertEventsForBackend(
        "default",
        {make_event("old-id", "channel-1", "Old", "1000", "2000")}));

    VdrEvent current = make_event(
        "current-id", "channel-1", "Current", "1000", "2000");
    MockEventAdapter adapter;
    adapter.events = {current};
    VdrService vdrService(adapter);
    EpgCacheService service(repository, vdrService);

    VdrEventQuery query;
    query.from = 900;
    query.timespan = 1200;
    query.channelEventLimit = 1;

    const EpgCacheRefreshResult result =
        service.refreshBackendWindow("default", query);

    assert(result.accepted);
    assert(result.authoritative);
    assert(result.stored);
    assert(result.removedEventCount == 0);
    assert(service.containsEventForBackend(
        "default", "channel-1", "old-id"));
    assert(service.containsEventForBackend(
        "default", "channel-1", "current-id"));
}
'''

replace_once(
    "core/vdr/tests/test_epg_cache_service.cpp",
    'int main()\n',
    cache_test + '\nint main()\n'
)

replace_once(
    "core/vdr/tests/test_epg_cache_service.cpp",
    '''    test_cache_reads_do_not_fetch_adapter_events();
    test_only_count_refresh_is_rejected();
''',
    '''    test_cache_reads_do_not_fetch_adapter_events();
    test_authoritative_refresh_reconciles_replaced_native_ids();
    test_truncated_channel_is_not_reconciled();
    test_only_count_refresh_is_rejected();
'''
)

# Frontend retry test.
replace_once(
    "web/frontend/tests/test_epg_metadata_detail.js",
    'let metadataRequest = null;\nlet recordingSearch = null;\n',
    'let metadataRequest = null;\n'
    'let metadataRequestCount = 0;\n'
    'const scheduledTimeouts = [];\n'
    'let recordingSearch = null;\n'
)

replace_once(
    "web/frontend/tests/test_epg_metadata_detail.js",
    '''    requestJson(pathName, options) {
      metadataRequest = {pathName, options};
      return Promise.resolve(metadata);
    },
''',
    '''    requestJson(pathName, options) {
      metadataRequest = {pathName, options};
      metadataRequestCount += 1;
      return Promise.resolve(metadataRequestCount < 3
        ? {available: false, status: 'pending'}
        : metadata);
    },
'''
)

replace_once(
    "web/frontend/tests/test_epg_metadata_detail.js",
    '''const window = {
  VdrSuitePlatform: {
''',
    '''const window = {
  setTimeout(callback) {
    scheduledTimeouts.push(callback);
    return scheduledTimeouts.length;
  },
  VdrSuitePlatform: {
'''
)

replace_once(
    "web/frontend/tests/test_epg_metadata_detail.js",
    '''  await Promise.resolve();
  await Promise.resolve();

  assert.ok(metadataRequest);
''',
    '''  for (let attempt = 0;
       attempt < 20 && detail.dataset.epgMetadataAvailable !== 'true';
       attempt += 1) {
    while (scheduledTimeouts.length) {
      scheduledTimeouts.shift()();
    }
    await Promise.resolve();
    await Promise.resolve();
  }

  assert.ok(metadataRequest);
  assert.strictEqual(metadataRequestCount, 3);
'''
)

