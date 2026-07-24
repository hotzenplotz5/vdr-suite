GenreIndexRepository::GenreIndexRepository(Database& database)
    : database_(database)
{
}

bool GenreIndexRepository::ensureSchema()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return ensureSchemaLocked();
}

bool GenreIndexRepository::ensureSchemaLocked() const
{
    if (schemaReady_)
    {
        return true;
    }

    if (!executeFoundationalSchema(database_) || !executeGenreSchema(database_)) return false;
    CanonicalGenreRegistry registry;
    for (const CanonicalGenre& genre : registry.allKnown())
    {
        if (!upsertGenre(database_.handle(), genre)) return false;
    }

    schemaReady_ = true;
    return true;
}

std::string GenreIndexRepository::normalizeBackendId(const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

bool GenreIndexRepository::isValidTargetType(const std::string& targetType)
{
    return targetType == "recording" || targetType == "program-event";
}

bool GenreIndexRepository::isEpgBrowseContentClass(const std::string& value)
{
    return value == "movie" ||
        value == "series" ||
        value == "documentary" ||
        value == "sports";
}

bool GenreIndexRepository::isFilmGenre(const std::string& value)
{
    return filmGenreIds().count(value) > 0;
}

std::string GenreIndexRepository::stableTargetId(
    const std::string& targetType,
    const std::string& backendId,
    const std::string& resourceKey)
{
    const std::string value = targetType + "\n" + normalizeBackendId(backendId) + "\n" + resourceKey;
    const std::uint64_t first = fnv1a64(value, 1469598103934665603ULL);
    const std::uint64_t second = fnv1a64(value, 1099511628211ULL ^ first);
    std::ostringstream output;
    output << "mdtgt_" << std::hex << std::setfill('0')
           << std::setw(16) << first << std::setw(16) << second;
    return output.str();
}

bool GenreIndexRepository::replaceEvidence(const GenreEvidenceInput& input)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked()) return false;
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;
    if (!replaceEvidenceLocked(input))
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

bool GenreIndexRepository::reconcileEpgBrowseClassification(
    const std::string& backendId,
    const std::string& resourceKey)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked() || resourceKey.empty()) return false;
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;
    const std::string targetId = stableTargetId(
        "program-event",
        normalizeBackendId(backendId),
        resourceKey);
    if (!reconcileEpgBrowseClassificationLocked(database_.handle(), targetId))
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

