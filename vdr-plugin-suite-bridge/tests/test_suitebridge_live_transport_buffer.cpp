#include "../suitebridge_live_replay_buffer.h"
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

void TestReplayStartUnavailableUntilCleanBoundary()
{
  SuiteBridgeTsReplayBuffer buffer(4);
  SuiteBridgeTsReplayBuffer::Sequence cursor = 99;

  assert(buffer.Push(MakePacket(1), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(2), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(!buffer.StartCursor(cursor));

  assert(buffer.Push(MakePacket(3), true) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.StartCursor(cursor));

  SuiteBridgeTsReplayBuffer::Packet packet{};
  assert(buffer.Read(cursor, packet) ==
         SuiteBridgeTsReplayBuffer::ReadResult::Available);
  assert(SequenceOf(packet) == 3);
}

void TestReplayReconnectStartsAtLatestCleanBoundary()
{
  SuiteBridgeTsReplayBuffer buffer(8);
  assert(buffer.Push(MakePacket(10), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(11), true) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(12), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);

  SuiteBridgeTsReplayBuffer::Sequence probeCursor = 0;
  assert(buffer.StartCursor(probeCursor));

  SuiteBridgeTsReplayBuffer::Packet packet{};
  assert(buffer.Read(probeCursor++, packet) ==
         SuiteBridgeTsReplayBuffer::ReadResult::Available);
  assert(SequenceOf(packet) == 11);
  assert(buffer.Read(probeCursor++, packet) ==
         SuiteBridgeTsReplayBuffer::ReadResult::Available);
  assert(SequenceOf(packet) == 12);

  assert(buffer.Push(MakePacket(13), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(14), true) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(15), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);

  SuiteBridgeTsReplayBuffer::Sequence workerCursor = 0;
  assert(buffer.StartCursor(workerCursor));
  assert(buffer.Read(workerCursor++, packet) ==
         SuiteBridgeTsReplayBuffer::ReadResult::Available);
  assert(SequenceOf(packet) == 14);
  assert(buffer.Read(workerCursor++, packet) ==
         SuiteBridgeTsReplayBuffer::ReadResult::Available);
  assert(SequenceOf(packet) == 15);
}

void TestReplayPreStartHistoryMayBeDiscarded()
{
  SuiteBridgeTsReplayBuffer buffer(3);
  assert(buffer.Push(MakePacket(1), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(2), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(3), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(4), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(5), true) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);

  SuiteBridgeTsReplayBuffer::Sequence cursor = 0;
  assert(buffer.StartCursor(cursor));
  SuiteBridgeTsReplayBuffer::Packet packet{};
  assert(buffer.Read(cursor, packet) ==
         SuiteBridgeTsReplayBuffer::ReadResult::Available);
  assert(SequenceOf(packet) == 5);
}

void TestReplayLatestCleanStartIsNeverSilentlyOverwritten()
{
  SuiteBridgeTsReplayBuffer buffer(3);
  assert(buffer.Push(MakePacket(20), true) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(21), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(22), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(23), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::StartWindowExhausted);

  SuiteBridgeTsReplayBuffer::Sequence cursor = 0;
  assert(buffer.StartCursor(cursor));
  SuiteBridgeTsReplayBuffer::Packet packet{};
  assert(buffer.Read(cursor, packet) ==
         SuiteBridgeTsReplayBuffer::ReadResult::Available);
  assert(SequenceOf(packet) == 20);
}

void TestReplayNewCleanStartMayReplaceOldWindow()
{
  SuiteBridgeTsReplayBuffer buffer(3);
  assert(buffer.Push(MakePacket(30), true) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(31), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(32), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(33), true) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);

  SuiteBridgeTsReplayBuffer::Sequence cursor = 0;
  assert(buffer.StartCursor(cursor));
  SuiteBridgeTsReplayBuffer::Packet packet{};
  assert(buffer.Read(cursor, packet) ==
         SuiteBridgeTsReplayBuffer::ReadResult::Available);
  assert(SequenceOf(packet) == 33);
}

void TestReplayLaggingReaderFailsClosedOnOverrun()
{
  SuiteBridgeTsReplayBuffer buffer(4);
  assert(buffer.Push(MakePacket(40), true) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(41), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);

  SuiteBridgeTsReplayBuffer::Sequence cursor = 0;
  assert(buffer.StartCursor(cursor));
  assert(buffer.Push(MakePacket(42), true) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(43), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);
  assert(buffer.Push(MakePacket(44), false) ==
         SuiteBridgeTsReplayBuffer::PushResult::Accepted);

  SuiteBridgeTsReplayBuffer::Packet packet{};
  assert(buffer.Read(cursor, packet) ==
         SuiteBridgeTsReplayBuffer::ReadResult::Overrun);
}

}  // namespace

int main()
{
  TestPacketContract();
  TestBoundedOverflowIsExplicit();
  TestConsumerPausePreservesPacketOrder();
  TestConcurrentProducerConsumerDoesNotSilentlyDrop();
  TestReplayStartUnavailableUntilCleanBoundary();
  TestReplayReconnectStartsAtLatestCleanBoundary();
  TestReplayPreStartHistoryMayBeDiscarded();
  TestReplayLatestCleanStartIsNeverSilentlyOverwritten();
  TestReplayNewCleanStartMayReplaceOldWindow();
  TestReplayLaggingReaderFailsClosedOnOverrun();
  std::cout << "suitebridge live TS transport buffer tests passed\n";
  return 0;
}
