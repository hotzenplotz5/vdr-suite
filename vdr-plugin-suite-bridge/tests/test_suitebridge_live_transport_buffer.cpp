#include "../suitebridge_live_replay_buffer.h"
#include "../suitebridge_live_socket_writer.h"
#include "../suitebridge_live_transport_buffer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

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

int SendFlags()
{
#ifdef MSG_NOSIGNAL
  return MSG_NOSIGNAL;
#else
  return 0;
#endif
}

std::size_t FillSocketUntilBlocked(int fd)
{
  std::array<std::uint8_t, 4096> filler{};
  std::size_t total = 0;
  while (true) {
    const ssize_t written = ::send(fd, filler.data(), filler.size(), SendFlags());
    if (written > 0) {
      total += static_cast<std::size_t>(written);
      continue;
    }
    if (written < 0 && errno == EINTR) continue;
    assert(written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));
    break;
  }
  return total;
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

void TestSocketWriterSurvivesTransientBackpressure()
{
  int sockets[2] = {-1, -1};
  assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);

  const int flags = ::fcntl(sockets[0], F_GETFL, 0);
  assert(flags >= 0);
  assert(::fcntl(sockets[0], F_SETFL, flags | O_NONBLOCK) == 0);
  const int sendBuffer = 4096;
  assert(::setsockopt(
      sockets[0], SOL_SOCKET, SO_SNDBUF, &sendBuffer, sizeof(sendBuffer)) == 0);

  const std::size_t filled = FillSocketUntilBlocked(sockets[0]);
  assert(filled > 0);

  const auto packet = MakePacket(77);
  std::vector<std::uint8_t> received(filled + packet.size());
  std::atomic<bool> stopping{false};
  std::atomic<bool> terminal{false};

  std::thread reader([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    std::size_t offset = 0;
    while (offset < received.size()) {
      const ssize_t count = ::recv(
          sockets[1], received.data() + offset, received.size() - offset, 0);
      if (count < 0 && errno == EINTR) continue;
      assert(count > 0);
      offset += static_cast<std::size_t>(count);
    }
  });

  const auto startedAt = std::chrono::steady_clock::now();
  assert(SuiteBridgeLiveSendAll(
      sockets[0], packet.data(), packet.size(), stopping, terminal));
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - startedAt);

  reader.join();
  assert(elapsed >= std::chrono::milliseconds(250));
  assert(std::equal(
      packet.begin(),
      packet.end(),
      received.end() - static_cast<std::ptrdiff_t>(packet.size())));

  ::close(sockets[0]);
  ::close(sockets[1]);
}

void TestSocketWriterFailsClosedOnDisconnect()
{
  int sockets[2] = {-1, -1};
  assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
  const int flags = ::fcntl(sockets[0], F_GETFL, 0);
  assert(flags >= 0);
  assert(::fcntl(sockets[0], F_SETFL, flags | O_NONBLOCK) == 0);
  ::close(sockets[1]);

  const auto packet = MakePacket(78);
  std::atomic<bool> stopping{false};
  std::atomic<bool> terminal{false};
  assert(!SuiteBridgeLiveSendAll(
      sockets[0], packet.data(), packet.size(), stopping, terminal));
  ::close(sockets[0]);
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
  TestSocketWriterSurvivesTransientBackpressure();
  TestSocketWriterFailsClosedOnDisconnect();
  TestReplayStartUnavailableUntilCleanBoundary();
  TestReplayReconnectStartsAtLatestCleanBoundary();
  TestReplayPreStartHistoryMayBeDiscarded();
  TestReplayLatestCleanStartIsNeverSilentlyOverwritten();
  TestReplayNewCleanStartMayReplaceOldWindow();
  TestReplayLaggingReaderFailsClosedOnOverrun();
  std::cout << "suitebridge live TS transport buffer tests passed\n";
  return 0;
}
