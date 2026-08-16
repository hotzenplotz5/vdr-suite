#include "Database.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerCreateReadbackEvidence.h"
#include "NativeTimerCreateReadbackExpectation.h"
#include "NativeTimerCreateReadbackVerificationService.h"
#include "NativeTimerSpecification.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

namespace
{
NativeTimerSpecification specification()
{
    NativeTimerSpecification value;
    value.channelId = "channel:1";
    value.title = "Managed Timer";
    value.directory = "VDR-Suite";
    value.day = "2026-08-17";
    value.weekdays = "-------";
    value.startTime = "0930";
    value.endTime = "1015";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

NativeTimerObservation observation(
    const std::string& nativeId,
    std::int64_t observedAt = 2100)
{
    NativeTimerObservation value;
    value.backendId = "backend:1";
    value.backendGeneration = 7;
    value.backendNativeTimerId = nativeId;
    value.observedAt = observedAt;
    value.observedState.channelId = "channel:1";
    value.observedState.title = "Managed Timer";
    value.observedState.directory = "VDR-Suite";
    value.observedState.day = "2026-08-17";
    value.observedState.weekdays = "-------";
    value.observedState.startTime = "930";
    value.observedState.endTime = "1015";
    value.observedState.priority = 50;
    value.observedState.lifetime = 99;
    value.observedState.enabled = true;
    value.observedState.vps = false;
    value.observedState.pending = true;
    value.observedFingerprint =
        nativeTimerObservedStateFingerprint(value.observedState);
    return value;
}

NativeTimerCreateReadbackCandidate candidate(
    const std::string& assignmentId,
    const std::string& bindingId,
    const std::string& nativeId)
{
    NativeTimerCreateReadbackCandidate value;
    value.timerAssignmentId = assignmentId;
    value.nativeTimerBindingId = bindingId;
    value.observation = observation(nativeId);
    return value;
}

NativeTimerCreateReadbackExpectation expectation(
    const std::string& operationId = "operation:1",
    const std::string& assignmentId = "assignment:1",
    const std::string& bindingId = "binding:1")
{
    NativeTimerCreateReadbackExpectation value;
    value.operationId = operationId;
    value.operationState =
        NativeTimerReadbackOperationState::executedUnverified;
    value.timerAssignmentId = assignmentId;
    value.nativeTimerBindingId = bindingId;
    value.backendId = "backend:1";
    value.backendGeneration = 7;
    value.readbackNotBefore = 2000;
    value.expectedSpecification = specification();
    value.expectedSpecificationFingerprint =
        nativeTimerSpecificationFingerprint(value.expectedSpecification);
    return value;
}

NativeTimerCreateReadbackEvidence evidence(
    std::initializer_list<NativeTimerCreateReadbackCandidate> candidates)
{
    NativeTimerCreateReadbackEvidence value;
    value.backendId = "backend:1";
    value.backendGeneration = 7;
    value.observedAt = 2100;
    value.completeness = NativeTimerCreateReadbackCompleteness::complete;
    value.candidates.assign(candidates.begin(), candidates.end());
    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    NativeTimerBindingRepository repository(database);
    assert(repository.ensureSchema());
    NativeTimerCreateReadbackVerificationService service(repository);

    const auto expected = expectation();
    const auto observed = evidence({candidate("assignment:1", "binding:1", "timer:17")});

    const auto verified = service.verify(expected, observed);
    assert(verified.status == NativeTimerCreateReadbackVerificationStatus::verified);
    assert(verified.binding.bindingRevision == "1");
    assert(verified.binding.nativeTimerBindingId == "binding:1");
    assert(verified.binding.backendNativeTimerId == "timer:17");
    assert(verified.binding.timerAssignmentId == "assignment:1");
    assert(verified.binding.ownership == NativeTimerBindingOwnership::managed);
    assert(verified.binding.lastVerifiedOperationId == "operation:1");
    assert(verified.binding.lastObservedAt == 2100);
    assert(verified.binding.missingSince == 0);
    assert(verified.binding.driftState == NativeTimerBindingDriftState::none);

    const auto replay = service.verify(expected, observed);
    assert(replay.status == NativeTimerCreateReadbackVerificationStatus::alreadyVerified);
    assert(replay.binding.bindingRevision == "1");

    assert(service.verify(expected, evidence({})).status ==
        NativeTimerCreateReadbackVerificationStatus::correlationNotFound);

    const auto ambiguous = evidence({
        candidate("assignment:1", "binding:1", "timer:18"),
        candidate("assignment:1", "binding:1", "timer:19")});
    assert(service.verify(expected, ambiguous).status ==
        NativeTimerCreateReadbackVerificationStatus::correlationAmbiguous);

    const auto withForeign = evidence({
        candidate("assignment:other", "binding:other", "timer:20"),
        candidate("assignment:1", "binding:1", "timer:17")});
    assert(service.verify(expected, withForeign).status ==
        NativeTimerCreateReadbackVerificationStatus::alreadyVerified);

    auto wrongBackend = observed;
    wrongBackend.backendId = "backend:other";
    for (auto& item : wrongBackend.candidates)
        item.observation.backendId = "backend:other";
    assert(service.verify(expected, wrongBackend).status ==
        NativeTimerCreateReadbackVerificationStatus::backendConflict);

    auto wrongGeneration = observed;
    wrongGeneration.backendGeneration = 8;
    for (auto& item : wrongGeneration.candidates)
        item.observation.backendGeneration = 8;
    assert(service.verify(expected, wrongGeneration).status ==
        NativeTimerCreateReadbackVerificationStatus::generationConflict);

    auto stale = observed;
    stale.observedAt = 1999;
    for (auto& item : stale.candidates)
        item.observation.observedAt = 1999;
    assert(service.verify(expected, stale).status ==
        NativeTimerCreateReadbackVerificationStatus::staleEvidence);

    auto changed = evidence({candidate("assignment:2", "binding:2", "timer:21")});
    changed.candidates[0].observation.observedState.startTime = "0945";
    changed.candidates[0].observation.observedFingerprint =
        nativeTimerObservedStateFingerprint(changed.candidates[0].observation.observedState);
    assert(service.verify(expectation("operation:2", "assignment:2", "binding:2"), changed).status ==
        NativeTimerCreateReadbackVerificationStatus::reconciliationRequired);

    assert(service.verify(
        expectation("operation:other", "assignment:1", "binding:1"), observed).status ==
        NativeTimerCreateReadbackVerificationStatus::bindingConflict);

    const auto nativeConflictEvidence = evidence({
        candidate("assignment:2", "binding:2", "timer:17")});
    assert(service.verify(
        expectation("operation:2", "assignment:2", "binding:2"),
        nativeConflictEvidence).status ==
        NativeTimerCreateReadbackVerificationStatus::nativeIdentityConflict);

    const auto assignmentConflictEvidence = evidence({
        candidate("assignment:1", "binding:3", "timer:22")});
    assert(service.verify(
        expectation("operation:3", "assignment:1", "binding:3"),
        assignmentConflictEvidence).status ==
        NativeTimerCreateReadbackVerificationStatus::assignmentBindingConflict);

    auto invalidExpectation = expected;
    invalidExpectation.readbackNotBefore = 0;
    assert(service.verify(invalidExpectation, observed).status ==
        NativeTimerCreateReadbackVerificationStatus::invalid);

    std::cout << "test_native_timer_create_readback_verification_service passed\n";
    return 0;
}
