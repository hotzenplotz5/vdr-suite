#include "TimerIntentRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace vdrsuite::timers
{
namespace
{

const char* kSelectColumns =
    "timer_intent_id,intent_revision,intent_type,state,owner_actor_id,created_by_actor_id,"
    "automation_source_type,automation_source_id,automation_occurrence_id,program_event_id,"
    "backend_event_backend_id,backend_event_channel_id,backend_event_event_id,backend_event_source_id,"
    "channel_canonical_id,channel_source_type,channel_source_id,channel_source_channel_id,"
    "schedule_start_at,schedule_stop_at,schedule_timezone,schedule_recurrence_rule,"
    "recording_start_margin_seconds,recording_stop_margin_seconds,recording_priority,recording_lifetime_days,"
    "recording_vps_preferred,recording_directory_policy,recording_naming_policy,recording_retention_policy_reference,"
    "assignment_allow_failover,assignment_require_operator_review,assignment_preferred_backend_ids,assignment_excluded_backend_ids,"
    "replica_desired_assignments,replica_require_backend_diversity,replica_require_site_diversity,"
    "replica_simultaneous_recording_intentional,replica_storage_policy_reference,replica_retention_policy_reference,replica_rationale,"
    "duplicate_prevent_equivalent_intent,duplicate_require_operator_review_on_ambiguity,created_at,updated_at,expires_at,semantic_identity";

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* text = sqlite3_column_text(statement, column);
    if (!text)
    {
        return {};
    }

    const int bytes = sqlite3_column_bytes(statement, column);
    return std::string(reinterpret_cast<const char*>(text), static_cast<std::size_t>(bytes));
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        return false;
    }

    return sqlite3_bind_text(
        statement,
        index,
        value.data(),
        static_cast<int>(value.size()),
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool parseRevisionToken(const std::string& token, std::int64_t& revision)
{
    if (token.empty())
    {
        return false;
    }

    std::int64_t value = 0;
    for (char ch : token)
    {
        if (ch < '0' || ch > '9')
        {
            return false;
        }

        const std::int64_t digit = ch - '0';
        if (value > (std::numeric_limits<std::int64_t>::max() - digit) / 10)
        {
            return false;
        }
        value = value * 10 + digit;
    }

    if (value <= 0)
    {
        return false;
    }

    revision = value;
    return true;
}

std::string encodeStrings(const std::vector<std::string>& values)
{
    std::string encoded;
    for (const auto& value : values)
    {
        encoded.append(std::to_string(value.size()));
        encoded.push_back(':');
        encoded.append(value);
    }
    return encoded;
}

bool decodeStrings(const std::string& encoded, std::vector<std::string>& values)
{
    values.clear();
    std::size_t position = 0;

    while (position < encoded.size())
    {
        const std::size_t colon = encoded.find(':', position);
        if (colon == std::string::npos || colon == position)
        {
            return false;
        }

        std::size_t length = 0;
        for (std::size_t index = position; index < colon; ++index)
        {
            const char ch = encoded[index];
            if (ch < '0' || ch > '9')
            {
                return false;
            }

            const std::size_t digit = static_cast<std::size_t>(ch - '0');
            if (length > (std::numeric_limits<std::size_t>::max() - digit) / 10)
            {
                return false;
            }
            length = length * 10 + digit;
        }

        position = colon + 1;
        if (length > encoded.size() - position)
        {
            return false;
        }

        values.emplace_back(encoded.substr(position, length));
        position += length;
    }

    return true;
}

bool parseIntentType(const std::string& value, TimerIntentType& type)
{
    if (value == "programme_event")
    {
        type = TimerIntentType::programmeEvent;
        return true;
    }
    if (value == "manual_window")
    {
        type = TimerIntentType::manualWindow;
        return true;
    }
    if (value == "recurring_schedule")
    {
        type = TimerIntentType::recurringSchedule;
        return true;
    }
    return false;
}

bool parseIntentState(const std::string& value, TimerIntentState& state)
{
    if (value == "draft") state = TimerIntentState::draft;
    else if (value == "active") state = TimerIntentState::active;
    else if (value == "paused") state = TimerIntentState::paused;
    else if (value == "satisfied") state = TimerIntentState::satisfied;
    else if (value == "cancel_requested") state = TimerIntentState::cancelRequested;
    else if (value == "cancelled") state = TimerIntentState::cancelled;
    else if (value == "expired") state = TimerIntentState::expired;
    else if (value == "failed") state = TimerIntentState::failed;
    else return false;
    return true;
}

bool bindIntentColumns(
    sqlite3_stmt* statement,
    int startIndex,
    const TimerIntent& intent,
    const std::string& semanticIdentity,
    bool includeId)
{
    int index = startIndex;
    std::int64_t revision = 0;
    if (!parseRevisionToken(intent.intentRevision, revision))
    {
        return false;
    }

    if (includeId && !bindText(statement, index++, intent.timerIntentId)) return false;
    if (sqlite3_bind_int64(statement, index++, revision) != SQLITE_OK) return false;
    if (!bindText(statement, index++, timerIntentTypeName(intent.spec.intentType))) return false;
    if (!bindText(statement, index++, timerIntentStateName(intent.state))) return false;
    if (!bindText(statement, index++, intent.spec.ownerActorId)) return false;
    if (!bindText(statement, index++, intent.createdByActorId)) return false;
    if (!bindText(statement, index++, intent.spec.automationSource.sourceType)) return false;
    if (!bindText(statement, index++, intent.spec.automationSource.sourceId)) return false;
    if (!bindText(statement, index++, intent.spec.automationSource.occurrenceId)) return false;
    if (!bindText(statement, index++, intent.spec.programEventId)) return false;
    if (!bindText(statement, index++, intent.spec.backendEventRef.backendId)) return false;
    if (!bindText(statement, index++, intent.spec.backendEventRef.channelId)) return false;
    if (!bindText(statement, index++, intent.spec.backendEventRef.eventId)) return false;
    if (!bindText(statement, index++, intent.spec.backendEventRef.sourceId)) return false;
    if (!bindText(statement, index++, intent.spec.channelRequirement.canonicalChannelId)) return false;
    if (!bindText(statement, index++, intent.spec.channelRequirement.sourceType)) return false;
    if (!bindText(statement, index++, intent.spec.channelRequirement.sourceId)) return false;
    if (!bindText(statement, index++, intent.spec.channelRequirement.sourceChannelId)) return false;
    if (sqlite3_bind_int64(statement, index++, intent.spec.schedule.startAt) != SQLITE_OK) return false;
    if (sqlite3_bind_int64(statement, index++, intent.spec.schedule.stopAt) != SQLITE_OK) return false;
    if (!bindText(statement, index++, intent.spec.schedule.timezone)) return false;
    if (!bindText(statement, index++, intent.spec.schedule.recurrenceRule)) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.recordingOptions.startMarginSeconds) != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.recordingOptions.stopMarginSeconds) != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.recordingOptions.priority) != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.recordingOptions.lifetimeDays) != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.recordingOptions.vpsPreferred ? 1 : 0) != SQLITE_OK) return false;
    if (!bindText(statement, index++, intent.spec.recordingOptions.directoryPolicy)) return false;
    if (!bindText(statement, index++, intent.spec.recordingOptions.namingPolicy)) return false;
    if (!bindText(statement, index++, intent.spec.recordingOptions.retentionPolicyReference)) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.assignmentPolicy.allowFailover ? 1 : 0) != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.assignmentPolicy.requireOperatorReview ? 1 : 0) != SQLITE_OK) return false;
    if (!bindText(statement, index++, encodeStrings(intent.spec.assignmentPolicy.preferredBackendIds))) return false;
    if (!bindText(statement, index++, encodeStrings(intent.spec.assignmentPolicy.excludedBackendIds))) return false;
    if (sqlite3_bind_int64(statement, index++, static_cast<sqlite3_int64>(intent.spec.replicaPolicy.desiredAssignments)) != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.replicaPolicy.requireBackendDiversity ? 1 : 0) != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.replicaPolicy.requireSiteDiversity ? 1 : 0) != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.replicaPolicy.simultaneousRecordingIntentional ? 1 : 0) != SQLITE_OK) return false;
    if (!bindText(statement, index++, intent.spec.replicaPolicy.storagePolicyReference)) return false;
    if (!bindText(statement, index++, intent.spec.replicaPolicy.retentionPolicyReference)) return false;
    if (!bindText(statement, index++, intent.spec.replicaPolicy.rationale)) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.duplicatePolicy.preventEquivalentIntent ? 1 : 0) != SQLITE_OK) return false;
    if (sqlite3_bind_int(statement, index++, intent.spec.duplicatePolicy.requireOperatorReviewOnAmbiguity ? 1 : 0) != SQLITE_OK) return false;
    if (sqlite3_bind_int64(statement, index++, intent.createdAt) != SQLITE_OK) return false;
    if (sqlite3_bind_int64(statement, index++, intent.updatedAt) != SQLITE_OK) return false;
    if (sqlite3_bind_int64(statement, index++, intent.expiresAt) != SQLITE_OK) return false;
    return bindText(statement, index, semanticIdentity);
}

