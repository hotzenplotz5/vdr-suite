#pragma once

#include "EpgSearchNativeFuzzyStartupRestoreDiagnostics.h"

#include <string>

class EpgSearchNativeFuzzyStartupRestoreDiagnosticsJsonSerializer
{
public:
    std::string serialize(
        const EpgSearchNativeFuzzyStartupRestoreDiagnostics& diagnostics) const;
};
