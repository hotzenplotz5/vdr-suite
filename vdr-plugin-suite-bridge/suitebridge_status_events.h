#ifndef VDR_SUITE_BRIDGE_STATUS_EVENTS_H
#define VDR_SUITE_BRIDGE_STATUS_EVENTS_H

#include "suitebridge_counter_continuity.h"
#include "suitebridge_status_snapshot.h"

#include <array>
#include <cstddef>

enum class SuiteBridgeStatusEventKind : std::size_t {
  ChannelSwitch,
  Recording,
  Replaying,
  TimerChange,
  Count,
};

class SuiteBridgeStatusEvents final {
public:
  SuiteBridgeStatusEvents() noexcept;

  unsigned long long Record(SuiteBridgeStatusEventKind kind) noexcept;
  unsigned long long Count(SuiteBridgeStatusEventKind kind) const noexcept;
  bool CounterOverflowed() const noexcept;
  const char *CounterEpoch() const noexcept;
  SuiteBridgeStatusSnapshot CaptureSnapshot(bool monitorActive) const noexcept;

  static const char *Name(SuiteBridgeStatusEventKind kind) noexcept;

private:
  std::array<
      SuiteBridgeSaturatingCounter,
      static_cast<std::size_t>(SuiteBridgeStatusEventKind::Count)> counts_;
  const SuiteBridgeCounterEpoch epoch_;
};

#endif
