#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_PREVIEW_SERVICE_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_PREVIEW_SERVICE_H

#include "suitebridge_command_result.h"
#include "suitebridge_native_timer_preview_core.h"
#include "suitebridge_native_timer_preview_vdr.h"

#include <functional>
#include <string>

// Dedicated READ-ONLY SVDRP handler. No EXEC or timer mutation code is shared.
// A valid positive result must originate in the VDR snapshot callback.
class SuiteBridgeNativeTimerPreviewService {
public:
  using Capture = std::function<vdrsuite_native_timer_preview::NativeSnapshot(
      const SuiteBridgeNativeTimerPreviewVdrRequest&)>;

  explicit SuiteBridgeNativeTimerPreviewService(
      std::string providerEpoch, Capture capture = {});

  SuiteBridgeCommandResult Handle(
      const char* command, const char* option) const;

private:
  std::string providerEpoch_;
  Capture capture_;
};

#endif
