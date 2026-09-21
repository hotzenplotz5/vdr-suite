#pragma once

#include "DashboardController.h"

#include <string>

class TeletextControlPlaneReadService;

class TeletextApiRuntime
{
public:
    static TeletextApiRuntime& instance();

    bool configure(TeletextControlPlaneReadService& readService);
    void reset();
    bool configured() const;

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

private:
    TeletextApiRuntime() = default;

    TeletextControlPlaneReadService* readService_ = nullptr;
};
