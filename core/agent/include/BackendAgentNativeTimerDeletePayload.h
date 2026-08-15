#pragma once

#include "BackendAgentNativeTimerDelete.h"
#include "BackendAgentNativeTimerDeleteFingerprint.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace vdrsuite::agent
{

struct BackendAgentNativeTimerDeletePayload
{
    std::string operationRevision;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::string expectedNativeTimerFingerprint;
    std::string timerAssignmentId;
    std::string backendNativeTimerId;
    std::int64_t controlPlaneClaimedAt = 0;
    BackendAgentLocalProviderSelection localProviderSelection;
};

namespace native_timer_delete_payload_detail
{

enum class ValueKind
{
    String,
    Unsigned,
};

struct Value
{
    ValueKind kind = ValueKind::String;
    std::string stringValue;
    std::uint64_t unsignedValue = 0;
};

class FlatJsonParser
{
public:
    explicit FlatJsonParser(const std::string& input) : input_(input) {}

    bool parse(std::map<std::string, Value>& values)
    {
        values.clear();
        if (input_.empty() || input_.size() > 4096) return false;
        skip();
        if (!consume('{')) return false;
        skip();
        if (consume('}')) return position_ == input_.size();
        while (position_ < input_.size())
        {
            std::string key;
            if (!parseString(key) || key.empty() || key.size() > 64) return false;
            skip();
            if (!consume(':')) return false;
            skip();
            Value value;
            if (!parseValue(value) ||
                !values.emplace(key, std::move(value)).second ||
                values.size() > 24)
                return false;
            skip();
            if (consume('}'))
            {
                skip();
                return position_ == input_.size();
            }
            if (!consume(',')) return false;
            skip();
        }
        return false;
    }

private:
    void skip()
    {
        while (position_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position_])))
            ++position_;
    }

    bool consume(char expected)
    {
        if (position_ >= input_.size() || input_[position_] != expected)
            return false;
        ++position_;
        return true;
    }

    bool parseString(std::string& value)
    {
        value.clear();
        if (!consume('"')) return false;
        while (position_ < input_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(input_[position_++]);
            if (character == '"') return true;
            if (character < 0x20U || character == 0x7fU) return false;
            if (character == '\\')
            {
                if (position_ >= input_.size()) return false;
                const char escaped = input_[position_++];
                if (escaped == '"' || escaped == '\\' || escaped == '/')
                    value.push_back(escaped);
                else
                    return false;
            }
            else
            {
                value.push_back(static_cast<char>(character));
            }
            if (value.size() > 1024) return false;
        }
        return false;
    }

    bool parseValue(Value& value)
    {
        if (position_ >= input_.size()) return false;
        if (input_[position_] == '"')
        {
            value.kind = ValueKind::String;
            return parseString(value.stringValue);
        }

        const std::size_t start = position_;
        while (position_ < input_.size() &&
               std::isdigit(static_cast<unsigned char>(input_[position_])))
            ++position_;
        if (start == position_ || position_ - start > 19 ||
            (position_ - start > 1 && input_[start] == '0'))
            return false;

        std::uint64_t parsed = 0;
        for (std::size_t index = start; index < position_; ++index)
        {
            const unsigned digit = static_cast<unsigned>(input_[index] - '0');
            if (parsed >
                (static_cast<std::uint64_t>(
                     std::numeric_limits<std::int64_t>::max()) -
                 digit) /
                    10U)
                return false;
            parsed = parsed * 10U + digit;
        }
        value.kind = ValueKind::Unsigned;
        value.unsignedValue = parsed;
        return true;
    }

    const std::string& input_;
    std::size_t position_ = 0;
};

inline bool exactKeys(
    const std::map<std::string, Value>& values,
    const std::vector<std::string>& expected)
{
    if (values.size() != expected.size()) return false;
    return std::all_of(expected.begin(), expected.end(), [&](const std::string& key) {
        return values.count(key) == 1;
    });
}

inline const Value* get(
    const std::map<std::string, Value>& values,
    const char* key,
    ValueKind kind)
{
    const auto found = values.find(key);
    return found != values.end() && found->second.kind == kind
        ? &found->second
        : nullptr;
}

inline bool safeToken(const std::string& value, std::size_t maximum)
{
    if (value.empty() || value.size() > maximum) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' ||
            character == '_' || character == '.' || character == ':';
    });
}

