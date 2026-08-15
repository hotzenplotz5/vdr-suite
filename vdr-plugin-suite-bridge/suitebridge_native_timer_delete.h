#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_DELETE_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_DELETE_H

#include "suitebridge_command_result.h"

#include <string>

class SuiteBridgeNativeTimerDeleteService final {
public:
  explicit SuiteBridgeNativeTimerDeleteService(std::string pluginInstanceEpoch);

  SuiteBridgeCommandResult Handle(
      const char *command,
      const char *option) const;

private:
  SuiteBridgeCommandResult capability(const char *option) const;
  SuiteBridgeCommandResult executeDisabled(const char *option) const;

  std::string pluginInstanceEpoch_;
};

#endif
