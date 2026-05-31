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
