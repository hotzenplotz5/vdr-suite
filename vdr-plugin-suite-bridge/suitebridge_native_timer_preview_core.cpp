#include "suitebridge_native_timer_preview_core.h"

namespace vdrsuite_native_timer_preview {
namespace {
constexpr std::size_t MaximumTimers = 4096;
constexpr std::size_t MaximumDevices = 64;
constexpr std::int64_t MaximumCandidateWindowSeconds = 24 * 3600;
}

Decision evaluate(const NativeSnapshot& s)
{
  Decision result;
  if (!s.proposedOneShot || !s.proposedChannelVerified ||
      !s.proposedFreeToAir || s.proposedStart <= 0 ||
      s.proposedStop <= s.proposedStart ||
      s.proposedStop - s.proposedStart >= MaximumCandidateWindowSeconds) {
    result.reason = "candidate_not_safely_projectable";
    return result;
  }
  if (!s.allTimersEnumerated || !s.allDevicesEnumerated ||
      s.existingTimers.size() > MaximumTimers || s.devices.empty() ||
      s.devices.size() > MaximumDevices) {
    result.reason = "native_snapshot_incomplete";
    return result;
  }
  // Include *every* configured Timer in the scan, including disabled ones.
  // Discard an inactive Timer only after the VDR producer verified its state.
  for (const ExistingTimer& timer : s.existingTimers) {
    ++result.consideredTimers;
    if (!timer.active) continue;
    if (!timer.exactOneShot || timer.start <= 0 ||
        timer.stop <= timer.start) {
      result.reason = "active_timer_schedule_indeterminate";
      return result;
    }
    if (timer.start < s.proposedStop &&
        s.proposedStart < timer.stop) {
      // Sharing one transponder might work; we deliberately do not infer
      // that without a validated native future device-allocation simulation.
      result.reason = "active_timer_overlap_not_simulated";
      return result;
    }
  }
  bool eligible = false;
  for (const Device& d : s.devices) {
    ++result.consideredDevices;
    if (!d.enumerated) {
      result.reason = "device_inventory_incomplete";
      return result;
    }
    if (d.supportsProposedChannel &&
        d.availableWithoutPreemptingRecording)
      eligible = true;
  }
  if (!eligible) {
    result.reason = "no_proven_nonpreempting_native_device";
    return result;
  }
  result.disposition = Disposition::candidateClear;
  result.reason = "isolated_one_shot_native_device_candidate_clear";
  return result;
}

} // namespace vdrsuite_native_timer_preview
