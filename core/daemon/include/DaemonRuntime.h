#pragma once

#include "Database.h"
#include "DashboardFacade.h"
#include "JobDashboardService.h"
#include "JobRepository.h"
#include "MetadataRepository.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"
#include "RuntimeConfig.h"

#include <atomic>
#include <memory>

class DaemonRuntime
{
public:
    DaemonRuntime();

    bool initialize();
    int run();
    void shutdown();

private:
    static void handleSignal(int signalNumber);

    bool initialized_;

    RuntimeConfig config_;
    Database database_;

    std::unique_ptr<JobRepository> jobRepository_;
    std::unique_ptr<RecordingRepository> recordingRepository_;
    std::unique_ptr<MetadataRepository> metadataRepository_;

    std::unique_ptr<JobDashboardService> jobDashboardService_;
    std::unique_ptr<RecordingDashboardService> recordingDashboardService_;
    std::unique_ptr<DashboardFacade> dashboardFacade_;

    static std::atomic<bool> shutdownRequested_;
};
