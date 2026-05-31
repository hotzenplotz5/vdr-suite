#pragma once

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

    static std::atomic<bool> shutdownRequested_;
};
