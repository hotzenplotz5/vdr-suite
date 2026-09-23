#include "Database.h"
#include "TimerAssignmentReadService.h"
#include "TimerAssignmentRepository.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{

TimerAssignment makeSelected()
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = "assignment:public-read";
    assignment.timerIntentId = "intent:public-read";
    assignment.intentRevision = "1";
    assignment.backendId = "backend:living-room";
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
        "selected for public read facade test"};
    assignment.decisionEvidence.decisionScore = 100;
    assignment.createdAt = 100;
    assignment.updatedAt = 100;
    return assignment;
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
    assert(database.execute(
        "INSERT INTO timer_intents "
        "(timer_intent_id,intent_revision) "
        "VALUES ('intent:public-read',1);"));

    TimerAssignmentRepository repository(database);
    assert(repository.ensureSchema());

    const auto created = repository.create(makeSelected());
    assert(created.ok());
    assert(created.assignment.assignmentRevision == "1");

    TimerAssignmentReadService service(repository);

    const auto owner =
        service.findForBackend(
            "assignment:public-read",
            "backend:living-room");
    assert(owner.ok());
    assert(owner.assignment.timerAssignmentId ==
        "assignment:public-read");
    assert(owner.assignment.backendId ==
        "backend:living-room");
    assert(owner.assignment.assignmentRevision == "1");

    const auto otherBackend =
        service.findForBackend(
            "assignment:public-read",
            "backend:bedroom");
    assert(otherBackend.status ==
        TimerAssignmentReadStatus::notFound);

    const auto missing =
        service.findForBackend(
            "assignment:missing",
            "backend:living-room");
    assert(missing.status ==
        TimerAssignmentReadStatus::notFound);

    assert(service.findForBackend(
        "",
        "backend:living-room").status ==
        TimerAssignmentReadStatus::invalid);
    assert(service.findForBackend(
        "assignment:public-read",
        "").status ==
        TimerAssignmentReadStatus::invalid);

    TimerAssignment provisioning = created.assignment;
    provisioning.state = TimerAssignmentState::provisioning;
    provisioning.updatedAt += 1;
    const auto updated =
        repository.update(
            provisioning,
            created.assignment.assignmentRevision);
    assert(updated.ok());
    assert(updated.assignment.assignmentRevision == "2");

    const auto current =
        service.findForBackend(
            "assignment:public-read",
            "backend:living-room");
    assert(current.ok());
    assert(current.assignment.assignmentRevision == "2");
    assert(current.assignment.state ==
        TimerAssignmentState::provisioning);

    std::cout
        << "test_timer_assignment_read_service passed\n";
    return 0;
}
