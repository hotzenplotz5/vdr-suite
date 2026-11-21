#include "suitebridge_tvscraper_adapter.h"

#include "suitebridge_image_dimensions.h"
#include "services.h"

#include <vdr/epg.h>
#include <vdr/tools.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace {

SuiteBridgeArtworkReference ToArtworkReference(
    const cTvMedia &media)
{
  if (media.path.empty()) {
    return {};
  }

  SuiteBridgeImageDimensions actualDimensions;
  if (!SuiteBridgeReadImageDimensions(media.path, actualDimensions)) {
    return {};
  }

  SuiteBridgeArtworkReference reference;
  reference.provider = SuiteBridgeArtworkProvider::TvScraper;
  reference.path = media.path;
  reference.width = actualDimensions.width;
  reference.height = actualDimensions.height;
  return reference;
}

SuiteBridgeEpgMediaType ToMediaType(tvType type) noexcept
{
  switch (type) {
  case tSeries:
    return SuiteBridgeEpgMediaType::Series;
  case tMovie:
    return SuiteBridgeEpgMediaType::Movie;
  case tNone:
    return SuiteBridgeEpgMediaType::None;
  }

  return SuiteBridgeEpgMediaType::None;
}

SuiteBridgeEpgExternalIdScope ToExternalIdScope(
    SuiteBridgeEpgMediaType mediaType) noexcept
{
  switch (mediaType) {
  case SuiteBridgeEpgMediaType::Series:
    return SuiteBridgeEpgExternalIdScope::Series;
  case SuiteBridgeEpgMediaType::Movie:
    return SuiteBridgeEpgExternalIdScope::Movie;
  case SuiteBridgeEpgMediaType::None:
    return SuiteBridgeEpgExternalIdScope::Unknown;
  }

  return SuiteBridgeEpgExternalIdScope::Unknown;
}

SuiteBridgeEpgPersonRole ToPersonRole(
    eCharacterType type) noexcept
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

SuiteBridgeEpgImageOrientation ToImageOrientation(
    eOrientation orientation) noexcept
{
  switch (orientation) {
  case eOrientation::landscape:
    return SuiteBridgeEpgImageOrientation::Landscape;
  case eOrientation::banner:
    return SuiteBridgeEpgImageOrientation::Banner;
  case eOrientation::portrait:
    return SuiteBridgeEpgImageOrientation::Portrait;
  case eOrientation::none:
    return SuiteBridgeEpgImageOrientation::Unknown;
  }

  return SuiteBridgeEpgImageOrientation::Unknown;
}

