#ifndef VDR_SUITE_BRIDGE_OSD_INPUT_H
#define VDR_SUITE_BRIDGE_OSD_INPUT_H

#include "suitebridge_command_result.h"
#include "suitebridge_osd_snapshot.h"

#include <array>
#include <cstddef>
#include <string>
#include <mutex>

class SuiteBridgeOsdInputService final {
public:
  static constexpr std::size_t RecentCommandCapacity = 64;

  SuiteBridgeCommandResult Handle(
      const char *command,
      const char *option,
      const SuiteBridgeOsdSnapshot &snapshot);

private:
  struct RecentCommand final {
    std::string commandId;
    std::string requestFingerprint;
  };

  const RecentCommand *Find(const std::string &commandId) const;
  void Remember(
      const std::string &commandId,
      const std::string &requestFingerprint);

  mutable std::mutex mutex_;
  std::array<RecentCommand, RecentCommandCapacity> recentCommands_{};
  std::size_t recentCommandIndex_ = 0;
};

#endif
