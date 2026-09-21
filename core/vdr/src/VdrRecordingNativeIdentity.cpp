#include "VdrRecordingNativeIdentity.h"

#include <cstdint>
#include <iomanip>
#include <sstream>

namespace
{

std::uint64_t fnv1a64(
    const std::string& value,
    std::uint64_t hash) noexcept
{
    for (const unsigned char character : value)
    {
        hash ^= static_cast<std::uint64_t>(character);
        hash *= 1099511628211ULL;
    }
    return hash;
}

bool validNativeId(const std::string& nativeId) noexcept
{
    if (nativeId.empty() ||
        nativeId.size() > VdrRecordingNativeIdentity::MaximumNativeIdBytes)
    {
        return false;
    }

    for (const unsigned char character : nativeId)
    {
        if (character == 0 || character == '\r' || character == '\n')
        {
            return false;
        }
    }

    return true;
}

}

std::string VdrRecordingNativeIdentity::keyForNativeId(
    const std::string& nativeId)
{
    if (!validNativeId(nativeId))
    {
        return {};
    }

    const std::string input =
        std::string("vdr-suite-recording-native-v1\n") + nativeId;
    const std::uint64_t first =
        fnv1a64(input, 14695981039346656037ULL);
    const std::uint64_t second =
        fnv1a64(input, 7809847782465536322ULL);

    std::ostringstream key;
    key << std::hex << std::nouppercase << std::setfill('0')
        << std::setw(16) << first
        << std::setw(16) << second;
    return key.str();
}

bool VdrRecordingNativeIdentity::isValidKey(
    const std::string& key) noexcept
{
    if (key.size() != KeyBytes)
    {
        return false;
    }

    for (const unsigned char character : key)
    {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f')))
        {
            return false;
        }
    }

    return true;
}