bool readIntent(sqlite3_stmt* statement, TimerIntent& intent)
{
    const sqlite3_int64 revision = sqlite3_column_int64(statement, 1);
    if (revision <= 0)
    {
        return false;
    }

    intent.timerIntentId = columnText(statement, 0);
    intent.intentRevision = std::to_string(revision);
    if (!parseIntentType(columnText(statement, 2), intent.spec.intentType)) return false;
    if (!parseIntentState(columnText(statement, 3), intent.state)) return false;
    intent.spec.ownerActorId = columnText(statement, 4);
    intent.createdByActorId = columnText(statement, 5);
    intent.spec.automationSource.sourceType = columnText(statement, 6);
    intent.spec.automationSource.sourceId = columnText(statement, 7);
    intent.spec.automationSource.occurrenceId = columnText(statement, 8);
    intent.spec.programEventId = columnText(statement, 9);
    intent.spec.backendEventRef.backendId = columnText(statement, 10);
    intent.spec.backendEventRef.channelId = columnText(statement, 11);
    intent.spec.backendEventRef.eventId = columnText(statement, 12);
    intent.spec.backendEventRef.sourceId = columnText(statement, 13);
    intent.spec.channelRequirement.canonicalChannelId = columnText(statement, 14);
    intent.spec.channelRequirement.sourceType = columnText(statement, 15);
    intent.spec.channelRequirement.sourceId = columnText(statement, 16);
    intent.spec.channelRequirement.sourceChannelId = columnText(statement, 17);
    intent.spec.schedule.startAt = sqlite3_column_int64(statement, 18);
    intent.spec.schedule.stopAt = sqlite3_column_int64(statement, 19);
    intent.spec.schedule.timezone = columnText(statement, 20);
    intent.spec.schedule.recurrenceRule = columnText(statement, 21);
    intent.spec.recordingOptions.startMarginSeconds = sqlite3_column_int(statement, 22);
    intent.spec.recordingOptions.stopMarginSeconds = sqlite3_column_int(statement, 23);
    intent.spec.recordingOptions.priority = sqlite3_column_int(statement, 24);
    intent.spec.recordingOptions.lifetimeDays = sqlite3_column_int(statement, 25);
    intent.spec.recordingOptions.vpsPreferred = sqlite3_column_int(statement, 26) != 0;
    intent.spec.recordingOptions.directoryPolicy = columnText(statement, 27);
    intent.spec.recordingOptions.namingPolicy = columnText(statement, 28);
    intent.spec.recordingOptions.retentionPolicyReference = columnText(statement, 29);
    intent.spec.assignmentPolicy.allowFailover = sqlite3_column_int(statement, 30) != 0;
    intent.spec.assignmentPolicy.requireOperatorReview = sqlite3_column_int(statement, 31) != 0;
    if (!decodeStrings(columnText(statement, 32), intent.spec.assignmentPolicy.preferredBackendIds)) return false;
    if (!decodeStrings(columnText(statement, 33), intent.spec.assignmentPolicy.excludedBackendIds)) return false;
    const sqlite3_int64 desiredAssignments = sqlite3_column_int64(statement, 34);
    if (desiredAssignments < 0 || desiredAssignments > std::numeric_limits<std::uint32_t>::max()) return false;
    intent.spec.replicaPolicy.desiredAssignments = static_cast<std::uint32_t>(desiredAssignments);
    intent.spec.replicaPolicy.requireBackendDiversity = sqlite3_column_int(statement, 35) != 0;
    intent.spec.replicaPolicy.requireSiteDiversity = sqlite3_column_int(statement, 36) != 0;
    intent.spec.replicaPolicy.simultaneousRecordingIntentional = sqlite3_column_int(statement, 37) != 0;
    intent.spec.replicaPolicy.storagePolicyReference = columnText(statement, 38);
    intent.spec.replicaPolicy.retentionPolicyReference = columnText(statement, 39);
    intent.spec.replicaPolicy.rationale = columnText(statement, 40);
    intent.spec.duplicatePolicy.preventEquivalentIntent = sqlite3_column_int(statement, 41) != 0;
    intent.spec.duplicatePolicy.requireOperatorReviewOnAmbiguity = sqlite3_column_int(statement, 42) != 0;
    intent.createdAt = sqlite3_column_int64(statement, 43);
    intent.updatedAt = sqlite3_column_int64(statement, 44);
    intent.expiresAt = sqlite3_column_int64(statement, 45);

    if (!timerIntentValid(intent))
    {
        return false;
    }

    return timerIntentSemanticIdentity(intent.spec) == columnText(statement, 46);
}

