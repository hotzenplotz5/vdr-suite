GenreOverview GenreIndexRepository::overview(
    const std::string& backendId,
    const std::string& targetType,
    std::int64_t fromTime,
    std::int64_t untilTime,
    const std::string& locale) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    GenreOverview result;
    result.backendId = normalizeBackendId(backendId);
    result.targetType = targetType;
    result.fromTime = fromTime;
    result.untilTime = untilTime;
    if (!ensureSchemaLocked() || !isValidTargetType(targetType)) return result;

    std::string timePredicate;
    if (targetType == "program-event")
    {
        timePredicate = " AND b.end_time>? AND b.start_time<? ";
    }

    const std::string totalsSql =
        "SELECT COUNT(DISTINCT b.metadata_target_id),COUNT(a.genre_id) "
        "FROM suite_metadata_target_bindings b LEFT JOIN suite_metadata_genre_assignments a "
        "ON a.metadata_target_id=b.metadata_target_id "
        "WHERE b.backend_id=? AND b.target_type=? AND b.lifecycle_state='active'" + timePredicate + ";";
    sqlite3_stmt* totals = nullptr;
    if (sqlite3_prepare_v2(database_.handle(), totalsSql.c_str(), -1, &totals, nullptr) == SQLITE_OK)
    {
        bindText(totals, 1, result.backendId);
        bindText(totals, 2, targetType);
        if (targetType == "program-event")
        {
            sqlite3_bind_int64(totals, 3, fromTime);
            sqlite3_bind_int64(totals, 4, untilTime);
        }
        if (sqlite3_step(totals) == SQLITE_ROW)
        {
            result.distinctItemCount = sqlite3_column_int(totals, 0);
            result.assignmentCount = sqlite3_column_int(totals, 1);
        }
    }
    sqlite3_finalize(totals);

    const std::string sql =
        "SELECT g.genre_id,g.label_de,g.label_en,g.known,COUNT(DISTINCT b.metadata_target_id),"
        "COUNT(DISTINCT CASE WHEN a.assignment_state IN('active','unknown','missing') THEN b.metadata_target_id END),"
        "COUNT(DISTINCT CASE WHEN a.assignment_state='stale' THEN b.metadata_target_id END),"
        "COUNT(DISTINCT CASE WHEN a.assignment_state='conflict' THEN b.metadata_target_id END),"
        "GROUP_CONCAT(DISTINCT a.source_kind) "
        "FROM suite_metadata_genre_assignments a "
        "JOIN suite_metadata_target_bindings b ON b.metadata_target_id=a.metadata_target_id "
        "JOIN suite_metadata_genres g ON g.genre_id=a.genre_id "
        "WHERE b.backend_id=? AND b.target_type=? AND b.lifecycle_state='active'" + timePredicate +
        "GROUP BY g.genre_id,g.label_de,g.label_en,g.known "
        "ORDER BY CASE WHEN g.genre_id='unclassified' THEN 1 ELSE 0 END,g.label_de,g.genre_id;";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) return result;
    bindText(statement, 1, result.backendId);
    bindText(statement, 2, targetType);
    if (targetType == "program-event")
    {
        sqlite3_bind_int64(statement, 3, fromTime);
        sqlite3_bind_int64(statement, 4, untilTime);
    }
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        GenreOverviewEntry entry;
        entry.genreId = columnText(statement, 0);
        entry.labelDe = columnText(statement, 1);
        entry.labelEn = columnText(statement, 2);
        entry.label = locale.rfind("en", 0) == 0 ? entry.labelEn : entry.labelDe;
        entry.known = sqlite3_column_int(statement, 3) != 0;
        entry.itemCount = sqlite3_column_int(statement, 4);
        entry.activeCount = sqlite3_column_int(statement, 5);
        entry.staleCount = sqlite3_column_int(statement, 6);
        entry.conflictCount = sqlite3_column_int(statement, 7);
        entry.sources = splitCommaList(columnText(statement, 8));
        result.genres.push_back(std::move(entry));
    }
    sqlite3_finalize(statement);
    return result;
}

