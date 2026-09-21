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

  const SuiteBridgeStatusSnapshot snapshot = CaptureSnapshot();

  isyslog(
      "suitebridge: status-monitor state=active counter-epoch=%s counter-overflow=%s",
      snapshot.CounterEpoch(),
      snapshot.CounterOverflow() ? "true" : "false");
  LogSnapshot(snapshot);
}

void SuiteBridgeStatusMonitor::Deactivate() noexcept
{
  if (!active_.exchange(false, std::memory_order_acq_rel)) {
    return;
  }

  const SuiteBridgeStatusSnapshot snapshot = events_.CaptureSnapshot(false);

  isyslog(
      "suitebridge: status-monitor state=inactive channel-switch=%llu recording=%llu replaying=%llu timer-change=%llu counter-epoch=%s counter-overflow=%s",
      snapshot.ChannelSwitchCount(),
      snapshot.RecordingCount(),
      snapshot.ReplayingCount(),
      snapshot.TimerChangeCount(),
      snapshot.CounterEpoch(),
      snapshot.CounterOverflow() ? "true" : "false");

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

SuiteBridgeOsdSnapshot SuiteBridgeStatusMonitor::CaptureOsdSnapshot() const noexcept
{
  return osdState_.CaptureSnapshot();
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
      "suitebridge: status-snapshot schema=%u active=%s total=%llu channel-switch=%llu recording=%llu replaying=%llu timer-change=%llu counter-epoch=%s counter-overflow=%s",
      SuiteBridgeStatusSnapshot::SchemaVersion(),
      snapshot.MonitorActive() ? "true" : "false",
      snapshot.TotalCount(),
      snapshot.ChannelSwitchCount(),
      snapshot.RecordingCount(),
      snapshot.ReplayingCount(),
      snapshot.TimerChangeCount(),
      snapshot.CounterEpoch(),
      snapshot.CounterOverflow() ? "true" : "false");

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

void SuiteBridgeStatusMonitor::MarksModified(
    const cMarks *marks)
{
  (void)marks;

  RecordEvent(SuiteBridgeStatusEventKind::MarksModified);
}

void SuiteBridgeStatusMonitor::OsdClear(void)
{
  osdState_.Clear();
}

void SuiteBridgeStatusMonitor::OsdTitle(const char *title)
{
  osdState_.Title(title);
}

void SuiteBridgeStatusMonitor::OsdStatusMessage(
    eMessageType type,
    const char *message)
{
  (void)type;
  osdState_.StatusMessage(message);
}

void SuiteBridgeStatusMonitor::OsdHelpKeys(
    const char *red,
    const char *green,
    const char *yellow,
    const char *blue)
{
  osdState_.HelpKeys(red, green, yellow, blue);
}

void SuiteBridgeStatusMonitor::OsdItem(
    const char *text,
    int index,
    bool selectable)
{
  osdState_.Item(text, index, selectable);
}

void SuiteBridgeStatusMonitor::OsdCurrentItem(
    const char *text,
    int index)
{
  osdState_.CurrentItem(text, index);
}

void SuiteBridgeStatusMonitor::OsdTextItem(
    const char *text,
    bool scroll)
{
  osdState_.TextItem(text, scroll);
}

void SuiteBridgeStatusMonitor::OsdChannel(const char *text)
{
  osdState_.Channel(text);
}

void SuiteBridgeStatusMonitor::OsdProgramme(
    time_t presentTime,
    const char *presentTitle,
    const char *presentSubtitle,
    time_t followingTime,
    const char *followingTitle,
    const char *followingSubtitle)
{
  osdState_.Programme(
      static_cast<std::int64_t>(presentTime),
      presentTitle,
      presentSubtitle,
      static_cast<std::int64_t>(followingTime),
      followingTitle,
      followingSubtitle);
}
