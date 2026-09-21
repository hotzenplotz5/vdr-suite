#include "suitebridge_recording_metadata_contract.h"

#include "suitebridge_recording_identity.h"

#include <cctype>
#include <cstring>
#include <sstream>

namespace {

bool EqualsIgnoreCase(const char *left, const char *right) noexcept
{
  if (!left || !right) {
    return false;
  }

  while (*left && *right) {
    if (std::toupper(static_cast<unsigned char>(*left)) !=
        std::toupper(static_cast<unsigned char>(*right))) {
      return false;
    }
    ++left;
    ++right;
  }

  return *left == '\0' && *right == '\0';
}

std::string EscapeJson(const std::string &value)
{
  std::string escaped;
  escaped.reserve(value.size());
  static const char hex[] = "0123456789abcdef";

  for (const unsigned char character : value) {
    switch (character) {
    case '"': escaped += "\\\""; break;
    case '\\': escaped += "\\\\"; break;
    case '\b': escaped += "\\b"; break;
    case '\f': escaped += "\\f"; break;
    case '\n': escaped += "\\n"; break;
    case '\r': escaped += "\\r"; break;
    case '\t': escaped += "\\t"; break;
    default:
      if (character < 0x20) {
        escaped += "\\u00";
        escaped += hex[(character >> 4) & 0x0f];
        escaped += hex[character & 0x0f];
      } else {
        escaped += static_cast<char>(character);
      }
      break;
    }
  }

  return escaped;
}

void AppendStringArray(
    std::ostringstream &stream,
    const std::vector<std::string> &values)
{
  stream << '[';
  bool first = true;
  for (const std::string &value : values) {
    if (!first) {
      stream << ',';
    }
    first = false;
    stream << '"' << EscapeJson(value) << '"';
  }
  stream << ']';
}

bool MetadataValid(const SuiteBridgeRecordingMetadata &metadata) noexcept
{
  if (!SuiteBridgeRecordingIdentity::IsValidKey(metadata.recordingKey) ||
      metadata.genres.size() > SuiteBridgeRecordingMetadata::kMaxGenres ||
      metadata.productionCountries.size() >
          SuiteBridgeRecordingMetadata::kMaxCountries ||
      metadata.networks.size() > SuiteBridgeRecordingMetadata::kMaxNetworks ||
      metadata.people.size() > SuiteBridgeRecordingMetadata::kMaxPeople ||
      metadata.images.size() > SuiteBridgeRecordingMetadata::kMaxImages) {
    return false;
  }

  if (!metadata.found) {
    return metadata.reason != SuiteBridgeRecordingMetadataReason::None;
  }

  if (metadata.reason != SuiteBridgeRecordingMetadataReason::None ||
      metadata.mediaType == SuiteBridgeRecordingMediaType::None ||
      metadata.providerId == 0) {
    return false;
  }

  for (const SuiteBridgeRecordingPerson &person : metadata.people) {
    if (person.name.empty()) {
      return false;
    }
  }

  for (const SuiteBridgeRecordingImage &image : metadata.images) {
    if (image.orientation == SuiteBridgeRecordingImageOrientation::Unknown ||
        !image.artwork.Valid()) {
      return false;
    }
  }

  return true;
}

void AppendArtwork(
    std::ostringstream &stream,
    const SuiteBridgeArtworkReference &artwork)
{
  stream << "{\"available\":"
         << (artwork.Valid() ? "true" : "false")
         << ",\"provider\":\""
         << (artwork.Valid() ? "tvscraper" : "none")
         << "\",\"path\":\"" << EscapeJson(artwork.path)
         << "\",\"width\":" << (artwork.Valid() ? artwork.width : 0)
         << ",\"height\":" << (artwork.Valid() ? artwork.height : 0)
         << '}';
}

} // namespace

SuiteBridgeRecordingMetadataRequest::SuiteBridgeRecordingMetadataRequest(
    const char *command,
    const char *option)
{
  handled_ = EqualsIgnoreCase(command, "RMETA");
  if (!handled_ || !option) {
    return;
  }

  const char *cursor = option;
  while (*cursor && std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }

  const char *start = cursor;
  while (*cursor && !std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }
  recordingKey_.assign(start, static_cast<std::size_t>(cursor - start));

  while (*cursor && std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }

  valid_ = *cursor == '\0' &&
      SuiteBridgeRecordingIdentity::IsValidKey(recordingKey_);
}

bool SuiteBridgeRecordingMetadataRequest::Handled() const noexcept
{
  return handled_;
}

bool SuiteBridgeRecordingMetadataRequest::Valid() const noexcept
{
  return valid_;
}

const std::string &SuiteBridgeRecordingMetadataRequest::RecordingKey() const noexcept
{
  return recordingKey_;
}

