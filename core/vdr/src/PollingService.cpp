#include "PollingService.h"

#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <chrono>
#include <string>

namespace {

long long elapsedMilliseconds(const std::chrono::steady_clock::time_point& started)
{
    const auto finished = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();
}

} // namespace

PollingService::PollingService(
    VdrSnapshotBuilder& snapshotBuilder,
    VdrService& vdrService,
    SnapshotCacheService& snapshotCacheService,
    IRuntimeLogger* logger,
    IRuntimeMeasurementSink* measurementSink)
    : PollingService(
          snapshotBuilder,
          vdrService,
          snapshotCacheService,
          "default",
          logger,
          measurementSink)
{
}

PollingService::PollingService(
    VdrSnapshotBuilder& snapshotBuilder,
    VdrService& vdrService,
    SnapshotCacheService& snapshotCacheService,
    const std::string& backendId,
    IRuntimeLogger* logger,
    IRuntimeMeasurementSink* measurementSink)
    : PollingService(
          snapshotBuilder,
          vdrService,
          snapshotCacheService,
          backendId,
          DomainRefreshPolicy(),
          logger,
          measurementSink)
{
}

PollingService::PollingService(
    VdrSnapshotBuilder& snapshotBuilder,
    VdrService& vdrService,
    SnapshotCacheService& snapshotCacheService,
    const std::string& backendId,
    DomainRefreshPolicy refreshPolicy,
    IRuntimeLogger* logger,
    IRuntimeMeasurementSink* measurementSink)
    : snapshotBuilder_(snapshotBuilder),
      vdrService_(vdrService),
      snapshotCacheService_(snapshotCacheService),
      backendId_(backendId),
      logger_(logger),
      measurementSink_(measurementSink),
      changeDetectionService_(),
      snapshotRefreshPlanner_(refreshPolicy),
      hasChangeState_(false)
{
}

void PollingService::log(RuntimeLogLevel level, const std::string& message) const
{
    if (logger_ == nullptr) {
        return;
    }

    logger_->write(RuntimeLogEntry{level, "PollingService", message});
}

void PollingService::recordMeasurement(const RuntimeMeasurement& measurement) const
{
    if (measurementSink_ == nullptr) {
        return;
    }

    measurementSink_->recordMeasurement(measurement);
}

