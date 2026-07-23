bool GenreIndexRepository::synchronizeRecordingCache(const std::string& backendId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked()) return false;
    const std::string normalizedBackend = normalizeBackendId(backendId);
    const bool nativeMetadataAvailable =
        database_.tableExists("vdr_recording_native_metadata") &&
        database_.tableExists("vdr_recording_native_text_list");
    const char* sql = nativeMetadataAvailable
        ? "SELECT c.cache_key,c.backend_native_id,COALESCE(m.content_state,''),COALESCE(m.last_attempt_state,''),"
          "COALESCE(m.provider,''),COALESCE(l.value,'') "
          "FROM vdr_recording_cache c "
          "LEFT JOIN vdr_recording_native_metadata m ON m.backend_id=c.backend_id AND m.backend_native_id=c.backend_native_id "
          "LEFT JOIN vdr_recording_native_text_list l ON l.backend_id=m.backend_id AND l.recording_key=m.recording_key AND l.kind='genre' "
          "WHERE c.backend_id=? ORDER BY c.cache_key,l.ordinal;"
        : "SELECT c.cache_key,c.backend_native_id,'','','','' FROM vdr_recording_cache c "
          "WHERE c.backend_id=? ORDER BY c.cache_key;";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    bindText(statement, 1, normalizedBackend);

    struct Aggregate { std::string nativeId; std::string contentState; std::string attemptState; std::string provider; std::vector<std::string> genres; };
    std::map<std::string, Aggregate> rows;
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const std::string key = columnText(statement, 0);
        Aggregate& row = rows[key];
        row.nativeId = columnText(statement, 1);
        row.contentState = columnText(statement, 2);
        row.attemptState = columnText(statement, 3);
        row.provider = columnText(statement, 4);
        const std::string genre = columnText(statement, 5);
        if (!genre.empty()) row.genres.push_back(genre);
    }
    sqlite3_finalize(statement);

    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;
    for (const auto& entry : rows)
    {
        GenreEvidenceInput input;
        input.backendId = normalizedBackend;
        input.targetType = "recording";
        input.resourceKey = entry.first;
        input.nativeId = entry.second.nativeId;
        input.providerId = providerIdFor(entry.second.provider, "recording-native");
        input.sourceKind = "recording-metadata";
        input.originalValues = entry.second.genres;
        input.confidence = entry.second.contentState == "found" ? 0.9 : 0.0;
        if (entry.second.attemptState == "failure" && !entry.second.genres.empty()) input.state = "stale";
        else input.state = entry.second.genres.empty() ? "missing" : "active";
        if (!replaceEvidenceLocked(input))
        {
            database_.execute("ROLLBACK;");
            return false;
        }
    }
    if (!retireMissingRecordingTargets(normalizedBackend))
    {
        database_.execute("ROLLBACK;");
        return false;
    }
    if (!database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return false;
    }
    return true;
}

bool GenreIndexRepository::synchronizeEpgCache(
    const std::string& backendId,
    std::int64_t fromTime,
    std::int64_t untilTime)
{
    const std::string normalizedBackend = normalizeBackendId(backendId);
    std::vector<GenreEvidenceInput> rows;

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (!ensureSchemaLocked() || untilTime <= fromTime) return false;
        const char* sql =
            "SELECT channel_id,event_id,start_time,end_time,content_descriptors FROM epg_events "
            "WHERE backend_id=? AND CAST(end_time AS INTEGER)>? AND CAST(start_time AS INTEGER)<? "
            "ORDER BY CAST(start_time AS INTEGER),channel_id,event_id;";
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
        bindText(statement, 1, normalizedBackend);
        sqlite3_bind_int64(statement, 2, fromTime);
        sqlite3_bind_int64(statement, 3, untilTime);

        while (sqlite3_step(statement) == SQLITE_ROW)
        {
            GenreEvidenceInput input;
            input.backendId = normalizedBackend;
            input.targetType = "program-event";
            input.channelId = columnText(statement, 0);
            input.nativeId = columnText(statement, 1);
            input.resourceKey = input.channelId + "\n" + input.nativeId;
            input.startTime = std::stoll(columnText(statement, 2).empty() ? "0" : columnText(statement, 2));
            input.endTime = std::stoll(columnText(statement, 3).empty() ? "0" : columnText(statement, 3));
            input.providerId = "vdr-epg";
            input.sourceKind = "dvb-content-descriptor";
            input.originalValues = splitLines(columnText(statement, 4));
            input.state = input.originalValues.empty() ? "missing" : "active";
            input.confidence = input.originalValues.empty() ? 0.0 : 0.65;
            rows.push_back(std::move(input));
        }
        sqlite3_finalize(statement);
    }

    constexpr std::size_t EpgSynchronizationBatchSize = 16;
    for (std::size_t begin = 0; begin < rows.size(); begin += EpgSynchronizationBatchSize)
    {
        const std::size_t end = std::min(
            rows.size(),
            begin + EpgSynchronizationBatchSize);

        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;
            for (std::size_t index = begin; index < end; ++index)
            {
                if (!replaceEvidenceLocked(rows[index]))
                {
                    database_.execute("ROLLBACK;");
                    return false;
                }
            }
            if (!database_.execute("COMMIT;"))
            {
                database_.execute("ROLLBACK;");
                return false;
            }
        }

        if (end < rows.size())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;
        if (!retireExpiredEpgTargets(normalizedBackend, fromTime - 86400))
        {
            database_.execute("ROLLBACK;");
            return false;
        }
        if (!database_.execute("COMMIT;"))
        {
            database_.execute("ROLLBACK;");
            return false;
        }
    }
    return true;
}
