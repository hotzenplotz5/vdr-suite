GenreEpgArtworkSelection GenreIndexRepository::epgPortraitArtwork(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    GenreEpgArtworkSelection selection;
    if (!ensureSchemaLocked() ||
        channelId.empty() ||
        eventId.empty() ||
        !database_.tableExists("epg_scraper_metadata_images"))
    {
        return selection;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT kind,image_index,width,height "
        "FROM epg_scraper_metadata_images "
        "WHERE backend_id=? AND channel_id=? AND event_id=? "
        "AND kind IN('preferred','gallery') "
        "AND path<>'' AND width>0 AND height>width "
        "ORDER BY CASE WHEN kind='preferred' THEN 0 ELSE 1 END,image_index "
        "LIMIT 1;";
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return selection;
    }

    bindText(statement, 1, normalizeBackendId(backendId));
    bindText(statement, 2, channelId);
    bindText(statement, 3, eventId);
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        selection.kind = columnText(statement, 0);
        selection.imageIndex = sqlite3_column_int(statement, 1);
        selection.width = sqlite3_column_int(statement, 2);
        selection.height = sqlite3_column_int(statement, 3);
        selection.available =
            (selection.kind == "preferred" || selection.kind == "gallery") &&
            selection.imageIndex >= 0 &&
            selection.width > 0 &&
            selection.height > selection.width;
    }
    sqlite3_finalize(statement);
    return selection;
}
