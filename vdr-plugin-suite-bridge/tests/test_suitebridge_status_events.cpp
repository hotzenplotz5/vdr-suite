#include "suitebridge_status_events.h"

#include <array>
#include <cassert>
#include <cstring>

int main()
{
  SuiteBridgeStatusEvents events;

  const std::array<SuiteBridgeStatusEventKind, 4> kinds = {{
      SuiteBridgeStatusEventKind::ChannelSwitch,
      SuiteBridgeStatusEventKind::Recording,
      SuiteBridgeStatusEventKind::Replaying,
      SuiteBridgeStatusEventKind::TimerChange,
  }};

  const std::array<const char *, 4> names = {{
      "channel-switch",
      "recording",
      "replaying",
      "timer-change",
  }};

  for (std::size_t index = 0; index < kinds.size(); ++index) {
    assert(events.Count(kinds[index]) == 0);
    assert(std::strcmp(
        SuiteBridgeStatusEvents::Name(kinds[index]),
        names[index]) == 0);
    assert(events.Record(kinds[index]) == 1);
    assert(events.Record(kinds[index]) == 2);
    assert(events.Count(kinds[index]) == 2);
  }

  assert(events.Record(SuiteBridgeStatusEventKind::Count) == 0);
  assert(events.Count(SuiteBridgeStatusEventKind::Count) == 0);
  assert(std::strcmp(
      SuiteBridgeStatusEvents::Name(SuiteBridgeStatusEventKind::Count),
      "unknown") == 0);

  return 0;
}
