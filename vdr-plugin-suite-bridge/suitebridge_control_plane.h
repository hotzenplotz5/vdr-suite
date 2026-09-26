#ifndef VDR_SUITE_BRIDGE_CONTROL_PLANE_H
#define VDR_SUITE_BRIDGE_CONTROL_PLANE_H

#include "SuiteBridgeControlPlaneProtocol.h"
#include "suitebridge_command_result.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

class SuiteBridgeControlPlane final
{
public:
  using Operation = vdrsuite::agent::control::Operation;
  using ServiceClass = vdrsuite::agent::control::ServiceClass;
  using Handler = std::function<SuiteBridgeCommandResult(
      Operation, const std::string &)>;
  using Logger = std::function<void(const std::string &)>;

  static constexpr std::size_t OperationCount = 14;
  static constexpr std::size_t ServiceClassCount = 3;

  struct Metrics final {
    std::array<std::uint64_t, OperationCount> admittedByOperation{};
    std::array<std::uint64_t, OperationCount> executedByOperation{};
    std::array<std::uint64_t, ServiceClassCount> queueHighWaterByClass{};
    std::array<std::uint64_t, ServiceClassCount> queueWaitNanosecondsByClass{};
    std::array<std::uint64_t, ServiceClassCount> executionNanosecondsByClass{};
    std::array<std::size_t, ServiceClassCount> queueDepthByClass{};
    std::array<std::uint64_t, ServiceClassCount> activeWorkersByClass{};
    std::uint64_t rejected = 0;
    std::uint64_t overloaded = 0;
    std::uint64_t deadlineExpiredBeforeExecution = 0;
    std::uint64_t protocolMismatch = 0;
    std::uint64_t peerRejected = 0;
    bool running = false;
  };

  explicit SuiteBridgeControlPlane(
      std::string socketPath =
          "/run/vdr/vdr-suite-control/control.sock",
      std::size_t criticalQueueCapacity = 16,
      std::size_t interactiveQueueCapacity = 16,
      std::size_t externalPluginQueueCapacity = 16);
  ~SuiteBridgeControlPlane();

  bool Start(Handler handler, Logger logger = {});
  void Stop();
  bool Running() const;
  Metrics SnapshotMetrics() const;
  const std::string &SocketPath() const { return socketPath_; }

private:
  struct Pending final {
    int fd = -1;
    vdrsuite::agent::control::Request request;
    std::uint64_t admittedAtNanoseconds = 0;
  };

  static std::size_t operationIndex(Operation operation);
  static std::size_t classIndex(ServiceClass serviceClass);

  void acceptLoop();
  void workerLoop(ServiceClass serviceClass);
  bool validatePeer(int fd);
  bool receiveRequest(
      int fd,
      vdrsuite::agent::control::Request &request,
      std::string &reason);
  void sendResult(
      int fd,
      const vdrsuite::agent::control::Request &request,
      vdrsuite::agent::control::Result result,
      int replyCode,
      const std::string &payload,
      const char *reason);
  void log(
      Operation operation,
      std::uint64_t requestId,
      const char *reason) const;
  void closeQueued(
      vdrsuite::agent::control::Result result,
      const char *reason);

  std::string socketPath_;
  std::array<std::size_t, ServiceClassCount> queueCapacity_;
  Handler handler_;
  Logger logger_;
  int listenFd_ = -1;
  std::atomic<bool> running_{false};
  std::atomic<bool> stopRequested_{false};
  std::thread acceptThread_;
  std::array<std::thread, ServiceClassCount> workerThreads_;
  mutable std::mutex queueMutex_;
  std::array<std::condition_variable, ServiceClassCount> queueChanged_;
  std::array<std::deque<Pending>, ServiceClassCount> queues_;

  std::array<std::atomic<std::uint64_t>, OperationCount>
      admittedByOperation_{};
  std::array<std::atomic<std::uint64_t>, OperationCount>
      executedByOperation_{};
  std::array<std::atomic<std::uint64_t>, ServiceClassCount>
      queueHighWaterByClass_{};
  std::array<std::atomic<std::uint64_t>, ServiceClassCount>
      queueWaitNanosecondsByClass_{};
  std::array<std::atomic<std::uint64_t>, ServiceClassCount>
      executionNanosecondsByClass_{};
  std::array<std::atomic<std::uint64_t>, ServiceClassCount>
      activeWorkersByClass_{};

  std::atomic<std::uint64_t> rejected_{0};
  std::atomic<std::uint64_t> overloaded_{0};
  std::atomic<std::uint64_t> deadlineExpiredBeforeExecution_{0};
  std::atomic<std::uint64_t> protocolMismatch_{0};
  std::atomic<std::uint64_t> peerRejected_{0};
};

#endif
