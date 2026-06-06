#include "VdrSnapshotBuilder.h"

#include "VdrService.h"

#include <chrono>
#include <string>

VdrSnapshotBuilder::VdrSnapshotBuilder(
    VdrService& vdrService,
    IRuntimeLogger* logger,
    IRuntimeMeasurementSink* measurementSink)
    : vdrService_(vdrService),
      logger_(logger),
      measurementSink_(measurementSink)
{
}

void VdrSnapshotBuilder::log(RuntimeLogLevel level, const std::string& message) const
{
    if (logger_ == nullptr) {
        return;
    }

    logger_->write(RuntimeLogEntry{level, "VdrSnapshotBuilder", message});
}

void VdrSnapshotBuilder::recordMeasurement(const RuntimeMeasurement& measurement) const
{
    if (measurementSink_ == nullptr) {
        return;
    }

    measurementSink_->recordMeasurement(measurement);
}

VdrStatus VdrSnapshotBuilder::buildStatus() const
{
    const auto started = std::chrono::steady_clock::now();
    auto result = vdrService_.getStatus();
    const auto finished = std::chrono::steady_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();

    log(RuntimeLogLevel::Info, "Build status finished (" + std::to_string(durationMs) + " ms)");

    RuntimeMeasurement measurement;
    measurement.component = "VdrSnapshotBuilder";
    measurement.operation = "Build status";
    measurement.durationMs = durationMs;
    recordMeasurement(measurement);

    return result;
}

std::vector<VdrRecording> VdrSnapshotBuilder::buildRecordings() const
{
    const auto started = std::chrono::steady_clock::now();
    auto result = vdrService_.getRecordings();
    const auto finished = std::chrono::steady_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();

    log(RuntimeLogLevel::Info, "Build recordings finished (" + std::to_string(durationMs) + " ms)");

    RuntimeMeasurement measurement;
    measurement.component = "VdrSnapshotBuilder";
    measurement.operation = "Build recordings";
    measurement.durationMs = durationMs;
    measurement.itemCount = result.size();
    recordMeasurement(measurement);

    return result;
}

std::vector<VdrTimer> VdrSnapshotBuilder::buildTimers() const
{
    const auto started = std::chrono::steady_clock::now();
    auto result = vdrService_.getTimers();
    const auto finished = std::chrono::steady_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();

    log(RuntimeLogLevel::Info, "Build timers finished (" + std::to_string(durationMs) + " ms)");

    RuntimeMeasurement measurement;
    measurement.component = "VdrSnapshotBuilder";
    measurement.operation = "Build timers";
    measurement.durationMs = durationMs;
    measurement.itemCount = result.size();
    recordMeasurement(measurement);

    return result;
}

std::vector<VdrChannel> VdrSnapshotBuilder::buildChannels() const
{
    const auto started = std::chrono::steady_clock::now();
    auto result = vdrService_.getChannels();
    const auto finished = std::chrono::steady_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();

    log(RuntimeLogLevel::Info, "Build channels finished (" + std::to_string(durationMs) + " ms)");

    RuntimeMeasurement measurement;
    measurement.component = "VdrSnapshotBuilder";
    measurement.operation = "Build channels";
    measurement.durationMs = durationMs;
    measurement.itemCount = result.size();
    recordMeasurement(measurement);

    return result;
}

std::vector<VdrEvent> VdrSnapshotBuilder::buildEvents() const
{
    const auto started = std::chrono::steady_clock::now();
    auto result = vdrService_.getEvents();
    const auto finished = std::chrono::steady_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();

    log(RuntimeLogLevel::Info, "Build events finished (" + std::to_string(durationMs) + " ms)");

    RuntimeMeasurement measurement;
    measurement.component = "VdrSnapshotBuilder";
    measurement.operation = "Build events";
    measurement.durationMs = durationMs;
    measurement.itemCount = result.size();
    recordMeasurement(measurement);

    return result;
}

VdrSnapshot VdrSnapshotBuilder::buildSnapshot() const
{
    VdrSnapshot snapshot;

    snapshot.status = buildStatus();
    snapshot.recordings = buildRecordings();
    snapshot.timers = buildTimers();
    snapshot.channels = buildChannels();
    snapshot.events = buildEvents();

    return snapshot;
}
