#ifndef VDR_SUITE_BRIDGE_RECORDING_CUT_VDR_H
#define VDR_SUITE_BRIDGE_RECORDING_CUT_VDR_H

#include "suitebridge_recording_cut.h"

class SuiteBridgeRecordingCutVdrMutationCallback final
    : public ISuiteBridgeRecordingCutMutationCallback {
public:
  SuiteBridgeRecordingCutMutationResult StartCut(
      const SuiteBridgeRecordingCutRequest &request) override;
};

#endif
