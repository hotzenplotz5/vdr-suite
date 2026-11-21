#ifndef VDR_SUITE_BRIDGE_HBBTV_COMMAND_H
#define VDR_SUITE_BRIDGE_HBBTV_COMMAND_H

#include "suitebridge_command_result.h"
#include "suitebridge_hbbtv_adapter.h"

class SuiteBridgeHbbtvCommandService final {
public:
  explicit SuiteBridgeHbbtvCommandService(
      const ISuiteBridgeHbbtvProvider *provider)
      : provider_(provider) {}

  SuiteBridgeCommandResult Handle(
      const char *command,
      const char *option) const;

private:
  const ISuiteBridgeHbbtvProvider *provider_;
};

#endif