bool selectById(
    Database& database,
    const std::string& timerIntentId,
    TimerIntent& intent,
    bool& found)
{
    found = false;
    sqlite3_stmt* statement = nullptr;
    const std::string sql = std::string("SELECT ") + kSelectColumns +
        " FROM timer_intents WHERE timer_intent_id=?;";

    if (sqlite3_prepare_v2(database.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }

    if (!bindText(statement, 1, timerIntentId))
    {
        sqlite3_finalize(statement);
        return false;
    }

    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        found = readIntent(statement, intent);
        const bool ok = found;
        sqlite3_finalize(statement);
        return ok;
    }

    const bool ok = step == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

TimerIntentRepositoryResult statusResult(TimerIntentRepositoryStatus status)
{
    TimerIntentRepositoryResult result;
    result.status = status;
    return result;
}

TimerIntentRepositoryResult statusResult(
    TimerIntentRepositoryStatus status,
    const TimerIntent& intent)
{
    TimerIntentRepositoryResult result;
    result.status = status;
    result.intent = intent;
    return result;
}

}

TimerIntentRepository::TimerIntentRepository(Database& database)
    : database_(database)
{
}

bool TimerIntentRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS timer_intents ("
        "timer_intent_id TEXT PRIMARY KEY NOT NULL,"
        "intent_revision INTEGER NOT NULL CHECK(intent_revision > 0),"
        "intent_type TEXT NOT NULL CHECK(intent_type IN ('programme_event','manual_window','recurring_schedule')),"
        "state TEXT NOT NULL CHECK(state IN ('draft','active','paused','satisfied','cancel_requested','cancelled','expired','failed')),"
        "owner_actor_id TEXT NOT NULL,"
        "created_by_actor_id TEXT NOT NULL,"
        "automation_source_type TEXT NOT NULL DEFAULT '',"
        "automation_source_id TEXT NOT NULL DEFAULT '',"
        "automation_occurrence_id TEXT NOT NULL DEFAULT '',"
        "program_event_id TEXT NOT NULL DEFAULT '',"
        "backend_event_backend_id TEXT NOT NULL DEFAULT '',"
        "backend_event_channel_id TEXT NOT NULL DEFAULT '',"
        "backend_event_event_id TEXT NOT NULL DEFAULT '',"
        "backend_event_source_id TEXT NOT NULL DEFAULT '',"
        "channel_canonical_id TEXT NOT NULL DEFAULT '',"
        "channel_source_type TEXT NOT NULL DEFAULT '',"
        "channel_source_id TEXT NOT NULL DEFAULT '',"
        "channel_source_channel_id TEXT NOT NULL DEFAULT '',"
        "schedule_start_at INTEGER NOT NULL,"
        "schedule_stop_at INTEGER NOT NULL,"
        "schedule_timezone TEXT NOT NULL,"
        "schedule_recurrence_rule TEXT NOT NULL DEFAULT '',"
        "recording_start_margin_seconds INTEGER NOT NULL,"
        "recording_stop_margin_seconds INTEGER NOT NULL,"
        "recording_priority INTEGER NOT NULL,"
        "recording_lifetime_days INTEGER NOT NULL,"
        "recording_vps_preferred INTEGER NOT NULL CHECK(recording_vps_preferred IN (0,1)),"
        "recording_directory_policy TEXT NOT NULL DEFAULT '',"
        "recording_naming_policy TEXT NOT NULL DEFAULT '',"
        "recording_retention_policy_reference TEXT NOT NULL DEFAULT '',"
        "assignment_allow_failover INTEGER NOT NULL CHECK(assignment_allow_failover IN (0,1)),"
        "assignment_require_operator_review INTEGER NOT NULL CHECK(assignment_require_operator_review IN (0,1)),"
        "assignment_preferred_backend_ids TEXT NOT NULL DEFAULT '',"
        "assignment_excluded_backend_ids TEXT NOT NULL DEFAULT '',"
        "replica_desired_assignments INTEGER NOT NULL CHECK(replica_desired_assignments > 0),"
        "replica_require_backend_diversity INTEGER NOT NULL CHECK(replica_require_backend_diversity IN (0,1)),"
        "replica_require_site_diversity INTEGER NOT NULL CHECK(replica_require_site_diversity IN (0,1)),"
        "replica_simultaneous_recording_intentional INTEGER NOT NULL CHECK(replica_simultaneous_recording_intentional IN (0,1)),"
        "replica_storage_policy_reference TEXT NOT NULL DEFAULT '',"
        "replica_retention_policy_reference TEXT NOT NULL DEFAULT '',"
        "replica_rationale TEXT NOT NULL DEFAULT '',"
        "duplicate_prevent_equivalent_intent INTEGER NOT NULL CHECK(duplicate_prevent_equivalent_intent IN (0,1)),"
        "duplicate_require_operator_review_on_ambiguity INTEGER NOT NULL CHECK(duplicate_require_operator_review_on_ambiguity IN (0,1)),"
        "created_at INTEGER NOT NULL,"
        "updated_at INTEGER NOT NULL,"
        "expires_at INTEGER NOT NULL DEFAULT 0,"
        "semantic_identity TEXT NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_timer_intents_semantic_identity "
        "ON timer_intents (semantic_identity,timer_intent_id);"
        "CREATE INDEX IF NOT EXISTS idx_timer_intents_state_schedule "
        "ON timer_intents (state,schedule_start_at,schedule_stop_at,timer_intent_id);");
}

