#include "suitebridge_status_monitor.h"

#include "suitebridge_capabilities.h"
#include "suitebridge_local_contract.h"

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

void SuiteBridgeStatusMonitor::RecordEvent(
    SuiteBridgeStatusEventKind kind) noexcept
{
  if (!IsActive()) {
    return;
  }

  (void)events_.Record(kind);
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

  const SuiteBridgeLocalContractPayload payload(
      SuiteBridgeCapabilities::SchemaVersion(),
      snapshot);

  if (!payload.Complete()) {
    esyslog(
        "suitebridge: local-contract-payload schema=%u result=truncated bytes=%zu",
        SuiteBridgeLocalContractPayload::SchemaVersion(),
        payload.Size());
    return;
  }

  isyslog(
      "suitebridge: local-contract-payload schema=%u result=prepared bytes=%zu payload=%s",
      SuiteBridgeLocalContractPayload::SchemaVersion(),
      payload.Size(),
      payload.Data());
}

void SuiteBridgeStatusMonitor::ChannelSwitch(
    const cDevice *device,
    int channelNumber,
    bool liveView)
{
  (void)device;
  (void)channelNumber;
  (void)liveView;

  RecordEvent(SuiteBridgeStatusEventKind::ChannelSwitch);
}

void SuiteBridgeStatusMonitor::Recording(
    const cDevice *device,
    const char *name,
    const char *fileName,
    bool on)
{
  (void)device;
  (void)name;
  (void)fileName;
  (void)on;

  RecordEvent(SuiteBridgeStatusEventKind::Recording);
}

void SuiteBridgeStatusMonitor::Replaying(
    const cControl *control,
    const char *name,
    const char *fileName,
    bool on)
{
  (void)control;
  (void)name;
  (void)fileName;
  (void)on;

  RecordEvent(SuiteBridgeStatusEventKind::Replaying);
}

void SuiteBridgeStatusMonitor::TimerChange(
    const cTimer *timer,
    eTimerChange change)
{
  (void)timer;
  (void)change;

  RecordEvent(SuiteBridgeStatusEventKind::TimerChange);
}
