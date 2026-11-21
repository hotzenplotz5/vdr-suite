#pragma once

#include "DaemonRuntime.h"

class DaemonApp
{
public:
    DaemonApp();

    int run();

private:
    DaemonRuntime runtime_;
};
