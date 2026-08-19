#include "../suitebridge_live_transport_buffer.h"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {

SuiteBridgeTsPacketBuffer::Packet MakePacket(std::uint32_t sequence)
{
  SuiteBridgeTsPacketBuffer::Packet packet{};
  packet[0] = SuiteBridgeTsPacketBuffer::SyncByte;
  packet[1] = static_cast<std::uint8_t>((sequence >> 24U) & 0xFFU);
  packet[2] = static_cast<std::uint8_t>((sequence >> 16U) & 0xFFU);
  packet[3] = static_cast<std::uint8_t>((sequence >> 8U) & 0xFFU);
  packet[4] = static_cast<std::uint8_t>(sequence & 0xFFU);
  for (std::size_t index = 5; index < packet.size(); ++index) {
    packet[index] = static_cast<std::uint8_t>((sequence + index) & 0xFFU);
  }
  return packet;
}

std::uint32_t SequenceOf(const SuiteBridgeTsPacketBuffer::Packet &packet)
{
  return (static_cast<std::uint32_t>(packet[1]) << 24U) |
         (static_cast<std::uint32_t>(packet[2]) << 16U) |
         (static_cast<std::uint32_t>(packet[3]) << 8U) |
         static_cast<std::uint32_t>(packet[4]);
}

void TestPacketContract()
{
  SuiteBridgeTsPacketBuffer buffer(2);
  auto packet = MakePacket(1);
  assert(buffer.Push(nullptr, packet.size()) ==
         SuiteBridgeTsPacketBuffer::PushResult::InvalidPacket);
  assert(buffer.Push(packet.data(), packet.size() - 1) ==
         SuiteBridgeTsPacketBuffer::PushResult::InvalidPacket);
  packet[0] = 0;
  assert(buffer.Push(packet.data(), packet.size()) ==
         SuiteBridgeTsPacketBuffer::PushResult::InvalidPacket);
}

void TestBoundedOverflowIsExplicit()
{
  SuiteBridgeTsPacketBuffer buffer(2);
  const auto first = MakePacket(1);
  const auto second = MakePacket(2);
  const auto overflow = MakePacket(3);

  assert(buffer.Push(first.data(), first.size()) ==
         SuiteBridgeTsPacketBuffer::PushResult::Accepted);
  assert(buffer.Push(second.data(), second.size()) ==
         SuiteBridgeTsPacketBuffer::PushResult::Accepted);
  assert(buffer.Push(overflow.data(), overflow.size()) ==
         SuiteBridgeTsPacketBuffer::PushResult::Full);

  SuiteBridgeTsPacketBuffer::Packet packet{};
  assert(buffer.Pop(packet));
  assert(packet == first);
  assert(buffer.Pop(packet));
  assert(packet == second);
  assert(!buffer.Pop(packet));
}

void TestConsumerPausePreservesPacketOrder()
{
  SuiteBridgeTsPacketBuffer buffer(4);
  const auto first = MakePacket(10);
  const auto second = MakePacket(11);
  const auto third = MakePacket(12);

  assert(buffer.Push(first.data(), first.size()) ==
         SuiteBridgeTsPacketBuffer::PushResult::Accepted);

  SuiteBridgeTsPacketBuffer::Packet packet{};
  assert(buffer.Pop(packet));
  assert(packet == first);

  // Simulate a disconnected consumer: producer keeps buffering and no reconnect
  // path is allowed to clear these packets.
  assert(buffer.Push(second.data(), second.size()) ==
         SuiteBridgeTsPacketBuffer::PushResult::Accepted);
  assert(buffer.Push(third.data(), third.size()) ==
         SuiteBridgeTsPacketBuffer::PushResult::Accepted);

  assert(buffer.Pop(packet));
  assert(packet == second);
  assert(buffer.Pop(packet));
  assert(packet == third);
}

void TestConcurrentProducerConsumerDoesNotSilentlyDrop()
{
  constexpr std::uint32_t PacketCount = 50000;
  SuiteBridgeTsPacketBuffer buffer(PacketCount);
  std::atomic<bool> producerFailed{false};

  std::thread producer([&]() {
    for (std::uint32_t sequence = 0; sequence < PacketCount; ++sequence) {
      const auto packet = MakePacket(sequence);
      if (buffer.Push(packet.data(), packet.size()) !=
          SuiteBridgeTsPacketBuffer::PushResult::Accepted) {
        producerFailed.store(true, std::memory_order_release);
        return;
      }
    }
  });

  std::thread consumer([&]() {
    SuiteBridgeTsPacketBuffer::Packet packet{};
    for (std::uint32_t expected = 0; expected < PacketCount;) {
      if (!buffer.Pop(packet)) {
        std::this_thread::yield();
        continue;
      }
      assert(packet[0] == SuiteBridgeTsPacketBuffer::SyncByte);
      assert(SequenceOf(packet) == expected);
      ++expected;
    }
  });

  producer.join();
  consumer.join();
  assert(!producerFailed.load(std::memory_order_acquire));
  assert(buffer.Empty());
}

}  // namespace

int main()
{
  TestPacketContract();
  TestBoundedOverflowIsExplicit();
  TestConsumerPausePreservesPacketOrder();
  TestConcurrentProducerConsumerDoesNotSilentlyDrop();
  std::cout << "suitebridge live TS transport buffer tests passed\n";
  return 0;
}
