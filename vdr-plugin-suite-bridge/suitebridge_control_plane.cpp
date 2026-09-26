#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "suitebridge_control_plane.h"

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{
using namespace vdrsuite::agent::control;

bool configure(int fd)
{
  const int descriptorFlags = fcntl(fd, F_GETFD, 0);
  const int statusFlags = fcntl(fd, F_GETFL, 0);
  return descriptorFlags >= 0 &&
      statusFlags >= 0 &&
      fcntl(fd, F_SETFD, descriptorFlags | FD_CLOEXEC) == 0 &&
      fcntl(fd, F_SETFL, statusFlags | O_NONBLOCK) == 0;
}

bool safePath(const std::string &path)
{
  return !path.empty() && path.size() < sizeof(sockaddr_un::sun_path);
}

} // namespace

SuiteBridgeControlPlane::SuiteBridgeControlPlane(
    std::string socketPath,
    std::size_t criticalQueueCapacity,
    std::size_t interactiveQueueCapacity,
    std::size_t externalPluginQueueCapacity)
    : socketPath_(std::move(socketPath)),
      queueCapacity_{
          std::max<std::size_t>(1, criticalQueueCapacity),
          std::max<std::size_t>(1, interactiveQueueCapacity),
          std::max<std::size_t>(1, externalPluginQueueCapacity)}
{
}

SuiteBridgeControlPlane::~SuiteBridgeControlPlane()
{
  Stop();
}

std::size_t SuiteBridgeControlPlane::operationIndex(Operation operation)
{
  const auto value = static_cast<std::uint16_t>(operation);
  return value >= 1 && value <= OperationCount
      ? static_cast<std::size_t>(value - 1)
      : 0;
}

std::size_t SuiteBridgeControlPlane::classIndex(ServiceClass serviceClass)
{
  switch (serviceClass) {
    case ServiceClass::CriticalControl:
      return 0;
    case ServiceClass::InteractiveControlRead:
      return 1;
    case ServiceClass::ExternalPluginInteractive:
      return 2;
  }
  return 0;
}

bool SuiteBridgeControlPlane::Start(Handler handler, Logger logger)
{
  if (running_.load() || !handler || !safePath(socketPath_))
    return false;

  handler_ = std::move(handler);
  logger_ = std::move(logger);
  stopRequested_.store(false);

  listenFd_ = socket(
      AF_UNIX,
      SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK,
      0);
  if (listenFd_ < 0)
    return false;

  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  std::copy(socketPath_.begin(), socketPath_.end(), address.sun_path);
  address.sun_path[socketPath_.size()] = '\0';
  unlink(socketPath_.c_str());

  if (bind(
          listenFd_,
          reinterpret_cast<const sockaddr *>(&address),
          sizeof(address)) != 0 ||
      chmod(socketPath_.c_str(), 0660) != 0 ||
      listen(listenFd_, 32) != 0) {
    close(listenFd_);
    listenFd_ = -1;
    unlink(socketPath_.c_str());
    return false;
  }

  running_.store(true);
  workerThreads_[0] = std::thread(
      &SuiteBridgeControlPlane::workerLoop,
      this,
      ServiceClass::CriticalControl);
  workerThreads_[1] = std::thread(
      &SuiteBridgeControlPlane::workerLoop,
      this,
      ServiceClass::InteractiveControlRead);
  workerThreads_[2] = std::thread(
      &SuiteBridgeControlPlane::workerLoop,
      this,
      ServiceClass::ExternalPluginInteractive);
  acceptThread_ = std::thread(
      &SuiteBridgeControlPlane::acceptLoop,
      this);
  return true;
}

void SuiteBridgeControlPlane::Stop()
{
  if (!running_.exchange(false))
    return;

  stopRequested_.store(true);
  if (listenFd_ >= 0) {
    shutdown(listenFd_, SHUT_RDWR);
    close(listenFd_);
    listenFd_ = -1;
  }

  for (auto &changed : queueChanged_)
    changed.notify_all();

  if (acceptThread_.joinable())
    acceptThread_.join();

  closeQueued(Result::ProviderUnavailable, "shutdown");

  for (auto &changed : queueChanged_)
    changed.notify_all();
  for (auto &worker : workerThreads_) {
    if (worker.joinable())
      worker.join();
  }

  unlink(socketPath_.c_str());
}

bool SuiteBridgeControlPlane::Running() const
{
  return running_.load();
}

