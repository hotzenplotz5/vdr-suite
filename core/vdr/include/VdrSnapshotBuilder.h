#ifndef VDR_SNAPSHOT_BUILDER_H
#define VDR_SNAPSHOT_BUILDER_H

#include "IRuntimeLogger.h"
#include "IRuntimeMeasurementSink.h"
#include "RuntimeLogLevel.h"
#include "RuntimeMeasurement.h"
#include "VdrChannel.h"
#include "VdrEvent.h"
#include "VdrEventQuery.h"
#include "VdrRecording.h"
#include "VdrSnapshot.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <string>
#include <vector>

class VdrService;

class VdrSnapshotBuilder {
public:
    explicit VdrSnapshotBuilder(
        VdrService& vdrService,
        IRuntimeLogger* logger = nullptr,
        IRuntimeMeasurementSink* measurementSink = nullptr);

    explicit VdrSnapshotBuilder(
        VdrService& vdrService,
        const std::string& backendId,
        IRuntimeLogger* logger,
        IRuntimeMeasurementSink* measurementSink);

    VdrStatus buildStatus() const;
    std::vector<VdrRecording> buildRecordings() const;
    std::vector<VdrTimer> buildTimers() const;
    std::vector<VdrChannel> buildChannels() const;
    std::vector<VdrEvent> buildEvents() const;
    std::vector<VdrEvent> buildEvents(const VdrEventQuery& query) const;

    VdrSnapshot buildSnapshot() const;
    VdrSnapshot buildSnapshotWithoutEvents() const;

private:
    VdrService& vdrService_;
    std::string backendId_;
    IRuntimeLogger* logger_;
    IRuntimeMeasurementSink* measurementSink_;

    void log(RuntimeLogLevel level, const std::string& message) const;
    void recordMeasurement(const RuntimeMeasurement& measurement) const;
};

#endif
