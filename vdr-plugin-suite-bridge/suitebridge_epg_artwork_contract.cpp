#include "suitebridge_epg_artwork_contract.h"

#include <cctype>
#include <cstdio>
#include <cstring>

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

} // namespace

SuiteBridgeEpgArtworkRequest::SuiteBridgeEpgArtworkRequest(
    const char *command,
    const char *option)
{
  handled_ = EqualsIgnoreCase(command, "ARTW");
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

bool SuiteBridgeEpgArtworkRequest::Handled() const noexcept
{
  return handled_;
}

bool SuiteBridgeEpgArtworkRequest::Valid() const noexcept
{
  return valid_;
}

const std::string &SuiteBridgeEpgArtworkRequest::ChannelId() const noexcept
{
  return channelId_;
}

unsigned int SuiteBridgeEpgArtworkRequest::EventId() const noexcept
{
  return eventId_;
}

SuiteBridgeEpgArtworkPayload::SuiteBridgeEpgArtworkPayload(
    const SuiteBridgeArtworkReference &artwork) noexcept
{
  const std::string escapedPath = EscapeJson(artwork.path);
  const char *provider = artwork.Valid() ? "tvscraper" : "none";

  const int written = std::snprintf(
      data_.data(),
      data_.size(),
      "{\"schema\":1,\"found\":%s,\"provider\":\"%s\",\"path\":\"%s\",\"width\":%d,\"height\":%d}",
      artwork.Valid() ? "true" : "false",
      provider,
      escapedPath.c_str(),
      artwork.Valid() ? artwork.width : 0,
      artwork.Valid() ? artwork.height : 0);

  if (written < 0) {
    data_[0] = '\0';
    return;
  }

  size_ = static_cast<std::size_t>(written);
  complete_ = size_ < data_.size();
  if (!complete_) {
    size_ = data_.size() - 1;
    data_[size_] = '\0';
  }
}

const char *SuiteBridgeEpgArtworkPayload::Data() const noexcept
{
  return data_.data();
}

std::size_t SuiteBridgeEpgArtworkPayload::Size() const noexcept
{
  return size_;
}

bool SuiteBridgeEpgArtworkPayload::Complete() const noexcept
{
  return complete_;
}