GenreEpgBrowseOverview GenreIndexRepository::epgBrowseOverview(
    const std::string& backendId,
    std::int64_t fromTime,
    std::int64_t untilTime,
    const std::string& locale) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    GenreEpgBrowseOverview result;
    result.backendId = normalizeBackendId(backendId);
    result.fromTime = fromTime;
    result.untilTime = untilTime;
    if (!ensureSchemaLocked() || untilTime <= fromTime) return result;

    std::map<std::string, int> counts;
    sqlite3_stmt* countStatement = nullptr;
    const char* countSql =
        "SELECT a.genre_id,COUNT(DISTINCT b.metadata_target_id) "
        "FROM suite_metadata_genre_assignments a "
        "JOIN suite_metadata_target_bindings b ON b.metadata_target_id=a.metadata_target_id "
        "JOIN epg_events e ON e.backend_id=b.backend_id "
        "AND e.channel_id=b.channel_id AND e.event_id=b.native_id "
        "WHERE b.backend_id=? AND b.target_type='program-event' "
        "AND b.lifecycle_state='active' "
        "AND a.source_kind='epg-browse-content-class' "
        "AND a.assignment_state='active' "
        "AND b.end_time>? AND b.start_time<? "
        "GROUP BY a.genre_id;";
    if (sqlite3_prepare_v2(
            database_.handle(),
            countSql,
            -1,
            &countStatement,
            nullptr) == SQLITE_OK)
    {
        bindText(countStatement, 1, result.backendId);
        sqlite3_bind_int64(countStatement, 2, fromTime);
        sqlite3_bind_int64(countStatement, 3, untilTime);
        while (sqlite3_step(countStatement) == SQLITE_ROW)
        {
            counts[columnText(countStatement, 0)] =
                sqlite3_column_int(countStatement, 1);
        }
    }
    sqlite3_finalize(countStatement);

    const bool english = locale.rfind("en", 0) == 0;
    const std::vector<std::string> categoryIds = {
        "movie",
        "series",
        "documentary",
        "sports"
    };
    CanonicalGenreRegistry registry;
    for (const std::string& categoryId : categoryIds)
    {
        const CanonicalGenre genre = registry.find(categoryId);
        GenreBrowseCategory category;
        category.id = categoryId;
        category.labelDe = categoryId == "movie" ? "Film" : genre.labelDe;
        category.labelEn = genre.labelEn;
        category.label = english ? category.labelEn : category.labelDe;
        category.itemCount = counts[categoryId];
        result.distinctItemCount += category.itemCount;
        result.assignmentCount += category.itemCount;
        result.categories.push_back(std::move(category));
    }

    sqlite3_stmt* childStatement = nullptr;
    const char* childSql =
        "SELECT g.genre_id,g.label_de,g.label_en,"
        "COUNT(DISTINCT b.metadata_target_id) "
        "FROM suite_metadata_genre_assignments c "
        "JOIN suite_metadata_target_bindings b "
        "ON b.metadata_target_id=c.metadata_target_id "
        "JOIN epg_events e ON e.backend_id=b.backend_id "
        "AND e.channel_id=b.channel_id AND e.event_id=b.native_id "
        "JOIN suite_metadata_genre_assignments a "
        "ON a.metadata_target_id=b.metadata_target_id "
        "JOIN suite_metadata_genres g ON g.genre_id=a.genre_id "
        "WHERE b.backend_id=? AND b.target_type='program-event' "
        "AND b.lifecycle_state='active' "
        "AND c.source_kind='epg-browse-content-class' "
        "AND c.assignment_state='active' AND c.genre_id='movie' "
        "AND a.source_kind NOT IN("
        "'scraper-media-type','epg-browse-content-class') "
        "AND a.assignment_state IN('active','unknown','conflict') "
        "AND a.genre_id IN("
        "'action','adventure','animation','drama','family','fantasy',"
        "'history','horror','disaster','comedy','war','crime','musical',"
        "'mystery','romance','science-fiction','thriller','western') "
        "AND b.end_time>? AND b.start_time<? "
        "GROUP BY g.genre_id,g.label_de,g.label_en "
        "HAVING COUNT(DISTINCT b.metadata_target_id)>0 "
        "ORDER BY g.label_de,g.genre_id;";
    if (sqlite3_prepare_v2(
            database_.handle(),
            childSql,
            -1,
            &childStatement,
            nullptr) == SQLITE_OK)
    {
        bindText(childStatement, 1, result.backendId);
        sqlite3_bind_int64(childStatement, 2, fromTime);
        sqlite3_bind_int64(childStatement, 3, untilTime);
        while (sqlite3_step(childStatement) == SQLITE_ROW)
        {
            GenreBrowseCategory child;
            child.id = columnText(childStatement, 0);
            child.labelDe = columnText(childStatement, 1);
            child.labelEn = columnText(childStatement, 2);
            child.label = english ? child.labelEn : child.labelDe;
            child.itemCount = sqlite3_column_int(childStatement, 3);
            result.categories.front().children.push_back(std::move(child));
        }
    }
    sqlite3_finalize(childStatement);
    return result;
}

