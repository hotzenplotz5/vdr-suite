from .common import replace_once

# ---------------------------------------------------------------------------
# Artwork and metadata persistence: reject writes for retired cache identities.
# ---------------------------------------------------------------------------

event_guard = r'''
bool eventCacheAllowsWrite(
    Database& database,
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId)
{
    if (!database.tableExists("epg_events"))
    {
        return true;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT 1 FROM epg_events "
        "WHERE backend_id=? AND channel_id=? AND event_id=? LIMIT 1;";
    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const std::string normalizedBackendId =
        backendId.empty() ? "default" : backendId;
    const bool bound =
        bindText(statement, 1, normalizedBackendId) &&
        bindText(statement, 2, channelId) &&
        bindText(statement, 3, eventId);
    const bool found = bound && sqlite3_step(statement) == SQLITE_ROW;
    sqlite3_finalize(statement);
    return found;
}
'''

replace_once(
    "core/vdr/src/EpgArtworkRepository.cpp",
    '''std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : std::string{};
}
}
''',
    '''std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : std::string{};
}
''' + event_guard + '''
}
'''
)

replace_once(
    "core/vdr/src/EpgArtworkRepository.cpp",
    '''    if (!artwork.valid() || !ensureSchemaLocked())
''',
    '''    if (!artwork.valid() ||
        !ensureSchemaLocked() ||
        !eventCacheAllowsWrite(
            database_,
            artwork.backendId,
            artwork.channelId,
            artwork.eventId))
'''
)

replace_once(
    "core/vdr/src/EpgArtworkRepository.cpp",
    '''    if (channelId.empty() || eventId.empty() || publicJson.empty() || !ensureSchemaLocked())
''',
    '''    if (channelId.empty() ||
        eventId.empty() ||
        publicJson.empty() ||
        !ensureSchemaLocked() ||
        !eventCacheAllowsWrite(database_, backendId, channelId, eventId))
'''
)

replace_once(
    "core/vdr/src/EpgArtworkRepository.cpp",
    '''    if (channelId.empty() || eventId.empty() || imageIndex < 0 ||
        (kind != "preferred" && kind != "person" && kind != "gallery") ||
        artwork.provider.empty() || artwork.path.empty() || artwork.width <= 0 ||
        artwork.height <= 0 || !ensureSchemaLocked())
''',
    '''    if (channelId.empty() || eventId.empty() || imageIndex < 0 ||
        (kind != "preferred" && kind != "person" && kind != "gallery") ||
        artwork.provider.empty() || artwork.path.empty() || artwork.width <= 0 ||
        artwork.height <= 0 || !ensureSchemaLocked() ||
        !eventCacheAllowsWrite(database_, backendId, channelId, eventId))
'''
)

