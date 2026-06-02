#include "DaemonRuntime.h"

#include "VdrAdapterFactory.h"

#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

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
    vdrConfig_.mode = "mock";
    vdrConfig_.host = "mock";
    vdrConfig_.port = 0;

    vdrAdapter_ =
        VdrAdapterFactory::create(vdrConfig_);

    vdrService_ =
        std::make_unique<VdrService>(
            *vdrAdapter_);

    vdrOverviewService_ =
        std::make_unique<VdrOverviewService>(
            *vdrService_);

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
        << "vdr-suite-daemon waiting for shutdown signal"
        << std::endl;

    while (!shutdownRequested_)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(250));
    }

    std::cout
        << "vdr-suite-daemon received shutdown signal"
        << std::endl;

    return 0;
}

void DaemonRuntime::shutdown()
{
    if (!initialized_)
    {
        return;
    }

    apiRouter_.reset();

    vdrController_.reset();
    vdrOverviewJsonSerializer_.reset();
    vdrOverviewService_.reset();
    vdrService_.reset();
    vdrAdapter_.reset();

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
