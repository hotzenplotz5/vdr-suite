#ifndef VDR_SUITE_BRIDGE_RECORDING_METADATA_H
#define VDR_SUITE_BRIDGE_RECORDING_METADATA_H

#include "suitebridge_artwork_reference.h"

#include <cstddef>
#include <string>
#include <vector>

enum class SuiteBridgeRecordingMetadataReason {
  None,
  RecordingNotFound,
  IdentityAmbiguous,
  ProviderNoMatch,
};

enum class SuiteBridgeRecordingMediaType {
  None,
  Series,
  Movie,
};

enum class SuiteBridgeRecordingPersonRole {
  Unknown,
  Actor,
  Director,
  Writer,
  Producer,
  Moderator,
  Guest,
  Composer,
  Other,
};

enum class SuiteBridgeRecordingImageOrientation {
  Unknown,
  Landscape,
  Banner,
  Portrait,
};

struct SuiteBridgeRecordingPerson final {
  SuiteBridgeRecordingPersonRole role =
      SuiteBridgeRecordingPersonRole::Unknown;
  std::string name;
  std::string characterName;
  SuiteBridgeArtworkReference image;
};

struct SuiteBridgeRecordingImage final {
  SuiteBridgeRecordingImageOrientation orientation =
      SuiteBridgeRecordingImageOrientation::Unknown;
  SuiteBridgeArtworkReference artwork;
};

struct SuiteBridgeRecordingMetadata final {
  static constexpr std::size_t kMaxGenres = 12;
  static constexpr std::size_t kMaxCountries = 8;
  static constexpr std::size_t kMaxNetworks = 8;
  static constexpr std::size_t kMaxPeople = 12;
  static constexpr std::size_t kMaxImages = 8;

  bool found = false;
  SuiteBridgeRecordingMetadataReason reason =
      SuiteBridgeRecordingMetadataReason::None;
  std::string recordingKey;
  SuiteBridgeRecordingMediaType mediaType =
      SuiteBridgeRecordingMediaType::None;
  int providerId = 0;
  int seasonNumber = 0;
  int episodeNumber = 0;
  int absoluteEpisodeNumber = 0;
  int runtimeMinutes = 0;
  int durationDeviationMinutes = 0;
  int scraperHd = 0;
  int scraperLanguage = 0;
  float popularity = 0.0F;
  float voteAverage = 0.0F;
  int voteCount = 0;
  bool adult = false;
  int collectionId = 0;
  int lastSeason = 0;

  std::string title;
  std::string originalTitle;
  std::string episodeName;
  std::string tagline;
  std::string overview;
  std::string releaseDate;
  std::string firstAired;
  std::string imdbId;
  std::string status;
  std::string collectionName;

  std::vector<std::string> genres;
  std::vector<std::string> productionCountries;
  std::vector<std::string> networks;
  std::vector<SuiteBridgeRecordingPerson> people;
  SuiteBridgeArtworkReference preferredArtwork;
  std::vector<SuiteBridgeRecordingImage> images;
};

const char *SuiteBridgeRecordingMetadataReasonName(
    SuiteBridgeRecordingMetadataReason reason) noexcept;
const char *SuiteBridgeRecordingMediaTypeName(
    SuiteBridgeRecordingMediaType type) noexcept;
const char *SuiteBridgeRecordingPersonRoleName(
    SuiteBridgeRecordingPersonRole role) noexcept;
const char *SuiteBridgeRecordingImageOrientationName(
    SuiteBridgeRecordingImageOrientation orientation) noexcept;

#endif
