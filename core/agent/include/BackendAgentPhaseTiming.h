#pragma once

#include <chrono>
#include <iostream>
#include <ostream>

// Diagnose only: no request identifiers, credentials or payloads in logs.
inline void reportSlowBackendAgentPhase(
    const char* phase,
    std::chrono::steady_clock::time_point started,
    std::ostream& output = std::cerr)
{
    const auto elapsed = std::chrono::steady_clock::now() - started;
    if (elapsed < std::chrono::seconds(1)) return;
    output << "BACKEND_AGENT_PHASE_SLOW phase=" << phase
           << " duration_ms="
           << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
           << std::endl;
}
