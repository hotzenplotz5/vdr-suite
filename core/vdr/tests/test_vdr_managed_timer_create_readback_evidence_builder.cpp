#include "NativeTimerInventoryEvidence.h"
#include "VdrManagedTimerCreateReadbackEvidenceBuilder.h"
#include "VdrTimerManagedCorrelation.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace vdrsuite::timers;

namespace
{
NativeTimerInventoryEvidence inventory(std::vector<std::string> ids)
{
    NativeTimerInventoryEvidence value;
    value.backendId = "backend:1";
    value.backendGeneration = 7;
    value.observedAt = 2100;
    value.completeness = NativeTimerInventoryCompleteness::complete;
    value.backendNativeTimerIds = std::move(ids);
    return value;
}

VdrTimer timer(const std::string& id, const std::string& aux = {})
{
    VdrTimer value;
    value.id = id;
    value.channelId = "channel:1";
    value.eventId = "event:1";
    value.title = "Managed Timer";
    value.directory = "VDR-Suite";
    value.aux = aux;
    value.day = "2026-08-17";
    value.weekdays = "-------";
    value.startTime = "0930";
    value.endTime = "1015";
    value.flags = 1;
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.pending = true;
    return value;
}

std::string managedAux(
    const std::string& assignmentId,
    const std::string& bindingId,
    const std::string& prefix = {})
{
    VdrTimerManagedCorrelation correlation;
    correlation.timerAssignmentId = assignmentId;
    correlation.nativeTimerBindingId = bindingId;
    const auto attached = attachVdrTimerManagedCorrelation(prefix, correlation);
    assert(attached.ok());
    return attached.aux;
}
}

int main()
{
    const auto complete = inventory({"10", "20", "30"});
    const std::string foreign =
        "<epgsearch><searchtimer>42</searchtimer></epgsearch>";
    const std::string aux = managedAux("assignment:1", "binding:1", foreign);

    std::vector<VdrTimer> timers = {
        timer("10"),
        timer("20", aux),
        timer("30", foreign),
    };

    const auto built = VdrManagedTimerCreateReadbackEvidenceBuilder::build(
        complete, timers);
    assert(built.ok());
    assert(built.evidence.backendId == "backend:1");
    assert(built.evidence.backendGeneration == 7);
    assert(built.evidence.observedAt == 2100);
    assert(built.evidence.completeness ==
        NativeTimerCreateReadbackCompleteness::complete);
    assert(built.evidence.candidates.size() == 1);
    assert(built.evidence.candidates[0].timerAssignmentId == "assignment:1");
    assert(built.evidence.candidates[0].nativeTimerBindingId == "binding:1");
    assert(built.evidence.candidates[0].observation.backendNativeTimerId == "20");
    assert(built.evidence.candidates[0].observation.observedState.title ==
        "Managed Timer");

    const std::string duplicateCorrelation =
        managedAux("assignment:dup", "binding:dup");
    const auto ambiguousEvidence =
        VdrManagedTimerCreateReadbackEvidenceBuilder::build(
            inventory({"40", "41"}),
            {timer("40", duplicateCorrelation), timer("41", duplicateCorrelation)});
    assert(ambiguousEvidence.ok());
    assert(ambiguousEvidence.evidence.candidates.size() == 2);
    assert(ambiguousEvidence.evidence.candidates[0].timerAssignmentId ==
        "assignment:dup");
    assert(ambiguousEvidence.evidence.candidates[1].timerAssignmentId ==
        "assignment:dup");

    const auto missingTimer = VdrManagedTimerCreateReadbackEvidenceBuilder::build(
        complete, {timer("10"), timer("20", aux)});
    assert(missingTimer.status ==
        VdrManagedTimerCreateReadbackEvidenceBuildStatus::inventoryMismatch);

    const auto duplicateNative =
        VdrManagedTimerCreateReadbackEvidenceBuilder::build(
            inventory({"10", "20"}), {timer("10"), timer("10")});
    assert(duplicateNative.status ==
        VdrManagedTimerCreateReadbackEvidenceBuildStatus::
            duplicateNativeTimerIdentity);

    auto invalidTimer = timer("20", aux);
    invalidTimer.channelId.clear();
    const auto invalidObservation =
        VdrManagedTimerCreateReadbackEvidenceBuilder::build(
            inventory({"20"}), {invalidTimer});
    assert(invalidObservation.status ==
        VdrManagedTimerCreateReadbackEvidenceBuildStatus::invalidTimerObservation);

    const auto malformed = VdrManagedTimerCreateReadbackEvidenceBuilder::build(
        inventory({"20"}), {timer("20", "<vdr-suite-managed-timer-broken/>")});
    assert(malformed.status ==
        VdrManagedTimerCreateReadbackEvidenceBuildStatus::
            malformedManagedCorrelation);

    const std::string oneMarker = managedAux("assignment:1", "binding:1");
    const auto conflicting = VdrManagedTimerCreateReadbackEvidenceBuilder::build(
        inventory({"20"}), {timer("20", oneMarker + oneMarker)});
    assert(conflicting.status ==
        VdrManagedTimerCreateReadbackEvidenceBuildStatus::
            conflictingManagedCorrelation);

    auto invalidInventory = inventory({"20", "10"});
    const auto invalidInventoryResult =
        VdrManagedTimerCreateReadbackEvidenceBuilder::build(
            invalidInventory, {timer("10"), timer("20")});
    assert(invalidInventoryResult.status ==
        VdrManagedTimerCreateReadbackEvidenceBuildStatus::invalidInventoryEvidence);

    std::cout << "test_vdr_managed_timer_create_readback_evidence_builder passed\n";
    return 0;
}
