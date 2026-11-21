#include "Database.h"
#include "TimerAssignmentReadService.h"
#include "TimerAssignmentRepository.h"

#include <cassert>
#include <cstdint>
#include <string>

using namespace vdrsuite::timers;

namespace
{

TimerAssignment makeSelected(
    const std::string& assignmentId,
    const std::string& intentId,
    const std::string& backendId,
    std::int64_t createdAt)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = assignmentId;
    assignment.timerIntentId = intentId;
    assignment.intentRevision = "1";
    assignment.backendId = backendId;
    assignment.backendGeneration = 7;
    assignment.state = TimerAssignmentState::selected;
    assignment.role = TimerAssignmentRole::primary;
    assignment.channelBinding = {
        "channel:ard-hd",
        "S19.2E-1-1019-10301",
        "canonical-channel-map",
        "mapping-revision:1"};
    assignment.capabilityRevision = "capability-revision:1";
    assignment.backendHealthRevision = "backend-health-revision:1";
    assignment.decisionPolicyVersion = "scheduler-policy:1";
    assignment.decisionEvidence.reasons = {
        "selected for Phase 69.D collection read test"};
    assignment.decisionEvidence.decisionScore = 100;
    assignment.createdAt = createdAt;
    assignment.updatedAt = createdAt;
    return assignment;
}

void addIntent(Database& database, const std::string& intentId)
{
    const std::string sql =
        "INSERT INTO timer_intents "
        "(timer_intent_id,intent_revision) VALUES ('" +
        intentId + "',1);";
    assert(database.execute(sql));
}

}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    assert(database.execute(
        "CREATE TABLE timer_intents ("
        "timer_intent_id TEXT PRIMARY KEY NOT NULL,"
        "intent_revision INTEGER NOT NULL CHECK(intent_revision > 0)"
        ");"));

    for (const std::string& intentId :
         {"intent:a", "intent:b", "intent:c", "intent:z"})
    {
        addIntent(database, intentId);
    }

    TimerAssignmentRepository repository(database);
    assert(repository.ensureSchema());

    assert(repository.create(
        makeSelected("assignment:a", "intent:a", "backend:one", 100)).ok());
    assert(repository.create(
        makeSelected("assignment:b", "intent:b", "backend:one", 101)).ok());
    assert(repository.create(
        makeSelected("assignment:c", "intent:c", "backend:one", 102)).ok());
    assert(repository.create(
        makeSelected("assignment:z", "intent:z", "backend:two", 103)).ok());

    TimerAssignmentReadService service(repository);

    const auto first =
        service.listForBackend("backend:one", "", 2U);
    assert(first.ok());
    assert(first.assignments.size() == 2U);
    assert(first.assignments.at(0).timerAssignmentId == "assignment:a");
    assert(first.assignments.at(1).timerAssignmentId == "assignment:b");
    assert(first.hasMore);

    const auto second =
        service.listForBackend(
            "backend:one",
            first.assignments.back().timerAssignmentId,
            2U);
    assert(second.ok());
    assert(second.assignments.size() == 1U);
    assert(second.assignments.front().timerAssignmentId == "assignment:c");
    assert(!second.hasMore);

    const auto end =
        service.listForBackend("backend:one", "assignment:c", 2U);
    assert(end.ok());
    assert(end.assignments.empty());
    assert(!end.hasMore);

    const auto other =
        service.listForBackend("backend:two", "", 10U);
    assert(other.ok());
    assert(other.assignments.size() == 1U);
    assert(other.assignments.front().timerAssignmentId == "assignment:z");

    const auto empty =
        service.listForBackend("backend:missing", "", 10U);
    assert(empty.ok());
    assert(empty.assignments.empty());
    assert(!empty.hasMore);

    assert(service.listForBackend("", "", 2U).status ==
        TimerAssignmentCollectionReadStatus::invalid);
    assert(service.listForBackend("backend:one", "", 0U).status ==
        TimerAssignmentCollectionReadStatus::invalid);
    assert(service.listForBackend("backend:one", "", 101U).status ==
        TimerAssignmentCollectionReadStatus::invalid);

    assert(repository.listForBackendAfter(
        "backend:one", "", 102U).status ==
        TimerAssignmentRepositoryStatus::invalid);

    return 0;
}
