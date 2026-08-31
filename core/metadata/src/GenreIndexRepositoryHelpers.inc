std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr ? std::string() : reinterpret_cast<const char*>(value);
}

void bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT);
}

std::int64_t nowEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

int boundedLimit(int value)
{
    if (value <= 0) return 48;
    return std::min(value, 100);
}

int boundedOffset(int value)
{
    return std::max(value, 0);
}

std::vector<std::string> splitLines(const std::string& value)
{
    std::vector<std::string> result;
    std::istringstream input(value);
    std::string line;
    while (std::getline(input, line))
    {
        if (!line.empty()) result.push_back(line);
    }
    return result;
}

std::vector<std::string> splitCommaList(const std::string& value)
{
    std::vector<std::string> result;
    std::istringstream input(value);
    std::string item;
    while (std::getline(input, item, ','))
    {
        if (!item.empty()) result.push_back(item);
    }
    return result;
}

std::string providerIdFor(const std::string& value, const std::string& fallback)
{
    std::string normalized;
    bool separator = false;
    for (const char character : value)
    {
        const unsigned char byte = static_cast<unsigned char>(character);
        if ((byte >= 'a' && byte <= 'z') || (byte >= '0' && byte <= '9'))
        {
            normalized.push_back(static_cast<char>(byte));
            separator = false;
        }
        else if (byte >= 'A' && byte <= 'Z')
        {
            normalized.push_back(static_cast<char>(byte - 'A' + 'a'));
            separator = false;
        }
        else if (!normalized.empty() && !separator)
        {
            normalized.push_back('-');
            separator = true;
        }
    }
    while (!normalized.empty() && normalized.back() == '-') normalized.pop_back();
    if (normalized.empty()) normalized = fallback;
    if (normalized.size() > 64) normalized.resize(64);
    return normalized;
}

std::string normalizeState(const std::string& requested, bool known, bool missing)
{
    if (requested == "stale" || requested == "conflict") return requested;
    if (missing) return "missing";
    if (!known) return "unknown";
    return "active";
}

bool stepDone(sqlite3_stmt* statement)
{
    return sqlite3_step(statement) == SQLITE_DONE;
}

std::uint64_t fnv1a64(const std::string& value, std::uint64_t seed)
{
    std::uint64_t hash = seed;
    for (const unsigned char character : value)
    {
        hash ^= static_cast<std::uint64_t>(character);
        hash *= 1099511628211ULL;
    }
    return hash;
}

bool insertProvider(sqlite3* database, const std::string& providerId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO suite_metadata_providers (provider_id, provider_kind, display_name) "
        "VALUES (?, 'plugin', ?) "
        "ON CONFLICT(provider_id) DO UPDATE SET updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    bindText(statement, 1, providerId);
    bindText(statement, 2, providerId);
    const bool ok = stepDone(statement);
    sqlite3_finalize(statement);
    return ok;
}

bool upsertGenre(sqlite3* database, const CanonicalGenre& genre)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO suite_metadata_genres "
        "(genre_id,label_de,label_en,known,updated_at) VALUES (?,?,?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(genre_id) DO UPDATE SET "
        "label_de=excluded.label_de,label_en=excluded.label_en,known=excluded.known,updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    bindText(statement, 1, genre.id);
    bindText(statement, 2, genre.labelDe);
    bindText(statement, 3, genre.labelEn);
    sqlite3_bind_int(statement, 4, genre.known ? 1 : 0);
    const bool ok = stepDone(statement);
    sqlite3_finalize(statement);
    return ok;
}

bool upsertTarget(
    sqlite3* database,
    const GenreEvidenceInput& input,
    const std::string& targetId)
{
    sqlite3_stmt* target = nullptr;
    const char* targetSql =
        "INSERT INTO suite_metadata_targets "
        "(metadata_target_id,target_type,lifecycle_state,revision,updated_at) "
        "VALUES (?,?,'active',1,CURRENT_TIMESTAMP) "
        "ON CONFLICT(metadata_target_id) DO UPDATE SET "
        "target_type=excluded.target_type,lifecycle_state='active',revision=revision+1,updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(database, targetSql, -1, &target, nullptr) != SQLITE_OK) return false;
    bindText(target, 1, targetId);
    bindText(target, 2, input.targetType);
    const bool targetOk = stepDone(target);
    sqlite3_finalize(target);
    if (!targetOk) return false;

    sqlite3_stmt* binding = nullptr;
    const char* bindingSql =
        "INSERT INTO suite_metadata_target_bindings "
        "(metadata_target_id,target_type,backend_id,resource_key,native_id,channel_id,start_time,end_time,lifecycle_state,updated_at) "
        "VALUES (?,?,?,?,?,?,?,?,'active',CURRENT_TIMESTAMP) "
        "ON CONFLICT(target_type,backend_id,resource_key) DO UPDATE SET "
        "metadata_target_id=excluded.metadata_target_id,native_id=excluded.native_id,channel_id=excluded.channel_id,"
        "start_time=excluded.start_time,end_time=excluded.end_time,lifecycle_state='active',updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(database, bindingSql, -1, &binding, nullptr) != SQLITE_OK) return false;
    bindText(binding, 1, targetId);
    bindText(binding, 2, input.targetType);
    bindText(binding, 3, input.backendId);
    bindText(binding, 4, input.resourceKey);
    bindText(binding, 5, input.nativeId);
    bindText(binding, 6, input.channelId);
    sqlite3_bind_int64(binding, 7, input.startTime);
    sqlite3_bind_int64(binding, 8, input.endTime);
    const bool bindingOk = stepDone(binding);
    sqlite3_finalize(binding);
    return bindingOk;
}

