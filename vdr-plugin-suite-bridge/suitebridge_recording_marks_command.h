#ifndef VDR_SUITE_BRIDGE_RECORDING_MARKS_COMMAND_H
#define VDR_SUITE_BRIDGE_RECORDING_MARKS_COMMAND_H

#include "suitebridge_command_result.h"

class SuiteBridgeRecordingMarksCommand final {
public:
  static SuiteBridgeCommandResult Handle(
      const char *command,
      const char *option);
};

#endif
