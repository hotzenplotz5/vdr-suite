bool executeGenreSchema(Database& database)
{
    if (!executeGenreSchemaV2(database)) return false;
    return database.execute(
        "DELETE FROM suite_metadata_genre_assignments "
        "WHERE source_kind='epg-browse-content-class' "
        "AND NOT EXISTS(SELECT 1 FROM suite_metadata_schema_versions WHERE version=7);"
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(7,'Live-aligned fused DVB and TVScraper EPG browse classification');");
}

bool insertLiveParityEpgBrowseClassification(
    sqlite3* database,
    const std::string& targetId,
    const std::string& backendId,
    const std::string& contentClass,
    double confidence,
    std::int64_t observedAt)
{
    static const std::string providerId = "suite-epg-browse";
    CanonicalGenreRegistry registry;
    const CanonicalGenre genre = registry.find(contentClass);
    if (!genre.known ||
        !insertProvider(database, providerId) ||
        !upsertGenre(database, genre))
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO suite_metadata_genre_assignments "
        "(metadata_target_id,backend_id,target_type,genre_id,provider_id,original_value,"
        "normalization_rule,source_kind,assignment_state,confidence,ordinal,observed_at,updated_at) "
        "VALUES (?,?, 'program-event',?,?,?,'epg-browse-taxonomy-v3',"
        "'epg-browse-content-class','active',?,0,?,CURRENT_TIMESTAMP);";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }
    bindText(statement, 1, targetId);
    bindText(statement, 2, backendId);
    bindText(statement, 3, contentClass);
    bindText(statement, 4, providerId);
    bindText(statement, 5, contentClass);
    sqlite3_bind_double(statement, 6, confidence);
    sqlite3_bind_int64(
        statement,
        7,
        observedAt > 0 ? observedAt : nowEpochSeconds());
    const bool ok = stepDone(statement);
    sqlite3_finalize(statement);
    return ok;
}

bool reconcileEpgBrowseClassificationLocked(
    sqlite3* database,
    const std::string& targetId)
{
    sqlite3_stmt* binding = nullptr;
    const char* bindingSql =
        "SELECT backend_id,start_time,end_time "
        "FROM suite_metadata_target_bindings "
        "WHERE metadata_target_id=? AND target_type='program-event' "
        "AND lifecycle_state='active' LIMIT 1;";
    if (sqlite3_prepare_v2(database, bindingSql, -1, &binding, nullptr) !=
        SQLITE_OK)
    {
        return false;
    }
    bindText(binding, 1, targetId);
    const bool activeTarget = sqlite3_step(binding) == SQLITE_ROW;
    const std::string backendId = activeTarget
        ? columnText(binding, 0)
        : std::string();
    const std::int64_t startTime = activeTarget
        ? sqlite3_column_int64(binding, 1)
        : 0;
    const std::int64_t endTime = activeTarget
        ? sqlite3_column_int64(binding, 2)
        : 0;
    sqlite3_finalize(binding);

    if (!removeEpgBrowseClassification(database, targetId)) return false;
    if (!activeTarget) return true;

    bool mediaMovie = false;
    bool mediaSeries = false;
    bool scraperDocumentary = false;
    bool scraperSports = false;
    bool dvbMovie = false;
    bool dvbSeries = false;
    bool dvbDocumentary = false;
    bool dvbSports = false;
    bool dvbNews = false;
    bool dvbSpecificFilmGenre = false;
    std::int64_t observedAt = 0;

    sqlite3_stmt* evidence = nullptr;
    const char* evidenceSql =
        "SELECT provider_id,genre_id,source_kind,assignment_state,observed_at "
        "FROM suite_metadata_genre_assignments "
        "WHERE metadata_target_id=? "
        "AND source_kind<>'epg-browse-content-class' "
        "ORDER BY provider_id,ordinal,genre_id;";
    if (sqlite3_prepare_v2(database, evidenceSql, -1, &evidence, nullptr) !=
        SQLITE_OK)
    {
        return false;
    }
    bindText(evidence, 1, targetId);
    while (sqlite3_step(evidence) == SQLITE_ROW)
    {
        const std::string providerId = columnText(evidence, 0);
        const std::string genreId = columnText(evidence, 1);
        const std::string sourceKind = columnText(evidence, 2);
        const std::string state = columnText(evidence, 3);
        observedAt = std::max(
            observedAt,
            static_cast<std::int64_t>(sqlite3_column_int64(evidence, 4)));

        if (!isUsableBrowseEvidenceState(state)) continue;

        if (sourceKind == "scraper-media-type")
        {
            mediaMovie = mediaMovie || genreId == "movie";
            mediaSeries = mediaSeries || genreId == "series";
            continue;
        }

        if (providerId == "tvscraper" &&
            sourceKind == "scraper-metadata")
        {
            scraperDocumentary =
                scraperDocumentary || genreId == "documentary";
            scraperSports = scraperSports || genreId == "sports";
            continue;
        }

        if (providerId != "vdr-epg" ||
            sourceKind != "dvb-content-descriptor")
        {
            continue;
        }

        dvbMovie = dvbMovie || genreId == "movie";
        dvbSeries = dvbSeries || genreId == "series";
        dvbDocumentary = dvbDocumentary || genreId == "documentary";
        dvbSports = dvbSports || genreId == "sports";
        dvbNews = dvbNews || genreId == "news";
        dvbSpecificFilmGenre = dvbSpecificFilmGenre ||
            (genreId != "drama" && filmGenreIds().count(genreId) > 0);
    }
    sqlite3_finalize(evidence);

    if (dvbNews)
    {
        return true;
    }

    std::string contentClass;
    double confidence = 0.0;

    if (dvbSports || scraperSports)
    {
        contentClass = "sports";
        confidence = dvbSports ? 0.95 : 0.90;
    }
    else if (dvbDocumentary || scraperDocumentary)
    {
        contentClass = "documentary";
        confidence = dvbDocumentary ? 0.95 : 0.90;
    }
    else if (mediaMovie && mediaSeries)
    {
        return true;
    }
    else if (mediaSeries)
    {
        contentClass = "series";
        confidence = 0.99;
    }
    else if (mediaMovie)
    {
        contentClass = "movie";
        confidence = 0.99;
    }
    else if (dvbSeries)
    {
        contentClass = "series";
        confidence = 0.80;
    }
    else
    {
        constexpr std::int64_t FeatureFilmMinimumSeconds = 70 * 60;
        const std::int64_t durationSeconds = std::max<std::int64_t>(
            0,
            endTime - startTime);
        if (!dvbMovie ||
            (!dvbSpecificFilmGenre &&
             durationSeconds < FeatureFilmMinimumSeconds))
        {
            return true;
        }
        contentClass = "movie";
        confidence = dvbSpecificFilmGenre ? 0.85 : 0.75;
    }

    return insertLiveParityEpgBrowseClassification(
        database,
        targetId,
        backendId,
        contentClass,
        confidence,
        observedAt);
}
