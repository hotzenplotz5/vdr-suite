#include "suitebridge_recording_metadata_command.h"

#include "suitebridge_plugin_identity.h"
#include "suitebridge_recording_identity.h"
#include "suitebridge_recording_metadata_contract.h"
#include "suitebridge_tvscraper_recording_adapter.h"

#include <vdr/recording.h>
#include <vdr/tools.h>

#include <cstddef>
#include <string>

SuiteBridgeCommandResult SuiteBridgeRecordingMetadataCommand::Handle(
    const char *command,
    const char *option)
{
  const SuiteBridgeRecordingMetadataRequest request(command, option);
  if (!request.Handled()) {
    return {};
  }

  SuiteBridgeCommandResult result;
  result.handled = true;
  if (!request.Valid()) {
    result.replyCode = 501;
    result.payload = std::string("Usage: PLUG ") +
        SuiteBridgePluginIdentity::Name + " RMETA <recording-key>";
    return result;
  }

  const std::string recordingKey = request.RecordingKey();
  SuiteBridgeRecordingMetadata metadata;
  metadata.recordingKey = recordingKey;
  SuiteBridgeTvScraperRecordingSession session;
  std::size_t matchCount = 0;

  {
    // cRecording is not copied. The borrowed pointer exists only while the VDR
    // recording-list read lock is held and is consumed synchronously by
    // TVScraper's public GetScraperVideo service.
    LOCK_RECORDINGS_READ;
    const cRecording *matchedRecording = nullptr;

    for (const cRecording *recording = Recordings->First();
         recording != nullptr;
         recording = Recordings->Next(recording)) {
      const char *nativeId = recording->FileName();
      if (!nativeId ||
          SuiteBridgeRecordingIdentity::KeyForNativeId(nativeId) !=
              recordingKey) {
        continue;
      }

      matchedRecording = recording;
      ++matchCount;
      if (matchCount > 1) {
        break;
      }
    }

    if (matchCount == 1 && matchedRecording) {
      const SuiteBridgeTvScraperRecordingAdapter adapter;
      session = adapter.Start(*matchedRecording);
    }
  }

  if (matchCount == 0) {
    metadata.reason =
        SuiteBridgeRecordingMetadataReason::RecordingNotFound;
  } else if (matchCount > 1) {
    metadata.reason =
        SuiteBridgeRecordingMetadataReason::IdentityAmbiguous;
  } else {
    switch (session.GetState()) {
    case SuiteBridgeTvScraperRecordingSession::State::Ready:
      // The VDR recording lock is released. The session owns only TVScraper's
      // result object and never dereferences cRecording again.
      metadata = session.Resolve(recordingKey);
      break;
    case SuiteBridgeTvScraperRecordingSession::State::ProviderNoMatch:
      metadata.reason =
          SuiteBridgeRecordingMetadataReason::ProviderNoMatch;
      break;
    case SuiteBridgeTvScraperRecordingSession::State::ServiceUnavailable:
    case SuiteBridgeTvScraperRecordingSession::State::NotStarted:
      result.replyCode = 451;
      result.payload = "Recording metadata provider unavailable";
      esyslog(
          "suitebridge: svdrp command=RMETA result=provider-error recording_key=%s",
          recordingKey.c_str());
      return result;
    }
  }

  try {
    const SuiteBridgeRecordingMetadataPayload payload(metadata);
    if (!payload.Complete()) {
      result.replyCode = 451;
      result.payload =
          "Recording metadata payload exceeds contract capacity";
      esyslog(
          "suitebridge: svdrp command=RMETA result=overflow recording_key=%s",
          recordingKey.c_str());
      return result;
    }

    result.replyCode = 250;
    result.payload.assign(payload.Data(), payload.Size());
    isyslog(
        "suitebridge: svdrp command=RMETA result=served recording_key=%s found=%s reason=%s bytes=%zu people=%zu images=%zu",
        recordingKey.c_str(),
        metadata.found ? "true" : "false",
        SuiteBridgeRecordingMetadataReasonName(metadata.reason),
        payload.Size(),
        metadata.people.size(),
        metadata.images.size());
    return result;
  } catch (...) {
    result.replyCode = 451;
    result.payload = "Recording metadata payload serialization failed";
    esyslog(
        "suitebridge: svdrp command=RMETA result=serialization-failed recording_key=%s",
        recordingKey.c_str());
    return result;
  }
}
