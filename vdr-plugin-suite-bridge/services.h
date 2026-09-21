#ifndef __TVSCRAPER_SERVICES_H
#define __TVSCRAPER_SERVICES_H

// TVScraper public VDR service contract.
// Kept locally like vdr-plugin-live so consumers do not depend on
// distribution-specific development header installation paths.
// Upstream: MarkusEh/vdr-plugin-tvscraper, services.h

#include <vdr/epg.h>
#include <vdr/plugin.h>
#include <vdr/recording.h>

#include <memory>
#include <string>
#include <vector>

enum tvType {
  tSeries,
  tMovie,
  tNone,
};

class cTvMedia {
public:
  cTvMedia(void)
      : path(),
        width(0),
        height(0)
  {
  }

  std::string path;
  int width;
  int height;
};

enum class eCharacterType {
  director = 1,
  writer = 2,
  actor = 3,
  guestStar = 4,
  crew = 5,
  creator = 6,
  producer = 7,
  showrunner = 8,
  musicalGuest = 9,
  host = 10,
  executiveProducer = 11,
  screenplay = 21,
  originalMusicComposer = 31,
  others = 51,
};

class cCharacter {
public:
  virtual eCharacterType getType() = 0;
  virtual const std::string &getPersonName() = 0;
  virtual const std::string &getCharacterName() = 0;
  virtual const cTvMedia &getImage() = 0;
  virtual ~cCharacter() {}
};

enum class eOrientation {
  none = 0,
  banner = 1,
  landscape = 2,
  portrait = 3,
};

class cOrientations {
public:
  cOrientations(
      eOrientation first = eOrientation::none,
      eOrientation second = eOrientation::none,
      eOrientation third = eOrientation::none)
      : m_orientations(
            static_cast<int>(first)
            | (static_cast<int>(second) << 3)
            | (static_cast<int>(third) << 6))
  {
  }

private:
  friend class cOrientationsInt;
  int m_orientations;
};

enum class eImageLevel {
  none = 0,
  episodeMovie = 1,
  seasonMovie = 2,
  tvShowCollection = 3,
  anySeasonCollection = 4,
};

class cImageLevels {
public:
  cImageLevels(
      eImageLevel first = eImageLevel::none,
      eImageLevel second = eImageLevel::none,
      eImageLevel third = eImageLevel::none,
      eImageLevel fourth = eImageLevel::none)
      : m_imageLevels(
            static_cast<int>(first)
            | (static_cast<int>(second) << 3)
            | (static_cast<int>(third) << 6)
            | (static_cast<int>(fourth) << 9))
  {
  }

private:
  friend class cImageLevelsInt;
  int m_imageLevels;
};

class cScraperVideo {
public:
  virtual tvType getVideoType() = 0;
  virtual int getDbId() = 0;
  virtual int getEpisodeNumber() = 0;
  virtual int getSeasonNumber() = 0;

  virtual bool getOverview(
      std::string *title,
      std::string *episodeName,
      std::string *releaseDate,
      int *runtime,
      std::string *imdbId,
      int *collectionId,
      std::string *collectionName = NULL) = 0;

  virtual cTvMedia getImage(
      cImageLevels imageLevels = cImageLevels(),
      cOrientations imageOrientations = cOrientations(),
      bool fullPath = true) = 0;

  virtual std::vector<cTvMedia> getImages(
      eOrientation orientation,
      int maxImages = 3,
      bool fullPath = true) = 0;

  virtual std::vector<std::unique_ptr<cCharacter>> getCharacters(
      bool fullPath = true) = 0;

  virtual int getDurationDeviation() = 0;
  virtual int getHD() = 0;
  virtual int getLanguage() = 0;

  virtual bool getMovieOrTv(
      std::string *title,
      std::string *originalTitle,
      std::string *tagline,
      std::string *overview,
      std::vector<std::string> *genres,
      std::string *homepage,
      std::string *releaseDate,
      bool *adult,
      int *runtime,
      float *popularity,
      float *voteAverage,
      int *voteCount,
      std::vector<std::string> *productionCountries,
      std::string *imdbId,
      int *budget,
      int *revenue,
      int *collectionId,
      std::string *collectionName,
      std::string *status,
      std::vector<std::string> *networks,
      int *lastSeason) = 0;

  virtual bool getEpisode(
      std::string *name,
      std::string *overview,
      int *absoluteNumber,
      std::string *firstAired,
      int *runtime,
      float *voteAverage,
      int *voteCount,
      std::string *imdbId) = 0;

  virtual ~cScraperVideo() {}
};

class cGetScraperVideo {
public:
  cGetScraperVideo(
      const cEvent *event = NULL,
      const cRecording *recording = NULL)
      : m_event(event),
        m_recording(recording)
  {
  }

  cPlugin *call(cPlugin *pScraper = NULL)
  {
    if (!pScraper) {
      return cPluginManager::CallFirstService("GetScraperVideo", this);
    }

    return pScraper->Service("GetScraperVideo", this)
        ? pScraper
        : NULL;
  }

  const cEvent *m_event;
  const cRecording *m_recording;
  std::unique_ptr<cScraperVideo> m_scraperVideo;
};

#endif // __TVSCRAPER_SERVICES_H