bool isUsableBrowseEvidenceState(const std::string& state)
{
    return state == "active" || state == "unknown" || state == "conflict";
}

std::vector<std::string> genreEvidenceComponents(
    const std::string& sourceKind,
    const std::string& originalValue)
{
    if (sourceKind != "dvb-content-descriptor" ||
        originalValue.find('/') == std::string::npos)
    {
        return {originalValue};
    }

    std::vector<std::string> components;
    std::istringstream input(originalValue);
    std::string component;
    while (std::getline(input, component, '/'))
    {
        const std::size_t first = component.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        const std::size_t last = component.find_last_not_of(" \t\r\n");
        components.push_back(component.substr(first, last - first + 1));
    }
    return components.empty() ? std::vector<std::string>{originalValue} : components;
}

const std::set<std::string>& filmGenreIds()
{
    static const std::set<std::string> values = {
        "action",
        "adventure",
        "animation",
        "drama",
        "family",
        "fantasy",
        "history",
        "horror",
        "disaster",
        "comedy",
        "war",
        "crime",
        "musical",
        "mystery",
        "romance",
        "science-fiction",
        "thriller",
        "western"
    };
    return values;
}

bool reconcileGenreConflicts(
    sqlite3* database,
    const std::string& targetId)
{
    sqlite3_stmt* restore = nullptr;
    const char* restoreSql =
        "UPDATE suite_metadata_genre_assignments SET assignment_state="
        "CASE WHEN original_value='' THEN 'missing' "
        "WHEN genre_id LIKE 'unknown-%' THEN 'unknown' ELSE 'active' END "
        "WHERE metadata_target_id=? AND assignment_state='conflict';";
    if (sqlite3_prepare_v2(database, restoreSql, -1, &restore, nullptr) != SQLITE_OK) return false;
    bindText(restore, 1, targetId);
    const bool restored = stepDone(restore);
    sqlite3_finalize(restore);
    if (!restored) return false;

    std::map<std::string, std::set<std::string>> providerGenres;
    sqlite3_stmt* read = nullptr;
    const char* readSql =
        "SELECT provider_id,genre_id FROM suite_metadata_genre_assignments "
        "WHERE metadata_target_id=? AND assignment_state IN('active','unknown') "
        "AND genre_id<>'unclassified' "
        "AND source_kind NOT IN('scraper-media-type','epg-browse-content-class','recording-folder-genre') "
        "ORDER BY provider_id,genre_id;";
    if (sqlite3_prepare_v2(database, readSql, -1, &read, nullptr) != SQLITE_OK) return false;
    bindText(read, 1, targetId);
    while (sqlite3_step(read) == SQLITE_ROW)
    {
        providerGenres[columnText(read, 0)].insert(columnText(read, 1));
    }
    sqlite3_finalize(read);
    if (providerGenres.size() < 2) return true;

    std::set<std::string> conflictingProviders;
    for (const auto& provider : providerGenres)
    {
        bool sharesGenre = false;
        for (const auto& other : providerGenres)
        {
            if (provider.first == other.first) continue;
            for (const std::string& genreId : provider.second)
            {
                if (other.second.count(genreId) > 0)
                {
                    sharesGenre = true;
                    break;
                }
            }
            if (sharesGenre) break;
        }
        if (!sharesGenre) conflictingProviders.insert(provider.first);
    }

    if (conflictingProviders.empty()) return true;
    sqlite3_stmt* mark = nullptr;
    const char* markSql =
        "UPDATE suite_metadata_genre_assignments SET assignment_state='conflict',updated_at=CURRENT_TIMESTAMP "
        "WHERE metadata_target_id=? AND provider_id=? AND assignment_state IN('active','unknown') "
        "AND source_kind NOT IN('scraper-media-type','epg-browse-content-class','recording-folder-genre');";
    if (sqlite3_prepare_v2(database, markSql, -1, &mark, nullptr) != SQLITE_OK) return false;
    for (const std::string& providerId : conflictingProviders)
    {
        sqlite3_reset(mark);
        sqlite3_clear_bindings(mark);
        bindText(mark, 1, targetId);
        bindText(mark, 2, providerId);
        if (!stepDone(mark))
        {
            sqlite3_finalize(mark);
            return false;
        }
    }
    sqlite3_finalize(mark);
    return true;
}

