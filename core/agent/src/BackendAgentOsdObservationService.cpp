#include "BackendAgentLifecycle.h"
#include "AuthorizationService.h"
#include "BackendRegistryService.h"
#include "SecurityIdentityRepository.h"

#include <algorithm>

bool BackendAgentLifecycleService::osdAuthorityCurrent(
    const std::string& backendId, const std::string& instanceId,
    std::uint64_t generation, std::int64_t now) const
{
    const auto backend = backendRegistryService_.getBackend(backendId);
    const auto agent = repository_.findAgentForBackend(backendId);
    if (now < 0 || !backend || !backend->enabled || !agent || agent->revoked ||
        agent->incompatible || generation == 0 || agent->backendGeneration != generation ||
        agent->agentInstanceId != instanceId || agent->leaseExpiresAt <= now ||
        agent->capabilityRevision == 0) return false;
    const auto actor = identityRepository_.findActor(agent->actorId);
    const auto device = identityRepository_.findDevice(agent->deviceId);
    const auto credential = identityRepository_.findCredential(agent->credentialId);
    if (!actor || !actor->active || actor->revoked || actor->type != ActorType::Agent ||
        !device || !device->active || device->revoked || device->actorId != agent->actorId ||
        !credential || !credential->active || credential->revoked || credential->expired ||
        credential->actorId != agent->actorId) return false;
    const auto facts = repository_.capabilitiesForAgent(agent->agentId);
    return std::find(facts.observationDomains.begin(), facts.observationDomains.end(),
                     "osd") != facts.observationDomains.end();
}

BackendAgentObservationResult BackendAgentLifecycleService::ingestOsdObservation(
    const RequestSecurityContext& context, const BackendAgentOsdObservation& observation,
    std::int64_t now)
{
    BackendAgentObservationResult result;
    result.reasonCode = "agent_authentication_required";
    if (!context.authenticated() || context.actor.type != ActorType::Agent) return result;
    result.reasonCode = "invalid_osd_observation";
    if (!validBackendAgentOsdObservation(observation)) return result;
    const auto agent = repository_.findAgentForBackend(observation.backendId);
    result.reasonCode = "osd_agent_authority_rejected";
    if (!agent || context.actor.actorId != agent->actorId || !context.credential ||
        context.credential->credentialId != agent->credentialId || !context.device ||
        context.device->deviceId != agent->deviceId ||
        !osdAuthorityCurrent(observation.backendId, observation.agentInstanceId,
                             observation.backendGeneration, now) ||
        observation.producerSequence != agent->heartbeatSequence ||
        now < agent->lastHeartbeatAt ||
        now - agent->lastHeartbeatAt > BackendAgentOsdObservation::FreshnessSeconds) return result;

    std::lock_guard<std::mutex> guard(osdMutex_);
    // A bounded latest-value cache, no persistent observation rows or event bodies.
    // Expired entries cannot be returned even if nobody has published since expiry.
    for (auto it = osdObservations_.begin(); it != osdObservations_.end();)
    {
        if (now < it->second.acceptedAt ||
            now - it->second.acceptedAt > BackendAgentOsdObservation::FreshnessSeconds)
            it = osdObservations_.erase(it);
        else ++it;
    }
    auto found = osdObservations_.find(observation.backendId);
    if (found != osdObservations_.end() &&
        found->second.observation.backendGeneration == observation.backendGeneration)
    {
        const auto& previous = found->second.observation;
        if (observation.producerSequence <= previous.producerSequence)
        {
            result.reasonCode = "osd_producer_sequence_conflict";
            if (observation.producerSequence != previous.producerSequence ||
                serializeBackendAgentOsdObservation(observation) !=
                    serializeBackendAgentOsdObservation(previous)) return result;
            result.replayed = true;
        }
    }
    if (found == osdObservations_.end() && osdObservations_.size() >= 64)
    {
        result.reasonCode = "osd_receiver_capacity_reached";
        return result;
    }
    if (!result.replayed) osdObservations_[observation.backendId] = {observation, now};
    result.accepted = true;
    result.producerSequence = observation.producerSequence;
    result.lastAcceptedSequence = observation.producerSequence;
    result.reasonCode = result.replayed ? "osd_observation_replayed" : "osd_observation_accepted";
    return result;
}

BackendAgentOsdReadResult BackendAgentLifecycleService::readOsdObservation(
    const RequestSecurityContext& context, const std::string& backendId,
    std::uint64_t expectedGeneration, std::int64_t now) const
{
    BackendAgentOsdReadResult result;
    // User-facing session/binding routes are a later slice. Even an internal
    // reader must supply a current authenticated user/service context and scope.
    result.reasonCode = "osd_view_denied";
    if (context.actor.type == ActorType::Agent || !context.authenticated() ||
        !AuthorizationService{}.authorize(context,
            {"osd.view", backendId, "osd.view"}).allowed) return result;
    std::lock_guard<std::mutex> guard(osdMutex_);
    auto found = osdObservations_.find(backendId);
    result.reasonCode = "osd_unavailable";
    if (found == osdObservations_.end()) return result;
    const auto& entry = found->second;
    if (now < entry.acceptedAt ||
        now - entry.acceptedAt > BackendAgentOsdObservation::FreshnessSeconds ||
        !osdAuthorityCurrent(backendId, entry.observation.agentInstanceId,
                             entry.observation.backendGeneration, now))
    {
        osdObservations_.erase(found);
        return result;
    }
    result.reasonCode = "backend_generation_conflict";
    if (expectedGeneration == 0 || expectedGeneration != entry.observation.backendGeneration)
        return result;
    result.snapshot = entry.observation.snapshot;
    using State = vdrsuite::agent::SuiteBridgeOsdFrameSourceState;
    result.available = result.snapshot.state == State::Current && result.snapshot.buffer.hasFrame;
    result.reasonCode = result.available ? "osd_current" :
        result.snapshot.state == State::ResyncRequired ? "osd_resync_required" : "osd_unavailable";
    return result;
}
