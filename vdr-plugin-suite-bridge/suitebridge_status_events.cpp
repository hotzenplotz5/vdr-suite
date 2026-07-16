#include "suitebridge_status_events.h"

namespace {

constexpr std::size_t EventIndex(SuiteBridgeStatusEventKind kind) noexcept
{
  return static_cast<std::size_t>(kind);
}

}

SuiteBridgeStatusEvents::SuiteBridgeStatusEvents() noexcept = default;

unsigned long long SuiteBridgeStatusEvents::Record(
    SuiteBridgeStatusEventKind kind) noexcept
{
  const std::size_t index = EventIndex(kind);

  if (index >= counts_.size()) {
    return 0;
  }

  return counts_[index].Increment();
}

unsigned long long SuiteBridgeStatusEvents::Count(
    SuiteBridgeStatusEventKind kind) const noexcept
{
  const std::size_t index = EventIndex(kind);

  if (index >= counts_.size()) {
    return 0;
  }

  return counts_[index].Value();
}

bool SuiteBridgeStatusEvents::CounterOverflowed() const noexcept
{
  for (const auto &counter : counts_) {
    if (counter.Overflowed()) {
      return true;
    }
  }

  return false;
}

const char *SuiteBridgeStatusEvents::CounterEpoch() const noexcept
{
  return epoch_.Data();
}

SuiteBridgeStatusSnapshot SuiteBridgeStatusEvents::CaptureSnapshot(
    bool monitorActive) const noexcept
{
  return SuiteBridgeStatusSnapshot(
      monitorActive,
      Count(SuiteBridgeStatusEventKind::ChannelSwitch),
      Count(SuiteBridgeStatusEventKind::Recording),
      Count(SuiteBridgeStatusEventKind::Replaying),
      Count(SuiteBridgeStatusEventKind::TimerChange),
      CounterEpoch(),
      CounterOverflowed());
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
