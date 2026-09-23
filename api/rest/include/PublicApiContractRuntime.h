#pragma once

#include "DashboardController.h"

#include <string>

class PublicApiContractRuntime
{
public:
    static PublicApiContractRuntime& instance();

    bool tryHandleGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        ApiResponse& response) const;

private:
    PublicApiContractRuntime() = default;
};
