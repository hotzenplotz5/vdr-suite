#ifndef VDR_SUITE_BRIDGE_LIVE_CAPABILITY_H
#define VDR_SUITE_BRIDGE_LIVE_CAPABILITY_H

#include "suitebridge_command_result.h"

#include <sstream>
#include <string>
#include <strings.h>

class SuiteBridgeLiveCapabilityService final {
public:
  explicit SuiteBridgeLiveCapabilityService(std::string pluginInstanceEpoch)
      : pluginInstanceEpoch_(std::move(pluginInstanceEpoch)) {}

  SuiteBridgeCommandResult Handle(const char *command, const char *option) const
  {
    if (command == nullptr || strcasecmp(command, "NLCAP") != 0) return {};
    SuiteBridgeCommandResult result;
    result.handled = true;
    if (option == nullptr || std::string(option) != "1") {
      result.replyCode = 504;
      result.payload = "live_capability_schema_unsupported";
      return result;
    }
    result.replyCode = 250;
    std::ostringstream payload;
    payload << "{\"providerId\":\"suitebridge:local\""
            << ",\"providerKind\":\"suitebridge\""
            << ",\"pluginInstanceEpoch\":\"" << pluginInstanceEpoch_ << "\""
            << ",\"providerGeneration\":1"
            << ",\"capabilityRevision\":1"
            << ",\"capability\":\"vdr.live.stream\""
            << ",\"available\":true}";
    result.payload = payload.str();
    return result;
  }

private:
  std::string pluginInstanceEpoch_;
};

#endif
