#include "suitebridge_counter_continuity.h"

#include <cassert>
#include <cctype>
#include <cstring>
#include <limits>
#include <type_traits>

namespace {

bool IsLowerHex(const char *value)
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
  static_assert(SuiteBridgeCounterEpoch::HexLength() == 32);
  static_assert(SuiteBridgeCounterEpoch::Capacity() == 33);
  static_assert(
      std::is_copy_constructible<SuiteBridgeCounterEpoch>::value);
  static_assert(
      !std::is_copy_assignable<SuiteBridgeCounterEpoch>::value);

  const SuiteBridgeCounterEpoch firstEpoch;
  const SuiteBridgeCounterEpoch copiedEpoch(firstEpoch);
  const SuiteBridgeCounterEpoch secondEpoch;

  assert(firstEpoch.Size() == 32);
  assert(secondEpoch.Size() == 32);
  assert(IsLowerHex(firstEpoch.Data()));
  assert(IsLowerHex(secondEpoch.Data()));
  assert(std::strcmp(firstEpoch.Data(), copiedEpoch.Data()) == 0);
  assert(std::strcmp(firstEpoch.Data(), secondEpoch.Data()) != 0);

  SuiteBridgeSaturatingCounter normal;

  assert(normal.Value() == 0);
  assert(!normal.Overflowed());
  assert(normal.Increment() == 1);
  assert(normal.Increment() == 2);
  assert(normal.Value() == 2);
  assert(!normal.Overflowed());

  constexpr unsigned long long maximum =
      std::numeric_limits<unsigned long long>::max();
  SuiteBridgeSaturatingCounter saturated(maximum - 1);

  assert(saturated.Value() == maximum - 1);
  assert(!saturated.Overflowed());
  assert(saturated.Increment() == maximum);
  assert(saturated.Value() == maximum);
  assert(!saturated.Overflowed());
  assert(saturated.Increment() == maximum);
  assert(saturated.Value() == maximum);
  assert(saturated.Overflowed());
  assert(saturated.Increment() == maximum);
  assert(saturated.Value() == maximum);
  assert(saturated.Overflowed());

  return 0;
}
