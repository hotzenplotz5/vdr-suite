#ifndef VDR_SUITE_BRIDGE_EPG_METADATA_H
#define VDR_SUITE_BRIDGE_EPG_METADATA_H

#include "suitebridge_artwork_reference.h"

#include <cstddef>
#include <string>
#include <vector>

enum class SuiteBridgeEpgMediaType {
  None,
  Series,
  Movie,
};

enum class SuiteBridgeEpgPersonRole {
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

enum class SuiteBridgeEpgImageOrientation {
  Unknown,
  Landscape,
  Banner,
  Portrait,
};

enum class SuiteBridgeEpgExternalIdProvider {
  Unknown,
  Imdb,
  Tmdb,
  Tvdb,
};

enum class SuiteBridgeEpgExternalIdScope {
  Unknown,
  Series,
  Season,
  Episode,
  Movie,
};

struct SuiteBridgeEpgExternalId final {
  SuiteBridgeEpgExternalIdProvider provider =
      SuiteBridgeEpgExternalIdProvider::Unknown;
  SuiteBridgeEpgExternalIdScope scope =
      SuiteBridgeEpgExternalIdScope::Unknown;
  std::string value;

  bool Valid() const noexcept
  {
    return provider != SuiteBridgeEpgExternalIdProvider::Unknown &&
        scope != SuiteBridgeEpgExternalIdScope::Unknown && !value.empty();
  }
};

struct SuiteBridgeEpgPerson final {
  SuiteBridgeEpgPersonRole role = SuiteBridgeEpgPersonRole::Unknown;
  std::string name;
  std::string characterName;
  SuiteBridgeArtworkReference image;
};

struct SuiteBridgeEpgImage final {
  SuiteBridgeEpgImageOrientation orientation =
      SuiteBridgeEpgImageOrientation::Unknown;
  SuiteBridgeArtworkReference artwork;
};

struct SuiteBridgeEpgMetadata final {
  // Keep the complete JSON reply below the agent transport's 8 KiB bound.
  static constexpr std::size_t kMaxGenres = 12;
  static constexpr std::size_t kMaxCountries = 8;
  static constexpr std::size_t kMaxNetworks = 8;
  static constexpr std::size_t kMaxExternalIds = 8;
  static constexpr std::size_t kMaxPeople = 12;
  static constexpr std::size_t kMaxImages = 8;

  bool found = false;
  SuiteBridgeEpgMediaType mediaType = SuiteBridgeEpgMediaType::None;
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
  // Transitional compatibility field. Qualified identities are carried in
  // externalIds without changing the schema-1 legacy field.
  std::string imdbId;
  std::string status;
  std::string collectionName;

  std::vector<std::string> genres;
  std::vector<std::string> productionCountries;
  std::vector<std::string> networks;
  std::vector<SuiteBridgeEpgExternalId> externalIds;
  std::vector<SuiteBridgeEpgPerson> people;
  SuiteBridgeArtworkReference preferredArtwork;
  std::vector<SuiteBridgeEpgImage> images;
};

inline bool SuiteBridgeEpgMediaTypeIsResolved(
    SuiteBridgeEpgMediaType type) noexcept
{
  return type == SuiteBridgeEpgMediaType::Series ||
      type == SuiteBridgeEpgMediaType::Movie;
}

inline const char *SuiteBridgeEpgMediaTypeName(
    SuiteBridgeEpgMediaType type) noexcept
{
  switch (type) {
  case SuiteBridgeEpgMediaType::Series:
    return "series";
  case SuiteBridgeEpgMediaType::Movie:
    return "movie";
  case SuiteBridgeEpgMediaType::None:
    return "none";
  }

  return "none";
}

inline const char *SuiteBridgeEpgPersonRoleName(
    SuiteBridgeEpgPersonRole role) noexcept
{
  switch (role) {
  case SuiteBridgeEpgPersonRole::Actor:
    return "actor";
  case SuiteBridgeEpgPersonRole::Director:
    return "director";
  case SuiteBridgeEpgPersonRole::Writer:
    return "writer";
  case SuiteBridgeEpgPersonRole::Producer:
    return "producer";
  case SuiteBridgeEpgPersonRole::Moderator:
    return "moderator";
  case SuiteBridgeEpgPersonRole::Guest:
    return "guest";
  case SuiteBridgeEpgPersonRole::Composer:
    return "composer";
  case SuiteBridgeEpgPersonRole::Other:
    return "other";
  case SuiteBridgeEpgPersonRole::Unknown:
    return "unknown";
  }

  return "unknown";
}

inline const char *SuiteBridgeEpgImageOrientationName(
    SuiteBridgeEpgImageOrientation orientation) noexcept
{
  switch (orientation) {
  case SuiteBridgeEpgImageOrientation::Landscape:
    return "landscape";
  case SuiteBridgeEpgImageOrientation::Banner:
    return "banner";
  case SuiteBridgeEpgImageOrientation::Portrait:
    return "portrait";
  case SuiteBridgeEpgImageOrientation::Unknown:
    return "unknown";
  }

  return "unknown";
}

inline const char *SuiteBridgeEpgExternalIdProviderName(
    SuiteBridgeEpgExternalIdProvider provider) noexcept
{
  switch (provider) {
  case SuiteBridgeEpgExternalIdProvider::Imdb:
    return "imdb";
  case SuiteBridgeEpgExternalIdProvider::Tmdb:
    return "tmdb";
  case SuiteBridgeEpgExternalIdProvider::Tvdb:
    return "tvdb";
  case SuiteBridgeEpgExternalIdProvider::Unknown:
    return "unknown";
  }

  return "unknown";
}

inline const char *SuiteBridgeEpgExternalIdScopeName(
    SuiteBridgeEpgExternalIdScope scope) noexcept
{
  switch (scope) {
  case SuiteBridgeEpgExternalIdScope::Series:
    return "series";
  case SuiteBridgeEpgExternalIdScope::Season:
    return "season";
  case SuiteBridgeEpgExternalIdScope::Episode:
    return "episode";
  case SuiteBridgeEpgExternalIdScope::Movie:
    return "movie";
  case SuiteBridgeEpgExternalIdScope::Unknown:
    return "unknown";
  }

  return "unknown";
}

#endif
