#ifndef VDR_SUITE_BRIDGE_EPG_METADATA_H
#define VDR_SUITE_BRIDGE_EPG_METADATA_H

#include <string>
#include <vector>

enum class SuiteBridgeEpgMediaType {
  None = 0,
  Series = 1,
  Movie = 2,
};

enum class SuiteBridgeEpgPersonRole {
  Unknown = 0,
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
  None = 0,
  Landscape,
  Banner,
  Portrait,
};

struct SuiteBridgeEpgImage {
  SuiteBridgeEpgImageOrientation orientation =
      SuiteBridgeEpgImageOrientation::None;
  std::string path;
  int width = 0;
  int height = 0;

  bool Valid() const noexcept
  {
    return orientation != SuiteBridgeEpgImageOrientation::None &&
        !path.empty() && width > 0 && height > 0;
  }
};

struct SuiteBridgeEpgPerson {
  SuiteBridgeEpgPersonRole role = SuiteBridgeEpgPersonRole::Unknown;
  std::string name;
  std::string characterName;
  SuiteBridgeEpgImage image;

  bool Valid() const noexcept
  {
    return !name.empty();
  }
};

struct SuiteBridgeEpgMetadata {
  bool found = false;
  SuiteBridgeEpgMediaType mediaType = SuiteBridgeEpgMediaType::None;
  int databaseId = 0;

  std::string title;
  std::string originalTitle;
  std::string episodeTitle;
  std::string tagline;
  std::string overview;
  std::string episodeOverview;
  std::string releaseDate;
  std::string firstAired;
  std::string imdbId;
  std::string collectionName;
  std::string status;

  int runtimeMinutes = 0;
  int collectionId = 0;
  int seasonNumber = 0;
  int episodeNumber = 0;
  int absoluteEpisodeNumber = 0;
  int lastSeason = 0;
  bool adult = false;
  float voteAverage = 0.0F;
  int voteCount = 0;

  std::vector<std::string> genres;
  std::vector<std::string> productionCountries;
  std::vector<std::string> networks;
  std::vector<SuiteBridgeEpgPerson> persons;
  std::vector<SuiteBridgeEpgImage> images;

  bool Valid() const noexcept
  {
    return found && mediaType != SuiteBridgeEpgMediaType::None;
  }
};

#endif
