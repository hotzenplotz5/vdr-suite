#include "BackendAgentNativeProbe.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace vdrsuite::agent
{
namespace
{
enum class ValueKind { String, Unsigned, Boolean };
struct Value
{
    ValueKind kind = ValueKind::String;
    std::string stringValue;
    std::uint64_t unsignedValue = 0;
    bool booleanValue = false;
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
            if (!parseValue(value) || !values.emplace(key, std::move(value)).second ||
                values.size() > 24) return false;
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
               std::isspace(static_cast<unsigned char>(input_[position_]))) ++position_;
    }

    bool consume(char expected)
    {
        if (position_ >= input_.size() || input_[position_] != expected) return false;
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
                else return false;
            }
            else value.push_back(static_cast<char>(character));
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
        if (input_.compare(position_, 4, "true") == 0)
        {
            value.kind = ValueKind::Boolean;
            value.booleanValue = true;
            position_ += 4;
            return true;
        }
        if (input_.compare(position_, 5, "false") == 0)
        {
            value.kind = ValueKind::Boolean;
            value.booleanValue = false;
            position_ += 5;
            return true;
        }
        const std::size_t start = position_;
        while (position_ < input_.size() &&
               std::isdigit(static_cast<unsigned char>(input_[position_]))) ++position_;
        if (start == position_ || position_ - start > 19 ||
            (position_ - start > 1 && input_[start] == '0')) return false;
        std::uint64_t parsed = 0;
        for (std::size_t index = start; index < position_; ++index)
        {
            const unsigned digit = static_cast<unsigned>(input_[index] - '0');
            if (parsed >
                (static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) - digit) / 10U)
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

bool exactKeys(
    const std::map<std::string, Value>& values,
    const std::vector<std::string>& expected)
{
    if (values.size() != expected.size()) return false;
    return std::all_of(expected.begin(), expected.end(), [&](const std::string& key) {
        return values.count(key) == 1;
    });
}

const Value* get(
    const std::map<std::string, Value>& values,
    const char* key,
    ValueKind kind)
{
    const auto found = values.find(key);
    return found != values.end() && found->second.kind == kind
        ? &found->second : nullptr;
}

bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' ||
            character == '_' || character == '.' || character == ':';
    });
}

std::string escape(const std::string& value)
{
    std::ostringstream output;
    for (unsigned char character : value)
    {
        if (character == '"') output << "\\\"";
        else if (character == '\\') output << "\\\\";
        else if (character >= 0x20U && character != 0x7fU)
            output << static_cast<char>(character);
    }
    return output.str();
}
}

bool backendAgentNativeProbeParsePayload(
    const std::string& payload,
    std::string& probeNonce)
{
    std::map<std::string, Value> values;
    if (!FlatJsonParser(payload).parse(values) ||
        !exactKeys(values, {"probeSchema", "probeNonce"})) return false;
    const Value* schema = get(values, "probeSchema", ValueKind::Unsigned);
    const Value* nonce = get(values, "probeNonce", ValueKind::String);
    if (!schema || schema->unsignedValue != 1 || !nonce || !safeIdentifier(nonce->stringValue)) return false;
    probeNonce = nonce->stringValue;
    return true;
}

bool backendAgentNativeProbeParseCapability(
    const std::string& payload,
    SuiteBridgeNativeProbeCapability& capability,
    std::string& reasonCode)
{
    std::map<std::string, Value> values;
    if (!FlatJsonParser(payload).parse(values) ||
        !exactKeys(values, {
            "nativeOperation", "nativeOperationSchema", "sideEffectClass",
            "mutations", "localProviderKind", "pluginInstanceEpoch"}))
    {
        reasonCode = "invalid_native_capability_payload";
        return false;
    }
    const Value* operation = get(values, "nativeOperation", ValueKind::String);
    const Value* schema = get(values, "nativeOperationSchema", ValueKind::Unsigned);
    const Value* sideEffect = get(values, "sideEffectClass", ValueKind::String);
    const Value* mutations = get(values, "mutations", ValueKind::String);
    const Value* provider = get(values, "localProviderKind", ValueKind::String);
    const Value* epoch = get(values, "pluginInstanceEpoch", ValueKind::String);
    if (!operation || !schema || !sideEffect || !mutations || !provider || !epoch)
    {
        reasonCode = "invalid_native_capability_payload";
        return false;
    }
    capability.nativeOperation = operation->stringValue;
    capability.nativeOperationSchema = schema->unsignedValue;
    capability.sideEffectClass = sideEffect->stringValue;
    capability.mutations = mutations->stringValue;
    capability.localProviderKind = provider->stringValue;
    capability.pluginInstanceEpoch = epoch->stringValue;
    if (!backendAgentNativeProbeCapabilityCompatible(capability))
    {
        reasonCode = "incompatible_native_capability";
        return false;
    }
    reasonCode = "native_capability_compatible";
    return true;
}

