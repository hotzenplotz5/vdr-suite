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

