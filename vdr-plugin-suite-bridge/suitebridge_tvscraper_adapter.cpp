#include "suitebridge_tvscraper_adapter.h"

#include "suitebridge_image_dimensions.h"
#include "services.h"

#include <vdr/epg.h>
#include <vdr/tools.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kMaximumTitleBytes = 256;
constexpr std::size_t kMaximumShortTextBytes = 512;
constexpr std::size_t kMaximumOverviewBytes = 2048;
constexpr std::size_t kMaximumListValueBytes = 128;
constexpr std::size_t kMaximumPersonNameBytes = 160;
constexpr std::size_t kMaximumCharacterNameBytes = 160;
constexpr std::size_t kMaximumListValues = 12;
constexpr std::size_t kMaximumPersons = 12;
constexpr int kMaximumImagesPerOrientation = 2;

std::string Bounded(std::string value, std::size_t maximumBytes)
{
  if (value.size() > maximumBytes) {
    value.resize(maximumBytes);
  }
  return value;
}

int NonNegative(int value) noexcept
{
  return std::max(0, value);
}

SuiteBridgeEpgMediaType MapMediaType(tvType type) noexcept
{
  switch (type) {
  case tSeries:
    return SuiteBridgeEpgMediaType::Series;
  case tMovie:
    return SuiteBridgeEpgMediaType::Movie;
  case tNone:
    break;
  }
  return SuiteBridgeEpgMediaType::None;
}

SuiteBridgeEpgPersonRole MapPersonRole(eCharacterType type) noexcept
{
  switch (type) {
  case eCharacterType::actor:
    return SuiteBridgeEpgPersonRole::Actor;
  case eCharacterType::director:
    return SuiteBridgeEpgPersonRole::Director;
  case eCharacterType::writer:
  case eCharacterType::screenplay:
    return SuiteBridgeEpgPersonRole::Writer;
  case eCharacterType::producer:
  case eCharacterType::executiveProducer:
  case eCharacterType::showrunner:
  case eCharacterType::creator:
    return SuiteBridgeEpgPersonRole::Producer;
  case eCharacterType::host:
    return SuiteBridgeEpgPersonRole::Moderator;
  case eCharacterType::guestStar:
  case eCharacterType::musicalGuest:
    return SuiteBridgeEpgPersonRole::Guest;
  case eCharacterType::originalMusicComposer:
    return SuiteBridgeEpgPersonRole::Composer;
  case eCharacterType::crew:
  case eCharacterType::others:
    return SuiteBridgeEpgPersonRole::Other;
  }
  return SuiteBridgeEpgPersonRole::Unknown;
}

SuiteBridgeEpgImage BuildImage(
    const cTvMedia &media,
    SuiteBridgeEpgImageOrientation orientation)
{
  SuiteBridgeEpgImage image;
  if (media.path.empty()) {
    return image;
  }

  SuiteBridgeImageDimensions actualDimensions;
  if (!SuiteBridgeReadImageDimensions(media.path, actualDimensions)) {
    return image;
  }

  image.orientation = orientation;
  image.path = media.path;
  image.width = actualDimensions.width;
  image.height = actualDimensions.height;
  return image;
}

void AppendBoundedValues(
    const std::vector<std::string> &source,
    std::vector<std::string> &target)
{
  for (const std::string &value : source) {
    if (target.size() >= kMaximumListValues) {
      return;
    }

    std::string bounded = Bounded(value, kMaximumListValueBytes);
    if (!bounded.empty()) {
      target.push_back(std::move(bounded));
    }
  }
}

void AppendImages(
    cScraperVideo &video,
    eOrientation sourceOrientation,
    SuiteBridgeEpgImageOrientation targetOrientation,
    std::set<std::string> &knownPaths,
    std::vector<SuiteBridgeEpgImage> &target)
{
  const std::vector<cTvMedia> media = video.getImages(
      sourceOrientation,
      kMaximumImagesPerOrientation,
      true);

  for (const cTvMedia &entry : media) {
    if (entry.path.empty() || knownPaths.count(entry.path) != 0) {
      continue;
    }

    SuiteBridgeEpgImage image = BuildImage(entry, targetOrientation);
    if (!image.Valid()) {
      continue;
    }

    knownPaths.insert(image.path);
    target.push_back(std::move(image));
  }
}

} // namespace

bool SuiteBridgeTvScraperAdapter::Available() noexcept
{
  return true;
}

