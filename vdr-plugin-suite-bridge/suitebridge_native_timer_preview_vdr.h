#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_PREVIEW_VDR_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_PREVIEW_VDR_H

#include "suitebridge_native_timer_preview_core.h"

#include <cstdint>
#include <string>

// Source-only VDR producer of a prospective read-only Timer snapshot.
// This is NOT an SVDRP command, capability, or admission proof. Before
// activation, its actual VDR 2.8.3 build/lock behavior requires acceptance.
struct SuiteBridgeNativeTimerPreviewVdrRequest {
  std::string channelId;
  std::string day;
  std::string weekdays;
  std::string startTime;
  std::string endTime;
  std::int64_t unixStart = 0;
  std::int64_t unixStop = 0;
  int priority = 50;
  bool enabled = true;
  bool vps = false;
};

vdrsuite_native_timer_preview::NativeSnapshot
SuiteBridgeCaptureNativeTimerPreviewVdr(
    const SuiteBridgeNativeTimerPreviewVdrRequest& request);

#endif
