#include "DaemonRuntime.h"

#include "NativeTimerCreateProductiveRuntime.h"
#include "RestfulApiNativeTimerInventoryReader.h"
#include "VdrManagedTimerCreateReadbackEvidenceBuilder.h"

#include <chrono>
#include <map>
#include <optional>
#include <string>

void DaemonRuntime::pollVdrAndUpdateChangeFeed()
{
    backendPollingCoordinator_->pollAll();

    using NativeCreateEvidence =
        vdrsuite::timers::NativeTimerCreateReadbackEvidence;
    std::map<std::string, std::optional<NativeCreateEvidence>>
        nativeCreateReadbackCache;

    const auto acquireNativeCreateReadback =
        [this, &nativeCreateReadbackCache](
            const vdrsuite::timers::NativeTimerCreateReadbackExpectation&
                expectation)
            -> std::optional<NativeCreateEvidence>
        {
            const std::string cacheKey =
                expectation.backendId + ":" +
                std::to_string(expectation.backendGeneration);
            const auto cached = nativeCreateReadbackCache.find(cacheKey);
            if (cached != nativeCreateReadbackCache.end())
                return cached->second;

            std::optional<NativeCreateEvidence> evidence;
            const auto agent = backendAgentRepository_
                ? backendAgentRepository_->findAgentForBackend(
                    expectation.backendId)
                : std::nullopt;
            if (!agent.has_value() ||
                agent->revoked ||
                agent->incompatible ||
                agent->backendGeneration != expectation.backendGeneration)
            {
                nativeCreateReadbackCache.emplace(cacheKey, evidence);
                return evidence;
            }

            for (const auto& context : backendRuntimeContexts_)
            {
                if (!context ||
                    context->backendId != expectation.backendId ||
                    !context->httpClient ||
                    !context->service)
                    continue;

                const std::vector<VdrTimer> timers =
                    context->service->getTimers();
                const std::int64_t observedAt =
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now()
                            .time_since_epoch()).count();
                if (observedAt <= 0)
                    break;

                vdrsuite::vdr::RestfulApiNativeTimerInventoryReader reader(
                    *context->httpClient);
                vdrsuite::vdr::RestfulApiNativeTimerInventoryReadRequest request;
                request.backendId = expectation.backendId;
                request.backendGeneration = expectation.backendGeneration;
                request.observedAt = observedAt;
                const auto inventory = reader.read(request);
                if (!inventory.ok())
                    break;

                const auto built =
                    VdrManagedTimerCreateReadbackEvidenceBuilder::build(
                        inventory.evidence, timers);
                if (built.ok())
                    evidence = built.evidence;
                break;
            }

            nativeCreateReadbackCache.emplace(cacheKey, evidence);
            return evidence;
        };

    if (mutationOperationRepository_ &&
        backendAgentCommandRepository_ &&
        backendAgentNativeTimerCreateReservationService_ &&
        nativeTimerCreateDispatchService_ &&
        backendAgentNativeTimerCreateActivationService_ &&
        nativeTimerCreateReadbackVerificationService_ &&
        timerAssignmentFulfillmentService_ &&
        nativeTimerCreateOperationCompletionService_)
    {
        vdrsuite::daemon::advanceNativeTimerCreateRuntimeOnce(
            *mutationOperationRepository_,
            *backendAgentCommandRepository_,
            *backendAgentNativeTimerCreateReservationService_,
            *nativeTimerCreateDispatchService_,
            *backendAgentNativeTimerCreateActivationService_,
            *nativeTimerCreateReadbackVerificationService_,
            *timerAssignmentFulfillmentService_,
            *nativeTimerCreateOperationCompletionService_,
            acquireNativeCreateReadback,
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now()
                    .time_since_epoch()).count());
    }

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        const int previousLatestSequenceNumber =
            snapshotChangeFeed_->latestSequenceNumber();

        std::vector<VdrChangeEvent> readyChanges;
        for (const auto& change : backendRuntimeContext->pollingService->changeEvents()) {
            if (change.type() == VdrChangeType::RecordingsChanged) {
                // Polling is also the recovery path when backend SSE is absent.
                // Publish recordings only after the folder cache has committed.
                recordingCacheRefreshQueue_.request(backendRuntimeContext->backendId);
            } else {
                readyChanges.push_back(change);
            }
        }

        if (backendRuntimeContext->suiteBridgeAgentRuntime) {
            const auto health =
                backendRuntimeContext->suiteBridgeAgentRuntime->health();
            const auto& observation = health.observation;

            if (embeddedBackendLifecycleService_) {
                const std::int64_t lifecycleNow =
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now()
                            .time_since_epoch()).count();
                const bool healthy =
                    health.running &&
                    observation.state ==
                        vdrsuite::agent::SuiteBridgeObservationState::SnapshotCurrent;
                embeddedBackendLifecycleService_->maintainBackend(
                    backendRuntimeContext->backendId,
                    healthy,
                    lifecycleNow);
            }

            if (observation.hasBaseline) {
                const bool snapshotCurrent =
                    observation.state ==
                    vdrsuite::agent::SuiteBridgeObservationState::SnapshotCurrent;

                if (backendRuntimeContext->recordingListChangeTracker.observe(
                        observation.baseline.counterEpoch,
                        observation.baseline.recordingList,
                        snapshotCurrent,
                        observation.baseline.counterOverflow)) {
                    // SuiteBridge is an additional native dirty hint only.
                    // The cache worker remains the sole inventory reader/writer,
                    // and completion is published only after a successful commit.
                    recordingCacheRefreshQueue_.request(
                        backendRuntimeContext->backendId);
                }

                if (backendRuntimeContext->recordingMarksChangeTracker.observe(
                        observation.baseline.counterEpoch,
                        observation.baseline.marksModified,
                        snapshotCurrent,
                        observation.baseline.counterOverflow)) {
                    readyChanges.emplace_back(
                        VdrChangeType::RecordingMarksChanged);
                }
            }
        }

        snapshotChangeFeedService_->appendChanges(
            *snapshotChangeFeed_,
            snapshotCacheService_->generation(),
            readyChanges,
            backendRuntimeContext->backendId);

        for (const auto& entry : snapshotChangeFeed_->entries()) {
            if (entry.sequenceNumber() > previousLatestSequenceNumber) {
                liveTransportService_->publishChangeFeedEntry(entry);
            }
        }
    }
}
