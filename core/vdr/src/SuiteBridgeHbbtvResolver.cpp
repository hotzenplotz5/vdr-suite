#include "SuiteBridgeHbbtvResolver.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr std::size_t kMaximumApplications = 16;

bool safeIdentity(const std::string& value, std::size_t maximumLength)
{
    return !value.empty() && value.size() <= maximumLength &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

bool safeText(const std::string& value, std::size_t maximumLength)
{
    return value.size() <= maximumLength &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return character >= 0x20U || character == '\t';
        });
}

class JsonCursor
{
public:
    explicit JsonCursor(const std::string& input)
        : input_(input)
    {
    }

    bool consume(char expected)
    {
        skipWhitespace();
        if (position_ >= input_.size() || input_[position_] != expected)
            return false;
        ++position_;
        return true;
    }

    bool atEnd()
    {
        skipWhitespace();
        return position_ == input_.size();
    }

    bool parseString(std::string& output)
    {
        skipWhitespace();
        if (position_ >= input_.size() || input_[position_] != '"')
            return false;
        ++position_;
        output.clear();

        while (position_ < input_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(input_[position_++]);
            if (character == '"') return true;
            if (character < 0x20U) return false;
            if (character != '\\')
            {
                output.push_back(static_cast<char>(character));
                continue;
            }

            if (position_ >= input_.size()) return false;
            const char escape = input_[position_++];
            switch (escape)
            {
                case '"': output.push_back('"'); break;
                case '\\': output.push_back('\\'); break;
                case '/': output.push_back('/'); break;
                case 'b': output.push_back('\b'); break;
                case 'f': output.push_back('\f'); break;
                case 'n': output.push_back('\n'); break;
                case 'r': output.push_back('\r'); break;
                case 't': output.push_back('\t'); break;
                default: return false;
            }
        }
        return false;
    }

    bool parseUnsigned(std::uint64_t& output)
    {
        skipWhitespace();
        if (position_ >= input_.size() ||
            !std::isdigit(static_cast<unsigned char>(input_[position_])))
            return false;

        std::uint64_t value = 0;
        while (position_ < input_.size() &&
               std::isdigit(static_cast<unsigned char>(input_[position_])))
        {
            const unsigned int digit =
                static_cast<unsigned int>(input_[position_] - '0');
            if (value >
                (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
                return false;
            value = value * 10U + digit;
            ++position_;
        }
        output = value;
        return true;
    }

    bool parseBool(bool& output)
    {
        skipWhitespace();
        if (input_.compare(position_, 4, "true") == 0)
        {
            position_ += 4;
            output = true;
            return true;
        }
        if (input_.compare(position_, 5, "false") == 0)
        {
            position_ += 5;
            output = false;
            return true;
        }
        return false;
    }

    bool skipValue(int depth = 0)
    {
        if (depth > 16) return false;
        skipWhitespace();
        if (position_ >= input_.size()) return false;

        if (input_[position_] == '"')
        {
            std::string ignored;
            return parseString(ignored);
        }
        if (input_[position_] == '{')
        {
            ++position_;
            bool first = true;
            while (true)
            {
                skipWhitespace();
                if (position_ < input_.size() && input_[position_] == '}')
                {
                    ++position_;
                    return true;
                }
                if (!first && !consume(',')) return false;
                std::string key;
                if (!parseString(key) || !consume(':') || !skipValue(depth + 1))
                    return false;
                first = false;
            }
        }
        if (input_[position_] == '[')
        {
            ++position_;
            bool first = true;
            while (true)
            {
                skipWhitespace();
                if (position_ < input_.size() && input_[position_] == ']')
                {
                    ++position_;
                    return true;
                }
                if (!first && !consume(',')) return false;
                if (!skipValue(depth + 1)) return false;
                first = false;
            }
        }
        bool ignoredBool = false;
        if (parseBool(ignoredBool)) return true;

        std::uint64_t ignoredNumber = 0;
        if (parseUnsigned(ignoredNumber)) return true;

        skipWhitespace();
        if (input_.compare(position_, 4, "null") == 0)
        {
            position_ += 4;
            return true;
        }
        return false;
    }

private:
    void skipWhitespace()
    {
        while (position_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position_])))
            ++position_;
    }

    const std::string& input_;
    std::size_t position_ = 0;
};

