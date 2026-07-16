#include "suitebridge_status_events.h"
#include "suitebridge_status_snapshot.h"

#include <cassert>
#include <type_traits>

int main()
{
  static_assert(!std::is_copy_assignable<SuiteBridgeStatusSnapshot>::value);

  SuiteBridgeStatusEvents events;

  const SuiteBridgeStatusSnapshot initial = events.CaptureSnapshot(false);
  assert(SuiteBridgeStatusSnapshot::SchemaVersion() == 1);
  assert(!initial.MonitorActive());
  assert(initial.TotalCount() == 0);

  events.Record(SuiteBridgeStatusEventKind::ChannelSwitch);
  events.Record(SuiteBridgeStatusEventKind::ChannelSwitch);
  events.Record(SuiteBridgeStatusEventKind::Recording);
  events.Record(SuiteBridgeStatusEventKind::Replaying);
  events.Record(SuiteBridgeStatusEventKind::TimerChange);

  const SuiteBridgeStatusSnapshot active = events.CaptureSnapshot(true);
  assert(active.MonitorActive());
  assert(active.ChannelSwitchCount() == 2);
  assert(active.RecordingCount() == 1);
  assert(active.ReplayingCount() == 1);
  assert(active.TimerChangeCount() == 1);
  assert(active.TotalCount() == 5);

  events.Record(SuiteBridgeStatusEventKind::ChannelSwitch);
  events.Record(SuiteBridgeStatusEventKind::TimerChange);

  assert(active.ChannelSwitchCount() == 2);
  assert(active.TimerChangeCount() == 1);
  assert(active.TotalCount() == 5);

  const SuiteBridgeStatusSnapshot later = events.CaptureSnapshot(false);
  assert(!later.MonitorActive());
  assert(later.ChannelSwitchCount() == 3);
  assert(later.RecordingCount() == 1);
  assert(later.ReplayingCount() == 1);
  assert(later.TimerChangeCount() == 2);
  assert(later.TotalCount() == 7);

  return 0;
}
