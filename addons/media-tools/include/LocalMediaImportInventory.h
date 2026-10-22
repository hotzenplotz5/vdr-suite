#pragma once

#include "PinnedMediaImportAgentExecutor.h"

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
    // one-level directory scan; only unchanged, aged candidates are exposed.
    // Discovery does not enqueue jobs or weaken Core authorization.
    // Fail closed: opaque IDs only, no paths or partial trusted inventory.
    bool listTrustedSourceIds(const std::string& backendId,
                              std::vector<std::string>& sourceIds);

private:
    std::filesystem::path installRoot_;
};

} // namespace vdrsuite::mediatools
