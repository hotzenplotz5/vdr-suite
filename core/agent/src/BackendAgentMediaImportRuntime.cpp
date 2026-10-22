#include "BackendAgentMediaImportRuntime.h"
#include "AddonInstallSnapshot.h"
#include "MediaToolsImportRunLock.h"
#include <utility>
namespace vdrsuite::agent {

BackendAgentMediaImportRuntime::BackendAgentMediaImportRuntime(
    std::string trustedRoot, std::string backendId, std::string statePath,
    IBackendAgentControlPlaneTransport& transport,
    CurrentContext context)
    : trustedRoot_(std::move(trustedRoot)), backendId_(std::move(backendId)),
      currentContext_(std::move(context)), inventory_(trustedRoot_),
      authorization_(std::move(statePath), transport, currentContext_) {}

bool BackendAgentMediaImportRuntime::available()
{
    try {
        if (!currentContext_) return false;
        const auto context = currentContext_();
        if (context.backendId != backendId_ ||
            context.agentId.empty() || context.agentInstanceId.empty() ||
            context.credentialSecret.empty() ||
            context.backendGeneration == 0 ||
            !vdrsuite::addons::inspectMediaToolsInstallSnapshot(
                trustedRoot_).bytesVerified() ||
            !vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
                trustedRoot_,
                vdrsuite::mediatools::MediaToolsImportRunLock::Mode::sharedExecution))
            return false;
        return inventory_.hasTrustedSource(backendId_);
    } catch (...) { return false; }
}

bool BackendAgentMediaImportRuntime::listTrustedSourceIds(
    std::vector<std::string>& out)
{
    out.clear();
    if (!available()) return false;
    try {
        return inventory_.listTrustedSourceIds(backendId_, out);
    } catch (...) {
        out.clear();
        return false;
    }
}

bool BackendAgentMediaImportRuntime::authorize(
    const BackendAgentCommandAssignment& assignment,
    const std::string& backendId, std::uint64_t generation)
{
    currentExecution_.reset();
    executionConsumed_ = false;
    if (backendId != backendId_ || !available()) return false;
    try {
        const auto context = currentContext_();
        if (context.backendGeneration != generation ||
            context.agentId != assignment.agentId ||
            context.agentInstanceId != assignment.agentInstanceId ||
            context.backendGeneration != assignment.backendGeneration)
            return false;
        auto pinned = std::make_unique<
            vdrsuite::mediatools::PinnedMediaImportAgentExecutor>(
                trustedRoot_, backendId_, generation,
                inventory_, authorization_);
        if (!pinned->authorize(assignment, backendId, generation)) return false;
        currentExecution_ = std::move(pinned);
        return true;
    } catch (...) { return false; }
}

vdrsuite::mediatools::MediaImportAgentOutcome
BackendAgentMediaImportRuntime::execute(
    const BackendAgentCommandAssignment& assignment)
{
    using Outcome = vdrsuite::mediatools::MediaImportAgentOutcome;
    if (executionConsumed_ || !currentExecution_) return Outcome::rejected;
    executionConsumed_ = true;
    try {
        // A changed root approval, source or effective server grant is denied
        // again inside the pinned executor immediately before fexecve.
        if (!available()) return Outcome::rejected;
        return currentExecution_->execute(assignment);
    } catch (...) { return Outcome::unknown; }
}

std::string BackendAgentMediaImportRuntime::takeRecordingEvidence()
{
    if (!executionConsumed_ || !currentExecution_) return {};
    try {
        return currentExecution_->takeRecordingEvidence();
    } catch (...) { return {}; }
}
} // namespace vdrsuite::agent
