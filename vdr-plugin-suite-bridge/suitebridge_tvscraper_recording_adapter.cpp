#include "suitebridge_tvscraper_recording_adapter.h"

#include "services.h"
#include "suitebridge_image_dimensions.h"

#include <vdr/recording.h>
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

SuiteBridgeRecordingMediaType ToMediaType(tvType type) noexcept
{
  switch (type) {
  case tSeries:
    return SuiteBridgeRecordingMediaType::Series;
  case tMovie:
    return SuiteBridgeRecordingMediaType::Movie;
  case tNone:
    return SuiteBridgeRecordingMediaType::None;
  }

  return SuiteBridgeRecordingMediaType::None;
}

SuiteBridgeRecordingPersonRole ToPersonRole(
    eCharacterType type) noexcept
{
  switch (type) {
  case eCharacterType::actor:
    return SuiteBridgeRecordingPersonRole::Actor;
  case eCharacterType::director:
    return SuiteBridgeRecordingPersonRole::Director;
  case eCharacterType::writer:
  case eCharacterType::screenplay:
    return SuiteBridgeRecordingPersonRole::Writer;
  case eCharacterType::producer:
  case eCharacterType::executiveProducer:
  case eCharacterType::showrunner:
  case eCharacterType::creator:
    return SuiteBridgeRecordingPersonRole::Producer;
  case eCharacterType::host:
    return SuiteBridgeRecordingPersonRole::Moderator;
  case eCharacterType::guestStar:
  case eCharacterType::musicalGuest:
    return SuiteBridgeRecordingPersonRole::Guest;
  case eCharacterType::originalMusicComposer:
    return SuiteBridgeRecordingPersonRole::Composer;
  case eCharacterType::crew:
  case eCharacterType::others:
    return SuiteBridgeRecordingPersonRole::Other;
  }

  return SuiteBridgeRecordingPersonRole::Unknown;
}

