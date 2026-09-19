#include "SuiteBridgeHbbtvRuntimeResolver.h"

#include <cctype>
#include <cstdint>
#include <limits>
#include <string>

namespace
{

class JsonCursor
{
public:
    explicit JsonCursor(const std::string& input) : input_(input) {}

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

        std::uint64_t ignored = 0;
        if (parseUnsigned(ignored)) return true;

        skipWhitespace();
        if (input_.compare(position_, 4, "true") == 0)
        {
            position_ += 4;
            return true;
        }
        if (input_.compare(position_, 5, "false") == 0)
        {
            position_ += 5;
            return true;
        }
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

struct RuntimeWire
{
    std::uint64_t schemaVersion = 0;
    std::string provider;
    std::uint64_t providerSchemaVersion = 0;
    std::string capability;
    std::string operation;
    std::string result;
    std::uint64_t resultCode = 0;
    std::string state;
    std::uint64_t stateCode = 0;
    std::string sessionId;
    std::string channel;
    std::uint64_t applicationId = 0;
    std::uint64_t descriptorRevision = 0;
    std::string action;

    bool haveSchema = false;
    bool haveProvider = false;
    bool haveProviderSchema = false;
    bool haveCapability = false;
    bool haveOperation = false;
    bool haveResult = false;
    bool haveResultCode = false;
    bool haveState = false;
    bool haveStateCode = false;
    bool haveSessionId = false;
    bool haveChannel = false;
    bool haveApplicationId = false;
    bool haveDescriptorRevision = false;
    bool haveAction = false;
};

bool parseRuntime(const std::string& payload, RuntimeWire& wire)
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
            wire.haveSchema = cursor.parseUnsigned(wire.schemaVersion);
        else if (key == "provider")
            wire.haveProvider = cursor.parseString(wire.provider);
        else if (key == "providerSchemaVersion")
            wire.haveProviderSchema =
                cursor.parseUnsigned(wire.providerSchemaVersion);
        else if (key == "capability")
            wire.haveCapability = cursor.parseString(wire.capability);
        else if (key == "operation")
            wire.haveOperation = cursor.parseString(wire.operation);
        else if (key == "result")
            wire.haveResult = cursor.parseString(wire.result);
        else if (key == "resultCode")
            wire.haveResultCode = cursor.parseUnsigned(wire.resultCode);
        else if (key == "state")
            wire.haveState = cursor.parseString(wire.state);
        else if (key == "stateCode")
            wire.haveStateCode = cursor.parseUnsigned(wire.stateCode);
        else if (key == "sessionId")
            wire.haveSessionId = cursor.parseString(wire.sessionId);
        else if (key == "channel")
            wire.haveChannel = cursor.parseString(wire.channel);
        else if (key == "applicationId")
            wire.haveApplicationId = cursor.parseUnsigned(wire.applicationId);
        else if (key == "descriptorRevision")
            wire.haveDescriptorRevision =
                cursor.parseUnsigned(wire.descriptorRevision);
        else if (key == "action")
            wire.haveAction = cursor.parseString(wire.action);
        else if (!cursor.skipValue())
            return false;

        if ((key == "schemaVersion" && !wire.haveSchema) ||
            (key == "provider" && !wire.haveProvider) ||
            (key == "providerSchemaVersion" && !wire.haveProviderSchema) ||
            (key == "capability" && !wire.haveCapability) ||
            (key == "operation" && !wire.haveOperation) ||
            (key == "result" && !wire.haveResult) ||
            (key == "resultCode" && !wire.haveResultCode) ||
            (key == "state" && !wire.haveState) ||
            (key == "stateCode" && !wire.haveStateCode) ||
            (key == "sessionId" && !wire.haveSessionId) ||
            (key == "channel" && !wire.haveChannel) ||
            (key == "applicationId" && !wire.haveApplicationId) ||
            (key == "descriptorRevision" && !wire.haveDescriptorRevision) ||
            (key == "action" && !wire.haveAction))
            return false;

        first = false;
    }

    return cursor.atEnd() &&
        wire.haveSchema &&
        wire.haveProvider &&
        wire.haveProviderSchema &&
        wire.haveCapability &&
        wire.haveOperation &&
        wire.haveResult &&
        wire.haveResultCode &&
        wire.haveState &&
        wire.haveStateCode &&
        wire.haveSessionId &&
        wire.haveChannel &&
        wire.haveApplicationId &&
        wire.haveDescriptorRevision;
}

const char* operationName(SuiteBridgeHbbtvRuntimeOperation operation)
{
    switch (operation)
    {
        case SuiteBridgeHbbtvRuntimeOperation::Launch: return "launch";
        case SuiteBridgeHbbtvRuntimeOperation::Status: return "status";
        case SuiteBridgeHbbtvRuntimeOperation::Input: return "input";
        case SuiteBridgeHbbtvRuntimeOperation::Close: return "close";
    }
    return nullptr;
}

