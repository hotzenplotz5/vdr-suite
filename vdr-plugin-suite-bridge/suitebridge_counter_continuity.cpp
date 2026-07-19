#include "suitebridge_counter_continuity.h"

#include <chrono>
#include <cstdint>
#include <limits>

#include <unistd.h>

namespace {

std::atomic<std::uint64_t> instanceSequence{0};

std::uint64_t Mix64(std::uint64_t value) noexcept
{
  value ^= value >> 30;
  value *= UINT64_C(0xbf58476d1ce4e5b9);
  value ^= value >> 27;
  value *= UINT64_C(0x94d049bb133111eb);
  value ^= value >> 31;
  return value;
}

void WriteHex64(
    std::uint64_t value,
    char *destination) noexcept
{
  static constexpr char hexDigits[] = "0123456789abcdef";

  for (std::size_t index = 0; index < 16; ++index) {
    const std::size_t shift = (15 - index) * 4;
    destination[index] = hexDigits[(value >> shift) & UINT64_C(0x0f)];
  }
}

}

SuiteBridgeCounterEpoch::SuiteBridgeCounterEpoch() noexcept
    : data_{{0}}
{
  const std::uint64_t sequence =
      instanceSequence.fetch_add(1, std::memory_order_relaxed) + 1;
  const std::uint64_t processId = static_cast<std::uint64_t>(getpid());
  const std::uint64_t monotonicTicks = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const std::uint64_t wallClockTicks = static_cast<std::uint64_t>(
      std::chrono::system_clock::now().time_since_epoch().count());

  std::uint64_t high = Mix64(
      monotonicTicks ^
      (processId << 32) ^
      sequence);
  std::uint64_t low = Mix64(
      wallClockTicks ^
      monotonicTicks ^
      (processId << 16) ^
      (sequence * UINT64_C(0x9e3779b97f4a7c15)));

  if (high == 0 && low == 0) {
    low = sequence;
  }

  WriteHex64(high, data_.data());
  WriteHex64(low, data_.data() + 16);
  data_[HexLength()] = '\0';
}

const char *SuiteBridgeCounterEpoch::Data() const noexcept
{
  return data_.data();
}

std::size_t SuiteBridgeCounterEpoch::Size() const noexcept
{
  return HexLength();
}

SuiteBridgeSaturatingCounter::SuiteBridgeSaturatingCounter(
    unsigned long long initialValue) noexcept
    : value_(initialValue),
      overflowed_(false)
{
}

unsigned long long SuiteBridgeSaturatingCounter::Increment() noexcept
{
  constexpr unsigned long long maximum =
      std::numeric_limits<unsigned long long>::max();

  unsigned long long current = value_.load(std::memory_order_relaxed);

  while (current != maximum) {
    if (value_.compare_exchange_weak(
            current,
            current + 1,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
      return current + 1;
    }
  }

  overflowed_.store(true, std::memory_order_release);
  return maximum;
}

unsigned long long SuiteBridgeSaturatingCounter::Value() const noexcept
{
  return value_.load(std::memory_order_relaxed);
}

bool SuiteBridgeSaturatingCounter::Overflowed() const noexcept
{
  return overflowed_.load(std::memory_order_acquire);
}
