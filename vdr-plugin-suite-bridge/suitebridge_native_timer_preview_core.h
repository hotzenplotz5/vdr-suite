#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_PREVIEW_CORE_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_PREVIEW_CORE_H

#include <cstdint>
#include <limits>
#include <vector>

// Pure, bounded decision kernel for a future read-only VDR-owned prospective
// Timer preview. This code neither acquires native VDR locks nor writes Timer
// state. Its inputs MUST eventually be collected from the VDR process under
// read locks; they cannot be sourced from a paired device or daemon guesses.
//
// Intentionally conservative: if any competing active Timer is recurrent,
// indeterminate, or overlaps the proposed airing, refuse to certify 'clear'
// even when sharing a tuner might be possible. No parallel VDR scheduler.
namespace vdrsuite_native_timer_preview {

// VDR schedules one-shot timers at minute precision: expand the evidence
// window outward. An unrounded EPG window could otherwise hide a conflict
// with a timer beginning or ending in the same native scheduling minute.
inline bool nativeMinuteExpandedWindow(std::int64_t start, std::int64_t stop,
                                       std::int64_t& roundedStart,
                                       std::int64_t& roundedStop)
{
  roundedStart = 0;
  roundedStop = 0;
  if (start <= 0 || stop <= start || stop - start >= 24 * 3600 ||
      stop > std::numeric_limits<std::int64_t>::max() - 60)
    return false;
  const std::int64_t begin = start - start % 60;
  const std::int64_t end = stop + (60 - stop % 60) % 60;
  if (begin <= 0 || end <= begin || end - begin >= 24 * 3600)
    return false;
  roundedStart = begin;
  roundedStop = end;
  return true;
}

enum class Disposition { unknown, candidateClear };

struct ExistingTimer {
  bool active = false;
  // Only a demonstrably non-repeating, bounded occurrence can pass.
  bool exactOneShot = false;
  std::int64_t start = 0;
  std::int64_t stop = 0;
};

struct Device {
  bool enumerated = false;
  bool supportsProposedChannel = false;
  // Must be supplied by a genuinely read-only VDR device query, not by an
  // inferred tuner count or reception assumptions.
  bool availableWithoutPreemptingRecording = false;
};

struct NativeSnapshot {
  std::int64_t proposedStart = 0;
  std::int64_t proposedStop = 0;
  bool proposedOneShot = false;
  bool proposedChannelVerified = false;
  bool proposedFreeToAir = false;
  bool allTimersEnumerated = false;
  bool allDevicesEnumerated = false;
  std::vector<ExistingTimer> existingTimers;
  std::vector<Device> devices;
};

struct Decision {
  Disposition disposition = Disposition::unknown;
  const char* reason = "evidence_unavailable";
  std::uint32_t consideredTimers = 0;
  std::uint32_t consideredDevices = 0;
};

Decision evaluate(const NativeSnapshot& snapshot);

} // namespace vdrsuite_native_timer_preview

#endif