struct ApplicationWire
{
    std::uint64_t applicationId = 0;
    std::uint64_t controlCode = 0;
    std::uint64_t priority = 0;
    std::string name;
    bool haveApplicationId = false;
    bool haveControlCode = false;
    bool havePriority = false;
    bool haveName = false;
};

bool parseApplication(JsonCursor& cursor, ApplicationWire& application)
{
    if (!cursor.consume('{')) return false;
    bool first = true;
    while (true)
    {
        if (cursor.consume('}')) break;
        if (!first && !cursor.consume(',')) return false;

        std::string key;
        if (!cursor.parseString(key) || !cursor.consume(':')) return false;

        if (key == "applicationId")
        {
            application.haveApplicationId =
                cursor.parseUnsigned(application.applicationId);
            if (!application.haveApplicationId) return false;
        }
        else if (key == "controlCode")
        {
            application.haveControlCode =
                cursor.parseUnsigned(application.controlCode);
            if (!application.haveControlCode) return false;
        }
        else if (key == "priority")
        {
            application.havePriority =
                cursor.parseUnsigned(application.priority);
            if (!application.havePriority) return false;
        }
        else if (key == "name")
        {
            application.haveName = cursor.parseString(application.name);
            if (!application.haveName) return false;
        }
        else if (!cursor.skipValue())
        {
            return false;
        }
        first = false;
    }

    return application.haveApplicationId &&
        application.haveControlCode &&
        application.havePriority &&
        application.haveName;
}

bool parseApplications(
    JsonCursor& cursor,
    std::vector<ApplicationWire>& applications)
{
    if (!cursor.consume('[')) return false;
    applications.clear();
    bool first = true;
    while (true)
    {
        if (cursor.consume(']')) return true;
        if (!first && !cursor.consume(',')) return false;
        ApplicationWire application;
        if (!parseApplication(cursor, application)) return false;
        applications.push_back(std::move(application));
        if (applications.size() > kMaximumApplications) return false;
        first = false;
    }
}

struct DiscoveryWire
{
    std::uint64_t schemaVersion = 0;
    std::string provider;
    std::uint64_t providerSchemaVersion = 0;
    std::string capability;
    std::string result;
    std::uint64_t resultCode = 0;
    std::string channel;
    bool receiverActive = false;
    std::uint64_t revision = 0;
    std::uint64_t observedAt = 0;
    std::vector<ApplicationWire> applications;

    bool haveSchema = false;
    bool haveProvider = false;
    bool haveProviderSchema = false;
    bool haveCapability = false;
    bool haveResult = false;
    bool haveResultCode = false;
    bool haveChannel = false;
    bool haveReceiverActive = false;
    bool haveRevision = false;
    bool haveObservedAt = false;
    bool haveApplications = false;
};

bool parseDiscovery(const std::string& payload, DiscoveryWire& wire)
{
    JsonCursor cursor(payload);
    if (!cursor.consume('{')) return false;

    bool first = true;
    while (true)
    {
        if (cursor.consume('}')) break;
        if (!first && !cursor.consume(',')) return false;

        std::string key;
        if (!cursor.parseString(key) || !cursor.consume(':')) return false;

        if (key == "schemaVersion")
        {
            wire.haveSchema = cursor.parseUnsigned(wire.schemaVersion);
            if (!wire.haveSchema) return false;
        }
        else if (key == "provider")
        {
            wire.haveProvider = cursor.parseString(wire.provider);
            if (!wire.haveProvider) return false;
        }
        else if (key == "providerSchemaVersion")
        {
            wire.haveProviderSchema =
                cursor.parseUnsigned(wire.providerSchemaVersion);
            if (!wire.haveProviderSchema) return false;
        }
        else if (key == "capability")
        {
            wire.haveCapability = cursor.parseString(wire.capability);
            if (!wire.haveCapability) return false;
        }
        else if (key == "result")
        {
            wire.haveResult = cursor.parseString(wire.result);
            if (!wire.haveResult) return false;
        }
        else if (key == "resultCode")
        {
            wire.haveResultCode = cursor.parseUnsigned(wire.resultCode);
            if (!wire.haveResultCode) return false;
        }
        else if (key == "channel")
        {
            wire.haveChannel = cursor.parseString(wire.channel);
            if (!wire.haveChannel) return false;
        }
        else if (key == "receiverActive")
        {
            wire.haveReceiverActive = cursor.parseBool(wire.receiverActive);
            if (!wire.haveReceiverActive) return false;
        }
        else if (key == "revision")
        {
            wire.haveRevision = cursor.parseUnsigned(wire.revision);
            if (!wire.haveRevision) return false;
        }
        else if (key == "observedAt")
        {
            wire.haveObservedAt = cursor.parseUnsigned(wire.observedAt);
            if (!wire.haveObservedAt) return false;
        }
        else if (key == "applications")
        {
            wire.haveApplications =
                parseApplications(cursor, wire.applications);
            if (!wire.haveApplications) return false;
        }
        else if (!cursor.skipValue())
        {
            return false;
        }
        first = false;
    }

    return cursor.atEnd() &&
        wire.haveSchema &&
        wire.haveProvider &&
        wire.haveProviderSchema &&
        wire.haveCapability &&
        wire.haveResult &&
        wire.haveResultCode &&
        wire.haveChannel &&
        wire.haveReceiverActive &&
        wire.haveRevision &&
        wire.haveObservedAt &&
        wire.haveApplications;
}

