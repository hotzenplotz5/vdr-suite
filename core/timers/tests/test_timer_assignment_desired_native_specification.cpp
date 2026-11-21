#include "Database.h"
#include "TimerAssignmentDesiredNativeTimerSpecification.h"
#include "TimerAssignmentRepository.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{
NativeTimerSpecification specification(
    const std::string& channelId = "S19.2E-1-1019-10301")
{
    NativeTimerSpecification value;
    value.channelId = channelId;
    value.title = "Phase 69.C";
    value.directory = "VDR-Suite";
    value.day = "2026-09-23";
    value.weekdays = "-------";
    value.startTime = "2015";
    value.endTime = "2145";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

TimerAssignment freshAssignment()
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = "assignment:desired-spec";
    assignment.timerIntentId = "intent:new";
    assignment.intentRevision = "1";
    assignment.backendId = "backend:alpha";
    assignment.backendGeneration = 7;
    assignment.state = TimerAssignmentState::selected;
    assignment.role = TimerAssignmentRole::primary;
    assignment.channelBinding = {
        "channel:ard",
        "S19.2E-1-1019-10301",
        "canonical-channel-map",
        "mapping:7"};
    assignment.capabilityRevision = "capability:7";
    assignment.backendHealthRevision = "health:7";
    assignment.decisionPolicyVersion = "timer-assignment-planner/1";
    assignment.decisionEvidence.reasons = {"selected_eligible_backend"};
    assignment.desiredNativeTimerSpecificationPresent = true;
    assignment.desiredNativeTimerSpecification = specification();
    assignment.createdAt = 200;
    assignment.updatedAt = 200;
    return assignment;
}

bool installLegacySchema(Database& database)
{
    return database.execute(
        "CREATE TABLE timer_intents ("
        "timer_intent_id TEXT PRIMARY KEY NOT NULL,"
        "intent_revision INTEGER NOT NULL CHECK(intent_revision > 0)"
        ");"
        "INSERT INTO timer_intents(timer_intent_id,intent_revision) "
        "VALUES('intent:legacy',1),('intent:new',1);"
        "CREATE TABLE timer_assignments ("
        "timer_assignment_id TEXT PRIMARY KEY NOT NULL,"
        "assignment_revision INTEGER NOT NULL,"
        "timer_intent_id TEXT NOT NULL,"
        "intent_revision TEXT NOT NULL,"
        "assignment_epoch INTEGER NOT NULL,"
        "backend_id TEXT NOT NULL DEFAULT '',"
        "backend_generation INTEGER NOT NULL DEFAULT 0,"
        "state TEXT NOT NULL,"
        "role TEXT NOT NULL,"
        "channel_canonical_id TEXT NOT NULL DEFAULT '',"
        "channel_backend_id TEXT NOT NULL DEFAULT '',"
        "channel_mapping_source TEXT NOT NULL DEFAULT '',"
        "channel_mapping_revision TEXT NOT NULL DEFAULT '',"
        "capability_revision TEXT NOT NULL DEFAULT '',"
        "backend_health_revision TEXT NOT NULL DEFAULT '',"
        "decision_policy_version TEXT NOT NULL,"
        "decision_reasons TEXT NOT NULL,"
        "decision_warnings TEXT NOT NULL DEFAULT '',"
        "decision_exclusions TEXT NOT NULL DEFAULT '',"
        "decision_conflict_facts TEXT NOT NULL DEFAULT '',"
        "decision_score INTEGER NOT NULL DEFAULT 0,"
        "native_timer_binding_id TEXT NOT NULL DEFAULT '',"
        "created_at INTEGER NOT NULL,"
        "updated_at INTEGER NOT NULL"
        ");"
        "INSERT INTO timer_assignments("
        "timer_assignment_id,assignment_revision,timer_intent_id,"
        "intent_revision,assignment_epoch,backend_id,backend_generation,"
        "state,role,channel_canonical_id,channel_backend_id,"
        "channel_mapping_source,channel_mapping_revision,"
        "capability_revision,backend_health_revision,"
        "decision_policy_version,decision_reasons,decision_warnings,"
        "decision_exclusions,decision_conflict_facts,decision_score,"
        "native_timer_binding_id,created_at,updated_at"
        ") VALUES("
        "'assignment:legacy',1,'intent:legacy','1',1,"
        "'backend:legacy',4,'selected','primary','channel:legacy',"
        "'C-1-2-3','legacy-map','mapping:legacy','capability:legacy',"
        "'health:legacy','timer-assignment-planner/1','6:legacy','','','',"
        "0,'',100,100"
        ");");
}
} // namespace

int main()
{
    const NativeTimerSpecification desired = specification();
    const std::string encoded =
        serializeTimerAssignmentDesiredNativeTimerSpecification(desired);
    assert(!encoded.empty());

    NativeTimerSpecification decoded;
    assert(parseTimerAssignmentDesiredNativeTimerSpecification(
        encoded, decoded));
    assert(timerAssignmentDesiredNativeTimerSpecificationEquivalent(
        desired, decoded));
    assert(!parseTimerAssignmentDesiredNativeTimerSpecification(
        "timer-assignment-native-specification/1|broken",
        decoded));

    Database database;
    assert(database.open(":memory:"));
    assert(installLegacySchema(database));

    TimerAssignmentRepository repository(database);
    assert(repository.ensureSchema());

    // Existing rows remain readable after the additive migration and are
    // explicitly marked as lacking a durable desired native specification.
    const auto legacy = repository.findById("assignment:legacy");
    assert(legacy.ok());
    assert(!legacy.assignment.desiredNativeTimerSpecificationPresent);

    const auto created = repository.create(freshAssignment());
    assert(created.ok());
    assert(created.assignment.assignmentRevision == "1");
    assert(created.assignment.desiredNativeTimerSpecificationPresent);
    assert(created.assignment.desiredNativeTimerSpecification.channelId
        == "S19.2E-1-1019-10301");

    const auto loaded = repository.findById("assignment:desired-spec");
    assert(loaded.ok());
    assert(loaded.assignment.desiredNativeTimerSpecificationPresent);
    assert(timerAssignmentDesiredNativeTimerSpecificationEquivalent(
        loaded.assignment.desiredNativeTimerSpecification,
        specification()));

    // The planned desired specification is immutable with the assignment
    // identity. A normal state transition may retain it, but may not replace it.
    TimerAssignment provisioning = loaded.assignment;
    provisioning.state = TimerAssignmentState::provisioning;
    provisioning.updatedAt += 1;
    const auto provisioned =
        repository.update(provisioning, loaded.assignment.assignmentRevision);
    assert(provisioned.ok());

    TimerAssignment changed = provisioned.assignment;
    changed.desiredNativeTimerSpecification.title = "different";
    changed.updatedAt += 1;
    assert(repository.update(
        changed, provisioned.assignment.assignmentRevision).status
        == TimerAssignmentRepositoryStatus::invalid);

    TimerAssignment mismatch = freshAssignment();
    mismatch.timerAssignmentId = "assignment:mismatch";
    mismatch.desiredNativeTimerSpecification =
        specification("S19.2E-1-1019-99999");
    mismatch.createdAt = 300;
    mismatch.updatedAt = 300;
    assert(repository.create(mismatch).status
        == TimerAssignmentRepositoryStatus::invalid);

    std::cout
        << "test_timer_assignment_desired_native_specification passed\n";
    return 0;
}
