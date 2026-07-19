#include "SuiteBridgeObservationService.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace vdrsuite::agent
{
namespace
{

bool positive(const std::chrono::milliseconds value)
{
    return value.count() > 0;
}

std::optional<SuiteBridgeObservationTimePoint> earlier(
    const std::optional<SuiteBridgeObservationTimePoint>& current,
    const SuiteBridgeObservationTimePoint candidate)
{
    if (!current || candidate < *current)
    {
        return candidate;
    }

    return current;
}

}

SuiteBridgeObservationService::SuiteBridgeObservationService(
    ISuiteBridgeLocalTransport& transport,
    SuiteBridgeObservationConfig config)
    : config_(std::move(config)),
      handshakeService_(transport)
{
    if (!positive(config_.pollInterval) ||
        !positive(config_.staleAfter) ||
        !positive(config_.offlineAfter) ||
        !positive(config_.reconnectInitial) ||
        !positive(config_.reconnectMaximum) ||
        config_.staleAfter < config_.pollInterval ||
        config_.offlineAfter <= config_.staleAfter ||
        config_.reconnectMaximum < config_.reconnectInitial)
    {
        throw std::invalid_argument(
            "invalid suite bridge observation timing configuration");
    }
}

void SuiteBridgeObservationService::start(
    const SuiteBridgeObservationTimePoint now)
{
    if (snapshot_.started)
    {
        return;
    }

    snapshot_.started = true;
    snapshot_.consecutiveFailures = 0;
    snapshot_.diagnostic.clear();
    snapshot_.nextAttemptAt = now;
    snapshot_.mutationsEnabled = false;
    rediscoveryRequired_ = true;
    setState(SuiteBridgeObservationState::Connecting, now);
}

void SuiteBridgeObservationService::stop(
    const SuiteBridgeObservationTimePoint now)
{
    if (!snapshot_.started)
    {
        return;
    }

    snapshot_.started = false;
    snapshot_.nextAttemptAt.reset();
    snapshot_.diagnostic = "suite bridge observation stopped";
    snapshot_.mutationsEnabled = false;
    rediscoveryRequired_ = true;
    setState(SuiteBridgeObservationState::Offline, now);
}

void SuiteBridgeObservationService::refresh(
    const SuiteBridgeObservationTimePoint now)
{
    if (!snapshot_.started)
    {
        return;
    }

    updateFreshness(now);
}

void SuiteBridgeObservationService::attempt(
    const SuiteBridgeObservationTimePoint now)
{
    if (!attemptDue(now))
    {
        return;
    }

    snapshot_.lastAttemptAt = now;
    snapshot_.diagnostic.clear();
    snapshot_.mutationsEnabled = false;
    setState(SuiteBridgeObservationState::Connecting, now);

    if (rediscoveryRequired_ || !snapshot_.hasDiscovery)
    {
        SuiteBridgeHandshakeResult discovery =
            handshakeService_.discover();

        if (discovery.status != SuiteBridgeHandshakeStatus::Compatible)
        {
            handleFailure(discovery, now);
            return;
        }

        snapshot_.hasDiscovery = true;
        snapshot_.discovery = discovery.discovery;
        setState(SuiteBridgeObservationState::Compatible, now);
    }

    const SuiteBridgeHandshakeResult result =
        handshakeService_.readSnapshot(snapshot_.discovery);

    if (!result.ready())
    {
        handleFailure(result, now);
        return;
    }

    acceptReadySnapshot(result, now);
}

bool SuiteBridgeObservationService::attemptDue(
    const SuiteBridgeObservationTimePoint now) const
{
    return snapshot_.started &&
           snapshot_.nextAttemptAt &&
           now >= *snapshot_.nextAttemptAt;
}

std::optional<SuiteBridgeObservationTimePoint>
SuiteBridgeObservationService::nextWakeAt(
    const SuiteBridgeObservationTimePoint now) const
{
    if (!snapshot_.started)
    {
        return std::nullopt;
    }

    std::optional<SuiteBridgeObservationTimePoint> next =
        snapshot_.nextAttemptAt;

    if (!snapshot_.hasBaseline || !snapshot_.lastSuccessAt)
    {
        return next;
    }

    const SuiteBridgeObservationTimePoint staleAt =
        *snapshot_.lastSuccessAt + config_.staleAfter;
    const SuiteBridgeObservationTimePoint offlineAt =
        *snapshot_.lastSuccessAt + config_.offlineAfter;

    if (now < staleAt)
    {
        next = earlier(next, staleAt);
    }
    else if (now < offlineAt)
    {
        next = earlier(next, offlineAt);
    }

    return next;
}

const SuiteBridgeObservationSnapshot&
SuiteBridgeObservationService::snapshot() const
{
    return snapshot_;
}

void SuiteBridgeObservationService::setState(
    const SuiteBridgeObservationState state,
    const SuiteBridgeObservationTimePoint now)
{
    if (snapshot_.state == state)
    {
        return;
    }

    snapshot_.state = state;
    snapshot_.lastStateChangeAt = now;
}

void SuiteBridgeObservationService::acceptReadySnapshot(
    const SuiteBridgeHandshakeResult& result,
    const SuiteBridgeObservationTimePoint now)
{
    SuiteBridgeSnapshotBaseline previous;
    const bool hadPrevious = baselineTracker_.hasBaseline();

    if (hadPrevious)
    {
        previous = baselineTracker_.baseline();
    }

    const SuiteBridgeBaselineUpdate update =
        baselineTracker_.apply(result.baseline);

    if (update == SuiteBridgeBaselineUpdate::RejectedCounterRegression)
    {
        SuiteBridgeHandshakeResult rejected;
        rejected.status = SuiteBridgeHandshakeStatus::InvalidSnapshotPayload;
        rejected.diagnostic =
            "suite bridge counters regressed within one epoch";
        handleFailure(rejected, now);
        return;
    }

    snapshot_.hasDiscovery = true;
    snapshot_.discovery = result.discovery;
    snapshot_.hasBaseline = true;
    snapshot_.baseline = baselineTracker_.baseline();
    snapshot_.hasDelta = false;
    snapshot_.delta = {};

    if (hadPrevious &&
        update == SuiteBridgeBaselineUpdate::UpdatedComparable &&
        baselineTracker_.deltaAvailable())
    {
        snapshot_.hasDelta = true;
        snapshot_.delta.total =
            snapshot_.baseline.total - previous.total;
        snapshot_.delta.channelSwitch =
            snapshot_.baseline.channelSwitch - previous.channelSwitch;
        snapshot_.delta.recording =
            snapshot_.baseline.recording - previous.recording;
        snapshot_.delta.replaying =
            snapshot_.baseline.replaying - previous.replaying;
        snapshot_.delta.timerChange =
            snapshot_.baseline.timerChange - previous.timerChange;
    }

    snapshot_.lastSuccessAt = now;
    snapshot_.consecutiveFailures = 0;
    snapshot_.nextAttemptAt = now + config_.pollInterval;
    snapshot_.mutationsEnabled = false;
    rediscoveryRequired_ = false;

    if (update == SuiteBridgeBaselineUpdate::ReplacedEpochChanged)
    {
        snapshot_.diagnostic =
            "suite bridge counter epoch changed; baseline replaced";
    }
    else if (update == SuiteBridgeBaselineUpdate::ReplacedOverflowed ||
             snapshot_.baseline.counterOverflow)
    {
        snapshot_.diagnostic =
            "suite bridge counter continuity overflowed";
    }
    else
    {
        snapshot_.diagnostic.clear();
    }

    if (update == SuiteBridgeBaselineUpdate::ReplacedOverflowed ||
        snapshot_.baseline.counterOverflow)
    {
        setState(SuiteBridgeObservationState::Overflowed, now);
    }
    else
    {
        setState(SuiteBridgeObservationState::SnapshotCurrent, now);
    }
}

void SuiteBridgeObservationService::handleFailure(
    const SuiteBridgeHandshakeResult& result,
    const SuiteBridgeObservationTimePoint now)
{
    if (snapshot_.consecutiveFailures <
        std::numeric_limits<unsigned int>::max())
    {
        ++snapshot_.consecutiveFailures;
    }

    snapshot_.hasDelta = false;
    snapshot_.delta = {};
    snapshot_.diagnostic = boundedDiagnostic(result.diagnostic);
    snapshot_.mutationsEnabled = false;
    rediscoveryRequired_ = true;

    if (result.status == SuiteBridgeHandshakeStatus::NotConfigured)
    {
        snapshot_.nextAttemptAt.reset();
        setState(SuiteBridgeObservationState::NotConfigured, now);
        return;
    }

    scheduleReconnect(now);

    if (snapshot_.hasBaseline && snapshot_.lastSuccessAt)
    {
        updateFreshness(now);
        return;
    }

    setState(initialFailureState(result.status), now);
}

void SuiteBridgeObservationService::updateFreshness(
    const SuiteBridgeObservationTimePoint now)
{
    if (!snapshot_.hasBaseline || !snapshot_.lastSuccessAt)
    {
        return;
    }

    const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - *snapshot_.lastSuccessAt);

    if (age >= config_.offlineAfter)
    {
        setState(SuiteBridgeObservationState::Offline, now);
        return;
    }

    if (age >= config_.staleAfter)
    {
        setState(SuiteBridgeObservationState::SnapshotStale, now);
        return;
    }

    if (snapshot_.consecutiveFailures > 0)
    {
        setState(SuiteBridgeObservationState::TransportDegraded, now);
        return;
    }

    if (snapshot_.baseline.counterOverflow)
    {
        setState(SuiteBridgeObservationState::Overflowed, now);
        return;
    }

    setState(SuiteBridgeObservationState::SnapshotCurrent, now);
}

