#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_CREATE_VDR_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_CREATE_VDR_H

#include "suitebridge_native_timer_create.h"

class SuiteBridgeNativeTimerCreateVdrMutationCallback final :
    public ISuiteBridgeNativeTimerCreateMutationCallback {
public:
  SuiteBridgeNativeTimerCreateMutationResult CreateTimer(
      const SuiteBridgeNativeTimerCreateRequest &request) override;
};

#endif
