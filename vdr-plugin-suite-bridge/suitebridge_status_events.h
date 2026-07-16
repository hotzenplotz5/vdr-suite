#ifndef VDR_SUITE_BRIDGE_STATUS_EVENTS_H
#define VDR_SUITE_BRIDGE_STATUS_EVENTS_H

#include <array>
#include <atomic>
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

  static const char *Name(SuiteBridgeStatusEventKind kind) noexcept;

private:
  std::array<
      std::atomic<unsigned long long>,
      static_cast<std::size_t>(SuiteBridgeStatusEventKind::Count)> counts_;
};

#endif