void PollingService::poll()
{
    const auto pollStarted = std::chrono::steady_clock::now();

    const VdrChangeState currentChangeState = vdrService_.getChangeState();

    changeEvents_.clear();
    lastUpdatePlan_ = SnapshotUpdatePlan();

    if (!hasChangeState_) {
        const auto initialPollStarted = std::chrono::steady_clock::now();
        snapshotCacheService_.updateSnapshotForBackend(backendId_, snapshotBuilder_.buildStartupSnapshot());

        RuntimeMeasurement initialPollMeasurement;
        initialPollMeasurement.component = "PollingService";
        initialPollMeasurement.operation = "Initial snapshot poll";
        initialPollMeasurement.durationMs = elapsedMilliseconds(initialPollStarted);
        recordMeasurement(initialPollMeasurement);

        lastChangeState_ = currentChangeState;
        hasChangeState_ = true;

        RuntimeMeasurement pollMeasurement;
        pollMeasurement.component = "PollingService";
        pollMeasurement.operation = "Poll cycle";
        pollMeasurement.durationMs = elapsedMilliseconds(pollStarted);
        recordMeasurement(pollMeasurement);

        return;
    }

    const auto changeDetectionStarted = std::chrono::steady_clock::now();
    changeEvents_ = changeDetectionService_.detectChanges(lastChangeState_, currentChangeState);

    RuntimeMeasurement changeDetectionMeasurement;
    changeDetectionMeasurement.component = "PollingService";
    changeDetectionMeasurement.operation = "Detect changes";
    changeDetectionMeasurement.durationMs = elapsedMilliseconds(changeDetectionStarted);
    changeDetectionMeasurement.itemCount = changeEvents_.size();
    recordMeasurement(changeDetectionMeasurement);

    const auto updatePlanStarted = std::chrono::steady_clock::now();
    lastUpdatePlan_ = snapshotRefreshPlanner_.createPlan(changeEvents_);

    RuntimeMeasurement updatePlanMeasurement;
    updatePlanMeasurement.component = "PollingService";
    updatePlanMeasurement.operation = "Create update plan";
    updatePlanMeasurement.durationMs = elapsedMilliseconds(updatePlanStarted);
    updatePlanMeasurement.itemCount = changeEvents_.size();
    recordMeasurement(updatePlanMeasurement);

    if (lastUpdatePlan_.requiresFullSnapshot()) {
        const auto fullRefreshStarted = std::chrono::steady_clock::now();
        snapshotCacheService_.updateSnapshotForBackend(backendId_, snapshotBuilder_.buildSnapshot());

        RuntimeMeasurement fullRefreshMeasurement;
        fullRefreshMeasurement.component = "PollingService";
        fullRefreshMeasurement.operation = "Full snapshot refresh";
        fullRefreshMeasurement.durationMs = elapsedMilliseconds(fullRefreshStarted);
        recordMeasurement(fullRefreshMeasurement);

        lastChangeState_ = currentChangeState;

        RuntimeMeasurement pollMeasurement;
        pollMeasurement.component = "PollingService";
        pollMeasurement.operation = "Poll cycle";
        pollMeasurement.durationMs = elapsedMilliseconds(pollStarted);
        recordMeasurement(pollMeasurement);

        return;
    }

    if (lastUpdatePlan_.hasRefreshWork()) {
        const auto partialRefreshStarted = std::chrono::steady_clock::now();

        if (lastUpdatePlan_.shouldRefreshStatus()) {
            const auto refreshStarted = std::chrono::steady_clock::now();
            snapshotCacheService_.updateStatusForBackend(backendId_, snapshotBuilder_.buildStatus());

            RuntimeMeasurement measurement;
            measurement.component = "PollingService";
            measurement.operation = "Status refresh";
            measurement.durationMs = elapsedMilliseconds(refreshStarted);
            recordMeasurement(measurement);
        }

        if (lastUpdatePlan_.shouldRefreshRecordings()) {
            const auto refreshStarted = std::chrono::steady_clock::now();
            snapshotCacheService_.updateRecordingsForBackend(backendId_, snapshotBuilder_.buildRecordings());

            RuntimeMeasurement measurement;
            measurement.component = "PollingService";
            measurement.operation = "Recordings refresh";
            measurement.durationMs = elapsedMilliseconds(refreshStarted);
            recordMeasurement(measurement);
        }

        if (lastUpdatePlan_.shouldRefreshTimers()) {
            const auto refreshStarted = std::chrono::steady_clock::now();
            snapshotCacheService_.updateTimersForBackend(backendId_, snapshotBuilder_.buildTimers());

            RuntimeMeasurement measurement;
            measurement.component = "PollingService";
            measurement.operation = "Timers refresh";
            measurement.durationMs = elapsedMilliseconds(refreshStarted);
            recordMeasurement(measurement);
        }

        if (lastUpdatePlan_.shouldRefreshChannels()) {
            const auto refreshStarted = std::chrono::steady_clock::now();
            snapshotCacheService_.updateChannelsForBackend(backendId_, snapshotBuilder_.buildChannels());

            RuntimeMeasurement measurement;
            measurement.component = "PollingService";
            measurement.operation = "Channels refresh";
            measurement.durationMs = elapsedMilliseconds(refreshStarted);
            recordMeasurement(measurement);
        }

        if (lastUpdatePlan_.shouldRefreshEvents()) {
            const auto refreshStarted = std::chrono::steady_clock::now();
            snapshotCacheService_.updateEventsForBackend(backendId_, snapshotBuilder_.buildEvents());

            RuntimeMeasurement measurement;
            measurement.component = "PollingService";
            measurement.operation = "Events refresh";
            measurement.durationMs = elapsedMilliseconds(refreshStarted);
            recordMeasurement(measurement);
        }

        if (lastUpdatePlan_.hasSelectiveEventRefresh()) {
            const auto refreshStarted = std::chrono::steady_clock::now();
            const auto events = snapshotBuilder_.buildEvents(lastUpdatePlan_.selectiveEventQuery());

            snapshotCacheService_.mergeEventsForBackend(backendId_, events);

            RuntimeMeasurement measurement;
            measurement.component = "PollingService";
            measurement.operation = "Selective events merge refresh";
            measurement.durationMs = elapsedMilliseconds(refreshStarted);
            measurement.itemCount = events.size();
            recordMeasurement(measurement);
        }

        RuntimeMeasurement partialRefreshMeasurement;
        partialRefreshMeasurement.component = "PollingService";
        partialRefreshMeasurement.operation = "Partial refresh";
        partialRefreshMeasurement.durationMs = elapsedMilliseconds(partialRefreshStarted);
        recordMeasurement(partialRefreshMeasurement);

        lastChangeState_ = currentChangeState;
    }

    RuntimeMeasurement pollMeasurement;
    pollMeasurement.component = "PollingService";
    pollMeasurement.operation = "Poll cycle";
    pollMeasurement.durationMs = elapsedMilliseconds(pollStarted);
    recordMeasurement(pollMeasurement);
}

const VdrSnapshot& PollingService::snapshot() const
{
    return snapshotCacheService_.cache().snapshot();
}

const std::vector<VdrChangeEvent>& PollingService::changeEvents() const
{
    return changeEvents_;
}

const SnapshotUpdatePlan& PollingService::lastUpdatePlan() const
{
    return lastUpdatePlan_;
}
