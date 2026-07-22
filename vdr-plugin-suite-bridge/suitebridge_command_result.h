#ifndef VDR_SUITE_BRIDGE_COMMAND_RESULT_H
#define VDR_SUITE_BRIDGE_COMMAND_RESULT_H

#include <string>

struct SuiteBridgeCommandResult final {
  bool handled = false;
  int replyCode = 0;
  std::string payload;
};

#endif
