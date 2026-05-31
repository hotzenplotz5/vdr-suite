#pragma once

#include "RuntimeConfig.h"

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

    static std::atomic<bool> shutdownRequested_;
};
