#pragma once

#include "DashboardController.h"

class EpgSearchNativeFuzzyStaleProbeAdministrationService;

class EpgSearchNativeFuzzyStaleProbeAdministrationController
{
public:
    explicit EpgSearchNativeFuzzyStaleProbeAdministrationController(
        EpgSearchNativeFuzzyStaleProbeAdministrationService& service);

    ApiResponse getStaleProbeResults();
    ApiResponse deleteStaleProbeResults();

private:
    EpgSearchNativeFuzzyStaleProbeAdministrationService& service_;
};