inline bool safeIdentifier(const std::string& value)
{
    return safeToken(value, 128);
}

inline bool exactProviderSelection(
    const BackendAgentLocalProviderSelection& selection)
{
    return backendAgentLocalProviderValidSelection(selection) &&
        selection.authorityDomain ==
            kBackendAgentNativeTimerDeleteAuthorityDomain &&
        selection.providerId == kBackendAgentNativeTimerDeleteProviderId &&
        selection.providerKind == kBackendAgentNativeTimerDeleteProviderKind &&
        selection.requiredCapability ==
            kBackendAgentNativeTimerDeleteCapability;
}

inline bool commonValid(const BackendAgentNativeTimerDeletePayload& payload)
{
    return safeIdentifier(payload.operationRevision) &&
        safeIdentifier(payload.nativeTimerBindingId) &&
        safeIdentifier(payload.expectedBindingRevision) &&
        safeIdentifier(payload.timerAssignmentId) &&
        safeIdentifier(payload.backendNativeTimerId) &&
        payload.controlPlaneClaimedAt > 0 &&
        exactProviderSelection(payload.localProviderSelection);
}

inline bool valid(const BackendAgentNativeTimerDeletePayload& payload)
{
    return commonValid(payload) &&
        backendAgentNativeTimerDeleteFingerprintTokenValid(
            payload.expectedNativeTimerFingerprint);
}

inline bool serializable(const BackendAgentNativeTimerDeletePayload& payload)
{
    return commonValid(payload) &&
        (backendAgentNativeTimerDeleteFingerprintTokenValid(
             payload.expectedNativeTimerFingerprint) ||
         backendAgentNativeTimerDeleteCanonicalFingerprintValid(
             payload.expectedNativeTimerFingerprint));
}

inline std::string transportFingerprint(
    const BackendAgentNativeTimerDeletePayload& payload)
{
    if (backendAgentNativeTimerDeleteFingerprintTokenValid(
            payload.expectedNativeTimerFingerprint))
        return payload.expectedNativeTimerFingerprint;
    return backendAgentNativeTimerDeleteFingerprintToken(
        payload.expectedNativeTimerFingerprint);
}

} // namespace native_timer_delete_payload_detail

inline std::string backendAgentNativeTimerDeletePayload(
    const BackendAgentNativeTimerDeletePayload& payload)
{
    using namespace native_timer_delete_payload_detail;
    if (!serializable(payload)) return {};
    const std::string fingerprint = transportFingerprint(payload);
    if (!backendAgentNativeTimerDeleteFingerprintTokenValid(fingerprint)) return {};
    const auto& selection = payload.localProviderSelection;
    std::ostringstream output;
    output << "{\"timerDeleteSchema\":1"
           << ",\"operationRevision\":\"" << payload.operationRevision << "\""
           << ",\"nativeTimerBindingId\":\"" << payload.nativeTimerBindingId << "\""
           << ",\"expectedBindingRevision\":\"" << payload.expectedBindingRevision << "\""
           << ",\"expectedNativeTimerFingerprint\":\""
           << fingerprint << "\""
           << ",\"timerAssignmentId\":\"" << payload.timerAssignmentId << "\""
           << ",\"backendNativeTimerId\":\"" << payload.backendNativeTimerId << "\""
           << ",\"controlPlaneClaimedAt\":" << payload.controlPlaneClaimedAt
           << ",\"backendId\":\"" << selection.backendId << "\""
           << ",\"authorityDomain\":\"" << selection.authorityDomain << "\""
           << ",\"providerId\":\"" << selection.providerId << "\""
           << ",\"providerKind\":\"" << selection.providerKind << "\""
           << ",\"ownershipGeneration\":" << selection.ownershipGeneration
           << ",\"providerInstanceEpoch\":\"" << selection.providerInstanceEpoch << "\""
           << ",\"providerGeneration\":" << selection.providerGeneration
           << ",\"capabilityRevision\":" << selection.capabilityRevision
           << ",\"requiredCapability\":\"" << selection.requiredCapability << "\"}"
           ;
    return output.str();
}

