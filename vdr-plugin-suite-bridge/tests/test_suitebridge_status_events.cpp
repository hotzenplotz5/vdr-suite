#include "suitebridge_status_events.h"

#include <array>
#include <cassert>
#include <cctype>
#include <cstring>

namespace {

bool IsLowerHexEpoch(const char *value)
{
  if (value == nullptr || std::strlen(value) != 32) {
    return false;
  }

  for (std::size_t index = 0; index < 32; ++index) {
    const unsigned char character =
        static_cast<unsigned char>(value[index]);

    if (!std::isdigit(character) &&
        !(character >= 'a' && character <= 'f')) {
      return false;
    }
  }

  return true;
}

}

int main()
{
  SuiteBridgeStatusEvents events;
  SuiteBridgeStatusEvents otherEvents;

  assert(IsLowerHexEpoch(events.CounterEpoch()));
  assert(IsLowerHexEpoch(otherEvents.CounterEpoch()));
  assert(std::strcmp(events.CounterEpoch(), otherEvents.CounterEpoch()) != 0);
  assert(!events.CounterOverflowed());

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

  assert(!events.CounterOverflowed());
  assert(events.Record(SuiteBridgeStatusEventKind::Count) == 0);
  assert(events.Count(SuiteBridgeStatusEventKind::Count) == 0);
  assert(std::strcmp(
      SuiteBridgeStatusEvents::Name(SuiteBridgeStatusEventKind::Count),
      "unknown") == 0);

  return 0;
}
