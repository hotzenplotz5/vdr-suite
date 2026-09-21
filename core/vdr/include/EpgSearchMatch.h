#pragma once

#include "VdrEvent.h"

#include <string>
#include <utility>
#include <vector>

class EpgSearchMatch
{
public:
    EpgSearchMatch(
        VdrEvent event,
        std::string backendId,
        std::vector<std::string> matchedFields)
        : event_(std::move(event)),
          backendId_(std::move(backendId)),
          matchedFields_(std::move(matchedFields))
    {
    }

    static EpgSearchMatch fromEvent(
        VdrEvent event)
    {
        return EpgSearchMatch(
            std::move(event),
            "",
            {});
    }

    const VdrEvent& event() const
    {
        return event_;
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    bool hasBackendId() const
    {
        return !backendId_.empty();
    }

    const std::vector<std::string>& matchedFields() const
    {
        return matchedFields_;
    }

    bool hasMatchedFields() const
    {
        return !matchedFields_.empty();
    }

private:
    VdrEvent event_;
    std::string backendId_;
    std::vector<std::string> matchedFields_;
};
