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

struct RecordingAssignmentSignature
{
    std::string providerId;
    std::string genreId;
    std::string originalValue;
    std::string normalizationRule;
    std::string sourceKind;
    std::string assignmentState;
    double confidence = 0.0;
    int ordinal = 0;

    bool operator==(const RecordingAssignmentSignature& other) const
    {
        return providerId == other.providerId &&
            genreId == other.genreId &&
            originalValue == other.originalValue &&
            normalizationRule == other.normalizationRule &&
            sourceKind == other.sourceKind &&
            assignmentState == other.assignmentState &&
            confidence == other.confidence &&
            ordinal == other.ordinal;
    }
};

struct RecordingEvidenceSignature
{
    std::string nativeId;
    std::string lifecycleState;
    std::vector<RecordingAssignmentSignature> assignments;

    bool operator==(const RecordingEvidenceSignature& other) const
    {
        return nativeId == other.nativeId &&
            lifecycleState == other.lifecycleState &&
            assignments == other.assignments;
    }
};

RecordingEvidenceSignature desiredRecordingEvidenceSignature(
    const GenreEvidenceInput& rawInput)
{
    GenreEvidenceInput input = rawInput;
    input.providerId = providerIdFor(
        input.providerId,
        input.targetType == "recording"
            ? "recording-native"
            : "vdr-epg");
    input.confidence = std::max(
        0.0,
        std::min(input.confidence, 1.0));

    RecordingEvidenceSignature signature;
    signature.nativeId = input.nativeId;
    signature.lifecycleState = "active";

    CanonicalGenreRegistry registry;
    std::vector<std::string> values = input.originalValues;
    if (values.empty()) values.push_back("");

    std::set<std::pair<std::string, std::string>> seen;
    int ordinal = 0;

    for (const std::string& originalValue : values)
    {
        const std::vector<std::string> components =
            genreEvidenceComponents(
                input.sourceKind,
                originalValue);

        for (const std::string& component : components)
        {
            const CanonicalGenre genre =
                registry.classify(component);
            const bool missing =
                CanonicalGenreRegistry::normalizeAlias(
                    component).empty();
            const std::pair<std::string, std::string> key(
                genre.id,
                originalValue);

            if (!seen.insert(key).second) continue;

            RecordingAssignmentSignature assignment;
            assignment.providerId = input.providerId;
            assignment.genreId = genre.id;
            assignment.originalValue = originalValue;
            assignment.normalizationRule =
                input.sourceKind == "dvb-content-descriptor"
                    ? "dvb-content-components-v1"
                    : "canonical-alias-v1";
            assignment.sourceKind = input.sourceKind;
            assignment.assignmentState = normalizeState(
                input.state,
                genre.known,
                missing);
            assignment.confidence = input.confidence;
            assignment.ordinal = ordinal++;

            signature.assignments.push_back(
                std::move(assignment));
        }
    }

    return signature;
}

RecordingEvidenceSignature desiredRecordingEvidenceSignature(
    const std::vector<GenreEvidenceInput>& inputs)
{
    RecordingEvidenceSignature signature;

    for (const GenreEvidenceInput& input : inputs)
    {
        RecordingEvidenceSignature part =
            desiredRecordingEvidenceSignature(input);
        if (signature.nativeId.empty())
        {
            signature.nativeId = part.nativeId;
            signature.lifecycleState = part.lifecycleState;
        }
        signature.assignments.insert(
            signature.assignments.end(),
            part.assignments.begin(),
            part.assignments.end());
    }

    std::sort(
        signature.assignments.begin(),
        signature.assignments.end(),
        [](const RecordingAssignmentSignature& left,
           const RecordingAssignmentSignature& right)
        {
            if (left.ordinal != right.ordinal)
                return left.ordinal < right.ordinal;
            if (left.providerId != right.providerId)
                return left.providerId < right.providerId;
            if (left.genreId != right.genreId)
                return left.genreId < right.genreId;
            return left.originalValue < right.originalValue;
        });

    return signature;
}

