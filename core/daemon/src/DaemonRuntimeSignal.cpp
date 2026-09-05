#include "DaemonRuntime.h"

#include <csignal>

void DaemonRuntime::handleSignal(int signalNumber)
{
    if (signalNumber == SIGINT || signalNumber == SIGTERM) {
        shutdownRequested_ = true;
    }
}
