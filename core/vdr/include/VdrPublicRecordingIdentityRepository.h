#pragma once

#include "VdrRecording.h"

#include <optional>
#include <string>
#include <vector>

class Database;

// Private server-side binding: sourceAddress MUST NOT enter public HTTP JSON.
struct VdrPublicRecordingBinding
{
    std::string publicRecordingId;
    std::string backendId;
    std::string sourceAddress;
};

class VdrPublicRecordingIdentityRepository
{
public:
    explicit VdrPublicRecordingIdentityRepository(Database& database);

    bool ensureSchema();

    // Atomic replacement of the observed inventory for ONE backend.
    // Never deduce move/rename from title or filename similarity.
    bool reconcileBackend(
        const std::string& backendId,
        const std::vector<VdrRecording>& recordings);

    std::vector<VdrPublicRecordingBinding> activeBindingsForBackend(
        const std::string& backendId) const;

    std::optional<VdrPublicRecordingBinding> findActiveBinding(
        const std::string& publicRecordingId) const;

    // Only call after a separately authorized, verified move operation.
    bool rebindVerifiedMove(
        const std::string& publicRecordingId,
        const std::string& backendId,
        const std::string& expectedSourceAddress,
        const std::string& newSourceAddress);

private:
    Database& database_;
};
