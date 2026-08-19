#ifndef VDR_SUITE_BRIDGE_LIVE_SOURCE_H
#define VDR_SUITE_BRIDGE_LIVE_SOURCE_H

#include "suitebridge_command_result.h"
#include "suitebridge_live_replay_buffer.h"
#include "suitebridge_live_transport_buffer.h"

#include <vdr/channels.h>
#include <vdr/device.h>
#include <vdr/receiver.h>
#include <vdr/remux.h>
#include <vdr/tools.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <memory>
#include <mutex>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

class SuiteBridgeLiveSourceService final {
private:
  enum class TerminalReason {
    None,
    BackpressureOverflow,
    TsPacketContractViolation,
    ReceiverPreempted,
    ReplayWindowExhausted,
    FrameBoundaryUnavailable,
    ClientReplayOverrun,
  };

  struct SharedState final {
    static constexpr std::size_t BufferBytes = 5U * 1024U * 1024U;
    static constexpr std::size_t BufferPackets =
        BufferBytes / SuiteBridgeTsPacketBuffer::PacketSize;

    SuiteBridgeTsPacketBuffer buffer{BufferPackets};
    std::mutex waitMutex;
    std::mutex clientMutex;
    std::condition_variable wake;
    std::atomic<bool> stopping{false};
    std::atomic<bool> terminal{false};
    std::atomic<bool> clientConnected{false};
    std::atomic<TerminalReason> terminalReason{TerminalReason::None};
    int clientFd = -1;

    void MarkTerminal(TerminalReason reason)
    {
      TerminalReason expected = TerminalReason::None;
      terminalReason.compare_exchange_strong(
          expected,
          reason,
          std::memory_order_acq_rel,
          std::memory_order_acquire);
      terminal.store(true, std::memory_order_release);
      wake.notify_all();
    }
  };

  class LiveReceiver final : public cReceiver {
  public:
    LiveReceiver(const cChannel *channel, std::shared_ptr<SharedState> state)
        : cReceiver(channel, LIVEPRIORITY), state_(std::move(state)) {}

  protected:
    void Receive(const uchar *data, int length) override
    {
      if (data == nullptr || length <= 0) return;
      if (state_->stopping.load(std::memory_order_acquire) ||
          state_->terminal.load(std::memory_order_acquire)) {
        return;
      }

      const auto result = state_->buffer.Push(
          reinterpret_cast<const std::uint8_t *>(data),
          static_cast<std::size_t>(length));
      if (result == SuiteBridgeTsPacketBuffer::PushResult::InvalidPacket) {
        state_->MarkTerminal(TerminalReason::TsPacketContractViolation);
        return;
      }
      if (result == SuiteBridgeTsPacketBuffer::PushResult::Full) {
        state_->MarkTerminal(TerminalReason::BackpressureOverflow);
        return;
      }
      state_->wake.notify_one();
    }

  private:
    std::shared_ptr<SharedState> state_;
  };

  struct Session final {
    using Packet = SuiteBridgeTsPacketBuffer::Packet;
    using ReplaySequence = SuiteBridgeTsReplayBuffer::Sequence;

    static constexpr std::size_t AnalysisMinimumBytes =
        MIN_TS_PACKETS_FOR_FRAME_DETECTOR * SuiteBridgeTsPacketBuffer::PacketSize;
    static constexpr std::size_t AnalysisMaximumBytes = SharedState::BufferBytes;
    static constexpr std::size_t WorkPacketsPerCycle = 512;

    std::string leaseId;
    std::string channelId;
    std::string socketPath;
    std::shared_ptr<SharedState> state = std::make_shared<SharedState>();
    std::vector<Packet> startupPackets;
    SuiteBridgeTsReplayBuffer replay{SharedState::BufferPackets};
    std::unique_ptr<cFrameDetector> frameDetector;
    std::vector<std::uint8_t> analysisBytes;
    cDevice *device = nullptr;
    std::unique_ptr<LiveReceiver> receiver;
    int listenFd = -1;
    std::thread writer;

    ~Session() { Stop(); }