// EPG reconciliation must not rewrite the same DVB evidence on every
// warmup. Compare the exact stored normalized evidence, not just a timestamp
// or a source descriptor: changed genre mappings still need reconciliation.
bool epgDvbEvidenceAlreadyCurrent(
    sqlite3* database, const GenreEvidenceInput& input)
{
    const char* sql =
        "SELECT b.native_id,b.lifecycle_state,b.start_time,b.end_time,"
        "a.provider_id,a.genre_id,a.original_value,a.normalization_rule,"
        "a.source_kind,a.assignment_state,a.confidence,a.ordinal "
        "FROM suite_metadata_target_bindings b "
        "LEFT JOIN suite_metadata_genre_assignments a "
        "ON a.metadata_target_id=b.metadata_target_id "
        "AND a.provider_id='vdr-epg' "
        "WHERE b.metadata_target_id=? AND b.backend_id=? "
        "AND b.target_type='program-event' "
        "ORDER BY a.ordinal,a.provider_id,a.genre_id,a.original_value;";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;

    bindText(statement, 1, GenreIndexRepository::stableTargetId(
        "program-event", input.backendId, input.resourceKey));
    bindText(statement, 2, input.backendId);

    RecordingEvidenceSignature existing;
    bool found = false;
    bool matchingTimes = false;
    int stepResult = SQLITE_DONE;
    while ((stepResult = sqlite3_step(statement)) == SQLITE_ROW)
    {
        if (!found)
        {
            found = true;
            existing.nativeId = columnText(statement, 0);
            existing.lifecycleState = columnText(statement, 1);
            matchingTimes =
                sqlite3_column_int64(statement, 2) == input.startTime &&
                sqlite3_column_int64(statement, 3) == input.endTime;
        }
        if (sqlite3_column_type(statement, 4) == SQLITE_NULL)
            continue;
        RecordingAssignmentSignature assignment;
        assignment.providerId = columnText(statement, 4);
        assignment.genreId = columnText(statement, 5);
        assignment.originalValue = columnText(statement, 6);
        assignment.normalizationRule = columnText(statement, 7);
        assignment.sourceKind = columnText(statement, 8);
        assignment.assignmentState = columnText(statement, 9);
        assignment.confidence = sqlite3_column_double(statement, 10);
        assignment.ordinal = sqlite3_column_int(statement, 11);
        existing.assignments.push_back(std::move(assignment));
    }
    sqlite3_finalize(statement);
    return found && matchingTimes && stepResult == SQLITE_DONE &&
        existing == desiredRecordingEvidenceSignature(input);
}

bool readRecordingEvidenceSignatures(
    sqlite3* database,
    const std::string& backendId,
    std::map<std::string, RecordingEvidenceSignature>& signatures)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT b.resource_key,b.native_id,b.lifecycle_state,"
        "COALESCE(a.provider_id,''),COALESCE(a.genre_id,''),"
        "COALESCE(a.original_value,''),"
        "COALESCE(a.normalization_rule,''),"
        "COALESCE(a.source_kind,''),"
        "COALESCE(a.assignment_state,''),"
        "COALESCE(a.confidence,0.0),COALESCE(a.ordinal,0) "
        "FROM suite_metadata_target_bindings b "
        "LEFT JOIN suite_metadata_genre_assignments a "
        "ON a.metadata_target_id=b.metadata_target_id "
        "AND a.source_kind IN("
        "'recording-metadata','recording-folder-genre') "
        "WHERE b.backend_id=? AND b.target_type='recording' "
        "ORDER BY b.resource_key,a.ordinal,a.provider_id,"
        "a.genre_id,a.original_value;";

    if (sqlite3_prepare_v2(
            database,
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    bindText(statement, 1, backendId);
    signatures.clear();

    int result = SQLITE_ROW;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        const std::string resourceKey =
            columnText(statement, 0);

        RecordingEvidenceSignature& signature =
            signatures[resourceKey];

        signature.nativeId = columnText(statement, 1);
        signature.lifecycleState = columnText(statement, 2);

        const std::string providerId =
            columnText(statement, 3);
        if (providerId.empty()) continue;

        RecordingAssignmentSignature assignment;
        assignment.providerId = providerId;
        assignment.genreId = columnText(statement, 4);
        assignment.originalValue = columnText(statement, 5);
        assignment.normalizationRule = columnText(statement, 6);
        assignment.sourceKind = columnText(statement, 7);
        assignment.assignmentState = columnText(statement, 8);
        assignment.confidence =
            sqlite3_column_double(statement, 9);
        assignment.ordinal =
            sqlite3_column_int(statement, 10);

        signature.assignments.push_back(
            std::move(assignment));
    }

    sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}
}

