#include "NativeTimerPublicDetailRevision.h"
#include "PublicResourcePreconditions.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

int main()
{
    using S = NativeTimerPublicDetailRevisionStatus;
    VdrTimer timer;
    timer.id = "17";
    timer.channelId = "C-1-1051-10301";
    timer.title = "Tagesschau";
    timer.day = "2026-10-20";
    timer.weekdays = "-------";
    timer.startTime = "2000";
    timer.endTime = "2015";
    timer.priority = 50;
    timer.lifetime = 99;
    timer.enabled = true;

    NativeTimerBinding binding;
    binding.nativeTimerBindingId = "binding-17";
    binding.bindingRevision = "8";
    binding.backendId = "backend-one";
    binding.backendGeneration = 7;
    binding.backendNativeTimerId = timer.id;
    binding.timerAssignmentId = "assignment-17";
    binding.ownership = NativeTimerBindingOwnership::managed;
    binding.observedState.channelId = timer.channelId;
    binding.observedState.title = timer.title;
    binding.observedState.day = timer.day;
    binding.observedState.weekdays = timer.weekdays;
    binding.observedState.startTime = timer.startTime;
    binding.observedState.endTime = timer.endTime;
    binding.observedState.priority = timer.priority;
    binding.observedState.lifetime = timer.lifetime;
    binding.observedState.enabled = timer.enabled;
    binding.observedFingerprint =
        nativeTimerObservedStateFingerprint(binding.observedState);
    binding.lastObservedAt = 1900;
    assert(nativeTimerBindingValid(binding));

    auto verify = [&](const VdrTimer& t, const NativeTimerBinding* b,
                      std::uint64_t generation = 7)
    {
        return nativeTimerPublicDetailRevision(
            "backend-one", generation, t, b);
    };
    const auto ok = verify(timer, &binding);
    assert(ok.status == S::verifiedManaged);
    assert(!ok.resourceRevision.empty());
    const auto tag = vdrsuite::http::publicStrongEntityTag(ok.resourceRevision);
    std::string decoded;
    assert(vdrsuite::http::publicStrongEntityTagResourceRevision(tag, decoded));
    assert(decoded == ok.resourceRevision);
    assert(verify(timer, nullptr).status == S::unboundReadOnly);
    assert(verify(timer, nullptr).resourceRevision.empty());

    auto altered = timer;
    altered.endTime = "2030";
    assert(verify(altered, &binding).status == S::unsafeEvidence);
    assert(verify(timer, &binding, 8).status == S::unsafeEvidence);
    auto renamed = timer;
    renamed.id = "18";
    assert(verify(renamed, &binding).status == S::unsafeEvidence);
    auto stale = binding;
    stale.bindingRevision = "9";
    stale.driftState = NativeTimerBindingDriftState::externalFieldChange;
    assert(verify(timer, &stale).status == S::unsafeEvidence);
    auto lost = binding;
    lost.missingSince = 1900;
    lost.driftState = NativeTimerBindingDriftState::expectedTransition;
    assert(verify(timer, &lost).status == S::unsafeEvidence);
    auto recording = timer;
    recording.recording = true;
    assert(verify(recording, &binding).status == S::unsafeEvidence);
    auto pending = timer;
    pending.pending = true;
    assert(verify(pending, &binding).status == S::unsafeEvidence);

    auto adopted = binding;
    adopted.ownership = NativeTimerBindingOwnership::adopted;
    assert(verify(timer, &adopted).status == S::verifiedAdopted);
    auto external = binding;
    external.ownership = NativeTimerBindingOwnership::external;
    external.timerAssignmentId.clear();
    assert(nativeTimerBindingValid(external));
    assert(verify(timer, &external).status == S::adoptionRequired);
    assert(!verify(timer, &external).resourceRevision.empty());
    auto ambiguous = binding;
    ambiguous.ownership = NativeTimerBindingOwnership::ambiguous;
    assert(verify(timer, &ambiguous).status == S::unsafeEvidence);

    auto corrupt = binding;
    corrupt.observedFingerprint = "bad";
    assert(verify(timer, &corrupt).status == S::unsafeEvidence);
    assert(verify(timer, &corrupt).resourceRevision.empty());
    std::cout << "Public native Timer detail revision PASS\n";
}
