#pragma once

#include <string>
#include <utility>

enum class CapabilityAvailability
{
    Available,
    Unsupported,
    NotConfigured,
    Offline,
    TemporarilyUnavailable
};

class CapabilityState
{
public:
    CapabilityState(
        std::string capabilityName,
        bool supported,
        CapabilityAvailability availability,
        std::string reason)
        : capabilityName_(std::move(capabilityName)),
          supported_(supported),
          availability_(availability),
          reason_(std::move(reason))
    {
    }

    static CapabilityState available(
        const std::string& capabilityName)
    {
        return CapabilityState(
            capabilityName,
            true,
            CapabilityAvailability::Available,
            "");
    }

    static CapabilityState unsupported(
        const std::string& capabilityName,
        const std::string& reason)
    {
        return CapabilityState(
            capabilityName,
            false,
            CapabilityAvailability::Unsupported,
            reason);
    }

    static CapabilityState notConfigured(
        const std::string& capabilityName,
        const std::string& reason)
    {
        return CapabilityState(
            capabilityName,
            false,
            CapabilityAvailability::NotConfigured,
            reason);
    }

    static CapabilityState offline(
        const std::string& capabilityName,
        const std::string& reason)
    {
        return CapabilityState(
            capabilityName,
            true,
            CapabilityAvailability::Offline,
            reason);
    }

    static CapabilityState temporarilyUnavailable(
        const std::string& capabilityName,
        const std::string& reason)
    {
        return CapabilityState(
            capabilityName,
            true,
            CapabilityAvailability::TemporarilyUnavailable,
            reason);
    }

    const std::string& capabilityName() const
    {
        return capabilityName_;
    }

    bool supported() const
    {
        return supported_;
    }

    CapabilityAvailability availability() const
    {
        return availability_;
    }

    const std::string& reason() const
    {
        return reason_;
    }

    bool availableNow() const
    {
        return supported_ &&
               availability_ == CapabilityAvailability::Available;
    }

private:
    std::string capabilityName_;
    bool supported_;
    CapabilityAvailability availability_;
    std::string reason_;
};
