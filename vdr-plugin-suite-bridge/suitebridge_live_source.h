#ifndef VDR_SUITE_BRIDGE_LIVE_SOURCE_H
#define VDR_SUITE_BRIDGE_LIVE_SOURCE_H

#include "suitebridge_command_result.h"

#include <vdr/channels.h>
#include <vdr/device.h>
#include <vdr/receiver.h>
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
#include <deque>
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
  struct SharedState final {
    std::mutex mutex;
    std::condition_variable wake;
    std::deque<std::vector<uchar>> queue;
    std::size_t queuedBytes = 0;
    bool stopping = false;
    bool terminal = false;
    bool clientConnected = false;
    std::string terminalReason;
    int clientFd = -1;
  };

  class LiveReceiver final : public cReceiver {
  public:
    LiveReceiver(const cChannel *channel, std::shared_ptr<SharedState> state)
        : cReceiver(channel, LIVEPRIORITY), state_(std::move(state)) {}

  protected:
    void Receive(const uchar *data, int length) override
    {
      if (data == nullptr || length <= 0) return;
      std::unique_lock<std::mutex> lock(state_->mutex, std::try_to_lock);
      if (!lock.owns_lock() || state_->stopping || state_->terminal ||
          !state_->clientConnected) {
        return;
      }
      const std::size_t bytes = static_cast<std::size_t>(length);
      if (bytes > MaximumQueuedBytes ||
          state_->queuedBytes > MaximumQueuedBytes - bytes) {
        state_->terminal = true;
        state_->terminalReason = "backpressure_overflow";
        state_->queue.clear();
        state_->queuedBytes = 0;
        state_->wake.notify_all();
        return;
      }
      state_->queue.emplace_back(data, data + length);
      state_->queuedBytes += bytes;
      lock.unlock();
      state_->wake.notify_one();
    }

  private:
    static constexpr std::size_t MaximumQueuedBytes = 2U * 1024U * 1024U;
    std::shared_ptr<SharedState> state_;
  };

  struct Session final {
    std::string leaseId;
    std::string channelId;
    std::string socketPath;
    std::shared_ptr<SharedState> state = std::make_shared<SharedState>();
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
      if (listenFd < 0 || receiver == nullptr || !receiver->IsAttached()) return false;
      try {
        writer = std::thread([this]() { WriterLoop(); });
      } catch (...) {
        return false;
      }
      return true;
    }

    void Stop()
    {
      {
        std::lock_guard<std::mutex> lock(state->mutex);
        if (state->stopping) return;
        state->stopping = true;
        if (state->clientFd >= 0) ::shutdown(state->clientFd, SHUT_RDWR);
      }
      state->wake.notify_all();
      if (receiver != nullptr && receiver->IsAttached() && device != nullptr) {
        device->Detach(receiver.get());
      }
      if (listenFd >= 0) ::shutdown(listenFd, SHUT_RDWR);
      if (writer.joinable()) writer.join();
      receiver.reset();
      device = nullptr;
      CloseSocket();
    }

    bool Attached() const { return receiver != nullptr && receiver->IsAttached(); }

    std::string StateName() const
    {
      std::lock_guard<std::mutex> lock(state->mutex);
      if (state->terminal) return "terminal";
      if (state->stopping) return "stopping";
      return Attached() ? "active" : "preempted";
    }

    std::string Reason() const
    {
      std::lock_guard<std::mutex> lock(state->mutex);
      if (!state->terminalReason.empty()) return state->terminalReason;
      if (!Attached()) return "receiver_preempted";
      return "none";
    }

  private:
    void CloseSocket()
    {
      if (listenFd >= 0) {
        ::close(listenFd);
        listenFd = -1;
      }
      if (!socketPath.empty()) ::unlink(socketPath.c_str());
    }

    bool SendChunk(int fd, const std::vector<uchar> &chunk)
    {
      std::size_t offset = 0;
      while (offset < chunk.size()) {
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
            chunk.data() + offset,
            chunk.size() - offset,
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

    void DisconnectClient(int fd)
    {
      {
        std::lock_guard<std::mutex> lock(state->mutex);
        if (state->clientFd == fd) state->clientFd = -1;
        state->clientConnected = false;
        state->queue.clear();
        state->queuedBytes = 0;
      }
      ::shutdown(fd, SHUT_RDWR);
      ::close(fd);
    }

    void WriterLoop()
    {
      int client = -1;
      while (true) {
        {
          std::lock_guard<std::mutex> lock(state->mutex);
          if (state->stopping || state->terminal) break;
        }
        if (receiver == nullptr || !receiver->IsAttached()) {
          std::lock_guard<std::mutex> lock(state->mutex);
          state->terminal = true;
          state->terminalReason = "receiver_preempted";
          break;
        }

        if (client < 0) {
          pollfd descriptor {};
          descriptor.fd = listenFd;
          descriptor.events = POLLIN;
          const int polled = ::poll(&descriptor, 1, 100);
          if (polled < 0 && errno == EINTR) continue;
          if (polled > 0 && (descriptor.revents & POLLIN) != 0) {
            client = ::accept(listenFd, nullptr, nullptr);
            if (client >= 0) {
              const int flags = ::fcntl(client, F_GETFL, 0);
              if (flags < 0 || ::fcntl(client, F_SETFL, flags | O_NONBLOCK) != 0) {
                ::close(client);
                client = -1;
              } else {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->queue.clear();
                state->queuedBytes = 0;
                state->clientFd = client;
                state->clientConnected = true;
              }
            }
          }
          continue;
        }

        std::vector<uchar> chunk;
        {
          std::unique_lock<std::mutex> lock(state->mutex);
          state->wake.wait_for(lock, std::chrono::milliseconds(100), [this]() {
            return state->stopping || state->terminal || !state->queue.empty();
          });
          if (state->stopping || state->terminal) break;
          if (state->queue.empty()) continue;
          chunk = std::move(state->queue.front());
          state->queue.pop_front();
          state->queuedBytes -= chunk.size();
        }
        if (!SendChunk(client, chunk)) {
          DisconnectClient(client);
          client = -1;
        }
      }
      if (client >= 0) DisconnectClient(client);
      std::lock_guard<std::mutex> lock(state->mutex);
      state->clientConnected = false;
      state->clientFd = -1;
      state->queue.clear();
      state->queuedBytes = 0;
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
