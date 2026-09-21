#ifndef VDR_SUITE_BRIDGE_RECORDING_MARKS_MODIFY_VDR_H
#define VDR_SUITE_BRIDGE_RECORDING_MARKS_MODIFY_VDR_H

#include "suitebridge_recording_marks_modify.h"

class SuiteBridgeRecordingMarksModifyVdrMutationCallback final
    : public ISuiteBridgeRecordingMarksModifyMutationCallback {
public:
  SuiteBridgeRecordingMarksModifyMutationResult ModifyMarks(
      const SuiteBridgeRecordingMarksModifyRequest &request) override;
};

#endif
