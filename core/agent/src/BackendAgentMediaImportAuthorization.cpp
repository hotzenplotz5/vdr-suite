#include "BackendAgentMediaImportAuthorization.h"
#include "BackendAgentClient.h"
#include "BackendAgentCommandJson.h"
#include "BackendAgentCommandStateStore.h"
#include "MediaToolsAgentExecutor.h"
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
ReceiptRevalidatingMediaImportAuthorization::
ReceiptRevalidatingMediaImportAuthorization(
    std::string path, IBackendAgentControlPlaneTransport& transport,
    CurrentAgentContext context)
    : statePath_(std::move(path)), transport_(transport),
      currentContext_(std::move(context)) {}

bool ReceiptRevalidatingMediaImportAuthorization::granted(
    const BackendAgentCommandAssignment& a) {
    if (!currentContext_ || a.commandType !=
        vdrsuite::mediatools::kMediaImportCommandType ||
        a.deadline <= nowSeconds()) return false;
    try {
        const auto ctx = currentContext_();
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
        // Replaying the EXACT acknowledged receipt triggers the server's
        // existing authoritative MediaImportOperationFence again. A first
        // receipt, offline/cache claim, or failure cannot authorize dispatch.
        const auto response = transport_.postAuthenticated(
            ctx.agentId, ctx.credentialSecret,
            "/api/agent/v1/commands/receipt",
            serializeBackendAgentCommandReceiptJson(state.receipt));
        return response.transportSucceeded && response.statusCode == 200 &&
            response.body ==
            "{\"outcome\":\"replayed\",\"reasonCode\":\"command_receipt_replayed\"}";
    } catch (...) { return false; }
}
} // namespace vdrsuite::agent
