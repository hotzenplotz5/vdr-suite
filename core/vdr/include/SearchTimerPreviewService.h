#pragma once

#include "SearchTimer.h"
#include "SearchTimerPreviewResult.h"
#include "VdrEvent.h"

#include <vector>

class SearchTimerPreviewService {
public:
    SearchTimerPreviewResult preview(
        const SearchTimer& searchTimer,
        const std::vector<VdrEvent>& events,
        int limit = 0,
        int offset = 0) const;
};
