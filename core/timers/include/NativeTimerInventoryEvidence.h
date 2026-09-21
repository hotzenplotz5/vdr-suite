#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::timers
{

enum class NativeTimerInventoryCompleteness
{
    unknown,
    complete
};

// Backend-neutral proof that one complete native Timer inventory was observed
// successfully for one exact backend generation. A legacy VdrSnapshot or a
// plain getTimers() vector is not sufficient to mint this evidence because
// transport/parser failure may be indistinguishable from an empty vector.
struct NativeTimerInventoryEvidence
{
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::int64_t observedAt = 0;
    NativeTimerInventoryCompleteness completeness =
        NativeTimerInventoryCompleteness::unknown;
    std::vector<std::string> backendNativeTimerIds;
};

struct NativeTimerAbsenceAssessmentRequest
{
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;
    std::int64_t notBefore = 0;
};

enum class NativeTimerAbsenceAssessmentStatus
{
    absent,
    present,
    backendConflict,
    generationConflict,
    staleEvidence,
    invalid
};

bool nativeTimerInventoryEvidenceValid(
    const NativeTimerInventoryEvidence& evidence);
bool nativeTimerAbsenceAssessmentRequestValid(
    const NativeTimerAbsenceAssessmentRequest& request);
NativeTimerAbsenceAssessmentStatus assessNativeTimerAbsence(
    const NativeTimerInventoryEvidence& evidence,
    const NativeTimerAbsenceAssessmentRequest& request);

}
