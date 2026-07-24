#ifndef VDR_SUITE_BRIDGE_EPG_COMMAND_HANDLER_H
#define VDR_SUITE_BRIDGE_EPG_COMMAND_HANDLER_H

#include "suitebridge_command_result.h"

class SuiteBridgeEpgCommandHandler final {
public:
  static SuiteBridgeCommandResult HandleArtwork(
      const char *command,
      const char *option);
  static SuiteBridgeCommandResult HandleMetadata(
      const char *command,
      const char *option);
  static SuiteBridgeCommandResult HandleTypeSnapshot(
      const char *command,
      const char *option);
};

#endif