SuiteBridgeArtworkReference
SuiteBridgeTvScraperAdapter::ResolvePreferredArtwork(
    const cEvent &event) const
{
  cGetScraperVideo request(&event, nullptr);
  cPlugin *scraper = request.call();

  if (!scraper) {
    isyslog(
        "suitebridge: tvscraper result=service-unavailable event=%u title=%s",
        event.EventID(),
        event.Title() ? event.Title() : "");
    return {};
  }

  if (!request.m_scraperVideo) {
    isyslog(
        "suitebridge: tvscraper result=no-video event=%u title=%s",
        event.EventID(),
        event.Title() ? event.Title() : "");
    return {};
  }

  const cTvMedia media = request.m_scraperVideo->getImage(
      cImageLevels(
          eImageLevel::episodeMovie,
          eImageLevel::seasonMovie,
          eImageLevel::tvShowCollection,
          eImageLevel::anySeasonCollection),
      cOrientations(
          eOrientation::landscape,
          eOrientation::banner,
          eOrientation::portrait),
      true);

  SuiteBridgeImageDimensions actualDimensions;
  if (!SuiteBridgeReadImageDimensions(media.path, actualDimensions)) {
    isyslog(
        "suitebridge: tvscraper result=invalid-artwork-file event=%u type=%d path=%s reported_width=%d reported_height=%d",
        event.EventID(),
        static_cast<int>(request.m_scraperVideo->getVideoType()),
        media.path.c_str(),
        media.width,
        media.height);
    return {};
  }

  SuiteBridgeArtworkReference reference;
  reference.provider = SuiteBridgeArtworkProvider::TvScraper;
  reference.path = media.path;
  reference.width = actualDimensions.width;
  reference.height = actualDimensions.height;

  isyslog(
      "suitebridge: tvscraper result=artwork event=%u type=%d path=%s reported_width=%d reported_height=%d actual_width=%d actual_height=%d",
      event.EventID(),
      static_cast<int>(request.m_scraperVideo->getVideoType()),
      media.path.c_str(),
      media.width,
      media.height,
      actualDimensions.width,
      actualDimensions.height);

  return reference;
}