SuiteBridgeRecordingMetadataPayload::SuiteBridgeRecordingMetadataPayload(
    const SuiteBridgeRecordingMetadata &metadata)
{
  if (!MetadataValid(metadata)) {
    return;
  }

  std::ostringstream stream;
  stream << "{\"schema\":1,\"found\":"
         << (metadata.found ? "true" : "false")
         << ",\"reason\":\""
         << SuiteBridgeRecordingMetadataReasonName(metadata.reason)
         << "\",\"provider\":\""
         << (metadata.found ? "tvscraper" : "none")
         << "\",\"recordingIdentitySchema\":1"
         << ",\"recordingKey\":\"" << EscapeJson(metadata.recordingKey)
         << "\",\"mediaType\":\""
         << SuiteBridgeRecordingMediaTypeName(metadata.mediaType)
         << "\",\"providerId\":" << metadata.providerId
         << ",\"seasonNumber\":" << metadata.seasonNumber
         << ",\"episodeNumber\":" << metadata.episodeNumber
         << ",\"absoluteEpisodeNumber\":"
         << metadata.absoluteEpisodeNumber
         << ",\"runtimeMinutes\":" << metadata.runtimeMinutes
         << ",\"durationDeviationMinutes\":"
         << metadata.durationDeviationMinutes
         << ",\"scraperHd\":" << metadata.scraperHd
         << ",\"scraperLanguage\":" << metadata.scraperLanguage
         << ",\"popularity\":" << metadata.popularity
         << ",\"voteAverage\":" << metadata.voteAverage
         << ",\"voteCount\":" << metadata.voteCount
         << ",\"adult\":" << (metadata.adult ? "true" : "false")
         << ",\"collectionId\":" << metadata.collectionId
         << ",\"lastSeason\":" << metadata.lastSeason
         << ",\"title\":\"" << EscapeJson(metadata.title)
         << "\",\"originalTitle\":\""
         << EscapeJson(metadata.originalTitle)
         << "\",\"episodeName\":\""
         << EscapeJson(metadata.episodeName)
         << "\",\"tagline\":\"" << EscapeJson(metadata.tagline)
         << "\",\"overview\":\"" << EscapeJson(metadata.overview)
         << "\",\"releaseDate\":\""
         << EscapeJson(metadata.releaseDate)
         << "\",\"firstAired\":\""
         << EscapeJson(metadata.firstAired)
         << "\",\"imdbId\":\"" << EscapeJson(metadata.imdbId)
         << "\",\"status\":\"" << EscapeJson(metadata.status)
         << "\",\"collectionName\":\""
         << EscapeJson(metadata.collectionName)
         << "\",\"genres\":";

  AppendStringArray(stream, metadata.genres);
  stream << ",\"productionCountries\":";
  AppendStringArray(stream, metadata.productionCountries);
  stream << ",\"networks\":";
  AppendStringArray(stream, metadata.networks);
  stream << ",\"preferredArtwork\":";
  AppendArtwork(stream, metadata.preferredArtwork);

  stream << ",\"people\":[";
  bool firstPerson = true;
  for (const SuiteBridgeRecordingPerson &person : metadata.people) {
    if (!firstPerson) {
      stream << ',';
    }
    firstPerson = false;
    stream << "{\"role\":\""
           << SuiteBridgeRecordingPersonRoleName(person.role)
           << "\",\"name\":\"" << EscapeJson(person.name)
           << "\",\"characterName\":\""
           << EscapeJson(person.characterName)
           << "\",\"image\":";
    AppendArtwork(stream, person.image);
    stream << '}';
  }
  stream << ']';

  stream << ",\"images\":[";
  bool firstImage = true;
  for (const SuiteBridgeRecordingImage &image : metadata.images) {
    if (!firstImage) {
      stream << ',';
    }
    firstImage = false;
    stream << "{\"orientation\":\""
           << SuiteBridgeRecordingImageOrientationName(image.orientation)
           << "\",\"artwork\":";
    AppendArtwork(stream, image.artwork);
    stream << '}';
  }
  stream << "]}";

  const std::string payload = stream.str();
  size_ = payload.size();
  complete_ = size_ < data_.size();
  if (!complete_) {
    size_ = data_.size() - 1;
  }

  std::memcpy(data_.data(), payload.data(), size_);
  data_[size_] = '\0';
}

const char *SuiteBridgeRecordingMetadataPayload::Data() const noexcept
{
  return data_.data();
}

std::size_t SuiteBridgeRecordingMetadataPayload::Size() const noexcept
{
  return size_;
}

bool SuiteBridgeRecordingMetadataPayload::Complete() const noexcept
{
  return complete_;
}
