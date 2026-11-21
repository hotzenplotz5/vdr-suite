#include "BackendAgentMediaShrinkAuthorization.h"
#include "BackendAgentClient.h"
#include "BackendAgentCommandJson.h"
#include "BackendAgentCommandStateStore.h"
#include "MediaShrinkAgentCommand.h"
#include "MediaShrinkAuthorityCodec.h"

#include <chrono>
#include <utility>

namespace vdrsuite::agent {
namespace {
std::int64_t nowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
bool exactCommand(const BackendAgentCommandAssignment& a,
                  const BackendAgentCommandAssignment& b) {
    return backendAgentCommandValidAssignment(a) &&
        backendAgentCommandValidAssignment(b) &&
        a.commandId == b.commandId && a.operationId == b.operationId &&
        a.jobId == b.jobId && a.attemptId == b.attemptId &&
        a.claimEpoch == b.claimEpoch &&
        a.backendId == b.backendId && a.agentId == b.agentId &&
        a.agentInstanceId == b.agentInstanceId &&
        a.backendGeneration == b.backendGeneration &&
        a.requestFingerprint == b.requestFingerprint &&
        backendAgentCommandFingerprint(a) == backendAgentCommandFingerprint(b);
}
bool exactReceipt(const BackendAgentCommandAssignment& a,
                  const BackendAgentCommandReceipt& r) {
    return backendAgentCommandValidReceipt(r) &&
        r.receiptCategory == "accepted" && r.reasonCode == "durably_recorded" &&
        r.commandId == a.commandId && r.jobId == a.jobId &&
        r.attemptId == a.attemptId && r.claimEpoch == a.claimEpoch &&
        r.requestFingerprint == a.requestFingerprint &&
        r.backendId == a.backendId && r.agentId == a.agentId &&
        r.agentInstanceId == a.agentInstanceId &&
        r.backendGeneration == a.backendGeneration;
}
}

ReceiptRevalidatingMediaShrinkAuthorization::
ReceiptRevalidatingMediaShrinkAuthorization(
    std::string path, IBackendAgentControlPlaneTransport& transport,
    CurrentContext context)
    : statePath_(std::move(path)), transport_(transport),
      context_(std::move(context)) {}

bool ReceiptRevalidatingMediaShrinkAuthorization::granted(
    const BackendAgentCommandAssignment& a)
{
    if (!context_ ||
        (a.commandType != vdrsuite::mediatools::kMediaShrinkStageCommandType &&
         a.commandType != vdrsuite::mediatools::kMediaShrinkPublishCommandType) ||
        a.deadline <= nowSeconds())
        return false;
    try {
        const auto ctx = context_();
        if (ctx.backendId != a.backendId || ctx.agentId != a.agentId ||
            ctx.agentInstanceId != a.agentInstanceId ||
            ctx.backendGeneration != a.backendGeneration ||
            ctx.backendGeneration == 0 || ctx.credentialSecret.empty())
            return false;
        commandstate::LocalState state;
        std::string reason;
        if (!commandstate::load(statePath_, state, reason) ||
            !state.receiptAcknowledged || state.resultPresent ||
            (state.dispatchState != "not_started" &&
             state.dispatchState != "starting") ||
            !exactCommand(a, state.assignment) ||
            !exactReceipt(a, state.receipt)) return false;

        // The Core backend replays the exact acknowledged receipt, which
        // reruns the latest actor/grant, install activation, lease/generation,
        // source/review CAS and command fence before any media write.
        const auto response = transport_.postAuthenticated(
            ctx.agentId, ctx.credentialSecret,
            "/api/agent/v1/commands/receipt",
            serializeBackendAgentCommandReceiptJson(state.receipt));
        return response.transportSucceeded && response.statusCode == 200 &&
            response.body ==
            R"({"outcome":"replayed","reasonCode":"command_receipt_replayed"})";
    } catch (...) { return false; }
}
std::optional<vdrsuite::mediatools::LocalMediaShrinkAuthority>
ReceiptRevalidatingMediaShrinkAuthorization::authority(
    const BackendAgentCommandAssignment& assignment)
{
    if (!granted(assignment) || !context_) return std::nullopt;
    try {
        // State can change between the preliminary permission replay and
        // authority issuance. Require the same protected, unconsumed receipt.
        const auto ctx = context_();
        if (ctx.backendId != assignment.backendId ||
            ctx.agentId != assignment.agentId ||
            ctx.agentInstanceId != assignment.agentInstanceId ||
            ctx.backendGeneration != assignment.backendGeneration ||
            ctx.credentialSecret.empty())
            return std::nullopt;
        commandstate::LocalState state;
        std::string reason;
        if (!commandstate::load(statePath_, state, reason) ||
            !state.receiptAcknowledged || state.resultPresent ||
            !exactCommand(assignment, state.assignment) ||
            !exactReceipt(assignment, state.receipt))
            return std::nullopt;
        const auto result = transport_.postAuthenticated(
            ctx.agentId, ctx.credentialSecret,
            "/api/agent/v1/media-shrink/authority",
            serializeBackendAgentCommandReceiptJson(state.receipt));
        if (!result.transportSucceeded || result.statusCode != 200)
            return std::nullopt;
        const auto authority =
            vdrsuite::mediatools::authoritywire::parse(result.body);
        if (!authority ||
            authority->backendId != assignment.backendId ||
            authority->operationId != assignment.operationId ||
            authority->backendGeneration != assignment.backendGeneration)
            return std::nullopt;
        vdrsuite::mediatools::MediaShrinkAgentSelection selection;
        if (!vdrsuite::mediatools::parseMediaShrinkAgentSelection(
                assignment.commandType, assignment.payload, selection) ||
            (selection.action ==
                vdrsuite::mediatools::ShrinkRequestAction::stage &&
             authority->recordingId != selection.recordingId))
            return std::nullopt;
        return authority;
    } catch (...) { return std::nullopt; }
}

} // namespace vdrsuite::agent
