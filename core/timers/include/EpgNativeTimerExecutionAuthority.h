#pragma once

#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerDeleteFingerprint.h"
#include "BackendAgentLocalProvider.h"
#include "TimerAssignmentPlanner.h"

#include <cstdint>
#include <string>

namespace vdrsuite::public_epg_timer
{
// Structurally fences facts obtained by the server. The selected provider
// must come from BackendAgentCommandRepository::selectLocalProvider, and
// its identity is verified again by Agent reservation prior to dispatch.
// The original identity is longer than TimerAssignmentPlanner's bounded
// 160-byte fence. Pin its full canonical fields via a domain-separated
// SHA-256 token rather than truncate it or relax existing planner limits.
inline std::string nativeTimerCreatePlanningAuthorityFence(
    const vdrsuite::agent::BackendAgentLocalProviderSelection& selection)
{
    const std::string canonical =
        vdrsuite::agent::backendAgentLocalProviderSelectionIdentity(selection);
    if (canonical.empty()) return {};
    const std::string digest =
        vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
            "epg-native-create-provider-fence/1|" + canonical);
    return digest.empty() ? std::string() : "epg-provider:" + digest;
}

inline bool currentNativeTimerCreateExecutionAuthority(
    const BackendAgentRecord& agent,
    const vdrsuite::agent::BackendAgentLocalProviderSelection& selection,
    const vdrsuite::timers::TimerAssignmentPlanningBackendCandidate& candidate,
    std::int64_t now)
{
    using namespace vdrsuite::agent;
    if (now <= 0 || agent.backendId.empty() ||
        agent.agentId.empty() || agent.agentInstanceId.empty() ||
        agent.revoked || agent.incompatible ||
        agent.backendGeneration == 0 ||
        agent.backendGeneration != candidate.currentBackendGeneration ||
        agent.backendId != candidate.backendId ||
        agent.heartbeatSequence == 0 ||
        agent.lastHeartbeatAt <= 0 || agent.lastHeartbeatAt > now ||
        now - agent.lastHeartbeatAt > 90 ||
        agent.leaseExpiresAt <= now ||
        agent.capabilityRevision == 0 ||
        !candidate.executionAuthorityCurrent ||
        selection.backendId != agent.backendId ||
        selection.authorityDomain != kBackendAgentNativeTimerCreateAuthorityDomain ||
        selection.providerId != kBackendAgentNativeTimerCreateProviderId ||
        selection.providerKind != kBackendAgentNativeTimerCreateProviderKind ||
        selection.requiredCapability != kBackendAgentNativeTimerCreateCapability ||
        !backendAgentLocalProviderValidSelection(selection))
        return false;

    // Opaque provider identity also fences ownership/provider epoch/revision.
    const std::string selectionFence =
        nativeTimerCreatePlanningAuthorityFence(selection);
    return !selectionFence.empty() &&
        selectionFence == candidate.executionAuthorityFence;
}
} // namespace vdrsuite::public_epg_timer
