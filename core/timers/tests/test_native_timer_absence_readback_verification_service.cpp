#include "Database.h"
#include "NativeTimerAbsenceReadbackExpectation.h"
#include "NativeTimerAbsenceReadbackVerificationService.h"
#include "NativeTimerBinding.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerInventoryEvidence.h"

#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

using namespace vdrsuite::timers;

namespace
{
NativeTimerObservedState state()
{
    NativeTimerObservedState value;
    value.channelId = "channel:1";
    value.weekdays = "-------";
    value.startTime = "930";
    value.endTime = "1000";
    return value;
}

NativeTimerBinding binding(
    const std::string& id,
    const std::string& nativeId,
    NativeTimerBindingOwnership ownership = NativeTimerBindingOwnership::managed)
{
    NativeTimerBinding value;
    value.nativeTimerBindingId = id;
    value.backendId = "backend:a";
    value.backendGeneration = 7;
    value.backendNativeTimerId = nativeId;
    value.timerAssignmentId = "assignment:" + id;
    value.ownership = ownership;
    if (ownership == NativeTimerBindingOwnership::external)
        value.timerAssignmentId.clear();
    value.observedState = state();
    value.observedFingerprint = nativeTimerObservedStateFingerprint(value.observedState);
    value.lastObservedAt = 1900;
    return value;
}

NativeTimerAbsenceReadbackExpectation expectation(const NativeTimerBinding& current)
{
    NativeTimerAbsenceReadbackExpectation value;
    value.operationId = "operation:delete:1";
    value.operationState = NativeTimerReadbackOperationState::executedUnverified;
    value.nativeTimerBindingId = current.nativeTimerBindingId;
    value.expectedBindingRevision = current.bindingRevision;
    value.backendId = current.backendId;
    value.backendGeneration = 8;
    value.backendNativeTimerId = current.backendNativeTimerId;
    value.readbackNotBefore = 2000;
    return value;
}

NativeTimerInventoryEvidence inventory(
    std::uint64_t generation = 8,
    std::int64_t observedAt = 2100,
    std::vector<std::string> ids = {})
{
    NativeTimerInventoryEvidence value;
    value.backendId = "backend:a";
    value.backendGeneration = generation;
    value.observedAt = observedAt;
    value.completeness = NativeTimerInventoryCompleteness::complete;
    value.backendNativeTimerIds = std::move(ids);
    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    NativeTimerBindingRepository repository(database);
    assert(repository.ensureSchema());
    NativeTimerAbsenceReadbackVerificationService service(repository);

    const auto created = repository.create(binding("binding:1", "timer:17"));
    assert(created.ok());
    assert(created.binding.bindingRevision == "1");

    const auto expected = expectation(created.binding);
    const auto verified = service.verify(expected, inventory());
    assert(verified.status == NativeTimerAbsenceReadbackVerificationStatus::verified);
    assert(verified.binding.bindingRevision == "2");
    assert(verified.binding.backendGeneration == 8);
    assert(verified.binding.lastObservedAt == 2100);
    assert(verified.binding.missingSince == 2100);
    assert(verified.binding.driftState == NativeTimerBindingDriftState::expectedTransition);
    assert(verified.binding.lastVerifiedOperationId == "operation:delete:1");
    assert(verified.binding.observedFingerprint == created.binding.observedFingerprint);

    const auto replay = service.verify(expected, inventory());
    assert(replay.status == NativeTimerAbsenceReadbackVerificationStatus::alreadyVerified);
    assert(replay.binding.bindingRevision == "2");

    const auto presentAfterVerified = service.verify(
        expected, inventory(8, 2200, {"timer:17"}));
    assert(presentAfterVerified.status ==
        NativeTimerAbsenceReadbackVerificationStatus::reconciliationRequired);
    assert(repository.findById("binding:1").binding.bindingRevision == "2");

    auto secondExpectation = expectation(verified.binding);
    secondExpectation.operationId = "operation:delete:2";
    secondExpectation.expectedBindingRevision = "1";
    assert(service.verify(secondExpectation, inventory(8, 2200)).status ==
        NativeTimerAbsenceReadbackVerificationStatus::bindingRevisionConflict);

    secondExpectation.expectedBindingRevision = "2";
    assert(service.verify(secondExpectation, inventory(8, 2050)).status ==
        NativeTimerAbsenceReadbackVerificationStatus::staleEvidence);
    assert(service.verify(secondExpectation, inventory(9, 2200)).status ==
        NativeTimerAbsenceReadbackVerificationStatus::generationConflict);

    auto wrongIdentity = secondExpectation;
    wrongIdentity.backendNativeTimerId = "timer:other";
    assert(service.verify(wrongIdentity, inventory(8, 2200)).status ==
        NativeTimerAbsenceReadbackVerificationStatus::identityConflict);

    const auto externalCreated = repository.create(binding(
        "binding:external", "timer:external",
        NativeTimerBindingOwnership::external));
    assert(externalCreated.ok());
    const auto externalExpectation = expectation(externalCreated.binding);
    assert(service.verify(externalExpectation, inventory()).status ==
        NativeTimerAbsenceReadbackVerificationStatus::ownershipConflict);

    NativeTimerBinding prior = binding("binding:prior", "timer:prior");
    prior.lastObservedAt = 1950;
    prior.missingSince = 1800;
    prior.driftState = NativeTimerBindingDriftState::ambiguous;
    const auto priorCreated = repository.create(prior);
    assert(priorCreated.ok());
    const auto priorVerified = service.verify(expectation(priorCreated.binding), inventory());
    assert(priorVerified.status == NativeTimerAbsenceReadbackVerificationStatus::verified);
    assert(priorVerified.binding.missingSince == 1800);
    assert(priorVerified.binding.driftState == NativeTimerBindingDriftState::ambiguous);
    assert(priorVerified.binding.lastVerifiedOperationId == "operation:delete:1");

    NativeTimerBinding classified = binding(
        "binding:external-delete", "timer:external-delete");
    classified.lastObservedAt = 1950;
    classified.missingSince = 1800;
    classified.driftState = NativeTimerBindingDriftState::externalDelete;
    const auto classifiedCreated = repository.create(classified);
    assert(classifiedCreated.ok());
    const auto classifiedVerified = service.verify(
        expectation(classifiedCreated.binding), inventory());
    assert(classifiedVerified.status == NativeTimerAbsenceReadbackVerificationStatus::verified);
    assert(classifiedVerified.binding.missingSince == 1800);
    assert(classifiedVerified.binding.driftState == NativeTimerBindingDriftState::externalDelete);

    auto invalidExpectation = expectation(classifiedVerified.binding);
    invalidExpectation.operationId = "operation:invalid";
    invalidExpectation.expectedBindingRevision = classifiedVerified.binding.bindingRevision;
    invalidExpectation.readbackNotBefore = 0;
    assert(service.verify(invalidExpectation, inventory()).status ==
        NativeTimerAbsenceReadbackVerificationStatus::invalid);

    auto incomplete = inventory();
    incomplete.completeness = NativeTimerInventoryCompleteness::unknown;
    assert(service.verify(expectation(classifiedVerified.binding), incomplete).status ==
        NativeTimerAbsenceReadbackVerificationStatus::invalid);

    auto missingExpectation = expectation(classifiedVerified.binding);
    missingExpectation.nativeTimerBindingId = "binding:missing";
    assert(service.verify(missingExpectation, inventory()).status ==
        NativeTimerAbsenceReadbackVerificationStatus::bindingNotFound);

    std::cout << "test_native_timer_absence_readback_verification_service passed\n";
    return 0;
}
