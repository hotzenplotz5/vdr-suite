#pragma once

#include "DashboardController.h"

#include <string>

class JobsController;
class MetadataController;
class RecordingsController;
class RuntimeDiagnosticsController;
class SnapshotChangeFeedController;
class VdrController;

class ApiRouter
{
public:
    ApiRouter(
        DashboardController& dashboardController,
        JobsController& jobsController,
        RecordingsController& recordingsController,
        MetadataController& metadataController,
        VdrController& vdrController,
        RuntimeDiagnosticsController& runtimeDiagnosticsController,
        SnapshotChangeFeedController& snapshotChangeFeedController);

    ApiResponse handleGet(
        const std::string& path);

private:
    DashboardController& dashboardController_;
    JobsController& jobsController_;
    RecordingsController& recordingsController_;
    MetadataController& metadataController_;
    VdrController& vdrController_;
    RuntimeDiagnosticsController& runtimeDiagnosticsController_;
    SnapshotChangeFeedController& snapshotChangeFeedController_;
};
