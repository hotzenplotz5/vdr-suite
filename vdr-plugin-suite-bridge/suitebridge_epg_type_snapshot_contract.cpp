#include "suitebridge_epg_type_snapshot_contract.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>

namespace {

bool ParseUnsignedLongLong(
    const std::string &value,
    unsigned long long &parsed)
{
  if (value.empty() ||
      !std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isdigit(character) != 0;
      })) {
    return false;
  }

  try {
    std::size_t consumed = 0;
    parsed = std::stoull(value, &consumed, 10);
    return consumed == value.size();
  } catch (...) {
    return false;
  }
}

bool SafeChannelId(const std::string &value)
{
  if (value.empty() || value.size() > 255) {
    return false;
  }

  return std::all_of(value.begin(), value.end(), [](unsigned char character) {
    return character > 0x20 && character != 0x7f &&
        character != ',' && character != ';' && character != '|';
  });
}

char MediaTypeToken(SuiteBridgeEpgMediaType type)
{
  switch (type) {
  case SuiteBridgeEpgMediaType::Series:
    return 'S';
  case SuiteBridgeEpgMediaType::Movie:
    return 'M';
  case SuiteBridgeEpgMediaType::None:
    break;
  }
  return 'N';
}

} // namespace

SuiteBridgeEpgTypeSnapshotRequest::SuiteBridgeEpgTypeSnapshotRequest(
    const char *command,
    const char *option)
{
  handled_ = command != nullptr && std::strcmp(command, "ETYPES") == 0;
  if (!handled_ || option == nullptr) {
    return;
  }

  std::istringstream input(option);
  std::string fromValue;
  std::string untilValue;
  std::string offsetValue;
  std::string limitValue;
  std::string extra;
  if (!(input >> fromValue >> untilValue >> offsetValue >> limitValue) ||
      (input >> extra)) {
    return;
  }

  unsigned long long from = 0;
  unsigned long long until = 0;
  unsigned long long offset = 0;
  unsigned long long limit = 0;
  if (!ParseUnsignedLongLong(fromValue, from) ||
      !ParseUnsignedLongLong(untilValue, until) ||
      !ParseUnsignedLongLong(offsetValue, offset) ||
      !ParseUnsignedLongLong(limitValue, limit) ||
      from > static_cast<unsigned long long>(
          std::numeric_limits<std::int64_t>::max()) ||
      until > static_cast<unsigned long long>(
          std::numeric_limits<std::int64_t>::max()) ||
      until <= from ||
      until - from > 72ULL * 60ULL * 60ULL ||
      limit == 0 || limit > 64 ||
      offset > 1000000ULL) {
    return;
  }

  fromTime_ = static_cast<std::int64_t>(from);
  untilTime_ = static_cast<std::int64_t>(until);
  offset_ = static_cast<std::uint64_t>(offset);
  limit_ = static_cast<std::size_t>(limit);
  valid_ = true;
}

bool SuiteBridgeEpgTypeSnapshotRequest::Handled() const noexcept
{
  return handled_;
}

bool SuiteBridgeEpgTypeSnapshotRequest::Valid() const noexcept
{
  return valid_;
}

std::int64_t SuiteBridgeEpgTypeSnapshotRequest::FromTime() const noexcept
{
  return fromTime_;
}

std::int64_t SuiteBridgeEpgTypeSnapshotRequest::UntilTime() const noexcept
{
  return untilTime_;
}

std::uint64_t SuiteBridgeEpgTypeSnapshotRequest::Offset() const noexcept
{
  return offset_;
}

std::size_t SuiteBridgeEpgTypeSnapshotRequest::Limit() const noexcept
{
  return limit_;
}

SuiteBridgeEpgTypeSnapshotPayload::SuiteBridgeEpgTypeSnapshotPayload(
    const SuiteBridgeEpgTypeSnapshotPage &page)
{
  std::string payload = "1|" + std::to_string(page.nextOffset) + "|" +
      std::to_string(page.scanned) + "|" + (page.done ? "1|" : "0|");

  bool first = true;
  for (const SuiteBridgeEpgTypeSnapshotItem &item : page.items) {
    if (!SafeChannelId(item.channelId) ||
        item.eventId == 0 ||
        item.startTime <= 0 ||
        item.endTime <= item.startTime ||
        item.mediaType == SuiteBridgeEpgMediaType::None) {
      complete_ = false;
      return;
    }

    if (!first) {
      payload.push_back(';');
    }
    first = false;
    payload += item.channelId;
    payload.push_back(',');
    payload += std::to_string(item.eventId);
    payload.push_back(',');
    payload += std::to_string(item.startTime);
    payload.push_back(',');
    payload += std::to_string(item.endTime);
    payload.push_back(',');
    payload.push_back(MediaTypeToken(item.mediaType));
  }

  if (payload.size() >= data_.size()) {
    complete_ = false;
    return;
  }

  std::copy(payload.begin(), payload.end(), data_.begin());
  data_[payload.size()] = '\0';
  size_ = payload.size();
  complete_ = true;
}

const char *SuiteBridgeEpgTypeSnapshotPayload::Data() const noexcept
{
  return data_.data();
}

std::size_t SuiteBridgeEpgTypeSnapshotPayload::Size() const noexcept
{
  return size_;
}

bool SuiteBridgeEpgTypeSnapshotPayload::Complete() const noexcept
{
  return complete_;
}