void SuiteBridgeObservationService::scheduleReconnect(
    const SuiteBridgeObservationTimePoint now)
{
    snapshot_.nextAttemptAt = now + reconnectDelay();
}

std::chrono::milliseconds
SuiteBridgeObservationService::reconnectDelay() const
{
    std::chrono::milliseconds delay = config_.reconnectInitial;

    for (unsigned int failure = 1;
         failure < snapshot_.consecutiveFailures &&
         delay < config_.reconnectMaximum;
         ++failure)
    {
        if (delay.count() > config_.reconnectMaximum.count() / 2)
        {
            delay = config_.reconnectMaximum;
            break;
        }

        delay *= 2;
    }

    return std::min(delay, config_.reconnectMaximum);
}

bool SuiteBridgeObservationService::incompatibleStatus(
    const SuiteBridgeHandshakeStatus status)
{
    switch (status)
    {
        case SuiteBridgeHandshakeStatus::DiscoveryReplyRejected:
        case SuiteBridgeHandshakeStatus::DiscoveryPayloadTooLarge:
        case SuiteBridgeHandshakeStatus::InvalidDiscoveryPayload:
        case SuiteBridgeHandshakeStatus::UnexpectedPlugin:
        case SuiteBridgeHandshakeStatus::IncompatibleDiscoverySchema:
        case SuiteBridgeHandshakeStatus::IncompatibleCapabilitySchema:
        case SuiteBridgeHandshakeStatus::IncompatibleSnapshotSchema:
        case SuiteBridgeHandshakeStatus::IncompatibleLocalContractSchema:
        case SuiteBridgeHandshakeStatus::RequiredCapabilityUnavailable:
        case SuiteBridgeHandshakeStatus::SnapshotReplyRejected:
        case SuiteBridgeHandshakeStatus::SnapshotPayloadTooLarge:
        case SuiteBridgeHandshakeStatus::InvalidSnapshotPayload:
        case SuiteBridgeHandshakeStatus::SnapshotInactive:
            return true;
        case SuiteBridgeHandshakeStatus::Compatible:
        case SuiteBridgeHandshakeStatus::Ready:
        case SuiteBridgeHandshakeStatus::NotConfigured:
        case SuiteBridgeHandshakeStatus::PluginMissing:
        case SuiteBridgeHandshakeStatus::LegacyOrUnknown:
        case SuiteBridgeHandshakeStatus::DiscoveryTransportError:
        case SuiteBridgeHandshakeStatus::SnapshotTransportError:
            return false;
    }

    return true;
}

