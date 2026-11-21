#include "RectoolsAdapter.h"

bool RectoolsAdapter::execute(const Job& job)
{
    // Phase 2:
    // Placeholder only.
    //
    // Rectools remains an external project.
    // Real execution will be implemented later via adapter calls.

    return job.recordingId > 0 && !job.jobType.empty();
}
