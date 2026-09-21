#pragma once

#include "DashboardController.h"

#include <string>

class EpgSearchNativeFuzzyOperatorRefreshService;

class EpgSearchNativeFuzzyOperatorRefreshController
{
public:
    explicit EpgSearchNativeFuzzyOperatorRefreshController(
        EpgSearchNativeFuzzyOperatorRefreshService& service);

    ApiResponse refreshBody(
        const std::string& body);

private:
    EpgSearchNativeFuzzyOperatorRefreshService& service_;
};