    bool PrepareSocket(const std::string &root)
    {
      if (root.empty() || root.front() != '/' || root.size() > 80 ||
          root.find("..") != std::string::npos) {
        return false;
      }
      if (::mkdir(root.c_str(), 0700) != 0 && errno != EEXIST) return false;
      struct stat status {};
      if (::lstat(root.c_str(), &status) != 0 || !S_ISDIR(status.st_mode)) {
        return false;
      }
      if (::chmod(root.c_str(), 0700) != 0) return false;

      socketPath = root + "/" + leaseId + ".sock";
      if (socketPath.size() >= sizeof(sockaddr_un::sun_path)) return false;
      ::unlink(socketPath.c_str());
      listenFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
      if (listenFd < 0) return false;
      const int flags = ::fcntl(listenFd, F_GETFL, 0);
      if (flags < 0 || ::fcntl(listenFd, F_SETFL, flags | O_NONBLOCK) != 0) {
        CloseSocket();
        return false;
      }
      sockaddr_un address {};
      address.sun_family = AF_UNIX;
      std::memcpy(address.sun_path, socketPath.c_str(), socketPath.size() + 1);
      if (::bind(listenFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 ||
          ::chmod(socketPath.c_str(), 0600) != 0 || ::listen(listenFd, 1) != 0) {
        CloseSocket();
        return false;
      }
      return true;
    }

    bool Attach()
    {
      const tChannelID nativeId = tChannelID::FromString(channelId.c_str());
      if (!nativeId.Valid()) return false;
      LOCK_CHANNELS_READ;
      const cChannel *channel = Channels->GetByChannelID(nativeId);
      if (channel == nullptr || channel->GroupSep()) return false;
      if (!PrepareStartupPackets(channel) || !PrepareFrameDetector(channel)) return false;
      device = cDevice::GetDevice(channel, LIVEPRIORITY, false);
      if (device == nullptr || !device->SwitchChannel(channel, false)) {
        device = nullptr;
        return false;
      }
      receiver = std::make_unique<LiveReceiver>(channel, state);
      if (!device->AttachReceiver(receiver.get())) {
        receiver.reset();
        device = nullptr;
        return false;
      }
      return true;
    }

    bool StartWriter()
    {
      if (listenFd < 0 || receiver == nullptr || !receiver->IsAttached() ||
          frameDetector == nullptr) {
        return false;
      }
      try {
        writer = std::thread([this]() { WriterLoop(); });
      } catch (...) {
        return false;
      }
      return true;
    }

    void Stop()
    {
      if (state->stopping.exchange(true, std::memory_order_acq_rel)) return;
      {
        std::lock_guard<std::mutex> lock(state->clientMutex);
        if (state->clientFd >= 0) ::shutdown(state->clientFd, SHUT_RDWR);
      }
      state->wake.notify_all();
      if (receiver != nullptr && receiver->IsAttached() && device != nullptr) {
        device->Detach(receiver.get());
      }
      if (listenFd >= 0) ::shutdown(listenFd, SHUT_RDWR);
      if (writer.joinable()) writer.join();
      receiver.reset();
      frameDetector.reset();
      device = nullptr;
      CloseSocket();
    }

    bool Attached() const { return receiver != nullptr && receiver->IsAttached(); }

    std::string StateName() const
    {
      if (state->terminal.load(std::memory_order_acquire)) return "terminal";
      if (state->stopping.load(std::memory_order_acquire)) return "stopping";
      return Attached() ? "active" : "preempted";
    }

    std::string Reason() const
    {
      switch (state->terminalReason.load(std::memory_order_acquire)) {
        case TerminalReason::BackpressureOverflow:
          return "backpressure_overflow";
        case TerminalReason::TsPacketContractViolation:
          return "ts_packet_contract_violation";
        case TerminalReason::ReceiverPreempted:
          return "receiver_preempted";
        case TerminalReason::ReplayWindowExhausted:
          return "replay_window_exhausted";
        case TerminalReason::FrameBoundaryUnavailable:
          return "frame_boundary_unavailable";
        case TerminalReason::ClientReplayOverrun:
          return "client_replay_overrun";
        case TerminalReason::None:
          break;
      }
      if (!Attached()) return "receiver_preempted";
      return "none";
    }

  private:
    bool PrepareStartupPackets(const cChannel *channel)
    {
      startupPackets.clear();
      cPatPmtGenerator generator(channel);
      if (!AppendStartupPacket(generator.GetPat())) return false;

      int index = 0;
      bool havePmt = false;
      while (const uchar *pmt = generator.GetPmt(index)) {
        if (!AppendStartupPacket(pmt)) return false;
        havePmt = true;
      }
      return havePmt;
    }

    bool PrepareFrameDetector(const cChannel *channel)
    {
      int pid = channel->Vpid();
      int type = channel->Vtype();
      if (!pid && channel->Apid(0)) {
        pid = channel->Apid(0);
        type = 0x04;
      }
      if (!pid && channel->Dpid(0)) {
        pid = channel->Dpid(0);
        type = 0x06;
      }
      if (pid <= 0) return false;

      try {
        frameDetector = std::make_unique<cFrameDetector>(pid, type);
        analysisBytes.reserve(AnalysisMinimumBytes * 4U);
      } catch (...) {
        frameDetector.reset();
        return false;
      }
      return true;
    }

    bool AppendStartupPacket(const uchar *data)
    {
      if (data == nullptr || data[0] != SuiteBridgeTsPacketBuffer::SyncByte) {
        return false;
      }
      Packet packet{};
      std::memcpy(packet.data(), data, packet.size());
      startupPackets.push_back(packet);
      return true;
    }

    bool QueueForFrameDetection(const Packet &packet)
    {
      if (analysisBytes.size() + packet.size() > AnalysisMaximumBytes) {
        state->MarkTerminal(TerminalReason::FrameBoundaryUnavailable);
        return false;
      }
      analysisBytes.insert(analysisBytes.end(), packet.begin(), packet.end());
      return ProcessDetectedFrames();
    }

    bool ProcessDetectedFrames()
    {
      while (analysisBytes.size() >= AnalysisMinimumBytes) {
        const int available = static_cast<int>(analysisBytes.size());
        const int count = frameDetector->Analyze(
            reinterpret_cast<const uchar *>(analysisBytes.data()), available);
        if (count == 0) return true;
        if (count < 0 || count > available ||
            (count % static_cast<int>(SuiteBridgeTsPacketBuffer::PacketSize)) != 0) {
          state->MarkTerminal(TerminalReason::TsPacketContractViolation);
          return false;
        }

        const bool cleanStart =
            frameDetector->Synced() && frameDetector->IndependentFrame();
        for (int offset = 0; offset < count;
             offset += static_cast<int>(SuiteBridgeTsPacketBuffer::PacketSize)) {
          const auto result = replay.Push(
              analysisBytes.data() + offset,
              SuiteBridgeTsPacketBuffer::PacketSize,
              cleanStart && offset == 0);
          if (result == SuiteBridgeTsReplayBuffer::PushResult::InvalidPacket) {
            state->MarkTerminal(TerminalReason::TsPacketContractViolation);
            return false;
          }
          if (result == SuiteBridgeTsReplayBuffer::PushResult::StartWindowExhausted) {
            state->MarkTerminal(TerminalReason::ReplayWindowExhausted);
            return false;
          }
        }
        analysisBytes.erase(analysisBytes.begin(), analysisBytes.begin() + count);
      }
      return true;
    }

    void CloseSocket()
    {
      if (listenFd >= 0) {
        ::close(listenFd);
        listenFd = -1;
      }
      if (!socketPath.empty()) ::unlink(socketPath.c_str());
    }

    bool SendPacket(int fd, const Packet &packet)
    {
      std::size_t offset = 0;
      while (offset < packet.size()) {
        pollfd descriptor {};
        descriptor.fd = fd;
        descriptor.events = POLLOUT;
        const int polled = ::poll(&descriptor, 1, 250);
        if (polled < 0 && errno == EINTR) continue;
        if (polled <= 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
          return false;
        }
        const ssize_t written = ::send(
            fd,
            packet.data() + offset,
            packet.size() - offset,
#ifdef MSG_NOSIGNAL
            MSG_NOSIGNAL
#else
            0
#endif
        );
        if (written > 0) {
          offset += static_cast<std::size_t>(written);
          continue;
        }
        if (written < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
          continue;
        }
        return false;
      }
      return true;
    }

    bool SendStartupPackets(int fd)
    {
      for (const auto &packet : startupPackets) {
        if (!SendPacket(fd, packet)) return false;
      }
      return true;
    }

    void SetClient(int fd)
    {
      {
        std::lock_guard<std::mutex> lock(state->clientMutex);
        state->clientFd = fd;
      }
      state->clientConnected.store(true, std::memory_order_release);
    }

    void DisconnectClient(int fd)
    {
      {
        std::lock_guard<std::mutex> lock(state->clientMutex);
        if (state->clientFd == fd) state->clientFd = -1;
      }
      state->clientConnected.store(false, std::memory_order_release);
      ::shutdown(fd, SHUT_RDWR);
      ::close(fd);
    }

    bool StoppingOrTerminal() const
    {
      return state->stopping.load(std::memory_order_acquire) ||
             state->terminal.load(std::memory_order_acquire);
    }

    bool TryAcceptClient(int &client)
    {
      pollfd descriptor {};
      descriptor.fd = listenFd;
      descriptor.events = POLLIN;
      const int polled = ::poll(&descriptor, 1, 0);
      if (polled < 0 && errno == EINTR) return false;
      if (polled <= 0 || (descriptor.revents & POLLIN) == 0) return false;

      client = ::accept(listenFd, nullptr, nullptr);
      if (client < 0) return false;
      const int flags = ::fcntl(client, F_GETFL, 0);
      if (flags < 0 || ::fcntl(client, F_SETFL, flags | O_NONBLOCK) != 0) {
        ::close(client);
        client = -1;
        return false;
      }
      SetClient(client);
      return true;
    }

    void WriterLoop()
    {
      int client = -1;
      bool clientStarted = false;
      ReplaySequence clientCursor = 0;

      while (true) {
        if (StoppingOrTerminal()) break;
        if (receiver == nullptr || !receiver->IsAttached()) {
          state->MarkTerminal(TerminalReason::ReceiverPreempted);
          break;
        }

        bool didWork = false;
        for (std::size_t processed = 0; processed < WorkPacketsPerCycle; ++processed) {
          Packet packet{};
          if (!state->buffer.Pop(packet)) break;
          didWork = true;
          if (!QueueForFrameDetection(packet)) break;
        }
        if (StoppingOrTerminal()) break;

        if (client < 0 && TryAcceptClient(client)) {
          clientStarted = false;
          clientCursor = 0;
          didWork = true;
        }

        if (client >= 0 && !clientStarted) {
          ReplaySequence start = 0;
          if (replay.StartCursor(start)) {
            if (!SendStartupPackets(client)) {
              DisconnectClient(client);
              client = -1;
            } else {
              clientCursor = start;
              clientStarted = true;
            }
            didWork = true;
          }
        }

        if (client >= 0 && clientStarted) {
          for (std::size_t sent = 0; sent < WorkPacketsPerCycle; ++sent) {
            Packet packet{};
            const auto result = replay.Read(clientCursor, packet);
            if (result == SuiteBridgeTsReplayBuffer::ReadResult::NotYetAvailable) break;
            if (result == SuiteBridgeTsReplayBuffer::ReadResult::Overrun) {
              state->MarkTerminal(TerminalReason::ClientReplayOverrun);
              break;
            }
            if (!SendPacket(client, packet)) {
              DisconnectClient(client);
              client = -1;
              clientStarted = false;
              break;
            }
            ++clientCursor;
            didWork = true;
          }
        }
        if (StoppingOrTerminal()) break;

        if (!didWork) {
          std::unique_lock<std::mutex> lock(state->waitMutex);
          state->wake.wait_for(lock, std::chrono::milliseconds(10), [this]() {
            return StoppingOrTerminal() || !state->buffer.Empty();
          });
        }
      }
      if (client >= 0) DisconnectClient(client);
      state->clientConnected.store(false, std::memory_order_release);
      {
        std::lock_guard<std::mutex> lock(state->clientMutex);
        state->clientFd = -1;
      }
    }
  };

public:
  static constexpr std::size_t MaximumSessions = 4;