inline bool backendAgentNativeTimerDeleteParsePayload(
    const std::string& encoded,
    BackendAgentNativeTimerDeletePayload& payload,
    std::string& reasonCode)
{
    using namespace native_timer_delete_payload_detail;
    std::map<std::string, Value> values;
    if (!FlatJsonParser(encoded).parse(values) ||
        !exactKeys(values, {
            "timerDeleteSchema", "operationRevision", "nativeTimerBindingId",
            "expectedBindingRevision", "expectedNativeTimerFingerprint",
            "timerAssignmentId", "backendNativeTimerId", "controlPlaneClaimedAt",
            "backendId", "authorityDomain", "providerId", "providerKind",
            "ownershipGeneration", "providerInstanceEpoch", "providerGeneration",
            "capabilityRevision", "requiredCapability"}))
    {
        reasonCode = "invalid_native_timer_delete_payload";
        return false;
    }

    const Value* schema = get(values, "timerDeleteSchema", ValueKind::Unsigned);
    const Value* operationRevision =
        get(values, "operationRevision", ValueKind::String);
    const Value* bindingId =
        get(values, "nativeTimerBindingId", ValueKind::String);
    const Value* bindingRevision =
        get(values, "expectedBindingRevision", ValueKind::String);
    const Value* nativeFingerprint =
        get(values, "expectedNativeTimerFingerprint", ValueKind::String);
    const Value* assignmentId =
        get(values, "timerAssignmentId", ValueKind::String);
    const Value* nativeId =
        get(values, "backendNativeTimerId", ValueKind::String);
    const Value* claimedAt =
        get(values, "controlPlaneClaimedAt", ValueKind::Unsigned);
    const Value* backendId = get(values, "backendId", ValueKind::String);
    const Value* authority = get(values, "authorityDomain", ValueKind::String);
    const Value* providerId = get(values, "providerId", ValueKind::String);
    const Value* providerKind = get(values, "providerKind", ValueKind::String);
    const Value* ownership =
        get(values, "ownershipGeneration", ValueKind::Unsigned);
    const Value* providerEpoch =
        get(values, "providerInstanceEpoch", ValueKind::String);
    const Value* providerGeneration =
        get(values, "providerGeneration", ValueKind::Unsigned);
    const Value* capabilityRevision =
        get(values, "capabilityRevision", ValueKind::Unsigned);
    const Value* requiredCapability =
        get(values, "requiredCapability", ValueKind::String);

    if (!schema || schema->unsignedValue != 1 || !operationRevision || !bindingId ||
        !bindingRevision || !nativeFingerprint || !assignmentId || !nativeId ||
        !claimedAt || !backendId || !authority || !providerId || !providerKind ||
        !ownership || !providerEpoch || !providerGeneration || !capabilityRevision ||
        !requiredCapability)
    {
        reasonCode = "invalid_native_timer_delete_payload";
        return false;
    }

    BackendAgentNativeTimerDeletePayload candidate;
    candidate.operationRevision = operationRevision->stringValue;
    candidate.nativeTimerBindingId = bindingId->stringValue;
    candidate.expectedBindingRevision = bindingRevision->stringValue;
    candidate.expectedNativeTimerFingerprint = nativeFingerprint->stringValue;
    candidate.timerAssignmentId = assignmentId->stringValue;
    candidate.backendNativeTimerId = nativeId->stringValue;
    candidate.controlPlaneClaimedAt =
        static_cast<std::int64_t>(claimedAt->unsignedValue);
    candidate.localProviderSelection.backendId = backendId->stringValue;
    candidate.localProviderSelection.authorityDomain = authority->stringValue;
    candidate.localProviderSelection.providerId = providerId->stringValue;
    candidate.localProviderSelection.providerKind = providerKind->stringValue;
    candidate.localProviderSelection.ownershipGeneration = ownership->unsignedValue;
    candidate.localProviderSelection.providerInstanceEpoch = providerEpoch->stringValue;
    candidate.localProviderSelection.providerGeneration = providerGeneration->unsignedValue;
    candidate.localProviderSelection.capabilityRevision = capabilityRevision->unsignedValue;
    candidate.localProviderSelection.requiredCapability =
        requiredCapability->stringValue;

    if (!valid(candidate))
    {
        reasonCode = "invalid_native_timer_delete_payload";
        return false;
    }

    payload = candidate;
    reasonCode.clear();
    return true;
}

} // namespace vdrsuite::agent
