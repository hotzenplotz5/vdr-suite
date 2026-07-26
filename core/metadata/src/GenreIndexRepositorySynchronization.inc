namespace
{
std::string recordingFolderGenreCandidate(
    const std::string& title,
    const std::string& path)
{
    const auto firstFolder = [](const std::string& value) -> std::string
    {
        const std::size_t separator = value.find_first_of("/~");
        if (separator == std::string::npos || separator == 0) return {};
        return value.substr(0, separator);
    };

    std::string candidate = firstFolder(title);
    if (candidate.empty() && !path.empty() && path.front() != '/')
    {
        candidate = firstFolder(path);
    }
    if (candidate.empty()) return {};

    CanonicalGenreRegistry registry;
    const CanonicalGenre genre = registry.classify(candidate);
    if (!genre.known ||
        genre.id == "unclassified" ||
        genre.id == "movie")
    {
        return {};
    }
    return candidate;
}
}

bool GenreIndexRepository::synchronizeRecordingCache(const std::string& backendId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked()) return false;
    const std::string normalizedBackend = normalizeBackendId(backendId);
    const bool nativeMetadataAvailable =
        database_.tableExists("vdr_recording_native_metadata") &&
        database_.tableExists("vdr_recording_native_text_list");
    const char* sql = nativeMetadataAvailable
        ? "SELECT c.cache_key,c.backend_native_id,c.title,c.path,COALESCE(m.content_state,''),COALESCE(m.last_attempt_state,''),"
          "COALESCE(m.provider,''),COALESCE(l.value,'') "
          "FROM vdr_recording_cache c "
          "LEFT JOIN vdr_recording_native_metadata m ON m.backend_id=c.backend_id AND m.backend_native_id=c.backend_native_id "
          "LEFT JOIN vdr_recording_native_text_list l ON l.backend_id=m.backend_id AND l.recording_key=m.recording_key AND l.kind='genre' "
          "WHERE c.backend_id=? ORDER BY c.cache_key,l.ordinal;"
        : "SELECT c.cache_key,c.backend_native_id,c.title,c.path,'','','','' FROM vdr_recording_cache c "
          "WHERE c.backend_id=? ORDER BY c.cache_key;";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    bindText(statement, 1, normalizedBackend);

    struct Aggregate
    {
        std::string nativeId;
        std::string title;
        std::string path;
        std::string contentState;
        std::string attemptState;
        std::string provider;
        std::vector<std::string> genres;
    };
    std::map<std::string, Aggregate> rows;
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const std::string key = columnText(statement, 0);
        Aggregate& row = rows[key];
        row.nativeId = columnText(statement, 1);
        row.title = columnText(statement, 2);
        row.path = columnText(statement, 3);
        row.contentState = columnText(statement, 4);
        row.attemptState = columnText(statement, 5);
        row.provider = columnText(statement, 6);
        const std::string genre = columnText(statement, 7);
        if (!genre.empty()) row.genres.push_back(genre);
    }
    sqlite3_finalize(statement);

    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;

    sqlite3_stmt* clearRecordingEvidence = nullptr;
    const char* clearRecordingEvidenceSql =
        "DELETE FROM suite_metadata_genre_assignments "
        "WHERE metadata_target_id=? "
        "AND source_kind IN('recording-metadata','recording-folder-genre');";
    if (sqlite3_prepare_v2(
            database_.handle(),
            clearRecordingEvidenceSql,
            -1,
            &clearRecordingEvidence,
            nullptr) != SQLITE_OK)
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    for (const auto& entry : rows)
    {
        const std::string targetId = stableTargetId(
            "recording",
            normalizedBackend,
            entry.first);
        sqlite3_reset(clearRecordingEvidence);
        sqlite3_clear_bindings(clearRecordingEvidence);
        bindText(clearRecordingEvidence, 1, targetId);
        if (!stepDone(clearRecordingEvidence))
        {
            sqlite3_finalize(clearRecordingEvidence);
            database_.execute("ROLLBACK;");
            return false;
        }

        const std::string folderGenre = recordingFolderGenreCandidate(
            entry.second.title,
            entry.second.path);
        const bool useFolderGenre =
            entry.second.genres.empty() && !folderGenre.empty();

        GenreEvidenceInput input;
        input.backendId = normalizedBackend;
        input.targetType = "recording";
        input.resourceKey = entry.first;
        input.nativeId = entry.second.nativeId;
        input.providerId = useFolderGenre
            ? "recording-folder"
            : providerIdFor(entry.second.provider, "recording-native");
        input.sourceKind = useFolderGenre
            ? "recording-folder-genre"
            : "recording-metadata";
        input.originalValues = useFolderGenre
            ? std::vector<std::string>{folderGenre}
            : entry.second.genres;
        input.confidence = useFolderGenre
            ? 0.70
            : (entry.second.contentState == "found" ? 0.9 : 0.0);
        if (!useFolderGenre &&
            entry.second.attemptState == "failure" &&
            !entry.second.genres.empty())
        {
            input.state = "stale";
        }
        else
        {
            input.state = input.originalValues.empty() ? "missing" : "active";
        }
        if (!replaceEvidenceLocked(input))
        {
            sqlite3_finalize(clearRecordingEvidence);
            database_.execute("ROLLBACK;");
            return false;
        }
    }
    sqlite3_finalize(clearRecordingEvidence);

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
                if (!replaceEvidenceLocked(rows[index]) ||
                    !reconcileEpgBrowseClassificationLocked(
                        database_.handle(),
                        stableTargetId(
                            rows[index].targetType,
                            rows[index].backendId,
                            rows[index].resourceKey)))
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