SuiteBridgeObservationState
SuiteBridgeObservationService::initialFailureState(
    const SuiteBridgeHandshakeStatus status)
{
    if (status == SuiteBridgeHandshakeStatus::NotConfigured)
    {
        return SuiteBridgeObservationState::NotConfigured;
    }

    if (status == SuiteBridgeHandshakeStatus::PluginMissing)
    {
        return SuiteBridgeObservationState::PluginMissing;
    }

    if (status == SuiteBridgeHandshakeStatus::LegacyOrUnknown)
    {
        return SuiteBridgeObservationState::LegacyOrUnknown;
    }

    if (incompatibleStatus(status))
    {
        return SuiteBridgeObservationState::Incompatible;
    }

    return SuiteBridgeObservationState::Offline;
}

std::string SuiteBridgeObservationService::boundedDiagnostic(
    const std::string& diagnostic)
{
    static constexpr std::size_t MaximumDiagnosticBytes = 256;

    std::string bounded = diagnostic;

    for (char& character : bounded)
    {
        if (character == '\r' || character == '\n')
        {
            character = ' ';
        }
    }

    if (bounded.size() <= MaximumDiagnosticBytes)
    {
        return bounded;
    }

    bounded.resize(MaximumDiagnosticBytes - 3);
    bounded += "...";
    return bounded;
}

}