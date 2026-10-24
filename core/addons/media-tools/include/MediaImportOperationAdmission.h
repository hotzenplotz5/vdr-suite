#pragma once

#include "MutationOperation.h"

#include <cstdint>
#include <string>

class BackendAgentRepository;
namespace vdrsuite::operations { class MutationOperationRepository; }

namespace vdrsuite::addons {
class MediaImportOperationFence;

enum class MediaImportOperationAdmissionStatus
{
    accepted,
    replayed,
    invalid,
    forbidden,
    idempotencyConflict,
    conflict,
    unavailable
};

struct MediaImportOperationAdmissionRequest
{
    std::string actorId;
    std::string backendId;
    std::string sourceId;
    std::string idempotencyKey;
    bool approveVideoReencode = false;
    std::int64_t now = 0;
};

struct MediaImportOperationAdmissionResult
{
    MediaImportOperationAdmissionStatus status =
        MediaImportOperationAdmissionStatus::unavailable;
    vdrsuite::operations::MutationOperation operation;
};

// One Core ADR-0042 immutable operation; never a second job queue.
// The trusted caller drives the existing Agent command assignment after
// this durable reservation. No file paths or media-side effects here.
class MediaImportOperationAdmissionService
{
public:
    MediaImportOperationAdmissionService(
        vdrsuite::operations::MutationOperationRepository& operations,
        BackendAgentRepository& agents,
        MediaImportOperationFence& fence);

    MediaImportOperationAdmissionResult admit(
        const MediaImportOperationAdmissionRequest& request);

private:
    vdrsuite::operations::MutationOperationRepository& operations_;
    BackendAgentRepository& agents_;
    MediaImportOperationFence& fence_;
};

} // namespace vdrsuite::addons
