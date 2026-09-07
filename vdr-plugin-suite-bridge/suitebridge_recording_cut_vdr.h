#ifndef VDR_SUITE_BRIDGE_RECORDING_CUT_VDR_H
#define VDR_SUITE_BRIDGE_RECORDING_CUT_VDR_H

#include "suitebridge_recording_cut.h"
#include "suitebridge_recording_cut_state.h"

#include <string>

class SuiteBridgeRecordingCutVdrMutationCallback final
    : public ISuiteBridgeRecordingCutMutationCallback {
public:
  SuiteBridgeRecordingCutState Inspect(
      const std::string &recordingKey) const;

  SuiteBridgeRecordingCutMutationResult StartCut(
      const SuiteBridgeRecordingCutRequest &request) override;
};

#endif