GenreRecordingPage GenreIndexRepository::recordingsByGenre(
    const std::string& backendId,
    const std::string& genreId,
    int limit,
    int offset) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    GenreRecordingPage page;
    page.backendId = normalizeBackendId(backendId);
    page.genreId = genreId;
    page.limit = boundedLimit(limit);
    page.offset = boundedOffset(offset);
    if (!ensureSchemaLocked()) return page;

    const char* countSql =
        "SELECT COUNT(DISTINCT b.metadata_target_id) FROM suite_metadata_genre_assignments a "
        "JOIN suite_metadata_target_bindings b ON b.metadata_target_id=a.metadata_target_id "
        "JOIN vdr_recording_cache c ON c.backend_id=b.backend_id AND c.cache_key=b.resource_key "
        "WHERE b.backend_id=? AND b.target_type='recording' AND b.lifecycle_state='active' AND a.genre_id=?;";
    sqlite3_stmt* count = nullptr;
    if (sqlite3_prepare_v2(database_.handle(), countSql, -1, &count, nullptr) == SQLITE_OK)
    {
        bindText(count, 1, page.backendId);
        bindText(count, 2, genreId);
        if (sqlite3_step(count) == SQLITE_ROW) page.totalCount = sqlite3_column_int(count, 0);
    }
    sqlite3_finalize(count);

    const char* sql =
        "SELECT c.recording_id,c.backend_id,c.backend_native_id,c.title,c.path,c.start_time,c.duration_seconds,c.size_mb,c.metadata_payload,"
        "(SELECT GROUP_CONCAT(DISTINCT a2.genre_id) FROM suite_metadata_genre_assignments a2 WHERE a2.metadata_target_id=b.metadata_target_id) "
        "FROM suite_metadata_genre_assignments a "
        "JOIN suite_metadata_target_bindings b ON b.metadata_target_id=a.metadata_target_id "
        "JOIN vdr_recording_cache c ON c.backend_id=b.backend_id AND c.cache_key=b.resource_key "
        "WHERE b.backend_id=? AND b.target_type='recording' AND b.lifecycle_state='active' AND a.genre_id=? "
        "GROUP BY b.metadata_target_id ORDER BY CAST(c.start_time AS INTEGER) DESC,c.title LIMIT ? OFFSET ?;";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) return page;
    bindText(statement, 1, page.backendId);
    bindText(statement, 2, genreId);
    sqlite3_bind_int(statement, 3, page.limit);
    sqlite3_bind_int(statement, 4, page.offset);
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        GenreRecordingItem item;
        item.id = columnText(statement, 0);
        item.backendId = columnText(statement, 1);
        item.backendNativeId = columnText(statement, 2);
        item.title = columnText(statement, 3);
        item.path = columnText(statement, 4);
        item.startTime = columnText(statement, 5);
        item.durationSeconds = sqlite3_column_int(statement, 6);
        item.sizeMb = sqlite3_column_int64(statement, 7);
        item.metadataPayload = columnText(statement, 8);
        item.genreIds = splitCommaList(columnText(statement, 9));
        page.recordings.push_back(std::move(item));
    }
    sqlite3_finalize(statement);
    return page;
}

GenreEpgPage GenreIndexRepository::epgByGenre(
    const std::string& backendId,
    const std::string& genreId,
    std::int64_t fromTime,
    std::int64_t untilTime,
    int limit,
    int offset) const
{
    if (isEpgBrowseContentClass(genreId))
    {
        return epgByBrowse(
            backendId,
            genreId,
            "",
            fromTime,
            untilTime,
            limit,
            offset);
    }
    if (isFilmGenre(genreId))
    {
        return epgByBrowse(
            backendId,
            "movie",
            genreId,
            fromTime,
            untilTime,
            limit,
            offset);
    }

    GenreEpgPage page;
    page.backendId = normalizeBackendId(backendId);
    page.genreId = genreId;
    page.fromTime = fromTime;
    page.untilTime = untilTime;
    page.limit = boundedLimit(limit);
    page.offset = boundedOffset(offset);
    return page;
}

