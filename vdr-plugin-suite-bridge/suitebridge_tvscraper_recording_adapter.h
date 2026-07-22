#ifndef VDR_SUITE_BRIDGE_TVSCRAPER_RECORDING_ADAPTER_H
#define VDR_SUITE_BRIDGE_TVSCRAPER_RECORDING_ADAPTER_H

#include "suitebridge_recording_metadata.h"

#include <memory>
#include <string>

class cRecording;

class SuiteBridgeTvScraperRecordingSession final {
public:
  enum class State {
    NotStarted,
    ServiceUnavailable,
    ProviderNoMatch,
    Ready,
  };

  SuiteBridgeTvScraperRecordingSession() noexcept;
  ~SuiteBridgeTvScraperRecordingSession();

  SuiteBridgeTvScraperRecordingSession(
      SuiteBridgeTvScraperRecordingSession &&other) noexcept;
  SuiteBridgeTvScraperRecordingSession &operator=(
      SuiteBridgeTvScraperRecordingSession &&other) noexcept;

  SuiteBridgeTvScraperRecordingSession(
      const SuiteBridgeTvScraperRecordingSession &) = delete;
  SuiteBridgeTvScraperRecordingSession &operator=(
      const SuiteBridgeTvScraperRecordingSession &) = delete;

  State GetState() const noexcept;
  SuiteBridgeRecordingMetadata Resolve(
      const std::string &recordingKey) const;

private:
  struct Impl;
  explicit SuiteBridgeTvScraperRecordingSession(
      std::unique_ptr<Impl> impl) noexcept;

  std::unique_ptr<Impl> impl_;

  friend class SuiteBridgeTvScraperRecordingAdapter;
};

class SuiteBridgeTvScraperRecordingAdapter final {
public:
  SuiteBridgeTvScraperRecordingSession Start(
      const cRecording &recording) const;
};

#endif
