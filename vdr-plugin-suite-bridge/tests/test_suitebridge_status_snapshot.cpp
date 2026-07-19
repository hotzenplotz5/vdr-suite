#include "suitebridge_status_events.h"
#include "suitebridge_status_snapshot.h"

#include <cassert>
#include <cstring>
#include <limits>
#include <type_traits>

int main()
{
  static_assert(!std::is_copy_assignable<SuiteBridgeStatusSnapshot>::value);
  static_assert(SuiteBridgeStatusSnapshot::SchemaVersion() == 2);
  static_assert(SuiteBridgeStatusSnapshot::CounterEpochLength() == 32);

  SuiteBridgeStatusEvents events;

  const SuiteBridgeStatusSnapshot initial = events.CaptureSnapshot(false);
  assert(!initial.MonitorActive());
  assert(initial.TotalCount() == 0);
  assert(std::strlen(initial.CounterEpoch()) == 32);
  assert(!initial.CounterOverflow());

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
  assert(std::strcmp(active.CounterEpoch(), initial.CounterEpoch()) == 0);
  assert(!active.CounterOverflow());

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
  assert(std::strcmp(later.CounterEpoch(), initial.CounterEpoch()) == 0);
  assert(!later.CounterOverflow());

  constexpr unsigned long long maximum =
      std::numeric_limits<unsigned long long>::max();
  const char *epoch = "0123456789abcdef0123456789abcdef";
  const SuiteBridgeStatusSnapshot saturatedTotal(
      true,
      maximum,
      1,
      0,
      0,
      epoch,
      false);

  assert(saturatedTotal.TotalCount() == maximum);
  assert(saturatedTotal.CounterOverflow());
  assert(std::strcmp(saturatedTotal.CounterEpoch(), epoch) == 0);

  const SuiteBridgeStatusSnapshot explicitOverflow(
      false,
      1,
      2,
      3,
      4,
      epoch,
      true);

  assert(explicitOverflow.TotalCount() == 10);
  assert(explicitOverflow.CounterOverflow());

  return 0;
}
