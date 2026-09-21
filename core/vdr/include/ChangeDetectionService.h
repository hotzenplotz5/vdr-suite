#ifndef CHANGE_DETECTION_SERVICE_H
#define CHANGE_DETECTION_SERVICE_H

#include "VdrChangeEvent.h"
#include "VdrChangeState.h"

#include <vector>

class ChangeDetectionService {
public:
    std::vector<VdrChangeEvent> detectChanges(
        const VdrChangeState& previous,
        const VdrChangeState& current) const;
};

#endif