bool GenreIndexRepository::synchronizeRecordingCache(
    const std::string& backendId)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked()) return false;

    const std::string normalizedBackend =
        normalizeBackendId(backendId);
    const bool nativeMetadataAvailable =
        database_.tableExists(
            "vdr_recording_native_metadata") &&
        database_.tableExists(
            "vdr_recording_native_text_list");

    const char* sql = nativeMetadataAvailable
        ? "SELECT c.cache_key,c.backend_native_id,c.title,c.path,"
          "COALESCE(m.content_state,''),"
          "COALESCE(m.last_attempt_state,''),"
          "COALESCE(m.provider,''),COALESCE(l.value,'') "
          "FROM vdr_recording_cache c "
          "LEFT JOIN vdr_recording_native_metadata m "
          "ON m.backend_id=c.backend_id "
          "AND m.backend_native_id=c.backend_native_id "
          "LEFT JOIN vdr_recording_native_text_list l "
          "ON l.backend_id=m.backend_id "
          "AND l.recording_key=m.recording_key "
          "AND l.kind='genre' "
          "WHERE c.backend_id=? "
          "ORDER BY c.cache_key,l.ordinal;"
        : "SELECT c.cache_key,c.backend_native_id,"
          "c.title,c.path,'','','','' "
          "FROM vdr_recording_cache c "
          "WHERE c.backend_id=? ORDER BY c.cache_key;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

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

    int sourceResult = SQLITE_ROW;
    while ((sourceResult = sqlite3_step(statement)) == SQLITE_ROW)
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
    if (sourceResult != SQLITE_DONE) return false;

    std::map<std::string, std::vector<GenreEvidenceInput>>
        desiredRows;
    std::map<std::string, RecordingEvidenceSignature>
        desiredSignatures;

    for (const auto& entry : rows)
    {
        const std::string folderGenre =
            recordingFolderGenreCandidate(
                entry.second.title,
                entry.second.path);
        std::vector<GenreEvidenceInput> inputs;

        if (!entry.second.genres.empty() || folderGenre.empty())
        {
            GenreEvidenceInput metadataInput;
            metadataInput.backendId = normalizedBackend;
            metadataInput.targetType = "recording";
            metadataInput.resourceKey = entry.first;
            metadataInput.nativeId = entry.second.nativeId;
            metadataInput.providerId = providerIdFor(
                entry.second.provider,
                "recording-native");
            metadataInput.sourceKind = "recording-metadata";
            metadataInput.originalValues = entry.second.genres;
            metadataInput.confidence =
                entry.second.contentState == "found"
                    ? 0.9
                    : 0.0;

            if (entry.second.attemptState == "failure" &&
                !entry.second.genres.empty())
            {
                metadataInput.state = "stale";
            }
            else
            {
                metadataInput.state =
                    metadataInput.originalValues.empty()
                        ? "missing"
                        : "active";
            }

            inputs.push_back(std::move(metadataInput));
        }

        if (!folderGenre.empty())
        {
            GenreEvidenceInput folderInput;
            folderInput.backendId = normalizedBackend;
            folderInput.targetType = "recording";
            folderInput.resourceKey = entry.first;
            folderInput.nativeId = entry.second.nativeId;
            folderInput.providerId = "recording-folder";
            folderInput.sourceKind = "recording-folder-genre";
            folderInput.originalValues = {folderGenre};
            folderInput.state = "active";
            folderInput.confidence = 0.70;
            inputs.push_back(std::move(folderInput));
        }

        desiredSignatures.emplace(
            entry.first,
            desiredRecordingEvidenceSignature(inputs));
        desiredRows.emplace(
            entry.first,
            std::move(inputs));
    }

    auto transactionLease =
        database_.acquireTransactionLease();

    std::map<std::string, RecordingEvidenceSignature>
        existingSignatures;

    if (!readRecordingEvidenceSignatures(
            database_.handle(),
            normalizedBackend,
            existingSignatures))
    {
        return false;
    }

    std::vector<std::string> changedRecordingKeys;

    for (const auto& desired : desiredSignatures)
    {
        const auto existing =
            existingSignatures.find(desired.first);

        if (existing == existingSignatures.end() ||
            !(existing->second == desired.second))
        {
            changedRecordingKeys.push_back(desired.first);
        }
    }

    bool needsRetirement = false;
    for (const auto& existing : existingSignatures)
    {
        if (desiredRows.count(existing.first) == 0 &&
            existing.second.lifecycleState != "retired")
        {
            needsRetirement = true;
            break;
        }
    }

    if (changedRecordingKeys.empty() && !needsRetirement)
    {
        return true;
    }

    if (!database_.execute(
            "BEGIN IMMEDIATE TRANSACTION;"))
    {
        return false;
    }

    sqlite3_stmt* clearRecordingEvidence = nullptr;

    if (!changedRecordingKeys.empty())
    {
        const char* clearSql =
            "DELETE FROM suite_metadata_genre_assignments "
            "WHERE metadata_target_id=? "
            "AND source_kind IN("
            "'recording-metadata',"
            "'recording-folder-genre');";

        if (sqlite3_prepare_v2(
                database_.handle(),
                clearSql,
                -1,
                &clearRecordingEvidence,
                nullptr) != SQLITE_OK)
        {
            database_.execute("ROLLBACK;");
            return false;
        }
    }

    for (const std::string& key : changedRecordingKeys)
    {
        const std::string targetId = stableTargetId(
            "recording",
            normalizedBackend,
            key);

        sqlite3_reset(clearRecordingEvidence);
        sqlite3_clear_bindings(clearRecordingEvidence);
        bindText(clearRecordingEvidence, 1, targetId);

        bool replaced = stepDone(clearRecordingEvidence);
        if (replaced)
        {
            for (const GenreEvidenceInput& input : desiredRows.at(key))
            {
                if (!replaceEvidenceLocked(input))
                {
                    replaced = false;
                    break;
                }
            }
        }

        if (!replaced)
        {
            sqlite3_finalize(clearRecordingEvidence);
            database_.execute("ROLLBACK;");
            return false;
        }
    }

    if (clearRecordingEvidence != nullptr)
    {
        sqlite3_finalize(clearRecordingEvidence);
    }

    if (needsRetirement &&
        !retireMissingRecordingTargets(normalizedBackend))
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
    const auto syncStarted = std::chrono::steady_clock::now();
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

    const auto rowsReadAt = std::chrono::steady_clock::now();
    const auto readMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        rowsReadAt - syncStarted).count();
    if (readMs >= 1000)
        std::cerr << "EPG_GENRE_SYNC_SLOW phase=read_rows duration_ms="
                  << readMs << " items=" << rows.size() << std::endl;

    constexpr std::size_t EpgSynchronizationBatchSize = 16;
    std::size_t unchangedCount = 0;
    std::size_t changedCount = 0;
    for (std::size_t begin = 0; begin < rows.size(); begin += EpgSynchronizationBatchSize)
    {
        const std::size_t end = std::min(
            rows.size(),
            begin + EpgSynchronizationBatchSize);

        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            const auto leaseStarted = std::chrono::steady_clock::now();
            auto transactionLease = database_.acquireTransactionLease();

            // Read and decide under the existing SQLite lease. Unchanged
            // batches never open a write transaction.
            std::vector<std::size_t> changed;
            for (std::size_t index = begin; index < end; ++index)
            {
                if (epgDvbEvidenceAlreadyCurrent(database_.handle(), rows[index]))
                    ++unchangedCount;
                else
                    changed.push_back(index);
            }
            if (changed.empty()) continue;

            changedCount += changed.size();
            const auto transactionStarted = std::chrono::steady_clock::now();
            if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;
            for (const std::size_t index : changed)
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
            const auto batchCommitted = std::chrono::steady_clock::now();
            const auto leaseWaitMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    transactionStarted - leaseStarted).count();
            const auto heldMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    batchCommitted - transactionStarted).count();
            if (leaseWaitMs >= 1000 || heldMs >= 1000)
            {
                std::cerr << "EPG_GENRE_SYNC_SLOW phase=batch"
                          << " lease_wait_ms=" << leaseWaitMs
                          << " held_ms=" << heldMs
                          << " begin=" << begin
                          << " count=" << changed.size() << std::endl;
            }
        }

        if (end < rows.size())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        const auto leaseStarted = std::chrono::steady_clock::now();
        auto transactionLease = database_.acquireTransactionLease();
        const auto transactionStarted = std::chrono::steady_clock::now();
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
        const auto cleanupCommitted = std::chrono::steady_clock::now();
        const auto waitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            transactionStarted - leaseStarted).count();
        const auto heldMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            cleanupCommitted - transactionStarted).count();
        if (waitMs >= 1000 || heldMs >= 1000)
        {
            std::cerr << "EPG_GENRE_SYNC_SLOW phase=retirement"
                      << " lease_wait_ms=" << waitMs
                      << " held_ms=" << heldMs << std::endl;
        }
    }
    const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - syncStarted).count();
    if (totalMs >= 1000)
        std::cerr << "EPG_GENRE_SYNC_TOTAL duration_ms=" << totalMs
                  << " items=" << rows.size()
                  << " unchanged=" << unchangedCount
                  << " changed=" << changedCount << std::endl;
    return true;
}