  explicit SuiteBridgeLiveSourceService(std::string pluginInstanceEpoch)
      : pluginInstanceEpoch_(std::move(pluginInstanceEpoch)) {}

  ~SuiteBridgeLiveSourceService() { StopAll(); }

  SuiteBridgeCommandResult Handle(const char *command, const char *option)
  {
    if (command == nullptr || strcasecmp(command, "NLIVE") != 0) return {};
    SuiteBridgeCommandResult result;
    result.handled = true;
    std::istringstream input(option == nullptr ? "" : option);
    std::string operation;
    std::string schema;
    std::string leaseId;
    std::string channelId;
    std::string instanceEpoch;
    input >> operation >> schema >> leaseId;
    if (schema != "1" || !SafeToken(leaseId, 64)) return Invalid(result);

    if (operation == "OPEN") {
      input >> channelId >> instanceEpoch;
      std::string extra;
      if (!SafeToken(channelId, 96) || !SafeToken(instanceEpoch, 128) ||
          (input >> extra)) {
        return Invalid(result);
      }
      if (instanceEpoch != pluginInstanceEpoch_) return Stale(result);
      return Open(result, leaseId, channelId);
    }
    if (operation == "CLOSE" || operation == "STATUS") {
      input >> instanceEpoch;
      std::string extra;
      if (!SafeToken(instanceEpoch, 128) || (input >> extra)) {
        return Invalid(result);
      }
      if (instanceEpoch != pluginInstanceEpoch_) return Stale(result);
      return operation == "CLOSE" ? Close(result, leaseId) : Status(result, leaseId);
    }
    return Invalid(result);
  }

