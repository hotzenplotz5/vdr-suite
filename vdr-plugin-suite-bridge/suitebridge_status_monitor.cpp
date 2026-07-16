#include "suitebridge_status_monitor.h"

#include <vdr/tools.h>

SuiteBridgeStatusMonitor::SuiteBridgeStatusMonitor() noexcept
    : active_(false)
{
}

void SuiteBridgeStatusMonitor::Activate() noexcept
{
  active_.store(true, std::memory_order_release);
  isyslog("suitebridge: status-monitor state=active");
  LogSnapshot(CaptureSnapshot());
}

void SuiteBridgeStatusMonitor::Deactivate() noexcept
{
  if (!active_.exchange(false, std::memory_order_acq_rel)) {
    return;
  }

  const SuiteBridgeStatusSnapshot snapshot = events_.CaptureSnapshot(false);

  isyslog(
      "suitebridge: status-monitor state=inactive channel-switch=%llu recording=%llu replaying=%llu timer-change=%llu",
      snapshot.ChannelSwitchCount(),
      snapshot.RecordingCount(),
      snapshot.ReplayingCount(),
      snapshot.TimerChangeCount());

  LogSnapshot(snapshot);
}

bool SuiteBridgeStatusMonitor::IsActive() const noexcept
{
  return active_.load(std::memory_order_acquire);
}

unsigned long long SuiteBridgeStatusMonitor::EventCount(
    SuiteBridgeStatusEventKind kind) const noexcept
{
  return events_.Count(kind);
}

SuiteBridgeStatusSnapshot SuiteBridgeStatusMonitor::CaptureSnapshot() const noexcept
{
  return events_.CaptureSnapshot(IsActive());
}

void SuiteBridgeStatusMonitor::LogSnapshot(
    const SuiteBridgeStatusSnapshot &snapshot) const noexcept
{
  isyslog(
      "suitebridge: status-snapshot schema=%u active=%s total=%llu channel-switch=%llu recording=%llu replaying=%llu timer-change=%llu",
      SuiteBridgeStatusSnapshot::SchemaVersion(),
      snapshot.MonitorActive() ? "true" : "false",
      snapshot.TotalCount(),
      snapshot.ChannelSwitchCount(),
      snapshot.RecordingCount(),
      snapshot.ReplayingCount(),
      snapshot.TimerChangeCount());
}

void SuiteBridgeStatusMonitor::ChannelSwitch(
    const cDevice *device,
    int channelNumber,
    bool liveView)
{
  if (!IsActive()) {
    return;
  }

  const unsigned long long sequence =
      events_.Record(SuiteBridgeStatusEventKind::ChannelSwitch);

  isyslog(
      "suitebridge: status-event type=channel-switch sequence=%llu channel=%d live=%s device=%s",
      sequence,
      channelNumber,
      liveView ? "true" : "false",
      device != nullptr ? "present" : "none");
}

void SuiteBridgeStatusMonitor::Recording(
    const cDevice *device,
    const char *name,
    const char *fileName,
    bool on)
{
  (void)name;
  (void)fileName;

  if (!IsActive()) {
    return;
  }

  const unsigned long long sequence =
      events_.Record(SuiteBridgeStatusEventKind::Recording);

  isyslog(
      "suitebridge: status-event type=recording sequence=%llu on=%s device=%s",
      sequence,
      on ? "true" : "false",
      device != nullptr ? "present" : "none");
}

void SuiteBridgeStatusMonitor::Replaying(
    const cControl *control,
    const char *name,
    const char *fileName,
    bool on)
{
  (void)name;
  (void)fileName;

  if (!IsActive()) {
    return;
  }

  const unsigned long long sequence =
      events_.Record(SuiteBridgeStatusEventKind::Replaying);

  isyslog(
      "suitebridge: status-event type=replaying sequence=%llu on=%s control=%s",
      sequence,
      on ? "true" : "false",
      control != nullptr ? "present" : "none");
}

void SuiteBridgeStatusMonitor::TimerChange(
    const cTimer *timer,
    eTimerChange change)
{
  if (!IsActive()) {
    return;
  }

  const unsigned long long sequence =
      events_.Record(SuiteBridgeStatusEventKind::TimerChange);

  isyslog(
      "suitebridge: status-event type=timer-change sequence=%llu change=%d timer=%s",
      sequence,
      static_cast<int>(change),
      timer != nullptr ? "present" : "none");
}