SuiteBridgeEpgMetadata SuiteBridgeTvScraperAdapter::ResolveMetadata(
    const cEvent &event) const
{
  cGetScraperVideo request(&event, nullptr);
  cPlugin *scraper = request.call();

  if (!scraper || !request.m_scraperVideo) {
    isyslog(
        "suitebridge: tvscraper metadata result=%s event=%u title=%s",
        scraper ? "no-video" : "service-unavailable",
        event.EventID(),
        event.Title() ? event.Title() : "");
    return {};
  }

  cScraperVideo &video = *request.m_scraperVideo;
  SuiteBridgeEpgMetadata metadata;
  metadata.mediaType = MapMediaType(video.getVideoType());
  if (metadata.mediaType == SuiteBridgeEpgMediaType::None) {
    isyslog(
        "suitebridge: tvscraper metadata result=unsupported-type event=%u title=%s",
        event.EventID(),
        event.Title() ? event.Title() : "");
    return {};
  }

  metadata.found = true;
  metadata.databaseId = NonNegative(video.getDbId());
  metadata.seasonNumber = NonNegative(video.getSeasonNumber());
  metadata.episodeNumber = NonNegative(video.getEpisodeNumber());

  std::string overviewTitle;
  std::string overviewEpisodeTitle;
  std::string overviewReleaseDate;
  std::string overviewImdbId;
  std::string overviewCollectionName;
  int overviewRuntime = 0;
  int overviewCollectionId = 0;
  if (video.getOverview(
          &overviewTitle,
          &overviewEpisodeTitle,
          &overviewReleaseDate,
          &overviewRuntime,
          &overviewImdbId,
          &overviewCollectionId,
          &overviewCollectionName)) {
    metadata.title = Bounded(overviewTitle, kMaximumTitleBytes);
    metadata.episodeTitle = Bounded(
        overviewEpisodeTitle,
        kMaximumTitleBytes);
    metadata.releaseDate = Bounded(
        overviewReleaseDate,
        kMaximumShortTextBytes);
    metadata.runtimeMinutes = NonNegative(overviewRuntime);
    metadata.imdbId = Bounded(overviewImdbId, kMaximumShortTextBytes);
    metadata.collectionId = NonNegative(overviewCollectionId);
    metadata.collectionName = Bounded(
        overviewCollectionName,
        kMaximumTitleBytes);
  }

  std::string title;
  std::string originalTitle;
  std::string tagline;
  std::string overview;
  std::vector<std::string> genres;
  std::string homepage;
  std::string releaseDate;
  bool adult = false;
  int runtime = 0;
  float popularity = 0.0F;
  float voteAverage = 0.0F;
  int voteCount = 0;
  std::vector<std::string> productionCountries;
  std::string imdbId;
  int budget = 0;
  int revenue = 0;
  int collectionId = 0;
  std::string collectionName;
  std::string status;
  std::vector<std::string> networks;
  int lastSeason = 0;

  if (video.getMovieOrTv(
          &title,
          &originalTitle,
          &tagline,
          &overview,
          &genres,
          &homepage,
          &releaseDate,
          &adult,
          &runtime,
          &popularity,
          &voteAverage,
          &voteCount,
          &productionCountries,
          &imdbId,
          &budget,
          &revenue,
          &collectionId,
          &collectionName,
          &status,
          &networks,
          &lastSeason)) {
    if (!title.empty()) {
      metadata.title = Bounded(title, kMaximumTitleBytes);
    }
    metadata.originalTitle = Bounded(originalTitle, kMaximumTitleBytes);
    metadata.tagline = Bounded(tagline, kMaximumShortTextBytes);
    metadata.overview = Bounded(overview, kMaximumOverviewBytes);
    if (!releaseDate.empty()) {
      metadata.releaseDate = Bounded(releaseDate, kMaximumShortTextBytes);
    }
    if (runtime > 0) {
      metadata.runtimeMinutes = runtime;
    }
    if (!imdbId.empty()) {
      metadata.imdbId = Bounded(imdbId, kMaximumShortTextBytes);
    }
    if (collectionId > 0) {
      metadata.collectionId = collectionId;
    }
    if (!collectionName.empty()) {
      metadata.collectionName = Bounded(
          collectionName,
          kMaximumTitleBytes);
    }
    metadata.status = Bounded(status, kMaximumShortTextBytes);
    metadata.lastSeason = NonNegative(lastSeason);
    metadata.adult = adult;
    metadata.voteAverage = std::max(0.0F, voteAverage);
    metadata.voteCount = NonNegative(voteCount);
    AppendBoundedValues(genres, metadata.genres);
    AppendBoundedValues(
        productionCountries,
        metadata.productionCountries);
    AppendBoundedValues(networks, metadata.networks);
  }

  if (metadata.mediaType == SuiteBridgeEpgMediaType::Series &&
      metadata.episodeNumber > 0) {
    std::string episodeName;
    std::string episodeOverview;
    int absoluteNumber = 0;
    std::string firstAired;
    int episodeRuntime = 0;
    float episodeVoteAverage = 0.0F;
    int episodeVoteCount = 0;
    std::string episodeImdbId;

    if (video.getEpisode(
            &episodeName,
            &episodeOverview,
            &absoluteNumber,
            &firstAired,
            &episodeRuntime,
            &episodeVoteAverage,
            &episodeVoteCount,
            &episodeImdbId)) {
      if (!episodeName.empty()) {
        metadata.episodeTitle = Bounded(
            episodeName,
            kMaximumTitleBytes);
      }
      metadata.episodeOverview = Bounded(
          episodeOverview,
          kMaximumOverviewBytes);
      metadata.absoluteEpisodeNumber = NonNegative(absoluteNumber);
      metadata.firstAired = Bounded(
          firstAired,
          kMaximumShortTextBytes);
      if (episodeRuntime > 0) {
        metadata.runtimeMinutes = episodeRuntime;
      }
      if (episodeVoteAverage > 0.0F) {
        metadata.voteAverage = episodeVoteAverage;
      }
      if (episodeVoteCount > 0) {
        metadata.voteCount = episodeVoteCount;
      }
      if (!episodeImdbId.empty()) {
        metadata.imdbId = Bounded(
            episodeImdbId,
            kMaximumShortTextBytes);
      }
    }
  }

  std::vector<std::unique_ptr<cCharacter>> characters =
      video.getCharacters(true);
  for (const std::unique_ptr<cCharacter> &character : characters) {
    if (!character || metadata.persons.size() >= kMaximumPersons) {
      break;
    }

    SuiteBridgeEpgPerson person;
    person.role = MapPersonRole(character->getType());
    person.name = Bounded(
        character->getPersonName(),
        kMaximumPersonNameBytes);
    person.characterName = Bounded(
        character->getCharacterName(),
        kMaximumCharacterNameBytes);
    person.image = BuildImage(
        character->getImage(),
        SuiteBridgeEpgImageOrientation::Portrait);
    if (person.Valid()) {
      metadata.persons.push_back(std::move(person));
    }
  }

  std::set<std::string> knownImagePaths;
  AppendImages(
      video,
      eOrientation::landscape,
      SuiteBridgeEpgImageOrientation::Landscape,
      knownImagePaths,
      metadata.images);
  AppendImages(
      video,
      eOrientation::banner,
      SuiteBridgeEpgImageOrientation::Banner,
      knownImagePaths,
      metadata.images);
  AppendImages(
      video,
      eOrientation::portrait,
      SuiteBridgeEpgImageOrientation::Portrait,
      knownImagePaths,
      metadata.images);

  isyslog(
      "suitebridge: tvscraper metadata result=metadata event=%u type=%d db=%d persons=%zu images=%zu title=%s",
      event.EventID(),
      static_cast<int>(video.getVideoType()),
      metadata.databaseId,
      metadata.persons.size(),
      metadata.images.size(),
      metadata.title.c_str());

  return metadata;
}
