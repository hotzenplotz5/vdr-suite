bool executeGenreSchema(Database& database)
{
    if (!executeGenreSchemaV2(database)) return false;
    return database.execute(
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(7,'Live-aligned fused DVB and TVScraper EPG browse classification');"
        "DELETE FROM suite_metadata_genre_assignments "
        "WHERE source_kind='epg-browse-content-class' "
        "AND NOT EXISTS(SELECT 1 FROM suite_metadata_schema_versions WHERE version=8);"
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(8,'Live-aligned title guards and last-known-good TVScraper EPG types');"
        "DELETE FROM suite_metadata_genre_assignments "
        "WHERE source_kind='epg-browse-content-class' "
        "AND NOT EXISTS(SELECT 1 FROM suite_metadata_schema_versions WHERE version=9);"
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(9,'Strict specific-genre DVB movie fallback reconciliation');"
        "DELETE FROM suite_metadata_genre_assignments "
        "WHERE source_kind='epg-browse-content-class' "
        "AND NOT EXISTS(SELECT 1 FROM suite_metadata_schema_versions WHERE version=10);"
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(10,'Migrate epg-browse-taxonomy-v4 classifications to documentary-first v5');"
        "DELETE FROM suite_metadata_genre_assignments "
        "WHERE source_kind='epg-browse-content-class' "
        "AND NOT EXISTS(SELECT 1 FROM suite_metadata_schema_versions WHERE version=11);"
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(11,'Add generic Nachrichten title guard to epg-browse-taxonomy-v6');"
        "DELETE FROM suite_metadata_genre_assignments "
        "WHERE source_kind='epg-browse-content-class' "
        "AND NOT EXISTS(SELECT 1 FROM suite_metadata_schema_versions WHERE version=12);"
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) VALUES"
        "(12,'Migrate epg-browse-taxonomy-v6 to fiction-confirmed v7 series');");
}

std::string liveParityAsciiLower(const std::string& value)
{
    std::string normalized = value;
    for (char& character : normalized)
    {
        if (character >= 'A' && character <= 'Z')
        {
            character = static_cast<char>(character - 'A' + 'a');
        }
    }
    return normalized;
}

bool liveParityAsciiAlphaNumeric(char character)
{
    return (character >= 'a' && character <= 'z') ||
        (character >= '0' && character <= '9');
}

bool liveParityStartsWithBrand(
    const std::string& normalizedValue,
    const std::string& normalizedBrand)
{
    if (normalizedValue.rfind(normalizedBrand, 0) != 0) return false;
    return normalizedValue.size() == normalizedBrand.size() ||
        !liveParityAsciiAlphaNumeric(normalizedValue[normalizedBrand.size()]);
}

bool liveParityStrongNewsTitle(
    const std::string& title,
    const std::string& subtitle)
{
    std::string normalized = liveParityAsciiLower(title + " " + subtitle);
    const std::size_t first = normalized.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    const std::size_t last = normalized.find_last_not_of(" \t\r\n");
    normalized = normalized.substr(first, last - first + 1);

    static const std::vector<std::string> brands = {
        "nachrichten",
        "tagesschau",
        "tagesschau24",
        "tagesthemen",
        "heute journal",
        "heute-journal",
        "heute xpress",
        "zdf heute",
        "rtl aktuell",
        "sat.1 nachrichten",
        "newstime",
        "n-tv nachrichten",
        "ntv nachrichten",
        "welt nachrichten",
        "ard-brennpunkt",
        "brennpunkt"
    };
    if (normalized == "heute" || normalized.rfind("heute ", 0) == 0)
    {
        return normalized.rfind("heute-show", 0) != 0 &&
            normalized.rfind("heute show", 0) != 0;
    }
    for (const std::string& brand : brands)
    {
        if (liveParityStartsWithBrand(normalized, brand)) return true;
        if (liveParityStartsWithBrand(normalized, "die " + brand)) return true;
    }
    return false;
}

bool liveParityStrongSportsTitle(
    const std::string& title,
    const std::string& subtitle)
{
    const std::string normalized = liveParityAsciiLower(title + " " + subtitle);
    static const std::vector<std::string> brands = {
        "sportschau",
        "das aktuelle sportstudio",
        "sportstudio",
        "ran bundesliga",
        "ran nfl",
        "ran racing",
        "bundesliga pur",
        "sport im osten",
        "sportclub",
        "sportarena"
    };
    for (const std::string& brand : brands)
    {
        if (liveParityStartsWithBrand(normalized, brand)) return true;
    }
    return false;
}

