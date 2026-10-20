#include "DaemonRuntime.h"

#include "NativeTimerCreateProductiveRuntime.h"
#include "NativeTimerDeleteProductiveRuntime.h"
#include "NativeTimerModifyProductiveRuntime.h"
#include "RestfulApiNativeTimerModifyReadbackReader.h"
#include "NativeTimerModifyReadbackVerificationService.h"
#include "NativeTimerBindingReadbackService.h"
#include "NativeTimerAbsenceReadbackExpectation.h"
#include "RestfulApiNativeTimerInventoryReader.h"
#include "VdrManagedTimerCreateReadbackEvidenceBuilder.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <map>
#include <optional>
#include <string>

void DaemonRuntime::pollVdrAndUpdateChangeFeed()
{
    backendPollingCoordinator_->pollAll();

    // Refresh unchanged, verified managed bindings well before the existing
    // five-minute public API mutation gate expires. Never use cached
    // snapshots or trigger discovery, reconciliation or a backend write.
    // The repository atomically blocks revision churn for any nonterminal
    // timer mutation, including accepted work not yet dispatched.
    if (nativeTimerBindingRepository_ && backendAgentRepository_)
    {
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (now > 210)
        {
            const auto candidates =
                nativeTimerBindingRepository_->listManagedForFreshness(
                    nativeTimerRefreshCursor_, now - 180, 4);
            if (candidates.ok())
            {
                if (candidates.bindings.empty())
                    nativeTimerRefreshCursor_.clear();
                vdrsuite::timers::NativeTimerBindingReadbackService service(
                    *nativeTimerBindingRepository_);
                for (const auto& binding : candidates.bindings)
                {
                    nativeTimerRefreshCursor_ = binding.nativeTimerBindingId;
                    const auto agent = backendAgentRepository_
                        ->findAgentForBackend(binding.backendId);
                    if (!agent || agent->revoked || agent->incompatible ||
                        agent->leaseExpiresAt < now ||
                        agent->backendGeneration != binding.backendGeneration)
                        continue;

                    for (const auto& context : backendRuntimeContexts_)
                    {
                        if (!context || !context->httpClient ||
                            context->backendId != binding.backendId)
                            continue;
                        try
                        {
                            vdrsuite::vdr::RestfulApiNativeTimerModifyReadbackReader
                                reader(*context->httpClient);
                            vdrsuite::vdr::NativeTimerModifyReadRequest request;
                            request.backendId = binding.backendId;
                            request.backendGeneration = binding.backendGeneration;
                            request.backendNativeTimerId =
                                binding.backendNativeTimerId;
                            request.observedAt = std::chrono::duration_cast<
                                std::chrono::seconds>(
                                    std::chrono::system_clock::now()
                                        .time_since_epoch()).count();
                            request.notBefore = request.observedAt;
                            const auto observed = reader.read(request);
                            const auto finish = std::chrono::duration_cast<
                                std::chrono::seconds>(
                                    std::chrono::system_clock::now()
                                        .time_since_epoch()).count();
                            const auto stillAgent = backendAgentRepository_
                                ->findAgentForBackend(binding.backendId);
                            if (observed.ok() && finish >= request.observedAt &&
                                finish - request.observedAt <= 30 &&
                                stillAgent && !stillAgent->revoked &&
                                !stillAgent->incompatible &&
                                stillAgent->leaseExpiresAt >= finish &&
                                stillAgent->backendGeneration ==
                                    binding.backendGeneration)
                            {
                                // A changed fingerprint or an in-flight
                                // operation is a no-write, fail-closed result.
                                service.applyPresentObservation(
                                    observed.observation, true);
                            }
                        }
                        catch (const std::exception& error)
                        {
                            std::cerr << "[WARNING] Native Timer passive "
                                         "readback unavailable for backend "
                                      << binding.backendId << ": "
                                      << error.what() << std::endl;
                        }
                        catch (...)
                        {
                            std::cerr << "[WARNING] Native Timer passive "
                                         "readback failed for backend "
                                      << binding.backendId << std::endl;
                        }
                        break;
                    }
                }
            }
        }
    }

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

                try
                {
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
                }
                catch (const std::exception& error)
                {
                    std::cerr
                        << "[WARNING] Native Timer CREATE readback unavailable for backend "
                        << expectation.backendId
                        << ": " << error.what()
                        << std::endl;
                }
                catch (...)
                {
                    std::cerr
                        << "[WARNING] Native Timer CREATE readback unavailable for backend "
                        << expectation.backendId
                        << ": unknown exception"
                        << std::endl;
                }
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

    // Phase-64 native DELETE is an ADR-0042 Agent operation. The writer
    // remains dormant until an actual capability is advertised and selected.
    // Accepted/uncertain mutations are processed only through durable
    // assignment and complete native-inventory readback.
    if (mutationOperationRepository_ &&
        backendAgentCommandRepository_ &&
        backendAgentRepository_ &&
        nativeTimerBindingRepository_ &&
        backendAgentNativeTimerDeleteAssignmentService_ &&
        nativeTimerDeleteDispatchService_ &&
        nativeTimerAbsenceReadbackVerificationService_ &&
        nativeTimerDeleteOperationCompletionService_)
    {
        const vdrsuite::daemon::NativeDeleteReadbackAcquirer acquireDelete =
            [this](
                const vdrsuite::timers::NativeTimerAbsenceReadbackExpectation&
                    expectation)
                -> std::optional<vdrsuite::timers::NativeTimerInventoryEvidence>
            {
                const auto agent = backendAgentRepository_
                    ->findAgentForBackend(expectation.backendId);
                if (!agent || agent->revoked || agent->incompatible ||
                    agent->backendGeneration != expectation.backendGeneration)
                    return std::nullopt;

                for (const auto& context : backendRuntimeContexts_)
                {
                    if (!context || !context->httpClient ||
                        context->backendId != expectation.backendId)
                        continue;
                    const auto observedAt =
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now()
                                .time_since_epoch()).count();
                    if (observedAt < expectation.readbackNotBefore)
                        return std::nullopt;
                    vdrsuite::vdr::RestfulApiNativeTimerInventoryReader reader(
                        *context->httpClient);
                    vdrsuite::vdr::RestfulApiNativeTimerInventoryReadRequest request;
                    request.backendId = expectation.backendId;
                    request.backendGeneration = expectation.backendGeneration;
                    request.observedAt = observedAt;
                    const auto inventory = reader.read(request);
                    if (!inventory.ok() ||
                        inventory.evidence.completeness !=
                            vdrsuite::timers::NativeTimerInventoryCompleteness::complete)
                        return std::nullopt;
                    return inventory.evidence;
                }
                return std::nullopt;
            };

        vdrsuite::daemon::advanceNativeTimerDeleteRuntimeOnce(
            *mutationOperationRepository_, *backendAgentCommandRepository_,
            *backendAgentRepository_, *nativeTimerBindingRepository_,
            *backendAgentNativeTimerDeleteAssignmentService_,
            *nativeTimerDeleteDispatchService_,
            *nativeTimerAbsenceReadbackVerificationService_,
            *nativeTimerDeleteOperationCompletionService_,
            acquireDelete,
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
    }

    // UPDATE and TOGGLE share a provider-fenced Agent but have distinct
    // operation families. A full, current native field readback is required;
    // cached snapshots cannot certify a successful mutation.
    if (mutationOperationRepository_ &&
        backendAgentCommandRepository_ && backendAgentRepository_ &&
        timerIntentRepository_ && timerAssignmentRepository_ &&
        nativeTimerBindingRepository_ &&
        backendAgentNativeTimerModifyAssignmentService_ &&
        nativeTimerModifyDispatchService_ &&
        nativeTimerModifyReadbackVerificationService_ &&
        nativeTimerModifyOperationCompletionService_)
    {
        const vdrsuite::daemon::NativeModifyReadbackAcquirer acquireModify =
            [this](
                const vdrsuite::timers::NativeTimerModifyReadbackExpectation&
                    expectation)
                -> std::optional<vdrsuite::timers::NativeTimerObservation>
            {
                const auto& target = expectation.payload;
                const auto agent = backendAgentRepository_
                    ->findAgentForBackend(target.backendId);
                if (!agent || agent->revoked || agent->incompatible ||
                    agent->backendGeneration != target.backendGeneration)
                    return std::nullopt;

                const auto observedAt =
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now()
                            .time_since_epoch()).count();
                if (observedAt < expectation.readbackNotBefore)
                    return std::nullopt;

                for (const auto& context : backendRuntimeContexts_)
                {
                    if (!context || !context->httpClient ||
                        context->backendId != target.backendId)
                        continue;
                    vdrsuite::vdr::RestfulApiNativeTimerModifyReadbackReader reader(
                        *context->httpClient);
                    vdrsuite::vdr::NativeTimerModifyReadRequest request;
                    request.backendId = target.backendId;
                    request.backendGeneration = target.backendGeneration;
                    request.backendNativeTimerId =
                        target.backendNativeTimerId;
                    request.observedAt = observedAt;
                    request.notBefore = expectation.readbackNotBefore;
                    const auto read = reader.read(request);
                    if (read.ok()) return read.observation;
                }
                return std::nullopt;
            };

        vdrsuite::daemon::advanceNativeTimerModifyRuntimeOnce(
            *mutationOperationRepository_, *backendAgentCommandRepository_,
            *backendAgentRepository_, *timerIntentRepository_,
            *timerAssignmentRepository_, *nativeTimerBindingRepository_,
            *backendAgentNativeTimerModifyAssignmentService_,
            *nativeTimerModifyDispatchService_,
            *nativeTimerModifyReadbackVerificationService_,
            *nativeTimerModifyOperationCompletionService_,
            acquireModify,
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
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
