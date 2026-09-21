#ifndef VDR_SUITE_BRIDGE_LIVE_TRANSPORT_BUFFER_H
#define VDR_SUITE_BRIDGE_LIVE_TRANSPORT_BUFFER_H

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

class SuiteBridgeTsPacketBuffer final {
public:
  static constexpr std::size_t PacketSize = 188;
  static constexpr std::uint8_t SyncByte = 0x47;
  using Packet = std::array<std::uint8_t, PacketSize>;

  enum class PushResult {
    Accepted,
    Full,
    InvalidPacket,
  };

  explicit SuiteBridgeTsPacketBuffer(std::size_t capacityPackets)
      : slots_((capacityPackets == 0 ? 1 : capacityPackets) + 1) {}

  SuiteBridgeTsPacketBuffer(const SuiteBridgeTsPacketBuffer &) = delete;
  SuiteBridgeTsPacketBuffer &operator=(const SuiteBridgeTsPacketBuffer &) = delete;

  PushResult Push(const std::uint8_t *data, std::size_t length) noexcept
  {
    if (data == nullptr || length != PacketSize || data[0] != SyncByte) {
      return PushResult::InvalidPacket;
    }

    const std::size_t head = head_.load(std::memory_order_relaxed);
    const std::size_t next = Advance(head);
    if (next == tail_.load(std::memory_order_acquire)) {
      return PushResult::Full;
    }

    std::memcpy(slots_[head].data(), data, PacketSize);
    head_.store(next, std::memory_order_release);
    return PushResult::Accepted;
  }

  bool Pop(Packet &packet) noexcept
  {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(std::memory_order_acquire)) return false;

    packet = slots_[tail];
    tail_.store(Advance(tail), std::memory_order_release);
    return true;
  }

  bool Empty() const noexcept
  {
    return tail_.load(std::memory_order_acquire) ==
           head_.load(std::memory_order_acquire);
  }

  std::size_t CapacityPackets() const noexcept { return slots_.size() - 1; }

private:
  std::size_t Advance(std::size_t index) const noexcept
  {
    ++index;
    return index == slots_.size() ? 0 : index;
  }

  std::vector<Packet> slots_;
  std::atomic<std::size_t> head_{0};
  std::atomic<std::size_t> tail_{0};
};

#endif
