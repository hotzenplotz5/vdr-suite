#pragma once

#include "SearchTimerPreviewEpgCache.h"

#include <string>
#include <vector>

struct SearchTimerPreviewEpgInputContextState
{
    std::string status = "ready";
    bool available = true;
    std::vector<std::string> warnings;
};

class SearchTimerPreviewEpgInputContext
{
public:
    static SearchTimerPreviewEpgInputContextState current()
    {
        return state();
    }

    static void resetReady()
    {
        state() = SearchTimerPreviewEpgInputContextState{};
    }

    static void setCacheStatus(
        SearchTimerPreviewEpgCacheStatus status,
        const std::string& backendId)
    {
        SearchTimerPreviewEpgInputContextState next;
        next.status = statusToString(status);
        next.available = status == SearchTimerPreviewEpgCacheStatus::Ready;

        if (!next.available)
        {
            next.warnings.push_back(
                "SearchTimer preview EPG input is " + next.status +
                " for backend " + normalizedBackendId(backendId) +
                "; match counts are not authoritative until EPG input is ready.");
        }

        state() = next;
    }

private:
    static SearchTimerPreviewEpgInputContextState& state()
    {
        static thread_local SearchTimerPreviewEpgInputContextState currentState;
        return currentState;
    }

    static std::string normalizedBackendId(
        const std::string& backendId)
    {
        return backendId.empty()
            ? "default"
            : backendId;
    }

    static std::string statusToString(
        SearchTimerPreviewEpgCacheStatus status)
    {
        switch (status)
        {
        case SearchTimerPreviewEpgCacheStatus::Unknown:
            return "unknown";
        case SearchTimerPreviewEpgCacheStatus::Warming:
            return "warming";
        case SearchTimerPreviewEpgCacheStatus::Ready:
            return "ready";
        case SearchTimerPreviewEpgCacheStatus::Stale:
            return "stale";
        case SearchTimerPreviewEpgCacheStatus::Unavailable:
            return "unavailable";
        }

        return "unknown";
    }
};