GenreEpgPage GenreIndexRepository::epgByBrowse(
    const std::string& backendId,
    const std::string& contentClass,
    const std::string& genreId,
    std::int64_t fromTime,
    std::int64_t untilTime,
    int limit,
    int offset) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    GenreEpgPage page;
    page.backendId = normalizeBackendId(backendId);
    page.contentClass = contentClass;
    page.genreId = genreId;
    page.fromTime = fromTime;
    page.untilTime = untilTime;
    page.limit = boundedLimit(limit);
    page.offset = boundedOffset(offset);
    if (!ensureSchemaLocked() ||
        untilTime <= fromTime ||
        !isEpgBrowseContentClass(contentClass) ||
        (!genreId.empty() &&
         (contentClass != "movie" || !isFilmGenre(genreId))))
    {
        return page;
    }

    const std::string genrePredicate = genreId.empty()
        ? std::string()
        : " AND EXISTS("
          "SELECT 1 FROM suite_metadata_genre_assignments g "
          "WHERE g.metadata_target_id=b.metadata_target_id "
          "AND g.genre_id=? "
          "AND g.source_kind NOT IN("
          "'scraper-media-type','epg-browse-content-class') "
          "AND g.assignment_state IN('active','unknown','conflict')) ";

    const std::string countSql =
        "SELECT COUNT(DISTINCT b.metadata_target_id) "
        "FROM suite_metadata_genre_assignments c "
        "JOIN suite_metadata_target_bindings b "
        "ON b.metadata_target_id=c.metadata_target_id "
        "JOIN epg_events e ON e.backend_id=b.backend_id "
        "AND e.channel_id=b.channel_id AND e.event_id=b.native_id "
        "WHERE b.backend_id=? AND b.target_type='program-event' "
        "AND b.lifecycle_state='active' "
        "AND c.source_kind='epg-browse-content-class' "
        "AND c.assignment_state='active' AND c.genre_id=? "
        "AND b.end_time>? AND b.start_time<? " +
        genrePredicate + ";";
    sqlite3_stmt* count = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            countSql.c_str(),
            -1,
            &count,
            nullptr) == SQLITE_OK)
    {
        bindText(count, 1, page.backendId);
        bindText(count, 2, contentClass);
        sqlite3_bind_int64(count, 3, fromTime);
        sqlite3_bind_int64(count, 4, untilTime);
        if (!genreId.empty()) bindText(count, 5, genreId);
        if (sqlite3_step(count) == SQLITE_ROW)
        {
            page.totalCount = sqlite3_column_int(count, 0);
        }
    }
    sqlite3_finalize(count);

    const std::string sql =
        "SELECT e.backend_id,e.channel_id,COALESCE(ch.name,''),"
        "e.event_id,e.title,e.subtitle,e.description,e.start_time,e.end_time,"
        "e.duration_seconds,"
        "CASE WHEN aw.path IS NOT NULL AND aw.path<>'' "
        "AND aw.width>0 AND aw.height>0 THEN 1 ELSE 0 END,"
        "COALESCE(aw.width,0),COALESCE(aw.height,0),"
        "(SELECT GROUP_CONCAT(DISTINCT a2.genre_id) "
        "FROM suite_metadata_genre_assignments a2 "
        "WHERE a2.metadata_target_id=b.metadata_target_id "
        "AND a2.source_kind NOT IN("
        "'scraper-media-type','epg-browse-content-class') "
        "AND a2.assignment_state IN('active','unknown','conflict')) "
        "FROM suite_metadata_genre_assignments c "
        "JOIN suite_metadata_target_bindings b "
        "ON b.metadata_target_id=c.metadata_target_id "
        "JOIN epg_events e ON e.backend_id=b.backend_id "
        "AND e.channel_id=b.channel_id AND e.event_id=b.native_id "
        "LEFT JOIN vdr_channel_cache ch "
        "ON ch.backend_id=e.backend_id AND ch.channel_id=e.channel_id "
        "LEFT JOIN epg_event_artwork aw "
        "ON aw.backend_id=e.backend_id AND aw.channel_id=e.channel_id "
        "AND aw.event_id=e.event_id "
        "WHERE b.backend_id=? AND b.target_type='program-event' "
        "AND b.lifecycle_state='active' "
        "AND c.source_kind='epg-browse-content-class' "
        "AND c.assignment_state='active' AND c.genre_id=? "
        "AND b.end_time>? AND b.start_time<? " +
        genrePredicate +
        "GROUP BY b.metadata_target_id "
        "ORDER BY b.start_time,e.channel_id,e.event_id LIMIT ? OFFSET ?;";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return page;
    }

    int binding = 1;
    bindText(statement, binding++, page.backendId);
    bindText(statement, binding++, contentClass);
    sqlite3_bind_int64(statement, binding++, fromTime);
    sqlite3_bind_int64(statement, binding++, untilTime);
    if (!genreId.empty()) bindText(statement, binding++, genreId);
    sqlite3_bind_int(statement, binding++, page.limit);
    sqlite3_bind_int(statement, binding, page.offset);

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        GenreEpgItem item;
        item.backendId = columnText(statement, 0);
        item.channelId = columnText(statement, 1);
        item.channelName = columnText(statement, 2);
        item.eventId = columnText(statement, 3);
        item.title = columnText(statement, 4);
        item.subtitle = columnText(statement, 5);
        item.description = columnText(statement, 6);
        item.contentClass = contentClass;
        item.startTime = std::stoll(
            columnText(statement, 7).empty()
                ? "0"
                : columnText(statement, 7));
        item.endTime = std::stoll(
            columnText(statement, 8).empty()
                ? "0"
                : columnText(statement, 8));
        item.durationSeconds = sqlite3_column_int(statement, 9);
        item.artworkAvailable = sqlite3_column_int(statement, 10) != 0;
        item.artworkWidth = sqlite3_column_int(statement, 11);
        item.artworkHeight = sqlite3_column_int(statement, 12);
        item.genreIds = splitCommaList(columnText(statement, 13));
        page.events.push_back(std::move(item));
    }
    sqlite3_finalize(statement);
    return page;
}

