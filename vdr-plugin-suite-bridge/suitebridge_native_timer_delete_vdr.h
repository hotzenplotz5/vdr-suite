#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_DELETE_VDR_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_DELETE_VDR_H

#include "suitebridge_native_timer_delete.h"

class SuiteBridgeNativeTimerDeleteVdrMutationCallback final :
    public ISuiteBridgeNativeTimerDeleteMutationCallback {
public:
  SuiteBridgeNativeTimerDeleteMutationResult DeleteTimer(
      const SuiteBridgeNativeTimerDeleteRequest &request) override;
};

#endif