SuiteBridgeRecordingImageOrientation ToImageOrientation(
    eOrientation orientation) noexcept
{
  switch (orientation) {
  case eOrientation::landscape:
    return SuiteBridgeRecordingImageOrientation::Landscape;
  case eOrientation::banner:
    return SuiteBridgeRecordingImageOrientation::Banner;
  case eOrientation::portrait:
    return SuiteBridgeRecordingImageOrientation::Portrait;
  case eOrientation::none:
    return SuiteBridgeRecordingImageOrientation::Unknown;
  }

  return SuiteBridgeRecordingImageOrientation::Unknown;
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

bool HasArtworkPath(
    const SuiteBridgeRecordingMetadata &metadata,
    const std::string &path)
{
  if (path.empty()) {
    return true;
  }

  if (metadata.preferredArtwork.Valid() &&
      metadata.preferredArtwork.path == path) {
    return true;
  }

  for (const SuiteBridgeRecordingImage &image : metadata.images) {
    if (image.artwork.Valid() && image.artwork.path == path) {
      return true;
    }
  }

  return false;
}

void AppendImages(
    cScraperVideo &video,
    eOrientation orientation,
    SuiteBridgeRecordingMetadata &metadata)
{
  if (metadata.images.size() >= SuiteBridgeRecordingMetadata::kMaxImages) {
    return;
  }

  const int remaining = static_cast<int>(
      SuiteBridgeRecordingMetadata::kMaxImages - metadata.images.size());
  const std::vector<cTvMedia> media = video.getImages(
      orientation,
      std::min(3, remaining),
      true);

  for (const cTvMedia &item : media) {
    if (metadata.images.size() >= SuiteBridgeRecordingMetadata::kMaxImages) {
      break;
    }

    SuiteBridgeArtworkReference reference = ToArtworkReference(item);
    if (!reference.Valid() || HasArtworkPath(metadata, reference.path)) {
      continue;
    }

    SuiteBridgeRecordingImage image;
    image.orientation = ToImageOrientation(orientation);
    image.artwork = std::move(reference);
    metadata.images.push_back(std::move(image));
  }
}

SuiteBridgeRecordingMetadata MapMetadata(
    cScraperVideo &video,
    const std::string &recordingKey)
{
  SuiteBridgeRecordingMetadata metadata;
  metadata.recordingKey = recordingKey;
  metadata.found = true;
  metadata.mediaType = ToMediaType(video.getVideoType());
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

  if (metadata.mediaType == SuiteBridgeRecordingMediaType::Series) {
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

  LimitVector(metadata.genres, SuiteBridgeRecordingMetadata::kMaxGenres);
  LimitVector(
      metadata.productionCountries,
      SuiteBridgeRecordingMetadata::kMaxCountries);
  LimitVector(metadata.networks, SuiteBridgeRecordingMetadata::kMaxNetworks);

  metadata.preferredArtwork = ToArtworkReference(video.getImage(
      cImageLevels(
          eImageLevel::episodeMovie,
          eImageLevel::seasonMovie,
          eImageLevel::tvShowCollection,
          eImageLevel::anySeasonCollection),
      cOrientations(
          eOrientation::landscape,
          eOrientation::banner,
          eOrientation::portrait),
      true));

  std::vector<std::unique_ptr<cCharacter>> characters =
      video.getCharacters(true);
  for (const std::unique_ptr<cCharacter> &character : characters) {
    if (!character ||
        metadata.people.size() >= SuiteBridgeRecordingMetadata::kMaxPeople) {
      break;
    }

    SuiteBridgeRecordingPerson person;
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
  return metadata;
}

} // namespace

struct SuiteBridgeTvScraperRecordingSession::Impl final {
  State state = State::NotStarted;
  int recordingId = -1;
  std::unique_ptr<cScraperVideo> video;
};

SuiteBridgeTvScraperRecordingSession::SuiteBridgeTvScraperRecordingSession() noexcept = default;

SuiteBridgeTvScraperRecordingSession::SuiteBridgeTvScraperRecordingSession(
    std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl))
{
}

SuiteBridgeTvScraperRecordingSession::~SuiteBridgeTvScraperRecordingSession() = default;

SuiteBridgeTvScraperRecordingSession::SuiteBridgeTvScraperRecordingSession(
    SuiteBridgeTvScraperRecordingSession &&other) noexcept = default;

SuiteBridgeTvScraperRecordingSession &
SuiteBridgeTvScraperRecordingSession::operator=(
    SuiteBridgeTvScraperRecordingSession &&other) noexcept = default;

SuiteBridgeTvScraperRecordingSession::State
SuiteBridgeTvScraperRecordingSession::GetState() const noexcept
{
  return impl_ ? impl_->state : State::NotStarted;
}

SuiteBridgeRecordingMetadata
SuiteBridgeTvScraperRecordingSession::Resolve(
    const std::string &recordingKey) const
{
  if (!impl_ || impl_->state != State::Ready || !impl_->video) {
    return {};
  }

  SuiteBridgeRecordingMetadata metadata =
      MapMetadata(*impl_->video, recordingKey);

  isyslog(
      "suitebridge: tvscraper recording metadata result=resolved recording_id=%d type=%s provider_id=%d people=%zu images=%zu",
      impl_->recordingId,
      SuiteBridgeRecordingMediaTypeName(metadata.mediaType),
      metadata.providerId,
      metadata.people.size(),
      metadata.images.size());
  return metadata;
}

SuiteBridgeTvScraperRecordingSession
SuiteBridgeTvScraperRecordingAdapter::Start(
    const cRecording &recording) const
{
  auto impl = std::make_unique<SuiteBridgeTvScraperRecordingSession::Impl>();
  impl->recordingId = recording.Id();

  cGetScraperVideo request(nullptr, &recording);
  cPlugin *scraper = request.call();
  if (!scraper) {
    impl->state =
        SuiteBridgeTvScraperRecordingSession::State::ServiceUnavailable;
    isyslog(
        "suitebridge: tvscraper recording metadata result=service-unavailable recording_id=%d",
        impl->recordingId);
    return SuiteBridgeTvScraperRecordingSession(std::move(impl));
  }

  if (!request.m_scraperVideo ||
      request.m_scraperVideo->getVideoType() == tNone ||
      request.m_scraperVideo->getDbId() == 0) {
    impl->state =
        SuiteBridgeTvScraperRecordingSession::State::ProviderNoMatch;
    isyslog(
        "suitebridge: tvscraper recording metadata result=no-video recording_id=%d",
        impl->recordingId);
    return SuiteBridgeTvScraperRecordingSession(std::move(impl));
  }

  impl->video = std::move(request.m_scraperVideo);
  impl->state = SuiteBridgeTvScraperRecordingSession::State::Ready;
  return SuiteBridgeTvScraperRecordingSession(std::move(impl));
}
