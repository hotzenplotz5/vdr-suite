#include "Database.h"
#include "NativeTimerBinding.h"
#include "NativeTimerBindingAbsenceApplicationService.h"
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
    value.enabled = true;
    return value;
}

NativeTimerBinding binding(
    const std::string& id,
    const std::string& nativeId,
    NativeTimerBindingDriftState drift = NativeTimerBindingDriftState::none)
{
    NativeTimerBinding value;
    value.nativeTimerBindingId = id;
    value.backendId = "backend:a";
    value.backendGeneration = 7;
    value.backendNativeTimerId = nativeId;
    value.timerAssignmentId = "assignment:" + id;
    value.ownership = NativeTimerBindingOwnership::managed;
    value.observedState = state();
    value.observedFingerprint = nativeTimerObservedStateFingerprint(value.observedState);
    value.lastObservedAt = 1900;
    value.driftState = drift;
    return value;
}

NativeTimerInventoryEvidence inventory(
    std::uint64_t generation,
    std::int64_t observedAt,
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
    NativeTimerBindingAbsenceApplicationService service(repository);

    const auto created = repository.create(binding("binding:1", "timer:17"));
    assert(created.ok());

    const auto missing = service.apply("binding:1", inventory(8, 2100));
    assert(missing.status == NativeTimerBindingAbsenceApplicationStatus::missingRecorded);
    assert(missing.binding.bindingRevision == "2");
    assert(missing.binding.backendGeneration == 8);
    assert(missing.binding.lastObservedAt == 2100);
    assert(missing.binding.missingSince == 2100);
    assert(missing.binding.driftState == NativeTimerBindingDriftState::ambiguous);
    assert(missing.binding.observedFingerprint == created.binding.observedFingerprint);
    assert(missing.binding.observedState.startTime == "930");

    const auto replay = service.apply("binding:1", inventory(8, 2100));
    assert(replay.status == NativeTimerBindingAbsenceApplicationStatus::alreadyCurrent);
    assert(replay.binding.bindingRevision == "2");

    const auto refreshed = service.apply("binding:1", inventory(8, 2200));
    assert(refreshed.status == NativeTimerBindingAbsenceApplicationStatus::missingRefreshed);
    assert(refreshed.binding.bindingRevision == "3");
    assert(refreshed.binding.lastObservedAt == 2200);
    assert(refreshed.binding.missingSince == 2100);
    assert(refreshed.binding.driftState == NativeTimerBindingDriftState::ambiguous);

    const auto stillPresentCreated = repository.create(
        binding("binding:present", "timer:present"));
    assert(stillPresentCreated.ok());
    const auto presentResult = service.apply(
        "binding:present", inventory(7, 2000, {"timer:present"}));
    assert(presentResult.status == NativeTimerBindingAbsenceApplicationStatus::present);
    assert(presentResult.binding.bindingRevision == "1");

    auto presentAfterMissingEvidence = inventory(8, 2300, {"timer:17"});
    const auto presentAfterMissing = service.apply("binding:1", presentAfterMissingEvidence);
    assert(presentAfterMissing.status ==
        NativeTimerBindingAbsenceApplicationStatus::reconciliationRequired);
    assert(repository.findById("binding:1").binding.bindingRevision == "3");

    NativeTimerBinding external = binding("binding:external", "timer:external");
    external.timerAssignmentId.clear();
    external.ownership = NativeTimerBindingOwnership::external;
    const auto externalCreated = repository.create(external);
    assert(externalCreated.ok());
    const auto externalMissing = service.apply(
        "binding:external", inventory(8, 2100));
    assert(externalMissing.status ==
        NativeTimerBindingAbsenceApplicationStatus::missingRecorded);
    assert(externalMissing.binding.ownership == NativeTimerBindingOwnership::external);
    assert(externalMissing.binding.driftState ==
        NativeTimerBindingDriftState::ambiguous);

    const auto expectedCreated = repository.create(binding(
        "binding:expected", "timer:expected",
        NativeTimerBindingDriftState::expectedTransition));
    assert(expectedCreated.ok());
    const auto expectedMissing = service.apply(
        "binding:expected", inventory(8, 2100));
    assert(expectedMissing.status == NativeTimerBindingAbsenceApplicationStatus::missingRecorded);
    assert(expectedMissing.binding.driftState == NativeTimerBindingDriftState::expectedTransition);

    NativeTimerBinding classified = binding("binding:classified", "timer:classified");
    classified.lastObservedAt = 2000;
    classified.missingSince = 2000;
    classified.driftState = NativeTimerBindingDriftState::externalDelete;
    const auto classifiedCreated = repository.create(classified);
    assert(classifiedCreated.ok());
    const auto classifiedRefresh = service.apply(
        "binding:classified", inventory(8, 2200));
    assert(classifiedRefresh.status == NativeTimerBindingAbsenceApplicationStatus::missingRefreshed);
    assert(classifiedRefresh.binding.missingSince == 2000);
    assert(classifiedRefresh.binding.driftState == NativeTimerBindingDriftState::externalDelete);

    auto wrongBackend = inventory(8, 2200);
    wrongBackend.backendId = "backend:b";
    assert(service.apply("binding:present", wrongBackend).status ==
        NativeTimerBindingAbsenceApplicationStatus::backendConflict);

    assert(service.apply("binding:present", inventory(6, 2200)).status ==
        NativeTimerBindingAbsenceApplicationStatus::staleGeneration);
    assert(service.apply("binding:present", inventory(7, 1800)).status ==
        NativeTimerBindingAbsenceApplicationStatus::staleEvidence);

    auto invalid = inventory(8, 2200);
    invalid.completeness = NativeTimerInventoryCompleteness::unknown;
    assert(service.apply("binding:present", invalid).status ==
        NativeTimerBindingAbsenceApplicationStatus::invalid);
    assert(service.apply("", inventory(8, 2200)).status ==
        NativeTimerBindingAbsenceApplicationStatus::invalid);
    assert(service.apply("binding:missing", inventory(8, 2200)).status ==
        NativeTimerBindingAbsenceApplicationStatus::bindingNotFound);

    std::cout << "test_native_timer_binding_absence_application_service passed\n";
    return 0;
}
