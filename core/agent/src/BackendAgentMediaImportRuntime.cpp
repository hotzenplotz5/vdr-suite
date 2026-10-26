#include "BackendAgentMediaImportRuntime.h"
#include "AddonInstallSnapshot.h"
#include "MediaImport.h"
#include "MediaToolsImportRunLock.h"
#include <utility>
#include <algorithm>
#include <chrono>
#include <limits>
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

bool BackendAgentMediaImportRuntime::verifiedInstallation(
    vdrsuite::mediatools::MediaToolsInstallationEvidence& out)
{
    out = {};
    try {
        if (!currentContext_) return false;
        const auto context = currentContext_();
        if (context.backendId != backendId_ || context.agentId.empty() ||
            context.agentInstanceId.empty() || context.backendGeneration == 0 ||
            context.credentialSecret.empty()) return false;
        auto lock = vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
            trustedRoot_,
            vdrsuite::mediatools::MediaToolsImportRunLock::Mode::sharedExecution);
        if (!lock) return false;
        const auto snapshot = vdrsuite::addons::inspectMediaToolsInstallSnapshot(
            trustedRoot_);
        if (!snapshot.bytesVerified()) return false;
        using vdrsuite::addons::InstallOrigin;
        if (snapshot.origin != InstallOrigin::manual &&
            snapshot.origin != InstallOrigin::debian) return false;
        out.origin = snapshot.origin == InstallOrigin::manual ? "manual" : "debian";
        out.version = snapshot.approvedVersion;
        out.sha256 = snapshot.approvedSha256;
        return true;
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

bool BackendAgentMediaImportRuntime::listTrustedSourceLabels(
    std::vector<vdrsuite::mediatools::MediaImportSourceLabel>& out)
{
    out.clear();
    if (!available()) return false;
    try {
        return inventory_.listTrustedSourceLabels(backendId_, out);
    } catch (...) {
        out.clear();
        return false;
    }
}

bool BackendAgentMediaImportRuntime::listTrustedSourceSnapshot(
    std::vector<std::string>& ids,
    std::vector<vdrsuite::mediatools::MediaImportSourceLabel>& labels,
    bool& partial)
{
    ids.clear();
    labels.clear();
    partial = true;
    if (!available()) return false;
    try {
        if (!inventory_.listTrustedSourceSnapshot(backendId_, labels, partial))
            return false;
        for (const auto& item : labels) ids.push_back(item.sourceId);
        return true;
    } catch (...) {
        ids.clear();
        labels.clear();
        partial = true;
        return false;
    }
}

bool BackendAgentMediaImportRuntime::inspectTrustedSource(
    const std::string& sourceId,
    vdrsuite::mediatools::MediaImportSourcePreview& out)
{
    using namespace vdrsuite::mediatools;
    out = {};
    out.sourceId = sourceId;
    // The requested key must already be an opaque backend-local inventory
    // token. Never accept an actor's file path, and never write a marker.
    if (sourceId.size() < 16U || sourceId.size() > 64U ||
        !std::all_of(sourceId.begin(), sourceId.end(),
            [](unsigned char c) {
                return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') || c == '_' || c == '-';
            }) || !available()) return false;
    try {
        auto lock = MediaToolsImportRunLock::tryAcquire(
            trustedRoot_, MediaToolsImportRunLock::Mode::sharedExecution);
        if (!lock) return false;
        LocalMediaImportSource local;
        if (!inventory_.resolve(backendId_, sourceId, local)) return false;
        const auto plan = inspectImportPreview(
            {local.sourceRoot, local.videoRoot, local.sourceFile});
        if (plan.sourceDevice != local.sourceDevice ||
            plan.sourceInode != local.sourceInode ||
            plan.sourceBytes != local.sourceBytes ||
            plan.sourceMtimeSeconds != local.sourceMtimeSeconds ||
            plan.sourceMtimeNanoseconds != local.sourceMtimeNanoseconds)
            return false;
        // The same plan/codec/capacity checks are used by the productive
        // importer. FFprobe reads the source but creates no VDR artifacts.
        NativeProcessRunner probe(std::chrono::seconds(5));
        const std::string codec = probeImportVideoCodec(plan, probe);
        const auto capacity = inspectImportCapacity(plan);
        const auto relative =
            plan.titleDirectory.lexically_relative(plan.videoRoot);
        if (relative.empty() || relative == "." ||
            relative.is_absolute() || relative.generic_string().size() > 512)
            return false;
        out.title = plan.title;
        out.targetFolder = relative.generic_string();
        out.videoCodec = codec;
        out.sourceBytes = static_cast<std::uint64_t>(plan.sourceBytes);
        out.availableBytes = capacity.availableBytes;
        // The user-facing total includes the mandatory production reserve.
        // Production itself continues to use the shared capacity policy.
        if (capacity.reserveBytes >
            std::numeric_limits<std::uint64_t>::max() - capacity.requiredBytes)
            return false;
        out.requiredBytes = capacity.requiredBytes + capacity.reserveBytes;
        out.videoReencodeRequired = importVideoNeedsReencode(codec);
        out.status = plan.duplicateTarget ? "duplicate" :
            !capacity.writable ? "unavailable" :
            capacity.sufficient ? "ready" : "insufficient";
        return true;
    } catch (...) {
        out = {};
        out.sourceId = sourceId;
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
        // The Agent identity and backend generation may change between
        // authorize() and execute(). Reject before touching the pinned
        // executable, independently of the server receipt revalidation.
        if (!available()) return Outcome::rejected;
        const auto context = currentContext_();
        if (context.backendId != backendId_ ||
            context.backendId != assignment.backendId ||
            context.backendGeneration != assignment.backendGeneration ||
            context.agentId != assignment.agentId ||
            context.agentInstanceId != assignment.agentInstanceId ||
            context.credentialSecret.empty())
            return Outcome::rejected;
        // A changed root approval, source or effective server grant is denied
        // again inside the pinned executor immediately before fexecve.
        return currentExecution_->execute(assignment);
    } catch (...) { return Outcome::unknown; }
}

std::string BackendAgentMediaImportRuntime::takeMarkerEvidence()
{
    if (!executionConsumed_ || !currentExecution_) return {};
    try {
        return currentExecution_->takeMarkerEvidence();
    } catch (...) { return {}; }
}

std::string BackendAgentMediaImportRuntime::takeRecordingEvidence()
{
    if (!executionConsumed_ || !currentExecution_) return {};
    try {
        return currentExecution_->takeRecordingEvidence();
    } catch (...) { return {}; }
}
} // namespace vdrsuite::agent
