#ifndef VDR_SUITE_BRIDGE_COUNTER_CONTINUITY_H
#define VDR_SUITE_BRIDGE_COUNTER_CONTINUITY_H

#include <array>
#include <atomic>
#include <cstddef>

class SuiteBridgeCounterEpoch final {
public:
  static constexpr std::size_t HexLength() noexcept
  {
    return 32;
  }

  static constexpr std::size_t Capacity() noexcept
  {
    return HexLength() + 1;
  }

  SuiteBridgeCounterEpoch() noexcept;

  SuiteBridgeCounterEpoch(const SuiteBridgeCounterEpoch &) noexcept = default;
  SuiteBridgeCounterEpoch &operator=(const SuiteBridgeCounterEpoch &) = delete;

  const char *Data() const noexcept;
  std::size_t Size() const noexcept;

private:
  std::array<char, Capacity()> data_;
};

class SuiteBridgeSaturatingCounter final {
public:
  explicit SuiteBridgeSaturatingCounter(
      unsigned long long initialValue = 0) noexcept;

  unsigned long long Increment() noexcept;
  unsigned long long Value() const noexcept;
  bool Overflowed() const noexcept;

private:
  std::atomic<unsigned long long> value_;
  std::atomic<bool> overflowed_;
};

#endif
