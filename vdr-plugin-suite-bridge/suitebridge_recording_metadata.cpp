#include "suitebridge_recording_metadata.h"

const char *SuiteBridgeRecordingMetadataReasonName(
    SuiteBridgeRecordingMetadataReason reason) noexcept
{
  switch (reason) {
  case SuiteBridgeRecordingMetadataReason::RecordingNotFound:
    return "recording-not-found";
  case SuiteBridgeRecordingMetadataReason::IdentityAmbiguous:
    return "identity-ambiguous";
  case SuiteBridgeRecordingMetadataReason::ProviderNoMatch:
    return "provider-no-match";
  case SuiteBridgeRecordingMetadataReason::None:
    return "none";
  }
  return "none";
}

const char *SuiteBridgeRecordingMediaTypeName(
    SuiteBridgeRecordingMediaType type) noexcept
{
  switch (type) {
  case SuiteBridgeRecordingMediaType::Series:
    return "series";
  case SuiteBridgeRecordingMediaType::Movie:
    return "movie";
  case SuiteBridgeRecordingMediaType::None:
    return "none";
  }
  return "none";
}

const char *SuiteBridgeRecordingPersonRoleName(
    SuiteBridgeRecordingPersonRole role) noexcept
{
  switch (role) {
  case SuiteBridgeRecordingPersonRole::Actor:
    return "actor";
  case SuiteBridgeRecordingPersonRole::Director:
    return "director";
  case SuiteBridgeRecordingPersonRole::Writer:
    return "writer";
  case SuiteBridgeRecordingPersonRole::Producer:
    return "producer";
  case SuiteBridgeRecordingPersonRole::Moderator:
    return "moderator";
  case SuiteBridgeRecordingPersonRole::Guest:
    return "guest";
  case SuiteBridgeRecordingPersonRole::Composer:
    return "composer";
  case SuiteBridgeRecordingPersonRole::Other:
    return "other";
  case SuiteBridgeRecordingPersonRole::Unknown:
    return "unknown";
  }
  return "unknown";
}

const char *SuiteBridgeRecordingImageOrientationName(
    SuiteBridgeRecordingImageOrientation orientation) noexcept
{
  switch (orientation) {
  case SuiteBridgeRecordingImageOrientation::Landscape:
    return "landscape";
  case SuiteBridgeRecordingImageOrientation::Banner:
    return "banner";
  case SuiteBridgeRecordingImageOrientation::Portrait:
    return "portrait";
  case SuiteBridgeRecordingImageOrientation::Unknown:
    return "unknown";
  }
  return "unknown";
}
