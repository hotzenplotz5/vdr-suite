#pragma once

#include "ISuiteBridgeLocalTransport.h"
#include "SuiteBridgeHandshakeService.h"
#include "SuiteBridgeObservation.h"

#include <chrono>
#include <optional>

namespace vdrsuite::agent
{

class SuiteBridgeObservationService
{
public:
    explicit SuiteBridgeObservationService(
        ISuiteBridgeLocalTransport& transport,
        SuiteBridgeObservationConfig config = {});

    void start(SuiteBridgeObservationTimePoint now);
    void stop(SuiteBridgeObservationTimePoint now);

    void refresh(SuiteBridgeObservationTimePoint now);
    void attempt(SuiteBridgeObservationTimePoint now);

    bool attemptDue(SuiteBridgeObservationTimePoint now) const;

    std::optional<SuiteBridgeObservationTimePoint> nextWakeAt(
        SuiteBridgeObservationTimePoint now) const;

    const SuiteBridgeObservationSnapshot& snapshot() const;

private:
    void setState(
        SuiteBridgeObservationState state,
        SuiteBridgeObservationTimePoint now);

    void acceptReadySnapshot(
        const SuiteBridgeHandshakeResult& result,
        SuiteBridgeObservationTimePoint now);

    void handleFailure(
        const SuiteBridgeHandshakeResult& result,
        SuiteBridgeObservationTimePoint now);

    void updateFreshness(
        SuiteBridgeObservationTimePoint now);

    void scheduleReconnect(
        SuiteBridgeObservationTimePoint now);

    std::chrono::milliseconds reconnectDelay() const;

    static bool incompatibleStatus(
        SuiteBridgeHandshakeStatus status);

    static SuiteBridgeObservationState initialFailureState(
        SuiteBridgeHandshakeStatus status);

    static std::string boundedDiagnostic(
        const std::string& diagnostic);

    SuiteBridgeObservationConfig config_;
    SuiteBridgeHandshakeService handshakeService_;
    SuiteBridgeBaselineTracker baselineTracker_;
    SuiteBridgeObservationSnapshot snapshot_;
    bool rediscoveryRequired_ = true;
};

}