bool acceptedResult(const std::string& result)
{
    return result == "ok" ||
        result == "no_applications" ||
        result == "receiver_inactive" ||
        result == "channel_mismatch" ||
        result == "no_live_service" ||
        result == "invalid_request";
}

} // namespace

SuiteBridgeHbbtvResolver::SuiteBridgeHbbtvResolver(
    ISuiteBridgeHbbtvTransport& transport)
    : transport_(transport)
{
}

BroadcastApplicationDiscoverySnapshot
SuiteBridgeHbbtvResolver::discoverApplications(
    const std::string& backendId,
    std::uint64_t backendGeneration,
    const std::string& channelId) const
{
    BroadcastApplicationDiscoverySnapshot snapshot;
    snapshot.channelId = channelId;

    if (!safeIdentity(backendId, 128) ||
        backendGeneration == 0 ||
        !safeIdentity(channelId, 128))
    {
        snapshot.error = "hbbtv_discovery_request_invalid";
        return snapshot;
    }

    const SuiteBridgeHbbtvCommandReply reply =
        transport_.discoverHbbtv(channelId);
    if (!reply.transportSucceeded || reply.replyCode != 250)
    {
        snapshot.error = "hbbtv_provider_unreachable";
        return snapshot;
    }

    DiscoveryWire wire;
    if (!parseDiscovery(reply.payload, wire) ||
        wire.schemaVersion != 1 ||
        wire.provider != "vdr-plugin-web" ||
        wire.providerSchemaVersion != 1 ||
        wire.capability != "broadcast.hbbtv.discovery" ||
        wire.channel != channelId ||
        !acceptedResult(wire.result) ||
        wire.resultCode > 5 ||
        wire.revision == 0)
    {
        snapshot.error = "hbbtv_discovery_payload_invalid";
        return snapshot;
    }

    snapshot.payloadValid = true;
    snapshot.receiverActive = wire.receiverActive;
    snapshot.result = wire.result;
    snapshot.revision = wire.revision;
    snapshot.observedAt = wire.observedAt;

    if (wire.result != "ok")
        return snapshot;

    for (const ApplicationWire& application : wire.applications)
    {
        if (application.applicationId == 0 ||
            application.applicationId >
                std::numeric_limits<std::uint32_t>::max() ||
            application.controlCode > 0xffU ||
            application.priority > 0xffU ||
            !safeText(application.name, 127))
        {
            snapshot.payloadValid = false;
            snapshot.applications.clear();
            snapshot.error = "hbbtv_application_descriptor_invalid";
            return snapshot;
        }

        BroadcastApplicationDescriptor descriptor;
        descriptor.ref.backendId = backendId;
        descriptor.ref.backendGeneration = backendGeneration;
        descriptor.ref.channelId = channelId;
        descriptor.ref.provider.providerId = "vdr-plugin-web";
        descriptor.ref.provider.providerSchemaVersion = 1;
        descriptor.ref.provider.capabilityRevision = wire.revision;
        descriptor.ref.provider.observedAt = wire.observedAt;
        descriptor.ref.applicationId =
            static_cast<std::uint32_t>(application.applicationId);
        descriptor.ref.descriptorRevision = wire.revision;
        descriptor.controlCode =
            static_cast<std::uint8_t>(application.controlCode);
        descriptor.priority =
            static_cast<std::uint8_t>(application.priority);
        descriptor.name = application.name;
        snapshot.applications.push_back(std::move(descriptor));
    }

    if (snapshot.applications.empty())
        snapshot.result = "no_applications";

    return snapshot;
}
