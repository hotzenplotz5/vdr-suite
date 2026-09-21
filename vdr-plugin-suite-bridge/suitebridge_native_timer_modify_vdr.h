#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_MODIFY_VDR_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_MODIFY_VDR_H

#include "suitebridge_native_timer_modify.h"

class SuiteBridgeNativeTimerModifyVdrMutationCallback final
    : public ISuiteBridgeNativeTimerModifyMutationCallback {
public:
  SuiteBridgeNativeTimerModifyMutationResult ModifyTimer(
      const SuiteBridgeNativeTimerModifyRequest &request) override;
};

#endif