bool liveParitySeriesFictionGenre(const std::string& genreId)
{
    static const std::vector<std::string> genres = {
        "action",
        "adventure",
        "animation",
        "comedy",
        "crime",
        "drama",
        "fantasy",
        "horror",
        "mystery",
        "romance",
        "science-fiction",
        "thriller",
        "war",
        "western"
    };
    return std::find(genres.begin(), genres.end(), genreId) != genres.end();
}

bool liveParitySeriesNonFictionGenre(const std::string& genreId)
{
    static const std::vector<std::string> genres = {
        "documentary",
        "news",
        "reality",
        "sports",
        "talk-show"
    };
    return std::find(genres.begin(), genres.end(), genreId) != genres.end();
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
        "VALUES (?,?, 'program-event',?,?,?,'epg-browse-taxonomy-v7',"
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
        "SELECT b.backend_id,COALESCE(e.title,''),COALESCE(e.subtitle,'') "
        "FROM suite_metadata_target_bindings b "
        "LEFT JOIN epg_events e ON e.backend_id=b.backend_id "
        "AND e.channel_id=b.channel_id AND e.event_id=b.native_id "
        "WHERE b.metadata_target_id=? AND b.target_type='program-event' "
        "AND b.lifecycle_state='active' LIMIT 1;";
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
    const std::string title = activeTarget
        ? columnText(binding, 1)
        : std::string();
    const std::string subtitle = activeTarget
        ? columnText(binding, 2)
        : std::string();
    sqlite3_finalize(binding);

    if (!removeEpgBrowseClassification(database, targetId)) return false;
    if (!activeTarget) return true;

    bool activeMediaMovie = false;
    bool activeMediaSeries = false;
    bool lastKnownMediaMovie = false;
    bool lastKnownMediaSeries = false;
    bool activeScraperDocumentary = false;
    bool activeScraperSports = false;
    bool lastKnownScraperDocumentary = false;
    bool lastKnownScraperSports = false;
    bool activeScraperFiction = false;
    bool lastKnownScraperFiction = false;
    bool activeScraperNonFiction = false;
    bool lastKnownScraperNonFiction = false;
    bool dvbMovie = false;
    bool dvbSeries = false;
    bool dvbDocumentary = false;
    bool dvbSports = false;
    bool dvbNews = false;
    bool dvbFiction = false;
    bool dvbNonFiction = false;
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

        if (sourceKind == "scraper-media-type")
        {
            if (state == "stale")
            {
                lastKnownMediaMovie = lastKnownMediaMovie || genreId == "movie";
                lastKnownMediaSeries = lastKnownMediaSeries || genreId == "series";
            }
            else if (isUsableBrowseEvidenceState(state))
            {
                activeMediaMovie = activeMediaMovie || genreId == "movie";
                activeMediaSeries = activeMediaSeries || genreId == "series";
            }
            continue;
        }

        if (providerId == "tvscraper" &&
            sourceKind == "scraper-metadata")
        {
            if (state == "stale")
            {
                lastKnownScraperDocumentary =
                    lastKnownScraperDocumentary || genreId == "documentary";
                lastKnownScraperSports =
                    lastKnownScraperSports || genreId == "sports";
                lastKnownScraperFiction =
                    lastKnownScraperFiction ||
                    liveParitySeriesFictionGenre(genreId);
                lastKnownScraperNonFiction =
                    lastKnownScraperNonFiction ||
                    liveParitySeriesNonFictionGenre(genreId);
            }
            else if (isUsableBrowseEvidenceState(state))
            {
                activeScraperDocumentary =
                    activeScraperDocumentary || genreId == "documentary";
                activeScraperSports =
                    activeScraperSports || genreId == "sports";
                activeScraperFiction =
                    activeScraperFiction ||
                    liveParitySeriesFictionGenre(genreId);
                activeScraperNonFiction =
                    activeScraperNonFiction ||
                    liveParitySeriesNonFictionGenre(genreId);
            }
            continue;
        }

        if (!isUsableBrowseEvidenceState(state) ||
            providerId != "vdr-epg" ||
            sourceKind != "dvb-content-descriptor")
        {
            continue;
        }

        dvbMovie = dvbMovie || genreId == "movie";
        dvbSeries = dvbSeries || genreId == "series";
        dvbDocumentary = dvbDocumentary || genreId == "documentary";
        dvbSports = dvbSports || genreId == "sports";
        dvbNews = dvbNews || genreId == "news";
        dvbFiction = dvbFiction || liveParitySeriesFictionGenre(genreId);
        dvbNonFiction =
            dvbNonFiction || liveParitySeriesNonFictionGenre(genreId);
        dvbSpecificFilmGenre = dvbSpecificFilmGenre ||
            (genreId != "drama" && filmGenreIds().count(genreId) > 0);
    }
    sqlite3_finalize(evidence);

    const bool strongNewsTitle = liveParityStrongNewsTitle(title, subtitle);
    const bool strongSportsTitle = liveParityStrongSportsTitle(title, subtitle);
    if (dvbNews || strongNewsTitle)
    {
        return true;
    }

    std::string contentClass;
    double confidence = 0.0;

    if (dvbSports || strongSportsTitle)
    {
        contentClass = "sports";
        confidence = dvbSports ? 0.95 : 0.90;
    }
    else if (dvbDocumentary)
    {
        contentClass = "documentary";
        confidence = 0.95;
    }
    else if (activeScraperSports || activeScraperDocumentary)
    {
        if (activeScraperSports == activeScraperDocumentary) return true;
        contentClass = activeScraperSports ? "sports" : "documentary";
        confidence = 0.95;
    }
    else if (activeScraperNonFiction || dvbNonFiction)
    {
        return true;
    }
    else if (activeMediaMovie || activeMediaSeries)
    {
        if (activeMediaMovie == activeMediaSeries) return true;
        if (activeMediaSeries)
        {
            if (!(activeScraperFiction || dvbFiction)) return true;
            contentClass = "series";
        }
        else
        {
            contentClass = "movie";
        }
        confidence = 0.99;
    }
    else if (lastKnownMediaMovie || lastKnownMediaSeries)
    {
        if (lastKnownMediaMovie == lastKnownMediaSeries) return true;
        if (lastKnownMediaSeries)
        {
            const bool scraperFiction =
                activeScraperFiction || lastKnownScraperFiction || dvbFiction;
            const bool scraperNonFiction =
                activeScraperNonFiction || lastKnownScraperNonFiction ||
                dvbNonFiction;
            if (!scraperFiction || scraperNonFiction) return true;
            contentClass = "series";
            confidence = 0.90;
        }
        else
        {
            const bool scraperSports = activeScraperSports || lastKnownScraperSports;
            const bool scraperDocumentary =
                activeScraperDocumentary || lastKnownScraperDocumentary;
            if (scraperSports || scraperDocumentary)
            {
                if (scraperSports == scraperDocumentary) return true;
                contentClass = scraperSports ? "sports" : "documentary";
                confidence = 0.90;
            }
            else
            {
                contentClass = "movie";
                confidence = 0.99;
            }
        }
    }
    else if (activeScraperSports || activeScraperDocumentary ||
             lastKnownScraperSports || lastKnownScraperDocumentary)
    {
        const bool scraperSports = activeScraperSports || lastKnownScraperSports;
        const bool scraperDocumentary =
            activeScraperDocumentary || lastKnownScraperDocumentary;
        if (scraperSports == scraperDocumentary) return true;
        contentClass = scraperSports ? "sports" : "documentary";
        confidence = 0.85;
    }
    else if (dvbSeries)
    {
        const bool fiction =
            activeScraperFiction || lastKnownScraperFiction || dvbFiction;
        const bool nonFiction =
            activeScraperNonFiction || lastKnownScraperNonFiction ||
            dvbNonFiction;
        if (!fiction || nonFiction) return true;
        contentClass = "series";
        confidence = 0.80;
    }
    else
    {
        if (!dvbMovie || !dvbSpecificFilmGenre)
        {
            return true;
        }
        contentClass = "movie";
        confidence = 0.85;
    }

    return insertLiveParityEpgBrowseClassification(
        database,
        targetId,
        backendId,
        contentClass,
        confidence,
        observedAt);
}
