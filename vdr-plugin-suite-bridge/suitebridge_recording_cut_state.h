#ifndef VDR_SUITE_BRIDGE_RECORDING_CUT_STATE_H
#define VDR_SUITE_BRIDGE_RECORDING_CUT_STATE_H

#include <string>

struct SuiteBridgeRecordingCutState final {
  static constexpr const char *Protocol = "vdr-suite-rcut-state/1";

  bool found = false;
  std::string recordingKey;
  std::string reason = "recording-not-found";
  bool ready = false;
  bool marksReadable = false;
  std::string marksRevision;
  bool marksFilePresent = false;
  int markCount = 0;
  int sequenceCount = 0;
  int inUseFlags = 0;
  int handlerUsage = 0;
  std::string editedRecordingKey;
  bool editedDestinationExists = false;
  bool editedRecordingFound = false;
};

#endif
