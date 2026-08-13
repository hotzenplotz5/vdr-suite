#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_DELETE_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_DELETE_H

#include "suitebridge_command_result.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

struct SuiteBridgeNativeTimerDeleteRequest final {
  std::string commandId;
  std::string requestFingerprint;
  std::string operationId;
  std::string operationRevision;
  std::string nativeTimerBindingId;
  std::string expectedBindingRevision;
  std::string timerAssignmentId;
  std::string backendNativeTimerId;
  std::string jobId;
  std::string attemptId;
  std::uint64_t claimEpoch = 0;
  std::string backendId;
  std::string agentId;
  std::string agentInstanceId;
  std::uint64_t backendGeneration = 0;
  std::uint64_t controlPlaneClaimedAt = 0;
  std::string authorityDomain;
  std::string providerId;
  std::string providerKind;
  std::uint64_t ownershipGeneration = 0;
  std::string providerInstanceEpoch;
  std::uint64_t providerGeneration = 0;
  std::uint64_t capabilityRevision = 0;
  std::string requiredCapability;
  std::uint64_t localStartingPersistedAt = 0;
};

enum class SuiteBridgeNativeTimerDeleteMutationDisposition {
  AppliedUnverified,
  RejectedWithoutEffect,
  OutcomeUnknown,
};

struct SuiteBridgeNativeTimerDeleteMutationResult final {
  SuiteBridgeNativeTimerDeleteMutationDisposition disposition =
      SuiteBridgeNativeTimerDeleteMutationDisposition::OutcomeUnknown;
  std::string evidenceReference;
};

class ISuiteBridgeNativeTimerDeleteMutationCallback {
public:
  virtual ~ISuiteBridgeNativeTimerDeleteMutationCallback() = default;

  virtual SuiteBridgeNativeTimerDeleteMutationResult DeleteTimer(
      const SuiteBridgeNativeTimerDeleteRequest &request) = 0;
};

class SuiteBridgeNativeTimerDeleteService final {
public:
  explicit SuiteBridgeNativeTimerDeleteService(
      std::string pluginInstanceEpoch,
      ISuiteBridgeNativeTimerDeleteMutationCallback *mutationCallback = nullptr,
      std::size_t maximumReplayEntries = 4096);

  SuiteBridgeCommandResult Handle(
      const char *command,
      const char *option);

  bool ExecutionConfigured() const noexcept;

private:
  struct ReplayEntry final {
    std::string commandId;
    std::string requestFingerprint;
    std::string canonicalRequest;
    bool terminal = false;
    SuiteBridgeNativeTimerDeleteMutationResult result;
  };

  SuiteBridgeCommandResult capability(const char *option) const;
  SuiteBridgeCommandResult execute(const char *option);
  SuiteBridgeNativeTimerDeleteMutationResult executeReserved(
      const SuiteBridgeNativeTimerDeleteRequest &request,
      const std::string &canonicalRequest,
      bool &replayed,
      bool &inProgress,
      bool &conflict,
      bool &ledgerFull);

  std::string pluginInstanceEpoch_;
  ISuiteBridgeNativeTimerDeleteMutationCallback *mutationCallback_ = nullptr;
  std::size_t maximumReplayEntries_ = 4096;
  std::mutex replayMutex_;
  std::unordered_map<std::string, ReplayEntry> replayByOperationId_;
  std::unordered_map<std::string, std::string> operationByCommandId_;
};

#endif