bool backendAgentNativeProbeParseEvidence(
    const std::string& payload,
    bool requireReadback,
    SuiteBridgeNativeProbeEvidence& evidence,
    std::string& reasonCode)
{
    std::map<std::string, Value> values;
    const std::vector<std::string> common = {
        "commandId", "requestFingerprint", "nativeOperation",
        "nativeOperationSchema", "pluginInstanceEpoch",
        "nativeExecutionSequence", "receiptCategory", "acceptedAt",
        "sideEffectClass", "resultCategory", "vdrActive",
        "mutationsState", "sideEffectObserved", "boundedDiagnostics",
        "completedAt"};
    std::vector<std::string> expected = common;
    if (requireReadback)
    {
        expected.push_back("readbackCategory");
        expected.push_back("duplicateDisposition");
    }
    if (!FlatJsonParser(payload).parse(values) || !exactKeys(values, expected))
    {
        reasonCode = "invalid_native_evidence_payload";
        return false;
    }
    const auto stringValue = [&](const char* key) {
        return get(values, key, ValueKind::String);
    };
    const auto unsignedValue = [&](const char* key) {
        return get(values, key, ValueKind::Unsigned);
    };
    const auto booleanValue = [&](const char* key) {
        return get(values, key, ValueKind::Boolean);
    };
    const Value *command=stringValue("commandId"),
        *fingerprint=stringValue("requestFingerprint"),
        *operation=stringValue("nativeOperation"),
        *schema=unsignedValue("nativeOperationSchema"),
        *epoch=stringValue("pluginInstanceEpoch"),
        *sequence=unsignedValue("nativeExecutionSequence"),
        *receipt=stringValue("receiptCategory"),
        *accepted=unsignedValue("acceptedAt"),
        *sideEffect=stringValue("sideEffectClass"),
        *result=stringValue("resultCategory"),
        *active=booleanValue("vdrActive"),
        *mutations=stringValue("mutationsState"),
        *observed=booleanValue("sideEffectObserved"),
        *diagnostics=stringValue("boundedDiagnostics"),
        *completed=unsignedValue("completedAt");
    if (!command||!fingerprint||!operation||!schema||!epoch||!sequence||
        !receipt||!accepted||!sideEffect||!result||!active||!mutations||
        !observed||!diagnostics||!completed)
    {
        reasonCode = "invalid_native_evidence_payload";
        return false;
    }
    evidence.commandId=command->stringValue;
    evidence.requestFingerprint=fingerprint->stringValue;
    evidence.nativeOperation=operation->stringValue;
    evidence.nativeOperationSchema=schema->unsignedValue;
    evidence.pluginInstanceEpoch=epoch->stringValue;
    evidence.nativeExecutionSequence=sequence->unsignedValue;
    evidence.receiptCategory=receipt->stringValue;
    evidence.acceptedAt=static_cast<std::int64_t>(accepted->unsignedValue);
    evidence.sideEffectClass=sideEffect->stringValue;
    evidence.resultCategory=result->stringValue;
    evidence.vdrActive=active->booleanValue;
    evidence.mutationsState=mutations->stringValue;
    evidence.sideEffectObserved=observed->booleanValue;
    evidence.boundedDiagnostics=diagnostics->stringValue;
    evidence.completedAt=static_cast<std::int64_t>(completed->unsignedValue);
    if (requireReadback)
    {
        const Value* readback=stringValue("readbackCategory");
        const Value* duplicate=stringValue("duplicateDisposition");
        if (!readback || !duplicate)
        {
            reasonCode = "invalid_native_evidence_payload";
            return false;
        }
        evidence.readbackCategory=readback->stringValue;
        evidence.duplicateDisposition=duplicate->stringValue;
    }
    if (!safeIdentifier(evidence.commandId) ||
        !safeIdentifier(evidence.requestFingerprint) ||
        !safeIdentifier(evidence.pluginInstanceEpoch) ||
        evidence.nativeExecutionSequence == 0 || evidence.acceptedAt <= 0 ||
        evidence.completedAt <= 0 ||
        evidence.nativeOperation != "vdr.native.probe" ||
        evidence.nativeOperationSchema != 1 ||
        evidence.sideEffectClass != "none" ||
        evidence.resultCategory != "succeeded" ||
        !evidence.vdrActive || evidence.mutationsState != "disabled" ||
        evidence.sideEffectObserved ||
        (evidence.receiptCategory != "accepted" &&
         evidence.receiptCategory != "duplicate") ||
        (requireReadback &&
         (evidence.readbackCategory != "verified" ||
          evidence.duplicateDisposition != "exact_replay")))
    {
        reasonCode = "native_evidence_invariant_failed";
        return false;
    }
    reasonCode = requireReadback
        ? "native_readback_verified" : "native_result_verified";
    return true;
}

bool backendAgentNativeProbeCapabilityCompatible(
    const SuiteBridgeNativeProbeCapability& capability)
{
    return capability.nativeOperation == "vdr.native.probe" &&
        capability.nativeOperationSchema == 1 &&
        capability.sideEffectClass == "none" &&
        capability.mutations == "disabled" &&
        capability.localProviderKind == "suitebridge" &&
        safeIdentifier(capability.pluginInstanceEpoch);
}

