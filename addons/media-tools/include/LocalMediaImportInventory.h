#pragma once

#include "PinnedMediaImportAgentExecutor.h"
#include "MediaImport.h"

#include <filesystem>
#include <string>
#include <vector>

namespace vdrsuite::mediatools {

// Read-only mapping approved by root on the backend host. This does not
// confer execution rights and cannot be populated by an Agent assignment.
// Missing or invalid inventory is always a denial.
class RootOwnedMediaImportInventory final : public ILocalMediaImportInventory {
public:
    explicit RootOwnedMediaImportInventory(std::filesystem::path installRoot = "/");
    bool resolve(const std::string& backendId, const std::string& sourceId,
                 LocalMediaImportSource& output) override;
    // Capability discovery: a protected ledger with at least one currently
    // inspectable source for this backend, without producing any side effect.
    bool hasTrustedSource(const std::string& backendId);
    // A root-owned TSV row ending in '*' opts in a bounded, read-only
    // recursive directory scan; only unchanged, aged candidates are exposed.
    // Discovery does not enqueue jobs or weaken Core authorization.
    // Opaque IDs only. Partial results must be explicitly marked incomplete.
    bool listTrustedSourceIds(const std::string& backendId,
                              std::vector<std::string>& sourceIds);
    bool listTrustedSourceLabels(
        const std::string& backendId,
        std::vector<MediaImportSourceLabel>& sources);
    // One consistent read of the verified bounded catalog and its
    // completion status; no filesystem paths escape to the client.
    bool listTrustedSourceSnapshot(
        const std::string& backendId,
        std::vector<MediaImportSourceLabel>& sources, bool& partial);

    // Resolve ONE opaque source ID through the existing trusted inventory,
    // then run a bounded, read-only importer probe on that exact file.
    // Not a write command, grant or browser-facing filesystem path.
    bool inspectTrustedSource(const std::string& backendId,
                              const std::string& sourceId,
                              ProcessRunner& runner,
                              ImportPreview& preview,
                              ImportCapacityPreview& capacity);

private:
    std::filesystem::path installRoot_;
};

} // namespace vdrsuite::mediatools