  void StopAll()
  {
    std::vector<std::shared_ptr<Session>> sessions;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (auto &entry : sessions_) sessions.push_back(entry.second);
      sessions_.clear();
    }
    for (auto &session : sessions) session->Stop();
  }

private:
  static bool SafeToken(const std::string &value, std::size_t maximum)
  {
    if (value.empty() || value.size() > maximum) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
      return std::isalnum(character) != 0 || character == '-' || character == '_' ||
          character == '.' || character == ':';
    });
  }

  static SuiteBridgeCommandResult Invalid(SuiteBridgeCommandResult result)
  {
    result.replyCode = 501;
    result.payload = "Usage: NLIVE OPEN|CLOSE|STATUS 1 <lease-id> [channel-id] <plugin-instance-epoch>";
    return result;
  }

  static SuiteBridgeCommandResult Stale(SuiteBridgeCommandResult result)
  {
    result.replyCode = 555;
    result.payload = "live_source_plugin_instance_epoch_stale";
    return result;
  }

  static std::string SocketRoot()
  {
    const char *configured = std::getenv("VDR_SUITE_LIVE_SOCKET_DIR");
    return configured == nullptr || *configured == '\0'
        ? std::string("/run/vdr/vdr-suite-live")
        : std::string(configured);
  }

  SuiteBridgeCommandResult Open(
      SuiteBridgeCommandResult result,
      const std::string &leaseId,
      const std::string &channelId)
  {
    std::shared_ptr<Session> session;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto existing = sessions_.find(leaseId);
      if (existing != sessions_.end()) {
        if (existing->second->channelId == channelId && existing->second->Attached()) {
          return Active(result, *existing->second);
        }
        result.replyCode = 550;
        result.payload = "live_source_lease_conflict";
        return result;
      }
      if (sessions_.size() >= MaximumSessions) {
        result.replyCode = 550;
        result.payload = "live_source_capacity_exhausted";
        return result;
      }
      session = std::make_shared<Session>();
      session->leaseId = leaseId;
      session->channelId = channelId;
      if (!session->PrepareSocket(SocketRoot())) {
        result.replyCode = 550;
        result.payload = "live_source_socket_unavailable";
        return result;
      }
      if (!session->Attach()) {
        session->Stop();
        result.replyCode = 550;
        result.payload = "live_source_receiver_unavailable";
        return result;
      }
      if (!session->StartWriter()) {
        session->Stop();
        result.replyCode = 550;
        result.payload = "live_source_writer_unavailable";
        return result;
      }
      sessions_.emplace(leaseId, session);
    }
    isyslog("suitebridge: live-source event=open lease=%s channel=%s socket=%s",
            leaseId.c_str(), channelId.c_str(), session->socketPath.c_str());
    return Active(result, *session);
  }

  SuiteBridgeCommandResult Close(
      SuiteBridgeCommandResult result,
      const std::string &leaseId)
  {
    std::shared_ptr<Session> session;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto found = sessions_.find(leaseId);
      if (found == sessions_.end()) {
        result.replyCode = 250;
        result.payload = "vdr-suite-live/1 state=terminal reason=already_closed";
        return result;
      }
      session = found->second;
      sessions_.erase(found);
    }
    session->Stop();
    isyslog("suitebridge: live-source event=close lease=%s channel=%s",
            leaseId.c_str(), session->channelId.c_str());
    result.replyCode = 250;
    result.payload = "vdr-suite-live/1 state=terminal reason=closed";
    return result;
  }

  SuiteBridgeCommandResult Status(
      SuiteBridgeCommandResult result,
      const std::string &leaseId)
  {
    std::shared_ptr<Session> session;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto found = sessions_.find(leaseId);
      if (found == sessions_.end()) {
        result.replyCode = 550;
        result.payload = "live_source_lease_unknown";
        return result;
      }
      session = found->second;
    }
    result.replyCode = 250;
    std::ostringstream output;
    output << "vdr-suite-live/1 state=" << session->StateName()
           << " reason=" << session->Reason()
           << " receiverAttached=" << (session->Attached() ? "true" : "false")
           << " channelId=" << session->channelId;
    result.payload = output.str();
    return result;
  }

  SuiteBridgeCommandResult Active(
      SuiteBridgeCommandResult result,
      const Session &session) const
  {
    result.replyCode = 250;
    std::ostringstream output;
    output << "vdr-suite-live/1 state=active receiverAttached=true channelId="
           << session.channelId << " socket=" << session.socketPath
           << " pluginInstanceEpoch=" << pluginInstanceEpoch_;
    result.payload = output.str();
    return result;
  }

  std::string pluginInstanceEpoch_;
  std::mutex mutex_;
  std::map<std::string, std::shared_ptr<Session>> sessions_;
};

#endif