TimerIntentRepositoryResult TimerIntentRepository::create(const TimerIntent& intent)
{
    if (!intent.intentRevision.empty())
    {
        return statusResult(TimerIntentRepositoryStatus::invalid);
    }

    TimerIntent durable = intent;
    durable.intentRevision = "1";
    durable.updatedAt = durable.createdAt;
    const std::string semanticIdentity = timerIntentSemanticIdentity(durable.spec);
    if (!timerIntentValid(durable) || semanticIdentity.empty())
    {
        return statusResult(TimerIntentRepositoryStatus::invalid);
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema())
    {
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }

    const char* sql =
        "INSERT INTO timer_intents ("
        "timer_intent_id,intent_revision,intent_type,state,owner_actor_id,created_by_actor_id,"
        "automation_source_type,automation_source_id,automation_occurrence_id,program_event_id,"
        "backend_event_backend_id,backend_event_channel_id,backend_event_event_id,backend_event_source_id,"
        "channel_canonical_id,channel_source_type,channel_source_id,channel_source_channel_id,"
        "schedule_start_at,schedule_stop_at,schedule_timezone,schedule_recurrence_rule,"
        "recording_start_margin_seconds,recording_stop_margin_seconds,recording_priority,recording_lifetime_days,"
        "recording_vps_preferred,recording_directory_policy,recording_naming_policy,recording_retention_policy_reference,"
        "assignment_allow_failover,assignment_require_operator_review,assignment_preferred_backend_ids,assignment_excluded_backend_ids,"
        "replica_desired_assignments,replica_require_backend_diversity,replica_require_site_diversity,replica_simultaneous_recording_intentional,"
        "replica_storage_policy_reference,replica_retention_policy_reference,replica_rationale,"
        "duplicate_prevent_equivalent_intent,duplicate_require_operator_review_on_ambiguity,created_at,updated_at,expires_at,semantic_identity"
        ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }

    if (!bindIntentColumns(statement, 1, durable, semanticIdentity, true))
    {
        sqlite3_finalize(statement);
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }

    const int step = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (step == SQLITE_DONE)
    {
        return statusResult(TimerIntentRepositoryStatus::ok, durable);
    }

    if (sqlite3_errcode(database_.handle()) == SQLITE_CONSTRAINT)
    {
        return statusResult(TimerIntentRepositoryStatus::alreadyExists);
    }
    return statusResult(TimerIntentRepositoryStatus::storageError);
}