bool removeEpgBrowseClassification(
    sqlite3* database,
    const std::string& targetId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM suite_metadata_genre_assignments "
        "WHERE metadata_target_id=? AND source_kind='epg-browse-content-class';";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    bindText(statement, 1, targetId);
    const bool ok = stepDone(statement);
    sqlite3_finalize(statement);
    return ok;
}

bool insertEpgBrowseClassification(
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
        "VALUES (?,?, 'program-event',?,?,?,'epg-browse-taxonomy-v2',"
        "'epg-browse-content-class','active',?,0,?,CURRENT_TIMESTAMP);";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    bindText(statement, 1, targetId);
    bindText(statement, 2, backendId);
    bindText(statement, 3, contentClass);
    bindText(statement, 4, providerId);
    bindText(statement, 5, contentClass);
    sqlite3_bind_double(statement, 6, confidence);
    sqlite3_bind_int64(statement, 7, observedAt > 0 ? observedAt : nowEpochSeconds());
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
        "SELECT backend_id FROM suite_metadata_target_bindings "
        "WHERE metadata_target_id=? AND target_type='program-event' "
        "AND lifecycle_state='active' LIMIT 1;";
    if (sqlite3_prepare_v2(database, bindingSql, -1, &binding, nullptr) != SQLITE_OK) return false;
    bindText(binding, 1, targetId);
    const bool activeTarget = sqlite3_step(binding) == SQLITE_ROW;
    const std::string backendId = activeTarget ? columnText(binding, 0) : std::string();
    sqlite3_finalize(binding);

    if (!removeEpgBrowseClassification(database, targetId)) return false;
    if (!activeTarget) return true;

    bool mediaMovie = false;
    bool mediaSeries = false;
    bool staleMediaType = false;
    bool scraperDocumentary = false;
    bool scraperSports = false;
    bool dvbDocumentary = false;
    bool dvbSports = false;
    std::int64_t observedAt = 0;

    sqlite3_stmt* evidence = nullptr;
    const char* evidenceSql =
        "SELECT provider_id,genre_id,source_kind,assignment_state,observed_at "
        "FROM suite_metadata_genre_assignments "
        "WHERE metadata_target_id=? AND source_kind<>'epg-browse-content-class' "
        "ORDER BY provider_id,ordinal,genre_id;";
    if (sqlite3_prepare_v2(database, evidenceSql, -1, &evidence, nullptr) != SQLITE_OK) return false;
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
                staleMediaType = true;
                continue;
            }
            if (!isUsableBrowseEvidenceState(state)) continue;
            mediaMovie = mediaMovie || genreId == "movie";
            mediaSeries = mediaSeries || genreId == "series";
            continue;
        }

        if (!isUsableBrowseEvidenceState(state)) continue;

        if (providerId == "tvscraper" && sourceKind == "scraper-metadata")
        {
            scraperDocumentary = scraperDocumentary || genreId == "documentary";
            scraperSports = scraperSports || genreId == "sports";
        }
        else if (providerId == "vdr-epg" && sourceKind == "dvb-content-descriptor")
        {
            dvbDocumentary = dvbDocumentary || genreId == "documentary";
            dvbSports = dvbSports || genreId == "sports";
        }
    }
    sqlite3_finalize(evidence);

    std::string contentClass;
    double confidence = 0.0;

    if (mediaMovie != mediaSeries)
    {
        if (mediaSeries)
        {
            contentClass = "series";
            confidence = 0.99;
        }
        else if (scraperDocumentary != scraperSports)
        {
            contentClass = scraperDocumentary ? "documentary" : "sports";
            confidence = 0.98;
        }
        else if (scraperDocumentary && scraperSports)
        {
            return true;
        }
        else
        {
            contentClass = "movie";
            confidence = 0.99;
        }
    }
    else if (mediaMovie && mediaSeries)
    {
        return true;
    }
    else if (staleMediaType)
    {
        return true;
    }
    else if (scraperDocumentary != scraperSports)
    {
        contentClass = scraperDocumentary ? "documentary" : "sports";
        confidence = 0.95;
    }
    else if (scraperDocumentary && scraperSports)
    {
        return true;
    }
    else
    {
        if (dvbDocumentary != dvbSports)
        {
            contentClass = dvbDocumentary ? "documentary" : "sports";
        }
        else
        {
            return true;
        }
        confidence = 0.65;
    }

    return insertEpgBrowseClassification(
        database,
        targetId,
        backendId,
        contentClass,
        confidence,
        observedAt);
}
