#ifndef VDR_SUITE_BRIDGE_NATIVE_PROBE_H
#define VDR_SUITE_BRIDGE_NATIVE_PROBE_H

#include "suitebridge_command_result.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

std::string GenerateSuiteBridgePluginInstanceEpoch();

class SuiteBridgeNativeProbeService final {
public:
  static constexpr std::size_t ReceiptCapacity = 64;

  explicit SuiteBridgeNativeProbeService(std::string pluginInstanceEpoch);

  const std::string &PluginInstanceEpoch() const noexcept;
  using VdrActiveProbe = std::function<bool()>;

  SuiteBridgeCommandResult Handle(
      const char *command,
      const char *option,
      const VdrActiveProbe &vdrActiveProbe);

private:
  struct Request final {
    std::string commandId;
    std::string requestFingerprint;
    std::string operationId;
    std::string jobId;
    std::string attemptId;
    std::uint64_t claimEpoch = 0;
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::string pluginInstanceEpoch;
    std::string probeNonce;
  };

  struct ReceiptEntry final {
    bool occupied = false;
    Request request;
    std::string requestIdentity;
    std::uint64_t nativeExecutionSequence = 0;
    std::int64_t acceptedAt = 0;
    std::int64_t completedAt = 0;
    bool vdrActive = false;
  };

  SuiteBridgeCommandResult capability(const char *option) const;
  SuiteBridgeCommandResult execute(
      const char *option,
      const VdrActiveProbe &vdrActiveProbe);
  SuiteBridgeCommandResult readback(const char *option) const;
  ReceiptEntry *find(const std::string &commandId);
  const ReceiptEntry *find(const std::string &commandId) const;
  ReceiptEntry *reserve();

  std::string pluginInstanceEpoch_;
  mutable std::mutex mutex_;
  std::array<ReceiptEntry, ReceiptCapacity> receipts_{};
  std::uint64_t nativeExecutionSequence_ = 0;
};

#endif
