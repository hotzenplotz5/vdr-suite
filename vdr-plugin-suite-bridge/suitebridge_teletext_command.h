#ifndef VDR_SUITE_BRIDGE_TELETEXT_COMMAND_H
#define VDR_SUITE_BRIDGE_TELETEXT_COMMAND_H

#include "suitebridge_command_result.h"
#include "suitebridge_teletext_adapter.h"

class SuiteBridgeTeletextCommandService final {
public:
  explicit SuiteBridgeTeletextCommandService(
      const ISuiteBridgeTeletextProvider *provider)
      : provider_(provider) {}

  SuiteBridgeCommandResult Handle(
      const char *command,
      const char *option) const;

private:
  const ISuiteBridgeTeletextProvider *provider_;
};

#endif
