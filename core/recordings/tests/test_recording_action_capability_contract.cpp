#include "RecordingActionCapabilityContract.h"

#include <cassert>
#include <string>

int main()
{
    RecordingActionCapabilityContract contract;

    assert(contract.requiredCapability(RecordingActionType::Check) ==
           "recording.action.check");
    assert(contract.requiredCapability(RecordingActionType::Repair) ==
           "recording.action.repair");
    assert(contract.requiredCapability(RecordingActionType::Shrink) ==
           "recording.action.shrink");
    assert(contract.requiredCapability(RecordingActionType::Cut) ==
           "recording.action.cut");
    assert(contract.requiredCapability(RecordingActionType::Pes2Ts) ==
           "recording.action.pes2ts");
    assert(contract.requiredCapability(RecordingActionType::Rename) ==
           "recording.action.rename");
    assert(contract.requiredCapability(RecordingActionType::Move) ==
           "recording.action.move");
    assert(contract.requiredCapability(RecordingActionType::Delete) ==
           "recording.action.delete");
    assert(contract.requiredCapability(RecordingActionType::MetadataRefresh) ==
           "recording.action.metadata-refresh");
    assert(contract.requiredCapability(RecordingActionType::Unknown).empty());

    RecordingActionCapabilitySet capabilitySet;
    capabilitySet.add("recording.action.delete");
    capabilitySet.add("recording.action.delete");
    capabilitySet.add("recording.action.move");

    assert(capabilitySet.capabilities.size() == 2);
    assert(capabilitySet.contains("recording.action.delete"));
    assert(capabilitySet.contains("recording.action.move"));
    assert(!capabilitySet.contains("recording.action.rename"));

    const RecordingActionCapabilityCheckResult deleteResult =
        contract.check(
            RecordingActionType::Delete,
            capabilitySet);

    assert(deleteResult.supported);
    assert(deleteResult.requiredCapability ==
           "recording.action.delete");
    assert(deleteResult.missingCapabilities.empty());

    const RecordingActionCapabilityCheckResult renameResult =
        contract.check(
            RecordingActionType::Rename,
            capabilitySet);

    assert(!renameResult.supported);
    assert(renameResult.requiredCapability ==
           "recording.action.rename");
    assert(renameResult.missingCapabilities.size() == 1);
    assert(renameResult.missingCapabilities.at(0) ==
           "recording.action.rename");

    const RecordingActionCapabilityCheckResult unknownResult =
        contract.check(
            RecordingActionType::Unknown,
            capabilitySet);

    assert(!unknownResult.supported);
    assert(unknownResult.requiredCapability.empty());
    assert(unknownResult.missingCapabilities.size() == 1);
    assert(unknownResult.missingCapabilities.at(0) ==
           "recording.action.unknown");

    const RecordingActionCapabilitySet restfulApiCapabilities =
        contract.restfulApiDefaultCapabilities();

    assert(restfulApiCapabilities.contains("recording.action.move"));
    assert(restfulApiCapabilities.contains("recording.action.rename"));
    assert(restfulApiCapabilities.contains("recording.action.delete"));
    assert(!restfulApiCapabilities.contains("recording.action.cut"));
    assert(!restfulApiCapabilities.contains("recording.action.undelete"));

    const RecordingActionCapabilitySet liveCapabilities =
        contract.liveReferenceCapabilities();

    assert(liveCapabilities.contains("recording.action.cut"));
    assert(liveCapabilities.contains("recording.action.rename"));
    assert(liveCapabilities.contains("recording.action.move"));
    assert(liveCapabilities.contains("recording.action.delete"));
    assert(liveCapabilities.contains("recording.action.metadata-refresh"));

    return 0;
}