const char* actionName(SuiteBridgeHbbtvInputAction action)
{
    switch (action)
    {
        case SuiteBridgeHbbtvInputAction::None: return nullptr;
        case SuiteBridgeHbbtvInputAction::Up: return "UP";
        case SuiteBridgeHbbtvInputAction::Down: return "DOWN";
        case SuiteBridgeHbbtvInputAction::Left: return "LEFT";
        case SuiteBridgeHbbtvInputAction::Right: return "RIGHT";
        case SuiteBridgeHbbtvInputAction::Ok: return "OK";
        case SuiteBridgeHbbtvInputAction::Back: return "BACK";
        case SuiteBridgeHbbtvInputAction::Red: return "RED";
        case SuiteBridgeHbbtvInputAction::Green: return "GREEN";
        case SuiteBridgeHbbtvInputAction::Yellow: return "YELLOW";
        case SuiteBridgeHbbtvInputAction::Blue: return "BLUE";
        case SuiteBridgeHbbtvInputAction::Digit0: return "0";
        case SuiteBridgeHbbtvInputAction::Digit1: return "1";
        case SuiteBridgeHbbtvInputAction::Digit2: return "2";
        case SuiteBridgeHbbtvInputAction::Digit3: return "3";
        case SuiteBridgeHbbtvInputAction::Digit4: return "4";
        case SuiteBridgeHbbtvInputAction::Digit5: return "5";
        case SuiteBridgeHbbtvInputAction::Digit6: return "6";
        case SuiteBridgeHbbtvInputAction::Digit7: return "7";
        case SuiteBridgeHbbtvInputAction::Digit8: return "8";
        case SuiteBridgeHbbtvInputAction::Digit9: return "9";
        case SuiteBridgeHbbtvInputAction::Play: return "PLAY";
        case SuiteBridgeHbbtvInputAction::Pause: return "PAUSE";
        case SuiteBridgeHbbtvInputAction::Stop: return "STOP";
        case SuiteBridgeHbbtvInputAction::FastForward: return "FAST_FORWARD";
        case SuiteBridgeHbbtvInputAction::Rewind: return "REWIND";
    }
    return nullptr;
}

const char* resultName(std::uint64_t code)
{
    switch (code)
    {
        case 0: return "ok";
        case 1: return "accepted";
        case 2: return "invalid_request";
        case 3: return "discovery_stale";
        case 4: return "application_not_launchable";
        case 5: return "busy";
        case 6: return "session_not_active";
        case 7: return "action_unsupported";
        case 8: return "runtime_unavailable";
    }
    return nullptr;
}

const char* stateName(std::uint64_t code)
{
    switch (code)
    {
        case 0: return "none";
        case 1: return "starting";
        case 2: return "active";
        case 3: return "closing";
        case 4: return "failed";
    }
    return nullptr;
}

bool validResult(const RuntimeWire& wire)
{
    const char* expectedResult = resultName(wire.resultCode);
    const char* expectedState = stateName(wire.stateCode);
    return expectedResult != nullptr &&
        expectedState != nullptr &&
        wire.result == expectedResult &&
        wire.state == expectedState;
}

} // namespace

SuiteBridgeHbbtvRuntimeResolver::SuiteBridgeHbbtvRuntimeResolver(
    ISuiteBridgeHbbtvTransport& transport)
    : transport_(transport)
{
}

SuiteBridgeHbbtvRuntimeResolution SuiteBridgeHbbtvRuntimeResolver::control(
    const SuiteBridgeHbbtvRuntimeRequest& request)
{
    SuiteBridgeHbbtvRuntimeResolution result;

    const SuiteBridgeHbbtvCommandReply reply = transport_.controlHbbtv(request);
    if (!reply.transportSucceeded || reply.replyCode != 250)
    {
        result.error = "hbbtv_runtime_provider_unreachable";
        return result;
    }

    RuntimeWire wire;
    if (!parseRuntime(reply.payload, wire) ||
        wire.schemaVersion != 1 ||
        wire.provider != "vdr-plugin-web" ||
        wire.providerSchemaVersion != 1 ||
        wire.capability != "broadcast.hbbtv.runtime" ||
        !validResult(wire) ||
        wire.applicationId > std::numeric_limits<std::uint32_t>::max())
    {
        result.error = "hbbtv_runtime_payload_invalid";
        return result;
    }

    const char* expectedOperation = operationName(request.operation);
    if (expectedOperation == nullptr ||
        wire.operation != expectedOperation ||
        wire.sessionId != request.sessionId ||
        wire.channel != request.channelId ||
        wire.applicationId != request.applicationId ||
        wire.descriptorRevision != request.descriptorRevision)
    {
        result.error = "hbbtv_runtime_identity_mismatch";
        return result;
    }

    if (request.operation == SuiteBridgeHbbtvRuntimeOperation::Input)
    {
        const char* expectedAction = actionName(request.inputAction);
        if (expectedAction == nullptr ||
            !wire.haveAction ||
            wire.action != expectedAction)
        {
            result.error = "hbbtv_runtime_action_mismatch";
            return result;
        }
    }
    else if (wire.haveAction)
    {
        result.error = "hbbtv_runtime_payload_invalid";
        return result;
    }

    result.payloadValid = true;
    result.operation = wire.operation;
    result.result = wire.result;
    result.resultCode = static_cast<std::uint8_t>(wire.resultCode);
    result.state = wire.state;
    result.stateCode = static_cast<std::uint8_t>(wire.stateCode);
    result.sessionId = wire.sessionId;
    result.channelId = wire.channel;
    result.applicationId = static_cast<std::uint32_t>(wire.applicationId);
    result.descriptorRevision = wire.descriptorRevision;
    result.action = wire.action;
    return result;
}
