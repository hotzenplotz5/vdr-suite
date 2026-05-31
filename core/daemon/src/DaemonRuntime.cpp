#include "DaemonRuntime.h"

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

    dashboardFacade_.reset();
    recordingDashboardService_.reset();
    jobDashboardService_.reset();

    metadataRepository_.reset();
    recordingRepository_.reset();
    jobRepository_.reset();

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
