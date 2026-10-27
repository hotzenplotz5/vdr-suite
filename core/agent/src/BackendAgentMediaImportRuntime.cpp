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
    if (sourceId.size() < 16U || sourceId.size() > 64U ||
        !std::all_of(sourceId.begin(), sourceId.end(),
            [](unsigned char c) {
                return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') || c == '_' || c == '-';
            })) {
        out.failureCode = "source_invalid";
        return false;
    }
    if (!available()) {
        out.failureCode = "agent_unavailable";
        return false;
    }
    try {
        out.failureCode = "maintenance_busy";
        auto lock = MediaToolsImportRunLock::tryAcquire(
            trustedRoot_, MediaToolsImportRunLock::Mode::sharedExecution);
        if (!lock) return false;
        out.failureCode = "inventory_missing";
        LocalMediaImportSource local;
        if (!inventory_.resolve(backendId_, sourceId, local)) return false;
        out.failureCode = "source_invalid";
        const auto plan = inspectImportPreview(
            {local.sourceRoot, local.videoRoot, local.sourceFile});
        out.failureCode = "source_changed";
        if (plan.sourceDevice != local.sourceDevice ||
            plan.sourceInode != local.sourceInode ||
            plan.sourceBytes != local.sourceBytes ||
            plan.sourceMtimeSeconds != local.sourceMtimeSeconds ||
            plan.sourceMtimeNanoseconds != local.sourceMtimeNanoseconds)
            return false;
        out.failureCode = "target_invalid";
        const auto relative =
            plan.titleDirectory.lexically_relative(plan.videoRoot);
        if (relative.empty() || relative == "." ||
            relative.is_absolute() || relative.generic_string().size() > 512)
            return false;
        out.title = plan.title;
        out.targetFolder = relative.generic_string();
        out.sourceBytes = static_cast<std::uint64_t>(plan.sourceBytes);
        const auto metadata = readImportLocalMetadata(plan);
        out.nfoOutline = metadata.shortText;
        out.nfoPlot = metadata.description;
        out.nfoYear = metadata.year;
        out.nfoGenre = metadata.genre;
        const auto artwork = readImportLocalArtwork(plan);
        out.posterDataUri = artwork.posterDataUri;
        out.fanartDataUri = artwork.fanartDataUri;
        // A protected destination collision is not itself proof of a duplicate.
        if (plan.duplicateTarget) {
            out.status = confirmedMediaImportDuplicate(plan)
                ? "duplicate" : "unavailable";
            out.failureCode = out.status == "duplicate" ? "" : "target_collision";
            return true;
        }
        out.failureCode = "ffprobe_failed";
        NativeProcessRunner probe(std::chrono::seconds(5));
        const std::string codec = probeImportVideoCodec(plan, probe);
        out.failureCode = "capacity_failed";
        const auto capacity = inspectImportCapacity(plan);
        out.videoCodec = codec;
        // Duration is display-only: unsupported duration is not an import error.
        try {
            const double seconds = probeImportDurationSeconds(plan, probe);
            if (seconds > 0.0 && seconds <= 4294967295.0)
                out.durationSeconds = static_cast<std::uint64_t>(seconds + 0.5);
        } catch (...) {}
        out.availableBytes = capacity.availableBytes;
        if (capacity.reserveBytes >
            std::numeric_limits<std::uint64_t>::max() - capacity.requiredBytes)
            return false;
        out.requiredBytes = capacity.requiredBytes + capacity.reserveBytes;
        out.videoReencodeRequired = importVideoNeedsReencode(codec);
        out.status = !capacity.writable ? "unavailable" :
            capacity.sufficient ? "ready" : "insufficient";
        out.failureCode = capacity.writable ? "" : "target_not_writable";
        return true;
    } catch (...) {
        // Keep only the stage; do not forward exception text or local paths.
        const std::string stage = out.failureCode;
        out = {};
        out.sourceId = sourceId;
        out.failureCode = stage.empty() ? "inspection_failed" : stage;
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
