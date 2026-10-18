#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_PREVIEW_CORE_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_PREVIEW_CORE_H

#include <cstdint>
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
