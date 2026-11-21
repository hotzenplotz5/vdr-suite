#include "suitebridge_recording_identity.h"

#include <cstdint>
#include <iomanip>
#include <sstream>

namespace {

std::uint64_t Fnv1a64(
    const std::string &value,
    std::uint64_t hash) noexcept
{
  for (const unsigned char character : value) {
    hash ^= static_cast<std::uint64_t>(character);
    hash *= 1099511628211ULL;
  }
  return hash;
}

bool ValidNativeId(const std::string &nativeId) noexcept
{
  if (nativeId.empty() ||
      nativeId.size() > SuiteBridgeRecordingIdentity::kMaximumNativeIdBytes) {
    return false;
  }

  for (const unsigned char character : nativeId) {
    if (character == 0 || character == '\r' || character == '\n') {
      return false;
    }
  }

  return true;
}

} // namespace

std::string SuiteBridgeRecordingIdentity::KeyForNativeId(
    const std::string &nativeId)
{
  if (!ValidNativeId(nativeId)) {
    return {};
  }

  const std::string input =
      std::string("vdr-suite-recording-native-v1\n") + nativeId;
  const std::uint64_t first =
      Fnv1a64(input, 14695981039346656037ULL);
  const std::uint64_t second =
      Fnv1a64(input, 7809847782465536322ULL);

  std::ostringstream key;
  key << std::hex << std::nouppercase << std::setfill('0')
      << std::setw(16) << first
      << std::setw(16) << second;
  return key.str();
}

bool SuiteBridgeRecordingIdentity::IsValidKey(
    const std::string &key) noexcept
{
  if (key.size() != kKeyBytes) {
    return false;
  }

  for (const unsigned char character : key) {
    if (!((character >= '0' && character <= '9') ||
          (character >= 'a' && character <= 'f'))) {
      return false;
    }
  }

  return true;
}
