#include "DaemonRuntime.h"

#include "BasicHttpClient.h"
#include "RestfulApiVdrAdapter.h"
#include "SimpleHttpListener.h"
#include "TestHttpServer.h"

#include <csignal>
#include <iostream>

std::atomic<bool> DaemonRuntime::shutdownRequested_(false);

DaemonRuntime::DaemonRuntime()
    : initialized_(false)
{
}

bool DaemonRuntime::initialize()
{
    std::cout
        << "vdr-suite-daemon runtime initializing"
        << std::endl;

    std::cout
        << "database path: "
        << config_.databasePath()
        << std::endl;

    if (!database_.open(config_.databasePath()))
    {
        std::cerr
            << "failed to open database"
            << std::endl;

        return false;
    }

    std::cout
        << "database opened"
        << std::endl;

    jobRepository_ =
        std::make_unique<JobRepository>(database_);

    recordingRepository_ =
        std::make_unique<RecordingRepository>(database_);

    metadataRepository_ =
        std::make_unique<MetadataRepository>(database_);

    jobDashboardService_ =
        std::make_unique<JobDashboardService>(
            *jobRepository_);

    recordingDashboardService_ =
        std::make_unique<RecordingDashboardService>(
            *recordingRepository_,
            *metadataRepository_);

    dashboardFacade_ =
        std::make_unique<DashboardFacade>(
            *jobDashboardService_,
            *recordingDashboardService_);

    dashboardJsonSerializer_ =
        std::make_unique<DashboardJsonSerializer>();

    dashboardController_ =
        std::make_unique<DashboardController>(
            *dashboardFacade_,
            *dashboardJsonSerializer_);

    jobsController_ =
        std::make_unique<JobsController>(
            *jobRepository_);

    recordingsController_ =
        std::make_unique<RecordingsController>(
            *recordingRepository_);

    metadataController_ =
        std::make_unique<MetadataController>(
            *metadataRepository_);

    std::cout
        << "REST controller runtime initialized"
        << std::endl;

    vdrConfig_.enabled = true;
    vdrConfig_.mode = "restfulapi";
    vdrConfig_.host = "127.0.0.1";
    vdrConfig_.port = 8002;

    vdrHttpClient_ =
        std::make_unique<BasicHttpClient>(
            vdrConfig_.host,
            vdrConfig_.port,
            &runtimeLogger_,
            &runtimeDiagnosticsService_);

    vdrAdapter_ =
        std::make_unique<RestfulApiVdrAdapter>(
            vdrConfig_,
            *vdrHttpClient_);

    vdrService_ =
        std::make_unique<VdrService>(
            *vdrAdapter_,
            &runtimeLogger_);

    vdrSnapshotBuilder_ =
        std::make_unique<VdrSnapshotBuilder>(
            *vdrService_,
            &runtimeLogger_,
            &runtimeDiagnosticsService_);

    snapshotCache_ =
        std::make_unique<SnapshotCache>();

    snapshotCacheService_ =
        std::make_unique<SnapshotCacheService>(
            *snapshotCache_);

    snapshotAccessService_ =
        std::make_unique<SnapshotAccessService>(
            *snapshotCacheService_);

    pollingService_ =
        std::make_unique<PollingService>(
            *vdrSnapshotBuilder_,
            *vdrService_,
            *snapshotCacheService_,
            &runtimeLogger_,
            &runtimeDiagnosticsService_);

    pollingService_->poll();

    std::cout
        << "VDR snapshot runtime initialized"
        << std::endl;

    vdrOverviewService_ =
        std::make_unique<VdrOverviewService>(
            *snapshotAccessService_);

    vdrOverviewJsonSerializer_ =
        std::make_unique<VdrOverviewJsonSerializer>();

    vdrController_ =
        std::make_unique<VdrController>(
            *vdrOverviewService_,
            *vdrOverviewJsonSerializer_);

    std::cout
        << "VDR controller runtime initialized"
        << std::endl;

    apiRouter_ =
        std::make_unique<ApiRouter>(
            *dashboardController_,
            *jobsController_,
            *recordingsController_,
            *metadataController_,
            *vdrController_);

    std::cout
        << "API router runtime initialized"
        << std::endl;

    httpServer_ =
        std::make_unique<TestHttpServer>(
            *apiRouter_);

    httpListener_ =
        std::make_unique<SimpleHttpListener>(
            "127.0.0.1",
            18080,
            *httpServer_);

    std::cout
        << "HTTP listener runtime initialized"
        << std::endl;

    std::cout
        << "dashboard runtime initialized"
        << std::endl;

    std::signal(SIGINT, DaemonRuntime::handleSignal);
    std::signal(SIGTERM, DaemonRuntime::handleSignal);

    shutdownRequested_ = false;
    initialized_ = true;

    return true;
}

int DaemonRuntime::run()
{
    if (!initialized_)
    {
        std::cerr
            << "vdr-suite-daemon runtime not initialized"
            << std::endl;

        return 1;
    }

    std::cout
        << "vdr-suite-daemon runtime running"
        << std::endl;

    std::cout
        << "vdr-suite-daemon serving HTTP on 127.0.0.1:18080"
        << std::endl;

    return httpListener_->runUntilStopped();
}

void DaemonRuntime::shutdown()
{
    if (!initialized_)
    {
        return;
    }

    httpListener_.reset();
    httpServer_.reset();
    apiRouter_.reset();

    std::cout
        << "HTTP server runtime stopped"
        << std::endl;

    vdrController_.reset();
    vdrOverviewJsonSerializer_.reset();
    vdrOverviewService_.reset();
    pollingService_.reset();
    snapshotAccessService_.reset();
    snapshotCacheService_.reset();
    snapshotCache_.reset();
    vdrSnapshotBuilder_.reset();
    vdrService_.reset();
    vdrAdapter_.reset();
    vdrHttpClient_.reset();

    metadataController_.reset();
    recordingsController_.reset();
    jobsController_.reset();
    dashboardController_.reset();
    dashboardJsonSerializer_.reset();

    dashboardFacade_.reset();
    recordingDashboardService_.reset();
    jobDashboardService_.reset();

    metadataRepository_.reset();
    recordingRepository_.reset();
    jobRepository_.reset();

    std::cout
        << "API router runtime stopped"
        << std::endl;

    std::cout
        << "REST controller runtime stopped"
        << std::endl;

    std::cout
        << "dashboard runtime stopped"
        << std::endl;

    database_.close();

    std::cout
        << "database closed"
        << std::endl;

    std::cout
        << "vdr-suite-daemon runtime shutting down"
        << std::endl;

    initialized_ = false;
}

void DaemonRuntime::handleSignal(int signalNumber)
{
    if (signalNumber == SIGINT || signalNumber == SIGTERM)
    {
        shutdownRequested_ = true;
    }
}
