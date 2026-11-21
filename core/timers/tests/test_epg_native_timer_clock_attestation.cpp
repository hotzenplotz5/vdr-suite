#include "EpgNativeTimerClockAttestation.h"

#include <cassert>
#include <cstdint>
#include <string>

using namespace vdrsuite::agent;

// Timer domain regression deliberately stays outside the Agent transport
// compilation unit. The Phase-64 cross-layer architecture boundary remains
// unchanged; this uses typed mock replies and does not touch a real VDR.
void testNativeTimerClockAttestationFromTypedSvdrpReply()
{
    {
        // Simulated *wire reply*: this exercises the same typed decoder as
        // the actual Agent/Daemon path. No live VDR or Timer is touched.
        vdrsuite::agent::BackendAgentLocalProviderSelection provider;
        provider.backendId = "default";
        provider.authorityDomain = "vdr.timer";
        provider.providerId = "suitebridge:local";
        provider.providerKind = "suitebridge";
        provider.ownershipGeneration = 9;
        provider.providerInstanceEpoch = "pie_1";
        provider.providerGeneration = 1;
        provider.capabilityRevision = 1;
        provider.requiredCapability = "vdr.timer.create";
        BackendAgentRecord agent;
        agent.backendId = "default";
        agent.agentId = "agt_1";
        agent.agentInstanceId = "agi_1";
        agent.backendGeneration = 7;
        agent.heartbeatSequence = 2;
        agent.capabilityRevision = 1;
        constexpr std::int64_t Start = 1792222800;
        constexpr std::int64_t Stop = 1792226400;
        constexpr std::int64_t Now = Start + 2;
        agent.lastHeartbeatAt = Now - 1;
        agent.leaseExpiresAt = Now + 30;
        vdrsuite::timers::TimerAssignmentPlanningBackendCandidate candidate;
        candidate.backendId = agent.backendId;
        candidate.currentBackendGeneration = agent.backendGeneration;
        candidate.executionAuthorityCurrent = true;
        candidate.executionAuthorityFence =
            vdrsuite::public_epg_timer::
                nativeTimerCreatePlanningAuthorityFence(provider);
        const SuiteBridgeCommandReply accepted{
            SuiteBridgeTransportStatus::Success, 900,
            "vdr-suite-ntcreate-clock/1 vdr.timer.create 1 "
            "suitebridge pie_1 1 1 1792222800 1792226400 0 0 "
            "2026-10-17 0740 2026-10-17 0840", {}};
        vdrsuite::public_epg_timer::ServerOwnedNativeClockEvidence evidence;
        std::string reason;
        assert(vdrsuite::public_epg_timer::attestBackendNativeTimerClockReply(
            accepted, agent, provider, candidate,
            Start, Stop, Now, evidence, reason));
        assert(reason == "native_timer_clock_attested");
        assert(evidence.current && evidence.backendId == "default");
        assert(evidence.backendGeneration == 7);
        assert(evidence.providerInstanceEpoch == provider.providerInstanceEpoch);

        // Exactly the response from an installed SuiteBridge that predates
        // CLOCK. CAP enabled does not make CLOCK available by implication.
        const SuiteBridgeCommandReply unsupported{
            SuiteBridgeTransportStatus::Success, 501,
            "vdr-suite-ntcreate-rejected/1 unsupported", {}};
        assert(!vdrsuite::public_epg_timer::attestBackendNativeTimerClockReply(
            unsupported, agent, provider, candidate,
            Start, Stop, Now, evidence, reason));
        assert(reason == "native_timer_clock_unavailable");
        assert(!evidence.current);

        auto staleAgent = agent;
        staleAgent.leaseExpiresAt = Now;
        assert(!vdrsuite::public_epg_timer::attestBackendNativeTimerClockReply(
            accepted, staleAgent, provider, candidate,
            Start, Stop, Now, evidence, reason));
        assert(reason == "native_timer_clock_attestation_stale_or_invalid");
        assert(!evidence.current);

        // Repeat the server-owned authority check against the CURRENT Agent,
        // not the copy captured before CLOCK / preview. A revoked Agent, an
        // expired lease or stale heartbeat may retain the same identity and
        // backend generation and must still fail the final admission gate.
        assert(vdrsuite::public_epg_timer::
            currentNativeTimerCreateExecutionAuthority(
                agent, provider, candidate, Now));
        auto liveAfterPreview = agent;
        liveAfterPreview.revoked = true;
        assert(!vdrsuite::public_epg_timer::
            currentNativeTimerCreateExecutionAuthority(
                liveAfterPreview, provider, candidate, Now));
        liveAfterPreview = agent;
        liveAfterPreview.incompatible = true;
        assert(!vdrsuite::public_epg_timer::
            currentNativeTimerCreateExecutionAuthority(
                liveAfterPreview, provider, candidate, Now));
        liveAfterPreview = agent;
        liveAfterPreview.leaseExpiresAt = Now;
        assert(!vdrsuite::public_epg_timer::
            currentNativeTimerCreateExecutionAuthority(
                liveAfterPreview, provider, candidate, Now));
        liveAfterPreview = agent;
        liveAfterPreview.lastHeartbeatAt = Now - 91;
        assert(!vdrsuite::public_epg_timer::
            currentNativeTimerCreateExecutionAuthority(
                liveAfterPreview, provider, candidate, Now));

        const SuiteBridgeCommandReply otherEpoch{
            SuiteBridgeTransportStatus::Success, 900,
            "vdr-suite-ntcreate-clock/1 vdr.timer.create 1 "
            "suitebridge pie_restarted 1 1 1792222800 1792226400 0 0 "
            "2026-10-17 0740 2026-10-17 0840", {}};
        assert(!vdrsuite::public_epg_timer::attestBackendNativeTimerClockReply(
            otherEpoch, agent, provider, candidate,
            Start, Stop, Now, evidence, reason));
        assert(reason == "native_timer_clock_reply_invalid_or_stale");
        assert(!evidence.current);
    }

}