std::vector<GenreEpgRefreshCandidate> GenreIndexRepository::epgRefreshCandidates(
    const std::string& backendId,
    std::int64_t fromTime,
    std::int64_t untilTime,
    const std::string& providerId,
    std::int64_t freshAfterEpoch,
    int limit) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<GenreEpgRefreshCandidate> candidates;
    if (!ensureSchemaLocked() || untilTime <= fromTime) return candidates;

    const int boundedCandidateLimit = std::max(1, std::min(limit, 64));
    const char* sql =
        "SELECT e.backend_id,e.channel_id,e.event_id,e.title,e.subtitle,e.description,e.start_time,e.end_time,e.duration_seconds,e.content_descriptors "
        "FROM epg_events e WHERE e.backend_id=? AND CAST(e.end_time AS INTEGER)>? AND CAST(e.start_time AS INTEGER)<? "
        "AND NOT EXISTS(SELECT 1 FROM suite_metadata_target_bindings b "
        "JOIN suite_metadata_genre_assignments a ON a.metadata_target_id=b.metadata_target_id "
        "WHERE b.backend_id=e.backend_id AND b.target_type='program-event' "
        "AND b.channel_id=e.channel_id AND b.native_id=e.event_id AND a.provider_id=? AND a.observed_at>=?) "
        "ORDER BY CAST(e.start_time AS INTEGER),e.channel_id,e.event_id LIMIT ?;";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) return candidates;
    bindText(statement, 1, normalizeBackendId(backendId));
    sqlite3_bind_int64(statement, 2, fromTime);
    sqlite3_bind_int64(statement, 3, untilTime);
    bindText(statement, 4, providerIdFor(providerId, "vdr-epg"));
    sqlite3_bind_int64(statement, 5, freshAfterEpoch);
    sqlite3_bind_int(statement, 6, boundedCandidateLimit);
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        GenreEpgRefreshCandidate candidate;
        candidate.backendId = columnText(statement, 0);
        candidate.channelId = columnText(statement, 1);
        candidate.eventId = columnText(statement, 2);
        candidate.title = columnText(statement, 3);
        candidate.subtitle = columnText(statement, 4);
        candidate.description = columnText(statement, 5);
        candidate.startTime = columnText(statement, 6);
        candidate.endTime = columnText(statement, 7);
        candidate.durationSeconds = sqlite3_column_int(statement, 8);
        candidate.contentDescriptors = splitLines(columnText(statement, 9));
        candidates.push_back(std::move(candidate));
    }
    sqlite3_finalize(statement);
    return candidates;
}

bool GenreIndexRepository::genreExists(const std::string& genreId) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked()) return false;
    sqlite3_stmt* statement = nullptr;
    const char* sql = "SELECT 1 FROM suite_metadata_genres WHERE genre_id=? LIMIT 1;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    bindText(statement, 1, genreId);
    const bool exists = sqlite3_step(statement) == SQLITE_ROW;
    sqlite3_finalize(statement);
    return exists;
}
