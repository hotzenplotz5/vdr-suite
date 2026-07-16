#include "suitebridge_status_events.h"

namespace {

constexpr std::size_t EventIndex(SuiteBridgeStatusEventKind kind) noexcept
{
  return static_cast<std::size_t>(kind);
}

}

SuiteBridgeStatusEvents::SuiteBridgeStatusEvents() noexcept
{
  for (auto &count : counts_) {
    count.store(0, std::memory_order_relaxed);
  }
}

unsigned long long SuiteBridgeStatusEvents::Record(
    SuiteBridgeStatusEventKind kind) noexcept
{
  const std::size_t index = EventIndex(kind);

  if (index >= counts_.size()) {
    return 0;
  }

  return counts_[index].fetch_add(1, std::memory_order_relaxed) + 1;
}

unsigned long long SuiteBridgeStatusEvents::Count(
    SuiteBridgeStatusEventKind kind) const noexcept
{
  const std::size_t index = EventIndex(kind);

  if (index >= counts_.size()) {
    return 0;
  }

  return counts_[index].load(std::memory_order_relaxed);
}

SuiteBridgeStatusSnapshot SuiteBridgeStatusEvents::CaptureSnapshot(
    bool monitorActive) const noexcept
{
  return SuiteBridgeStatusSnapshot(
      monitorActive,
      Count(SuiteBridgeStatusEventKind::ChannelSwitch),
      Count(SuiteBridgeStatusEventKind::Recording),
      Count(SuiteBridgeStatusEventKind::Replaying),
      Count(SuiteBridgeStatusEventKind::TimerChange));
}

const char *SuiteBridgeStatusEvents::Name(
    SuiteBridgeStatusEventKind kind) noexcept
{
  switch (kind) {
    case SuiteBridgeStatusEventKind::ChannelSwitch:
      return "channel-switch";
    case SuiteBridgeStatusEventKind::Recording:
      return "recording";
    case SuiteBridgeStatusEventKind::Replaying:
      return "replaying";
    case SuiteBridgeStatusEventKind::TimerChange:
      return "timer-change";
    case SuiteBridgeStatusEventKind::Count:
      break;
  }

  return "unknown";
}