SuiteBridgeControlPlane::Metrics
SuiteBridgeControlPlane::SnapshotMetrics() const
{
  Metrics metrics;
  for (std::size_t i = 0; i < OperationCount; ++i) {
    metrics.admittedByOperation[i] = admittedByOperation_[i].load();
    metrics.executedByOperation[i] = executedByOperation_[i].load();
  }
  for (std::size_t i = 0; i < ServiceClassCount; ++i) {
    metrics.queueHighWaterByClass[i] =
        queueHighWaterByClass_[i].load();
    metrics.queueWaitNanosecondsByClass[i] =
        queueWaitNanosecondsByClass_[i].load();
    metrics.executionNanosecondsByClass[i] =
        executionNanosecondsByClass_[i].load();
    metrics.activeWorkersByClass[i] =
        activeWorkersByClass_[i].load();
  }
  metrics.rejected = rejected_.load();
  metrics.overloaded = overloaded_.load();
  metrics.deadlineExpiredBeforeExecution =
      deadlineExpiredBeforeExecution_.load();
  metrics.protocolMismatch = protocolMismatch_.load();
  metrics.peerRejected = peerRejected_.load();
  {
    std::lock_guard<std::mutex> guard(queueMutex_);
    for (std::size_t i = 0; i < ServiceClassCount; ++i)
      metrics.queueDepthByClass[i] = queues_[i].size();
  }
  metrics.running = running_.load();
  return metrics;
}

bool SuiteBridgeControlPlane::validatePeer(int fd)
{
#ifdef SO_PEERCRED
  ucred credentials{};
  socklen_t size = sizeof(credentials);
  if (getsockopt(
          fd,
          SOL_SOCKET,
          SO_PEERCRED,
          &credentials,
          &size) != 0)
    return false;
  return credentials.uid == geteuid() || credentials.uid == 0;
#else
  (void)fd;
  return false;
#endif
}

bool SuiteBridgeControlPlane::receiveRequest(
    int fd,
    Request &request,
    std::string &reason)
{
  pollfd descriptor{};
  descriptor.fd = fd;
  descriptor.events = POLLIN;
  int ready = 0;
  do {
    ready = poll(&descriptor, 1, 100);
  } while (ready < 0 && errno == EINTR);

  if (ready <= 0 || (descriptor.revents & POLLIN) == 0) {
    reason = "request_read_timeout";
    return false;
  }

  std::vector<std::uint8_t> frame(MaximumRequestFrameBytes + 1);
  const ssize_t received = recv(
      fd,
      frame.data(),
      frame.size(),
      MSG_TRUNC);
  if (received <= 0 ||
      static_cast<std::size_t>(received) > MaximumRequestFrameBytes) {
    reason = "request_frame_size";
    return false;
  }

  frame.resize(static_cast<std::size_t>(received));
  return decodeRequest(frame, request, reason);
}

void SuiteBridgeControlPlane::sendResult(
    int fd,
    const Request &request,
    Result result,
    int replyCode,
    const std::string &payload,
    const char *reason)
{
  Response response;
  response.result = result;
  response.requestId =
      request.requestId == 0 ? 1 : request.requestId;
  response.replyCode = replyCode;
  response.payload = payload.size() <= MaximumResponsePayloadBytes
      ? payload
      : "response_payload_too_large";

  std::vector<std::uint8_t> frame;
  if (encodeResponse(response, frame)) {
    pollfd descriptor{};
    descriptor.fd = fd;
    descriptor.events = POLLOUT;
    int ready = 0;
    do {
      ready = poll(&descriptor, 1, 100);
    } while (ready < 0 && errno == EINTR);

    if (ready > 0 &&
        (descriptor.revents & POLLOUT) != 0) {
      (void)send(
          fd,
          frame.data(),
          frame.size(),
          MSG_NOSIGNAL);
    }
  }

  log(request.operation, request.requestId, reason);
}

void SuiteBridgeControlPlane::log(
    Operation operation,
    std::uint64_t requestId,
    const char *reason) const
{
  if (!logger_)
    return;

  std::ostringstream message;
  message << "suitebridge: control-plane operation="
          << operationName(operation)
          << " class="
          << static_cast<unsigned>(
              serviceClass(operation))
          << " request-id=" << requestId
          << " reason="
          << (reason ? reason : "unknown");
  logger_(message.str());
}

