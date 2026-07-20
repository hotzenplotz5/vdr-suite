#include "suitebridge_image_dimensions.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <vector>

namespace {

constexpr std::size_t MaximumHeaderBytes = 1024U * 1024U;

std::uint16_t ReadBigEndian16(
    const std::vector<unsigned char> &data,
    const std::size_t offset) noexcept
{
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(data[offset]) << 8U) |
      static_cast<std::uint16_t>(data[offset + 1U]));
}

std::uint32_t ReadBigEndian32(
    const std::vector<unsigned char> &data,
    const std::size_t offset) noexcept
{
  return
      (static_cast<std::uint32_t>(data[offset]) << 24U) |
      (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
      (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
      static_cast<std::uint32_t>(data[offset + 3U]);
}

bool AssignDimensions(
    const std::uint32_t width,
    const std::uint32_t height,
    SuiteBridgeImageDimensions &dimensions) noexcept
{
  if (width == 0U || height == 0U ||
      width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
      height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
    return false;
  }

  dimensions.width = static_cast<int>(width);
  dimensions.height = static_cast<int>(height);
  return true;
}

bool ParsePng(
    const std::vector<unsigned char> &data,
    SuiteBridgeImageDimensions &dimensions) noexcept
{
  static constexpr std::array<unsigned char, 8> Signature = {
      0x89U, 0x50U, 0x4eU, 0x47U, 0x0dU, 0x0aU, 0x1aU, 0x0aU};

  if (data.size() < 24U ||
      !std::equal(Signature.begin(), Signature.end(), data.begin()) ||
      data[12] != 'I' || data[13] != 'H' ||
      data[14] != 'D' || data[15] != 'R') {
    return false;
  }

  return AssignDimensions(
      ReadBigEndian32(data, 16U),
      ReadBigEndian32(data, 20U),
      dimensions);
}

bool IsStartOfFrame(const unsigned char marker) noexcept
{
  switch (marker) {
  case 0xc0U: case 0xc1U: case 0xc2U: case 0xc3U:
  case 0xc5U: case 0xc6U: case 0xc7U:
  case 0xc9U: case 0xcaU: case 0xcbU:
  case 0xcdU: case 0xceU: case 0xcfU:
    return true;
  default:
    return false;
  }
}

bool ParseJpeg(
    const std::vector<unsigned char> &data,
    SuiteBridgeImageDimensions &dimensions) noexcept
{
  if (data.size() < 4U || data[0] != 0xffU || data[1] != 0xd8U) {
    return false;
  }

  std::size_t offset = 2U;
  while (offset < data.size()) {
    while (offset < data.size() && data[offset] != 0xffU) {
      ++offset;
    }
    while (offset < data.size() && data[offset] == 0xffU) {
      ++offset;
    }
    if (offset >= data.size()) {
      return false;
    }

    const unsigned char marker = data[offset++];
    if (marker == 0xd9U || marker == 0xdaU) {
      return false;
    }
    if (marker == 0x01U || (marker >= 0xd0U && marker <= 0xd8U)) {
      continue;
    }
    if (offset + 2U > data.size()) {
      return false;
    }

    const std::uint16_t segmentLength = ReadBigEndian16(data, offset);
    if (segmentLength < 2U ||
        offset + static_cast<std::size_t>(segmentLength) > data.size()) {
      return false;
    }

    if (IsStartOfFrame(marker)) {
      if (segmentLength < 7U) {
        return false;
      }
      return AssignDimensions(
          ReadBigEndian16(data, offset + 5U),
          ReadBigEndian16(data, offset + 3U),
          dimensions);
    }

    offset += static_cast<std::size_t>(segmentLength);
  }

  return false;
}

} // namespace

bool SuiteBridgeReadImageDimensions(
    const std::string &path,
    SuiteBridgeImageDimensions &dimensions) noexcept
{
  dimensions = {};
  if (path.empty()) {
    return false;
  }

  try {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
      return false;
    }

    std::vector<unsigned char> data(MaximumHeaderBytes);
    input.read(
        reinterpret_cast<char *>(data.data()),
        static_cast<std::streamsize>(data.size()));
    data.resize(static_cast<std::size_t>(input.gcount()));

    return ParsePng(data, dimensions) || ParseJpeg(data, dimensions);
  } catch (...) {
    dimensions = {};
    return false;
  }
}
