#pragma once

#include "MutationOperation.h"

#include <cstdint>
#include <string>
#include <vector>

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
    std::string videoDecision; // "desktop", "skip", "duplicate" or "release"
    std::string metadata; // optional immutable pre-found TMDb values; no lookup
    std::int64_t now = 0;
};

struct MediaImportOperationAdmissionResult
{
    MediaImportOperationAdmissionStatus status =
        MediaImportOperationAdmissionStatus::unavailable;
    vdrsuite::operations::MutationOperation operation;
};

// A batch is an atomic reservation of ordinary Core media.import Operations.
// The numbered mb1_<32 hex>_NNN idempotency keys preserve selection order;
// no independent batch job, executor, or SQLite table is introduced.
struct MediaImportBatchSource
{
    std::string sourceId;
    std::string metadata;
};
struct MediaImportBatchAdmissionRequest
{
    std::string actorId;
    std::string backendId;
    std::string batchId;
    std::vector<MediaImportBatchSource> sources;
    std::int64_t now = 0;
};
struct MediaImportBatchAdmissionResult
{
    MediaImportOperationAdmissionStatus status =
        MediaImportOperationAdmissionStatus::unavailable;
    std::vector<vdrsuite::operations::MutationOperation> operations;
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
    MediaImportBatchAdmissionResult admitBatch(
        const MediaImportBatchAdmissionRequest& request);

private:
    MediaImportOperationAdmissionResult admitInternal(
        const MediaImportOperationAdmissionRequest& request,
        bool inExistingTransaction);
    vdrsuite::operations::MutationOperationRepository& operations_;
    BackendAgentRepository& agents_;
    MediaImportOperationFence& fence_;
};

} // namespace vdrsuite::addons
