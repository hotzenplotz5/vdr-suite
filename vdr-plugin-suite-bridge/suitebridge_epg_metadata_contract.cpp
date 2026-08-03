#include "suitebridge_epg_metadata_contract.h"

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
  for (unsigned char character : value) {
    switch (character) {
    case '"':
      escaped += "\\\"";
      break;
    case '\\':
      escaped += "\\\\";
      break;
    case '\b':
      escaped += "\\b";
      break;
    case '\f':
      escaped += "\\f";
      break;
    case '\n':
      escaped += "\\n";
      break;
    case '\r':
      escaped += "\\r";
      break;
    case '\t':
      escaped += "\\t";
      break;
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

bool ExternalIdsValid(
    const std::vector<SuiteBridgeEpgExternalId> &externalIds)
{
  if (externalIds.size() > SuiteBridgeEpgMetadata::kMaxExternalIds) {
    return false;
  }

  for (std::size_t index = 0; index < externalIds.size(); ++index) {
    const SuiteBridgeEpgExternalId &externalId = externalIds[index];
    if (!externalId.Valid() || externalId.value.size() > 128) {
      return false;
    }

    for (std::size_t other = 0; other < index; ++other) {
      const SuiteBridgeEpgExternalId &candidate = externalIds[other];
      if (candidate.provider == externalId.provider &&
          candidate.scope == externalId.scope &&
          candidate.value == externalId.value) {
        return false;
      }
    }
  }

  return true;
}

void AppendExternalIds(
    std::ostringstream &stream,
    const std::vector<SuiteBridgeEpgExternalId> &externalIds)
{
  stream << '[';
  bool first = true;
  for (const SuiteBridgeEpgExternalId &externalId : externalIds) {
    if (!first) {
      stream << ',';
    }
    first = false;
    stream << "{\"provider\":\""
           << SuiteBridgeEpgExternalIdProviderName(externalId.provider)
           << "\",\"scope\":\""
           << SuiteBridgeEpgExternalIdScopeName(externalId.scope)
           << "\",\"value\":\"" << EscapeJson(externalId.value)
           << "\"}";
  }
  stream << ']';
}

void AppendArtwork(
    std::ostringstream &stream,
    const SuiteBridgeArtworkReference &artwork)
{
  stream << "{\"available\":"
         << (artwork.Valid() ? "true" : "false")
         << ",\"provider\":\""
         << (artwork.Valid() ? "tvscraper" : "none")
         << "\",\"origin\":\""
         << (artwork.Valid() ? "primary-metadata" : "none")
         << "\",\"path\":\"" << EscapeJson(artwork.path)
         << "\",\"width\":" << (artwork.Valid() ? artwork.width : 0)
         << ",\"height\":" << (artwork.Valid() ? artwork.height : 0)
         << '}';
}

} // namespace

SuiteBridgeEpgMetadataRequest::SuiteBridgeEpgMetadataRequest(
    const char *command,
    const char *option)
{
  handled_ = EqualsIgnoreCase(command, "META");
  if (!handled_ || !option) {
    return;
  }

  const char *cursor = option;
  while (*cursor && std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }

  const char *channelStart = cursor;
  while (*cursor && !std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }
  channelId_.assign(
      channelStart,
      static_cast<std::size_t>(cursor - channelStart));

  while (*cursor && std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }

  if (channelId_.empty() ||
      !std::isdigit(static_cast<unsigned char>(*cursor))) {
    return;
  }

  unsigned long parsedEventId = 0;
  while (std::isdigit(static_cast<unsigned char>(*cursor))) {
    parsedEventId = parsedEventId * 10UL
        + static_cast<unsigned long>(*cursor - '0');
    if (parsedEventId > 0xffffffffUL) {
      return;
    }
    ++cursor;
  }

  while (*cursor && std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }

  if (*cursor != '\0' || parsedEventId == 0) {
    return;
  }

  eventId_ = static_cast<unsigned int>(parsedEventId);
  valid_ = true;
}

bool SuiteBridgeEpgMetadataRequest::Handled() const noexcept
{
  return handled_;
}

bool SuiteBridgeEpgMetadataRequest::Valid() const noexcept
{
  return valid_;
}

const std::string &SuiteBridgeEpgMetadataRequest::ChannelId() const noexcept
{
  return channelId_;
}

unsigned int SuiteBridgeEpgMetadataRequest::EventId() const noexcept
{
  return eventId_;
}

SuiteBridgeEpgMetadataPayload::SuiteBridgeEpgMetadataPayload(
    const SuiteBridgeEpgMetadata &metadata)
{
  const bool resolved = SuiteBridgeEpgMediaTypeIsResolved(metadata.mediaType);
  if (metadata.found != resolved ||
      (resolved && !ExternalIdsValid(metadata.externalIds))) {
    *this = SuiteBridgeEpgMetadataPayload(SuiteBridgeEpgMetadata{});
    return;
  }

  std::ostringstream stream;
  stream << "{\"schema\":1,\"found\":"
         << (metadata.found ? "true" : "false")
         << ",\"provider\":\""
         << (metadata.found ? "tvscraper" : "none")
         << "\",\"mediaType\":\""
         << SuiteBridgeEpgMediaTypeName(metadata.mediaType)
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
         << "\",\"externalIds\":";

  AppendExternalIds(stream, metadata.externalIds);
  stream << ",\"status\":\"" << EscapeJson(metadata.status)
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
  for (const SuiteBridgeEpgPerson &person : metadata.people) {
    if (!firstPerson) {
      stream << ',';
    }
    firstPerson = false;
    stream << "{\"role\":\""
           << SuiteBridgeEpgPersonRoleName(person.role)
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
  for (const SuiteBridgeEpgImage &image : metadata.images) {
    if (!firstImage) {
      stream << ',';
    }
    firstImage = false;
    stream << "{\"orientation\":\""
           << SuiteBridgeEpgImageOrientationName(image.orientation)
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

const char *SuiteBridgeEpgMetadataPayload::Data() const noexcept
{
  return data_.data();
}

std::size_t SuiteBridgeEpgMetadataPayload::Size() const noexcept
{
  return size_;
}

bool SuiteBridgeEpgMetadataPayload::Complete() const noexcept
{
  return complete_;
}
