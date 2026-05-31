#pragma once

#include "RuntimeConfig.h"
#include "Database.h"

#include <atomic>

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

    static std::atomic<bool> shutdownRequested_;
};