void SuiteBridgeControlPlane::acceptLoop()
{
  while (!stopRequested_.load()) {
    pollfd descriptor{};
    descriptor.fd = listenFd_;
    descriptor.events = POLLIN;
    int ready = 0;
    do {
      ready = poll(&descriptor, 1, 100);
    } while (ready < 0 && errno == EINTR);

    if (ready <= 0 ||
        (descriptor.revents & POLLIN) == 0)
      continue;

    const int client = accept(listenFd_, nullptr, nullptr);
    if (client < 0)
      continue;
    if (!configure(client)) {
      close(client);
      continue;
    }

    Request request;
    if (!validatePeer(client)) {
      ++peerRejected_;
      ++rejected_;
      request.requestId = 1;
      sendResult(
          client,
          request,
          Result::PeerRejected,
          0,
          {},
          "peer_rejected");
      close(client);
      continue;
    }

    std::string reason;
    if (!receiveRequest(client, request, reason)) {
      ++protocolMismatch_;
      ++rejected_;
      if (request.requestId == 0)
        request.requestId = 1;
      sendResult(
          client,
          request,
          Result::ProtocolMismatch,
          0,
          {},
          "protocol_mismatch");
      close(client);
      continue;
    }

    const std::uint64_t now = monotonicNowNanoseconds();
    if (now == 0 || now >= request.deadlineNanoseconds) {
      ++deadlineExpiredBeforeExecution_;
      ++rejected_;
      sendResult(
          client,
          request,
          Result::DeadlineExpired,
          0,
          {},
          "deadline_expired_at_admission");
      close(client);
      continue;
    }

    const ServiceClass service =
        serviceClass(request.operation);
    const std::size_t lane = classIndex(service);

    bool admitted = false;
    std::size_t depth = 0;
    {
      std::lock_guard<std::mutex> guard(queueMutex_);
      if (!stopRequested_.load() &&
          queues_[lane].size() < queueCapacity_[lane]) {
        queues_[lane].push_back(
            Pending{client, request, now});
        admitted = true;
        depth = queues_[lane].size();
      }
    }

    if (!admitted) {
      ++overloaded_;
      ++rejected_;
      sendResult(
          client,
          request,
          Result::Overloaded,
          0,
          {},
          "queue_full");
      close(client);
      continue;
    }

    ++admittedByOperation_[operationIndex(request.operation)];

    auto high = queueHighWaterByClass_[lane].load();
    while (depth > high &&
           !queueHighWaterByClass_[lane].compare_exchange_weak(
               high,
               depth)) {
    }

    queueChanged_[lane].notify_one();
  }
}

void SuiteBridgeControlPlane::workerLoop(
    ServiceClass serviceClassValue)
{
  const std::size_t lane = classIndex(serviceClassValue);

  while (true) {
    Pending pending;
    {
      std::unique_lock<std::mutex> lock(queueMutex_);
      queueChanged_[lane].wait(lock, [this, lane] {
        return stopRequested_.load() ||
            !queues_[lane].empty();
      });

      if (queues_[lane].empty()) {
        if (stopRequested_.load())
          break;
        continue;
      }

      pending = std::move(queues_[lane].front());
      queues_[lane].pop_front();
    }

    const std::uint64_t beforeExecution =
        monotonicNowNanoseconds();
    if (beforeExecution == 0 ||
        beforeExecution >=
            pending.request.deadlineNanoseconds) {
      ++deadlineExpiredBeforeExecution_;
      ++rejected_;
      sendResult(
          pending.fd,
          pending.request,
          Result::DeadlineExpired,
          0,
          {},
          "deadline_expired_before_execution");
      close(pending.fd);
      continue;
    }

    queueWaitNanosecondsByClass_[lane].fetch_add(
        beforeExecution - pending.admittedAtNanoseconds);

    activeWorkersByClass_[lane].fetch_add(1);
    const std::uint64_t started = monotonicNowNanoseconds();
    const SuiteBridgeCommandResult result =
        handler_(
            pending.request.operation,
            pending.request.payload);
    const std::uint64_t finished =
        monotonicNowNanoseconds();
    activeWorkersByClass_[lane].fetch_sub(1);

    if (finished >= started)
      executionNanosecondsByClass_[lane].fetch_add(
          finished - started);

    ++executedByOperation_[
        operationIndex(pending.request.operation)];

    if (!result.handled) {
      ++rejected_;
      sendResult(
          pending.fd,
          pending.request,
          Result::ProviderUnavailable,
          result.replyCode,
          result.payload,
          "provider_unavailable");
    } else {
      const Result disposition =
          (result.replyCode >= 200 &&
           result.replyCode < 300) ||
              result.replyCode == 900
          ? Result::Success
          : result.replyCode == 555
          ? Result::StaleRejected
          : Result::NativeRejected;
      sendResult(
          pending.fd,
          pending.request,
          disposition,
          result.replyCode,
          result.payload,
          disposition == Result::Success
              ? "success"
              : disposition == Result::StaleRejected
              ? "stale_rejected"
              : "native_rejected");
    }

    close(pending.fd);
  }
}

void SuiteBridgeControlPlane::closeQueued(
    Result result,
    const char *reason)
{
  std::array<std::deque<Pending>, ServiceClassCount> pending;
  {
    std::lock_guard<std::mutex> guard(queueMutex_);
    for (std::size_t i = 0; i < ServiceClassCount; ++i)
      pending[i].swap(queues_[i]);
  }

  for (auto &queue : pending) {
    for (auto &entry : queue) {
      ++rejected_;
      sendResult(
          entry.fd,
          entry.request,
          result,
          0,
          {},
          reason);
      close(entry.fd);
    }
  }
}
