#pragma once

#include "AddonInstallSnapshot.h"
#include "MediaToolsAgentExecutor.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace vdrsuite::mediatools {

// Resolved only by a backend-local, trusted media inventory. The Agent must
// never copy any of these filesystem paths from an HTTP/API command payload.
struct LocalMediaImportSource {
    std::filesystem::path sourceRoot;
    std::filesystem::path videoRoot;
    std::filesystem::path sourceFile;
};

class ILocalMediaImportInventory {
public:
    virtual ~ILocalMediaImportInventory() = default;
    virtual bool resolve(const std::string& backendId,
                         const std::string& sourceId,
                         LocalMediaImportSource& output) = 0;
};

// An eventual Core adapter must resolve the *effective* backend-scoped
// addons.media.import right and operation identity; no admin shortcut.
class ILocalMediaImportAuthorization {
public:
    virtual ~ILocalMediaImportAuthorization() = default;
    virtual bool granted(const BackendAgentCommandAssignment& assignment) = 0;
};

// Real pinned-fd execution path, but NOT auto-installed or registered in
// apps/agent/main.cpp. Synthetic root/permissions are used by isolated CI.
// The local command state itself continues to be owned by Core/Agent.
class PinnedMediaImportAgentExecutor final : public IMediaImportAgentExecutor {
public:
    PinnedMediaImportAgentExecutor(std::string approvalRoot,
                                  std::string backendId,
                                  std::uint64_t backendGeneration,
                                  ILocalMediaImportInventory& inventory,
                                  ILocalMediaImportAuthorization& authorization);
    bool available() override;
    bool authorize(const BackendAgentCommandAssignment& assignment,
                   const std::string& backendId,
                   std::uint64_t generation) override;
    MediaImportAgentOutcome execute(
        const BackendAgentCommandAssignment& assignment) override;

private:
    std::string root_;
    std::string backendId_;
    std::uint64_t generation_;
    ILocalMediaImportInventory& inventory_;
    ILocalMediaImportAuthorization& authorization_;
    std::unique_ptr<vdrsuite::addons::MediaToolsInstallSnapshot> pinned_;
    std::string pendingCommandId_;
    std::string pendingFingerprint_;
};

} // namespace vdrsuite::mediatools