bool backendAgentNativeProbeEvidenceMatches(
    const SuiteBridgeNativeProbeEvidence& evidence,
    const SuiteBridgeNativeProbeRequest& request,
    bool requireReadback)
{
    return evidence.commandId == request.commandId &&
        evidence.requestFingerprint == request.requestFingerprint &&
        evidence.nativeOperation == "vdr.native.probe" &&
        evidence.nativeOperationSchema == 1 &&
        evidence.pluginInstanceEpoch == request.pluginInstanceEpoch &&
        evidence.nativeExecutionSequence > 0 &&
        evidence.vdrActive && evidence.mutationsState == "disabled" &&
        evidence.sideEffectClass == "none" &&
        !evidence.sideEffectObserved &&
        (!requireReadback ||
         (evidence.readbackCategory == "verified" &&
          evidence.duplicateDisposition == "exact_replay"));
}

std::string backendAgentNativeProbeCapabilityEvidence(
    const SuiteBridgeNativeProbeCapability& capability)
{
    std::ostringstream output;
    output << "{\"nativeOperation\":\"" << escape(capability.nativeOperation)
           << "\",\"nativeOperationSchema\":" << capability.nativeOperationSchema
           << ",\"sideEffectClass\":\"" << escape(capability.sideEffectClass)
           << "\",\"mutations\":\"" << escape(capability.mutations)
           << "\",\"localProviderKind\":\"" << escape(capability.localProviderKind)
           << "\",\"pluginInstanceEpoch\":\""
           << escape(capability.pluginInstanceEpoch) << "\"}";
    return output.str();
}


std::string backendAgentNativeProbeReceiptEvidence(
    const SuiteBridgeNativeProbeEvidence& evidence)
{
    std::ostringstream output;
    output << "{\"commandId\":\"" << escape(evidence.commandId)
           << "\",\"requestFingerprint\":\""
           << escape(evidence.requestFingerprint)
           << "\",\"nativeOperation\":\""
           << escape(evidence.nativeOperation)
           << "\",\"nativeOperationSchema\":"
           << evidence.nativeOperationSchema
           << ",\"pluginInstanceEpoch\":\""
           << escape(evidence.pluginInstanceEpoch)
           << "\",\"nativeExecutionSequence\":"
           << evidence.nativeExecutionSequence
           << ",\"receiptCategory\":\""
           << escape(evidence.receiptCategory)
           << "\",\"acceptedAt\":" << evidence.acceptedAt
           << ",\"sideEffectClass\":\""
           << escape(evidence.sideEffectClass) << "\"}";
    return output.str();
}

std::string backendAgentNativeProbeResultEvidence(
    const SuiteBridgeNativeProbeEvidence& evidence)
{
    std::ostringstream output;
    output << "{\"commandId\":\"" << escape(evidence.commandId)
           << "\",\"requestFingerprint\":\""
           << escape(evidence.requestFingerprint)
           << "\",\"nativeOperation\":\""
           << escape(evidence.nativeOperation)
           << "\",\"nativeOperationSchema\":"
           << evidence.nativeOperationSchema
           << ",\"pluginInstanceEpoch\":\""
           << escape(evidence.pluginInstanceEpoch)
           << "\",\"nativeExecutionSequence\":"
           << evidence.nativeExecutionSequence
           << ",\"resultCategory\":\""
           << escape(evidence.resultCategory)
           << "\",\"vdrActive\":"
           << (evidence.vdrActive ? "true" : "false")
           << ",\"mutationsState\":\""
           << escape(evidence.mutationsState)
           << "\",\"sideEffectObserved\":"
           << (evidence.sideEffectObserved ? "true" : "false")
           << ",\"boundedDiagnostics\":\""
           << escape(evidence.boundedDiagnostics)
           << "\",\"completedAt\":" << evidence.completedAt << '}';
    return output.str();
}

std::string backendAgentNativeProbeReadbackEvidence(
    const SuiteBridgeNativeProbeEvidence& evidence)
{
    std::ostringstream output;
    output << "{\"commandId\":\"" << escape(evidence.commandId)
           << "\",\"requestFingerprint\":\""
           << escape(evidence.requestFingerprint)
           << "\",\"nativeOperation\":\""
           << escape(evidence.nativeOperation)
           << "\",\"nativeOperationSchema\":"
           << evidence.nativeOperationSchema
           << ",\"pluginInstanceEpoch\":\""
           << escape(evidence.pluginInstanceEpoch)
           << "\",\"nativeExecutionSequence\":"
           << evidence.nativeExecutionSequence
           << ",\"vdrActive\":"
           << (evidence.vdrActive ? "true" : "false")
           << ",\"mutationsState\":\""
           << escape(evidence.mutationsState)
           << "\",\"sideEffectObserved\":"
           << (evidence.sideEffectObserved ? "true" : "false")
           << ",\"readbackCategory\":\""
           << escape(evidence.readbackCategory)
           << "\",\"duplicateDisposition\":\""
           << escape(evidence.duplicateDisposition) << "\"}";
    return output.str();
}

}
