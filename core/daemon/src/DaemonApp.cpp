#include "DaemonApp.h"

#include <iostream>

DaemonApp::DaemonApp()
{
}

int DaemonApp::run()
{
    std::cout
        << "vdr-suite-daemon starting"
        << std::endl;

    if (!runtime_.initialize())
    {
        std::cerr
            << "failed to initialize daemon runtime"
            << std::endl;

        return 1;
    }

    int result =
        runtime_.run();

    runtime_.shutdown();

    return result;
}