bool GenreIndexRepository::replaceEvidenceLocked(const GenreEvidenceInput& rawInput)
{
    GenreEvidenceInput input = rawInput;
    input.backendId = normalizeBackendId(input.backendId);
    if (!isValidTargetType(input.targetType) || input.resourceKey.empty()) return false;
    input.providerId = providerIdFor(input.providerId, input.targetType == "recording" ? "recording-native" : "vdr-epg");
    if (input.observedAt <= 0) input.observedAt = nowEpochSeconds();
    input.confidence = std::max(0.0, std::min(input.confidence, 1.0));

    const std::string targetId = stableTargetId(input.targetType, input.backendId, input.resourceKey);
    if (!insertProvider(database_.handle(), input.providerId) || !upsertTarget(database_.handle(), input, targetId)) return false;

    if (input.state == "stale" && input.originalValues.empty())
    {
        sqlite3_stmt* stale = nullptr;
        const char* staleSql = "UPDATE suite_metadata_genre_assignments SET assignment_state='stale',observed_at=?,updated_at=CURRENT_TIMESTAMP WHERE metadata_target_id=? AND provider_id=?;";
        if (sqlite3_prepare_v2(database_.handle(), staleSql, -1, &stale, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int64(stale, 1, input.observedAt);
        bindText(stale, 2, targetId);
        bindText(stale, 3, input.providerId);
        const bool staleOk = stepDone(stale);
        const int changedRows = sqlite3_changes(database_.handle());
        sqlite3_finalize(stale);
        if (!staleOk) return false;
        if (changedRows > 0) return reconcileGenreConflicts(database_.handle(), targetId);
    }

    sqlite3_stmt* remove = nullptr;
    const char* removeSql =
        "DELETE FROM suite_metadata_genre_assignments WHERE metadata_target_id=? AND provider_id=?;";
    if (sqlite3_prepare_v2(database_.handle(), removeSql, -1, &remove, nullptr) != SQLITE_OK) return false;
    bindText(remove, 1, targetId);
    bindText(remove, 2, input.providerId);
    const bool removed = stepDone(remove);
    sqlite3_finalize(remove);
    if (!removed) return false;

    CanonicalGenreRegistry registry;
    std::vector<std::string> values = input.originalValues;
    if (values.empty()) values.push_back("");

    std::set<std::pair<std::string, std::string>> seen;
    sqlite3_stmt* insert = nullptr;
    const char* insertSql =
        "INSERT INTO suite_metadata_genre_assignments "
        "(metadata_target_id,backend_id,target_type,genre_id,provider_id,original_value,normalization_rule,source_kind,assignment_state,confidence,ordinal,observed_at,updated_at) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,CURRENT_TIMESTAMP);";
    if (sqlite3_prepare_v2(database_.handle(), insertSql, -1, &insert, nullptr) != SQLITE_OK) return false;

    int ordinal = 0;
    for (const std::string& originalValue : values)
    {
        const std::vector<std::string> components =
            genreEvidenceComponents(input.sourceKind, originalValue);
        for (const std::string& component : components)
        {
            const CanonicalGenre genre = registry.classify(component);
            const bool missing =
                CanonicalGenreRegistry::normalizeAlias(component).empty();
            const std::string state =
                normalizeState(input.state, genre.known, missing);
            const std::pair<std::string, std::string> key(
                genre.id,
                originalValue);
            if (!seen.insert(key).second) continue;
            if (!upsertGenre(database_.handle(), genre))
            {
                sqlite3_finalize(insert);
                return false;
            }

            sqlite3_reset(insert);
            sqlite3_clear_bindings(insert);
            bindText(insert, 1, targetId);
            bindText(insert, 2, input.backendId);
            bindText(insert, 3, input.targetType);
            bindText(insert, 4, genre.id);
            bindText(insert, 5, input.providerId);
            bindText(insert, 6, originalValue);
            bindText(
                insert,
                7,
                input.sourceKind == "dvb-content-descriptor"
                    ? "dvb-content-components-v1"
                    : "canonical-alias-v1");
            bindText(insert, 8, input.sourceKind);
            bindText(insert, 9, state);
            sqlite3_bind_double(insert, 10, input.confidence);
            sqlite3_bind_int(insert, 11, ordinal++);
            sqlite3_bind_int64(insert, 12, input.observedAt);
            if (!stepDone(insert))
            {
                sqlite3_finalize(insert);
                return false;
            }
        }
    }
    sqlite3_finalize(insert);
    return reconcileGenreConflicts(database_.handle(), targetId);
}

bool GenreIndexRepository::markProviderEvidenceStale(
    const std::string& backendId,
    const std::string& targetType,
    const std::string& resourceKey,
    const std::string& providerId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked()) return false;
    const std::string targetId = stableTargetId(
        targetType,
        normalizeBackendId(backendId),
        resourceKey);
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE suite_metadata_genre_assignments SET assignment_state='stale',observed_at=?,updated_at=CURRENT_TIMESTAMP "
        "WHERE metadata_target_id=? AND provider_id=?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(statement, 1, nowEpochSeconds());
    bindText(statement, 2, targetId);
    bindText(statement, 3, providerIdFor(providerId, "vdr-epg"));
    const bool ok = stepDone(statement);
    sqlite3_finalize(statement);
    return ok && reconcileGenreConflicts(database_.handle(), targetId);
}

bool GenreIndexRepository::providerEvidenceNeedsRefresh(
    const std::string& backendId,
    const std::string& targetType,
    const std::string& resourceKey,
    const std::string& providerId,
    std::int64_t freshAfterEpoch) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked() || !isValidTargetType(targetType)) return true;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT MAX(observed_at),"
        "SUM(CASE WHEN assignment_state IN('active','unknown','conflict') THEN 1 ELSE 0 END) "
        "FROM suite_metadata_genre_assignments WHERE metadata_target_id=? AND provider_id=?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) return true;
    bindText(statement, 1, stableTargetId(targetType, normalizeBackendId(backendId), resourceKey));
    bindText(statement, 2, providerIdFor(providerId, "vdr-epg"));
    bool refresh = true;
    if (sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_type(statement, 0) != SQLITE_NULL)
    {
        const std::int64_t observedAt = sqlite3_column_int64(statement, 0);
        const int usableAssignments = sqlite3_column_int(statement, 1);
        refresh = usableAssignments <= 0 || observedAt < freshAfterEpoch;
    }
    sqlite3_finalize(statement);
    return refresh;
}

bool GenreIndexRepository::retireMissingRecordingTargets(const std::string& backendId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked()) return false;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE suite_metadata_target_bindings SET lifecycle_state='retired',updated_at=CURRENT_TIMESTAMP "
        "WHERE backend_id=? AND target_type='recording' AND NOT EXISTS("
        "SELECT 1 FROM vdr_recording_cache c WHERE c.backend_id=suite_metadata_target_bindings.backend_id "
        "AND c.cache_key=suite_metadata_target_bindings.resource_key);";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    bindText(statement, 1, normalizeBackendId(backendId));
    const bool ok = stepDone(statement);
    sqlite3_finalize(statement);
    return ok;
}

bool GenreIndexRepository::retireExpiredEpgTargets(
    const std::string& backendId,
    std::int64_t beforeEndTime)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked()) return false;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE suite_metadata_target_bindings SET lifecycle_state='retired',updated_at=CURRENT_TIMESTAMP "
        "WHERE backend_id=? AND target_type='program-event' AND end_time>0 AND end_time<?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    bindText(statement, 1, normalizeBackendId(backendId));
    sqlite3_bind_int64(statement, 2, beforeEndTime);
    const bool ok = stepDone(statement);
    sqlite3_finalize(statement);
    return ok;
}
