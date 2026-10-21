#pragma once

#include "PinnedMediaImportAgentExecutor.h"

#include <filesystem>

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

private:
    std::filesystem::path installRoot_;
};

} // namespace vdrsuite::mediatools
