#pragma once

#include <cstdint>
#include <string_view>

namespace vdrsuite::mediatools {

// Add-on-owned, zero-side-effect request shape. The Agent/Operations
// owner supplies these fields from an authenticated canonical assignment;
// never deserialize an actor's JSON directly into this authority context.
struct AgentImportEnvelope {
    std::string_view moduleId;
    std::string_view packageName;
    std::string_view commandType;
    std::string_view operationId;
    std::string_view commandId;
    std::string_view backendId;
    std::uint64_t operationGeneration = 0;
    std::uint64_t agentGeneration = 0;
    std::int64_t operationDeadline = 0;
    std::int64_t commandDeadline = 0;
};

// This identifies whether an envelope can be CONSIDERED by a future
// canonical Agent adapter. It does not validate actor permissions,
// installation provenance, input paths, capacity or recording outcomes.
// No positive result constitutes permission to execute.
enum class ImportEnvelopeReason {
    candidateOnly,
    invalidIdentity,
    missingOperation,
    staleGeneration,
    invalidDeadline
};

inline ImportEnvelopeReason inspectImportEnvelope(
    const AgentImportEnvelope& request, std::int64_t now)
{
    if (request.moduleId != "rectools" ||
        request.packageName != "vdr-suite-addon-media-tools" ||
        request.commandType != "addon.media.import")
        return ImportEnvelopeReason::invalidIdentity;
    if (request.operationId.empty() || request.commandId.empty() ||
        request.backendId.empty() || request.backendId == "*")
        return ImportEnvelopeReason::missingOperation;
    if (request.operationGeneration == 0 || request.agentGeneration == 0 ||
        request.operationGeneration != request.agentGeneration)
        return ImportEnvelopeReason::staleGeneration;
    if (now <= 0 || request.operationDeadline <= now ||
        request.commandDeadline <= now ||
        request.commandDeadline > request.operationDeadline)
        return ImportEnvelopeReason::invalidDeadline;
    return ImportEnvelopeReason::candidateOnly;
}

} // namespace vdrsuite::mediatools