cTvMedia PreferredArtwork(cScraperVideo &video)
{
  if (video.getVideoType() == tSeries) {
    return video.getImage(
        cImageLevels(
            eImageLevel::seasonMovie,
            eImageLevel::tvShowCollection,
            eImageLevel::anySeasonCollection,
            eImageLevel::episodeMovie),
        cOrientations(
            eOrientation::portrait,
            eOrientation::landscape,
            eOrientation::banner),
        true);
  }

  return video.getImage(
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
}

template <typename T>
void LimitVector(std::vector<T> &values, std::size_t limit)
{
  if (values.size() > limit) {
    values.resize(limit);
  }
}

void AssignIfNotEmpty(
    std::string &target,
    const std::string &value)
{
  if (!value.empty()) {
    target = value;
  }
}

void AppendExternalId(
    SuiteBridgeEpgMetadata &metadata,
    SuiteBridgeEpgExternalIdProvider provider,
    SuiteBridgeEpgExternalIdScope scope,
    const std::string &value)
{
  if (value.empty() ||
      metadata.externalIds.size() >= SuiteBridgeEpgMetadata::kMaxExternalIds) {
    return;
  }

  SuiteBridgeEpgExternalId externalId;
  externalId.provider = provider;
  externalId.scope = scope;
  externalId.value = value;
  if (!externalId.Valid()) {
    return;
  }

  for (const SuiteBridgeEpgExternalId &existing : metadata.externalIds) {
    if (existing.provider == externalId.provider &&
        existing.scope == externalId.scope &&
        existing.value == externalId.value) {
      return;
    }
  }

  metadata.externalIds.push_back(std::move(externalId));
}

bool HasArtworkPath(
    const SuiteBridgeEpgMetadata &metadata,
    const std::string &path)
{
  if (path.empty()) {
    return true;
  }

  if (metadata.preferredArtwork.Valid() &&
      metadata.preferredArtwork.path == path) {
    return true;
  }

  for (const SuiteBridgeEpgImage &image : metadata.images) {
    if (image.artwork.Valid() && image.artwork.path == path) {
      return true;
    }
  }

  return false;
}

void AppendImages(
    cScraperVideo &video,
    eOrientation orientation,
    SuiteBridgeEpgMetadata &metadata)
{
  if (metadata.images.size() >= SuiteBridgeEpgMetadata::kMaxImages) {
    return;
  }

  const int remaining = static_cast<int>(
      SuiteBridgeEpgMetadata::kMaxImages - metadata.images.size());
  const std::vector<cTvMedia> media = video.getImages(
      orientation,
      std::min(3, remaining),
      true);

  for (const cTvMedia &item : media) {
    if (metadata.images.size() >= SuiteBridgeEpgMetadata::kMaxImages) {
      break;
    }

    SuiteBridgeArtworkReference reference = ToArtworkReference(item);
    if (!reference.Valid() || HasArtworkPath(metadata, reference.path)) {
      continue;
    }

    SuiteBridgeEpgImage image;
    image.orientation = ToImageOrientation(orientation);
    image.artwork = std::move(reference);
    metadata.images.push_back(std::move(image));
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

  const cTvMedia media = PreferredArtwork(*request.m_scraperVideo);

  const SuiteBridgeArtworkReference reference = ToArtworkReference(media);
  if (!reference.Valid()) {
    isyslog(
        "suitebridge: tvscraper result=invalid-artwork-file event=%u type=%d path=%s reported_width=%d reported_height=%d",
        event.EventID(),
        static_cast<int>(request.m_scraperVideo->getVideoType()),
        media.path.c_str(),
        media.width,
        media.height);
    return {};
  }

  isyslog(
      "suitebridge: tvscraper result=artwork event=%u type=%d path=%s reported_width=%d reported_height=%d actual_width=%d actual_height=%d",
      event.EventID(),
      static_cast<int>(request.m_scraperVideo->getVideoType()),
      media.path.c_str(),
      media.width,
      media.height,
      reference.width,
      reference.height);

  return reference;
}

SuiteBridgeEpgMetadata
SuiteBridgeTvScraperAdapter::ResolveMetadata(
    const cEvent &event) const
{
  cGetScraperVideo request(&event, nullptr);
  cPlugin *scraper = request.call();

  if (!scraper) {
    isyslog(
        "suitebridge: tvscraper metadata result=service-unavailable event=%u title=%s",
        event.EventID(),
        event.Title() ? event.Title() : "");
    return {};
  }

  if (!request.m_scraperVideo) {
    isyslog(
        "suitebridge: tvscraper metadata result=no-video event=%u title=%s",
        event.EventID(),
        event.Title() ? event.Title() : "");
    return {};
  }

  cScraperVideo &video = *request.m_scraperVideo;
  SuiteBridgeEpgMetadata metadata;
  metadata.mediaType = ToMediaType(video.getVideoType());
  if (!SuiteBridgeEpgMediaTypeIsResolved(metadata.mediaType)) {
    isyslog(
        "suitebridge: tvscraper metadata result=no-resolved-video event=%u type=%s provider_id=%d title=%s",
        event.EventID(),
        SuiteBridgeEpgMediaTypeName(metadata.mediaType),
        video.getDbId(),
        event.Title() ? event.Title() : "");
    return {};
  }

  metadata.found = true;
  metadata.providerId = video.getDbId();
  metadata.seasonNumber = video.getSeasonNumber();
  metadata.episodeNumber = video.getEpisodeNumber();
  metadata.durationDeviationMinutes = video.getDurationDeviation();
  metadata.scraperHd = video.getHD();
  metadata.scraperLanguage = video.getLanguage();

  std::string overviewTitle;
  std::string overviewEpisodeName;
  std::string overviewReleaseDate;
  std::string overviewImdbId;
  std::string overviewCollectionName;
  int overviewRuntime = 0;
  int overviewCollectionId = 0;

  if (video.getOverview(
          &overviewTitle,
          &overviewEpisodeName,
          &overviewReleaseDate,
          &overviewRuntime,
          &overviewImdbId,
          &overviewCollectionId,
          &overviewCollectionName)) {
    AssignIfNotEmpty(metadata.title, overviewTitle);
    AssignIfNotEmpty(metadata.episodeName, overviewEpisodeName);
    AssignIfNotEmpty(metadata.releaseDate, overviewReleaseDate);
    AssignIfNotEmpty(metadata.imdbId, overviewImdbId);
    if (metadata.mediaType == SuiteBridgeEpgMediaType::Movie) {
      AppendExternalId(
          metadata,
          SuiteBridgeEpgExternalIdProvider::Imdb,
          SuiteBridgeEpgExternalIdScope::Movie,
          overviewImdbId);
    }
    AssignIfNotEmpty(metadata.collectionName, overviewCollectionName);
    if (overviewRuntime > 0) {
      metadata.runtimeMinutes = overviewRuntime;
    }
    if (overviewCollectionId > 0) {
      metadata.collectionId = overviewCollectionId;
    }
  }

  std::string detailedTitle;
  std::string detailedOriginalTitle;
  std::string detailedTagline;
  std::string detailedOverview;
  std::vector<std::string> detailedGenres;
  std::string homepage;
  std::string detailedReleaseDate;
  bool adult = false;
  int detailedRuntime = 0;
  float popularity = 0.0F;
  float voteAverage = 0.0F;
  int voteCount = 0;
  std::vector<std::string> productionCountries;
  std::string detailedImdbId;
  int budget = 0;
  int revenue = 0;
  int collectionId = 0;
  std::string collectionName;
  std::string status;
  std::vector<std::string> networks;
  int lastSeason = 0;

  if (video.getMovieOrTv(
          &detailedTitle,
          &detailedOriginalTitle,
          &detailedTagline,
          &detailedOverview,
          &detailedGenres,
          &homepage,
          &detailedReleaseDate,
          &adult,
          &detailedRuntime,
          &popularity,
          &voteAverage,
          &voteCount,
          &productionCountries,
          &detailedImdbId,
          &budget,
          &revenue,
          &collectionId,
          &collectionName,
          &status,
          &networks,
          &lastSeason)) {
    AssignIfNotEmpty(metadata.title, detailedTitle);
    AssignIfNotEmpty(metadata.originalTitle, detailedOriginalTitle);
    AssignIfNotEmpty(metadata.tagline, detailedTagline);
    AssignIfNotEmpty(metadata.overview, detailedOverview);
    AssignIfNotEmpty(metadata.releaseDate, detailedReleaseDate);
    AssignIfNotEmpty(metadata.imdbId, detailedImdbId);
    AppendExternalId(
        metadata,
        SuiteBridgeEpgExternalIdProvider::Imdb,
        ToExternalIdScope(metadata.mediaType),
        detailedImdbId);
    AssignIfNotEmpty(metadata.collectionName, collectionName);
    AssignIfNotEmpty(metadata.status, status);
    metadata.genres = std::move(detailedGenres);
    metadata.productionCountries = std::move(productionCountries);
    metadata.networks = std::move(networks);
    metadata.adult = adult;
    metadata.popularity = popularity;
    metadata.voteAverage = voteAverage;
    metadata.voteCount = voteCount;
    metadata.lastSeason = lastSeason;
    if (detailedRuntime > 0) {
      metadata.runtimeMinutes = detailedRuntime;
    }
    if (collectionId > 0) {
      metadata.collectionId = collectionId;
    }
  }

  if (metadata.mediaType == SuiteBridgeEpgMediaType::Series) {
    std::string episodeName;
    std::string episodeOverview;
    int absoluteEpisodeNumber = 0;
    std::string firstAired;
    int episodeRuntime = 0;
    float episodeVoteAverage = 0.0F;
    int episodeVoteCount = 0;
    std::string episodeImdbId;

    if (video.getEpisode(
            &episodeName,
            &episodeOverview,
            &absoluteEpisodeNumber,
            &firstAired,
            &episodeRuntime,
            &episodeVoteAverage,
            &episodeVoteCount,
            &episodeImdbId)) {
      AssignIfNotEmpty(metadata.episodeName, episodeName);
      AssignIfNotEmpty(metadata.overview, episodeOverview);
      AssignIfNotEmpty(metadata.firstAired, firstAired);
      AssignIfNotEmpty(metadata.imdbId, episodeImdbId);
      AppendExternalId(
          metadata,
          SuiteBridgeEpgExternalIdProvider::Imdb,
          SuiteBridgeEpgExternalIdScope::Episode,
          episodeImdbId);
      metadata.absoluteEpisodeNumber = absoluteEpisodeNumber;
      if (episodeRuntime > 0) {
        metadata.runtimeMinutes = episodeRuntime;
      }
      if (episodeVoteAverage > 0.0F) {
        metadata.voteAverage = episodeVoteAverage;
      }
      if (episodeVoteCount > 0) {
        metadata.voteCount = episodeVoteCount;
      }
    }
  }

  LimitVector(metadata.genres, SuiteBridgeEpgMetadata::kMaxGenres);
  LimitVector(
      metadata.productionCountries,
      SuiteBridgeEpgMetadata::kMaxCountries);
  LimitVector(metadata.networks, SuiteBridgeEpgMetadata::kMaxNetworks);

  metadata.preferredArtwork = ToArtworkReference(PreferredArtwork(video));

  std::vector<std::unique_ptr<cCharacter>> characters =
      video.getCharacters(true);
  for (const std::unique_ptr<cCharacter> &character : characters) {
    if (!character ||
        metadata.people.size() >= SuiteBridgeEpgMetadata::kMaxPeople) {
      break;
    }

    SuiteBridgeEpgPerson person;
    person.role = ToPersonRole(character->getType());
    person.name = character->getPersonName();
    person.characterName = character->getCharacterName();
    person.image = ToArtworkReference(character->getImage());

    if (!person.name.empty()) {
      metadata.people.push_back(std::move(person));
    }
  }

  AppendImages(video, eOrientation::landscape, metadata);
  AppendImages(video, eOrientation::banner, metadata);
  AppendImages(video, eOrientation::portrait, metadata);

  isyslog(
      "suitebridge: tvscraper metadata result=resolved event=%u type=%s provider_id=%d people=%zu images=%zu",
      event.EventID(),
      SuiteBridgeEpgMediaTypeName(metadata.mediaType),
      metadata.providerId,
      metadata.people.size(),
      metadata.images.size());

  return metadata;
}
