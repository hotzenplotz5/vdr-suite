#pragma once
#include "BackendAgentMediaImportAuthorization.h"
#include "LocalMediaImportInventory.h"
#include "MediaToolsAgentExecutor.h"
#include "PinnedMediaImportAgentExecutor.h"
#include <functional>
#include <memory>
#include <string>

class IBackendAgentControlPlaneTransport;

namespace vdrsuite::agent {
// The Agent composition's *only* local Media Import executor.
// Not installed by COMMAND_TYPES, manifests, or package presence. The
// shipped binary constructs it only after explicit CLI opt-in.
class BackendAgentMediaImportRuntime final
    : public vdrsuite::mediatools::IMediaImportAgentExecutor {
public:
    using CurrentContext = std::function<BackendAgentCommandClientContext()>;
    BackendAgentMediaImportRuntime(
        std::string trustedRoot, std::string backendId, std::string statePath,
        IBackendAgentControlPlaneTransport& transport,
        CurrentContext currentContext);
    bool available() override;
    bool verifiedInstallation(
        vdrsuite::mediatools::MediaToolsInstallationEvidence& out) override;
    bool listTrustedSourceIds(std::vector<std::string>& out) override;
    bool listTrustedSourceLabels(
        std::vector<vdrsuite::mediatools::MediaImportSourceLabel>& out) override;
    bool authorize(const BackendAgentCommandAssignment& assignment,
                   const std::string& backendId,
                   std::uint64_t generation) override;
    vdrsuite::mediatools::MediaImportAgentOutcome execute(
        const BackendAgentCommandAssignment& assignment) override;
    std::string takeRecordingEvidence() override;
    std::string takeMarkerEvidence() override;
private:
    std::string trustedRoot_;
    std::string backendId_;
    CurrentContext currentContext_;
    vdrsuite::mediatools::RootOwnedMediaImportInventory inventory_;
    ReceiptRevalidatingMediaImportAuthorization authorization_;
    std::unique_ptr<vdrsuite::mediatools::PinnedMediaImportAgentExecutor>
        currentExecution_;
    bool executionConsumed_ = false;
};
} // namespace vdrsuite::agent
