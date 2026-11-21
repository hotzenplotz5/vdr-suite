from .common import replace_once

# Artwork persistence must not recreate retired cache identities.
artwork_guard_test = r'''
void testArtworkRejectsRetiredEventWhenEventCacheExists()
{
    const char* databasePath =
        "/tmp/vdr-suite-epg-artwork-repository-event-guard-test.db";
    std::remove(databasePath);

    Database database;
    assert(database.open(databasePath));

    EpgArtworkRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.execute(
        "CREATE TABLE epg_events("
        "backend_id TEXT NOT NULL,channel_id TEXT NOT NULL,event_id TEXT NOT NULL,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "INSERT INTO epg_events VALUES('default','channel-1','current');"));

    EpgArtworkReference current = makeArtwork(
        "default", "channel-1", "current", "/current.jpg");
    EpgArtworkReference stale = makeArtwork(
        "default", "channel-1", "stale", "/stale.jpg");

    assert(repository.upsert(current));
    assert(!repository.upsert(stale));
    assert(repository.upsertMetadataJson(
        "default", "channel-1", "current", "{}", 1));
    assert(!repository.upsertMetadataJson(
        "default", "channel-1", "stale", "{}", 1));
    assert(repository.upsertMetadataImage(
        "default", "channel-1", "current", "preferred", 0, current));
    assert(!repository.upsertMetadataImage(
        "default", "channel-1", "stale", "preferred", 0, stale));
}
'''

replace_once(
    "core/vdr/tests/test_epg_artwork_repository.cpp",
    '''}
}

int main()
''',
    '''}
''' + artwork_guard_test + '''
}

int main()
'''
)

replace_once(
    "core/vdr/tests/test_epg_artwork_repository.cpp",
    '''    testArtworkUpsertAndRemoval();
    return 0;
''',
    '''    testArtworkUpsertAndRemoval();
    testArtworkRejectsRetiredEventWhenEventCacheExists();
    return 0;
'''
)
