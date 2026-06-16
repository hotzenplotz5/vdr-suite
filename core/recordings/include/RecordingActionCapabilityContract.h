#pragma once

#include "RecordingAction.h"

#include <algorithm>
#include <string>
#include <vector>

struct RecordingActionCapabilitySet
{
    std::vector<std::string> capabilities;

    bool contains(
        const std::string& capability) const
    {
        return std::find(
                   capabilities.begin(),
                   capabilities.end(),
                   capability) != capabilities.end();
    }

    void add(
        const std::string& capability)
    {
        if (!contains(capability))
        {
            capabilities.push_back(capability);
        }
    }
};

struct RecordingActionCapabilityCheckResult
{
    bool supported = false;
    std::string requiredCapability;
    std::vector<std::string> missingCapabilities;
};

class RecordingActionCapabilityContract
{
public:
    std::string requiredCapability(
        RecordingActionType action) const
    {
        switch (action)
        {
        case RecordingActionType::Check:
            return "recording.action.check";
        case RecordingActionType::Repair:
            return "recording.action.repair";
        case RecordingActionType::Shrink:
            return "recording.action.shrink";
        case RecordingActionType::Cut:
            return "recording.action.cut";
        case RecordingActionType::Pes2Ts:
            return "recording.action.pes2ts";
        case RecordingActionType::Rename:
            return "recording.action.rename";
        case RecordingActionType::Move:
            return "recording.action.move";
        case RecordingActionType::Delete:
            return "recording.action.delete";
        case RecordingActionType::MetadataRefresh:
            return "recording.action.metadata-refresh";
        case RecordingActionType::Unknown:
            return "";
        }

        return "";
    }

    RecordingActionCapabilityCheckResult check(
        RecordingActionType action,
        const RecordingActionCapabilitySet& capabilitySet) const
    {
        RecordingActionCapabilityCheckResult result;
        result.requiredCapability = requiredCapability(action);

        if (result.requiredCapability.empty())
        {
            result.supported = false;
            result.missingCapabilities.push_back(
                "recording.action.unknown");
            return result;
        }

        result.supported =
            capabilitySet.contains(result.requiredCapability);

        if (!result.supported)
        {
            result.missingCapabilities.push_back(
                result.requiredCapability);
        }

        return result;
    }

    RecordingActionCapabilitySet restfulApiDefaultCapabilities() const
    {
        RecordingActionCapabilitySet capabilitySet;
        capabilitySet.add("recording.action.move");
        capabilitySet.add("recording.action.rename");
        capabilitySet.add("recording.action.delete");
        return capabilitySet;
    }

    RecordingActionCapabilitySet liveReferenceCapabilities() const
    {
        RecordingActionCapabilitySet capabilitySet;
        capabilitySet.add("recording.action.cut");
        capabilitySet.add("recording.action.rename");
        capabilitySet.add("recording.action.move");
        capabilitySet.add("recording.action.delete");
        capabilitySet.add("recording.action.metadata-refresh");
        return capabilitySet;
    }
};
