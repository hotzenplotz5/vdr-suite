#include "suitebridge_epg_metadata_contract.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

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

const char *MediaTypeName(SuiteBridgeEpgMediaType type) noexcept
{
  switch (type) {
  case SuiteBridgeEpgMediaType::Series:
    return "series";
  case SuiteBridgeEpgMediaType::Movie:
    return "movie";
  case SuiteBridgeEpgMediaType::None:
    break;
  }
  return "none";
}

const char *PersonRoleName(SuiteBridgeEpgPersonRole role) noexcept
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
    break;
  }
  return "unknown";
}

const char *OrientationName(
    SuiteBridgeEpgImageOrientation orientation) noexcept
{
  switch (orientation) {
  case SuiteBridgeEpgImageOrientation::Landscape:
    return "landscape";
  case SuiteBridgeEpgImageOrientation::Banner:
    return "banner";
  case SuiteBridgeEpgImageOrientation::Portrait:
    return "portrait";
  case SuiteBridgeEpgImageOrientation::None:
    break;
  }
  return "none";
}

void AppendQuoted(std::string &json, const std::string &value)
{
  json.push_back('"');
  json += EscapeJson(value);
  json.push_back('"');
}

void AppendStringField(
    std::string &json,
    const char *name,
    const std::string &value)
{
  json += ",\"";
  json += name;
  json += "\":";
  AppendQuoted(json, value);
}

void AppendIntField(std::string &json, const char *name, int value)
{
  json += ",\"";
  json += name;
  json += "\":";
  json += std::to_string(value);
}

void AppendBoolField(std::string &json, const char *name, bool value)
{
  json += ",\"";
  json += name;
  json += "\":";
  json += value ? "true" : "false";
}

void AppendFloatField(std::string &json, const char *name, float value)
{
  char buffer[64] = {};
  const int written = std::snprintf(
      buffer,
      sizeof(buffer),
      "%.2f",
      static_cast<double>(value));
  json += ",\"";
  json += name;
  json += "\":";
  if (written > 0 && static_cast<std::size_t>(written) < sizeof(buffer)) {
    json.append(buffer, static_cast<std::size_t>(written));
  } else {
    json += "0.00";
  }
}

void AppendStringArray(
    std::string &json,
    const char *name,
    const std::vector<std::string> &values)
{
  json += ",\"";
  json += name;
  json += "\":[";
  bool first = true;
  for (const std::string &value : values) {
    if (!first) {
      json.push_back(',');
    }
    AppendQuoted(json, value);
    first = false;
  }
  json.push_back(']');
}

void AppendImage(std::string &json, const SuiteBridgeEpgImage &image)
{
  json += "{\"orientation\":\"";
  json += OrientationName(image.orientation);
  json += "\",\"path\":";
  AppendQuoted(json, image.path);
  json += ",\"width\":";
  json += std::to_string(image.width);
  json += ",\"height\":";
  json += std::to_string(image.height);
  json.push_back('}');
}

void AppendImages(
    std::string &json,
    const std::vector<SuiteBridgeEpgImage> &images)
{
  json += ",\"images\":[";
  bool first = true;
  for (const SuiteBridgeEpgImage &image : images) {
    if (!image.Valid()) {
      continue;
    }
    if (!first) {
      json.push_back(',');
    }
    AppendImage(json, image);
    first = false;
  }
  json.push_back(']');
}

void AppendPersons(
    std::string &json,
    const std::vector<SuiteBridgeEpgPerson> &persons)
{
  json += ",\"persons\":[";
  bool first = true;
  for (const SuiteBridgeEpgPerson &person : persons) {
    if (!person.Valid()) {
      continue;
    }
    if (!first) {
      json.push_back(',');
    }

    json += "{\"role\":\"";
    json += PersonRoleName(person.role);
    json += "\",\"name\":";
    AppendQuoted(json, person.name);
    json += ",\"characterName\":";
    AppendQuoted(json, person.characterName);
    json += ",\"image\":";
    if (person.image.Valid()) {
      AppendImage(json, person.image);
    } else {
      json += "null";
    }
    json.push_back('}');
    first = false;
  }
  json.push_back(']');
}

} // namespace

SuiteBridgeEpgMetadataRequest::SuiteBridgeEpgMetadataRequest(
    const char *command,
    const char *option)
{
  handled_ = EqualsIgnoreCase(command, "EPMD");
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
  channelId_.assign(channelStart, static_cast<std::size_t>(cursor - channelStart));

  while (*cursor && std::isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }

  if (channelId_.empty() || !std::isdigit(static_cast<unsigned char>(*cursor))) {
    return;
  }

  unsigned long parsedEventId = 0;
  while (std::isdigit(static_cast<unsigned char>(*cursor))) {
    parsedEventId = parsedEventId * 10UL + static_cast<unsigned long>(*cursor - '0');
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
    const SuiteBridgeEpgMetadata &metadata) noexcept
{
  std::string json = "{\"schema\":1,\"found\":";
  json += metadata.Valid() ? "true" : "false";
  json += ",\"provider\":\"";
  json += metadata.Valid() ? "tvscraper" : "none";
  json.push_back('"');

  if (metadata.Valid()) {
    AppendStringField(json, "mediaType", MediaTypeName(metadata.mediaType));
    AppendIntField(json, "databaseId", metadata.databaseId);
    AppendStringField(json, "title", metadata.title);
    AppendStringField(json, "originalTitle", metadata.originalTitle);
    AppendStringField(json, "episodeTitle", metadata.episodeTitle);
    AppendStringField(json, "tagline", metadata.tagline);
    AppendStringField(json, "overview", metadata.overview);
    AppendStringField(json, "episodeOverview", metadata.episodeOverview);
    AppendStringField(json, "releaseDate", metadata.releaseDate);
    AppendStringField(json, "firstAired", metadata.firstAired);
    AppendStringField(json, "imdbId", metadata.imdbId);
    AppendIntField(json, "collectionId", metadata.collectionId);
    AppendStringField(json, "collectionName", metadata.collectionName);
    AppendStringField(json, "status", metadata.status);
    AppendIntField(json, "runtimeMinutes", metadata.runtimeMinutes);
    AppendIntField(json, "seasonNumber", metadata.seasonNumber);
    AppendIntField(json, "episodeNumber", metadata.episodeNumber);
    AppendIntField(
        json,
        "absoluteEpisodeNumber",
        metadata.absoluteEpisodeNumber);
    AppendIntField(json, "lastSeason", metadata.lastSeason);
    AppendBoolField(json, "adult", metadata.adult);
    AppendFloatField(json, "voteAverage", metadata.voteAverage);
    AppendIntField(json, "voteCount", metadata.voteCount);
    AppendStringArray(json, "genres", metadata.genres);
    AppendStringArray(
        json,
        "productionCountries",
        metadata.productionCountries);
    AppendStringArray(json, "networks", metadata.networks);
    AppendPersons(json, metadata.persons);
    AppendImages(json, metadata.images);
  }

  json.push_back('}');
  if (json.size() >= data_.size()) {
    size_ = data_.size() - 1;
    std::memcpy(data_.data(), json.data(), size_);
    data_[size_] = '\0';
    return;
  }

  size_ = json.size();
  std::memcpy(data_.data(), json.data(), size_);
  data_[size_] = '\0';
  complete_ = true;
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
