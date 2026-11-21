#ifndef VDR_SUITE_BRIDGE_LIVE_REPLAY_BUFFER_H
#define VDR_SUITE_BRIDGE_LIVE_REPLAY_BUFFER_H

#include "suitebridge_live_transport_buffer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

class SuiteBridgeTsReplayBuffer final {
public:
  using Packet = SuiteBridgeTsPacketBuffer::Packet;
  using Sequence = std::uint64_t;

  enum class PushResult {
    Accepted,
    InvalidPacket,
    StartWindowExhausted,
  };

  enum class ReadResult {
    Available,
    NotYetAvailable,
    Overrun,
  };

  explicit SuiteBridgeTsReplayBuffer(std::size_t capacityPackets)
      : slots_(capacityPackets == 0 ? 1 : capacityPackets) {}

  PushResult Push(
      const std::uint8_t *data,
      std::size_t length,
      bool cleanStart) noexcept
  {
    if (data == nullptr || length != SuiteBridgeTsPacketBuffer::PacketSize ||
        data[0] != SuiteBridgeTsPacketBuffer::SyncByte) {
      return PushResult::InvalidPacket;
    }

    if (size_ == slots_.size()) {
      if (haveStart_ && oldestSequence_ == startSequence_ && !cleanStart) {
        return PushResult::StartWindowExhausted;
      }
      ++oldestSequence_;
      --size_;
    }

    const Sequence sequence = nextSequence_;
    std::memcpy(
        slots_[static_cast<std::size_t>(sequence % slots_.size())].data(),
        data,
        SuiteBridgeTsPacketBuffer::PacketSize);
    ++nextSequence_;
    ++size_;

    if (cleanStart) {
      startSequence_ = sequence;
      haveStart_ = true;
    }
    return PushResult::Accepted;
  }

  PushResult Push(const Packet &packet, bool cleanStart) noexcept
  {
    return Push(packet.data(), packet.size(), cleanStart);
  }

  bool StartCursor(Sequence &sequence) const noexcept
  {
    if (!haveStart_ || startSequence_ < oldestSequence_ ||
        startSequence_ >= nextSequence_) {
      return false;
    }
    sequence = startSequence_;
    return true;
  }

  ReadResult Read(Sequence sequence, Packet &packet) const noexcept
  {
    if (sequence < oldestSequence_) return ReadResult::Overrun;
    if (sequence >= nextSequence_) return ReadResult::NotYetAvailable;
    packet = slots_[static_cast<std::size_t>(sequence % slots_.size())];
    return ReadResult::Available;
  }

  std::size_t CapacityPackets() const noexcept { return slots_.size(); }
  std::size_t Size() const noexcept { return size_; }
  Sequence OldestSequence() const noexcept { return oldestSequence_; }
  Sequence NextSequence() const noexcept { return nextSequence_; }

private:
  std::vector<Packet> slots_;
  Sequence oldestSequence_ = 0;
  Sequence nextSequence_ = 0;
  Sequence startSequence_ = 0;
  std::size_t size_ = 0;
  bool haveStart_ = false;
};

#endif