TimerIntentRepositoryResult TimerIntentRepository::findById(const std::string& timerIntentId)
{
    if (timerIntentId.empty())
    {
        return statusResult(TimerIntentRepositoryStatus::invalid);
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema())
    {
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }

    TimerIntent intent;
    bool found = false;
    if (!selectById(database_, timerIntentId, intent, found))
    {
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }
    if (!found)
    {
        return statusResult(TimerIntentRepositoryStatus::notFound);
    }
    return statusResult(TimerIntentRepositoryStatus::ok, intent);
}

TimerIntentRepositoryListResult TimerIntentRepository::findEquivalent(const TimerIntentSpec& spec)
{
    TimerIntentRepositoryListResult result;
    const std::string semanticIdentity = timerIntentSemanticIdentity(spec);
    if (semanticIdentity.empty())
    {
        result.status = TimerIntentRepositoryStatus::invalid;
        return result;
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema())
    {
        result.status = TimerIntentRepositoryStatus::storageError;
        return result;
    }

    sqlite3_stmt* statement = nullptr;
    const std::string sql = std::string("SELECT ") + kSelectColumns +
        " FROM timer_intents WHERE semantic_identity=? ORDER BY timer_intent_id;";
    if (sqlite3_prepare_v2(database_.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK)
    {
        result.status = TimerIntentRepositoryStatus::storageError;
        return result;
    }
    if (!bindText(statement, 1, semanticIdentity))
    {
        sqlite3_finalize(statement);
        result.status = TimerIntentRepositoryStatus::storageError;
        return result;
    }

    int step = SQLITE_ROW;
    while ((step = sqlite3_step(statement)) == SQLITE_ROW)
    {
        TimerIntent intent;
        if (!readIntent(statement, intent))
        {
            sqlite3_finalize(statement);
            result.intents.clear();
            result.status = TimerIntentRepositoryStatus::storageError;
            return result;
        }
        result.intents.push_back(intent);
    }

    sqlite3_finalize(statement);
    result.status = step == SQLITE_DONE
        ? TimerIntentRepositoryStatus::ok
        : TimerIntentRepositoryStatus::storageError;
    if (!result.ok())
    {
        result.intents.clear();
    }
    return result;
}

TimerIntentRepositoryResult TimerIntentRepository::update(
    const TimerIntent& next,
    const std::string& expectedRevision)
{
    std::int64_t expectedRevisionNumber = 0;
    if (!parseRevisionToken(expectedRevision, expectedRevisionNumber) ||
        next.intentRevision != expectedRevision)
    {
        return statusResult(TimerIntentRepositoryStatus::invalid);
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema() || !database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }

    TimerIntent current;
    bool found = false;
    if (!selectById(database_, next.timerIntentId, current, found))
    {
        database_.execute("ROLLBACK;");
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }
    if (!found)
    {
        database_.execute("ROLLBACK;");
        return statusResult(TimerIntentRepositoryStatus::notFound);
    }
    if (!timerIntentRevisionMatches(expectedRevision, current.intentRevision))
    {
        database_.execute("ROLLBACK;");
        return statusResult(TimerIntentRepositoryStatus::conflict, current);
    }

    if (timerIntentTerminal(current.state) ||
        next.createdAt != current.createdAt ||
        next.createdByActorId != current.createdByActorId ||
        next.updatedAt <= current.updatedAt ||
        (next.state != current.state && !timerIntentCanTransition(current.state, next.state)))
    {
        database_.execute("ROLLBACK;");
        return statusResult(TimerIntentRepositoryStatus::invalid);
    }

    if (expectedRevisionNumber == std::numeric_limits<std::int64_t>::max())
    {
        database_.execute("ROLLBACK;");
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }

    TimerIntent durable = next;
    durable.intentRevision = std::to_string(expectedRevisionNumber + 1);
    const std::string semanticIdentity = timerIntentSemanticIdentity(durable.spec);
    if (!timerIntentValid(durable) || semanticIdentity.empty())
    {
        database_.execute("ROLLBACK;");
        return statusResult(TimerIntentRepositoryStatus::invalid);
    }

    const char* sql =
        "UPDATE timer_intents SET "
        "intent_revision=?,intent_type=?,state=?,owner_actor_id=?,created_by_actor_id=?,"
        "automation_source_type=?,automation_source_id=?,automation_occurrence_id=?,program_event_id=?,"
        "backend_event_backend_id=?,backend_event_channel_id=?,backend_event_event_id=?,backend_event_source_id=?,"
        "channel_canonical_id=?,channel_source_type=?,channel_source_id=?,channel_source_channel_id=?,"
        "schedule_start_at=?,schedule_stop_at=?,schedule_timezone=?,schedule_recurrence_rule=?,"
        "recording_start_margin_seconds=?,recording_stop_margin_seconds=?,recording_priority=?,recording_lifetime_days=?,"
        "recording_vps_preferred=?,recording_directory_policy=?,recording_naming_policy=?,recording_retention_policy_reference=?,"
        "assignment_allow_failover=?,assignment_require_operator_review=?,assignment_preferred_backend_ids=?,assignment_excluded_backend_ids=?,"
        "replica_desired_assignments=?,replica_require_backend_diversity=?,replica_require_site_diversity=?,replica_simultaneous_recording_intentional=?,"
        "replica_storage_policy_reference=?,replica_retention_policy_reference=?,replica_rationale=?,"
        "duplicate_prevent_equivalent_intent=?,duplicate_require_operator_review_on_ambiguity=?,created_at=?,updated_at=?,expires_at=?,semantic_identity=? "
        "WHERE timer_intent_id=? AND intent_revision=?;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        database_.execute("ROLLBACK;");
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }

    if (!bindIntentColumns(statement, 1, durable, semanticIdentity, false) ||
        !bindText(statement, 47, durable.timerIntentId) ||
        sqlite3_bind_int64(statement, 48, expectedRevisionNumber) != SQLITE_OK)
    {
        sqlite3_finalize(statement);
        database_.execute("ROLLBACK;");
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }

    const int step = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (step != SQLITE_DONE || sqlite3_changes(database_.handle()) != 1)
    {
        database_.execute("ROLLBACK;");
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }

    if (!database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return statusResult(TimerIntentRepositoryStatus::storageError);
    }

    return statusResult(TimerIntentRepositoryStatus::ok, durable);
}

}
